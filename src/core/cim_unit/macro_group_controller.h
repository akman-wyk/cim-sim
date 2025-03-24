//
// Created by wyk on 2024/7/24.
//

#pragma once
#include <vector>

#include "macro_group_module.h"

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
    [[noreturn]] void processIPUAndIssue();

private:
    const CimUnitConfig& config_;

    // socket from MacroGroup
    SubmoduleSocket<MacroGroupControllerPayload> controller_socket_;

    // modules in MacroGroupController
    MacroGroupModule sram_read_;
    MacroGroupModule post_process_;
    MacroGroupModule adder_tree_;
    MacroGroupModule shift_adder_;
    MacroGroupModule result_adder_;

    // sockets to MacroGroup
    sc_core::sc_event& next_sub_ins_;
};

}  // namespace cimsim
