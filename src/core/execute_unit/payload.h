//
// Created by wyk on 2025/3/10.
//

#pragma once
#include "better-enums/enum.h"
#include "config/config.h"
#include "core/payload.h"
#include "systemc.h"
#include "util/macro_scope.h"

namespace cimsim {

BETTER_ENUM(ScalarOperator, int,  // NOLINT(*-explicit-constructor)
            add = 0, sub, mul, div, sll, srl, sra, mod, min, max, s_and, s_or, eq, ne, gt, lt, lui, load, store, assign)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(ScalarOperator)

BETTER_ENUM(CimControlOperator, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            set_activation = 0, only_output, output_sum, output_sum_move)

BETTER_ENUM(TransferType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            local_trans = 0, global_load, global_store, send, receive)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(TransferType)

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr);

std::stringstream& operator<<(std::stringstream& out, const std::vector<int>& list);

std::stringstream& operator<<(std::stringstream& out, const std::unordered_map<int, int>& map);

struct ExecuteInsPayload {
    ExecuteInsPayload() = default;
    explicit ExecuteInsPayload(InstructionPayload ins) : ins(ins) {}
    virtual ~ExecuteInsPayload() = default;
    InstructionPayload ins{};
};

struct ExecuteUnitPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ExecuteUnitPayload)

    std::shared_ptr<ExecuteInsPayload> payload{nullptr};

    ExecuteUnitPayload() = default;
    bool operator==(const ExecuteUnitPayload& another) const {
        if (payload == nullptr || another.payload == nullptr) {
            return payload == another.payload;
        }
        return payload->ins == another.payload->ins;
    }
};

struct SIMDInsPayload : public ExecuteInsPayload {
    // compute ins and functor info
    const SIMDInstructionConfig* ins_cfg{nullptr};
    const SIMDFunctorConfig* func_cfg{nullptr};

    // data width info
    SIMDInputsArray inputs_bit_width{0, 0, 0, 0};
    int output_bit_width{0};

    // data address info
    SIMDInputsArray inputs_address_byte{0, 0, 0, 0};
    int output_address_byte{0};

    // vector length info
    int len{0};

    DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, ins_cfg, func_cfg, inputs_bit_width, output_bit_width,
                                         inputs_address_byte, output_address_byte, len)
};

struct TransferInsPayload : public ExecuteInsPayload {
    TransferType type{TransferType::local_trans};

    int src_address_byte{0};
    int dst_address_byte{0};
    int size_byte{0};

    int src_id{0};
    int dst_id{0};
    int transfer_id_tag{0};

    DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(TransferInsPayload, ins, type, src_address_byte, dst_address_byte, size_byte,
                                         src_id, dst_id, transfer_id_tag)
};

struct ScalarInsPayload : public ExecuteInsPayload {
    ScalarOperator op{};

    int src1_value{0}, src2_value{0}, offset{0};
    int dst_reg{0};

    bool write_special_register{false};

    DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(ScalarInsPayload, ins, op, src1_value, src2_value, offset, dst_reg,
                                         write_special_register)
};

struct CimComputeInsPayload : public ExecuteInsPayload {
    // input info
    int input_addr_byte{0}, input_len{0}, input_bit_width{0};

    // group info
    int activation_group_num{0};
    int group_input_step_byte{0};

    // macro info
    int row{0};

    // bit sparse
    bool bit_sparse{false};
    int bit_sparse_meta_addr_byte{0};

    // value sparse
    bool value_sparse{false};
    int value_sparse_mask_addr_byte{0};

    DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(CimComputeInsPayload, ins, input_addr_byte, input_len, input_bit_width,
                                         activation_group_num, group_input_step_byte, row, bit_sparse,
                                         bit_sparse_meta_addr_byte, value_sparse, value_sparse_mask_addr_byte)
};

struct CimControlInsPayload : public ExecuteInsPayload {
    CimControlOperator op{CimControlOperator::set_activation};

    // set activation
    bool group_broadcast{false};
    int group_id{0};
    int mask_addr_byte{0};

    // output result
    int activation_group_num{0};
    int output_addr_byte{0}, output_cnt_per_group{0}, output_bit_width{0}, output_mask_addr_byte{0};

    DEFINE_EXECUTE_INS_PAYLOAD_FUNCTIONS(CimControlInsPayload, ins, op, group_broadcast, group_id, mask_addr_byte,
                                         activation_group_num, output_addr_byte, output_cnt_per_group, output_bit_width,
                                         output_mask_addr_byte)
};

}  // namespace cimsim
