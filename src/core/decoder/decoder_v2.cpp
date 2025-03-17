//
// Created by wyk on 2025/3/5.
//

#include "core/core.h"
#include "decoder.h"
#include "fmt/format.h"

namespace cimsim {

std::shared_ptr<ExecuteInsPayload> DecoderV2::decode(const InstV2& ins, int pc, int& pc_increment,
                                                     ResourceAllocatePayload& conflict_info) {
    ins_id_++;

    // decode to get payload
    std::shared_ptr<ExecuteInsPayload> payload{};
    pc_increment = 1;

    if (auto op_class = ins.getOpcodeClass(); op_class == +OPCODE_CLASS::CIM) {
        payload = decodeCimIns(ins);
    } else if (op_class == +OPCODE_CLASS::VEC_OP) {
        payload = decodeSIMDIns(ins);
    } else if (op_class == +OPCODE_CLASS::SC) {
        payload = decodeScalarIns(ins);
    } else if (op_class == +OPCODE_CLASS::TRANS) {
        payload = decodeTransferIns(ins);
    } else if (op_class == +OPCODE_CLASS::CONTROL) {
        payload = std::make_shared<ExecuteInsPayload>(InstructionPayload{.unit_type = ExecuteUnitType::control});
        pc_increment = decodeControlInsAndGetPCIncrement(ins);
    }
    payload->ins.pc = pc;
    payload->ins.ins_id = ins_id_;

    // decode to get conflict info
    if (auto found = execute_unit_map_.find(payload->ins.unit_type._to_string()); found == execute_unit_map_.end()) {
        conflict_info = {.ins_id = ins_id_, .unit_type = payload->ins.unit_type};
    } else {
        conflict_info = found->second->getDataConflictInfo(payload);
    }

    return payload;
}

std::shared_ptr<ExecuteInsPayload> DecoderV2::decodeCimIns(const InstV2& ins) const {
    std::shared_ptr<ExecuteInsPayload> payload{nullptr};
    if (auto op = ins.getOpcodeEnum(); op == +OPCODE::CIM_MVM) {
        CimComputeInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_compute;

        p.input_addr_byte = reg_unit_->readRegister(ins.rs, false);
        p.input_len = reg_unit_->readRegister(ins.rt, false);
        p.input_bit_width = reg_unit_->readRegister(SpecialRegId::cim_input_bit_width, true);
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.group_input_step_byte = reg_unit_->readRegister(SpecialRegId::group_input_step, true);
        p.row = reg_unit_->readRegister(ins.re, false);
        p.bit_sparse = ins.SP_B;
        p.bit_sparse_meta_addr_byte = reg_unit_->readRegister(SpecialRegId::bit_sparse_meta_addr, true);
        p.value_sparse = ins.SP_V;
        p.value_sparse_mask_addr_byte = reg_unit_->readRegister(SpecialRegId::value_sparse_mask_addr, true);

        payload = std::make_shared<CimComputeInsPayload>(p);
    } else if (op == +OPCODE::CIM_CFG) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = CimControlOperator::set_activation;
        p.group_broadcast = ins.GRP_B;
        p.group_id = reg_unit_->readRegister(ins.rs, false);
        p.mask_addr_byte = reg_unit_->readRegister(ins.rt, false);

        payload = std::make_shared<CimControlInsPayload>(p);
    } else if (op == +OPCODE::CIM_OUT) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = ins.OSUM_MOV ? CimControlOperator::output_sum_move
               : ins.OSUM   ? CimControlOperator::output_sum
                            : CimControlOperator::only_output;
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.output_addr_byte = reg_unit_->readRegister(ins.rd, false);
        p.output_cnt_per_group = reg_unit_->readRegister(ins.rs, false);
        p.output_bit_width = reg_unit_->readRegister(SpecialRegId::cim_output_bit_width, true);
        p.output_mask_addr_byte = reg_unit_->readRegister(ins.rt, false);

        payload = std::make_shared<CimControlInsPayload>(p);
    }
    return payload;
}

std::shared_ptr<ExecuteInsPayload> DecoderV2::decodeSIMDIns(const InstV2& ins) const {
    SIMDInsPayload p;
    p.ins.unit_type = ExecuteUnitType::simd;

    auto input_cnt = static_cast<unsigned int>(((ins.opcode >> 2) & 0x11) + 1);
    auto opcode = static_cast<unsigned int>(ins.funct);

    int i1_addr = reg_unit_->readRegister(ins.rs, false);
    int i2_addr = (input_cnt < 2) ? 0 : reg_unit_->readRegister(ins.rt, false);
    int i3_addr = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::input_3_address, true);
    int i4_addr = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::input_4_address, true);

    int i1_bit_width = reg_unit_->readRegister(SpecialRegId::simd_input_1_bit_width, true);
    int i2_bit_width = (input_cnt < 2) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_2_bit_width, true);
    int i3_bit_width = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_3_bit_width, true);
    int i4_bit_width = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_4_bit_width, true);

    p.inputs_bit_width = SIMDInputsArray{i1_bit_width, i2_bit_width, i3_bit_width, i4_bit_width};
    p.inputs_address_byte = SIMDInputsArray{i1_addr, i2_addr, i3_addr, i4_addr};
    p.output_bit_width = reg_unit_->readRegister(SpecialRegId::simd_output_bit_width, true);
    p.output_address_byte = reg_unit_->readRegister(ins.rd, false);
    p.len = reg_unit_->readRegister(ins.re, false);

    const auto& [ins_cfg, func_cfg] = getSIMDInstructionAndFunctor(input_cnt, opcode, p.inputs_bit_width);
    if (ins_cfg == nullptr || func_cfg == nullptr) {
        std::cerr << fmt::format("No match {}, Invalid SIMD instruction: \n{}",
                                 (ins_cfg == nullptr ? "inst" : "functor"), p.toString());
        return std::make_shared<ExecuteInsPayload>(InstructionPayload{.unit_type = ExecuteUnitType::none});
    }
    p.ins_cfg = ins_cfg;
    p.func_cfg = func_cfg;

    return std::make_shared<SIMDInsPayload>(p);
}

std::shared_ptr<ExecuteInsPayload> DecoderV2::decodeScalarIns(const InstV2& ins) const {
    ScalarInsPayload p;
    p.ins.unit_type = ExecuteUnitType::scalar;

    if (auto op = ins.getOpcodeEnum(); op == +OPCODE::SC_RR) {
        p.op = ScalarOperator::_from_integral(ins.funct);
        p.src1_value = reg_unit_->readRegister(ins.rs, false);
        p.src2_value = reg_unit_->readRegister(ins.rt, false);
        p.dst_reg = ins.rd;
    } else if (op == +OPCODE::SC_RI) {
        p.op = ScalarOperator::_from_integral(ins.funct);
        p.src1_value = reg_unit_->readRegister(ins.rs, false);
        p.src2_value = ins.imm;
        p.dst_reg = ins.rd;
    } else if (op == +OPCODE::SC_LD || op == +OPCODE::SC_LDG) {
        p.op = ScalarOperator::load;
        p.src1_value = reg_unit_->readRegister(ins.rs, false);
        p.offset = ins.imm;
        p.dst_reg = ins.rd;
    } else if (op == +OPCODE::SC_ST || op == +OPCODE::SC_STG) {
        p.op = ScalarOperator::store;
        p.src1_value = reg_unit_->readRegister(ins.rs, false);
        p.src2_value = reg_unit_->readRegister(ins.rt, false);
        p.offset = ins.imm;
    } else if (op == +OPCODE::G_LI || op == +OPCODE::S_LI) {
        p.op = ScalarOperator::assign;
        p.src1_value = ins.imm;
        p.dst_reg = ins.rd;
        p.write_special_register = (op == +OPCODE::S_LI);
    } else if (op == +OPCODE::GS_MOV || op == +OPCODE::SG_MOV) {
        p.op = ScalarOperator::assign;
        p.src1_value = reg_unit_->readRegister(ins.rs, op == +OPCODE::SG_MOV);
        p.dst_reg = ins.rd;
        p.write_special_register = (op == +OPCODE::GS_MOV);
    }
    return std::make_shared<ScalarInsPayload>(p);
}

std::shared_ptr<ExecuteInsPayload> DecoderV2::decodeTransferIns(const InstV2& ins) const {
    TransferInsPayload p;
    p.ins.unit_type = ExecuteUnitType::transfer;

    if (auto op = ins.getOpcodeEnum(); op == +OPCODE::MEM_CPY) {
        p.type = TransferType::local_trans;
        p.src_address_byte = reg_unit_->readRegister(ins.rs, false) + ((ins.opcode & 0b000010) != 0 ? ins.imm : 0);
        p.dst_address_byte = reg_unit_->readRegister(ins.rd, false) + ((ins.opcode & 0b000001) != 0 ? ins.imm : 0);
        p.size_byte = reg_unit_->readRegister(ins.rt, false);

        if (as_.isAddressGlobal(p.src_address_byte)) {
            p.type = TransferType::global_load;
        } else if (as_.isAddressGlobal(p.dst_address_byte)) {
            p.type = TransferType::global_store;
        }
    } else if (op == +OPCODE::SEND) {
        p.type = TransferType::send;
        p.src_id = core_->getCoreId();
        p.src_address_byte = reg_unit_->readRegister(ins.rs, false);
        p.dst_id = reg_unit_->readRegister(ins.rt, false);
        p.dst_address_byte = reg_unit_->readRegister(ins.rd, false);
        p.size_byte = reg_unit_->readRegister(ins.re, false);
        p.transfer_id_tag = reg_unit_->readRegister(ins.rf, false);
    } else if (op == +OPCODE::RECV) {
        p.type = TransferType::receive;
        p.src_id = reg_unit_->readRegister(ins.rs, false);
        p.src_address_byte = reg_unit_->readRegister(ins.rt, false);
        p.dst_id = core_->getCoreId();
        p.dst_address_byte = reg_unit_->readRegister(ins.rd, false);
        p.size_byte = reg_unit_->readRegister(ins.re, false);
        p.transfer_id_tag = reg_unit_->readRegister(ins.rf, false);
    }
    return std::make_shared<TransferInsPayload>(p);
}

int DecoderV2::decodeControlInsAndGetPCIncrement(const InstV2& ins) const {
    auto op = ins.getOpcodeEnum();
    if (op == +OPCODE::JMP) {
        return ins.imm;
    }

    int src_value1 = reg_unit_->readRegister(ins.rs, false);
    int src_value2 = reg_unit_->readRegister(ins.rt, false);
    bool branch = false;
    switch (op) {
        case OPCODE::BEQ: branch = (src_value1 == src_value2); break;
        case OPCODE::BNE: branch = (src_value1 != src_value2); break;
        case OPCODE::BGT: branch = (src_value1 > src_value2); break;
        case OPCODE::BLT: branch = (src_value1 < src_value2); break;
        default: break;
    }
    return branch ? ins.imm : 1;
}

}  // namespace cimsim
