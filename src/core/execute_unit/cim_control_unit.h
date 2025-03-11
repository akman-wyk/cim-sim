//
// Created by wyk on 2024/8/1.
//

#pragma once
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/cim_unit/cim_unit.h"
#include "execute_unit.h"
#include "memory/memory_socket.h"
#include "payload.h"

namespace cimsim {

class CimControlUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(CimControlUnit);

    CimControlUnit(const char* name, const CimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    void bindCimUnit(CimUnit* cim_unit);

    EnergyReporter getEnergyReporter() override;

    ResourceAllocatePayload getDataConflictInfo(const CimControlInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void processSetActivation(const CimControlInsPayload& payload);
    void processOnlyOutput(const CimControlInsPayload& payload);
    void processOutputSum(const CimControlInsPayload& payload);
    void processOutputSumMove(const CimControlInsPayload& payload);

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;

    SubmoduleSocket<CimControlInsPayload> execute_socket_;

    MemorySocket local_memory_socket_;
    CimUnit* cim_unit_{};

    EnergyCounter result_adder_energy_counter_;
};

}  // namespace cimsim
