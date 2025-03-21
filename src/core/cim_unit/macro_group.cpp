//
// Created by wyk on 2024/7/24.
//

#include "macro_group.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

MacroGroup::MacroGroup(const sc_core::sc_module_name& name, const cimsim::CimUnitConfig &config, const cimsim::SimConfig &sim_config,
                       cimsim::Core *core, cimsim::Clock *clk, bool macro_simulation)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , controller_("Controller", config, sim_config, core, clk, next_sub_ins_, result_adder_socket_)
    , activation_macro_cnt_(config.macro_group_size) {
    SC_THREAD(processIssue)
    SC_THREAD(processResultAdderSubmodule)

    for (int i = 0; i < (macro_simulation ? 1 : config_.macro_group_size); i++) {
        auto macro_name = fmt::format("Macro_{}", i);
        bool independent_ipu = config_.value_sparse || i == 0;
        macro_list_.push_back(
            new Macro(macro_name.c_str(), config_, sim_config, core, clk, independent_ipu, &result_adder_socket_));
    }
}

void MacroGroup::startExecute(cimsim::MacroGroupPayload payload) {
    macro_group_socket_.payload = std::move(payload);
    macro_group_socket_.start_exec.notify();
}

void MacroGroup::waitUntilFinishIfBusy() {
    macro_group_socket_.waitUntilFinishIfBusy();
}

EnergyReporter MacroGroup::getEnergyReporter() {
    EnergyReporter macro_group_reporter;
    for (auto *macro : macro_list_) {
        macro_group_reporter.accumulate(macro->getEnergyReporter(), true);
    }
    return std::move(macro_group_reporter);
}

void MacroGroup::setReleaseResourceFunc(std::function<void(int)> release_resource_func) {
    release_resource_func_ = std::move(release_resource_func);
}

void MacroGroup::setFinishInsFunc(std::function<void()> finish_ins_func) {
    finish_ins_func_ = std::move(finish_ins_func);
}

void MacroGroup::setMacrosActivationElementColumn(
    const std::vector<unsigned char> &macros_activation_element_col_mask) {
    for (int i = 0; i < macro_list_.size(); i++) {
        int start_index = i * macro_size_.element_cnt_per_compartment;
        macro_list_[i]->setActivationElementColumn(macros_activation_element_col_mask, start_index);
    }

    activation_macro_cnt_ = std::transform_reduce(
        macro_list_.begin(), macro_list_.end(), 0, [](int a, int b) { return a + b; },
        [](const Macro *macro) { return (macro->getActivationElementColumnCount() > 0) ? 1 : 0; });
}

int MacroGroup::getActivationMacroCount() const {
    return activation_macro_cnt_;
}

int MacroGroup::getActivationElementColumnCount() const {
    return std::transform_reduce(
        macro_list_.begin(), macro_list_.end(), 0, [](int a, int b) { return a + b; },
        [](const Macro *macro) { return macro->getActivationElementColumnCount(); });
}

void MacroGroup::processIssue() {
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
                                       .simulated_group_cnt = payload.simulated_group_cnt,
                                       .simulated_macro_cnt = payload.simulated_macro_cnt};
            if (macro_id < payload.macro_inputs.size()) {
                macro_payload.inputs.swap(payload.macro_inputs[macro_id]);
            }

            auto *macro = macro_list_[macro_id];
            macro->waitUntilFinishIfBusy();
            macro->startExecute(std::move(macro_payload));
        }

        controller_.waitUntilFinishIfBusy();
        controller_.start({.cim_ins_info = cim_ins_info,
                           .last_group = payload.last_group,
                           .input_bit_width = payload.input_bit_width,
                           .bit_sparse = payload.bit_sparse});
        wait(next_sub_ins_);

        macro_group_socket_.finish();
    }
}

void MacroGroup::processResultAdderSubmodule() {
    while (true) {
        result_adder_socket_.waitUntilStart();

        auto &sub_ins_info = result_adder_socket_.payload.sub_ins_info;
        auto &cim_ins_info = sub_ins_info.cim_ins_info;
        CORE_LOG(fmt::format("{} start result adder, ins pc: {}, sub ins num: {}", getName(), cim_ins_info.ins_pc,
                        cim_ins_info.sub_ins_num));

        if (sub_ins_info.last_group && cim_ins_info.last_sub_ins && release_resource_func_) {
            release_resource_func_(cim_ins_info.ins_id);
        }

        double latency = config_.result_adder.latency_cycle * period_ns_;
        wait(latency, SC_NS);

        if (finish_ins_func_ && sub_ins_info.last_group && cim_ins_info.last_sub_ins) {
            finish_ins_func_();
        }

        CORE_LOG(fmt::format("{} end result adder, ins pc: {}, sub ins num: {}", getName(), cim_ins_info.ins_pc,
                        cim_ins_info.sub_ins_num));
        result_adder_socket_.finish();
    }
}

}  // namespace cimsim
