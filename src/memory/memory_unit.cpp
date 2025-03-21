//
// Created by wyk on 2024/7/4.
//

#include "memory_unit.h"

#include "core/core.h"
#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

MemoryUnit::MemoryUnit(const sc_core::sc_module_name &name, const cimsim::MemoryUnitConfig &config,
                       const cimsim::SimConfig &sim_config, cimsim::Core *core, cimsim::Clock *clk, bool is_global)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , sim_config_(sim_config)
    , as_(AddressSapce::getInstance())
    , is_global_(is_global) {
    memory_list_.resize(as_.getMemoryCount(is_global_));
    for (const auto &mem_cfg : config_.memory_list) {
        for (int i = 0; i < mem_cfg.duplicate_cnt; i++) {
            std::string mem_name = getDuplicateMemoryName(mem_cfg.getMemoryName(), i, mem_cfg.duplicate_cnt);
            int mem_id = as_.getMemoryId(mem_name);
            auto mem_ptr =
                mem_cfg.type == +MemoryType::ram
                    ? std::make_shared<Memory>(mem_name.c_str(), mem_cfg.ram_config, sim_config, core, clk)
                    : std::make_shared<Memory>(mem_name.c_str(), mem_cfg.reg_buffer_config, sim_config, core, clk);
            mem_ptr->setMemoryID(mem_id);
            memory_list_[mem_id] = mem_ptr;
        }
    }
}

void MemoryUnit::mountMemory(MemoryHardware *memory_hardware) {
    int mem_id = as_.getMemoryId(memory_hardware->getMemoryName());
    memory_hardware->setMemoryID(mem_id);
    auto memory_module_name =
        fmt::format("{}MountedMemory_{}", (is_global_ ? "Global" : "Local"), memory_hardware->getMemoryName());
    memory_list_[mem_id] =
        std::make_shared<Memory>(memory_module_name.c_str(), memory_hardware, sim_config_, core_, clk_);
}

void MemoryUnit::access(const std::shared_ptr<MemoryAccessPayload> &payload) {
    auto memory = getMemoryByAddress(payload->address_byte);
    if (memory == nullptr) {
        std::cerr << fmt::format(
                         "Invalid memory {} with ins NO.'{}': address does not match any memory's address space",
                         payload->access_type._to_string(), payload->ins.pc)
                  << std::endl;
        return;
    }
    payload->address_byte -= memory->getAddressSpaceOffset();
    memory->access(payload);
    wait(payload->finish_access);
}

EnergyReporter MemoryUnit::getEnergyReporter() {
    EnergyReporter memory_unit_reporter;
    for (auto &memory : memory_list_) {
        if (memory && !memory->isMount()) {
            memory_unit_reporter.addSubModule(memory->getName(), memory->getEnergyReporter());
        }
    }
    return std::move(memory_unit_reporter);
}

int MemoryUnit::getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return memory_list_[memory_id]->getMemoryDataWidthByte(access_type);
}

int MemoryUnit::getMemorySizeById(int memory_id) const {
    return memory_list_[memory_id]->getMemorySizeByte();
}

std::shared_ptr<Memory> MemoryUnit::getMemoryByAddress(int address_byte) {
    int mem_id = is_global_ ? as_.getGlobalMemoryId(address_byte) : as_.getLocalMemoryId(address_byte);
    return memory_list_[mem_id];
}

}  // namespace cimsim
