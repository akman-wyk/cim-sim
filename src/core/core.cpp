//
// Created by wyk on 2024/8/8.
//

#include "core.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

Core::ExecuteUnitInfo::ExecuteUnitInfo(ExecuteUnitType type, ExecuteUnit *execute_unit,
                                       const sc_event &decode_new_ins_trigger)
    : type(type)
    , execute_unit(execute_unit)
    , stall_handler(fmt::format("StallHandler_{}", type._to_string()).c_str(), decode_new_ins_trigger, type)
    , signals(type)
    , conflict_signal(fmt::format("{}_conflict_signal", type._to_string()).c_str()) {}

Core::Core(const sc_module_name &name, const CoreConfig &config, const BaseInfo &base_info, Clock *clk, int global_id,
           std::vector<Instruction> ins_list, std::function<void()> finish_run_call)
    : BaseModule(name, base_info)
    , core_config_(config)
    , ins_list_(std::move(ins_list))

    , cim_unit_("CimUnit", core_config_.cim_unit_config, base_info)
    , local_memory_unit_("LocalMemoryUnit", core_config_.local_memory_unit_config, base_info, false)
    , reg_unit_("RegUnit", core_config_.register_unit_config, base_info)
    , core_switch_("Switch", base_info)
    , decoder_("Decoder", config, base_info)

    , scalar_unit_("ScalarUnit", core_config_.scalar_unit_config, base_info, clk)
    , simd_unit_("SIMDUnit", core_config_.simd_unit_config, base_info, clk)
    , reduce_unit_("ReduceUnit", config.reduce_unit_config, base_info, clk)
    , transfer_unit_("TransferUnit", core_config_.transfer_unit_config, base_info, clk, global_id)
    , cim_compute_unit_("CimComputeUnit", core_config_.cim_unit_config, base_info, clk)
    , cim_control_unit_("CimControlUnit", core_config_.cim_unit_config, base_info, clk)

    , finish_run_call_(std::move(finish_run_call)) {
    setThreadAndMethod();
    bindModules();
}

void Core::bindNetwork(Network *network) {
    core_switch_.bindNetwork(network);
}

EnergyReporter Core::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("ScalarUnit", scalar_unit_.getEnergyReporter());
    reporter.addSubModule("SIMDUnit", simd_unit_.getEnergyReporter());
    reporter.addSubModule("ReduceUnit", reduce_unit_.getEnergyReporter());
    reporter.addSubModule("CimUnit", cim_compute_unit_.getEnergyReporter());
    reporter.addSubModule("CimUnit", cim_control_unit_.getEnergyReporter());
    reporter.addSubModule("LocalMemoryUnit", local_memory_unit_.getEnergyReporter());
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
        for (auto &exe_unit_info : execute_unit_list_) {
            if (exe_unit_info->type == cur_ins_payload_->ins.unit_type) {
                exe_unit_info->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = cur_ins_payload_});
            } else {
                exe_unit_info->signals.id_ex_payload_.write(ExecuteUnitPayload{.payload = nullptr});
            }
        }
    }
}

void Core::processStall() {
    bool stall = id_finish_.read() || std::any_of(execute_unit_list_.begin(), execute_unit_list_.end(),
                                                  [](const std::shared_ptr<ExecuteUnitInfo> &exe_unit_info) {
                                                      return exe_unit_info->conflict_signal.read();
                                                  });
    id_stall_.write(stall);
}

void Core::processIdExEnable() {
    for (auto &exe_unit_info : execute_unit_list_) {
        exe_unit_info->signals.id_ex_enable_.write(!id_stall_.read());
    }
}

void Core::processFinishRun() {
    if (std::all_of(execute_unit_list_.begin(), execute_unit_list_.end(),
                    [](const std::shared_ptr<ExecuteUnitInfo> &exe_unit_info) {
                        return exe_unit_info->signals.unit_finish_.read();
                    })) {
        CORE_LOG(fmt::format("finish run"));
        finish_run_call_();
    }
}

void Core::bindExecuteUnit(ExecuteUnitType type, ExecuteUnit *execute_unit) {
    auto exe_unit_info = std::make_shared<ExecuteUnitInfo>(type, execute_unit, decode_new_ins_trigger_);
    execute_unit_list_.emplace_back(exe_unit_info);

    // bind local memory unit
    execute_unit->bindLocalMemoryUnit(&local_memory_unit_);

    // bind handler of processStall and processFinishRun
    sensitive << processStall_handle_ << exe_unit_info->conflict_signal;
    sensitive << processFinishRun_handle_ << exe_unit_info->signals.unit_finish_;

    // bind exeunit ports
    exe_unit_info->execute_unit->ports_.bind(exe_unit_info->signals);
    exe_unit_info->execute_unit->ports_.id_finish_port_.bind(id_finish_);

    // bind stall handler
    exe_unit_info->stall_handler.bind(exe_unit_info->signals, exe_unit_info->conflict_signal, &cur_ins_conflict_info_);

    // bind decoder
    decoder_.bindExecuteUnit(type, execute_unit);
}
void Core::bindModules() {
    // bind execute unit
    bindExecuteUnit(ExecuteUnitType::scalar, &scalar_unit_);
    bindExecuteUnit(ExecuteUnitType::simd, &simd_unit_);
    bindExecuteUnit(ExecuteUnitType::reduce, &reduce_unit_);
    bindExecuteUnit(ExecuteUnitType::transfer, &transfer_unit_);
    bindExecuteUnit(ExecuteUnitType::cim_compute, &cim_compute_unit_);
    bindExecuteUnit(ExecuteUnitType::cim_control, &cim_control_unit_);

    // other bindings
    decoder_.bindRegUnit(&reg_unit_);
    scalar_unit_.bindRegUnit(&reg_unit_);

    transfer_unit_.bindSwitch(&core_switch_);

    cim_compute_unit_.bindCimUnit(&cim_unit_);
    cim_control_unit_.bindCimUnit(&cim_unit_);

    local_memory_unit_.mountMemory(&cim_unit_);
}

void Core::setThreadAndMethod() {
    SC_THREAD(processDecode)
    SC_THREAD(processUpdatePC)

    SC_METHOD(processIssue)
    sensitive << decode_new_ins_trigger_ << id_stall_;

    SC_METHOD(processIdExEnable)
    sensitive << id_stall_;

    processStall_handle_ = sc_get_curr_simcontext()->create_method_process(
        "processStall", false, static_cast<SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processStall), this, nullptr);
    sensitive << processStall_handle_ << id_finish_;

    processFinishRun_handle_ = sc_get_curr_simcontext()->create_method_process(
        "processFinishRun", false, static_cast<SC_ENTRY_FUNC>(&SC_CURRENT_USER_MODULE::processFinishRun), this,
        nullptr);
}

}  // namespace cimsim
