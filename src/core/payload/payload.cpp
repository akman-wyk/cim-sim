//
// Created by wyk on 2024/7/8.
//

#include "payload.h"

#include "util/util.h"

namespace pimsim {

bool InstructionPayload::valid() const {
    return pc != -1 && ins_id != -1;
}

void InstructionPayload::clear() {
    pc = -1;
    ins_id = -1;
}

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr) {
    out << arr[0];
    for (int i = 1; i < arr.size(); i++) {
        out << ", " << arr[i];
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set) {
    for (auto it = set.begin(); it != set.end(); ++it) {
        if (it != set.begin()) {
            out << ", ";
        }
        out << *it;
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::vector<int>& list) {
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it != list.begin()) {
            out << ", ";
        }
        out << *it;
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::unordered_map<int, int>& map) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) {
            out << ", ";
        }
        out << it->first << ": " << it->second;
    }
    return out;
}

void DataConflictPayload::addReadMemoryId(int memory_id) {
    read_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

void DataConflictPayload::addReadMemoryId(const std::initializer_list<int>& memory_id_list) {
    read_memory_id.insert(memory_id_list);
    used_memory_id.insert(memory_id_list);
}

void DataConflictPayload::addWriteMemoryId(int memory_id) {
    write_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

void DataConflictPayload::addReadWriteMemoryId(int memory_id) {
    read_memory_id.insert(memory_id);
    write_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

bool DataConflictPayload::checkDataConflict(const DataConflictPayload& ins_conflict_payload,
                                            const DataConflictPayload& unit_conflict_payload) {
    if (ins_conflict_payload.unit_type == unit_conflict_payload.unit_type) {
        return SetsIntersection(unit_conflict_payload.write_memory_id, ins_conflict_payload.read_memory_id);
    }
    return SetsIntersection(unit_conflict_payload.used_memory_id, ins_conflict_payload.used_memory_id);
}

DataConflictPayload& DataConflictPayload::operator+=(const pimsim::DataConflictPayload& other) {
    this->read_memory_id.insert(other.read_memory_id.begin(), other.read_memory_id.end());
    this->write_memory_id.insert(other.write_memory_id.begin(), other.write_memory_id.end());
    this->used_memory_id.insert(other.used_memory_id.begin(), other.used_memory_id.end());
    this->unit_type = other.unit_type;
    return *this;
}

DEFINE_PIM_PAYLOAD_FUNCTIONS(DataConflictPayload, ins_id, unit_type, read_memory_id, write_memory_id, used_memory_id)

DEFINE_PIM_PAYLOAD_FUNCTIONS(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width, output_bit_width,
                             inputs_address_byte, output_address_byte, len)

DEFINE_PIM_PAYLOAD_FUNCTIONS(TransferInsPayload, ins, type, src_address_byte, dst_address_byte, size_byte, src_id,
                             dst_id, transfer_id_tag)

DEFINE_PIM_PAYLOAD_FUNCTIONS(ScalarInsPayload, ins, op, src1_value, src2_value, offset, dst_reg, write_special_register)

DEFINE_PIM_PAYLOAD_FUNCTIONS(PimComputeInsPayload, ins, input_addr_byte, input_len, input_bit_width,
                             activation_group_num, group_input_step_byte, row, bit_sparse, bit_sparse_meta_addr_byte,
                             value_sparse, value_sparse_mask_addr_byte)

DEFINE_PIM_PAYLOAD_FUNCTIONS(PimControlInsPayload, ins, op, group_broadcast, group_id, mask_addr_byte,
                             activation_group_num, output_addr_byte, output_cnt_per_group, output_bit_width,
                             output_mask_addr_byte)

DEFINE_PIM_PAYLOAD_FUNCTIONS(RegUnitReadRequest, rs1_id, rs2_id, rs3_id, rs4_id, rd_id, rs1_read_double,
                             rs2_read_double, special_reg_ids)

DEFINE_PIM_PAYLOAD_FUNCTIONS(RegUnitReadResponse, rs1_value, rs2_value, rs3_value, rs4_value, rd_value,
                             rs1_double_value, rs2_double_value, special_reg_values)

DEFINE_PIM_PAYLOAD_FUNCTIONS(RegUnitWriteRequest, reg_id, reg_value, write_special_register)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(InstructionPayload, pc, ins_id)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDInsPayload, ins, input_cnt, opcode, inputs_bit_width,
                                               output_bit_width, inputs_address_byte, output_address_byte, len)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferInsPayload, ins, type, src_address_byte, dst_address_byte,
                                               size_byte, src_id, dst_id, transfer_id_tag)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ScalarInsPayload, ins, op, src1_value, src2_value, offset, dst_reg,
                                               write_special_register)

}  // namespace pimsim
