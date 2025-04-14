//
// Created by wyk on 2024/7/16.
//

#include "../base/test_payload.h"
#include "core/execute_unit/transfer_unit.h"
#include "execute_unit_test.h"

namespace cimsim {

struct TransferTestInstruction {
    TransferInsPayload payload;
};

struct TransferTestInfo {
    std::vector<TransferTestInstruction> code{};
    TestExpectedInfo expected{};
};

void to_json(nlohmann::ordered_json& j, const DataPathType& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, DataPathType& m) {
    const auto str = j.get<std::string>();
    m = DataPathType::_from_string(str.c_str());
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(DataPathPayload, type, local_dedicated_data_path_id)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferInsPayload, ins, type, data_path_payload, src_address_byte,
                                               dst_address_byte, size_byte, src_id, dst_id, transfer_id_tag)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferTestInfo, code, expected)

class TransferUnitTestModule
    : public ExecuteUnitTestModule<TransferUnitTestModule, TransferUnit, TransferUnitConfig, TransferInsPayload,
                                   TransferTestInstruction, TestExpectedInfo, TransferTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter{0, 0, 0, EnergyCounter::getRunningTimeNS()};
        reporter.addSubModule(local_memory_unit_.getName(),
                              local_memory_unit_.getEnergyCounterPtr()->getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyCounterPtr()->getEnergyReporter());
        return std::move(reporter);
    }
};

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, TransferTestInfo& test_info) {
        return new TransferUnitTestModule{"transfer_unit_test_module",
                                          "TransferUnit",
                                          config.chip_config.core_config.transfer_unit_config,
                                          config,
                                          clk,
                                          std::move(test_info.code),
                                          ExecuteUnitType::transfer};
    };
    return cimsim_unit_test<TransferUnitTestModule>(argc, argv, initializer);
}
