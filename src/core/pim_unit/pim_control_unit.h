//
// Created by wyk on 2024/8/1.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"
#include "cim_unit.h"

namespace pimsim {

class PimControlUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimControlUnit);

    PimControlUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    void bindCimUnit(CimUnit* cim_unit);

    EnergyReporter getEnergyReporter() override;

private:
    void checkPimControlInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void processSetActivation(const PimControlInsPayload& payload);
    void processOnlyOutput(const PimControlInsPayload& payload);
    void processOutputSum(const PimControlInsPayload& payload);
    void processOutputSumMove(const PimControlInsPayload& payload);

    void finishInstruction();
    void finishRun();

    DataConflictPayload getDataConflictInfo(const PimControlInsPayload& payload) const;

public:
    ExecuteUnitResponseIOPorts<PimControlInsPayload> ports_;

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    SubmoduleSocket<PimControlInsPayload> execute_socket_;

    FSM<PimControlInsPayload> fsm_;
    sc_core::sc_signal<PimControlInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimControlInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;
    CimUnit* cim_unit_{};

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_id_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    EnergyCounter result_adder_energy_counter_;
};

}  // namespace pimsim
