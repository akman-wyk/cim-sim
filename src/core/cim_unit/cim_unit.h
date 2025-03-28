//
// Created by wyk on 2025/2/13.
//

#pragma once

#include <functional>

#include "config/config.h"
#include "macro_group.h"
#include "memory/memory_hardware.h"

namespace cimsim {

class CimUnit : public MemoryHardware {
public:
    SC_HAS_PROCESS(CimUnit);

    CimUnit(const sc_module_name& name, const CimUnitConfig& config, const BaseInfo& base_info);

    EnergyReporter getEnergyReporter() override;

    // As a local memory
    sc_time accessAndGetDelay(MemoryAccessPayload& payload) override;
    int getMemoryDataWidthByte(MemoryAccessType access_type) const override;
    int getMemorySizeByte() const override;
    const std::string& getMemoryName() override;

    // As a cim compute unit
    int getConfigMacroGroupCount() const;
    int getActualMacroGroupCount() const;
    bool isMacroSimulation() const;

    void setMacroGroupActivationElementColumn(const std::vector<unsigned char>& mask, bool group_broadcast,
                                              int group_id);
    int getMacroGroupActivationElementColumnCount(int group_id) const;
    int getMacroGroupActivationMacroCount(int group_id) const;
    int getMacroGroupMaxActivationMacroCount() const;

    void runMacroGroup(int group_id, MacroGroupPayload group_payload);

    // Other Interface
    void bindCimComputeUnit(const std::function<void(int)>& release_resource_func,
                            const std::function<void()>& finish_ins_func);

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;
    const int cim_byte_size_;
    const int cim_bit_width_;
    const int cim_byte_width_;

    int config_group_cnt_;
    bool macro_simulation_;  // whether to user one actual macro to simulate all logic macros in one core
    std::vector<std::shared_ptr<MacroGroup>> macro_group_list_;

    int local_memory_id_{-1};

    EnergyCounter sram_read_energy_counter_;
    EnergyCounter sram_write_energy_counter_;
};

}  // namespace cimsim
