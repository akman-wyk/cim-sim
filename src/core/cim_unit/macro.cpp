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
    , activation_element_col_cnt_(config.macro_size.element_cnt_per_compartment) {
    SC_THREAD(processIPUAndIssue)

    int simulation_macro_group_cnt = macro_simulation ? config_.macro_total_cnt / config_.macro_group_size : 1;
    int simulation_macro_cnt = macro_simulation ? config_.macro_total_cnt : 1;

    // ipu
    if (config_.bit_serial) {
        const int ipu_cnt = 1 * simulation_macro_group_cnt;
        ipu_energy_counter_.setStaticPowerMW(config_.ipu.static_power_mW * ipu_cnt);
        if (independent_ipu_) {
            cim_unit_energy_counter.addSubEnergyCounter("ipu", &ipu_energy_counter_);
        }
    }

    // sram read
    sram_read_ = std::make_shared<MacroModule>("sram_read", base_info, config, getSRAMReadDynamicPower,
                                               config_.sram.read_latency_cycle, 1);
    const int sram_cnt = 1 * simulation_macro_cnt;
    sram_read_->setStaticPower(config_.sram.static_power_mW * sram_cnt);
    cim_unit_energy_counter.addSubEnergyCounter("sram read", sram_read_->getEnergyCounterPtr());

    // bit sparse post process
    if (config_.bit_sparse) {
        post_process_ = std::make_shared<MacroModule>("post_proecess", base_info, config, getPostProcessDynamicPower,
                                                      config_.bit_sparse_config.latency_cycle, 1);
        const int post_process_cnt = macro_size_.row_cnt_per_element * 1 * macro_size_.element_cnt_per_compartment *
                                     macro_size_.compartment_cnt_per_macro * simulation_macro_cnt;
        post_process_->setStaticPower(config_.bit_sparse_config.static_power_mW * post_process_cnt);
        cim_unit_energy_counter.addSubEnergyCounter("meta buffer", &meta_buffer_energy_counter_);
        cim_unit_energy_counter.addSubEnergyCounter("post process", post_process_->getEnergyCounterPtr());
    }

    // mult
    if (config_.independent_mult) {
        mult_ = std::make_shared<MacroModule>("mult", base_info, config, config.mult, getMultDynamicPower);
        const int mult_cnt =
            macro_size_.element_cnt_per_compartment * macro_size_.compartment_cnt_per_macro * simulation_macro_cnt;
        mult_->setStaticPower(config_.mult.static_power_mW * mult_cnt);
        cim_unit_energy_counter.addSubEnergyCounter("mult", mult_->getEnergyCounterPtr());
    }

    // adder tree
    adder_tree_ =
        std::make_shared<MacroModule>("adder_tree", base_info, config, config.adder_tree, getAdderTreeDynamicPower);
    const int adder_tree_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;
    adder_tree_->setStaticPower(config_.adder_tree.static_power_mW * adder_tree_cnt);
    cim_unit_energy_counter.addSubEnergyCounter("adder tree", adder_tree_->getEnergyCounterPtr());

    // shift adder
    if (config_.bit_serial) {
        shift_adder_ = std::make_shared<MacroModule>("shift_adder", base_info, config, config.shift_adder,
                                                     getShiftAdderDynamicPower);
        const int shift_adder_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;
        shift_adder_->setStaticPower(config_.shift_adder.static_power_mW * shift_adder_cnt);
        cim_unit_energy_counter.addSubEnergyCounter("shift adder", shift_adder_->getEnergyCounterPtr());
    }

    // result adder
    result_adder_ = std::make_shared<MacroModule>("result_adder", base_info, config, config.result_adder,
                                                  getResultAdderDynamicPower);
    const int result_adder_cnt = macro_size_.element_cnt_per_compartment * simulation_macro_cnt;
    result_adder_->setStaticPower(config_.result_adder.static_power_mW * result_adder_cnt);
    cim_unit_energy_counter.addSubEnergyCounter("result adder", result_adder_->getEnergyCounterPtr());

    // bind next stage socket
    std::vector modules{sram_read_, post_process_, mult_, adder_tree_, shift_adder_};
    modules.erase(std::remove_if(modules.begin(), modules.end(), [](const auto &ptr) { return ptr == nullptr; }),
                  modules.end());
    for (int i = 0; i < modules.size(); i++) {
        if (i < modules.size() - 1) {
            modules[i]->bindNextStageSocket(modules[i + 1]->getExecuteSocket(), false);
        } else {
            modules[i]->bindNextStageSocket(result_adder_->getExecuteSocket(), true);
        }
    }
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
    result_adder_->bindNextStageSocket(next_module_socket, false);
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
                 .inst_profiler_operator = "meta_buffer_read"});
        }

        for (int batch = 0; batch < batch_cnt; batch++) {
            submodule_payload.batch_info = std::make_shared<MacroBatchInfo>(
                MacroBatchInfo{.batch_num = batch, .last_batch = (batch == batch_cnt - 1)});

            CORE_LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num,
                                 submodule_payload.batch_info->batch_num));
            double dynamic_power_mW = config_.bit_serial ? config_.ipu.dynamic_power_mW : 0;
            double latency = config_.bit_serial ? config_.ipu.latency_cycle * period_ns_ : 0;
            ipu_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW * sub_ins_info.simulated_group_cnt,
                                                   {.core_id = core_id_,
                                                    .ins_id = payload.cim_ins_info.ins_id,
                                                    .inst_opcode = payload.cim_ins_info.inst_opcode,
                                                    .inst_group_tag = payload.cim_ins_info.inst_group_tag,
                                                    .inst_profiler_operator = "ipu"});
            wait(latency, SC_NS);

            waitAndStartNextStage(submodule_payload, *(sram_read_->getExecuteSocket()));
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

double Macro::getMultDynamicPower(const CimUnitConfig &config, const MacroSubmodulePayload &payload) {
    double dynamic_power_mW = config.mult.dynamic_power_mW * payload.sub_ins_info->activation_element_col_cnt *
                              payload.sub_ins_info->compartment_num;
    return dynamic_power_mW * payload.sub_ins_info->simulated_macro_cnt;
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
    int activation_compartment_num, batch_num;
    if (data_mode_ == +DataMode::real_data) {
        activation_compartment_num = static_cast<int>(std::count_if(
            payload.inputs.begin(), payload.inputs.begin() + valid_input_cnt, [](auto input) { return input != 0; }));
        if (!config_.bit_serial) {
            batch_num = activation_compartment_num == 0 ? 0 : 1;
        } else if (config_.input_bit_sparse && payload.bit_sparse) {
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
        activation_compartment_num = payload.input_len;
        batch_num = config_.bit_serial ? payload.input_bit_width : 1;
    }

    return {batch_num, activation_compartment_num};
}

}  // namespace cimsim
