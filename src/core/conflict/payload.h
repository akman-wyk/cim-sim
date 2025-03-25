//
// Created by wyk on 2025/3/10.
//

#pragma once
#include "config/config.h"
#include "config/constant.h"
#include "core/payload.h"
#include "systemc.h"

namespace cimsim {

class MemoryBitmap {
public:
    void set(int index);
    void unset(int index);

    MemoryBitmap& operator|=(const MemoryBitmap& ano);
    bool operator==(const MemoryBitmap& ano) const;
    [[nodiscard]] bool intersectionWith(const MemoryBitmap& ano) const;

    [[nodiscard]] std::vector<int> getIndexList() const;

    friend std::stringstream& operator<<(std::stringstream& out, const MemoryBitmap& memory_bitmap);

private:
    BitmapCellType bitmap_[MEMORY_BITMAP_SIZE]{};
};

struct ResourceAllocatePayload {
public:
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ResourceAllocatePayload)
    DECLARE_CIM_PAYLOAD_FUNCTIONS(ResourceAllocatePayload)

    void addReadMemoryId(int memory_id) {
        read_memory_id.set(memory_id);
        used_memory_id.set(memory_id);
    }

    template <class... Args>
    void addReadMemoryId(int memory_id, Args... args) {
        read_memory_id.set(memory_id);
        used_memory_id.set(memory_id);
        addReadMemoryId(args...);
    }

    void addWriteMemoryId(int memory_id) {
        write_memory_id.set(memory_id);
        used_memory_id.set(memory_id);
    }

    void addReadWriteMemoryId(int memory_id) {
        read_memory_id.set(memory_id);
        write_memory_id.set(memory_id);
        used_memory_id.set(memory_id);
    }

    [[nodiscard]] bool conflictWithIns(const ResourceAllocatePayload& ins_resource_allocate) const;

    ResourceAllocatePayload& operator+=(const ResourceAllocatePayload& other);

public:
    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};
    const SIMDFunctorConfig* simd_functor_cfg{nullptr};
    const ReduceFunctorConfig* reduce_functor_cfg{nullptr};

    MemoryBitmap read_memory_id;
    MemoryBitmap write_memory_id;
    MemoryBitmap used_memory_id;
};

struct ResourceReleasePayload {
    MAKE_SIGNAL_TYPE_TRACE_STREAM(ResourceReleasePayload)

    int ins_id{-1};

    DECLARE_CIM_PAYLOAD_FUNCTIONS(ResourceReleasePayload)
};

}  // namespace cimsim
