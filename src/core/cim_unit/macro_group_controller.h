//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "payload.h"

namespace pimsim {

class MacroGroupController : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroupController);

    MacroGroupController(const std::string& name, const PimUnitConfig& config, const SimConfig& sim_config, Core* core,
                         Clock* clk, sc_core::sc_event& next_sub_ins,
                         SubmoduleSocket<MacroGroupSubmodulePayload>& result_adder_socket);

    void start(MacroGroupControllerPayload payload);
    void waitUntilFinishIfBusy();

private:
    static void waitAndStartNextSubmodule(const MacroGroupSubmodulePayload& cur_payload,
                                          SubmoduleSocket<MacroGroupSubmodulePayload>& next_submodule_socket);

    [[noreturn]] void processIPUAndIssue();
    [[noreturn]] void processSRAMSubmodule();
    [[noreturn]] void processPostProcessSubmodule();
    [[noreturn]] void processAdderTreeSubmodule1();
    [[noreturn]] void processAdderTreeSubmodule2();
    [[noreturn]] void processShiftAdderSubmodule();

private:
    const PimUnitConfig& config_;

    // socket from MacroGroup
    SubmoduleSocket<MacroGroupControllerPayload> controller_socket_;

    // sockets in MacroGroupController
    SubmoduleSocket<MacroGroupSubmodulePayload> sram_socket_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> post_process_socket_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> adder_tree_socket_1_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> adder_tree_socket_2_{};
    SubmoduleSocket<MacroGroupSubmodulePayload> shift_adder_socket_{};

    // sockets to MacroGroup
    sc_core::sc_event& next_sub_ins_;
    SubmoduleSocket<MacroGroupSubmodulePayload>& result_adder_socket_;
};

}  // namespace pimsim
