//
// Created by wyk on 2024/7/20.
//

#include "macro.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

#define LOG(msg)

Macro::Macro(const char *name, const pimsim::PimUnitConfig &config, const pimsim::SimConfig &sim_config,
             pimsim::Core *core, pimsim::Clock *clk, bool independent_ipu,
             SubmoduleSocket<MacroGroupSubmodulePayload> *result_adder_socket_ptr)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , independent_ipu_(independent_ipu)
    , activation_element_col_cnt_(config.macro_size.element_cnt_per_compartment)
    , result_adder_socket_ptr_(result_adder_socket_ptr) {
    for (int i = 0; i < IntDivCeil(macro_size_.element_cnt_per_compartment, BYTE_TO_BIT); i++) {
        activation_element_col_mask_.push_back(BYTE_MAX_VALUE);
    }

    SC_THREAD(processIPUAndIssue)
    SC_THREAD(processSRAMSubmodule)
    SC_THREAD(processPostProcessSubmodule)
    SC_THREAD(processAdderTreeSubmodule1)
    SC_THREAD(processAdderTreeSubmodule2)
    SC_THREAD(processShiftAdderSubmodule)

    const int ipu_cnt = 1;
    const int sram_cnt = 1;
    const int post_process_cnt = config_.bit_sparse
                                     ? macro_size_.row_cnt_per_element * 1 * macro_size_.element_cnt_per_compartment *
                                           macro_size_.compartment_cnt_per_macro
                                     : 0;
    const int adder_tree_cnt = macro_size_.element_cnt_per_compartment;
    const int shift_adder_cnt = macro_size_.element_cnt_per_compartment;
    const int result_adder_cnt = macro_size_.element_cnt_per_compartment;

    ipu_energy_counter_.setStaticPowerMW(config_.ipu.static_power_mW * ipu_cnt);
    sram_energy_counter_.setStaticPowerMW(config_.sram.static_power_mW * sram_cnt);
    post_process_energy_counter_.setStaticPowerMW(config_.bit_sparse_config.static_power_mW * post_process_cnt);
    adder_tree_energy_counter_.setStaticPowerMW(config_.adder_tree.static_power_mW * adder_tree_cnt);
    shift_adder_energy_counter_.setStaticPowerMW(config_.shift_adder.static_power_mW * shift_adder_cnt);
    result_adder_energy_counter_.setStaticPowerMW(config_.result_adder.static_power_mW * result_adder_cnt);
}

void Macro::startExecute(pimsim::MacroPayload payload) {
    macro_socket_.payload = std::move(payload);
    macro_socket_.start_exec.notify();
}

void Macro::waitUntilFinishIfBusy() {
    macro_socket_.waitUntilFinishIfBusy();
}

EnergyReporter Macro::getEnergyReporter() {
    EnergyReporter macro_reporter;
    if (independent_ipu_) {
        macro_reporter.addSubModule("ipu", EnergyReporter{ipu_energy_counter_});
    }
    if (config_.bit_sparse) {
        macro_reporter.addSubModule("meta buffer", EnergyReporter{meta_buffer_energy_counter_});
    }
    macro_reporter.addSubModule("sram read", EnergyReporter{sram_energy_counter_});
    macro_reporter.addSubModule("post process", EnergyReporter{post_process_energy_counter_});
    macro_reporter.addSubModule("adder tree", EnergyReporter{adder_tree_energy_counter_});
    macro_reporter.addSubModule("shift adder", EnergyReporter{shift_adder_energy_counter_});
    macro_reporter.addSubModule("result adder", EnergyReporter{result_adder_energy_counter_});
    return std::move(macro_reporter);
}

void Macro::waitAndStartNextSubmodule(const pimsim::MacroSubmodulePayload &cur_payload,
                                      SubmoduleSocket<pimsim::MacroSubmodulePayload> &next_submodule_socket) {
    next_submodule_socket.waitUntilFinishIfBusy();
    if (cur_payload.batch_info.first_batch) {
        next_submodule_socket.payload.sub_ins_info = cur_payload.sub_ins_info;
    }
    next_submodule_socket.payload.batch_info = cur_payload.batch_info;
    next_submodule_socket.start_exec.notify();
}

void Macro::setFinishRunFunction(std::function<void()> finish_func) {
    finish_run_func_ = std::move(finish_func);
}

void Macro::setActivationElementColumn(const std::vector<unsigned char> &macros_activation_element_col_mask,
                                       int start_index) {
    activation_element_col_cnt_ = 0;
    for (int i = 0; i < macro_size_.element_cnt_per_compartment; i++) {
        if (getMaskBit(macros_activation_element_col_mask, i + start_index) != 0) {
            activation_element_col_cnt_++;
            activation_element_col_mask_[i / BYTE_TO_BIT] |= (1 << (i % BYTE_TO_BIT));
        } else {
            activation_element_col_mask_[i / BYTE_TO_BIT] &= (~(1 << (i % BYTE_TO_BIT)));
        }
    }
}

int Macro::getActivationElementColumnCount() const {
    return activation_element_col_cnt_;
}

void Macro::processIPUAndIssue() {
    while (true) {
        macro_socket_.waitUntilStart();

        if (activation_element_col_cnt_ > 0) {
            const auto &payload = macro_socket_.payload;
            const auto &pim_ins_info = payload.pim_ins_info;
            LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), pim_ins_info.ins_pc,
                            pim_ins_info.sub_ins_num));

            auto [batch_cnt, activation_compartment_num] = getBatchCountAndActivationCompartmentCount(payload);
            MacroSubInsInfo sub_ins_info{.pim_ins_info = pim_ins_info,
                                         .compartment_num = activation_compartment_num,
                                         .bit_sparse = payload.bit_sparse,
                                         .activation_element_col_cnt = activation_element_col_cnt_,
                                         .activation_element_col_mask = activation_element_col_mask_,
                                         .total_activation_group_cnt = payload.total_activation_group_cnt,
                                         .total_activation_macro_cnt = payload.total_activation_macro_cnt};
            MacroSubmodulePayload submodule_payload{.sub_ins_info = sub_ins_info};

            for (int batch = 0; batch < batch_cnt; batch++) {
                submodule_payload.batch_info = {
                    .batch_num = batch, .first_batch = (batch == 0), .last_batch = (batch == batch_cnt - 1)};
                LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, submodule_payload.batch_info.batch_num));

                double dynamic_power_mW = config_.ipu.dynamic_power_mW;
                double latency = config_.ipu.latency_cycle * period_ns_;
                ipu_energy_counter_.addDynamicEnergyPJ(latency,
                                                       dynamic_power_mW * sub_ins_info.total_activation_group_cnt);
                wait(latency, SC_NS);

                waitAndStartNextSubmodule(submodule_payload, sram_socket_);
            }
        }

        macro_socket_.finish();
    }
}

void Macro::processSRAMSubmodule() {
    while (true) {
        sram_socket_.waitUntilStart();

        const auto &payload = sram_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start sram read, ins pc: {}, sub ins num: {}, batch: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double dynamic_power_mW = config_.sram.read_dynamic_power_per_bit_mW * macro_size_.bit_width_per_row * 1 *
                                  macro_size_.element_cnt_per_compartment * macro_size_.compartment_cnt_per_macro;
        double latency = config_.sram.read_latency_cycle * period_ns_;
        sram_energy_counter_.addDynamicEnergyPJ(latency,
                                                dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt);
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, post_process_socket_);

        sram_socket_.finish();
    }
}

void Macro::processPostProcessSubmodule() {
    while (true) {
        post_process_socket_.waitUntilStart();

        const auto &payload = post_process_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        if (config_.bit_sparse && payload.sub_ins_info.bit_sparse) {
            LOG(fmt::format("{} start post process, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                            pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

            if (payload.batch_info.first_batch) {
                int meta_size_byte = config_.bit_sparse_config.mask_bit_width *
                                     macro_size_.element_cnt_per_compartment * macro_size_.compartment_cnt_per_macro /
                                     BYTE_TO_BIT;
                double meta_read_dynamic_power_mW = config_.bit_sparse_config.reg_buffer_dynamic_power_mW_per_unit *
                                                    IntDivCeil(meta_size_byte, config_.bit_sparse_config.unit_byte);
                meta_buffer_energy_counter_.addDynamicEnergyPJ(
                    period_ns_, meta_read_dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt);
            }

            double dynamic_power_mW = config_.bit_sparse_config.dynamic_power_mW *
                                      payload.sub_ins_info.activation_element_col_cnt *
                                      payload.sub_ins_info.compartment_num;
            double latency = config_.bit_sparse_config.latency_cycle * period_ns_;
            post_process_energy_counter_.addDynamicEnergyPJ(
                latency == 0.0 ? period_ns_ : latency,
                dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt);
            wait(latency, SC_NS);
        }

        waitAndStartNextSubmodule(payload, adder_tree_socket_1_);

        post_process_socket_.finish();
    }
}

void Macro::processAdderTreeSubmodule1() {
    while (true) {
        adder_tree_socket_1_.waitUntilStart();

        const auto &payload = adder_tree_socket_1_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start adder tree stage 1, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                        pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double dynamic_power_mW = config_.adder_tree.dynamic_power_mW;
        double latency = period_ns_;
        for (int i = 0; i < macro_size_.element_cnt_per_compartment; i++) {
            if (getMaskBit(payload.sub_ins_info.activation_element_col_mask, i) != 0) {
                adder_tree_energy_counter_.addDynamicEnergyPJ(
                    latency, dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt,
                    sc_core::sc_time_stamp(), i);
            }
        }
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, adder_tree_socket_2_);

        adder_tree_socket_1_.finish();
    }
}

void Macro::processAdderTreeSubmodule2() {
    while (true) {
        adder_tree_socket_2_.waitUntilStart();

        const auto &payload = adder_tree_socket_2_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start adder tree stage 2, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                        pim_ins_info.ins_pc, pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double dynamic_power_mW = config_.adder_tree.dynamic_power_mW;
        double latency = period_ns_;
        for (int i = 0; i < macro_size_.element_cnt_per_compartment; i++) {
            if (getMaskBit(payload.sub_ins_info.activation_element_col_mask, i) != 0) {
                adder_tree_energy_counter_.addDynamicEnergyPJ(
                    latency, dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt,
                    sc_core::sc_time_stamp(), i);
            }
        }
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, shift_adder_socket_);

        adder_tree_socket_2_.finish();
    }
}

void Macro::processShiftAdderSubmodule() {
    while (true) {
        shift_adder_socket_.waitUntilStart();

        const auto &payload = shift_adder_socket_.payload;
        const auto &pim_ins_info = payload.sub_ins_info.pim_ins_info;
        LOG(fmt::format("{} start shift adder, ins pc: {}, sub ins num: {}, batch: {}", getName(), pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num, payload.batch_info.batch_num));

        double dynamic_power_mW =
            config_.shift_adder.dynamic_power_mW * payload.sub_ins_info.activation_element_col_cnt;
        double latency = config_.shift_adder.latency_cycle * period_ns_;
        shift_adder_energy_counter_.addDynamicEnergyPJ(
            latency, dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt);
        wait(latency, SC_NS);

        if (payload.batch_info.last_batch) {
            if (result_adder_socket_ptr_ != nullptr) {
                result_adder_socket_ptr_->waitUntilFinishIfBusy();
            }
            dynamic_power_mW = config_.result_adder.dynamic_power_mW * payload.sub_ins_info.activation_element_col_cnt;
            latency = config_.result_adder.latency_cycle * period_ns_;
            result_adder_energy_counter_.addDynamicEnergyPJ(
                latency, dynamic_power_mW * payload.sub_ins_info.total_activation_macro_cnt);
        }

        if (pim_ins_info.last_ins && pim_ins_info.last_sub_ins && payload.batch_info.last_batch && finish_run_func_) {
            finish_run_func_();
        }

        shift_adder_socket_.finish();
    }
}

std::pair<int, int> Macro::getBatchCountAndActivationCompartmentCount(const MacroPayload &payload) {
    int valid_input_cnt = std::min(macro_size_.compartment_cnt_per_macro, static_cast<int>(payload.inputs.size()));
    int activation_compartment_num = static_cast<int>(std::count_if(
        payload.inputs.begin(), payload.inputs.begin() + valid_input_cnt, [](auto input) { return input != 0; }));
    int batch_num;
    if (config_.input_bit_sparse && payload.bit_sparse) {
        batch_num = 0;
        for (int i = 0; i < payload.input_bit_width; i++) {
            if (std::any_of(payload.inputs.begin(), payload.inputs.begin() + valid_input_cnt,
                            [i](auto input) { return (input & (1 << i)) != 0; })) {
                batch_num++;
            }
        }
    } else {
        batch_num = payload.input_bit_width;
    }

    return {batch_num, activation_compartment_num};
}

#undef LOG

}  // namespace pimsim
