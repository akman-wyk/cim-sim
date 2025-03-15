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
class Switch;

class MemorySocket {
public:
    MemorySocket() = default;

    ~MemorySocket();

    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);
    void bindSwitchAndGlobalMemory(Switch* _switch, int core_id, int global_memory_switch_id);

    std::vector<uint8_t> readLocal(const InstructionPayload& ins, int address_byte, int size_byte);
    void writeLocal(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    std::vector<uint8_t> loadGlobal(const InstructionPayload& ins, int address_byte, int size_byte);
    void storeGlobal(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    [[nodiscard]] int getLocalMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;
    [[nodiscard]] int getLocalMemorySizeById(int memory_id) const;

private:
    MemoryUnit* local_memory_unit_{nullptr};
    Switch* switch_{nullptr};
    int core_id_{0};
    int global_memory_switch_id_{-1};

    sc_core::sc_event* finish_read_{nullptr};
    sc_core::sc_event* finish_write_{nullptr};

    sc_core::sc_event* finish_load_{nullptr};
    sc_core::sc_event* finish_store_{nullptr};
    sc_core::sc_event* finish_load_trans_{nullptr};
    sc_core::sc_event* finish_store_trans_{nullptr};
};

}  // namespace cimsim
