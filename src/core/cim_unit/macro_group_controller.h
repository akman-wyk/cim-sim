//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "base_component/base_module.h"
#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "payload.h"

namespace cimsim {

using MacroGroupSubmoduleSocket = SubmoduleSocket<MacroGroupSubmodulePayload>;

class MacroGroupController : public BaseModule {
public:
    SC_HAS_PROCESS(MacroGroupController);

    MacroGroupController(const sc_core::sc_module_name& name, const CimUnitConfig& config, const SimConfig& sim_config,
                         Core* core, Clock* clk, sc_core::sc_event& next_sub_ins);

    void start(MacroGroupControllerPayload payload);
    void waitUntilFinishIfBusy();

    void setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func);
    void setFinishInsFunc(std::function<void()> finish_ins_func);

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processSRAMSubmodule();
    [[noreturn]] void processPostProcessSubmodule();
    [[noreturn]] void processAdderTreeSubmodule1();
    [[noreturn]] void processAdderTreeSubmodule2();
    [[noreturn]] void processShiftAdderSubmodule();
    [[noreturn]] void processResultAdderSubmodule();

private:
    const CimUnitConfig& config_;

    // socket from MacroGroup
    SubmoduleSocket<MacroGroupControllerPayload> controller_socket_;

    // sockets in MacroGroupController
    sc_core::sc_event cur_sub_ins_next_batch_;
    MacroGroupSubmoduleSocket sram_socket_{};
    MacroGroupSubmoduleSocket post_process_socket_{};
    MacroGroupSubmoduleSocket adder_tree_socket_1_{};
    MacroGroupSubmoduleSocket adder_tree_socket_2_{};
    MacroGroupSubmoduleSocket shift_adder_socket_{};
    MacroGroupSubmoduleSocket result_adder_socket_{};

    // sockets to MacroGroup
    sc_core::sc_event& next_sub_ins_;

    std::function<void(int ins_id)> release_resource_func_;
    std::function<void()> finish_ins_func_;
};

}  // namespace cimsim
