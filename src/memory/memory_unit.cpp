//
// Created by wyk on 2024/7/4.
//

#include "memory_unit.h"

#include "core/core.h"
#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

MemoryUnit::MemoryUnit(const char *name, const cimsim::MemoryUnitConfig &config, const cimsim::SimConfig &sim_config,
                       cimsim::Core *core, cimsim::Clock *clk, bool is_global)
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
            auto mem_ptr = mem_cfg.type == +MemoryType::ram
                               ? std::make_shared<Memory>(mem_name, mem_cfg.ram_config, sim_config, core, clk)
                               : std::make_shared<Memory>(mem_name, mem_cfg.reg_buffer_config, sim_config, core, clk);
            memory_list_[mem_id] = mem_ptr;
        }
    }
}

std::vector<uint8_t> MemoryUnit::read_data(const cimsim::InstructionPayload &ins, int address_byte, int size_byte,
                                           sc_core::sc_event &finish_access) {
    auto memory = getMemoryByAddress(address_byte);
    if (memory == nullptr) {
        std::cerr << fmt::format("Core id: {}, Invalid memory read with ins NO.'{}': address {} does not match any "
                                 "memory's address space",
                                 core_->getCoreId(), ins.pc, address_byte)
                  << std::endl;
        return {};
    }

    auto payload = std::make_shared<MemoryAccessPayload>(
        MemoryAccessPayload{.ins = ins,
                            .access_type = MemoryAccessType::read,
                            .address_byte = address_byte - memory->getAddressSpaceOffset(),
                            .size_byte = size_byte,
                            .finish_access = finish_access});
    memory->access(payload);
    wait(payload->finish_access);

    return std::move(payload->data);
}

void MemoryUnit::write_data(const cimsim::InstructionPayload &ins, int address_byte, int size_byte,
                            std::vector<uint8_t> data, sc_core::sc_event &finish_access) {
    auto memory = getMemoryByAddress(address_byte);
    if (memory == nullptr) {
        std::cerr << fmt::format(
                         "Invalid memory write with ins NO.'{}': address does not match any memory's address space",
                         ins.pc)
                  << std::endl;
        return;
    }

    auto payload = std::make_shared<MemoryAccessPayload>(
        MemoryAccessPayload{.ins = ins,
                            .access_type = MemoryAccessType::write,
                            .address_byte = address_byte - memory->getAddressSpaceOffset(),
                            .size_byte = size_byte,
                            .data = std::move(data),
                            .finish_access = finish_access});
    memory->access(payload);
    wait(payload->finish_access);
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

void MemoryUnit::bindCimUnit(CimUnit *cim_unit) {
    int mem_id = as_.getMemoryId(cim_unit->getMemoryName());
    memory_list_[mem_id] = std::make_shared<Memory>("cim unit", cim_unit, sim_config_, core_, clk_);
    cim_unit->setLocalMemoryId(mem_id);
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
