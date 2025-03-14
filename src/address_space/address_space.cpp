//
// Created by wyk on 2025/3/14.
//

#include "address_space.h"

#include <iostream>

#include "fmt/format.h"

namespace cimsim {

#define ERROR_MEMORY_ID        -10
#define ERROR_MEMORY_AS_OFFSET -1

void AddressSapce::initialize(const ChipConfig& chip_config) {
    if (as_ptr_ == nullptr) {
        as_ptr_ = std::unique_ptr<AddressSapce>(new AddressSapce(chip_config));
    }
}

const AddressSapce& AddressSapce::getInstance() {
    return *as_ptr_;
}

std::unique_ptr<AddressSapce> AddressSapce::as_ptr_{nullptr};

AddressSapce::AddressSapce(const ChipConfig& chip_config) {
    auto mem_map = chip_config.getMemoryNameToSizeMap();
    int offset = 0;
    for (auto& as_element : chip_config.address_space_config) {
        auto& mem_info = mem_map[as_element.name];
        int size = as_element.size < 0 ? mem_info.size : as_element.size;

        AddressSpaceInfo as_info;
        if (mem_info.is_global) {
            as_info = {.as_offset = offset, .memory_id = global_mem_cnt_, .is_global = true};
            global_mem_cnt_++;
        } else {
            as_info = {.as_offset = offset, .memory_id = local_mem_cnt_, .is_global = false};
            local_mem_cnt_++;
        }

        as_name_map_.emplace(as_element.name, as_info);
        as_address_map_.emplace(offset, as_info);
        offset += size;
    }
}

int AddressSapce::getMemoryId(const std::string& name) const {
    auto found = as_name_map_.find(name);
    if (found == as_name_map_.end()) {
        std::cerr << fmt::format("Name \'{}\' does not match any memory", name) << std::endl;
        return ERROR_MEMORY_ID;
    }
    return found->second.memory_id;
}

int AddressSapce::getMemoryAddressSpaceOffset(const std::string& name) const {
    auto found = as_name_map_.find(name);
    if (found == as_name_map_.end()) {
        std::cerr << fmt::format("Name \'{}\' does not match any memory", name) << std::endl;
        return ERROR_MEMORY_AS_OFFSET;
    }
    return found->second.as_offset;
}

int AddressSapce::getMemoryCount(bool is_global) const {
    return is_global ? global_mem_cnt_ : local_mem_cnt_;
}

int AddressSapce::getLocalMemoryId(int address_byte) const {
    auto it = as_address_map_.upper_bound(address_byte);
    if (it == as_address_map_.begin()) {
        std::cerr << fmt::format("Address \'{}\' does not match any memory's address space", address_byte) << std::endl;
        return ERROR_MEMORY_ID;
    }

    --it;
    if (it->second.is_global) {
        std::cerr << fmt::format("Address \'{}\' match a global memory, but requires local memory", address_byte)
                  << std::endl;
        return ERROR_MEMORY_ID;
    }
    return it->second.memory_id;
}

int AddressSapce::getGlobalMemoryId(int address_byte) const {
    auto it = as_address_map_.upper_bound(address_byte);
    if (it == as_address_map_.begin()) {
        std::cerr << fmt::format("Address \'{}\' does not match any memory's address space", address_byte) << std::endl;
        return ERROR_MEMORY_ID;
    }

    --it;
    if (!it->second.is_global) {
        std::cerr << fmt::format("Address \'{}\' match a local memory, but requires global memory", address_byte)
                  << std::endl;
        return ERROR_MEMORY_ID;
    }
    return it->second.memory_id;
}

bool AddressSapce::isAddressGlobal(int address_byte) const {
    auto it = as_address_map_.upper_bound(address_byte);
    if (it == as_address_map_.begin()) {
        std::cerr << fmt::format("Address \'{}\' does not match any memory's address space", address_byte) << std::endl;
        return false;
    }

    --it;
    return it->second.is_global;
}

}  // namespace cimsim
