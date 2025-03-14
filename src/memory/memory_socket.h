//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <cstdint>
#include <vector>

#include "payload.h"
#include "systemc.h"

namespace cimsim {

class MemoryUnit;

class MemorySocket {
public:
    MemorySocket() = default;

    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);

    std::vector<uint8_t> readData(const InstructionPayload& ins, int address_byte, int size_byte);

    void writeData(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    int getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;

    int getMemorySizeById(int memory_id) const;

private:
    MemoryUnit* local_memory_unit_{nullptr};
    sc_core::sc_event finish_read_;
    sc_core::sc_event finish_write_;
};

}  // namespace cimsim
