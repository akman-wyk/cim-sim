//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "config/config.h"
#include "macro.h"
#include "macro_group_controller.h"

namespace cimsim {

class MacroGroup : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroup);

    MacroGroup(const char* name, const CimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
               bool macro_simulation = false);

    void startExecute(MacroGroupPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

    void setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func);
    void setFinishInsFunc(std::function<void()> finish_ins_func);

    void setMacrosActivationElementColumn(const std::vector<unsigned char>& macros_activation_element_col_mask);
    int getActivationMacroCount() const;
    int getActivationElementColumnCount() const;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processResultAdderSubmodule();

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;

    MacroGroupController controller_;
    std::vector<Macro*> macro_list_;
    int activation_macro_cnt_{0};

    SubmoduleSocket<MacroGroupPayload> macro_group_socket_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> result_adder_socket_{};

    std::function<void(int ins_id)> release_resource_func_;
    std::function<void()> finish_ins_func_;

    sc_core::sc_event next_sub_ins_;
};

}  // namespace cimsim
