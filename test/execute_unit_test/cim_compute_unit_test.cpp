//
// Created by wyk on 2024/8/2.
//

#include "../base/test_payload.h"
#include "core/execute_unit/cim_compute_unit.h"
#include "execute_unit_test.h"

namespace cimsim {

struct CimComputeTestInstruction {
    CimComputeInsPayload payload;
};

struct CimComputeTestInfo {
    std::vector<CimComputeTestInstruction> code{};
    std::vector<unsigned char> groups_activation_element_col_mask_;
    TestExpectedInfo expected;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimComputeInsPayload, ins, input_addr_byte, input_len, input_bit_width,
                                               activation_group_num, group_input_step_byte, row, bit_sparse,
                                               bit_sparse_meta_addr_byte, value_sparse, value_sparse_mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimComputeTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimComputeTestInfo, code, groups_activation_element_col_mask_, expected)

class CimComputeUnitTestModule
    : public ExecuteUnitTestModule<CimComputeUnitTestModule, CimComputeUnit, CimUnitConfig, CimComputeInsPayload,
                                   CimComputeTestInstruction, TestExpectedInfo, CimComputeTestInfo> {
public:
    CimComputeUnitTestModule(const char* name, const char* test_unit_name, const CimUnitConfig& test_unit_config,
                             const Config& config, Clock* clk, std::vector<CimComputeTestInstruction> codes)
        : TestBaseModule(name, test_unit_name, test_unit_config, config, clk, std::move(codes),
                         ExecuteUnitType::cim_compute)
        , cim_unit_("CimUnit", config.chip_config.core_config.cim_unit_config, config.sim_config, nullptr, clk) {
        test_unit_.bindCimUnit(&cim_unit_);
        local_memory_unit_.mountMemory(&cim_unit_);
    }

    void setGroupActivationElementColumnMask(const std::vector<unsigned char>& groups_activation_element_col_mask) {
        cim_unit_.setMacroGroupActivationElementColumn(groups_activation_element_col_mask, true, 0);
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    CimUnit cim_unit_;
};

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, CimComputeTestInfo& test_info) {
        auto* cim_compute_unit = new CimComputeUnitTestModule{
            "CimComputeUnitTestModule", "CimComputeUnit", config.chip_config.core_config.cim_unit_config, config, clk,
            std::move(test_info.code)};
        cim_compute_unit->setGroupActivationElementColumnMask(test_info.groups_activation_element_col_mask_);
        return cim_compute_unit;
    };
    return cimsim_unit_test<CimComputeUnitTestModule>(argc, argv, initializer);
}
