//
// Created by wyk on 2024/11/11.
//

#include "chip.h"

#include "fmt/format.h"

namespace cimsim {

Chip::Chip(const sc_module_name& name, const Config& config, const ProfilerConfig& profiler_config,
           const std::vector<std::vector<Instruction>>& core_ins_list)
    : BaseModule(name, BaseInfo{.sim_config = config.sim_config})
    , clk_("Clock", config.sim_config.period_ns)
    , global_memory_("GlobalMemory", config.chip_config.global_memory_config, config.sim_config)
    , network_("Network", config.chip_config.network_config, config.sim_config)
    , profiler_(profiler_config) {
    int global_id = config.chip_config.global_memory_config.global_memory_switch_id;
    for (int core_id = 0; core_id < config.chip_config.core_cnt; core_id++) {
        std::string core_name = fmt::format("Core_{}", core_id);
        BaseInfo base_info{config.sim_config, core_id};
        auto core = std::make_shared<Core>(core_name.c_str(), config.chip_config.core_config, base_info, &clk_,
                                           global_id, core_ins_list[core_id], [this]() { this->processFinishRun(); });
        core->bindNetwork(&network_);
        core_list_.emplace_back(core);

        energy_counter_.addSubEnergyCounter("Core Overview", core->getEnergyCounterPtr());
    }
    global_memory_.bindNetwork(&network_);

    energy_counter_.addSubEnergyCounter("GlobalMemory", global_memory_.getEnergyCounterPtr());
    energy_counter_.addSubEnergyCounter("Network", network_.getEnergyCounterPtr());

    profiler_.bindHardware(&energy_counter_, core_list_);
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
    profiler_.report(os, reporter.getLatencyNs());
    return std::move(reporter);
}

EnergyReporter Chip::getEnergyReporter() const {
    return energy_counter_.getEnergyReporter();
}

EnergyReporter Chip::getCoresEnergyReporter() const {
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
        profiler_.finishRun();
    }
}

bool Chip::checkRegValues(int core_id, const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                          const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values) const {
    return core_list_[core_id]->checkRegValues(general_reg_expected_values, special_reg_expected_values);
}

}  // namespace cimsim
