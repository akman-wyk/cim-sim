//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/energy_counter.h"
#include "config/config.h"
#include "memory_hardware.h"

namespace cimsim {

class RAM : public MemoryHardware {
public:
    SC_HAS_PROCESS(RAM);

    RAM(const char* name, const RAMConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    sc_core::sc_time accessAndGetDelay(MemoryAccessPayload& payload) override;

    EnergyReporter getEnergyReporter() override;

    int getMemoryDataWidthByte(MemoryAccessType access_type) const override;
    int getMemorySizeByte() const override;
    const std::string& getMemoryName() override;

private:
    void initialData();

private:
    const RAMConfig& config_;

    std::vector<uint8_t> data_;

    EnergyCounter static_energy_counter_;
    EnergyCounter read_energy_counter_;
    EnergyCounter write_energy_counter_;
};

}  // namespace cimsim
