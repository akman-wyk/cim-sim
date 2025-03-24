//
// Created by wyk on 2025/2/28.
//

#include "decoder.h"
#include "fmt/format.h"
#include "isa/isa_v2.h"

namespace cimsim {

std::shared_ptr<ExecuteInsPayload> DecoderV3::decode(const InstV3& ins, int pc, int& pc_increment,
                                                     ResourceAllocatePayload& conflict_info) {
    ins_id_++;

    // decode to get payload
    std::shared_ptr<ExecuteInsPayload> payload{};
    pc_increment = 1;

    if (int opcode = ins.getOpcode(); (opcode & OPCODE_MASK::INST_CLASS_2BIT) == OPCODE_CLASS::CIM) {
        payload = decodeCimIns(ins);
    } else if ((opcode & OPCODE_MASK::INST_CLASS_2BIT) == OPCODE_CLASS::VEC_OP) {
        payload = decodeSIMDIns(ins);
    } else if ((opcode & OPCODE_MASK::INST_CLASS_2BIT) == OPCODE_CLASS::SC) {
        payload = decodeScalarIns(ins);
    } else if ((opcode & OPCODE_MASK::INST_CLASS_3BIT) == OPCODE_CLASS::TRANS) {
        payload = decodeTransferIns(ins);
    } else {
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

std::shared_ptr<ExecuteInsPayload> DecoderV3::decodeCimIns(const InstV3& ins) const {
    std::shared_ptr<ExecuteInsPayload> payload{nullptr};
    if (int opcode = ins.getOpcode(); (opcode & OPCODE_MASK::INST_TYPE_2BIT) == OPCODE::CIM_MVM) {
        CimComputeInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_compute;

        p.input_addr_byte = reg_unit_->readRegister(ins.getR1(), false);
        p.input_len = reg_unit_->readRegister(ins.getR2(), false);
        p.input_bit_width = reg_unit_->readRegister(SpecialRegId::cim_input_bit_width, true);
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.group_input_step_byte = reg_unit_->readRegister(SpecialRegId::group_input_step, true);
        p.row = reg_unit_->readRegister(ins.getR3(), false);
        p.bit_sparse = ins.getFlag(FLAG_POSITION::CIM_MVM_SP_B);
        p.bit_sparse_meta_addr_byte = reg_unit_->readRegister(SpecialRegId::bit_sparse_meta_addr, true);
        p.value_sparse = ins.getFlag(FLAG_POSITION::CIM_MVM_SP_V);
        p.value_sparse_mask_addr_byte = reg_unit_->readRegister(SpecialRegId::value_sparse_mask_addr, true);

        payload = std::make_shared<CimComputeInsPayload>(p);
    } else if ((opcode & OPCODE_MASK::INST_TYPE_2BIT) == OPCODE::CIM_CFG) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = CimControlOperator::set_activation;
        p.group_broadcast = ins.getFlag(FLAG_POSITION::CIM_CFG_GRP_B);
        p.group_id = reg_unit_->readRegister(ins.getR1(), false);
        p.mask_addr_byte = reg_unit_->readRegister(ins.getR2(), false);

        payload = std::make_shared<CimControlInsPayload>(p);
    } else if ((opcode & OPCODE_MASK::INST_TYPE_2BIT) == OPCODE::CIM_OUT) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = ins.getFlag(FLAG_POSITION::CIM_OUT_OSUM_MOV) ? CimControlOperator::output_sum_move
               : ins.getFlag(FLAG_POSITION::CIM_OUT_OSUM)   ? CimControlOperator::output_sum
                                                            : CimControlOperator::only_output;
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.output_addr_byte = reg_unit_->readRegister(ins.getR3(), false);
        p.output_cnt_per_group = reg_unit_->readRegister(ins.getR1(), false);
        p.output_bit_width = reg_unit_->readRegister(SpecialRegId::cim_output_bit_width, true);
        p.output_mask_addr_byte = reg_unit_->readRegister(ins.getR2(), false);

        payload = std::make_shared<CimControlInsPayload>(p);
    }
    return payload;
}

std::shared_ptr<ExecuteInsPayload> DecoderV3::decodeSIMDIns(const InstV3& ins) const {
    SIMDInsPayload p;
    p.ins.unit_type = ExecuteUnitType::simd;

    auto input_cnt = static_cast<unsigned int>(((ins.getOpcode() >> 2) & 0x11) + 1);
    auto opcode = static_cast<unsigned int>(ins.getFunctType2());

    int i1_addr = reg_unit_->readRegister(ins.getR1(), false);
    int i2_addr = (input_cnt < 2) ? 0 : reg_unit_->readRegister(ins.getR2(), false);
    int i3_addr = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::input_3_address, true);
    int i4_addr = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::input_4_address, true);

    int i1_bit_width = reg_unit_->readRegister(SpecialRegId::simd_input_1_bit_width, true);
    int i2_bit_width = (input_cnt < 2) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_2_bit_width, true);
    int i3_bit_width = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_3_bit_width, true);
    int i4_bit_width = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_4_bit_width, true);

    p.inputs_bit_width = SIMDInputsArray{i1_bit_width, i2_bit_width, i3_bit_width, i4_bit_width};
    p.inputs_address_byte = SIMDInputsArray{i1_addr, i2_addr, i3_addr, i4_addr};
    p.output_bit_width = reg_unit_->readRegister(SpecialRegId::simd_output_bit_width, true);
    p.output_address_byte = reg_unit_->readRegister(ins.getR3(), false);
    p.len = reg_unit_->readRegister(ins.getR4(), false);

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

std::shared_ptr<ExecuteInsPayload> DecoderV3::decodeScalarIns(const InstV3& ins) const {
    ScalarInsPayload p;
    p.ins.unit_type = ExecuteUnitType::scalar;

    if (int opcode = ins.getOpcode(); opcode == OPCODE::SC_RR) {
        p.op = ScalarOperator::_from_integral(ins.getFunctType2());
        p.src1_value = reg_unit_->readRegister(ins.getR1(), false);
        p.src2_value = reg_unit_->readRegister(ins.getR2(), false);
        p.dst_reg = ins.getR3();
    } else if (opcode == OPCODE::SC_RI) {
        p.op = ScalarOperator::_from_integral(ins.getFunctType3());
        p.src1_value = reg_unit_->readRegister(ins.getR1(), false);
        p.src2_value = ins.getImmType3();
        p.dst_reg = ins.getR2();
    } else if (opcode == OPCODE::SC_LD || opcode == OPCODE::SC_LDG) {
        p.op = ScalarOperator::load;
        p.src1_value = reg_unit_->readRegister(ins.getR1(), false);
        p.offset = ins.getImmType4();
        p.dst_reg = ins.getR2();
    } else if (opcode == OPCODE::SC_ST || opcode == OPCODE::SC_STG) {
        p.op = ScalarOperator::store;
        p.src1_value = reg_unit_->readRegister(ins.getR1(), false);
        p.src2_value = reg_unit_->readRegister(ins.getR2(), false);
        p.offset = ins.getImmType4();
    } else if (opcode == OPCODE::G_LI || opcode == OPCODE::S_LI) {
        p.op = ScalarOperator::assign;
        p.src1_value = ins.getImmType5();
        p.dst_reg = ins.getR1();
        p.write_special_register = (opcode == OPCODE::S_LI);
    } else if (opcode == OPCODE::GS_MOV || opcode == OPCODE::SG_MOV) {
        p.op = ScalarOperator::assign;
        p.src1_value = reg_unit_->readRegister(ins.getR1(), opcode == OPCODE::SG_MOV);
        p.dst_reg = ins.getR2();
        p.write_special_register = opcode == OPCODE::GS_MOV;
    }
    return std::make_shared<ScalarInsPayload>(p);
}

std::shared_ptr<ExecuteInsPayload> DecoderV3::decodeTransferIns(const InstV3& ins) const {
    TransferInsPayload p;
    p.ins.unit_type = ExecuteUnitType::transfer;

    if (int opcode = ins.getOpcode(); (opcode & OPCODE_MASK::TRANS_TYPE_4BIT) == OPCODE::MEM_CPY) {
        p.type = TransferType::local_trans;
        p.src_address_byte = reg_unit_->readRegister(ins.getR1(), false) +
                             (ins.getFlag(FLAG_POSITION::MEM_CPY_SRC_MASK) ? ins.getImmType3() : 0);
        p.dst_address_byte = reg_unit_->readRegister(ins.getR3(), false) +
                             (ins.getFlag(FLAG_POSITION::MEM_CPY_DST_MASK) ? ins.getImmType3() : 0);
        p.size_byte = reg_unit_->readRegister(ins.getR2(), false);

        if (as_.isAddressGlobal(p.src_address_byte)) {
            p.type = TransferType::global_load;
        } else if (as_.isAddressGlobal(p.dst_address_byte)) {
            p.type = TransferType::global_store;
        }
    } else if ((opcode & OPCODE_MASK::TRANS_TYPE_5BIT) == OPCODE::SEND) {
        p.type = TransferType::send;
        p.src_id = core_id_;
        p.src_address_byte = reg_unit_->readRegister(ins.getR1(), false);
        p.dst_id = reg_unit_->readRegister(ins.getR2(), false);
        p.dst_address_byte = reg_unit_->readRegister(ins.getR3(), false);
        p.size_byte = reg_unit_->readRegister(ins.getR4(), false);
        p.transfer_id_tag = reg_unit_->readRegister(ins.getR5(), false);
    } else if ((opcode & OPCODE_MASK::TRANS_TYPE_5BIT) == OPCODE::RECV) {
        p.type = TransferType::receive;
        p.src_id = reg_unit_->readRegister(ins.getR1(), false);
        p.src_address_byte = reg_unit_->readRegister(ins.getR2(), false);
        p.dst_id = core_id_;
        p.dst_address_byte = reg_unit_->readRegister(ins.getR3(), false);
        p.size_byte = reg_unit_->readRegister(ins.getR4(), false);
        p.transfer_id_tag = reg_unit_->readRegister(ins.getR5(), false);
    }
    return std::make_shared<TransferInsPayload>(p);
}

int DecoderV3::decodeControlInsAndGetPCIncrement(const InstV3& ins) const {
    int opcode = ins.getOpcode();
    if (opcode == OPCODE::JMP) {
        return ins.getImmType6();
    }

    int src_value1 = reg_unit_->readRegister(ins.getR1(), false);
    int src_value2 = reg_unit_->readRegister(ins.getR2(), false);
    bool branch = false;
    switch (opcode) {
        case OPCODE::BEQ: branch = (src_value1 == src_value2); break;
        case OPCODE::BNE: branch = (src_value1 != src_value2); break;
        case OPCODE::BGT: branch = (src_value1 > src_value2); break;
        case OPCODE::BLT: branch = (src_value1 < src_value2); break;
        default: break;
    }
    return branch ? ins.getImmType4() : 1;
}

}  // namespace cimsim
