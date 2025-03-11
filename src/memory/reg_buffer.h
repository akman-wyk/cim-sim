//
// Created by wyk on 2024/7/4.
//

#pragma once
#include "base_component/base_module.h"
#include "memory_hardware.h"

namespace cimsim {

class RegBuffer : public MemoryHardware {
public:
    SC_HAS_PROCESS(RegBuffer);

    RegBuffer(const char* name, const RegBufferConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    sc_core::sc_time accessAndGetDelay(MemoryAccessPayload& payload) override;

    EnergyReporter getEnergyReporter() override;

    int getMemoryDataWidthByte(MemoryAccessType access_type) const override;
    int getMemorySizeByte() const override;

private:
    void initialData();

private:
    const RegBufferConfig& config_;

    std::vector<uint8_t> data_;

    EnergyCounter static_energy_counter_;
    EnergyCounter read_energy_counter_;
    EnergyCounter write_energy_counter_;
};

}  // namespace cimsim
