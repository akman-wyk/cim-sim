//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "config/config.h"
#include "memory_hardware.h"

namespace cimsim {

class RegBuffer : public MemoryHardware {
public:
    SC_HAS_PROCESS(RegBuffer);

    RegBuffer(const sc_module_name& name, const RegBufferConfig& config, const BaseInfo& base_info);

    sc_time accessAndGetDelay(MemoryAccessPayload& payload) override;

    int getMemoryDataWidthByte(MemoryAccessType access_type) const override;
    int getMemorySizeByte() const override;

private:
    void initialData();

private:
    const RegBufferConfig& config_;

    std::vector<uint8_t> data_;

    EnergyCounter read_energy_counter_;
    EnergyCounter write_energy_counter_;
};

}  // namespace cimsim
