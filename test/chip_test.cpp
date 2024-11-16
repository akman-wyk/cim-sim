//
// Created by wyk on 2024/11/11.
//

#include <vector>

#include "base/test_macro.h"
#include "base/test_payload.h"
#include "chip/chip.h"
#include "config/config.h"
#include "fmt/format.h"
#include "isa/instruction.h"
#include "systemc.h"
#include "util/util.h"

namespace pimsim {

struct ChipTestInfo {
    std::vector<std::vector<Instruction>> code;
    TestExpectedInfo expected;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ChipTestInfo, code, expected);

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
    auto test_info = ins_j.get<ChipTestInfo>();

    Chip chip{"Chip", config, test_info.code};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = chip.report(ofs, true);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getDynamicEnergyPJ(), test_info.expected.energy_pj)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
