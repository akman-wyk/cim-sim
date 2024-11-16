//
// Created by wyk on 2024/7/29.
//

#include "pim_compute_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimComputeUnit::PimComputeUnit(const char *name, const pimsim::PimUnitConfig &config,
                               const pimsim::SimConfig &sim_config, pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , fsm_("PimComputeFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimComputeInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processSubIns)
    SC_THREAD(readValueSparseMaskSubmodule)
    SC_THREAD(readBitSparseMetaSubmodule)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;

    config_group_cnt_ = config_.macro_total_cnt / config_.macro_group_size;
    for (int group_id = 0; group_id < 1; group_id++) {
        auto macro_name = fmt::format("MacroGroup_{}", group_id);
        auto macro_group = new MacroGroup{macro_name.c_str(), config_, sim_config, core, clk};
        macro_group->setFinishInsFunc([this](int ins_id) {
            finish_ins_ = true;
            finish_ins_id_ = ins_id;
            finish_ins_trigger_.notify(SC_ZERO_TIME);
        });
        macro_group->setFinishRunFunc([this]() {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        });
        macro_group_list_.emplace_back(macro_group);
    }

    if (config_.value_sparse) {
        value_sparse_network_energy_counter_.setStaticPowerMW(config_.value_sparse_config.static_power_mW);
    }
    if (config_.bit_sparse) {
        meta_buffer_energy_counter_.setStaticPowerMW(config_.bit_sparse_config.reg_buffer_static_power_mW);
    }
}

void PimComputeUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

EnergyReporter PimComputeUnit::getEnergyReporter() {
    EnergyReporter pim_compute_reporter;
    if (config_.value_sparse) {
        pim_compute_reporter.addSubModule("value sparsity network",
                                          EnergyReporter{value_sparse_network_energy_counter_});
    }
    if (config_.bit_sparse) {
        pim_compute_reporter.addSubModule("meta buffer", EnergyReporter{meta_buffer_energy_counter_});
    }
    for (auto *macro_group : macro_group_list_) {
        pim_compute_reporter.accumulate(macro_group->getEnergyReporter(), true);
    }
    return std::move(pim_compute_reporter);
}

void PimComputeUnit::setMacroGroupActivationElementColumn(const std::vector<unsigned char> &mask, bool group_broadcast,
                                                          int group_id) {
    if (group_broadcast) {
        for (auto *macro_group : macro_group_list_) {
            macro_group->setMacrosActivationElementColumn(mask);
        }
    } else if (0 <= group_id && group_id < macro_group_list_.size()) {
        macro_group_list_[group_id]->setMacrosActivationElementColumn(mask);
    }
}

int PimComputeUnit::getMacroGroupActivationElementColumnCount(int group_id) const {
    if (group_id < 0 || group_id >= macro_group_list_.size()) {
        return 0;
    }
    return macro_group_list_[group_id]->getActivationElementColumnCount();
}

int PimComputeUnit::getMacroGroupActivationMacroCount(int group_id) const {
    if (group_id < 0 || group_id >= macro_group_list_.size()) {
        return 0;
    }
    return macro_group_list_[group_id]->getActivationMacroCount();
}

void PimComputeUnit::checkPimComputeInst() {
    if (const auto &payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimComputeUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        ports_.busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim compute start, pc: {}", payload.ins.pc));
        ports_.data_conflict_port_.write(getDataConflictInfo(payload));

        process_sub_ins_socket_.waitUntilFinishIfBusy();
        process_sub_ins_socket_.payload = {
            .pim_ins_info = {.ins_pc = payload.ins.pc,
                             .sub_ins_num = 1,
                             .last_ins = isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round,
                             .last_sub_ins = true,
                             .ins_id = payload.ins.ins_id},
            .ins_payload = payload,
            .group_max_activation_macro_cnt = std::transform_reduce(
                macro_group_list_.begin(), macro_group_list_.end(), 0, [](int a, int b) { return std::max(a, b); },
                [](const MacroGroup *macro_group) { return macro_group->getActivationMacroCount(); })};
        process_sub_ins_socket_.start_exec.notify();

        if (!process_sub_ins_socket_.payload.pim_ins_info.last_sub_ins) {
            wait(next_sub_ins_);
        }

        ports_.busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void PimComputeUnit::processSubIns() {
    while (true) {
        process_sub_ins_socket_.waitUntilStart();

        this->energy_counter_.addDynamicEnergyPJ(8 * period_ns_, 0);

        const auto &sub_ins_payload = process_sub_ins_socket_.payload;
        const auto &pim_ins_info = sub_ins_payload.pim_ins_info;
        LOG(fmt::format("Pim compute sub ins start, pc: {}, sub ins: {}", pim_ins_info.ins_pc,
                        pim_ins_info.sub_ins_num));

        processSubInsReadData(sub_ins_payload);
        processSubInsCompute(sub_ins_payload);

        if (!pim_ins_info.last_sub_ins) {
            next_sub_ins_.notify();
        }

        process_sub_ins_socket_.finish();
    }
}

void PimComputeUnit::processSubInsReadData(const pimsim::PimComputeSubInsPayload &sub_ins_payload) {
    const auto &payload = sub_ins_payload.ins_payload;

    // start reading data in parallel
    // read value sparse mask data
    if (config_.value_sparse && payload.value_sparse) {
        read_value_sparse_mask_socket_.payload = {
            .ins = payload.ins,
            .addr_byte = payload.value_sparse_mask_addr_byte,
            .size_byte = config_.value_sparse_config.mask_bit_width * payload.input_len *
                         sub_ins_payload.group_max_activation_macro_cnt / BYTE_TO_BIT};
        read_value_sparse_mask_socket_.start_exec.notify();
    }
    // read bit sparse meta data
    if (config_.bit_sparse && payload.bit_sparse) {
        // TODO: 如果需要读取多次的话，尚存在一些问题
        int group_cnt = std::min(payload.activation_group_num, static_cast<int>(macro_group_list_.size()));
        read_bit_sparse_meta_socket_.payload = {
            .ins = payload.ins,
            .addr_byte = payload.bit_sparse_meta_addr_byte,
            .size_byte = config_.bit_sparse_config.mask_bit_width * macro_size_.element_cnt_per_compartment *
                         macro_size_.compartment_cnt_per_macro * sub_ins_payload.group_max_activation_macro_cnt /
                         BYTE_TO_BIT};
        read_bit_sparse_meta_socket_.start_exec.notify();
    }

    // wait for read data finish
    wait(SC_ZERO_TIME);
    read_value_sparse_mask_socket_.waitUntilFinishIfBusy();
    read_bit_sparse_meta_socket_.waitUntilFinishIfBusy();
}

void PimComputeUnit::processSubInsCompute(const PimComputeSubInsPayload &sub_ins_payload) {
    const auto &payload = sub_ins_payload.ins_payload;

    // process groups list
    int size_byte = payload.input_bit_width * payload.input_len / BYTE_TO_BIT;
    auto get_address_byte = [&](int group_id) {
        return payload.input_addr_byte + payload.group_input_step_byte * group_id;
    };
    int group_cnt = std::min(payload.activation_group_num, static_cast<int>(macro_group_list_.size()));
    int total_activation_group_cnt = std::min(payload.activation_group_num, config_group_cnt_);
    int total_activation_macro_cnt = total_activation_group_cnt * config_.macro_group_size;
    for (int group_id = 0; group_id < group_cnt; group_id++) {
        MacroGroupPayload group_payload{.pim_ins_info = sub_ins_payload.pim_ins_info,
                                        .last_group = group_id == group_cnt - 1,
                                        .row = payload.row,
                                        .input_bit_width = payload.input_bit_width,
                                        .bit_sparse = config_.bit_sparse && payload.bit_sparse,
                                        .total_activation_group_cnt = total_activation_group_cnt,
                                        .total_activation_macro_cnt = total_activation_macro_cnt};
        for (int i = 0; i < total_activation_group_cnt; i++) {
            group_payload.macro_inputs =
                getMacroGroupInputs(group_id, get_address_byte(group_id), size_byte, sub_ins_payload);
        }
        auto *macro_group = macro_group_list_[group_id];
        macro_group->waitUntilFinishIfBusy();
        macro_group->startExecute(std::move(group_payload));

        if (config_.value_sparse && payload.value_sparse &&
            (group_id + 1) % config_.value_sparse_config.output_macro_group_cnt == 0) {
            double dynamic_power_mW = config_.value_sparse_config.dynamic_power_mW;
            double latency = config_.value_sparse_config.latency_cycle * period_ns_;
            value_sparse_network_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
            wait(latency, SC_NS);
        }
    }
    wait(SC_ZERO_TIME);
}

std::vector<std::vector<unsigned long long>> PimComputeUnit::getMacroGroupInputs(
    int group_id, int addr_byte, int size_byte, const pimsim::PimComputeSubInsPayload &sub_ins_payload) {
    const auto &payload = sub_ins_payload.ins_payload;

    auto read_data = local_memory_socket_.readData(payload.ins, addr_byte, size_byte);
    if (data_mode_ == +DataMode::not_real_data) {
        return {};
    }

    std::vector<unsigned long long> input_data;
    if (payload.input_bit_width == BYTE_TO_BIT) {
        input_data.resize(read_data.size());
        std::transform(read_data.begin(), read_data.end(), input_data.begin(),
                       [](unsigned char data) { return static_cast<unsigned long long>(data); });
    }

    std::vector<std::vector<unsigned long long>> macro_group_inputs;
    if (config_.value_sparse && payload.value_sparse) {
        const auto &mask_byte_data = read_value_sparse_mask_socket_.payload.data;
        for (int macro_id = 0; macro_id < macro_group_list_[group_id]->getActivationMacroCount(); macro_id++) {
            std::vector<unsigned long long> macro_input;
            macro_input.reserve(macro_size_.compartment_cnt_per_macro);
            for (int i = 0; i < payload.input_len; i++) {
                if (getMaskBit(mask_byte_data, macro_id * payload.input_len + i) != 0) {
                    macro_input.push_back(input_data[i]);
                }
                if (macro_input.size() == macro_size_.compartment_cnt_per_macro) {
                    break;
                }
            }
            macro_group_inputs.push_back(std::move(macro_input));
        }
    } else {
        for (int i = 0; i < macro_group_list_[group_id]->getActivationMacroCount(); i++) {
            macro_group_inputs.push_back(input_data);
        }
    }

    return std::move(macro_group_inputs);
}

void PimComputeUnit::readValueSparseMaskSubmodule() {
    while (true) {
        read_value_sparse_mask_socket_.waitUntilStart();

        auto &payload = read_value_sparse_mask_socket_.payload;
        payload.data = local_memory_socket_.readData(payload.ins, payload.addr_byte, payload.size_byte);

        read_value_sparse_mask_socket_.finish();
    }
}

void PimComputeUnit::readBitSparseMetaSubmodule() {
    while (true) {
        read_bit_sparse_meta_socket_.waitUntilStart();

        auto &payload = read_bit_sparse_meta_socket_.payload;
        payload.data = local_memory_socket_.readData(payload.ins, payload.addr_byte, payload.size_byte);

        double dynamic_power_mW = config_.bit_sparse_config.reg_buffer_dynamic_power_mW_per_unit *
                                  IntDivCeil(payload.size_byte, config_.bit_sparse_config.unit_byte);
        meta_buffer_energy_counter_.addDynamicEnergyPJ(period_ns_, dynamic_power_mW);

        read_bit_sparse_meta_socket_.finish();
    }
}

void PimComputeUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_id_port_.write(finish_ins_id_);
}

void PimComputeUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

DataConflictPayload PimComputeUnit::getDataConflictInfo(const pimsim::PimComputeInsPayload &payload) {
    DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_compute};
    conflict_payload.use_pim_unit = true;

    int input_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.input_addr_byte);
    conflict_payload.addReadMemoryId(input_memory_id);

    if (config_.value_sparse && payload.value_sparse) {
        int mask_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.value_sparse_mask_addr_byte);
        conflict_payload.addReadMemoryId(mask_memory_id);
    }

    if (config_.bit_sparse && payload.bit_sparse) {
        int meta_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.bit_sparse_meta_addr_byte);
        conflict_payload.addReadMemoryId(meta_memory_id);
    }

    return std::move(conflict_payload);
}

}  // namespace pimsim
