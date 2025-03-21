//
// Created by wyk on 2024/7/25.
//

#include <vector>

#include "../base/test_macro.h"
#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "core/cim_unit/macro_group.h"
#include "core/cim_unit/payload.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"
#include "util/util.h"

namespace cimsim {

struct MacroGroupExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

struct MacroGroupTestInstruction {
    MacroGroupPayload payload;
    std::vector<unsigned char> macros_activation_element_col_mask;
};

struct MacroGroupTestInfo {
    std::vector<MacroGroupTestInstruction> code{};
    MacroGroupExpectedInfo expected{};
};

class MacroGroupTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroupTestModule);

    MacroGroupTestModule(const sc_core::sc_module_name& name, const Config& config, Clock* clk,
                         std::vector<MacroGroupTestInstruction> codes)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , macro_group_("MacroGroup_0", config.chip_config.core_config.cim_unit_config, config.sim_config, nullptr,
                       clk) {
        macro_group_ins_list_ = std::move(codes);

        SC_THREAD(issue)

        macro_group_.setFinishInsFunc([&]() {
            running_ins_cnt_--;
            if (id_finish_ && running_ins_cnt_ == 0) {
                wait(SC_ZERO_TIME);
                this->running_time_ = sc_core::sc_time_stamp();
                sc_stop();
            }
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

        bool next_new_ins = true;
        for (auto& macro_group_ins : macro_group_ins_list_) {
            macro_group_.waitUntilFinishIfBusy();

            if (next_new_ins) {
                running_ins_cnt_++;
                next_new_ins = false;
            }
            if (macro_group_ins.payload.cim_ins_info.last_sub_ins) {
                next_new_ins = true;
            }

            macro_group_.setMacrosActivationElementColumn(macro_group_ins.macros_activation_element_col_mask);
            macro_group_.startExecute(std::move(macro_group_ins.payload));
            wait(SC_ZERO_TIME);
        }
        id_finish_ = true;
    }

private:
    std::vector<MacroGroupTestInstruction> macro_group_ins_list_;

    MacroGroup macro_group_;

    int running_ins_cnt_{0};
    bool id_finish_{false};

    sc_core::sc_time running_time_;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimInsInfo, ins_pc, sub_ins_num, last_sub_ins)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupPayload, cim_ins_info, last_group, row, input_bit_width,
                                               bit_sparse, macro_inputs)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupExpectedInfo, time_ns, energy_pj)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupTestInstruction, payload, macros_activation_element_col_mask)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroGroupTestInfo, code, expected)

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./MacroGroupTest [config_file] [instruction_file] [report_file]" << std::endl;
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
    AddressSapce::initialize(config.chip_config);

    auto test_info = readTypeFromJsonFile<MacroGroupTestInfo>(instruction_file);
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
