//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <queue>

#include "base_component/base_module.h"
#include "config/config.h"
#include "memory_hardware.h"
#include "payload.h"

namespace cimsim {

class Memory : public BaseModule {
public:
    SC_HAS_PROCESS(Memory);

    Memory(const std::string& name, const RAMConfig& ram_config, const SimConfig& sim_config, Core* core, Clock* clk);

    Memory(const std::string& name, const RegBufferConfig& reg_buffer_config, const SimConfig& sim_config, Core* core,
           Clock* clk);

    Memory(const std::string& name, MemoryHardware* memory_hardware, const SimConfig& sim_config, Core* core,
           Clock* clk);

    ~Memory() override;

    void access(std::shared_ptr<MemoryAccessPayload> payload);

    [[nodiscard]] int getAddressSpaceOffset() const;
    [[nodiscard]] int getMemoryDataWidthByte(MemoryAccessType access_type) const;
    [[nodiscard]] int getMemorySizeByte() const;
    [[nodiscard]] bool isMount() const;

    EnergyReporter getEnergyReporter() override;

private:
    [[noreturn]] void process();

private:
    bool is_mount;  // whether this memory is a mount memory
    int as_offset_;

    std::queue<std::shared_ptr<MemoryAccessPayload>> access_queue_;
    MemoryHardware* hardware_;

    sc_core::sc_event start_process_;
};

}  // namespace cimsim
