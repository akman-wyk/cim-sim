//
// Created by wyk on 2024/11/11.
//

#pragma once
#include "base_component/base_module.h"
#include "core/core.h"
#include "memory/global_memory.h"
#include "profiler/profiler.h"

namespace cimsim {

class Chip : public BaseModule {
public:
    Chip(const sc_module_name& name, const Config& config, const ProfilerConfig& profiler_config,
         const std::vector<std::vector<Instruction>>& core_ins_list);

    Reporter report(std::ostream& os, bool report_every_core_energy);

    EnergyReporter getEnergyReporter() const;

    EnergyReporter getCoresEnergyReporter() const;

    bool checkRegValues(int core_id, const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                        const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values) const;

private:
    void processFinishRun();

private:
    Clock clk_;
    std::vector<std::shared_ptr<Core>> core_list_;
    GlobalMemory global_memory_;
    Network network_;

    int finish_run_core_cnt_{0};
    sc_time running_time_{};

    Profiler profiler_;
};

}  // namespace cimsim
