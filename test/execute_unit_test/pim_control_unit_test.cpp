//
// Created by wyk on 2024/8/2.
//

#include "core/cim_unit/cim_unit.h"
#include "core/execute_unit/cim_control_unit.h"
#include "core/payload.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimControlTestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};

    std::vector<int> groups_activation_macro_cnt{};
    std::vector<int> groups_activation_element_col_cnt{};
};

struct PimControlTestInstruction {
    PimControlInsPayload payload;
};

struct PimControlTestInfo {
    std::vector<PimControlTestInstruction> code{};
    PimControlTestExpectedInfo expected{};
};

void to_json(nlohmann::ordered_json& j, const PimControlOperator& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, PimControlOperator& m) {
    const auto str = j.get<std::string>();
    m = PimControlOperator::_from_string(str.c_str());
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimControlTestExpectedInfo, time_ns, energy_pj,
                                               groups_activation_macro_cnt, groups_activation_element_col_cnt)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimControlInsPayload, ins, op, group_broadcast, group_id, mask_addr_byte,
                                               activation_group_num, output_addr_byte, output_cnt_per_group,
                                               output_bit_width, output_mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimControlTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimControlTestInfo, code, expected)

class PimControlUnitTestModule
    : public ExecuteUnitTestModule<PimControlUnitTestModule, PimControlUnit, PimUnitConfig, PimControlInsPayload,
                                   PimControlTestInstruction, PimControlTestExpectedInfo, PimControlTestInfo> {
public:
    PimControlUnitTestModule(const char* name, const char* test_unit_name, const PimUnitConfig& test_unit_config,
                             const Config& config, Clock* clk, std::vector<PimControlTestInstruction> codes)
        : TestBaseModule(name, test_unit_name, test_unit_config, config, clk, std::move(codes),
                         ExecuteUnitType::pim_control)
        , cim_unit_("CimUnit", config.chip_config.core_config.pim_unit_config, config.sim_config, nullptr, clk) {
        test_unit_.bindCimUnit(&cim_unit_);
        local_memory_unit_.bindCimUnit(&cim_unit_);
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    bool checkTestResult(const pimsim::PimControlTestExpectedInfo& expected) override {
        for (int group_id = 0; group_id < expected.groups_activation_element_col_cnt.size(); group_id++) {
            if (cim_unit_.getMacroGroupActivationElementColumnCount(group_id) !=
                expected.groups_activation_element_col_cnt[group_id]) {
                std::cout << fmt::format("activation element col cnt error, group id: {}, expected: {}, actual: {}",
                                         group_id, expected.groups_activation_element_col_cnt[group_id],
                                         cim_unit_.getMacroGroupActivationElementColumnCount(group_id))
                          << std::endl;
                return false;
            }
        }
        for (int group_id = 0; group_id < expected.groups_activation_macro_cnt.size(); group_id++) {
            if (cim_unit_.getMacroGroupActivationMacroCount(group_id) !=
                expected.groups_activation_macro_cnt[group_id]) {
                std::cout << fmt::format("activation macro cnt error, group id: {}, expected: {}, actual: {}", group_id,
                                         expected.groups_activation_macro_cnt[group_id],
                                         cim_unit_.getMacroGroupActivationMacroCount(group_id))
                          << std::endl;
                return false;
            }
        }
        return true;
    }

private:
    CimUnit cim_unit_;
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto pim_control_unit_test_module_initializer = [](const Config& config, Clock* clk,
                                                       PimControlTestInfo& test_info) {
        return new PimControlUnitTestModule{
            "PimControlUnitTestModule", "PimControlUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<PimControlUnitTestModule>(argc, argv, pim_control_unit_test_module_initializer);
}
