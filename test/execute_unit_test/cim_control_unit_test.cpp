//
// Created by wyk on 2024/8/2.
//

#include "core/cim_unit/cim_unit.h"
#include "core/execute_unit/cim_control_unit.h"
#include "execute_unit_test.h"

namespace cimsim {

struct CimControlTestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};

    std::vector<int> groups_activation_macro_cnt{};
    std::vector<int> groups_activation_element_col_cnt{};
};

struct CimControlTestInstruction {
    CimControlInsPayload payload;
};

struct CimControlTestInfo {
    std::vector<CimControlTestInstruction> code{};
    CimControlTestExpectedInfo expected{};
};

void to_json(nlohmann::ordered_json& j, const CimControlOperator& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, CimControlOperator& m) {
    const auto str = j.get<std::string>();
    m = CimControlOperator::_from_string(str.c_str());
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimControlTestExpectedInfo, time_ns, energy_pj,
                                               groups_activation_macro_cnt, groups_activation_element_col_cnt)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimControlInsPayload, ins, op, group_broadcast, group_id, mask_addr_byte,
                                               activation_group_num, output_addr_byte, output_cnt_per_group,
                                               output_bit_width, output_mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimControlTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimControlTestInfo, code, expected)

class CimControlUnitTestModule
    : public ExecuteUnitTestModule<CimControlUnitTestModule, CimControlUnit, CimUnitConfig, CimControlInsPayload,
                                   CimControlTestInstruction, CimControlTestExpectedInfo, CimControlTestInfo> {
public:
    CimControlUnitTestModule(const sc_core::sc_module_name& name, const char* test_unit_name,
                             const CimUnitConfig& test_unit_config, const Config& config, Clock* clk,
                             std::vector<CimControlTestInstruction> codes)
        : TestBaseModule(name, test_unit_name, test_unit_config, config, clk, std::move(codes),
                         ExecuteUnitType::cim_control)
        , cim_unit_("CimUnit", config.chip_config.core_config.cim_unit_config, BaseInfo{config.sim_config}) {
        test_unit_.bindCimUnit(&cim_unit_);
        local_memory_unit_.mountMemory(&cim_unit_);
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    bool checkTestResult(const cimsim::CimControlTestExpectedInfo& expected) override {
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

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    auto cim_control_unit_test_module_initializer = [](const Config& config, Clock* clk,
                                                       CimControlTestInfo& test_info) {
        return new CimControlUnitTestModule{
            "CimControlUnitTestModule", "CimControlUnit", config.chip_config.core_config.cim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return cimsim_unit_test<CimControlUnitTestModule>(argc, argv, cim_control_unit_test_module_initializer);
}
