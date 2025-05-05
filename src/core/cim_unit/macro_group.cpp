//
// Created by wyk on 2024/7/24.
//

#include "macro_group.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

MacroGroup::MacroGroup(const sc_module_name &name, const CimUnitConfig &config, const BaseInfo &base_info,
                       EnergyCounter &cim_unit_energy_counter, bool macro_simulation)
    : BaseModule(name, base_info)
    , config_(config)
    , macro_size_(config.macro_size)
    , activation_macro_cnt_(config.macro_group_size) {
    SC_THREAD(processIPUAndIssue)

    // build macro modules
    sram_read_ = std::make_shared<MacroGroupModule>("sram_read", base_info, config_.sram.read_latency_cycle, 1, false);
    if (config_.bit_sparse) {
        post_process_ = std::make_shared<MacroGroupModule>(
            "post_process", base_info, config_.bit_sparse ? config_.bit_sparse_config.latency_cycle : 0, 1, false);
    }
    if (config_.independent_mult) {
        mult_ = std::make_shared<MacroGroupModule>("mult", base_info, config_.mult, false);
    }
    adder_tree_ = std::make_shared<MacroGroupModule>("adder_tree", base_info, config_.adder_tree, false);
    if (config_.bit_serial) {
        shift_adder_ = std::make_shared<MacroGroupModule>("shift_adder", base_info, config_.shift_adder, false);
    }
    result_adder_ = std::make_shared<MacroGroupModule>("result_adder", base_info, config_.result_adder, true);

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

    for (int i = 0; i < (macro_simulation ? 1 : config_.macro_group_size); i++) {
        auto macro_name = fmt::format("Macro_{}", i);
        bool independent_ipu = config_.value_sparse || i == 0;
        macro_list_.push_back(std::make_shared<Macro>(macro_name.c_str(), config_, base_info, independent_ipu,
                                                      cim_unit_energy_counter, macro_simulation));
    }
}

void MacroGroup::startExecute(cimsim::MacroGroupPayload payload) {
    macro_group_socket_.payload = std::move(payload);
    macro_group_socket_.start_exec.notify();
}

void MacroGroup::waitUntilFinishIfBusy() {
    macro_group_socket_.waitUntilFinishIfBusy();
}

void MacroGroup::setReleaseResourceFunc(std::function<void(int)> release_resource_func) {
    result_adder_->setReleaseResourceFunc(std::move(release_resource_func));
}

void MacroGroup::setFinishInsFunc(std::function<void()> finish_ins_func) {
    result_adder_->setFinishInsFunc(std::move(finish_ins_func));
}

void MacroGroup::setMacrosActivationElementColumn(
    const std::vector<unsigned char> &macros_activation_element_col_mask) {
    for (int i = 0; i < macro_list_.size(); i++) {
        int start_index = i * macro_size_.element_cnt_per_compartment;
        macro_list_[i]->setActivationElementColumn(macros_activation_element_col_mask, start_index);
    }

    activation_macro_cnt_ = std::transform_reduce(
        macro_list_.begin(), macro_list_.end(), 0, [](int a, int b) { return a + b; },
        [](const std::shared_ptr<Macro> &macro) { return (macro->getActivationElementColumnCount() > 0) ? 1 : 0; });
}

int MacroGroup::getActivationMacroCount() const {
    return activation_macro_cnt_;
}

int MacroGroup::getActivationElementColumnCount() const {
    return std::transform_reduce(
        macro_list_.begin(), macro_list_.end(), 0, [](int a, int b) { return a + b; },
        [](const std::shared_ptr<Macro> &macro) { return macro->getActivationElementColumnCount(); });
}

void MacroGroup::processIPUAndIssue() {
    while (true) {
        macro_group_socket_.waitUntilStart();

        auto &payload = macro_group_socket_.payload;
        auto &cim_ins_info = payload.cim_ins_info;
        CORE_LOG(fmt::format("{} start, ins pc: {}, sub ins num: {}", getName(), cim_ins_info.ins_pc,
                             cim_ins_info.sub_ins_num));

        for (int macro_id = 0; macro_id < macro_list_.size(); macro_id++) {
            MacroPayload macro_payload{.cim_ins_info = cim_ins_info,
                                       .row = payload.row,
                                       .input_bit_width = payload.input_bit_width,
                                       .bit_sparse = payload.bit_sparse,
                                       .input_len = payload.input_len,
                                       .simulated_group_cnt = payload.simulated_group_cnt,
                                       .simulated_macro_cnt = payload.simulated_macro_cnt};
            if (macro_id < payload.macro_inputs.size()) {
                macro_payload.inputs.swap(payload.macro_inputs[macro_id]);
            }

            auto &macro = macro_list_[macro_id];
            macro->waitUntilFinishIfBusy();
            macro->startExecute(std::move(macro_payload));
        }

        MacroGroupSubmodulePayload submodule_payload{
            .sub_ins_info = std::make_shared<MacroGroupSubInsInfo>(MacroGroupSubInsInfo{
                .cim_ins_info = cim_ins_info, .last_group = payload.last_group, .bit_sparse = payload.bit_sparse})};
        int batch_count = config_.bit_serial ? payload.input_bit_width : 1;
        for (int batch = 0; batch < batch_count; batch++) {
            submodule_payload.batch_info = std::make_shared<MacroBatchInfo>(
                MacroBatchInfo{.batch_num = batch, .last_batch = (batch == batch_count - 1)});

            CORE_LOG(fmt::format("{} start ipu and issue, ins pc: {}, sub ins num: {}, batch: {}", getName(),
                                 cim_ins_info.ins_pc, cim_ins_info.sub_ins_num,
                                 submodule_payload.batch_info->batch_num));
            double latency = config_.bit_serial ? config_.ipu.latency_cycle * period_ns_ : 0;
            wait(latency, SC_NS);
            waitAndStartNextStage(submodule_payload, *(sram_read_->getExecuteSocket()));
        }

        macro_group_socket_.finish();
    }
}

}  // namespace cimsim
