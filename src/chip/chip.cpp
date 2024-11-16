//
// Created by wyk on 2024/11/11.
//

#include "chip.h"

#include "core/core.h"
#include "fmt/format.h"

namespace pimsim {

Chip::Chip(const char* name, const Config& config, const std::vector<std::vector<Instruction>>& core_ins_list)
    : BaseModule(name, config.sim_config, nullptr, nullptr)
    , clk_("Clock", config.sim_config.period_ns)
    , global_memory_("GlobalMemory", config.chip_config.global_memory_config, config.sim_config, &clk_)
    , network_("Network", config.chip_config.network_config, config.sim_config) {
    for (int core_id = 0; core_id < config.chip_config.core_cnt; core_id++) {
        std::string core_name = fmt::format("Core_{}", core_id);
        auto core = std::make_shared<Core>(core_id, core_name.c_str(), config, &clk_, core_ins_list[core_id],
                                           [this]() { this->processFinishRun(); });
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
    EnergyReporter energy_reporter, core_total_energy_reporter;
    for (auto& core : core_list_) {
        core_total_energy_reporter.accumulate(core->getEnergyReporter(), true);
    }
    energy_reporter.addSubModule("Core Overview", core_total_energy_reporter);
    energy_reporter.addSubModule("GlobalMemory", global_memory_.getEnergyReporter());
    energy_reporter.addSubModule("Network", network_.getEnergyReporter());
    return std::move(energy_reporter);
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
        running_time_ = sc_core::sc_time_stamp();
        sc_stop();
    }
}

}  // namespace pimsim
