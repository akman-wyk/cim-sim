//
// Created by wyk on 2024/8/8.
//

#include "core.h"

#include <utility>

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

Core::ExecuteUnitRegistration::ExecuteUnitRegistration(ExecuteUnitType type, ExecuteUnit *execute_unit,
                                                       sc_event &decode_new_ins_trigger)
    : type(type), execute_unit(execute_unit), stall_handler(decode_new_ins_trigger, type) {}

Core::Core(int core_id, const char *name, const Config &config, Clock *clk, std::vector<Instruction> ins_list,
           std::function<void()> finish_run_call)
    : BaseModule(name, config.sim_config, this, clk)
    , core_id_(core_id)
    , core_config_(config.chip_config.core_config)
    , ins_list_(std::move(ins_list))

    , cim_unit_("CimUnit", core_config_.cim_unit_config, config.sim_config, this, clk)
    , local_memory_unit_("LocalMemoryUnit", core_config_.local_memory_unit_config, config.sim_config, this, clk)
    , reg_unit_("RegUnit", core_config_.register_unit_config, config.sim_config, this, clk)
    , core_switch_("CoreSwitch", config.sim_config, this, clk, core_id)
    , decoder_("Decoder", config.chip_config, config.sim_config, this, clk)

    , scalar_unit_("ScalarUnit", core_config_.scalar_unit_config, config.sim_config, this, clk)
    , simd_unit_("SIMDUnit", core_config_.simd_unit_config, config.sim_config, this, clk)
    , transfer_unit_("TransferUnit", core_config_.transfer_unit_config, config.sim_config, this, clk, core_id,
                     config.chip_config.global_memory_config.global_memory_switch_id)
    , cim_compute_unit_("CimComputeUnit", core_config_.cim_unit_config, config.sim_config, this, clk)
    , cim_control_unit_("CimControlUnit", core_config_.cim_unit_config, config.sim_config, this, clk)

    , finish_run_call_(std::move(finish_run_call)) {
    setThreadAndMethod();
    bindModules();
}

void Core::bindNetwork(Network *network) {
    core_switch_.bindNetwork(network);
}

EnergyReporter Core::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("ScalarUnit", EnergyReporter{scalar_unit_.getEnergyReporter()});
    reporter.addSubModule("SIMDUnit", EnergyReporter{simd_unit_.getEnergyReporter()});
    reporter.addSubModule("CimUnit", EnergyReporter{cim_compute_unit_.getEnergyReporter()});
    reporter.addSubModule("CimUnit", EnergyReporter{cim_control_unit_.getEnergyReporter()});
    reporter.addSubModule("LocalMemoryUnit", EnergyReporter{local_memory_unit_.getEnergyReporter()});
    return std::move(reporter);
}

bool Core::checkRegValues(const std::array<int, GENERAL_REG_NUM> &general_reg_expected_values,
                          const std::array<int, SPECIAL_REG_NUM> &special_reg_expected_values) {
    return reg_unit_.checkRegValues(general_reg_expected_values, special_reg_expected_values);
}

bool Core::checkInsStat(const std::string &expected_ins_stat_file) const {
    // return decoder_.checkInsStat(expected_ins_stat_file);
    return true;
}

int Core::getCoreId() const {
    return core_id_;
}

[[noreturn]] void Core::processDecode() {
    wait(period_ns_ - 1, SC_NS);

    std::shared_ptr<ExecuteInsPayload> payload{nullptr};
    while (true) {
        if (cur_ins_conflict_info_.unit_type == +ExecuteUnitType::none) {
            if (ins_index_ < ins_list_.size()) {
                cur_ins_payload_ =
                    decoder_.decode(ins_list_[ins_index_], ins_index_ + 1, pc_increment_, cur_ins_conflict_info_);
                decode_new_ins_trigger_.notify();
            } else {
                pc_increment_ = 0;
                id_finish_.write(true);
            }
        }
        wait(period_ns_, SC_NS);
    }
}

[[noreturn]] void Core::processUpdatePC() {
    while (true) {
        if (!id_stall_.read()) {
            ins_index_ += pc_increment_;
            cur_ins_conflict_info_ = ResourceAllocatePayload{.ins_id = -1, .unit_type = ExecuteUnitType::none};
        }
        wait(period_ns_, SC_NS);
    }
}

void Core::processIssue() {
    if (cur_ins_payload_ != nullptr) {
        for (auto &registration : execute_unit_list_) {
            if (registration->type == cur_ins_payload_->ins.unit_type) {
                registration->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = cur_ins_payload_});
            } else {
                registration->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = nullptr});
            }
        }
    }
}

void Core::processStall() {
    bool stall = id_finish_.read() || std::any_of(execute_unit_list_.begin(), execute_unit_list_.end(),
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
    if (std::all_of(execute_unit_list_.begin(), execute_unit_list_.end(),
                    [](const std::shared_ptr<ExecuteUnitRegistration> &registration) {
                        return registration->signals.unit_finish_.read();
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
    sensitive << processFinishRun_handle_ << registration->signals.unit_finish_;

    // bind exeunit ports
    registration->execute_unit->ports_.bind(registration->signals);
    registration->execute_unit->ports_.id_finish_port_.bind(id_finish_);

    // bind stall handler
    registration->stall_handler.bind(registration->signals, registration->conflict_signal, &cur_ins_conflict_info_);

    // bind decoder
    decoder_.bindExecuteUnit(type, execute_unit);
}
void Core::bindModules() {
    // bind and set modules
    int end_pc = static_cast<int>(ins_list_.size());
    scalar_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    scalar_unit_.bindRegUnit(&reg_unit_);

    simd_unit_.bindLocalMemoryUnit(&local_memory_unit_);

    transfer_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    transfer_unit_.bindSwitch(&core_switch_);

    cim_compute_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    cim_compute_unit_.bindCimUnit(&cim_unit_);

    cim_control_unit_.bindLocalMemoryUnit(&local_memory_unit_);
    cim_control_unit_.bindCimUnit(&cim_unit_);

    local_memory_unit_.bindCimUnit(&cim_unit_);

    decoder_.bindRegUnit(&reg_unit_);

    // register execute unit
    registerExecuteUnit(ExecuteUnitType::scalar, &scalar_unit_);
    registerExecuteUnit(ExecuteUnitType::simd, &simd_unit_);
    registerExecuteUnit(ExecuteUnitType::transfer, &transfer_unit_);
    registerExecuteUnit(ExecuteUnitType::cim_compute, &cim_compute_unit_);
    registerExecuteUnit(ExecuteUnitType::cim_control, &cim_control_unit_);
}

void Core::setThreadAndMethod() {
    SC_THREAD(processDecode)
    SC_THREAD(processUpdatePC)

    SC_METHOD(processIssue)
    sensitive << decode_new_ins_trigger_ << id_stall_;

    SC_METHOD(processIdExEnable)
    sensitive << id_stall_;

    processStall_handle_ = sc_core::sc_get_curr_simcontext()->create_method_process(
        "processStall", false, static_cast<sc_core::SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processStall), this,
        nullptr);
    sensitive << processStall_handle_ << id_finish_;

    processFinishRun_handle_ = sc_core::sc_get_curr_simcontext()->create_method_process(
        "processFinishRun", false, static_cast<sc_core::SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processFinishRun), this,
        nullptr);
}

}  // namespace cimsim
