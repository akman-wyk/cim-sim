//
// Created by wyk on 2024/11/11.
//

#pragma once
#include "base_component/base_module.h"
#include "isa/instruction.h"
#include "memory/global_memory.h"

namespace pimsim {

class Chip : public BaseModule {
public:
    Chip(const char* name, const Config& config, const std::vector<std::vector<Instruction>>& core_ins_list);

    Reporter report(std::ostream& os, bool report_every_core_energy);

    EnergyReporter getEnergyReporter() override;

    EnergyReporter getCoresEnergyReporter();

private:
    void processFinishRun();

private:
    Clock clk_;
    std::vector<std::shared_ptr<Core>> core_list_;
    GlobalMemory global_memory_;
    Network network_;

    EnergyCounter energy_counter_;

    int finish_run_core_cnt_{0};
    sc_core::sc_time running_time_{};
};

}  // namespace pimsim
