//
// Created by wyk on 2024/8/8.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/memory_socket.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/payload/execute_unit_payload.h"
#include "core/payload/payload.h"

namespace pimsim {

class PimLoadUnit : public BaseModule {
public:
    SC_HAS_PROCESS(PimLoadUnit);

    PimLoadUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    EnergyReporter getEnergyReporter() override;

private:
    void checkPimLoadInst();

    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void finishInstruction();
    void finishRun();

public:
    ExecuteUnitResponseIOPorts<PimLoadInsPayload> ports_;

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    SubmoduleSocket<PimLoadInsPayload> execute_socket_;

    FSM<PimLoadInsPayload> fsm_;
    sc_core::sc_signal<PimLoadInsPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<PimLoadInsPayload>> fsm_in_;

    MemorySocket local_memory_socket_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_pc_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};

    EnergyCounter sram_write_energy_counter_;
};

}  // namespace pimsim
