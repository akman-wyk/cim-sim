//
// Created by wyk on 2024/8/12.
//

#include "base/test_macro.h"
#include "base/test_payload.h"
#include "base_component/clock.h"
#include "config/config.h"
#include "core/core.h"
#include "fmt/format.h"
#include "isa/instruction.h"
#include "nlohmann/json.hpp"
#include "systemc.h"
#include "util/util.h"

namespace pimsim {

struct CoreTestRegisterInfo {
    bool check{false};
    std::array<int, GENERAL_REG_NUM> general_reg_expected_values{};
    std::array<int, SPECIAL_REG_NUM> special_reg_expected_values{};
};

struct CoreTestInfo {
    std::vector<Instruction> code{};
    TestExpectedInfo expected;
    CoreTestRegisterInfo reg_info;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreTestRegisterInfo, check, general_reg_expected_values,
                                               special_reg_expected_values)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreTestInfo, code, expected, reg_info)

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    std::string exec_file_name{argv[0]};
    if (argc != 4) {
        std::cout << fmt::format("Usage: {} [config_file] [instruction_file] [report_file]", exec_file_name)
                  << std::endl;
        return INVALID_USAGE;
    }

    auto* config_file = argv[1];
    auto* instruction_file = argv[2];
    auto* report_file = argv[3];

    std::ifstream config_ifs;
    config_ifs.open(config_file);
    nlohmann::ordered_json config_j = nlohmann::ordered_json::parse(config_ifs);
    config_ifs.close();
    auto config = config_j.get<Config>();
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return INVALID_CONFIG;
    }

    std::ifstream ins_ifs;
    ins_ifs.open(instruction_file);
    nlohmann::ordered_json ins_j = nlohmann::ordered_json::parse(ins_ifs);
    ins_ifs.close();
    auto test_info = ins_j.get<CoreTestInfo>();

    Clock clk{"clock", config.sim_config.period_ns};
    Core core{"core", config, &clk, std::move(test_info.code), std::cout};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = core.getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getDynamicEnergyPJ(), test_info.expected.energy_pj) &&
        (!test_info.reg_info.check || core.checkRegValues(test_info.reg_info.general_reg_expected_values,
                                                          test_info.reg_info.special_reg_expected_values))) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
