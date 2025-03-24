//
// Created by wyk on 2024/7/20.
//

#include "macro.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

Macro::Macro(const sc_core::sc_module_name &name, const cimsim::CimUnitConfig &config,
             const cimsim::SimConfig &sim_config, cimsim::Core *core, cimsim::Clock *clk, bool independent_ipu)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , independent_ipu_(independent_ipu)
    , activation_element_col_cnt_(config.macro_size.element_cnt_per_compartment) {
    SC_THREAD(processIPUAndIssue)
    SC_THREAD(processSRAMSubmodule)
    SC_THREAD(processPostProcessSubmodule)
    SC_THREAD(processAdderTreeSubmodule1)
    SC_THREAD(processAdderTreeSubmodule2)
    SC_THREAD(processShiftAdderSubmodule)
    SC_THREAD(processResultAdderSubmodule)

    constexpr int ipu_cnt = 1;
    constexpr int sram_cnt = 1;
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

void Macro::startExecute(cimsim::MacroPayload payload) {
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

void Macro::setFinishInsFunction(std::function<void()> finish_func) {
    finish_ins_func_ = std::move(finish_func);
}

void Macro::setActivationElementColumn(const std::vector<unsigned char> &macros_activation_element_col_mask,
                                       int start_index) {
    activation_element_col_cnt_ = 0;
    for (int i = 0; i < macro_size_.element_cnt_per_compartment; i++) {
        if (getMaskBit(macros_activation_element_col_mask, i + start_index) != 0) {
            activation_element_col_cnt_++;
        }
    }
}

int Macro::getActivationElementColumnCount() const {
    return activation_element_col_cnt_;
}

void Macro::processIPUAndIssue() {
    while (true) {
        macro_socket_.waitUntilStart();

        if (activation_element_col_cnt_ == 0) {
            macro_socket_.finish();
            continue;
        }

        const auto &payload = macro_socket_.payload;
        const auto &cim_ins_info = payload.cim_ins_info;
        CORE_LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), cim_ins_info.ins_pc,
                             cim_ins_info.sub_ins_num));

        auto [batch_cnt, activation_compartment_num] = getBatchCountAndActivationCompartmentCount(payload);
        MacroSubInsInfo sub_ins_info{.cim_ins_info = cim_ins_info,
                                     .compartment_num = activation_compartment_num,
                                     .bit_sparse = payload.bit_sparse,
                                     .activation_element_col_cnt = activation_element_col_cnt_,
                                     .simulated_group_cnt = payload.simulated_group_cnt,
                                     .simulated_macro_cnt = payload.simulated_macro_cnt};
        MacroSubmodulePayload submodule_payload{.sub_ins_info = std::make_shared<MacroSubInsInfo>(sub_ins_info)};

        if (batch_cnt > 0) {
            int meta_size_byte = config_.bit_sparse_config.mask_bit_width * macro_size_.element_cnt_per_compartment *
                                 macro_size_.compartment_cnt_per_macro / BYTE_TO_BIT;
            double meta_read_dynamic_power_mW = config_.bit_sparse_config.reg_buffer_dynamic_power_mW_per_unit *
                                                IntDivCeil(meta_size_byte, config_.bit_sparse_config.unit_byte);
            meta_buffer_energy_counter_.addDynamicEnergyPJ(
                period_ns_, meta_read_dynamic_power_mW * submodule_payload.sub_ins_info->simulated_macro_cnt);
        }

        for (int batch = 0; batch < batch_cnt; batch++) {
            submodule_payload.batch_info = std::make_shared<MacroBatchInfo>(
                MacroBatchInfo{.batch_num = batch, .last_batch = (batch == batch_cnt - 1)});

            CORE_LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num,
                                 submodule_payload.batch_info->batch_num));
            double dynamic_power_mW = config_.ipu.dynamic_power_mW;
            double latency = config_.ipu.latency_cycle * period_ns_;
            ipu_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW * sub_ins_info.simulated_group_cnt);
            wait(latency, SC_NS);

            waitAndStartNextStage(submodule_payload, sram_socket_);
        }

        macro_socket_.finish();
    }
}

void Macro::processSRAMSubmodule() {
    while (true) {
        sram_socket_.waitUntilStart();

        const auto &payload = sram_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start sram read, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW = config_.sram.read_dynamic_power_per_bit_mW * macro_size_.bit_width_per_row * 1 *
                                  macro_size_.element_cnt_per_compartment * macro_size_.compartment_cnt_per_macro;
        double latency = config_.sram.read_latency_cycle * period_ns_;
        sram_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, post_process_socket_);

        sram_socket_.finish();
    }
}

void Macro::processPostProcessSubmodule() {
    while (true) {
        post_process_socket_.waitUntilStart();

        const auto &payload = post_process_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        if (config_.bit_sparse && payload.sub_ins_info->bit_sparse) {
            CORE_LOG(fmt::format("{} start post process, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

            double dynamic_power_mW = config_.bit_sparse_config.dynamic_power_mW *
                                      payload.sub_ins_info->activation_element_col_cnt *
                                      payload.sub_ins_info->compartment_num;
            double latency = config_.bit_sparse_config.latency_cycle * period_ns_;
            post_process_energy_counter_.addDynamicEnergyPJ(
                latency == 0.0 ? period_ns_ : latency, dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);
            wait(latency, SC_NS);
        }

        waitAndStartNextStage(payload, adder_tree_socket_1_);

        post_process_socket_.finish();
    }
}

void Macro::processAdderTreeSubmodule1() {
    while (true) {
        adder_tree_socket_1_.waitUntilStart();

        const auto &payload = adder_tree_socket_1_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start adder tree stage 1, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW =
            config_.adder_tree.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
        double latency = period_ns_;
        adder_tree_energy_counter_.addDynamicEnergyPJ(latency,
                                                      dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, adder_tree_socket_2_);

        adder_tree_socket_1_.finish();
    }
}

void Macro::processAdderTreeSubmodule2() {
    while (true) {
        adder_tree_socket_2_.waitUntilStart();

        const auto &payload = adder_tree_socket_2_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start adder tree stage 2, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW =
            config_.adder_tree.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
        double latency = period_ns_;
        adder_tree_energy_counter_.addDynamicEnergyPJ(latency,
                                                      dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, shift_adder_socket_);

        adder_tree_socket_2_.finish();
    }
}

void Macro::processShiftAdderSubmodule() {
    while (true) {
        shift_adder_socket_.waitUntilStart();

        const auto &payload = shift_adder_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start shift adder, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW =
            config_.shift_adder.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
        double latency = config_.shift_adder.latency_cycle * period_ns_;
        shift_adder_energy_counter_.addDynamicEnergyPJ(latency,
                                                       dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);
        wait(latency, SC_NS);

        if (payload.batch_info->last_batch) {
            waitAndStartNextStage(payload, result_adder_socket_);
        }

        shift_adder_socket_.finish();
    }
}

void Macro::processResultAdderSubmodule() {
    while (true) {
        result_adder_socket_.waitUntilStart();

        const auto &payload = result_adder_socket_.payload;
        const auto &cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} start result adder, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                             cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double dynamic_power_mW =
            config_.result_adder.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
        double latency = config_.result_adder.latency_cycle * period_ns_;
        result_adder_energy_counter_.addDynamicEnergyPJ(latency,
                                                        dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt);

        if (finish_ins_func_ && cim_ins_info.last_sub_ins && payload.batch_info->last_batch) {
            finish_ins_func_();
        }

        result_adder_socket_.finish();
    }
}

std::pair<int, int> Macro::getBatchCountAndActivationCompartmentCount(const MacroPayload &payload) const {
    int valid_input_cnt = std::min(macro_size_.compartment_cnt_per_macro, static_cast<int>(payload.inputs.size()));
    int activation_compartment_num = static_cast<int>(std::count_if(
        payload.inputs.begin(), payload.inputs.begin() + valid_input_cnt, [](auto input) { return input != 0; }));
    int batch_num;
    if (data_mode_ == +DataMode::real_data) {
        if (config_.input_bit_sparse && payload.bit_sparse) {
            batch_num = 0;
            for (int i = 0; i < payload.input_bit_width; i++) {
                if (std::any_of(payload.inputs.begin(), payload.inputs.begin() + valid_input_cnt,
                                [i](auto input) { return (input & (1 << i)) != 0; })) {
                    batch_num++;
                }
            }
        } else {
            batch_num = activation_compartment_num == 0 ? 0 : payload.input_bit_width;
        }
    } else {
        batch_num = payload.input_bit_width;
    }

    return {batch_num, activation_compartment_num};
}

}  // namespace cimsim
