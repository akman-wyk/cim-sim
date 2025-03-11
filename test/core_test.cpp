//
// Created by wyk on 2024/8/12.
//

#include "base/test_macro.h"
#include "base/test_payload.h"
#include "base_component/clock.h"
#include "config/config.h"
#include "core/core.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "systemc.h"
#include "util/util.h"

namespace cimsim {

struct CoreTestInfo {
    std::vector<Instruction> code{};
    TestExpectedInfo expected;
    CoreTestRegisterInfo reg_info;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreTestInfo, code, expected, reg_info)

}  // namespace cimsim

using namespace cimsim;

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

    auto config = readTypeFromJsonFile<Config>(config_file);
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return INVALID_CONFIG;
    }

    auto test_info = readTypeFromJsonFile<CoreTestInfo>(instruction_file);
    Clock clk{"clock", config.sim_config.period_ns};
    sc_core::sc_time running_time;
    auto finish_run_call = [&running_time] {
        running_time = sc_core::sc_time_stamp();
        sc_stop();
        EnergyCounter::setRunningTimeNS(running_time);
    };
    Core core(0, "Core_0", config, &clk, std::move(test_info.code), finish_run_call);
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = Reporter{running_time.to_seconds() * 1000, core.getName(), core.getEnergyReporter(), 0};
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
