//
// Created by wyk on 2024/7/20.
//

#pragma once
#include <functional>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "payload.h"

namespace cimsim {

class Macro : public BaseModule {
public:
    SC_HAS_PROCESS(Macro);

    Macro(const sc_core::sc_module_name& name, const CimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
          bool independent_ipu, SubmoduleSocket<MacroGroupSubmodulePayload>* result_adder_socket_ptr = nullptr);

    void startExecute(MacroPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

    static void waitAndStartNextSubmodule(const MacroSubmodulePayload& cur_payload,
                                          SubmoduleSocket<MacroSubmodulePayload>& next_submodule_socket);

    void setFinishInsFunction(std::function<void()> finish_func);

    void setActivationElementColumn(const std::vector<unsigned char>& macros_activation_element_col_mask,
                                    int start_index = 0);
    int getActivationElementColumnCount() const;

private:
    [[noreturn]] void processIPUAndIssue();
    [[noreturn]] void processSRAMSubmodule();
    [[noreturn]] void processPostProcessSubmodule();
    [[noreturn]] void processAdderTreeSubmodule1();
    [[noreturn]] void processAdderTreeSubmodule2();
    [[noreturn]] void processShiftAdderSubmodule();

    std::pair<int, int> getBatchCountAndActivationCompartmentCount(const MacroPayload& payload) const;

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;
    bool independent_ipu_;
    int activation_element_col_cnt_;
    std::vector<unsigned char> activation_element_col_mask_{};

    SubmoduleSocket<MacroPayload> macro_socket_{};

    SubmoduleSocket<MacroSubmodulePayload> sram_socket_{};
    SubmoduleSocket<MacroSubmodulePayload> post_process_socket_{};
    SubmoduleSocket<MacroSubmodulePayload> adder_tree_socket_1_{};
    SubmoduleSocket<MacroSubmodulePayload> adder_tree_socket_2_{};
    SubmoduleSocket<MacroSubmodulePayload> shift_adder_socket_{};

    SubmoduleSocket<MacroGroupSubmodulePayload>* result_adder_socket_ptr_{nullptr};

    EnergyCounter ipu_energy_counter_;
    EnergyCounter sram_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
    EnergyCounter post_process_energy_counter_;
    EnergyCounter adder_tree_energy_counter_{true};
    EnergyCounter shift_adder_energy_counter_;
    EnergyCounter result_adder_energy_counter_;

    // for test
    std::function<void()> finish_ins_func_{};
};

}  // namespace cimsim
