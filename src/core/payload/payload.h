//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <array>
#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config/config.h"
#include "isa/isa.h"
#include "payload_enum.h"
#include "systemc.h"
#include "util/macro_scope.h"

namespace pimsim {

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr);

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set);

std::stringstream& operator<<(std::stringstream& out, const std::vector<int>& list);

std::stringstream& operator<<(std::stringstream& out, const std::unordered_map<int, int>& map);

struct InstructionPayload {
    int pc{-1};
    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};

    [[nodiscard]] bool valid() const;

    void clear();

    friend std::ostream& operator<<(std::ostream& out, const InstructionPayload& ins) {
        out << "pc: " << ins.pc << ", ins id: " << ins.ins_id << ", unit type: " << ins.unit_type << "\n";
        return out;
    }

    bool operator==(const InstructionPayload& another) const {
        return pc == another.pc && ins_id == another.ins_id && unit_type == another.unit_type;
    }

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(InstructionPayload)
};

struct MemoryAccessPayload {
    InstructionPayload ins{};

    MemoryAccessType access_type;
    int address_byte;  // byte
    int size_byte;     // byte
    std::vector<uint8_t> data;
    sc_core::sc_event& finish_access;
};

struct DataConflictPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(DataConflictPayload)

    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};

    std::unordered_set<int> read_memory_id;
    std::unordered_set<int> write_memory_id;
    std::unordered_set<int> used_memory_id;

    DECLARE_PIM_PAYLOAD_FUNCTIONS(DataConflictPayload)

    void addReadMemoryId(int memory_id);
    void addReadMemoryId(const std::initializer_list<int>& memory_id_list);
    void addWriteMemoryId(int memory_id);
    void addReadWriteMemoryId(int memory_id);

    static bool checkDataConflict(const DataConflictPayload& ins_conflict_payload,
                                  const DataConflictPayload& unit_conflict_payload);

    DataConflictPayload& operator+=(const DataConflictPayload& other);
};

struct SIMDInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(SIMDInsPayload)

    InstructionPayload ins{};

    // compute type info
    unsigned int input_cnt{2};
    unsigned int opcode{0x00};

    // data width info
    std::array<int, SIMD_MAX_INPUT_NUM> inputs_bit_width{0, 0, 0, 0};
    int output_bit_width{0};

    // data address info
    std::array<int, SIMD_MAX_INPUT_NUM> inputs_address_byte{0, 0, 0, 0};
    int output_address_byte{0};

    // vector length info
    int len{0};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInsPayload)
};

struct TransferInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(TransferInsPayload)

    InstructionPayload ins{};

    TransferType type{TransferType::local_trans};

    int src_address_byte{0};
    int dst_address_byte{0};
    int size_byte{0};

    int src_id{0};
    int dst_id{0};
    int transfer_id_tag{0};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(TransferInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TransferInsPayload)
};

struct ScalarInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ScalarInsPayload)

    InstructionPayload ins{};

    ScalarOperator op{};

    int src1_value{0}, src2_value{0}, offset{0};
    int dst_reg{0};

    bool write_special_register{false};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(ScalarInsPayload)
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarInsPayload)
};

struct PimComputeInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(PimComputeInsPayload)

    InstructionPayload ins{};

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

    DECLARE_PIM_PAYLOAD_FUNCTIONS(PimComputeInsPayload)
};

struct PimControlInsPayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(PimControlInsPayload)

    InstructionPayload ins{};

    PimControlOperator op{PimControlOperator::set_activation};

    // set activation
    bool group_broadcast{false};
    int group_id{0};
    int mask_addr_byte{0};

    // output result
    int activation_group_num{0};
    int output_addr_byte{0}, output_cnt_per_group{0}, output_bit_width{0}, output_mask_addr_byte{0};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(PimControlInsPayload);
};

struct RegUnitReadRequest {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(RegUnitReadRequest)

    int rs1_id{0}, rs2_id{0}, rs3_id{0}, rs4_id{0}, rd_id{0};
    bool rs1_read_double{false}, rs2_read_double{false};

    std::vector<int> special_reg_ids{};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(RegUnitReadRequest)
};

struct RegUnitReadResponse {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(RegUnitReadResponse)

    int rs1_value{0}, rs2_value{0}, rs3_value{0}, rs4_value{0}, rd_value{0};
    int rs1_double_value{0}, rs2_double_value{0};

    std::unordered_map<int, int> special_reg_values{};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(RegUnitReadResponse)
};

struct RegUnitWriteRequest {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(RegUnitWriteRequest)

    int reg_id{0}, reg_value{0};
    bool write_special_register{false};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(RegUnitWriteRequest)
};

}  // namespace pimsim
