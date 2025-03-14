//
// Created by wyk on 2025/2/13.
//

#include "cim_unit.h"

#include "core/core.h"
#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

CimUnit::CimUnit(const char* name, const CimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk)
    : MemoryHardware(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config_.macro_size)
    , cim_byte_size_(config_.getByteSize())
    , cim_bit_width_(config_.getBitWidth())
    , cim_byte_width_(config_.getByteWidth())
    , config_group_cnt_(config_.macro_total_cnt / config_.macro_group_size)
    , macro_simulation_(sim_config.data_mode == +DataMode::not_real_data && !config_.bit_sparse &&
                        !config_.input_bit_sparse && !config_.value_sparse) {
    for (int group_id = 0; group_id < (macro_simulation_ ? 1 : config_group_cnt_); group_id++) {
        auto macro_name = fmt::format("MacroGroup_{}", group_id);
        auto macro_group = new MacroGroup{macro_name.c_str(), config_, sim_config, core, clk, macro_simulation_};
        macro_group_list_.emplace_back(macro_group);
    }
}

EnergyReporter CimUnit::getEnergyReporter() {
    EnergyReporter cim_unit_energy_reporter{};
    cim_unit_energy_reporter.addSubModule("sram read", EnergyReporter{sram_read_energy_counter_});
    cim_unit_energy_reporter.addSubModule("sram write", EnergyReporter{sram_write_energy_counter_});
    for (auto* macro_group : macro_group_list_) {
        cim_unit_energy_reporter.accumulate(macro_group->getEnergyReporter(), true);
    }
    return std::move(cim_unit_energy_reporter);
}

int CimUnit::getMemorySizeByte() const {
    return cim_byte_size_;
}

int CimUnit::getMemoryDataWidthByte(MemoryAccessType access_type) const {
    return cim_byte_width_;
}

const std::string& CimUnit::getMemoryName() {
    return config_.name_as_memory;
}

sc_core::sc_time CimUnit::accessAndGetDelay(MemoryAccessPayload& payload) {
    if (payload.address_byte < 0 || payload.address_byte + payload.size_byte > cim_byte_size_) {
        std::cerr << fmt::format("Core id: {}, Invalid memory access with ins NO.'{}': address {} overflow, size: {}, "
                                 "config size: {}",
                                 core_->getCoreId(), payload.ins.pc, payload.address_byte, payload.size_byte,
                                 cim_byte_size_)
                  << std::endl;
        return {0.0, sc_core::SC_NS};
    }

    int payload_bit_size = payload.size_byte * BYTE_TO_BIT;
    int process_times = IntDivCeil(payload_bit_size, cim_bit_width_);
    double latency;
    if (payload.access_type == +MemoryAccessType::read) {
        double dynamic_power_mW = config_.sram.read_dynamic_power_per_bit_mW * cim_bit_width_;
        latency = config_.sram.read_latency_cycle * period_ns_ * process_times;
        sram_read_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
    } else {
        double dynamic_power_mW = config_.sram.write_dynamic_power_per_bit_mW * cim_bit_width_;
        latency = config_.sram.write_latency_cycle * period_ns_ * process_times;
        sram_write_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
    }

    return {latency, sc_core::SC_NS};
}

int CimUnit::getConfigMacroGroupCount() const {
    return config_group_cnt_;
}

int CimUnit::getActualMacroGroupCount() const {
    return static_cast<int>(macro_group_list_.size());
}

bool CimUnit::isMacroSimulation() const {
    return macro_simulation_;
}

void CimUnit::setMacroGroupActivationElementColumn(const std::vector<unsigned char>& mask, bool group_broadcast,
                                                   int group_id) {
    if (group_broadcast) {
        for (auto* macro_group : macro_group_list_) {
            macro_group->setMacrosActivationElementColumn(mask);
        }
    } else if (0 <= group_id && group_id < macro_group_list_.size()) {
        macro_group_list_[group_id]->setMacrosActivationElementColumn(mask);
    }
}

int CimUnit::getMacroGroupActivationMacroCount(int group_id) const {
    if (group_id < 0 || group_id >= macro_group_list_.size()) {
        return 0;
    }
    return macro_group_list_[group_id]->getActivationMacroCount();
}

int CimUnit::getMacroGroupActivationElementColumnCount(int group_id) const {
    if (group_id < 0 || group_id >= macro_group_list_.size()) {
        return 0;
    }
    return macro_group_list_[group_id]->getActivationElementColumnCount();
}

int CimUnit::getMacroGroupMaxActivationMacroCount() const {
    return std::transform_reduce(
        macro_group_list_.begin(), macro_group_list_.end(), 0, [](int a, int b) { return std::max(a, b); },
        [](const MacroGroup* macro_group) { return macro_group->getActivationMacroCount(); });
}

void CimUnit::runMacroGroup(int group_id, MacroGroupPayload group_payload) {
    auto* macro_group = macro_group_list_[group_id];
    macro_group->waitUntilFinishIfBusy();
    macro_group->startExecute(std::move(group_payload));
}

void CimUnit::bindCimComputeUnit(const std::function<void(int)>& release_resource_func,
                                 const std::function<void()>& finish_ins_func) {
    for (auto* macro_group : macro_group_list_) {
        macro_group->setReleaseResourceFunc(release_resource_func);
        macro_group->setFinishInsFunc(finish_ins_func);
    }
}

void CimUnit::setLocalMemoryId(int memory_id) {
    local_memory_id_ = memory_id;
}

int CimUnit::getLocalMemoryId() const {
    return local_memory_id_;
}

}  // namespace cimsim
