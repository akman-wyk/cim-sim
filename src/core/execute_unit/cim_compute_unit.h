//
// Created by wyk on 2024/7/29.
//

#pragma once
#include <vector>

#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/cim_unit/cim_unit.h"
#include "execute_unit.h"
#include "memory/memory_socket.h"
#include "payload.h"

namespace pimsim {

struct PimComputeReadDataPayload {
    InstructionPayload ins{};
    int addr_byte{0};
    int size_byte{0};
    std::vector<unsigned char> data{};
};

struct PimComputeSubInsPayload {
    PimInsInfo pim_ins_info{};
    PimComputeInsPayload ins_payload;
    int group_max_activation_macro_cnt{};
};

class PimComputeUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(PimComputeUnit);

    PimComputeUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);
    void bindCimUnit(CimUnit* cim_unit);

    EnergyReporter getEnergyReporter() override;

    ResourceAllocatePayload getDataConflictInfo(const PimComputeInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processSubIns();
    void processSubInsReadData(const PimComputeSubInsPayload& sub_ins_payload);
    void processSubInsCompute(const PimComputeSubInsPayload& sub_ins_payload);

    [[noreturn]] void readValueSparseMaskSubmodule();
    [[noreturn]] void readBitSparseMetaSubmodule();

    std::vector<std::vector<unsigned long long>> getMacroGroupInputs(int group_id, int addr_byte, int size_byte,
                                                                     const PimComputeSubInsPayload& sub_ins_payload);

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    CimUnit* cim_unit_{nullptr};

    sc_core::sc_event next_sub_ins_;
    SubmoduleSocket<PimComputeSubInsPayload> process_sub_ins_socket_;
    SubmoduleSocket<PimComputeReadDataPayload> read_value_sparse_mask_socket_;
    SubmoduleSocket<PimComputeReadDataPayload> read_bit_sparse_meta_socket_;

    MemorySocket local_memory_socket_;

    EnergyCounter value_sparse_network_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace pimsim
