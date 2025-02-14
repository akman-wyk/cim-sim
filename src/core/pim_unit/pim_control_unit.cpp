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
    : BaseModule(name, sim_config, core, clk), config_(config), macro_size_(config.macro_size), fsm_("PimControlFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimControlInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
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

void PimControlUnit::checkPimControlInst() {
    if (const auto &payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimControlUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        ports_.busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim set start, pc: {}", payload.ins.pc));

        ports_.data_conflict_port_.write(getDataConflictInfo(payload));

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        ports_.busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
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
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }

        execute_socket_.finish();
    }
}

void PimControlUnit::processSetActivation(const PimControlInsPayload &payload) {
    // read mask
    int mask_size_byte =
        IntDivCeil(1 * macro_size_.element_cnt_per_compartment * config_.macro_group_size, BYTE_TO_BIT);
    auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.mask_addr_byte, mask_size_byte);

    finish_ins_ = true;
    finish_ins_id_ = payload.ins.ins_id;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

    if (cim_unit_ != nullptr) {
        cim_unit_->setMacroGroupActivationElementColumn(mask_byte_data, payload.group_broadcast, payload.group_id);
    }
}

void PimControlUnit::processOnlyOutput(const PimControlInsPayload &payload) {
    finish_ins_ = true;
    finish_ins_id_ = payload.ins.ins_id;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

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

    finish_ins_ = true;
    finish_ins_id_ = payload.ins.ins_id;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

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

    finish_ins_ = true;
    finish_ins_id_ = payload.ins.ins_id;
    finish_ins_trigger_.notify(SC_ZERO_TIME);

    // write to memory
    int valid_output_cnt_per_group = sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    local_memory_socket_.writeData(payload.ins, payload.output_addr_byte, size_byte, {});
}

void PimControlUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_id_port_.write(finish_ins_id_);
}

void PimControlUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

DataConflictPayload PimControlUnit::getDataConflictInfo(const PimControlInsPayload &payload) const {
    DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_control};
    switch (payload.op) {
        case PimControlOperator::set_activation: {
            conflict_payload.use_pim_unit = true;
            conflict_payload.addReadMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.mask_addr_byte));
            break;
        }
        case PimControlOperator::only_output:
        case PimControlOperator::output_sum:
        case PimControlOperator::output_sum_move: {
            conflict_payload.use_pim_unit = true;
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

}  // namespace pimsim
