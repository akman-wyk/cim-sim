//
// Created by wyk on 2025/3/10.
//

#include "payload.h"

#include "util/util.h"

namespace cimsim {

void MemoryBitmap::set(int index) {
    bitmap_[index >> LOG2_BITMAP_CELL_BIT_WIDTH] |= (1 << (index & BITMAP_CELL_BIT_WIDTH_MASK));
}

void MemoryBitmap::unset(int index) {
    bitmap_[index >> LOG2_BITMAP_CELL_BIT_WIDTH] &= (~(1 << (index & BITMAP_CELL_BIT_WIDTH_MASK)));
}

MemoryBitmap& MemoryBitmap::operator|=(const MemoryBitmap& ano) {
    for (int i = 0; i < MEMORY_BITMAP_SIZE; i++) {
        bitmap_[i] |= ano.bitmap_[i];
    }
    return *this;
}

bool MemoryBitmap::operator==(const MemoryBitmap& ano) const {
    for (int i = 0; i < MEMORY_BITMAP_SIZE; i++) {
        if (bitmap_[i] != ano.bitmap_[i]) {
            return false;
        }
    }
    return true;
}

bool MemoryBitmap::intersectionWith(const MemoryBitmap& ano) const {
    for (int i = 0; i < MEMORY_BITMAP_SIZE; i++) {
        if ((bitmap_[i] & ano.bitmap_[i]) != 0) {
            return true;
        }
    }
    return false;
}

std::vector<int> MemoryBitmap::getIndexList() const {
    std::vector<int> index_list;
    int offset = 0;
    for (auto cell : bitmap_) {
        while (cell != 0) {
            index_list.emplace_back(offset + __builtin_ctz(cell));
            cell &= (cell - 1);
        }
        offset += BITMAP_CELL_BIT_WIDTH;
    }
    return std::move(index_list);
}

std::stringstream& operator<<(std::stringstream& out, const MemoryBitmap& memory_bitmap) {
    out << "[";
    auto index_list = memory_bitmap.getIndexList();
    for (int i = 0; i < index_list.size(); i++) {
        if (i > 0) {
            out << ", ";
        }
        out << index_list[i];
    }
    out << "]";
    return out;
}

DEFINE_CIM_PAYLOAD_FUNCTIONS(ResourceAllocatePayload, ins_id, unit_type, read_memory_id, write_memory_id,
                             used_memory_id)

bool ResourceAllocatePayload::conflictWithIns(const ResourceAllocatePayload& ins_resource_allocate) const {
    if (unit_type == +ExecuteUnitType::simd && ins_resource_allocate.unit_type == +ExecuteUnitType::simd &&
        simd_functor_cfg != nullptr && ins_resource_allocate.simd_functor_cfg != nullptr &&
        simd_functor_cfg != ins_resource_allocate.simd_functor_cfg) {
        return true;
    }
    if (unit_type == +ExecuteUnitType::reduce && ins_resource_allocate.unit_type == +ExecuteUnitType::reduce &&
        reduce_functor_cfg != nullptr && ins_resource_allocate.reduce_functor_cfg != nullptr &&
        reduce_functor_cfg != ins_resource_allocate.reduce_functor_cfg) {
        return true;
    }
    if (ins_resource_allocate.unit_type == this->unit_type) {
        return this->write_memory_id.intersectionWith(ins_resource_allocate.read_memory_id);
    }
    return this->used_memory_id.intersectionWith(ins_resource_allocate.used_memory_id);
}

ResourceAllocatePayload& ResourceAllocatePayload::operator+=(const cimsim::ResourceAllocatePayload& other) {
    this->read_memory_id |= other.read_memory_id;
    this->write_memory_id |= other.write_memory_id;
    this->used_memory_id |= other.used_memory_id;
    this->simd_functor_cfg = other.simd_functor_cfg;
    this->reduce_functor_cfg = other.reduce_functor_cfg;
    return *this;
}

DEFINE_CIM_PAYLOAD_FUNCTIONS(ResourceReleasePayload, ins_id)

}  // namespace cimsim
