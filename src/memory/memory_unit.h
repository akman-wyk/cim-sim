//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "core/cim_unit/cim_unit.h"
#include "memory.h"

namespace cimsim {

class MemoryUnit : public BaseModule {
public:
    SC_HAS_PROCESS(MemoryUnit);

    MemoryUnit(const char* name, const MemoryUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
               bool is_global);

    void mountMemory(MemoryHardware* memory_hardware);

    void access(const std::shared_ptr<MemoryAccessPayload>& payload);

    EnergyReporter getEnergyReporter() override;

    int getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;
    int getMemorySizeById(int memory_id) const;

private:
    std::shared_ptr<Memory> getMemoryByAddress(int address_byte);

private:
    const MemoryUnitConfig& config_;
    const SimConfig& sim_config_;
    const AddressSapce& as_;

    bool is_global_;
    std::vector<std::shared_ptr<Memory>> memory_list_;
};

}  // namespace cimsim
