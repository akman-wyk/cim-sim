//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <cstdint>
#include <vector>

#include "core/payload/payload.h"
#include "systemc.h"

namespace pimsim {

class LocalMemoryUnit;

class MemorySocket {
public:
    MemorySocket() = default;

    void bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit);

    std::vector<uint8_t> readData(const InstructionPayload& ins, int address_byte, int size_byte);

    void writeData(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    int getLocalMemoryIdByAddress(int address_byte) const;

    int getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;

private:
    LocalMemoryUnit* local_memory_unit_{nullptr};
    sc_core::sc_event finish_read_;
    sc_core::sc_event finish_write_;
};

}  // namespace pimsim
