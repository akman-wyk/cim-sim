//
// Created by wyk on 2025/3/14.
//

#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "config/config.h"

namespace cimsim {

struct AddressSpaceInfo {
    int as_offset{0};
    int memory_id{-1};
    bool is_global{false};
};

class AddressSapce {
public:
    static void initialize(const ChipConfig& chip_config);

    static const AddressSapce& getInstance();

private:
    static std::unique_ptr<AddressSapce> as_ptr_;

public:
    [[nodiscard]] int getMemoryId(const std::string& name) const;
    [[nodiscard]] int getMemoryAddressSpaceOffset(const std::string& name) const;
    [[nodiscard]] int getMemoryCount(bool is_global) const;

    [[nodiscard]] int getLocalMemoryId(int address_byte) const;
    [[nodiscard]] int getGlobalMemoryId(int address_byte) const;
    [[nodiscard]] bool isAddressGlobal(int address_byte) const;

private:
    explicit AddressSapce(const ChipConfig& chip_config);

    std::unordered_map<std::string, AddressSpaceInfo> as_name_map_;
    std::map<int, AddressSpaceInfo> as_address_map_;
    int local_mem_cnt_{0};
    int global_mem_cnt_{0};
};

}  // namespace cimsim
