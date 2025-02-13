//
// Created by wyk on 2024/8/8.
//

#include "core.h"

#include <utility>

#include "fmt/format.h"
#include "isa/isa.h"
#include "util/log.h"

namespace pimsim {

Core::Core(int core_id, const char *name, const Config &config, Clock *clk, std::vector<Instruction> ins_list,
           std::function<void()> finish_run_call, bool check, std::ostream &reg_stat_os)
    : BaseModule(name, config.sim_config, this, clk)
    , core_id_(core_id)
    , core_config_(config.chip_config.core_config)
    , global_memory_addressing_(config.chip_config.global_memory_config.addressing)

    , check(check)
    , reg_stat_os_(reg_stat_os)
    , ins_list_(std::move(ins_list))
    , scalar_unit_("ScalarUnit", core_config_.scalar_unit_config, config.sim_config, this, clk)
    , simd_unit_("SIMDUnit", core_config_.simd_unit_config, config.sim_config, this, clk)
    , transfer_unit_("TransferUnit", core_config_.transfer_unit_config, config.sim_config, this, clk, core_id,
                     config.chip_config.global_memory_config.global_memory_switch_id)

    , cim_unit_("CimUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_compute_unit_("PimComputeUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_load_unit_("PimLoadUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_output_unit_("PimOutputUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_set_unit_("PimSetUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_transfer_unit_("PimTransferUnit", core_config_.pim_unit_config, config.sim_config, this, clk)

    , local_memory_unit_("LocalMemoryUnit", core_config_.local_memory_unit_config, config.sim_config, this, clk)
    , reg_unit_("RegUnit", core_config_.register_unit_config, config.sim_config, this, clk)
    , core_switch_("CoreSwitch", config.sim_config, this, clk, core_id)

    , scalar_stall_handler_(decode_new_ins_trigger_)
    , simd_stall_handler_(decode_new_ins_trigger_)
    , transfer_stall_handler_(decode_new_ins_trigger_)
    , pim_compute_stall_handler_(decode_new_ins_trigger_)
    , pim_load_stall_handler_(decode_new_ins_trigger_)
    , pim_output_stall_handler_(decode_new_ins_trigger_)
    , pim_set_stall_handler_(decode_new_ins_trigger_)
    , pim_transfer_stall_handler_(decode_new_ins_trigger_)

    , finish_run_call_(std::move(finish_run_call)) {
    SC_THREAD(issue)

    SC_METHOD(processStall)
    sensitive << scalar_conflict_ << simd_conflict_ << transfer_conflict_ << pim_compute_conflict_ << pim_load_conflict_
              << pim_output_conflict_ << pim_set_conflict_ << pim_transfer_conflict_;

    SC_METHOD(processIdExEnable)
    sensitive << id_stall_;

    SC_METHOD(processFinishRun)
    sensitive << scalar_signals_.finish_run_ << simd_signals_.finish_run_ << transfer_signals_.finish_run_
              << pim_compute_signals_.finish_run_ << pim_load_signals_.finish_run_ << pim_output_signals_.finish_run_
              << pim_set_signals_.finish_run_ << pim_transfer_signals_.finish_run_;

    // bind and set modules
    int end_pc = static_cast<int>(ins_list_.size());
    scalar_unit_.ports_.bind(scalar_signals_);
    scalar_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    scalar_unit_.bindRegUnit(&reg_unit_);
    scalar_unit_.setEndPC(end_pc);

    simd_unit_.ports_.bind(simd_signals_);
    simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    simd_unit_.setEndPC(end_pc);

    transfer_unit_.ports_.bind(transfer_signals_);
    transfer_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    transfer_unit_.bindSwitch(&core_switch_);
    transfer_unit_.setEndPC(end_pc);

    pim_compute_unit_.ports_.bind(pim_compute_signals_);
    pim_compute_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_compute_unit_.bindCimUnit(&cim_unit_);
    pim_compute_unit_.setEndPC(end_pc);

    pim_load_unit_.ports_.bind(pim_load_signals_);
    pim_load_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_load_unit_.setEndPC(end_pc);

    pim_output_unit_.ports_.bind(pim_output_signals_);
    pim_output_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_output_unit_.setEndPC(end_pc);

    pim_set_unit_.ports_.bind(pim_set_signals_);
    pim_set_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_set_unit_.bindCimUnit(&cim_unit_);
    pim_set_unit_.setEndPC(end_pc);

    pim_transfer_unit_.ports_.bind(pim_transfer_signals_);
    pim_transfer_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_transfer_unit_.setEndPC(end_pc);

    reg_unit_.write_req_port_.bind(write_req_signal_);
    reg_unit_.read_req_port_.bind(read_req_signal_);
    reg_unit_.read_rsp_port_.bind(read_rsp_signal_);

    local_memory_unit_.bindCimUnit(&cim_unit_);

    // bind stall handler
    scalar_stall_handler_.bind(scalar_signals_, scalar_conflict_, &cur_ins_conflict_info_);
    simd_stall_handler_.bind(simd_signals_, simd_conflict_, &cur_ins_conflict_info_);
    transfer_stall_handler_.bind(transfer_signals_, transfer_conflict_, &cur_ins_conflict_info_);
    pim_compute_stall_handler_.bind(pim_compute_signals_, pim_compute_conflict_, &cur_ins_conflict_info_);
    pim_load_stall_handler_.bind(pim_load_signals_, pim_load_conflict_, &cur_ins_conflict_info_);
    pim_output_stall_handler_.bind(pim_output_signals_, pim_output_conflict_, &cur_ins_conflict_info_);
    pim_set_stall_handler_.bind(pim_set_signals_, pim_set_conflict_, &cur_ins_conflict_info_);
    pim_transfer_stall_handler_.bind(pim_transfer_signals_, pim_transfer_conflict_, &cur_ins_conflict_info_);
}

void Core::bindNetwork(Network *network) {
    core_switch_.bindNetwork(network);
}

EnergyReporter Core::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("ScalarUnit", EnergyReporter{scalar_unit_.getEnergyReporter()});
    reporter.addSubModule("SIMDUnit", EnergyReporter{simd_unit_.getEnergyReporter()});
    reporter.addSubModule("PimUnit", EnergyReporter{pim_compute_unit_.getEnergyReporter()});
    reporter.addSubModule("PimLoad", EnergyReporter{pim_load_unit_.getEnergyReporter()});
    reporter.addSubModule("PimOutput", EnergyReporter{pim_output_unit_.getEnergyReporter()});
    reporter.addSubModule("PimTransfer", EnergyReporter{pim_transfer_unit_.getEnergyReporter()});
    reporter.addSubModule("LocalMemoryUnit", EnergyReporter{local_memory_unit_.getEnergyReporter()});
    return std::move(reporter);
}

bool Core::checkRegValues(const std::array<int, GENERAL_REG_NUM> &general_reg_expected_values,
                          const std::array<int, SPECIAL_REG_NUM> &special_reg_expected_values) {
    return reg_unit_.checkRegValues(general_reg_expected_values, special_reg_expected_values);
}

bool Core::checkInsStat(const std::string &expected_ins_stat_file) const {
    std::ifstream expected_ins_stat_ifs;
    expected_ins_stat_ifs.open(expected_ins_stat_file);
    nlohmann::ordered_json expected_ins_stat_j = nlohmann::ordered_json::parse(expected_ins_stat_ifs);
    expected_ins_stat_ifs.close();

    auto expected_ins_stat = expected_ins_stat_j.get<InsStat>();
    return expected_ins_stat == ins_stat_;
}

int Core::getCoreId() const {
    return core_id_;
}

[[noreturn]] void Core::issue() {
    ScalarInsPayload scalar_nop{};
    SIMDInsPayload simd_nop{};
    TransferInsPayload transfer_nop{};
    PimComputeInsPayload pim_compute_nop{};
    PimLoadInsPayload pim_load_nop{};
    PimOutputInsPayload pim_output_nop{};
    PimSetInsPayload pim_set_nop{};
    PimTransferInsPayload pim_transfer_nop{};

    wait(period_ns_ - 1, SC_NS);

    int pc_increment = 0;
    while (true) {
        if (cur_ins_conflict_info_.unit_type == +ExecuteUnitType::none) {
            if (ins_index_ < ins_list_.size()) {
                pc_increment = decodeAndGetPCIncrement();
                decode_new_ins_trigger_.notify();
            } else {
                pc_increment = 0;
            }
        }
        wait(0.1, SC_NS);

        if (!id_stall_.read() && cur_ins_conflict_info_.unit_type != +ExecuteUnitType::none) {
            scalar_signals_.id_ex_payload_.write(scalar_payload_);
            simd_signals_.id_ex_payload_.write(simd_payload_);
            transfer_signals_.id_ex_payload_.write(transfer_payload_);
            pim_compute_signals_.id_ex_payload_.write(pim_compute_payload_);
            pim_load_signals_.id_ex_payload_.write(pim_load_payload_);
            pim_output_signals_.id_ex_payload_.write(pim_output_payload_);
            pim_set_signals_.id_ex_payload_.write(pim_set_payload_);
            pim_transfer_signals_.id_ex_payload_.write(pim_transfer_payload_);

            ins_index_ += pc_increment;
            cur_ins_conflict_info_ = DataConflictPayload{.ins_id = -1, .unit_type = ExecuteUnitType::none};
        } else {
            scalar_signals_.id_ex_payload_.write(scalar_nop);
            simd_signals_.id_ex_payload_.write(simd_nop);
            transfer_signals_.id_ex_payload_.write(transfer_nop);
            pim_compute_signals_.id_ex_payload_.write(pim_compute_nop);
            pim_load_signals_.id_ex_payload_.write(pim_load_nop);
            pim_output_signals_.id_ex_payload_.write(pim_output_nop);
            pim_set_signals_.id_ex_payload_.write(pim_set_nop);
            pim_transfer_signals_.id_ex_payload_.write(pim_transfer_nop);
        }
        wait(period_ns_ - 0.1, SC_NS);
    }
}

void Core::processStall() {
    bool stall = scalar_conflict_.read() || simd_conflict_.read() || transfer_conflict_.read() ||
                 pim_compute_conflict_.read() || pim_load_conflict_.read() || pim_output_conflict_.read() ||
                 pim_set_conflict_.read() || pim_transfer_conflict_.read();
    id_stall_.write(stall);
}

void Core::processIdExEnable() {
    scalar_signals_.id_ex_enable_.write(!id_stall_.read());
    simd_signals_.id_ex_enable_.write(!id_stall_.read());
    transfer_signals_.id_ex_enable_.write(!id_stall_.read());
    pim_compute_signals_.id_ex_enable_.write(!id_stall_.read());
    pim_load_signals_.id_ex_enable_.write(!id_stall_.read());
    pim_output_signals_.id_ex_enable_.write(!id_stall_.read());
    pim_set_signals_.id_ex_enable_.write(!id_stall_.read());
    pim_transfer_signals_.id_ex_enable_.write(!id_stall_.read());
}

void Core::processFinishRun() {
    if (scalar_signals_.finish_run_.read() || simd_signals_.finish_run_.read() ||
        transfer_signals_.finish_run_.read() || pim_compute_signals_.finish_run_.read() ||
        pim_load_signals_.finish_run_.read() || pim_output_signals_.finish_run_.read() ||
        pim_set_signals_.finish_run_.read() || pim_transfer_signals_.finish_run_.read()) {
        LOG(fmt::format("finish run"));
        finish_run_call_();
    }
}

int Core::decodeAndGetPCIncrement() {
    scalar_payload_.ins.clear();
    simd_payload_.ins.clear();
    transfer_payload_.ins.clear();
    pim_compute_payload_.ins.clear();
    pim_load_payload_.ins.clear();
    pim_output_payload_.ins.clear();
    pim_set_payload_.ins.clear();
    pim_transfer_payload_.ins.clear();

    InstructionPayload ins_payload{.pc = ins_index_ + 1, .ins_id = ins_id_++};

    cur_ins_conflict_info_.unit_type = ExecuteUnitType::none;
    cur_ins_conflict_info_.ins_id = -1;

    const auto &ins = ins_list_[ins_index_];
    if (check) {
        reg_stat_os_ << fmt::format("pc: {}, ins id: {}, general reg: [{}]\n", ins_payload.pc, ins_payload.ins_id,
                                    reg_unit_.getGeneralRegistersString());
    }

    ins_stat_.addInsCount(ins.class_code, ins.type, ins.opcode, core_config_);
    if (ins.class_code == InstClass::control) {
        return decodeControlInsAndGetPCIncrement(ins, ins_payload);
    }

    if (ins.class_code == InstClass::scalar) {
        decodeScalarIns(ins, ins_payload);
    } else if (ins.class_code == InstClass::simd) {
        decodeSIMDIns(ins, ins_payload);
    } else if (ins.class_code == InstClass::transfer) {
        decodeTransferIns(ins, ins_payload);
    } else if (ins.class_code == InstClass::pim) {
        if (ins.type == PIMInstType::compute) {
            decodePimComputeIns(ins, ins_payload);
        } else if (ins.type == PIMInstType::set) {
            decodePimSetIns(ins, ins_payload);
        } else if (ins.type == PIMInstType::output) {
            decodePimOutputIns(ins, ins_payload);
        } else if (ins.type == PIMInstType::transfer) {
            decodePimTransferIns(ins, ins_payload);
        }
    }
    return 1;
}

void Core::decodeScalarIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload scalar_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::scalar};

    if (ins.type == ScalarInstType::RR) {
        ScalarOperator op{};
        switch (ins.opcode) {
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
        }
        scalar_payload_ = ScalarInsPayload{.ins = scalar_ins_payload,
                                           .op = op,
                                           .src1_value = reg_unit_.readRegister(ins.rs1, false),
                                           .src2_value = reg_unit_.readRegister(ins.rs2, false),
                                           .offset = 0,
                                           .dst_reg = ins.rd,
                                           .write_special_register = false};
    } else if (ins.type == ScalarInstType::RI) {
        ScalarOperator op{};
        switch (ins.opcode) {
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
        }
        scalar_payload_ = ScalarInsPayload{.ins = scalar_ins_payload,
                                           .op = op,
                                           .src1_value = reg_unit_.readRegister(ins.rs1, false),
                                           .src2_value = ins.imm,
                                           .offset = 0,
                                           .dst_reg = ins.rd,
                                           .write_special_register = false};
    } else if (ins.type == ScalarInstType::SL) {
        if (ins.opcode == ScalarSLInstOpcode::load_local || ins.opcode == ScalarSLInstOpcode::load_global) {
            scalar_payload_ = ScalarInsPayload{.ins = scalar_ins_payload,
                                               .op = ScalarOperator::load,
                                               .src1_value = reg_unit_.readRegister(ins.rs1, false),
                                               .src2_value = 0,
                                               .offset = ins.offset,
                                               .dst_reg = ins.rs2,
                                               .write_special_register = false};
        } else {
            scalar_payload_ = ScalarInsPayload{.ins = scalar_ins_payload,
                                               .op = ScalarOperator::store,
                                               .src1_value = reg_unit_.readRegister(ins.rs1, false),
                                               .src2_value = reg_unit_.readRegister(ins.rs2, false),
                                               .offset = ins.offset,
                                               .dst_reg = 0,
                                               .write_special_register = false};
        }
    } else if (ins.type == ScalarInstType::Assign) {
        scalar_payload_.ins = scalar_ins_payload;
        scalar_payload_.op = ScalarOperator::assign;
        scalar_payload_.src2_value = 0;
        scalar_payload_.offset = 0;
        if (ins.opcode == ScalarAssignInstOpcode::li_general || ins.opcode == ScalarAssignInstOpcode::li_special) {
            scalar_payload_.src1_value = ins.imm;
            scalar_payload_.dst_reg = ins.rd;
            scalar_payload_.write_special_register = (ins.opcode == ScalarAssignInstOpcode::li_special);
        } else if (ins.opcode == ScalarAssignInstOpcode::assign_general_to_special) {
            scalar_payload_.src1_value = reg_unit_.readRegister(ins.rs1, false);
            scalar_payload_.dst_reg = ins.rs2;
            scalar_payload_.write_special_register = true;
        } else if (ins.opcode == ScalarAssignInstOpcode::assign_special_to_general) {
            scalar_payload_.src1_value = reg_unit_.readRegister(ins.rs2, true);
            scalar_payload_.dst_reg = ins.rs1;
            scalar_payload_.write_special_register = false;
        }
    }

    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = scalar_payload_.ins.ins_id, .unit_type = ExecuteUnitType::scalar};
}

void Core::decodeSIMDIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload simd_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::simd};

    int input_cnt = ins.input_num + 1;

    int i1_addr = reg_unit_.readRegister(ins.rs1, false);
    int i2_addr = (input_cnt < 2) ? 0 : reg_unit_.readRegister(ins.rs2, false);
    int i3_addr = (input_cnt < 3) ? 0 : reg_unit_.readRegister(SpecialRegId::input_3_address, true);
    int i4_addr = (input_cnt < 4) ? 0 : reg_unit_.readRegister(SpecialRegId::input_4_address, true);

    int i1_bit_width = reg_unit_.readRegister(SpecialRegId::simd_input_1_bit_width, true);
    int i2_bit_width = (input_cnt < 2) ? 0 : reg_unit_.readRegister(SpecialRegId::simd_input_2_bit_width, true);
    int i3_bit_width = (input_cnt < 3) ? 0 : reg_unit_.readRegister(SpecialRegId::simd_input_3_bit_width, true);
    int i4_bit_width = (input_cnt < 4) ? 0 : reg_unit_.readRegister(SpecialRegId::simd_input_4_bit_width, true);

    simd_payload_ =
        SIMDInsPayload{.ins = simd_ins_payload,
                       .input_cnt = static_cast<unsigned int>(input_cnt),
                       .opcode = static_cast<unsigned int>(ins.opcode),
                       .inputs_bit_width = {i1_bit_width, i2_bit_width, i3_bit_width, i4_bit_width},
                       .output_bit_width = reg_unit_.readRegister(SpecialRegId::simd_output_bit_width, true),
                       .inputs_address_byte = {i1_addr, i2_addr, i3_addr, i4_addr},
                       .output_address_byte = reg_unit_.readRegister(ins.rd, false),
                       .len = reg_unit_.readRegister(ins.rs3, false)};

    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = simd_payload_.ins.ins_id, .unit_type = ExecuteUnitType::simd};
    for (unsigned int i = 0; i < simd_payload_.input_cnt; i++) {
        cur_ins_conflict_info_.addReadMemoryId(
            local_memory_unit_.getLocalMemoryIdByAddress(simd_payload_.inputs_address_byte[i]));
    }
    cur_ins_conflict_info_.addWriteMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(simd_payload_.output_address_byte));
}

void Core::decodeTransferIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    if (ins.type == +TransferInstType::trans) {
        int src_address_byte = reg_unit_.readRegister(ins.rs1, false) + (ins.offset_mask & 0b10) * ins.offset;
        int dst_address_byte = reg_unit_.readRegister(ins.rd, false) + (ins.offset_mask & 0b01) * ins.offset;
        int size_byte = reg_unit_.readRegister(ins.rs2, false);

        const auto &pim_as = core_config_.pim_unit_config.address_space;
        // if (pim_as.offset_byte <= dst_address_byte && dst_address_byte + size_byte <= pim_as.end()) {
        //     pim_load_payload_ = PimLoadInsPayload{
        //         .ins = {.pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::pim_load},
        //         .src_address_byte = src_address_byte,
        //         .size_byte = size_byte};

        //     cur_ins_conflict_info_ =
        //         DataConflictPayload{.ins_id = pim_load_payload_.ins.ins_id, .unit_type = ExecuteUnitType::pim_load};
        //     cur_ins_conflict_info_.use_pim_unit = true;
        //     cur_ins_conflict_info_.addReadMemoryId(
        //         local_memory_unit_.getLocalMemoryIdByAddress(pim_load_payload_.src_address_byte));
        // } else {
        TransferType type = TransferType::local_trans;
        if (global_memory_addressing_.offset_byte <= src_address_byte &&
            src_address_byte + size_byte <= global_memory_addressing_.end()) {
            type = TransferType::global_load;
            src_address_byte -= global_memory_addressing_.offset_byte;
        } else if (global_memory_addressing_.offset_byte <= dst_address_byte &&
                   dst_address_byte + size_byte <= global_memory_addressing_.end()) {
            type = TransferType::global_store;
            dst_address_byte -= global_memory_addressing_.offset_byte;
        }

        transfer_payload_ = TransferInsPayload{
            .ins = {.pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::transfer},
            .type = type,
            .src_address_byte = src_address_byte,
            .dst_address_byte = dst_address_byte,
            .size_byte = size_byte};

        cur_ins_conflict_info_ =
            DataConflictPayload{.ins_id = transfer_payload_.ins.ins_id, .unit_type = ExecuteUnitType::transfer};
        if (type == +TransferType::local_trans || type == +TransferType::global_store) {
            cur_ins_conflict_info_.addReadMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(src_address_byte));
        }
        if (type == +TransferType::local_trans || type == +TransferType::global_load) {
            cur_ins_conflict_info_.addWriteMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(dst_address_byte));
        }
        // }
    } else if (ins.type == +TransferInstType::send) {
        int src_address_byte = reg_unit_.readRegister(ins.rs1, false);

        transfer_payload_ = TransferInsPayload{
            .ins = {.pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::transfer},
            .type = TransferType::send,
            .src_address_byte = src_address_byte,
            .dst_address_byte = reg_unit_.readRegister(ins.rd2, false),
            .size_byte = reg_unit_.readRegister(ins.reg_len, false),
            .src_id = core_id_,
            .dst_id = reg_unit_.readRegister(ins.rd1, false),
            .transfer_id_tag = reg_unit_.readRegister(ins.reg_id, false)};
        cur_ins_conflict_info_ =
            DataConflictPayload{.ins_id = transfer_payload_.ins.ins_id, .unit_type = ExecuteUnitType::transfer};
        cur_ins_conflict_info_.addReadMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(src_address_byte));
    } else if (ins.type == +TransferInstType::receive) {
        int dst_address_byte = reg_unit_.readRegister(ins.rd, false);

        transfer_payload_ = TransferInsPayload{
            .ins = {.pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::transfer},
            .type = TransferType::receive,
            .src_address_byte = reg_unit_.readRegister(ins.rs2, false),
            .dst_address_byte = dst_address_byte,
            .size_byte = reg_unit_.readRegister(ins.reg_len, false),
            .src_id = reg_unit_.readRegister(ins.rs1, false),
            .dst_id = core_id_,
            .transfer_id_tag = reg_unit_.readRegister(ins.reg_id, false)};
        cur_ins_conflict_info_ =
            DataConflictPayload{.ins_id = transfer_payload_.ins.ins_id, .unit_type = ExecuteUnitType::transfer};
        cur_ins_conflict_info_.addWriteMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(dst_address_byte));
    }
}

void Core::decodePimComputeIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload pim_compute_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::pim_compute};

    pim_compute_payload_ = PimComputeInsPayload{
        .ins = pim_compute_ins_payload,
        .input_addr_byte = reg_unit_.readRegister(ins.rs1, false),
        .input_len = reg_unit_.readRegister(ins.rs2, false),
        .input_bit_width = reg_unit_.readRegister(SpecialRegId::pim_input_bit_width, true),
        .activation_group_num = reg_unit_.readRegister(SpecialRegId::activation_group_num, true),
        .group_input_step_byte = reg_unit_.readRegister(SpecialRegId::group_input_step, true),
        .row = reg_unit_.readRegister(ins.rs3, false),
        .bit_sparse = (ins.bit_sparse != 0),
        .bit_sparse_meta_addr_byte = reg_unit_.readRegister(SpecialRegId::bit_sparse_meta_addr, true),
        .value_sparse = (ins.value_sparse != 0),
        .value_sparse_mask_addr_byte = reg_unit_.readRegister(SpecialRegId::value_sparse_mask_addr, true)};

    const auto &pim_unit_config = core_config_.pim_unit_config;
    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = pim_compute_payload_.ins.ins_id, .unit_type = ExecuteUnitType::pim_compute};
    cur_ins_conflict_info_.use_pim_unit = true;
    cur_ins_conflict_info_.addReadMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(pim_compute_payload_.input_addr_byte));
    if (pim_unit_config.value_sparse && pim_compute_payload_.value_sparse) {
        cur_ins_conflict_info_.addReadMemoryId(
            local_memory_unit_.getLocalMemoryIdByAddress(pim_compute_payload_.value_sparse_mask_addr_byte));
    }
    if (pim_unit_config.bit_sparse && pim_compute_payload_.bit_sparse) {
        cur_ins_conflict_info_.addReadMemoryId(
            local_memory_unit_.getLocalMemoryIdByAddress(pim_compute_payload_.bit_sparse_meta_addr_byte));
    }
}

void Core::decodePimOutputIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload pim_output_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::pim_output};

    PimOutputType pim_output_type = (ins.outsum_move != 0) ? PimOutputType::output_sum_move
                                    : (ins.outsum != 0)    ? PimOutputType::output_sum
                                                           : PimOutputType::only_output;
    pim_output_payload_ =
        PimOutputInsPayload{.ins = pim_output_ins_payload,
                            .activation_group_num = reg_unit_.readRegister(SpecialRegId::activation_group_num, true),
                            .output_type = pim_output_type,
                            .output_addr_byte = reg_unit_.readRegister(ins.rd, false),
                            .output_cnt_per_group = reg_unit_.readRegister(ins.rs1, false),
                            .output_bit_width = reg_unit_.readRegister(SpecialRegId::pim_output_bit_width, true),
                            .output_mask_addr_byte = reg_unit_.readRegister(ins.rs2, false)};

    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = pim_output_payload_.ins.ins_id, .unit_type = ExecuteUnitType::pim_output};
    cur_ins_conflict_info_.use_pim_unit = true;
    cur_ins_conflict_info_.addWriteMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(pim_output_payload_.output_addr_byte));
    if (pim_output_payload_.output_type == +PimOutputType::output_sum) {
        cur_ins_conflict_info_.addReadMemoryId(
            local_memory_unit_.getLocalMemoryIdByAddress(pim_output_payload_.output_mask_addr_byte));
    }
}

void Core::decodePimSetIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload pim_set_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::pim_set};

    pim_set_payload_ = PimSetInsPayload{.ins = pim_set_ins_payload,
                                        .group_broadcast = (ins.group_broadcast != 0),
                                        .group_id = reg_unit_.readRegister(ins.rs1, false),
                                        .mask_addr_byte = reg_unit_.readRegister(ins.rs2, false)};

    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = pim_set_payload_.ins.ins_id, .unit_type = ExecuteUnitType::pim_set};
    cur_ins_conflict_info_.use_pim_unit = true;
    cur_ins_conflict_info_.addReadMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(pim_set_payload_.mask_addr_byte));
}

void Core::decodePimTransferIns(const pimsim::Instruction &ins, const pimsim::InstructionPayload &ins_payload) {
    InstructionPayload pim_transfer_ins_payload{
        .pc = ins_payload.pc, .ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::pim_transfer};

    pim_transfer_payload_ =
        PimTransferInsPayload{.ins = pim_transfer_ins_payload,
                              .output_num = reg_unit_.readRegister(ins.rs2, false),
                              .output_bit_width = reg_unit_.readRegister(SpecialRegId::pim_output_bit_width, true),
                              .output_mask_addr_byte = reg_unit_.readRegister(ins.rs3, false),
                              .src_addr_byte = reg_unit_.readRegister(ins.rs1, false),
                              .dst_addr_byte = reg_unit_.readRegister(ins.rd, false),
                              .buffer_addr_byte = reg_unit_.readRegister(ins.rs4, false)};

    cur_ins_conflict_info_ =
        DataConflictPayload{.ins_id = pim_transfer_payload_.ins.ins_id, .unit_type = ExecuteUnitType::pim_transfer};
    cur_ins_conflict_info_.addReadMemoryId(
        {local_memory_unit_.getLocalMemoryIdByAddress(pim_transfer_payload_.src_addr_byte),
         local_memory_unit_.getLocalMemoryIdByAddress(pim_transfer_payload_.output_mask_addr_byte)});
    cur_ins_conflict_info_.addWriteMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(pim_transfer_payload_.dst_addr_byte));
    cur_ins_conflict_info_.addReadWriteMemoryId(
        local_memory_unit_.getLocalMemoryIdByAddress(pim_transfer_payload_.buffer_addr_byte));
}

int Core::decodeControlInsAndGetPCIncrement(const pimsim::Instruction &ins, const InstructionPayload &ins_payload) {
    cur_ins_conflict_info_ = DataConflictPayload{.ins_id = ins_payload.ins_id, .unit_type = ExecuteUnitType::control};

    if (ins.type == ControlInstType::jmp) {
        return ins.offset;
    }

    int src_value1 = reg_unit_.readRegister(ins.rs1, false);
    int src_value2 = reg_unit_.readRegister(ins.rs2, false);
    bool branch = false;
    switch (ins.type) {
        case ControlInstType::beq: branch = (src_value1 == src_value2); break;
        case ControlInstType::bne: branch = (src_value1 != src_value2); break;
        case ControlInstType::bgt: branch = (src_value1 > src_value2); break;
        case ControlInstType::blt: branch = (src_value1 < src_value2); break;
    }
    if (branch) {
        return ins.offset;
    } else {
        return 1;
    }
}

}  // namespace pimsim
