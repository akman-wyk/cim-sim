//
// Created by wyk on 2025/2/15.
//

#pragma once
#include "base_component/base_module.h"
#include "core/execute_unit/execute_unit.h"
#include "core/reg_unit/reg_unit.h"
#include "isa/inst_v1.h"
#include "isa/inst_v2.h"
#include "isa/inst_v3.h"
#include "util/ins_stat.h"

namespace pimsim {

template <class Inst>
class Decoder : public BaseModule {
public:
    using Instruction = Inst;

    Decoder(const char* name, const ChipConfig& chip_config, const SimConfig& sim_config, Core* core, Clock* clk)
        : BaseModule(name, sim_config, core, clk)
        , global_as_(chip_config.global_memory_config.addressing)
        , simd_unit_config_(chip_config.core_config.simd_unit_config) {}

    void bindRegUnit(RegUnit* reg_unit) {
        this->reg_unit_ = reg_unit;
    }
    void bindExecuteUnit(ExecuteUnitType type, ExecuteUnit* execute) {
        this->execute_unit_map_.insert({type._to_string(), execute});
    }

    // bool checkInsStat(const std::string& expected_ins_stat_file) const;

    virtual std::shared_ptr<ExecuteInsPayload> decode(const Inst& ins, int pc, int& pc_increment,
                                                      ResourceAllocatePayload& conflict_info) = 0;

private:
    virtual std::shared_ptr<ExecuteInsPayload> decodePimIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const Inst& ins) const = 0;
    virtual std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const Inst& ins) const = 0;
    virtual int decodeControlInsAndGetPCIncrement(const Inst& ins) const = 0;

protected:
    const AddressSpaceConfig& global_as_;
    const SIMDUnitConfig& simd_unit_config_;

    int ins_id_{0};
    // InsStat ins_stat_{};

    RegUnit* reg_unit_{};
    std::unordered_map<std::string, ExecuteUnit*> execute_unit_map_;
};

class DecoderV1 : public Decoder<InstV1> {
public:
    using Decoder::Decoder;

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    std::shared_ptr<ExecuteInsPayload> decode(const InstV1& ins, int pc, int& pc_increment,
                                              ResourceAllocatePayload& conflict_info) override;

private:
    std::shared_ptr<ExecuteInsPayload> decodePimIns(const InstV1& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const InstV1& ins) const override;
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
    std::shared_ptr<ExecuteInsPayload> decodePimIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const InstV2& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const InstV2& ins) const override;
    int decodeControlInsAndGetPCIncrement(const InstV2& ins) const override;
};

class DecoderV3 : public Decoder<InstV3> {
public:
    using Decoder::Decoder;
    std::shared_ptr<ExecuteInsPayload> decode(const InstV3& ins, int pc, int& pc_increment,
                                              ResourceAllocatePayload& conflict_info) override;

private:
    std::shared_ptr<ExecuteInsPayload> decodePimIns(const InstV3& ins) const override;
    std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const InstV3& ins) const override;
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

}  // namespace pimsim
