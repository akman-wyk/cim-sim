//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "config/config.h"
#include "macro.h"
#include "macro_group_module.h"

namespace cimsim {

class MacroGroup : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroup);

    MacroGroup(const sc_module_name& name, const CimUnitConfig& config, const BaseInfo& base_info,
               EnergyCounter& cim_unit_energy_counter, bool macro_simulation = false);

    void startExecute(MacroGroupPayload payload);
    void waitUntilFinishIfBusy();

    void setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func);
    void setFinishInsFunc(std::function<void()> finish_ins_func);

    void setMacrosActivationElementColumn(const std::vector<unsigned char>& macros_activation_element_col_mask);
    int getActivationMacroCount() const;
    int getActivationElementColumnCount() const;

private:
    [[noreturn]] void processIPUAndIssue();

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;

    std::vector<std::shared_ptr<Macro>> macro_list_;
    int activation_macro_cnt_{0};

    SubmoduleSocket<MacroGroupPayload> macro_group_socket_{};

    // modules in MacroGroup, for controlling
    std::shared_ptr<MacroGroupModule> sram_read_{nullptr};
    std::shared_ptr<MacroGroupModule> post_process_{nullptr};
    std::shared_ptr<MacroGroupModule> mult_{nullptr};
    std::shared_ptr<MacroGroupModule> adder_tree_{nullptr};
    std::shared_ptr<MacroGroupModule> shift_adder_{nullptr};
    std::shared_ptr<MacroGroupModule> result_adder_{nullptr};
};

}  // namespace cimsim
