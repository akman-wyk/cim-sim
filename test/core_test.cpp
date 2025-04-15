//
// Created by wyk on 2024/8/12.
//

#include "base/test_macro.h"
#include "base/test_payload.h"
#include "base_component/clock.h"
#include "chip/chip.h"
#include "config/config.h"
#include "core/core.h"
#include "fmt/format.h"
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
    if (argc != 5) {
        std::cout << fmt::format("Usage: {} [config_file] [profiler_config_file] [instruction_file] [report_file]",
                                 exec_file_name)
                  << std::endl;
        return INVALID_USAGE;
    }

    auto* config_file = argv[1];
    auto* profiler_config_file = argv[2];
    auto* instruction_file = argv[3];
    auto* report_file = argv[4];

    auto config = readTypeFromJsonFile<Config>(config_file);
    auto profiler_config = readTypeFromJsonFile<ProfilerConfig>(profiler_config_file);
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return INVALID_CONFIG;
    }
    config.chip_config.network_config = {.bus_width_byte = 16,
                                         .network_config_file_path = "../test_data/chip_v2/network_config_3.json"};
    AddressSapce::initialize(config.chip_config);

    auto test_info = readTypeFromJsonFile<CoreTestInfo>(instruction_file);
    std::vector<std::vector<Instruction>> chip_code{test_info.code};
    Chip chip{"Chip", config, profiler_config, chip_code};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = chip.report(ofs, false);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getDynamicEnergyPJ(), test_info.expected.energy_pj) &&
        (!test_info.reg_info.check || chip.checkRegValues(0, test_info.reg_info.general_reg_expected_values,
                                                          test_info.reg_info.special_reg_expected_values))) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
