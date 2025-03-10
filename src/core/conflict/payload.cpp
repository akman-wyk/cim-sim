//
// Created by wyk on 2025/3/10.
//

#include "payload.h"

#include "util/util.h"

namespace pimsim {

std::stringstream& operator<<(std::stringstream& out, const std::unordered_set<int>& set) {
    for (auto it = set.begin(); it != set.end(); ++it) {
        if (it != set.begin()) {
            out << ", ";
        }
        out << *it;
    }
    return out;
}

DEFINE_PIM_PAYLOAD_FUNCTIONS(ResourceAllocatePayload, ins_id, unit_type, read_memory_id, write_memory_id, used_memory_id)

void ResourceAllocatePayload::addReadMemoryId(int memory_id) {
    read_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

void ResourceAllocatePayload::addReadMemoryId(const std::initializer_list<int>& memory_id_list) {
    read_memory_id.insert(memory_id_list);
    used_memory_id.insert(memory_id_list);
}

void ResourceAllocatePayload::addWriteMemoryId(int memory_id) {
    write_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

void ResourceAllocatePayload::addReadWriteMemoryId(int memory_id) {
    read_memory_id.insert(memory_id);
    write_memory_id.insert(memory_id);
    used_memory_id.insert(memory_id);
}

bool ResourceAllocatePayload::checkDataConflict(const ResourceAllocatePayload& ins_conflict_payload,
                                            const ResourceAllocatePayload& unit_conflict_payload) {
    if (ins_conflict_payload.unit_type == unit_conflict_payload.unit_type) {
        return SetsIntersection(unit_conflict_payload.write_memory_id, ins_conflict_payload.read_memory_id);
    }
    return SetsIntersection(unit_conflict_payload.used_memory_id, ins_conflict_payload.used_memory_id);
}

ResourceAllocatePayload& ResourceAllocatePayload::operator+=(const pimsim::ResourceAllocatePayload& other) {
    this->read_memory_id.insert(other.read_memory_id.begin(), other.read_memory_id.end());
    this->write_memory_id.insert(other.write_memory_id.begin(), other.write_memory_id.end());
    this->used_memory_id.insert(other.used_memory_id.begin(), other.used_memory_id.end());
    this->unit_type = other.unit_type;
    return *this;
}

DEFINE_PIM_PAYLOAD_FUNCTIONS(ResourceReleasePayload, ins_id)

}  // namespace pimsim
