//
// Created by wyk on 2025/3/24.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "payload.h"

namespace cimsim {

using MacroDynamicPowerFunc = double (*)(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
using MacroStageSocket = SubmoduleSocket<MacroSubmodulePayload>;

class MacroPipelineStage : public BaseModule {
public:
    SC_HAS_PROCESS(MacroPipelineStage);

    MacroPipelineStage(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core, Clock* clk,
                       const CimUnitConfig& config, const std::string& macro_name, const std::string& module_name,
                       MacroDynamicPowerFunc get_dynamic_power_func, int pipeline_stage_latency_cycle,
                       EnergyCounter& module_energy_counter);

    [[noreturn]] void processExecute();

public:
    MacroStageSocket exec_socket_;
    MacroStageSocket* next_stage_socket_{nullptr};
    bool last_batch_trigger_next_{false};

private:
    const CimUnitConfig& config_;
    const std::string& macro_name_;
    const std::string& module_name_;

    MacroDynamicPowerFunc get_dynamic_power_func_;
    int pipeline_stage_latency_cycle_;

    EnergyCounter& module_energy_counter_;
};

class MacroModule : public BaseModule {
public:
    MacroModule(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core, Clock* clk,
                const CimUnitConfig& config, const std::string& macro_name,
                MacroDynamicPowerFunc get_dynamic_power_func, int latency_cycle, int pipeline_stage_cnt);

    MacroStageSocket* getExecuteSocket() const;
    void bindNextStageSocket(MacroStageSocket* next_stage_socket, bool last_batch_trigger);

    void setStaticPower(double power);
    EnergyReporter getEnergyReporter() override;

private:
    std::vector<std::shared_ptr<MacroPipelineStage>> stage_list_{};
    EnergyCounter module_energy_counter_;
};

}  // namespace cimsim
