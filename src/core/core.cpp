//
// Created by wyk on 2024/8/8.
//

#include "core.h"

#include <utility>

#include "fmt/format.h"
#include "isa/isa.h"
#include "util/log.h"

namespace pimsim {

ExecuteUnitRegistration::ExecuteUnitRegistration(ExecuteUnitType type, ExecuteUnit *execute_unit,
                                                 sc_event &decode_new_ins_trigger)
    : type(type), execute_unit(execute_unit), stall_handler(decode_new_ins_trigger, type) {}

Core::Core(int core_id, const char *name, const Config &config, Clock *clk, std::vector<Instruction> ins_list,
           std::function<void()> finish_run_call)
    : BaseModule(name, config.sim_config, this, clk)
    , core_id_(core_id)
    , core_config_(config.chip_config.core_config)

    , ins_list_(std::move(ins_list))
    , scalar_unit_("ScalarUnit", core_config_.scalar_unit_config, config.sim_config, this, clk)
    , simd_unit_("SIMDUnit", core_config_.simd_unit_config, config.sim_config, this, clk)
    , transfer_unit_("TransferUnit", core_config_.transfer_unit_config, config.sim_config, this, clk, core_id,
                     config.chip_config.global_memory_config.global_memory_switch_id)

    , cim_unit_("CimUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_compute_unit_("PimComputeUnit", core_config_.pim_unit_config, config.sim_config, this, clk)
    , pim_control_unit_("PimControlUnit", core_config_.pim_unit_config, config.sim_config, this, clk)

    , local_memory_unit_("LocalMemoryUnit", core_config_.local_memory_unit_config, config.sim_config, this, clk)
    , reg_unit_("RegUnit", core_config_.register_unit_config, config.sim_config, this, clk)
    , core_switch_("CoreSwitch", config.sim_config, this, clk, core_id)
    , decoder_("Decoder", config.chip_config, config.sim_config, this, clk)

    , finish_run_call_(std::move(finish_run_call)) {
    SC_THREAD(issue)

    processStall_handle_ = sc_core::sc_get_curr_simcontext()->create_method_process(
        "processStall", false, static_cast<sc_core::SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processStall), this,
        nullptr);
    processFinishRun_handle_ = sc_core::sc_get_curr_simcontext()->create_method_process(
        "processFinishRun", false, static_cast<sc_core::SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processFinishRun), this,
        nullptr);

    SC_METHOD(processIdExEnable)
    sensitive << id_stall_;

    // bind and set modules
    int end_pc = static_cast<int>(ins_list_.size());
    scalar_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    scalar_unit_.bindRegUnit(&reg_unit_);
    scalar_unit_.setEndPC(end_pc);

    simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    simd_unit_.setEndPC(end_pc);

    transfer_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    transfer_unit_.bindSwitch(&core_switch_);
    transfer_unit_.setEndPC(end_pc);

    pim_compute_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_compute_unit_.bindCimUnit(&cim_unit_);
    pim_compute_unit_.setEndPC(end_pc);

    pim_control_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    pim_control_unit_.bindCimUnit(&cim_unit_);
    pim_control_unit_.setEndPC(end_pc);

    reg_unit_.write_req_port_.bind(write_req_signal_);
    reg_unit_.read_req_port_.bind(read_req_signal_);
    reg_unit_.read_rsp_port_.bind(read_rsp_signal_);

    local_memory_unit_.bindCimUnit(&cim_unit_);

    decoder_.bindRegUnit(&reg_unit_);

    // register execute unit
    registerExecuteUnit(ExecuteUnitType::scalar, &scalar_unit_);
    registerExecuteUnit(ExecuteUnitType::simd, &simd_unit_);
    registerExecuteUnit(ExecuteUnitType::transfer, &transfer_unit_);
    registerExecuteUnit(ExecuteUnitType::pim_compute, &pim_compute_unit_);
    registerExecuteUnit(ExecuteUnitType::pim_control, &pim_control_unit_);
}

void Core::bindNetwork(Network *network) {
    core_switch_.bindNetwork(network);
}

EnergyReporter Core::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("ScalarUnit", EnergyReporter{scalar_unit_.getEnergyReporter()});
    reporter.addSubModule("SIMDUnit", EnergyReporter{simd_unit_.getEnergyReporter()});
    reporter.addSubModule("PimUnit", EnergyReporter{pim_compute_unit_.getEnergyReporter()});
    reporter.addSubModule("PimUnit", EnergyReporter{pim_control_unit_.getEnergyReporter()});
    reporter.addSubModule("LocalMemoryUnit", EnergyReporter{local_memory_unit_.getEnergyReporter()});
    return std::move(reporter);
}

bool Core::checkRegValues(const std::array<int, GENERAL_REG_NUM> &general_reg_expected_values,
                          const std::array<int, SPECIAL_REG_NUM> &special_reg_expected_values) {
    return reg_unit_.checkRegValues(general_reg_expected_values, special_reg_expected_values);
}

bool Core::checkInsStat(const std::string &expected_ins_stat_file) const {
    return decoder_.checkInsStat(expected_ins_stat_file);
}

int Core::getCoreId() const {
    return core_id_;
}

[[noreturn]] void Core::issue() {
    wait(period_ns_ - 1, SC_NS);

    int pc_increment = 0;
    std::shared_ptr<ExecuteInsPayload> payload{nullptr};
    while (true) {
        if (cur_ins_conflict_info_.unit_type == +ExecuteUnitType::none) {
            if (ins_index_ < ins_list_.size()) {
                payload = decoder_.decode(ins_list_[ins_index_], ins_index_ + 1, pc_increment, cur_ins_conflict_info_);
                decode_new_ins_trigger_.notify();
            } else {
                pc_increment = 0;
            }
        }
        wait(0.1, SC_NS);

        if (!id_stall_.read() && cur_ins_conflict_info_.unit_type != +ExecuteUnitType::none) {
            if (payload != nullptr) {
                for (auto &registration : execute_unit_list_) {
                    if (registration->type == payload->ins.unit_type) {
                        registration->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = payload});
                    } else {
                        registration->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = nullptr});
                    }
                }
            }

            ins_index_ += pc_increment;
            cur_ins_conflict_info_ = DataConflictPayload{.ins_id = -1, .unit_type = ExecuteUnitType::none};
        } else {
            for (auto &registration : execute_unit_list_) {
                registration->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = nullptr});
            }
        }
        wait(period_ns_ - 0.1, SC_NS);
    }
}

void Core::processStall() {
    bool stall = std::any_of(execute_unit_list_.begin(), execute_unit_list_.end(),
                             [](const std::shared_ptr<ExecuteUnitRegistration> &registration) {
                                 return registration->conflict_signal.read();
                             });
    id_stall_.write(stall);
}

void Core::processIdExEnable() {
    for (auto &registration : execute_unit_list_) {
        registration->signals.id_ex_enable_.write(!id_stall_.read());
    }
}

void Core::processFinishRun() {
    if (std::any_of(execute_unit_list_.begin(), execute_unit_list_.end(),
                    [](const std::shared_ptr<ExecuteUnitRegistration> &registration) {
                        return registration->signals.finish_run_.read();
                    })) {
        LOG(fmt::format("finish run"));
        finish_run_call_();
    }
}

void Core::registerExecuteUnit(ExecuteUnitType type, ExecuteUnit *execute_unit) {
    auto registration = std::make_shared<ExecuteUnitRegistration>(type, execute_unit, decode_new_ins_trigger_);
    execute_unit_list_.emplace_back(registration);

    // bind handler of processStall and processFinishRun
    sensitive << processStall_handle_ << registration->conflict_signal;
    sensitive << processFinishRun_handle_ << registration->signals.finish_run_;

    // bind exeunit ports
    registration->execute_unit->ports_.bind(registration->signals);

    // bind stall handler
    registration->stall_handler.bind(registration->signals, registration->conflict_signal, &cur_ins_conflict_info_);

    // bind decoder
    decoder_.bindExecuteUnit(type, execute_unit);
}

}  // namespace pimsim
