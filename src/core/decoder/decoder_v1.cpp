//
// Created by wyk on 2025/3/5.
//

#include "decoder.h"
#include "fmt/format.h"
#include "isa/isa.h"
#include "util/util.h"

namespace cimsim {

bool DecoderV1::checkInsStat(const std::string& expected_ins_stat_file) const {
    auto expected_ins_stat = readTypeFromJsonFile<InsStat>(expected_ins_stat_file);
    return expected_ins_stat == ins_stat_;
}

std::shared_ptr<ExecuteInsPayload> DecoderV1::decode(const InstV1& ins, int pc, int& pc_increment,
                                                     ResourceAllocatePayload& conflict_info) {
    // std::cout << fmt::format("pc: {}, ins id: {}, general reg: [{}]\n", ins_payload.pc, ins_payload.ins_id,
    //                             reg_unit_.getGeneralRegistersString());
    ins_id_++;
    ins_stat_.addInsCount(ins.class_code, ins.type, ins.opcode, simd_unit_config_);

    std::shared_ptr<ExecuteInsPayload> payload{};
    pc_increment = 1;

    if (ins.class_code == InstClass::scalar) {
        payload = decodeScalarIns(ins);
    } else if (ins.class_code == InstClass::simd) {
        payload = decodeVectorIns(ins);
    } else if (ins.class_code == InstClass::transfer) {
        payload = decodeTransferIns(ins);
    } else if (ins.class_code == InstClass::cim) {
        payload = decodeCimIns(ins);
    } else if (ins.class_code == InstClass::control) {
        payload = std::make_shared<ExecuteInsPayload>(InstructionPayload{.unit_type = ExecuteUnitType::control});
        pc_increment = decodeControlInsAndGetPCIncrement(ins);
    }
    payload->ins.pc = pc;
    payload->ins.ins_id = ins_id_;

    if (auto found = execute_unit_map_.find(payload->ins.unit_type._to_string()); found == execute_unit_map_.end()) {
        conflict_info = {.ins_id = ins_id_, .unit_type = payload->ins.unit_type};
    } else {
        conflict_info = found->second->getDataConflictInfo(payload);
    }

    return payload;
}

std::shared_ptr<ExecuteInsPayload> DecoderV1::decodeCimIns(const InstV1& ins) const {
    std::shared_ptr<ExecuteInsPayload> payload{nullptr};
    if (ins.type == CIMInstType::compute) {
        CimComputeInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_compute;

        p.input_addr_byte = reg_unit_->readRegister(ins.rs1, false);
        p.input_len = reg_unit_->readRegister(ins.rs2, false);
        p.input_bit_width = reg_unit_->readRegister(SpecialRegId::cim_input_bit_width, true);
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.group_input_step_byte = reg_unit_->readRegister(SpecialRegId::group_input_step, true);
        p.row = reg_unit_->readRegister(ins.rs3, false);
        p.bit_sparse = (ins.bit_sparse != 0);
        p.bit_sparse_meta_addr_byte = reg_unit_->readRegister(SpecialRegId::bit_sparse_meta_addr, true);
        p.value_sparse = (ins.value_sparse != 0);
        p.value_sparse_mask_addr_byte = reg_unit_->readRegister(SpecialRegId::value_sparse_mask_addr, true);

        payload = std::make_shared<CimComputeInsPayload>(p);
    } else if (ins.type == CIMInstType::set) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = CimControlOperator::set_activation;
        p.group_broadcast = (ins.group_broadcast != 0);
        p.group_id = reg_unit_->readRegister(ins.rs1, false);
        p.mask_addr_byte = reg_unit_->readRegister(ins.rs2, false);

        payload = std::make_shared<CimControlInsPayload>(p);
    } else if (ins.type == CIMInstType::output) {
        CimControlInsPayload p;
        p.ins.unit_type = ExecuteUnitType::cim_control;

        p.op = (ins.outsum_move != 0) ? CimControlOperator::output_sum_move
               : (ins.outsum != 0)    ? CimControlOperator::output_sum
                                      : CimControlOperator::only_output;
        p.activation_group_num = reg_unit_->readRegister(SpecialRegId::activation_group_num, true);
        p.output_addr_byte = reg_unit_->readRegister(ins.rd, false);
        p.output_cnt_per_group = reg_unit_->readRegister(ins.rs1, false);
        p.output_bit_width = reg_unit_->readRegister(SpecialRegId::cim_output_bit_width, true);
        p.output_mask_addr_byte = reg_unit_->readRegister(ins.rs2, false);

        payload = std::make_shared<CimControlInsPayload>(p);
    }
    return payload;
}

std::shared_ptr<ExecuteInsPayload> DecoderV1::decodeVectorIns(const InstV1& ins) const {
    SIMDInsPayload p;
    p.ins.unit_type = ExecuteUnitType::simd;

    auto input_cnt = static_cast<unsigned int>(ins.input_num + 1);
    auto opcode = static_cast<unsigned int>(ins.opcode);

    int i1_addr = reg_unit_->readRegister(ins.rs1, false);
    int i2_addr = (input_cnt < 2) ? 0 : reg_unit_->readRegister(ins.rs2, false);
    int i3_addr = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_3_address, true);
    int i4_addr = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::simd_input_4_address, true);

    int i1_bit_width = reg_unit_->readRegister(SpecialRegId::vector_input_1_bit_width, true);
    int i2_bit_width = (input_cnt < 2) ? 0 : reg_unit_->readRegister(SpecialRegId::vector_input_2_bit_width, true);
    int i3_bit_width = (input_cnt < 3) ? 0 : reg_unit_->readRegister(SpecialRegId::vector_input_3_bit_width, true);
    int i4_bit_width = (input_cnt < 4) ? 0 : reg_unit_->readRegister(SpecialRegId::vector_input_4_bit_width, true);

    p.inputs_bit_width = SIMDInputsArray{i1_bit_width, i2_bit_width, i3_bit_width, i4_bit_width};
    p.inputs_address_byte = SIMDInputsArray{i1_addr, i2_addr, i3_addr, i4_addr};
    p.output_bit_width = reg_unit_->readRegister(SpecialRegId::vector_output_bit_width, true);
    p.output_address_byte = reg_unit_->readRegister(ins.rd, false);
    p.len = reg_unit_->readRegister(ins.rs3, false);

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

std::shared_ptr<ExecuteInsPayload> DecoderV1::decodeScalarIns(const InstV1& ins) const {
    ScalarInsPayload p;
    p.ins.unit_type = ExecuteUnitType::scalar;

    if (ins.type == ScalarInstType::RR) {
        p.op = decodeScalarRROpcode(ins.opcode);
        p.src1_value = reg_unit_->readRegister(ins.rs1, false);
        p.src2_value = reg_unit_->readRegister(ins.rs2, false);
        p.dst_reg = ins.rd;
    } else if (ins.type == ScalarInstType::RI) {
        p.op = decodeScalarRIOpcode(ins.opcode);
        p.src1_value = reg_unit_->readRegister(ins.rs1, false);
        p.src2_value = ins.imm;
        p.dst_reg = ins.rd;
    } else if (ins.type == ScalarInstType::SL) {
        p.offset = ins.offset;
        p.src1_value = reg_unit_->readRegister(ins.rs1, false);
        if (ins.opcode == ScalarSLInstOpcode::load_local || ins.opcode == ScalarSLInstOpcode::load_global) {
            p.op = ScalarOperator::load;
            p.dst_reg = ins.rs2;
        } else {
            p.op = ScalarOperator::store;
            p.src2_value = reg_unit_->readRegister(ins.rs2, false);
        }
    } else if (ins.type == ScalarInstType::Assign) {
        p.op = ScalarOperator::assign;
        if (ins.opcode == ScalarAssignInstOpcode::li_general || ins.opcode == ScalarAssignInstOpcode::li_special) {
            p.src1_value = ins.imm;
            p.dst_reg = ins.rd;
            p.write_special_register = (ins.opcode == ScalarAssignInstOpcode::li_special);
        } else if (ins.opcode == ScalarAssignInstOpcode::assign_general_to_special) {
            p.src1_value = reg_unit_->readRegister(ins.rs1, false);
            p.dst_reg = ins.rs2;
            p.write_special_register = true;
        } else if (ins.opcode == ScalarAssignInstOpcode::assign_special_to_general) {
            p.src1_value = reg_unit_->readRegister(ins.rs2, true);
            p.dst_reg = ins.rs1;
            p.write_special_register = false;
        }
    }
    return std::make_shared<ScalarInsPayload>(p);
}

std::shared_ptr<ExecuteInsPayload> DecoderV1::decodeTransferIns(const InstV1& ins) const {
    TransferInsPayload p;
    p.ins.unit_type = ExecuteUnitType::transfer;

    if (ins.type == +TransferInstType::trans) {
        p.type = TransferType::local_trans;
        p.src_address_byte = reg_unit_->readRegister(ins.rs1, false) + (ins.offset_mask & 0b10) * ins.offset;
        p.dst_address_byte = reg_unit_->readRegister(ins.rd, false) + (ins.offset_mask & 0b01) * ins.offset;
        p.size_byte = reg_unit_->readRegister(ins.rs2, false);

        if (as_.isAddressGlobal(p.src_address_byte)) {
            p.type = TransferType::global_load;
        } else if (as_.isAddressGlobal(p.dst_address_byte)) {
            p.type = TransferType::global_store;
        }
    } else if (ins.type == +TransferInstType::send) {
        p.type = TransferType::send;
        p.src_address_byte = reg_unit_->readRegister(ins.rs1, false);
        p.dst_address_byte = reg_unit_->readRegister(ins.rd2, false);
        p.size_byte = reg_unit_->readRegister(ins.reg_len, false);
        p.src_id = core_id_;
        p.dst_id = reg_unit_->readRegister(ins.rd1, false);
        p.transfer_id_tag = reg_unit_->readRegister(ins.reg_id, false);
    } else if (ins.type == +TransferInstType::receive) {
        p.type = TransferType::receive;
        p.src_address_byte = reg_unit_->readRegister(ins.rs2, false);
        p.dst_address_byte = reg_unit_->readRegister(ins.rd, false);
        p.size_byte = reg_unit_->readRegister(ins.reg_len, false);
        p.src_id = reg_unit_->readRegister(ins.rs1, false);
        p.dst_id = core_id_;
        p.transfer_id_tag = reg_unit_->readRegister(ins.reg_id, false);
    }
    return std::make_shared<TransferInsPayload>(p);
}

int DecoderV1::decodeControlInsAndGetPCIncrement(const InstV1& ins) const {
    if (ins.type == ControlInstType::jmp) {
        return ins.offset;
    }

    int src_value1 = reg_unit_->readRegister(ins.rs1, false);
    int src_value2 = reg_unit_->readRegister(ins.rs2, false);
    bool branch = false;
    switch (ins.type) {
        case ControlInstType::beq: branch = (src_value1 == src_value2); break;
        case ControlInstType::bne: branch = (src_value1 != src_value2); break;
        case ControlInstType::bgt: branch = (src_value1 > src_value2); break;
        case ControlInstType::blt: branch = (src_value1 < src_value2); break;
        default: break;
    }
    if (branch) {
        return ins.offset;
    }
    return 1;
}

ScalarOperator DecoderV1::decodeScalarRROpcode(int opcode) {
    ScalarOperator op{ScalarOperator::add};
    switch (opcode) {
        case ScalarRRInstOpcode::add: op = ScalarOperator::add; break;
        case ScalarRRInstOpcode::sub: op = ScalarOperator::sub; break;
        case ScalarRRInstOpcode::mul: op = ScalarOperator::mul; break;
        case ScalarRRInstOpcode::div: op = ScalarOperator::div; break;
        case ScalarRRInstOpcode::sll: op = ScalarOperator::sll; break;
        case ScalarRRInstOpcode::srl: op = ScalarOperator::srl; break;
        case ScalarRRInstOpcode::sra: op = ScalarOperator::sra; break;
        case ScalarRRInstOpcode::mod: op = ScalarOperator::mod; break;
        case ScalarRRInstOpcode::min: op = ScalarOperator::min; break;
        case ScalarRRInstOpcode::max: op = ScalarOperator::max; break;
        case ScalarRRInstOpcode::s_and: op = ScalarOperator::s_and; break;
        case ScalarRRInstOpcode::s_or: op = ScalarOperator::s_or; break;
        case ScalarRRInstOpcode::eq: op = ScalarOperator::eq; break;
        case ScalarRRInstOpcode::ne: op = ScalarOperator::ne; break;
        case ScalarRRInstOpcode::gt: op = ScalarOperator::gt; break;
        case ScalarRRInstOpcode::lt: op = ScalarOperator::lt; break;
        default: break;
    }
    return op;
}

ScalarOperator DecoderV1::decodeScalarRIOpcode(int opcode) {
    ScalarOperator op{ScalarOperator::add};
    switch (opcode) {
        case ScalarRIInstOpcode::addi: op = ScalarOperator::add; break;
        case ScalarRIInstOpcode::subi: op = ScalarOperator::sub; break;
        case ScalarRIInstOpcode::muli: op = ScalarOperator::mul; break;
        case ScalarRIInstOpcode::divi: op = ScalarOperator::div; break;
        case ScalarRIInstOpcode::slli: op = ScalarOperator::sll; break;
        case ScalarRIInstOpcode::srli: op = ScalarOperator::srl; break;
        case ScalarRIInstOpcode::srai: op = ScalarOperator::sra; break;
        case ScalarRIInstOpcode::modi: op = ScalarOperator::mod; break;
        case ScalarRIInstOpcode::mini: op = ScalarOperator::min; break;
        case ScalarRIInstOpcode::maxi: op = ScalarOperator::max; break;
        case ScalarRIInstOpcode::andi: op = ScalarOperator::s_and; break;
        case ScalarRIInstOpcode::ori: op = ScalarOperator::s_or; break;
        case ScalarRIInstOpcode::eqi: op = ScalarOperator::eq; break;
        case ScalarRIInstOpcode::nei: op = ScalarOperator::ne; break;
        case ScalarRIInstOpcode::gti: op = ScalarOperator::gt; break;
        case ScalarRIInstOpcode::lti: op = ScalarOperator::lt; break;
        default: break;
    }
    return op;
}

}  // namespace cimsim
