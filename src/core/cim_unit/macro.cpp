//
// Created by wyk on 2024/7/20.
//

#include "macro.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

Macro::Macro(const sc_module_name &name, const cimsim::CimUnitConfig &config, const BaseInfo &base_info,
             bool independent_ipu, EnergyCounter &cim_unit_energy_counter, bool macro_simulation)
    : BaseModule(name, base_info)
    , config_(config)
    , macro_size_(config.macro_size)
    , independent_ipu_(independent_ipu)
    , activation_element_col_cnt_(config.macro_size.element_cnt_per_compartment)
    , sram_read_("sram_read", base_info, config, getSRAMReadDynamicPower, config_.sram.read_latency_cycle, 1)
    , post_process_("post_proecess", base_info, config, getPostProcessDynamicPower,
                    config_.bit_sparse ? config_.bit_sparse_config.latency_cycle : 0, 1)
    , adder_tree_("adder_tree", base_info, config, config.adder_tree, getAdderTreeDynamicPower)
    , shift_adder_("shift_adder", base_info, config, config.shift_adder, getShiftAdderDynamicPower)
    , result_adder_("result_adder", base_info, config, config.result_adder, getResultAdderDynamicPower) {
    SC_THREAD(processIPUAndIssue)

    // set static energy power
    int simulation_macro_group_cnt = macro_simulation ? config_.macro_total_cnt / config_.macro_group_size : 1;
    int simulation_macro_cnt = macro_simulation ? config_.macro_total_cnt : 1;

    const int ipu_cnt = 1 * simulation_macro_group_cnt;
    const int sram_cnt = 1 * simulation_macro_cnt;
    const int post_process_cnt =
        (config_.bit_sparse ? macro_size_.row_cnt_per_element * 1 * macro_size_.element_cnt_per_compartment *
                                  macro_size_.compartment_cnt_per_macro
                            : 0) *
        simulation_macro_cnt;
    const int adder_tree_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;
    const int shift_adder_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;
    const int result_adder_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;

    ipu_energy_counter_.setStaticPowerMW(config_.ipu.static_power_mW * ipu_cnt);
    sram_read_.setStaticPower(config_.sram.static_power_mW * sram_cnt);
    post_process_.setStaticPower(config_.bit_sparse_config.static_power_mW * post_process_cnt);
    adder_tree_.setStaticPower(config_.adder_tree.static_power_mW * adder_tree_cnt);
    shift_adder_.setStaticPower(config_.shift_adder.static_power_mW * shift_adder_cnt);
    result_adder_.setStaticPower(config_.result_adder.static_power_mW * result_adder_cnt);

    if (independent_ipu_) {
        cim_unit_energy_counter.addSubEnergyCounter("ipu", &ipu_energy_counter_);
    }
    if (config_.bit_sparse) {
        cim_unit_energy_counter.addSubEnergyCounter("meta buffer", &meta_buffer_energy_counter_);
    }
    cim_unit_energy_counter.addSubEnergyCounter("sram read", sram_read_.getEnergyCounterPtr());
    cim_unit_energy_counter.addSubEnergyCounter("post process", post_process_.getEnergyCounterPtr());
    cim_unit_energy_counter.addSubEnergyCounter("adder tree", adder_tree_.getEnergyCounterPtr());
    cim_unit_energy_counter.addSubEnergyCounter("shift adder", shift_adder_.getEnergyCounterPtr());
    cim_unit_energy_counter.addSubEnergyCounter("result adder", result_adder_.getEnergyCounterPtr());

    // bind next stage socket
    sram_read_.bindNextStageSocket(post_process_.getExecuteSocket(), false);
    post_process_.bindNextStageSocket(adder_tree_.getExecuteSocket(), false);
    adder_tree_.bindNextStageSocket(shift_adder_.getExecuteSocket(), false);
    shift_adder_.bindNextStageSocket(result_adder_.getExecuteSocket(), true);
}

void Macro::startExecute(cimsim::MacroPayload payload) {
    macro_socket_.payload = std::move(payload);
    macro_socket_.start_exec.notify();
}

void Macro::waitUntilFinishIfBusy() {
    macro_socket_.waitUntilFinishIfBusy();
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

void Macro::bindNextModuleSocket(MacroStageSocket *next_module_socket) {
    result_adder_.bindNextStageSocket(next_module_socket, false);
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

        if (config_.bit_sparse && payload.bit_sparse && batch_cnt > 0) {
            int meta_size_byte = config_.bit_sparse_config.mask_bit_width * macro_size_.element_cnt_per_compartment *
                                 macro_size_.compartment_cnt_per_macro / BYTE_TO_BIT;
            double meta_read_dynamic_power_mW = config_.bit_sparse_config.reg_buffer_dynamic_power_mW_per_unit *
                                                IntDivCeil(meta_size_byte, config_.bit_sparse_config.unit_byte);
            meta_buffer_energy_counter_.addDynamicEnergyPJ(
                period_ns_, meta_read_dynamic_power_mW * submodule_payload.sub_ins_info->simulated_macro_cnt,
                {.core_id = core_id_,
                 .ins_id = payload.cim_ins_info.ins_id,
                 .inst_opcode = payload.cim_ins_info.inst_opcode,
                 .inst_group_tag = payload.cim_ins_info.inst_group_tag,
                 .inst_profiler_operator = InstProfilerOperator::memory});
        }

        for (int batch = 0; batch < batch_cnt; batch++) {
            submodule_payload.batch_info = std::make_shared<MacroBatchInfo>(
                MacroBatchInfo{.batch_num = batch, .last_batch = (batch == batch_cnt - 1)});

            CORE_LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num,
                                 submodule_payload.batch_info->batch_num));
            double dynamic_power_mW = config_.ipu.dynamic_power_mW;
            double latency = config_.ipu.latency_cycle * period_ns_;
            ipu_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW * sub_ins_info.simulated_group_cnt,
                                                   {.core_id = core_id_,
                                                    .ins_id = payload.cim_ins_info.ins_id,
                                                    .inst_opcode = payload.cim_ins_info.inst_opcode,
                                                    .inst_group_tag = payload.cim_ins_info.inst_group_tag,
                                                    .inst_profiler_operator = InstProfilerOperator::computation});
            wait(latency, SC_NS);

            waitAndStartNextStage(submodule_payload, *(sram_read_.getExecuteSocket()));
        }

        macro_socket_.finish();
    }
}

double Macro::getSRAMReadDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.sram.read_dynamic_power_per_bit_mW * config.macro_size.bit_width_per_row * 1 *
                              config.macro_size.element_cnt_per_compartment *
                              config.macro_size.compartment_cnt_per_macro;
    return dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt;
}

double Macro::getPostProcessDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.bit_sparse_config.dynamic_power_mW *
                              payload.sub_ins_info->activation_element_col_cnt * payload.sub_ins_info->compartment_num;
    return config.bit_sparse && payload.sub_ins_info->bit_sparse
               ? dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt
               : 0;
}

double Macro::getAdderTreeDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.adder_tree.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
    return dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt;
}

double Macro::getShiftAdderDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.shift_adder.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
    return dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt;
}

double Macro::getResultAdderDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.result_adder.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt;
    return dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt;
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
