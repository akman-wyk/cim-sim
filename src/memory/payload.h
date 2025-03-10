//
// Created by wyk on 2025/3/10.
//

#pragma once
#include "core/payload.h"
#include "systemc.h"

namespace pimsim {

BETTER_ENUM(MemoryAccessType, int, read = 1, write = 2)  // NOLINT(*-explicit-constructor, *-no-recursion)

struct MemoryAccessPayload {
    InstructionPayload ins{};

    MemoryAccessType access_type;
    int address_byte;  // byte
    int size_byte;     // byte
    std::vector<uint8_t> data;
    sc_core::sc_event& finish_access;
};

}  // namespace pimsim
