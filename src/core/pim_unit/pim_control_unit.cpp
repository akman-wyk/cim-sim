//
// Created by wyk on 2024/8/1.
//

#include "pim_control_unit.h"

#include "fmt/format.h"
#include "pim_compute_unit.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimControlUnit::PimControlUnit(const char *name, const pimsim::PimUnitConfig &config,
                               const pimsim::SimConfig &sim_config, pimsim::Core *core, pimsim::Clock *clk)
    : ExecuteUnit(name, sim_config, core, clk, ExecuteUnitType::pim_control)
    , config_(config)
    , macro_size_(config.macro_size) {
    SC_THREAD(processIssue)
    SC_THREAD(processExecute)
}

void PimControlUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void PimControlUnit::bindCimUnit(CimUnit *cim_unit) {
    cim_unit_ = cim_unit;
}

EnergyReporter PimControlUnit::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("result adder", EnergyReporter{result_adder_energy_counter_});
    return std::move(reporter);
}

void PimControlUnit::processIssue() {
    while (true) {
        auto payload = waitForExecuteAndGetPayload<PimControlInsPayload>();
        LOG(fmt::format("Pim set start, pc: {}", payload->ins.pc));

        ports_.data_conflict_port_.write(getDataConflictInfo(*payload));

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = *payload;
        execute_socket_.start_exec.notify();

        readyForNextExecute();
    }
}

void PimControlUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Pim control start execute, pc: {}", payload.ins.pc));

        switch (payload.op) {
            case PimControlOperator::set_activation: processSetActivation(payload); break;
            case PimControlOperator::only_output: processOnlyOutput(payload); break;
            case PimControlOperator::output_sum: processOutputSum(payload); break;
            case PimControlOperator::output_sum_move: processOutputSumMove(payload); break;
            default: break;
        }

        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            triggerFinishRun();
        }

        execute_socket_.finish();
    }
}

void PimControlUnit::processSetActivation(const PimControlInsPayload &payload) {
    // read mask
    int mask_size_byte =
        IntDivCeil(1 * macro_size_.element_cnt_per_compartment * config_.macro_group_size, BYTE_TO_BIT);
    auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.mask_addr_byte, mask_size_byte);

    triggerFinishInstruction(payload.ins.ins_id);

    if (cim_unit_ != nullptr) {
        cim_unit_->setMacroGroupActivationElementColumn(mask_byte_data, payload.group_broadcast, payload.group_id);
    }
}

void PimControlUnit::processOnlyOutput(const PimControlInsPayload &payload) {
    triggerFinishInstruction(payload.ins.ins_id);

    int size_byte =
        IntDivCeil(payload.output_bit_width * payload.output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimControlUnit::processOutputSum(const PimControlInsPayload &payload) {
    // read and process sum mask
    int mask_size_byte = IntDivCeil(payload.output_cnt_per_group, BYTE_TO_BIT);
    auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.output_mask_addr_byte, mask_size_byte);
    int sum_times_per_group = 0;
    for (int i = 0; i < payload.output_cnt_per_group; i++) {
        if (getMaskBit(mask_byte_data, i) != 0) {
            sum_times_per_group++;
        }
    }

    // sum
    double sum_latency = config_.result_adder.latency_cycle * period_ns_;
    double sum_dynamic_power_mW =
        config_.result_adder.dynamic_power_mW * sum_times_per_group * payload.activation_group_num;
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW);

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    triggerFinishInstruction(payload.ins.ins_id);

    // write to memory
    int valid_output_cnt_per_group = payload.output_cnt_per_group - sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimControlUnit::processOutputSumMove(const PimControlInsPayload &payload) {
    int sum_times_per_group = payload.output_cnt_per_group;

    double sum_latency = config_.result_adder.latency_cycle * period_ns_;
    double sum_dynamic_power_mW =
        config_.result_adder.dynamic_power_mW * sum_times_per_group * payload.activation_group_num;
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW);

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    triggerFinishInstruction(payload.ins.ins_id);

    // write to memory
    int valid_output_cnt_per_group = sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

DataConflictPayload PimControlUnit::getDataConflictInfo(const PimControlInsPayload &payload) const {
    DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_control};
    switch (payload.op) {
        case PimControlOperator::set_activation: {
            conflict_payload.addReadMemoryId({local_memory_socket_.getLocalMemoryIdByAddress(payload.mask_addr_byte),
                                              cim_unit_->getLocalMemoryId()});
            break;
        }
        case PimControlOperator::only_output:
        case PimControlOperator::output_sum:
        case PimControlOperator::output_sum_move: {
            conflict_payload.addReadMemoryId(cim_unit_->getLocalMemoryId());
            conflict_payload.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.output_addr_byte));
            if (payload.op == +PimControlOperator::output_sum) {
                conflict_payload.addReadMemoryId(
                    local_memory_socket_.getLocalMemoryIdByAddress(payload.output_mask_addr_byte));
            }
        }
        default: break;
    }

    return std::move(conflict_payload);
}

DataConflictPayload PimControlUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload> &payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<PimControlInsPayload>(payload));
}

}  // namespace pimsim
