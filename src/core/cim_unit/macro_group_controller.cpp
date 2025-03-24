//
// Created by wyk on 2024/7/24.
//

#include "macro_group_controller.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

MacroGroupController::MacroGroupController(const sc_core::sc_module_name &name, const cimsim::CimUnitConfig &config,
                                           const cimsim::SimConfig &sim_config, cimsim::Core *core, cimsim::Clock *clk,
                                           sc_core::sc_event &next_sub_ins)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , sram_read_("sram_read", sim_config, core, clk, getName(), config_.sram.read_latency_cycle, 1, false)
    , post_process_("post_process", sim_config, core, clk, getName(),
                    config_.bit_sparse ? config_.bit_sparse_config.latency_cycle : 0, 1, false)
    , adder_tree_("adder_tree", sim_config, core, clk, getName(), config_.adder_tree.latency_cycle,
                  config_.adder_tree.pipeline_stage_cnt, false)
    , shift_adder_("shift_adder", sim_config, core, clk, getName(), config_.shift_adder.latency_cycle,
                   config_.shift_adder.pipeline_stage_cnt, false)
    , result_adder_("result_adder", sim_config, core, clk, getName(), config_.result_adder.latency_cycle,
                    config_.result_adder.pipeline_stage_cnt, true)
    , next_sub_ins_(next_sub_ins) {
    SC_THREAD(processIPUAndIssue)

    sram_read_.bindNextStageSocket(post_process_.getExecuteSocket(), false);
    post_process_.bindNextStageSocket(adder_tree_.getExecuteSocket(), false);
    adder_tree_.bindNextStageSocket(shift_adder_.getExecuteSocket(), false);
    shift_adder_.bindNextStageSocket(result_adder_.getExecuteSocket(), true);
}

void MacroGroupController::start(cimsim::MacroGroupControllerPayload payload) {
    controller_socket_.payload = payload;
    controller_socket_.start_exec.notify();
}

void MacroGroupController::waitUntilFinishIfBusy() {
    controller_socket_.waitUntilFinishIfBusy();
}

void MacroGroupController::setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func) {
    result_adder_.setReleaseResourceFunc(std::move(release_resource_func));
}

void MacroGroupController::setFinishInsFunc(std::function<void()> finish_ins_func) {
    result_adder_.setFinishInsFunc(std::move(finish_ins_func));
}

void MacroGroupController::processIPUAndIssue() {
    while (true) {
        controller_socket_.waitUntilStart();

        const auto &payload = controller_socket_.payload;
        const auto &cim_ins_info = payload.cim_ins_info;
        CORE_LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), cim_ins_info.ins_pc,
                             cim_ins_info.sub_ins_num));

        MacroGroupSubmodulePayload submodule_payload{
            .sub_ins_info = std::make_shared<MacroGroupSubInsInfo>(MacroGroupSubInsInfo{
                .cim_ins_info = cim_ins_info, .last_group = payload.last_group, .bit_sparse = payload.bit_sparse})};
        int batch_count = payload.input_bit_width;
        for (int batch = 0; batch < batch_count; batch++) {
            submodule_payload.batch_info = std::make_shared<MacroBatchInfo>(
                MacroBatchInfo{.batch_num = batch, .last_batch = (batch == batch_count - 1)});

            CORE_LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num,
                                 submodule_payload.batch_info->batch_num));
            double latency = config_.ipu.latency_cycle * period_ns_;
            wait(latency, SC_NS);
            waitAndStartNextStage(submodule_payload, *(sram_read_.getExecuteSocket()));
        }

        controller_socket_.finish();
        next_sub_ins_.notify();
    }
}

}  // namespace cimsim