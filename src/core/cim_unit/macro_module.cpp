//
// Created by wyk on 2025/3/24.
//

#include "macro_module.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

MacroPipelineStage::MacroPipelineStage(const sc_module_name& name, const BaseInfo& base_info,
                                       const CimUnitConfig& config, MacroDynamicPowerFunc get_power, int latency_cycle,
                                       EnergyCounter& module_energy_counter, const std::string& module_name)
    : BaseModule(name, base_info)
    , config_(config)
    , get_power_(get_power)
    , latency_cycle_(latency_cycle)
    , module_energy_counter_(module_energy_counter)
    , module_name_(module_name) {
    SC_THREAD(processExecute)
}

void MacroPipelineStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        const auto& cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}, batch: {}", getFullName(), cim_ins_info.ins_pc,
                             cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW = get_power_(config_, payload);
        double latency = latency_cycle_ * period_ns_;
        module_energy_counter_.addDynamicEnergyPJ(std::max(latency, period_ns_), dynamic_power_mW,
                                                  {.core_id = core_id_,
                                                   .ins_id = cim_ins_info.ins_id,
                                                   .inst_opcode = cim_ins_info.inst_opcode,
                                                   .inst_group_tag = cim_ins_info.inst_group_tag,
                                                   .inst_profiler_operator = module_name_});
        wait(latency, SC_NS);

        if (next_stage_socket_ != nullptr && (!last_batch_trigger_next_ || payload.batch_info->last_batch)) {
            waitAndStartNextStage(payload, *next_stage_socket_);
        }

        exec_socket_.finish();
    }
}

MacroModule::MacroModule(const sc_module_name& name, const BaseInfo& base_info, const CimUnitConfig& config,
                         MacroDynamicPowerFunc get_power, int latency_cycle, int pipeline_stage_cnt)
    : BaseModule(name, base_info), module_energy_counter_(pipeline_stage_cnt > 1) {
    int pipeline_stage_latency_cycle = latency_cycle / pipeline_stage_cnt;
    stage_list_.emplace_back(std::make_shared<MacroPipelineStage>(
        "pipeline_0", base_info, config, get_power, pipeline_stage_latency_cycle, module_energy_counter_, getName()));

    for (int i = 1; i < pipeline_stage_cnt; i++) {
        auto stage_ptr =
            std::make_shared<MacroPipelineStage>(fmt::format("pipeline_{}", i).c_str(), base_info, config, get_power,
                                                 pipeline_stage_latency_cycle, module_energy_counter_, getName());
        stage_list_[i - 1]->next_stage_socket_ = &(stage_ptr->exec_socket_);
        stage_list_.emplace_back(stage_ptr);
    }
}

MacroModule::MacroModule(const sc_module_name& name, const BaseInfo& base_info, const CimUnitConfig& config,
                         const CimModuleConfig& module_config, MacroDynamicPowerFunc get_power)
    : MacroModule(name, base_info, config, get_power, module_config.latency_cycle, module_config.pipeline_stage_cnt) {}

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

EnergyCounter* MacroModule::getEnergyCounterPtr() {
    return &module_energy_counter_;
}

}  // namespace cimsim
