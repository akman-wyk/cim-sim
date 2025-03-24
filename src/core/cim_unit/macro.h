//
// Created by wyk on 2024/7/20.
//

#pragma once
#include <functional>

#include "base_component/base_module.h"
#include "config/config.h"
#include "macro_module.h"

namespace cimsim {

using MacroSubmoduleSocket = SubmoduleSocket<MacroSubmodulePayload>;

class Macro : public BaseModule {
public:
    SC_HAS_PROCESS(Macro);

    Macro(const sc_module_name& name, const CimUnitConfig& config, const BaseInfo& base_info, bool independent_ipu);

    void startExecute(MacroPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

    void setActivationElementColumn(const std::vector<unsigned char>& macros_activation_element_col_mask,
                                    int start_index = 0);
    int getActivationElementColumnCount() const;

    void bindNextModuleSocket(MacroStageSocket* next_module_socket);

private:
    [[noreturn]] void processIPUAndIssue();

    static double getSRAMReadDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getPostProcessDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getAdderTreeDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getShiftAdderDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getResultAdderDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);

    std::pair<int, int> getBatchCountAndActivationCompartmentCount(const MacroPayload& payload) const;

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;
    bool independent_ipu_;
    int activation_element_col_cnt_;

    SubmoduleSocket<MacroPayload> macro_socket_{};

    MacroModule sram_read_;
    MacroModule post_process_;
    MacroModule adder_tree_;
    MacroModule shift_adder_;
    MacroModule result_adder_;

    EnergyCounter ipu_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace cimsim
