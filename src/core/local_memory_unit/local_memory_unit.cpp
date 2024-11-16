//
// Created by wyk on 2024/7/4.
//

#include "local_memory_unit.h"

#include "core/core.h"
#include "fmt/core.h"
#include "util/util.h"

namespace pimsim {

LocalMemoryUnit::LocalMemoryUnit(const char *name, const pimsim::LocalMemoryUnitConfig &config,
                                 const pimsim::SimConfig &sim_config, const PimUnitConfig &pim_config,
                                 pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), pim_config_(pim_config) {
    for (const auto &local_memory_config : config_.local_memory_list) {
        if (local_memory_config.type == +LocalMemoryType::ram)
            local_memory_list_.emplace_back(
                std::make_shared<Memory>(local_memory_config.name.c_str(), local_memory_config.ram_config,
                                         local_memory_config.addressing, sim_config, core, clk));
        else {
            local_memory_list_.emplace_back(
                std::make_shared<Memory>(local_memory_config.name.c_str(), local_memory_config.reg_buffer_config,
                                         local_memory_config.addressing, sim_config, core, clk));
        }
    }
}

std::vector<uint8_t> LocalMemoryUnit::read_data(const pimsim::InstructionPayload &ins, int address_byte, int size_byte,
                                                sc_core::sc_event &finish_access) {
    auto local_memory = getLocalMemoryByAddress(address_byte);
    if (local_memory == nullptr) {
        std::cerr << fmt::format("Core id: {}, Invalid memory read with ins NO.'{}': address {} does not match any "
                                 "local memory's address space",
                                 core_->getCoreId(), ins.pc, address_byte)
                  << std::endl;
        return {};
    }

    auto payload = std::make_shared<MemoryAccessPayload>(
        MemoryAccessPayload{.ins = ins,
                            .access_type = MemoryAccessType::read,
                            .address_byte = address_byte - local_memory->getAddressSpaceBegin(),
                            .size_byte = size_byte,
                            .finish_access = finish_access});
    local_memory->access(payload);
    wait(payload->finish_access);

    return std::move(payload->data);
}

void LocalMemoryUnit::write_data(const pimsim::InstructionPayload &ins, int address_byte, int size_byte,
                                 std::vector<uint8_t> data, sc_core::sc_event &finish_access) {
    if (address_byte >= pim_config_.address_space.offset_byte &&
        address_byte + size_byte < pim_config_.address_space.end()) {
        // calculate config
        int macro_bit_width = pim_config_.macro_size.bit_width_per_row * pim_config_.macro_size.element_cnt_per_compartment;
        int pim_bit_width = pim_config_.sram.as_mode == +PimSRAMAddressSpaceContinuousMode::intergroup
                                ? macro_bit_width * pim_config_.macro_total_cnt
                                : macro_bit_width * pim_config_.macro_group_size;
        int weight_bit_size = size_byte * BYTE_TO_BIT;
        int process_times = IntDivCeil(weight_bit_size, pim_bit_width);

        // load weight
        double dynamic_power_mW = pim_config_.sram.write_dynamic_power_per_bit_mW * pim_bit_width;
        double latency = pim_config_.sram.write_latency_cycle * period_ns_ * process_times;

        pim_load_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
        wait(latency, SC_NS);
    } else {
        auto local_memory = getLocalMemoryByAddress(address_byte);
        if (local_memory == nullptr) {
            std::cerr
                << fmt::format(
                       "Invalid memory write with ins NO.'{}': address does not match any local memory's address space",
                       ins.pc)
                << std::endl;
            return;
        }

        auto payload = std::make_shared<MemoryAccessPayload>(
            MemoryAccessPayload{.ins = ins,
                                .access_type = MemoryAccessType::write,
                                .address_byte = address_byte - local_memory->getAddressSpaceBegin(),
                                .size_byte = size_byte,
                                .data = std::move(data),
                                .finish_access = finish_access});
        local_memory->access(payload);
        wait(payload->finish_access);
    }
}

EnergyReporter LocalMemoryUnit::getEnergyReporter() {
    EnergyReporter local_memory_unit_reporter;
    for (auto &local_memory : local_memory_list_) {
        local_memory_unit_reporter.addSubModule(local_memory->getName(), local_memory->getEnergyReporter());
    }
    local_memory_unit_reporter.addSubModule("PimLoad", EnergyReporter{pim_load_energy_counter_});
    return std::move(local_memory_unit_reporter);
}

int LocalMemoryUnit::getLocalMemoryIdByAddress(int address_byte) const {
    for (int i = 0; i < local_memory_list_.size(); i++) {
        auto &local_memory = local_memory_list_[i];
        if (local_memory->getAddressSpaceBegin() <= address_byte && address_byte < local_memory->getAddressSpaceEnd()) {
            return i;
        }
    }
    return -1;
}

int LocalMemoryUnit::getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const {
    return local_memory_list_[memory_id]->getMemoryDataWidthByte(access_type);
}

int LocalMemoryUnit::getMemorySizeById(int memory_id) const {
    return local_memory_list_[memory_id]->getMemorySizeByte();
}

std::shared_ptr<Memory> LocalMemoryUnit::getLocalMemoryByAddress(int address_byte) {
    for (auto &local_memory : local_memory_list_) {
        if (local_memory->getAddressSpaceBegin() <= address_byte && address_byte < local_memory->getAddressSpaceEnd()) {
            return local_memory;
        }
    }
    return nullptr;
}

}  // namespace pimsim
