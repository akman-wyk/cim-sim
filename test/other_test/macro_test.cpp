//
// Created by wyk on 2024/7/21.
//

#include "../base/test_macro.h"
#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "config/config.h"
#include "core/cim_unit/macro.h"
#include "core/cim_unit/payload.h"
#include "util/macro_scope.h"
#include "util/util.h"

namespace cimsim {

struct MacroTestConfig {
    bool independent_ipu{false};
};

struct MacroExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

struct MacroTestInstruction {
    MacroPayload payload;
    std::vector<unsigned char> activation_element_col_mask;
};

struct MacroTestInfo {
    std::vector<MacroTestInstruction> code{};
    MacroTestConfig config{};
    MacroExpectedInfo expected{};
};

class MacroTestModule : public BaseModule {
public:
    SC_HAS_PROCESS(MacroTestModule);

    MacroTestModule(const char* name, const Config& config, Clock* clk, std::vector<MacroTestInstruction> codes,
                    const MacroTestConfig& test_config)
        : BaseModule(name, config.sim_config, nullptr, clk)
        , macro_("macro", config.chip_config.core_config.cim_unit_config, config.sim_config, nullptr, clk,
                 test_config.independent_ipu) {
        macro_ins_list_ = std::move(codes);

        SC_THREAD(issue)

        macro_.setFinishInsFunction([&]() {
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
        reporter.addSubModule(macro_.getName(), macro_.getEnergyReporter());
        return std::move(reporter);
    }

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1e3, "MacroTestModule", getEnergyReporter(), 0};
    }

private:
    void issue() {
        wait(10, SC_NS);

        bool next_new_ins = true;
        for (int i = 0; i < macro_ins_list_.size(); i++) {
            auto& macro_ins = macro_ins_list_[i];
            macro_.waitUntilFinishIfBusy();

            if (next_new_ins) {
                running_ins_cnt_++;
                next_new_ins = false;
            }
            if (macro_ins.payload.cim_ins_info.last_sub_ins) {
                next_new_ins = true;
            }

            macro_.setActivationElementColumn(macro_ins.activation_element_col_mask);
            macro_.startExecute(macro_ins.payload);

            if (i == macro_ins_list_.size() - 1) {
                break;
            }

            double latency = macro_ins.payload.input_bit_width * period_ns_;
            wait(latency, SC_NS);
        }
        id_finish_ = true;
    }

private:
    // instruction list
    std::vector<MacroTestInstruction> macro_ins_list_;

    // modules
    Macro macro_;

    int running_ins_cnt_{0};
    bool id_finish_{false};

    sc_core::sc_time running_time_;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimInsInfo, ins_pc, sub_ins_num, last_sub_ins)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroPayload, cim_ins_info, row, input_bit_width, bit_sparse, inputs)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroTestConfig, independent_ipu)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroExpectedInfo, time_ns, energy_pj)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroTestInstruction, payload, activation_element_col_mask)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MacroTestInfo, code, config, expected)

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 4) {
        std::cout << "Usage: ./MacroTest [config_file] [instruction_file] [report_file]" << std::endl;
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

    auto test_info = readTypeFromJsonFile<MacroTestInfo>(instruction_file);
    Clock clk{"clock", config.sim_config.period_ns};
    MacroTestModule test_module{"MacroTestModule", config, &clk, std::move(test_info.code), test_info.config};
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
