//
// Created by wyk on 2024/8/2.
//

#include "core/payload/payload.h"
#include "core/pim_unit/cim_unit.h"
#include "core/pim_unit/pim_compute_unit.h"
#include "core/pim_unit/pim_set_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimSetTestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};

    std::vector<int> groups_activation_macro_cnt{};
    std::vector<int> groups_activation_element_col_cnt{};
};

struct PimSetTestInstruction {
    PimSetInsPayload payload;
};

struct PimSetTestInfo {
    std::vector<PimSetTestInstruction> code{};
    PimSetTestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestExpectedInfo, time_ns, energy_pj, groups_activation_macro_cnt,
                                               groups_activation_element_col_cnt)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetInsPayload, ins, group_broadcast, group_id, mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimSetTestInfo, code, expected)

class PimSetUnitTestModule
    : public ExecuteUnitTestModule<PimSetUnitTestModule, PimSetUnit, PimUnitConfig, PimSetInsPayload,
                                   PimSetTestInstruction, PimSetTestExpectedInfo, PimSetTestInfo> {
public:
    PimSetUnitTestModule(const char* name, const char* test_unit_name, const PimUnitConfig& test_unit_config,
                         const Config& config, Clock* clk, std::vector<PimSetTestInstruction> codes)
        : TestBaseModule(name, test_unit_name, test_unit_config, config, clk, std::move(codes))
        , cim_unit_("CimUnit", config.chip_config.core_config.pim_unit_config, config.sim_config, nullptr, clk) {
        test_unit_.bindCimUnit(&cim_unit_);
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    bool checkTestResult(const pimsim::PimSetTestExpectedInfo& expected) override {
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
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::PimSetInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.ins_id = ins_payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_set};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addReadMemoryId(local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.mask_addr_byte));
        return std::move(conflict_payload);
    }

private:
    CimUnit cim_unit_;
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto pim_set_unit_test_module_initializer = [](const Config& config, Clock* clk, PimSetTestInfo& test_info) {
        return new PimSetUnitTestModule{
            "PimSetUnitTestModule",   "PimSetUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
    };
    return pimsim_unit_test<PimSetUnitTestModule>(argc, argv, pim_set_unit_test_module_initializer);
}
