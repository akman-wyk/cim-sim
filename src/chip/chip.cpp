//
// Created by wyk on 2024/11/11.
//

#include "chip.h"

#include "fmt/format.h"

namespace cimsim {

Chip::Chip(const sc_module_name& name, const Config& config, const std::vector<std::vector<Instruction>>& core_ins_list)
    : BaseModule(name, BaseInfo{.sim_config = config.sim_config})
    , clk_("Clock", config.sim_config.period_ns)
    , global_memory_("GlobalMemory", config.chip_config.global_memory_config, config.sim_config)
    , network_("Network", config.chip_config.network_config, config.sim_config) {
    int global_id = config.chip_config.global_memory_config.global_memory_switch_id;
    for (int core_id = 0; core_id < config.chip_config.core_cnt; core_id++) {
        std::string core_name = fmt::format("Core_{}", core_id);
        BaseInfo base_info{config.sim_config, core_id};
        auto core = std::make_shared<Core>(
            core_name.c_str(), config.chip_config.core_config, base_info, &clk_, global_id, core_ins_list[core_id],
            [this]() { this->processFinishRun(); }, &core_overview_energy_counter_);
        core->bindNetwork(&network_);
        core_list_.emplace_back(std::move(core));
    }
    global_memory_.bindNetwork(&network_);
}

Reporter Chip::report(std::ostream& os, bool report_every_core_energy) {
    EnergyCounter::setRunningTimeNS(running_time_);
    Reporter reporter{running_time_.to_seconds() * 1000, getName(), getEnergyReporter(), 0};
    reporter.report(os);

    if (report_every_core_energy) {
        Reporter cores_reporter{running_time_.to_seconds() * 1000, "Cores", getCoresEnergyReporter(), 0};
        os << "\nEvery core energy form:\n";
        cores_reporter.reportEnergyForm(os);
    }

    return std::move(reporter);
}

EnergyReporter Chip::getEnergyReporter() {
    EnergyReporter reporter{0, 0, 0, EnergyCounter::getRunningTimeNS()};
    reporter.addSubModule("Core Overview", core_overview_energy_counter_.getEnergyReporter());
    reporter.addSubModule("GlobalMemory", global_memory_.getEnergyReporter());
    reporter.addSubModule("Network", network_.getEnergyReporter());
    return std::move(reporter);
}

EnergyReporter Chip::getCoresEnergyReporter() {
    EnergyReporter core_list_energy_reporter;
    for (auto& core : core_list_) {
        core_list_energy_reporter.addSubModule(core->getName(), core->getEnergyReporter());
    }
    return std::move(core_list_energy_reporter);
}

void Chip::processFinishRun() {
    finish_run_core_cnt_++;
    if (finish_run_core_cnt_ == core_list_.size()) {
        running_time_ = sc_time_stamp();
        sc_stop();
    }
}

}  // namespace cimsim
