//
// Created by wyk on 2024/8/13.
//

#pragma once
#include <string>

#include "chip/chip.h"
#include "config/config.h"

namespace cimsim {

class LayerSimulator {
public:
    LayerSimulator(std::string config_file, std::string profiler_config_file, std::string instruction_file, bool check);

    void run();

    void report(std::ostream& os, const std::string& report_json_file, bool report_every_core_energy);

    // [[nodiscard]] bool checkInsStat() const;
    // [[nodiscard]] bool checkReg() const;

private:
    [[nodiscard]] std::vector<std::vector<Instruction>> getCoreInstructionList() const;

private:
    std::shared_ptr<Chip> chip_;

    Config config_;
    ProfilerConfig profiler_config_;

    std::string config_file_;
    std::string profiler_config_file_;
    std::string instruction_file_;
    // std::string global_image_file_;
    // std::string expected_ins_stat_file_;
    // std::string expected_reg_file_;
    // std::string actual_reg_file_;
    bool check_;
};

}  // namespace cimsim
