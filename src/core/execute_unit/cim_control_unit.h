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

namespace pimsim {

class PimControlUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(PimControlUnit);

    PimControlUnit(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    void bindCimUnit(CimUnit* cim_unit);

    EnergyReporter getEnergyReporter() override;

    ResourceAllocatePayload getDataConflictInfo(const PimControlInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processExecute();

    void processSetActivation(const PimControlInsPayload& payload);
    void processOnlyOutput(const PimControlInsPayload& payload);
    void processOutputSum(const PimControlInsPayload& payload);
    void processOutputSumMove(const PimControlInsPayload& payload);

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;

    SubmoduleSocket<PimControlInsPayload> execute_socket_;

    MemorySocket local_memory_socket_;
    CimUnit* cim_unit_{};

    EnergyCounter result_adder_energy_counter_;
};

}  // namespace pimsim
