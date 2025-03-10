//
// Created by wyk on 2025/3/10.
//

#pragma once
#include "core/payload.h"
#include "systemc.h"

namespace pimsim {

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set);

struct ResourceAllocatePayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ResourceAllocatePayload)

    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};

    std::unordered_set<int> read_memory_id;
    std::unordered_set<int> write_memory_id;
    std::unordered_set<int> used_memory_id;

    DECLARE_PIM_PAYLOAD_FUNCTIONS(ResourceAllocatePayload)

    void addReadMemoryId(int memory_id);
    void addReadMemoryId(const std::initializer_list<int>& memory_id_list);
    void addWriteMemoryId(int memory_id);
    void addReadWriteMemoryId(int memory_id);

    static bool checkDataConflict(const ResourceAllocatePayload& ins_conflict_payload,
                                  const ResourceAllocatePayload& unit_conflict_payload);

    ResourceAllocatePayload& operator+=(const ResourceAllocatePayload& other);
};

struct ResourceReleasePayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ResourceReleasePayload)

    int ins_id{-1};

    DECLARE_PIM_PAYLOAD_FUNCTIONS(ResourceReleasePayload)
};

}  // namespace pimsim
