//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <cstdint>
#include <vector>

#include "memory/payload.h"
#include "systemc.h"

namespace cimsim {

class MemoryUnit;

class MemorySocket {
public:
    MemorySocket() = default;

    ~MemorySocket();

    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);

    std::vector<uint8_t> readLocal(const InstructionPayload& ins, int address_byte, int size_byte);
    void writeLocal(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    [[nodiscard]] int getLocalMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;
    [[nodiscard]] int getLocalMemorySizeById(int memory_id) const;

private:
    MemoryUnit* local_memory_unit_{nullptr};

    sc_event* finish_read_{nullptr};
    sc_event* finish_write_{nullptr};
};

}  // namespace cimsim
