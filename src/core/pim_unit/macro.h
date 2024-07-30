//
// Created by wyk on 2024/7/20.
//

#pragma once
#include <functional>

#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "pim_payload.h"

namespace pimsim {

class Macro : public BaseModule {
public:
    SC_HAS_PROCESS(Macro);

    Macro(const char* name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
          bool independent_ipu, SubmoduleSocket<MacroGroupSubmodulePayload>* result_adder_socket_ptr = nullptr);

    void startExecute(MacroPayload payload);
    void waitUntilFinishIfBusy();

    EnergyReporter getEnergyReporter() override;

    static void waitAndStartNextSubmodule(const MacroSubmodulePayload& cur_payload,
                                          SubmoduleSocket<MacroSubmodulePayload>& next_submodule_socket);

    void setFinishRunFunction(std::function<void()> finish_func);

private:
    [[noreturn]] void processIPUAndIssue();
    [[noreturn]] void processSRAMSubmodule();
    [[noreturn]] void processPostProcessSubmodule();
    [[noreturn]] void processAdderTreeSubmodule1();
    [[noreturn]] void processAdderTreeSubmodule2();
    [[noreturn]] void processShiftAdderSubmodule();

    std::pair<int, int> getBatchCountAndActivationCompartmentCount(const MacroPayload& payload);

private:
    const PimUnitConfig& config_;
    const PimMacroSizeConfig& macro_size_;
    bool independent_ipu_;

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
    EnergyCounter adder_tree_energy_counter_;
    EnergyCounter shift_adder_energy_counter_;
    EnergyCounter result_adder_energy_counter_;

    // for test
    std::function<void()> finish_run_func_{};
};

}  // namespace pimsim
