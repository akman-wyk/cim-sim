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

    Macro(const sc_module_name& name, const CimUnitConfig& config, const BaseInfo& base_info, bool independent_ipu,
          EnergyCounter& cim_unit_energy_counter, bool macro_simulation = false);

    void startExecute(MacroPayload payload);
    void waitUntilFinishIfBusy();

    void setActivationElementColumn(const std::vector<unsigned char>& macros_activation_element_col_mask,
                                    int start_index = 0);
    int getActivationElementColumnCount() const;

    void bindNextModuleSocket(MacroStageSocket* next_module_socket);

private:
    [[noreturn]] void processIPUAndIssue();

    static double getSRAMReadDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getPostProcessDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
    static double getMultDynamicPower(const CimUnitConfig& config, const MacroSubmodulePayload& payload);
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

    std::shared_ptr<MacroModule> sram_read_{nullptr};
    std::shared_ptr<MacroModule> post_process_{nullptr};
    std::shared_ptr<MacroModule> mult_{nullptr};
    std::shared_ptr<MacroModule> adder_tree_{nullptr};
    std::shared_ptr<MacroModule> shift_adder_{nullptr};
    std::shared_ptr<MacroModule> result_adder_{nullptr};

    EnergyCounter ipu_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace cimsim
