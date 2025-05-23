//
// Created by wyk on 2025/2/15.
//

#pragma once
#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "core/execute_unit/execute_unit.h"
#include "core/reg_unit/reg_unit.h"
#include "data_path_manager.h"
#include "isa/inst_v1.h"
#include "isa/inst_v2.h"
#include "isa/inst_v3.h"
#include "util/ins_stat.h"

namespace cimsim {

template <class Inst>
class Decoder : public BaseModule {
public:
    using Instruction = Inst;

    Decoder(const sc_module_name& name, const CoreConfig& core_config, const BaseInfo& base_info)
        : BaseModule(name, base_info)
        , as_(AddressSapce::getInstance())
        , simd_unit_config_(core_config.simd_unit_config)
        , reduce_unit_config_(core_config.reduce_unit_config)
        , data_path_manager_(core_config.transfer_unit_config.local_dedicated_data_path_list) {
        for (const auto& ins_config : simd_unit_config_.instruction_list) {
            simd_ins_config_map_.emplace(getSIMDInstructionIdentityCode(ins_config.input_cnt, ins_config.opcode),
                                         &ins_config);
        }
        for (const auto& functor_config : simd_unit_config_.functor_list) {
            simd_func_config_map_.emplace(functor_config.name, &functor_config);
        }
        for (const auto& functor_config : reduce_unit_config_.functor_list) {
            reduce_func_config_map_.emplace(functor_config.funct, &functor_config);
        }
    }

    void bindRegUnit(RegUnit* reg_unit) {
        this->reg_unit_ = reg_unit;
    }
    void bindExecuteUnit(ExecuteUnitType type, ExecuteUnit* execute) {
        this->execute_unit_map_.insert({type._to_string(), execute});
    }

    // bool checkInsStat(const std::string& expected_ins_stat_file) const;

    virtual std::shared_ptr<ExecuteInsPayload> decode(const Inst& ins, int pc, int& pc_increment,
                                                      ResourceAllocatePayload& conflict_info) = 0;

protected:
    static unsigned int getSIMDInstructionIdentityCode(unsigned int input_cnt, unsigned int opcode) {
        return ((input_cnt << SIMD_INSTRUCTION_OPCODE_BIT_LENGTH) | opcode);
    }

    std::pair<const SIMDInstructionConfig*, const SIMDFunctorConfig*> getSIMDInstructionAndFunctor(
        unsigned int input_cnt, unsigned int opcode, const SIMDInputsArray& inputs_bit_width) const {
        auto ins_found = simd_ins_config_map_.find(getSIMDInstructionIdentityCode(input_cnt, opcode));
        if (ins_found == simd_ins_config_map_.end()) {
            return {nullptr, nullptr};
        }

        auto* ins_config = ins_found->second;
        for (const auto& bind_info : ins_config->functor_binding_list) {
            if (bind_info.input_bit_width.inputs == inputs_bit_width) {
                if (auto functor_found = simd_func_config_map_.find(bind_info.functor_name);
                    functor_found != simd_func_config_map_.end()) {
                    if (auto* functor_config = functor_found->second;
                        input_cnt == functor_config->input_cnt &&
                        inputs_bit_width == functor_config->data_bit_width.inputs) {
                        return {ins_config, functor_config};
                    }
                }
            }
        }

        return {ins_config, nullptr};
    }

    const ReduceFunctorConfig* getReduceFunctor(unsigned int funct, int input_bit_width, int output_bit_width) const {
        auto found = reduce_func_config_map_.find(funct);
        if (found == reduce_func_config_map_.end()) {
            return nullptr;
        }

        if (auto* func_config = found->second;
            func_config->input_bit_width == input_bit_width && func_config->output_bit_width == output_bit_width) {
            return func_config;
        }
        return nullptr;
    }

private:
    virtual std::shared_ptr<ExecuteInsPayload> decodeCimIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeVectorIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const Inst& ins) const = 0;
    virtual int decodeControlInsAndGetPCIncrement(const Inst& ins) const = 0;

protected:
    const AddressSapce& as_;
    const SIMDUnitConfig& simd_unit_config_;
    const ReduceUnitConfig& reduce_unit_config_;
    const DataPathManager data_path_manager_;

    int ins_id_{0};
    // InsStat ins_stat_{};

    RegUnit* reg_unit_{};
    std::unordered_map<std::string, ExecuteUnit*> execute_unit_map_;

private:
    std::unordered_map<unsigned int, const SIMDInstructionConfig*> simd_ins_config_map_;
    std::unordered_map<std::string, const SIMDFunctorConfig*> simd_func_config_map_;

    std::unordered_map<unsigned int, const ReduceFunctorConfig*> reduce_func_config_map_;
};

class DecoderV1 : public Decoder<InstV1> {
public:
    using Decoder::Decoder;

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    std::shared_ptr<ExecuteInsPayload> decode(const InstV1& ins, int pc, int& pc_increment,
                                              ResourceAllocatePayload& conflict_info) override;

private:
    std::shared_ptr<ExecuteInsPayload> decodeCimIns(const InstV1& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeVectorIns(const InstV1& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const InstV1& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const InstV1& ins) const override;
    int decodeControlInsAndGetPCIncrement(const InstV1& ins) const override;

    static ScalarOperator decodeScalarRROpcode(int opcode);
    static ScalarOperator decodeScalarRIOpcode(int opcode);

private:
    InsStat ins_stat_{};
};

class DecoderV2 : public Decoder<InstV2> {
public:
    using Decoder::Decoder;
    std::shared_ptr<ExecuteInsPayload> decode(const InstV2& ins, int pc, int& pc_increment,
                                              ResourceAllocatePayload& conflict_info) override;

private:
    std::shared_ptr<ExecuteInsPayload> decodeCimIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeVectorIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const InstV2& ins) const override;
    int decodeControlInsAndGetPCIncrement(const InstV2& ins) const override;

    std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const InstV2& ins) const;
    std::shared_ptr<ExecuteInsPayload> decodeReduceIns(const InstV2& ins) const;
};

class DecoderV3 : public Decoder<InstV3> {
public:
    using Decoder::Decoder;
    std::shared_ptr<ExecuteInsPayload> decode(const InstV3& ins, int pc, int& pc_increment,
                                              ResourceAllocatePayload& conflict_info) override;

private:
    std::shared_ptr<ExecuteInsPayload> decodeCimIns(const InstV3& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeVectorIns(const InstV3& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const InstV3& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const InstV3& ins) const override;
    int decodeControlInsAndGetPCIncrement(const InstV3& ins) const override;

private:
    struct FLAG_POSITION {
        enum {
            CIM_MVM_GRP = 5,
            CIM_MVM_GRP_I = 4,
            CIM_MVM_SP_V = 3,
            CIM_MVM_SP_B = 2,
            CIM_CFG_GRP_B = 15,
            CIM_OUT_OSUM = 5,
            CIM_OUT_OSUM_MOV = 4,
            MEM_CPY_SRC_MASK = 27,
            MEM_CPY_DST_MASK = 26
        };
    };
};

}  // namespace cimsim
