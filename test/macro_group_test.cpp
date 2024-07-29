//
// Created by wyk on 2024/7/25.
//

#include <vector>

#include "base_component/base_module.h"
#include "core/pim_unit/macro_group.h"
#include "core/pim_unit/pim_payload.h"
#include "nlohmann/json.hpp"
#include "test_macro.h"
#include "util/macro_scope.h"
#include "util/util.h"

namespace pimsim {

struct MacroGroupExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

struct MacroGroupTestInfo {
    std::vector<MacroGroupPayload> code{};
    MacroGroupExpectedInfo expected{};
};

class MacroGroupTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroupTestModule);

    MacroGroupTestModule(const char* name, const Config& config, Clock* clk, std::vector<MacroGroupPayload> codes)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , macro_group_("MacroGroup_0", config.chip_config.core_config.pim_unit_config, config.sim_config, nullptr,
                       clk) {
        macro_group_ins_list_ = std::move(codes);

        SC_THREAD(issue)

        macro_group_.setFinishRunFunc([&]() {
            wait(SC_ZERO_TIME);
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        });
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(macro_group_.getName(), macro_group_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1e3, "MacroGroupTestModule", getEnergyReporter(), 0};
    }

private:
    void issue() {
        wait(10, SC_NS);
        for (auto& macro_group_ins : macro_group_ins_list_) {
            macro_group_.waitUntilFinishIfBusy();
            macro_group_.startExecute(std::move(macro_group_ins));
            wait(SC_ZERO_TIME);
        }
    }

private:
    std::vector<MacroGroupPayload> macro_group_ins_list_;

    MacroGroup macro_group_;

    sc_core::sc_time running_time_;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimInsInfo, ins_pc, sub_ins_num, last_ins, last_sub_ins)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupPayload, pim_ins_info, last_group, row, input_bit_width,
                                               activation_element_col_num, bit_sparse, macro_inputs)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupExpectedInfo, time_ns, energy_pj)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupTestInfo, code, expected)

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./MacroGroupTest [config_file] [instruction_file] [report_file]" << std::endl;
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
    auto test_info = ins_j.get<MacroGroupTestInfo>();

    Clock clk{"clock", config.sim_config.period_ns};
    MacroGroupTestModule test_module{"MacroGroupTestModule", config, &clk, std::move(test_info.code)};
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = test_module.getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getTotalEnergyPJ(), test_info.expected.energy_pj)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}
