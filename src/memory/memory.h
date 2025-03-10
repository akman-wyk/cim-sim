//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <queue>

#include "base_component/base_module.h"
#include "config/config.h"
#include "memory_hardware.h"
#include "payload.h"

namespace pimsim {

class Memory : public BaseModule {
public:
    SC_HAS_PROCESS(Memory);

    Memory(const char* name, const RAMConfig& ram_config, const AddressSpaceConfig& addressing,
           const SimConfig& sim_config, Core* core, Clock* clk);

    Memory(const char* name, const RegBufferConfig& reg_buffer_config, const AddressSpaceConfig& addressing,
           const SimConfig& sim_config, Core* core, Clock* clk);

    Memory(const char* name, MemoryHardware* memory_hardware, const AddressSpaceConfig& addressing,
           const SimConfig& sim_config, Core* core, Clock* clk);

    ~Memory() override;

    void access(std::shared_ptr<MemoryAccessPayload> payload);

    [[nodiscard]] int getAddressSpaceBegin() const;
    [[nodiscard]] int getAddressSpaceEnd() const;
    [[nodiscard]] int getMemoryDataWidthByte(MemoryAccessType access_type) const;
    [[nodiscard]] int getMemorySizeByte() const;
    [[nodiscard]] bool isMount() const;

    EnergyReporter getEnergyReporter() override;

private:
    [[noreturn]] void process();

private:
    bool is_mount;  // whether this memory is a mount memory
    const AddressSpaceConfig& addressing_;

    std::queue<std::shared_ptr<MemoryAccessPayload>> access_queue_;
    MemoryHardware* hardware_;

    sc_core::sc_event start_process_;
};

}  // namespace pimsim
