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
    : BaseModule(name, sim_config, core, clk), config_(config), next_sub_ins_(next_sub_ins) {
    SC_THREAD(processIssue)
    SC_THREAD(processSRAMSubmodule)
    SC_THREAD(processPostProcessSubmodule)
    SC_THREAD(processAdderTreeSubmodule1)
    SC_THREAD(processAdderTreeSubmodule2)
    SC_THREAD(processShiftAdderSubmodule)
    SC_THREAD(processResultAdderSubmodule)
}

void MacroGroupController::start(cimsim::MacroGroupControllerPayload payload) {
    controller_socket_.payload = payload;
    controller_socket_.start_exec.notify();
}

void MacroGroupController::waitUntilFinishIfBusy() {
    controller_socket_.waitUntilFinishIfBusy();
}

void MacroGroupController::setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func) {
    release_resource_func_ = std::move(release_resource_func);
}

void MacroGroupController::setFinishInsFunc(std::function<void()> finish_ins_func) {
    finish_ins_func_ = std::move(finish_ins_func);
}

void MacroGroupController::processIssue() {
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
            waitAndStartNextStage(submodule_payload, sram_socket_);
        }

        controller_socket_.finish();
        next_sub_ins_.notify();
    }
}

void MacroGroupController::processSRAMSubmodule() {
    while (true) {
        sram_socket_.waitUntilStart();

        const auto &payload = sram_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start sram read, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double latency = config_.sram.read_latency_cycle * period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, post_process_socket_);
        sram_socket_.finish();
    }
}

void MacroGroupController::processPostProcessSubmodule() {
    while (true) {
        post_process_socket_.waitUntilStart();

        const auto &payload = post_process_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        if (config_.bit_sparse && payload.sub_ins_info->bit_sparse) {
            CORE_LOG(fmt::format("{} start post process, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

            double latency = config_.bit_sparse_config.latency_cycle * period_ns_;
            wait(latency, SC_NS);
        }

        waitAndStartNextStage(payload, adder_tree_socket_1_);
        post_process_socket_.finish();
    }
}

void MacroGroupController::processAdderTreeSubmodule1() {
    while (true) {
        adder_tree_socket_1_.waitUntilStart();

        const auto &payload = adder_tree_socket_1_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start adder tree stage 1, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double latency = period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, adder_tree_socket_2_);
        adder_tree_socket_1_.finish();
    }
}

void MacroGroupController::processAdderTreeSubmodule2() {
    while (true) {
        adder_tree_socket_2_.waitUntilStart();

        const auto &payload = adder_tree_socket_2_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start adder tree stage 2, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double latency = period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, shift_adder_socket_);
        adder_tree_socket_2_.finish();
    }
}

void MacroGroupController::processShiftAdderSubmodule() {
    while (true) {
        shift_adder_socket_.waitUntilStart();

        const auto &payload = shift_adder_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start shift adder, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double latency = config_.shift_adder.latency_cycle * period_ns_;
        wait(latency, SC_NS);

        if (payload.batch_info->last_batch) {
            waitAndStartNextStage(payload, result_adder_socket_);
        }

        shift_adder_socket_.finish();
    }
}

void MacroGroupController::processResultAdderSubmodule() {
    while (true) {
        result_adder_socket_.waitUntilStart();

        const auto &payload = result_adder_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start result adder, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        if (payload.sub_ins_info->last_group && cim_ins_info.last_sub_ins && release_resource_func_) {
            release_resource_func_(cim_ins_info.ins_id);
        }

        double latency = config_.result_adder.latency_cycle * period_ns_;
        wait(latency, SC_NS);

        if (finish_ins_func_ && payload.sub_ins_info->last_group && cim_ins_info.last_sub_ins) {
            finish_ins_func_();
        }

        CORE_LOG(fmt::format("{} end result adder, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));
        result_adder_socket_.finish();
    }
}

}  // namespace cimsim