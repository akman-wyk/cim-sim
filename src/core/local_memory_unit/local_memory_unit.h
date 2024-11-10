//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "base_component/base_module.h"
#include "core/payload/payload.h"
#include "memory/memory.h"

namespace pimsim {

class LocalMemoryUnit : public BaseModule {
public:
    SC_HAS_PROCESS(LocalMemoryUnit);

    LocalMemoryUnit(const char* name, const LocalMemoryUnitConfig& config, const SimConfig& sim_config, Core* core,
                    Clock* clk);

    std::vector<uint8_t> read_data(const InstructionPayload& ins, int address_byte, int size_byte,
                                   sc_core::sc_event& finish_access);

    void write_data(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data,
                    sc_core::sc_event& finish_access);

    EnergyReporter getEnergyReporter() override;

    int getLocalMemoryIdByAddress(int address_byte) const;

    int getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;

    int getMemorySizeById(int memory_id) const;

private:
    std::shared_ptr<Memory> getLocalMemoryByAddress(int address_byte);

private:
    const LocalMemoryUnitConfig& config_;

    std::vector<std::shared_ptr<Memory>> local_memory_list_;
};

}  // namespace pimsim
