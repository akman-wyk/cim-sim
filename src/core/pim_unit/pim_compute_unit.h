//
// Created by wyk on 2024/7/29.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/payload.h"
#include "macro_group.h"
#include "core/payload/execute_unit_payload.h"

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

class PimComputeUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimComputeUnit);

    PimComputeUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    EnergyReporter getEnergyReporter() override;

    void setMacroGroupActivationElementColumn(const std::vector<unsigned char>& mask, bool group_broadcast,
                                              int group_id);

    int getMacroGroupActivationElementColumnCount(int group_id) const;
    int getMacroGroupActivationMacroCount(int group_id) const;

private:
    void checkPimComputeInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processSubIns();
    void processSubInsReadData(const PimComputeSubInsPayload& sub_ins_payload);
    void processSubInsCompute(const PimComputeSubInsPayload& sub_ins_payload);

    [[noreturn]] void readValueSparseMaskSubmodule();
    [[noreturn]] void readBitSparseMetaSubmodule();

    void finishInstruction();
    void finishRun();

    std::vector<std::vector<unsigned long long>> getMacroGroupInputs(int group_id, int addr_byte, int size_byte,
                                                                     const PimComputeSubInsPayload& sub_ins_payload);

    DataConflictPayload getDataConflictInfo(const PimComputeInsPayload& payload);

public:
    ExecuteUnitResponseIOPorts<PimComputeInsPayload> ports_;

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;
    int config_group_cnt_;

    std::vector<MacroGroup*> macro_group_list_;

    sc_core::sc_event next_sub_ins_;
    SubmoduleSocket<PimComputeSubInsPayload> process_sub_ins_socket_;
    SubmoduleSocket<PimComputeReadDataPayload> read_value_sparse_mask_socket_;
    SubmoduleSocket<PimComputeReadDataPayload> read_bit_sparse_meta_socket_;

    FSM<PimComputeInsPayload> fsm_;
    sc_core::sc_signal<PimComputeInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimComputeInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_id_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    EnergyCounter value_sparse_network_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace pimsim
