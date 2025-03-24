//
// Created by wyk on 2025/3/24.
//

#include "macro_module.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

MacroPipelineStage::MacroPipelineStage(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core,
                                       Clock* clk, const CimUnitConfig& config, const std::string& macro_name,
                                       const std::string& module_name, MacroDynamicPowerFunc get_dynamic_power_func,
                                       int pipeline_stage_latency_cycle, EnergyCounter& module_energy_counter)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_name_(macro_name)
    , module_name_(module_name)
    , get_dynamic_power_func_(get_dynamic_power_func)
    , pipeline_stage_latency_cycle_(pipeline_stage_latency_cycle)
    , module_energy_counter_(module_energy_counter) {
    SC_THREAD(processExecute)
}

void MacroPipelineStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        const auto& cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} {} {} start, ins pc: {}, sub ins num: {}, batch: {}", macro_name_, module_name_,
                             getName(), cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW = get_dynamic_power_func_(config_, payload);
        double latency = pipeline_stage_latency_cycle_ * period_ns_;
        module_energy_counter_.addDynamicEnergyPJ(std::max(latency, period_ns_), dynamic_power_mW);
        wait(latency, SC_NS);

        if (next_stage_socket_ != nullptr && (!last_batch_trigger_next_ || payload.batch_info->last_batch)) {
            waitAndStartNextStage(payload, *next_stage_socket_);
        }

        exec_socket_.finish();
    }
}

MacroModule::MacroModule(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core, Clock* clk,
                         const CimUnitConfig& config, const std::string& macro_name,
                         MacroDynamicPowerFunc get_dynamic_power_func, int latency_cycle, int pipeline_stage_cnt)
    : BaseModule(name, sim_config, core, clk), module_energy_counter_(pipeline_stage_cnt > 1) {
    int pipeline_stage_latency_cycle = latency_cycle / pipeline_stage_cnt;
    stage_list_.emplace_back(std::make_shared<MacroPipelineStage>(
        "pipeline_0", sim_config, core, clk, config, macro_name, getName(), get_dynamic_power_func,
        pipeline_stage_latency_cycle, module_energy_counter_));

    for (int i = 1; i < pipeline_stage_cnt; i++) {
        auto stage_ptr = std::make_shared<MacroPipelineStage>(
            fmt::format("pipeline_{}", i).c_str(), sim_config, core, clk, config, macro_name, getName(),
            get_dynamic_power_func, pipeline_stage_latency_cycle, module_energy_counter_);
        stage_list_[i - 1]->next_stage_socket_ = &(stage_ptr->exec_socket_);
        stage_list_.emplace_back(stage_ptr);
    }
}

MacroStageSocket* MacroModule::getExecuteSocket() const {
    return &(stage_list_[0]->exec_socket_);
}

void MacroModule::bindNextStageSocket(MacroStageSocket* next_stage_socket, bool last_batch_trigger) {
    auto& last_stage_ptr = stage_list_[stage_list_.size() - 1];
    last_stage_ptr->next_stage_socket_ = next_stage_socket;
    last_stage_ptr->last_batch_trigger_next_ = last_batch_trigger;
}

void MacroModule::setStaticPower(double power) {
    module_energy_counter_.setStaticPowerMW(power);
}

EnergyReporter MacroModule::getEnergyReporter() {
    return EnergyReporter{module_energy_counter_};
}

}  // namespace cimsim
