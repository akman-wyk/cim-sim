//
// Created by wyk on 2024/7/16.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/transfer_unit/transfer_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct TransferTestInstruction {
    TransferInsPayload payload;
};

struct TransferTestInfo {
    std::vector<TransferTestInstruction> code{};
    TestExpectedInfo expected{};
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInfo, code, expected)

class TransferUnitTestModule
    : public ExecuteUnitTestModule<TransferUnitTestModule, TransferUnit, TransferUnitConfig, TransferInsPayload,
                                   TransferTestInstruction, TestExpectedInfo, TransferTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, TransferTestInfo& test_info) {
        return new TransferUnitTestModule{"transfer_unit_test_module",
                                          "TransferUnit",
                                          config.chip_config.core_config.transfer_unit_config,
                                          config,
                                          clk,
                                          std::move(test_info.code), ExecuteUnitType::transfer};
    };
    return pimsim_unit_test<TransferUnitTestModule>(argc, argv, initializer);
}
