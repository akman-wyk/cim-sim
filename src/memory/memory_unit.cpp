//
// Created by wyk on 2024/7/4.
//

#include "memory_unit.h"

#include "core/core.h"
#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

#define ErrorMemoryId -10

MemoryUnit::MemoryUnit(const char *name, const cimsim::MemoryUnitConfig &config, const cimsim::SimConfig &sim_config,
                       cimsim::Core *core, cimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), sim_config_(sim_config) {
    for (const auto &memory_config : config_.memory_list) {
        if (memory_config.type == +MemoryType::ram)
            memory_list_.emplace_back(std::make_shared<Memory>(memory_config.name.c_str(), memory_config.ram_config,
                                                               memory_config.addressing, sim_config, core, clk));
        else {
            memory_list_.emplace_back(std::make_shared<Memory>(memory_config.name.c_str(),
                                                               memory_config.reg_buffer_config,
                                                               memory_config.addressing, sim_config, core, clk));
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
                            .address_byte = address_byte - memory->getAddressSpaceBegin(),
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
                            .address_byte = address_byte - memory->getAddressSpaceBegin(),
                            .size_byte = size_byte,
                            .data = std::move(data),
                            .finish_access = finish_access});
    memory->access(payload);
    wait(payload->finish_access);
}

void MemoryUnit::access(const std::shared_ptr<MemoryAccessPayload>& payload) {
    auto memory = getMemoryByAddress(payload->address_byte);
    if (memory == nullptr) {
        std::cerr << fmt::format(
                         "Invalid memory {} with ins NO.'{}': address does not match any memory's address space",
                         payload->access_type._to_string(), payload->ins.pc)
                  << std::endl;
        return;
    }
    payload->address_byte -= memory->getAddressSpaceBegin();
    memory->access(payload);
    wait(payload->finish_access);
}

EnergyReporter MemoryUnit::getEnergyReporter() {
    EnergyReporter memory_unit_reporter;
    for (auto &memory : memory_list_) {
        if (!memory->isMount()) {
            memory_unit_reporter.addSubModule(memory->getName(), memory->getEnergyReporter());
        }
    }
    return std::move(memory_unit_reporter);
}

void MemoryUnit::bindCimUnit(CimUnit *cim_unit) {
    memory_list_.emplace_back(
        std::make_shared<Memory>("cim unit", cim_unit, cim_unit->getAddressSpaceConfig(), sim_config_, core_, clk_));
    cim_unit->bindLocalMemoryUnit(getMemoryIdByAddress(cim_unit->getAddressSpaceConfig().offset_byte));
}

int MemoryUnit::getMemoryIdByAddress(int address_byte) const {
    for (int i = 0; i < memory_list_.size(); i++) {
        auto &memory = memory_list_[i];
        if (memory->getAddressSpaceBegin() <= address_byte && address_byte < memory->getAddressSpaceEnd()) {
            return i;
        }
    }
    return ErrorMemoryId;
}

int MemoryUnit::getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return memory_list_[memory_id]->getMemoryDataWidthByte(access_type);
}

int MemoryUnit::getMemorySizeById(int memory_id) const {
    return memory_list_[memory_id]->getMemorySizeByte();
}

std::shared_ptr<Memory> MemoryUnit::getMemoryByAddress(int address_byte) {
    for (auto &memory : memory_list_) {
        if (memory->getAddressSpaceBegin() <= address_byte && address_byte < memory->getAddressSpaceEnd()) {
            return memory;
        }
    }
    return nullptr;
}

}  // namespace cimsim
