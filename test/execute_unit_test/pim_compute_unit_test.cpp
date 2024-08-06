//
// Created by wyk on 2024/8/2.
//

#include "../base/test_payload.h"
#include "core/payload/payload.h"
#include "core/pim_unit/pim_compute_unit.h"
#include "execute_unit_test.h"

namespace pimsim {

struct PimComputeTestInstruction {
    PimComputeInsPayload payload;
};

struct PimComputeTestInfo {
    std::vector<PimComputeTestInstruction> code{};
    std::vector<unsigned char> groups_activation_element_col_mask_;
    TestExpectedInfo expected;
};

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimComputeInsPayload, ins, input_addr_byte, input_len, input_bit_width,
                                               activation_group_num, group_input_step_byte, row, bit_sparse,
                                               bit_sparse_meta_addr_byte, value_sparse, value_sparse_mask_addr_byte)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimComputeTestInstruction, payload)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(PimComputeTestInfo, code, groups_activation_element_col_mask_, expected)

class PimComputeUnitTestModule
    : public ExecuteUnitTestModule<PimComputeUnitTestModule, PimComputeUnit, PimUnitConfig, PimComputeInsPayload,
                                   PimComputeTestInstruction, TestExpectedInfo, PimComputeTestInfo> {
public:
    using TestBaseModule::TestBaseModule;

    void setGroupActivationElementColumnMask(const std::vector<unsigned char>& groups_activation_element_col_mask) {
        test_unit_.setMacroGroupActivationElementColumn(groups_activation_element_col_mask, true, 0);
    }

    EnergyReporter getEnergyReporter() override {
        EnergyReporter reporter;
        reporter.addSubModule(local_memory_unit_.getName(), local_memory_unit_.getEnergyReporter());
        reporter.addSubModule(test_unit_.getName(), test_unit_.getEnergyReporter());
        return std::move(reporter);
    }

private:
    DataConflictPayload getInsPayloadConflictInfos(const pimsim::PimComputeInsPayload& ins_payload) override {
        DataConflictPayload conflict_payload{.pc = ins_payload.ins.pc};
        conflict_payload.use_pim_unit = true;

        int input_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.input_addr_byte);
        conflict_payload.addReadMemoryId(input_memory_id);

        if (test_unit_config_.value_sparse && ins_payload.value_sparse) {
            int mask_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.value_sparse_mask_addr_byte);
            conflict_payload.addReadMemoryId(mask_memory_id);
        }

        if (test_unit_config_.bit_sparse && ins_payload.bit_sparse) {
            int meta_memory_id = local_memory_unit_.getLocalMemoryIdByAddress(ins_payload.bit_sparse_meta_addr_byte);
            conflict_payload.addReadMemoryId(meta_memory_id);
        }

        return std::move(conflict_payload);
    }
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    auto initializer = [](const Config& config, Clock* clk, PimComputeTestInfo& test_info) {
        auto* pim_compute_unit = new PimComputeUnitTestModule{
            "PimComputeUnitTestModule", "PimComputeUnit", config.chip_config.core_config.pim_unit_config, config, clk,
            std::move(test_info.code)};
        pim_compute_unit->setGroupActivationElementColumnMask(test_info.groups_activation_element_col_mask_);
        return pim_compute_unit;
    };
    return pimsim_unit_test<PimComputeUnitTestModule>(argc, argv, initializer);
}
