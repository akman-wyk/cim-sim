//
// Created by wyk on 2024/7/24.
//

#include "macro_group_controller.h"

#include "fmt/format.h"
#include "util/log.h"

namespace pimsim {

MacroGroupController::MacroGroupController(const std::string &name, const pimsim::PimUnitConfig &config,
                                           const pimsim::SimConfig &sim_config, pimsim::Core *core, pimsim::Clock *clk,
                                           sc_core::sc_event &next_sub_ins,
                                           SubmoduleSocket<pimsim::MacroGroupSubmodulePayload> &result_adder_socket)
    : BaseModule(name.c_str(), sim_config, core, clk)
    , config_(config)
    , next_sub_ins_(next_sub_ins)
    , result_adder_socket_(result_adder_socket) {
    SC_THREAD(processIPUAndIssue)
    SC_THREAD(processSRAMSubmodule)
    SC_THREAD(processPostProcessSubmodule)
    SC_THREAD(processAdderTreeSubmodule1)
    SC_THREAD(processAdderTreeSubmodule2)
    SC_THREAD(processShiftAdderSubmodule);
}

void MacroGroupController::start(pimsim::MacroGroupControllerPayload payload) {
    controller_socket_.payload = payload;
    controller_socket_.start_exec.notify();
}

void MacroGroupController::waitUntilFinishIfBusy() {
    controller_socket_.waitUntilFinishIfBusy();
}

void MacroGroupController::waitAndStartNextSubmodule(
    const pimsim::MacroGroupSubmodulePayload &cur_payload,
    SubmoduleSocket<pimsim::MacroGroupSubmodulePayload> &next_submodule_socket) {
    next_submodule_socket.waitUntilFinishIfBusy();
    if (cur_payload.batch_info.first_batch) {
        next_submodule_socket.payload.sub_ins_info = cur_payload.sub_ins_info;
    }
    next_submodule_socket.payload.batch_info = cur_payload.batch_info;
    next_submodule_socket.start_exec.notify();
}

void MacroGroupController::processIPUAndIssue() {
    while (true) {
        controller_socket_.waitUntilStart();

        const auto &payload = controller_socket_.payload;
        const auto &pim_ins_info = payload.pim_ins_info;
        LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num));

        MacroGroupSubmodulePayload submodule_payload{.sub_ins_info = {.pim_ins_info = pim_ins_info,
                                                                      .last_group = payload.last_group,
                                                                      .bit_sparse = payload.bit_sparse}};
        int batch_count = payload.input_bit_width;
        for (int batch = 0; batch < batch_count; batch++) {
            submodule_payload.batch_info = {
                .batch_num = batch, .first_batch = (batch == 0), .last_batch = (batch == batch_count - 1)};
            LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                            pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, submodule_payload.batch_info.batch_num));

            double latency = config_.ipu.latency_cycle * period_ns_;
            wait(latency, SC_NS);

            waitAndStartNextSubmodule(submodule_payload, sram_socket_);
        }

        controller_socket_.finish();
        next_sub_ins_.notify();
    }
}

void MacroGroupController::processSRAMSubmodule() {
    while (true) {
        sram_socket_.waitUntilStart();

        const auto &payload = sram_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start sram read, ins pc: {}, sub ins num: {}, batch: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double latency = config_.sram.read_latency_cycle * period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, post_process_socket_);
        sram_socket_.finish();
    }
}

void MacroGroupController::processPostProcessSubmodule() {
    while (true) {
        post_process_socket_.waitUntilStart();

        const auto &payload = post_process_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        if (config_.bit_sparse && payload.sub_ins_info.bit_sparse) {
            LOG(fmt::format("{} start post process, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                            pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

            double latency = config_.bit_sparse_config.latency_cycle * period_ns_;
            wait(latency, SC_NS);
        }

        waitAndStartNextSubmodule(payload, adder_tree_socket_1_);
        post_process_socket_.finish();
    }
}

void MacroGroupController::processAdderTreeSubmodule1() {
    while (true) {
        adder_tree_socket_1_.waitUntilStart();

        const auto &payload = adder_tree_socket_1_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start adder tree stage 1, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                        pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double latency = period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, adder_tree_socket_2_);
        adder_tree_socket_1_.finish();
    }
}

void MacroGroupController::processAdderTreeSubmodule2() {
    while (true) {
        adder_tree_socket_2_.waitUntilStart();

        const auto &payload = adder_tree_socket_2_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start adder tree stage 2, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                        pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double latency = period_ns_;
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, shift_adder_socket_);
        adder_tree_socket_2_.finish();
    }
}

void MacroGroupController::processShiftAdderSubmodule() {
    while (true) {
        shift_adder_socket_.waitUntilStart();

        const auto &payload = shift_adder_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start shift adder, ins pc: {}, sub ins num: {}, batch: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double latency = config_.shift_adder.latency_cycle * period_ns_;
        wait(latency, SC_NS);

        if (payload.batch_info.last_batch) {
            result_adder_socket_.waitUntilFinishIfBusy();
            result_adder_socket_.payload = payload;
            result_adder_socket_.start_exec.notify();
        }

        shift_adder_socket_.finish();
    }
}

}  // namespace pimsim