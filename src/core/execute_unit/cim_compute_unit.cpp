//
// Created by wyk on 2024/7/29.
//

#include "cim_compute_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

CimComputeUnit::CimComputeUnit(const sc_module_name &name, const CimUnitConfig &config, const BaseInfo &base_info,
                               Clock *clk)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::cim_compute), config_(config), macro_size_(config.macro_size) {
    SC_THREAD(processIssue)
    SC_THREAD(processSubIns)
    SC_THREAD(readValueSparseMaskSubmodule)
    SC_THREAD(readBitSparseMetaSubmodule)

    if (config_.value_sparse) {
        value_sparse_network_energy_counter_.setStaticPowerMW(config_.value_sparse_config.static_power_mW);
        energy_counter_.addSubEnergyCounter("value sparsity network", &value_sparse_network_energy_counter_);
    }
    if (config_.bit_sparse) {
        meta_buffer_energy_counter_.setStaticPowerMW(config_.bit_sparse_config.reg_buffer_static_power_mW);
        energy_counter_.addSubEnergyCounter("meta buffer", &meta_buffer_energy_counter_);
    }
}

void CimComputeUnit::bindCimUnit(CimUnit *cim_unit) {
    cim_unit_ = cim_unit;
    cim_unit_->bindCimComputeUnit([this](int ins_id) { releaseResource(ins_id); }, [this]() { finishInstruction(); });
}

void CimComputeUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<CimComputeInsPayload>();

        CORE_LOG(fmt::format("Cim compute start, pc: {}", payload->ins.pc));
        ports_.resource_allocate_.write(getDataConflictInfo(*payload));

        for (int batch_num = 0; batch_num < payload->batch_cnt; batch_num++) {
            process_sub_ins_socket_.waitUntilFinishIfBusy();
            process_sub_ins_socket_.payload = {
                .cim_ins_info = {.ins_pc = payload->ins.pc,
                                 .sub_ins_num = batch_num,
                                 .last_sub_ins = batch_num == payload->batch_cnt - 1,
                                 .ins_id = payload->ins.ins_id,
                                 .inst_opcode = payload->ins.inst_opcode,
                                 .inst_group_tag = payload->ins.inst_group_tag},
                .ins_payload = getSubInsCimComputeInsPayload(payload, batch_num),
                .group_max_activation_macro_cnt = cim_unit_->getMacroGroupMaxActivationMacroCount()};
            process_sub_ins_socket_.start_exec.notify();

            if (!process_sub_ins_socket_.payload.cim_ins_info.last_sub_ins) {
                wait(next_sub_ins_);
            }
        }

        readyForNextExecute();
    }
}

void CimComputeUnit::processSubIns() {
    while (true) {
        process_sub_ins_socket_.waitUntilStart();

        const auto &sub_ins_payload = process_sub_ins_socket_.payload;
        const auto &cim_ins_info = sub_ins_payload.cim_ins_info;
        CORE_LOG(fmt::format("Cim compute sub ins start, pc: {}, sub ins: {}", cim_ins_info.ins_pc,
                             cim_ins_info.sub_ins_num));

        processSubInsReadData(sub_ins_payload);
        processSubInsCompute(sub_ins_payload);

        if (!cim_ins_info.last_sub_ins) {
            next_sub_ins_.notify();
        }

        process_sub_ins_socket_.finish();
    }
}

void CimComputeUnit::processSubInsReadData(const cimsim::CimComputeSubInsPayload &sub_ins_payload) {
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
        // int group_cnt = std::min(payload.activation_group_num, static_cast<int>(macro_group_list_.size()));
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

void CimComputeUnit::processSubInsCompute(const CimComputeSubInsPayload &sub_ins_payload) {
    const auto &payload = sub_ins_payload.ins_payload;

    // process groups list
    int size_byte = payload.input_bit_width * payload.input_len / BYTE_TO_BIT;
    auto get_address_byte = [&](int group_id) {
        return payload.input_addr_byte + payload.group_input_step_byte * group_id;
    };
    int group_cnt = std::min(payload.activation_group_num, static_cast<int>(cim_unit_->getActualMacroGroupCount()));
    int total_activation_group_cnt = cim_unit_->isMacroSimulation()
                                         ? std::min(payload.activation_group_num, cim_unit_->getConfigMacroGroupCount())
                                         : 1;
    int total_activation_macro_cnt =
        cim_unit_->isMacroSimulation() ? total_activation_group_cnt * config_.macro_group_size : 1;
    for (int group_id = 0; group_id < group_cnt; group_id++) {
        MacroGroupPayload group_payload{.cim_ins_info = sub_ins_payload.cim_ins_info,
                                        .last_group = group_id == group_cnt - 1,
                                        .row = payload.row,
                                        .input_bit_width = payload.input_bit_width,
                                        .bit_sparse = config_.bit_sparse && payload.bit_sparse,
                                        .input_len = std::min(payload.input_len, macro_size_.compartment_cnt_per_macro),
                                        .simulated_group_cnt = total_activation_group_cnt,
                                        .simulated_macro_cnt = total_activation_macro_cnt};
        for (int i = 0; i < total_activation_group_cnt; i++) {
            group_payload.macro_inputs =
                getMacroGroupInputs(group_id, get_address_byte(group_id), size_byte, sub_ins_payload);
        }
        cim_unit_->runMacroGroup(group_id, std::move(group_payload));

        if (config_.value_sparse && payload.value_sparse &&
            (group_id + 1) % config_.value_sparse_config.output_macro_group_cnt == 0) {
            double dynamic_power_mW = config_.value_sparse_config.dynamic_power_mW;
            double latency = config_.value_sparse_config.latency_cycle * period_ns_;
            value_sparse_network_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW,
                                                                    {.core_id = core_id_,
                                                                     .ins_id = payload.ins.ins_id,
                                                                     .inst_opcode = payload.ins.inst_opcode,
                                                                     .inst_group_tag = payload.ins.inst_group_tag,
                                                                     .inst_profiler_operator = "value_sparse"});
            wait(latency, SC_NS);
        }
    }
    wait(SC_ZERO_TIME);
}

std::vector<std::vector<unsigned long long>> CimComputeUnit::getMacroGroupInputs(
    int group_id, int addr_byte, int size_byte, const cimsim::CimComputeSubInsPayload &sub_ins_payload) {
    const auto &payload = sub_ins_payload.ins_payload;

    auto read_data = memory_socket_.readLocal(payload.ins, addr_byte, size_byte);
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
        for (int macro_id = 0; macro_id < cim_unit_->getMacroGroupActivationMacroCount(group_id); macro_id++) {
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
        for (int i = 0; i < cim_unit_->getMacroGroupActivationMacroCount(group_id); i++) {
            macro_group_inputs.push_back(input_data);
        }
    }

    return std::move(macro_group_inputs);
}

void CimComputeUnit::readValueSparseMaskSubmodule() {
    while (true) {
        read_value_sparse_mask_socket_.waitUntilStart();

        auto &payload = read_value_sparse_mask_socket_.payload;
        payload.data = memory_socket_.readLocal(payload.ins, payload.addr_byte, payload.size_byte);

        read_value_sparse_mask_socket_.finish();
    }
}

void CimComputeUnit::readBitSparseMetaSubmodule() {
    while (true) {
        read_bit_sparse_meta_socket_.waitUntilStart();

        auto &payload = read_bit_sparse_meta_socket_.payload;
        payload.data = memory_socket_.readLocal(payload.ins, payload.addr_byte, payload.size_byte);

        double dynamic_power_mW = config_.bit_sparse_config.reg_buffer_dynamic_power_mW_per_unit *
                                  IntDivCeil(payload.size_byte, config_.bit_sparse_config.unit_byte);
        meta_buffer_energy_counter_.addDynamicEnergyPJ(period_ns_, dynamic_power_mW,
                                                       {.core_id = core_id_,
                                                        .ins_id = payload.ins.ins_id,
                                                        .inst_opcode = payload.ins.inst_opcode,
                                                        .inst_group_tag = payload.ins.inst_group_tag,
                                                        .inst_profiler_operator = "meta_buffer_write"});

        read_bit_sparse_meta_socket_.finish();
    }
}

ResourceAllocatePayload CimComputeUnit::getDataConflictInfo(const cimsim::CimComputeInsPayload &payload) const {
    ResourceAllocatePayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::cim_compute};

    int input_memory_id = as_.getLocalMemoryId(payload.input_addr_byte);
    conflict_payload.addReadMemoryId(input_memory_id, cim_unit_->getMemoryID());

    if (config_.value_sparse && payload.value_sparse) {
        int mask_memory_id = as_.getLocalMemoryId(payload.value_sparse_mask_addr_byte);
        conflict_payload.addReadMemoryId(mask_memory_id);
    }

    if (config_.bit_sparse && payload.bit_sparse) {
        int meta_memory_id = as_.getLocalMemoryId(payload.bit_sparse_meta_addr_byte);
        conflict_payload.addReadMemoryId(meta_memory_id);
    }

    return conflict_payload;
}

ResourceAllocatePayload CimComputeUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload> &payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<CimComputeInsPayload>(payload));
}

CimComputeInsPayload CimComputeUnit::getSubInsCimComputeInsPayload(
    const std::shared_ptr<CimComputeInsPayload> &ins_payload, int batch_num) {
    auto sub_ins_payload = *ins_payload;
    sub_ins_payload.input_addr_byte +=
        batch_num * IntDivCeil(sub_ins_payload.input_len * sub_ins_payload.input_bit_width, BYTE_TO_BIT);
    sub_ins_payload.row += batch_num;
    return sub_ins_payload;
}

}  // namespace cimsim
