//
// Created by wyk on 2024/7/9.
//

#include "../base/test_payload.h"
#include "core/execute_unit/simd_unit.h"
#include "execute_unit_test.h"

namespace cimsim {

struct SIMDTestInstruction {
    SIMDInsPayload payload;
};

struct SIMDTestInfo {
    std::vector<SIMDTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDInsPayload, ins, inputs_bit_width, output_bit_width,
                                               inputs_address_byte, output_address_byte, len)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDTestInfo, code, expected)

class SIMDUnitTestModule : public ExecuteUnitTestModule<SIMDUnitTestModule, SIMDUnit, SIMDUnitConfig, SIMDInsPayload,
                                                        SIMDTestInstruction, TestExpectedInfo, SIMDTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

    SIMDInsPayload decode(const SIMDInsPayload& payload) override {
        auto p = payload;
        p.ins_cfg = &test_unit_config_.instruction_list[1];
        p.func_cfg = &test_unit_config_.functor_list[1];
        return p;
    }
};

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, SIMDTestInfo& test_info) {
        return new SIMDUnitTestModule{"SIMD_unit_test_module",
                                      "SIMDUnit",
                                      config.chip_config.core_config.simd_unit_config,
                                      config,
                                      clk,
                                      std::move(test_info.code),
                                      ExecuteUnitType::simd};
    };
    return cimsim_unit_test<SIMDUnitTestModule>(argc, argv, initializer);
}
