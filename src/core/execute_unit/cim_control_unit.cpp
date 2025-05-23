//
// Created by wyk on 2024/8/1.
//

#include "cim_control_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

CimControlUnit::CimControlUnit(const sc_module_name &name, const CimUnitConfig &config, const BaseInfo &base_info,
                               Clock *clk)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::cim_control), config_(config), macro_size_(config.macro_size) {
    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    energy_counter_.addSubEnergyCounter("result adder", &result_adder_energy_counter_);
}

void CimControlUnit::bindCimUnit(CimUnit *cim_unit) {
    cim_unit_ = cim_unit;
}

void CimControlUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<CimControlInsPayload>();
        CORE_LOG(fmt::format("Cim set start, pc: {}", payload->ins.pc));

        ports_.resource_allocate_.write(getDataConflictInfo(*payload));

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = *payload;
        execute_socket_.start_exec.notify();

        readyForNextExecute();
    }
}

void CimControlUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        CORE_LOG(fmt::format("Cim control start execute, pc: {}", payload.ins.pc));

        switch (payload.op) {
            case CimControlOperator::set_activation: processSetActivation(payload); break;
            case CimControlOperator::only_output: processOnlyOutput(payload); break;
            case CimControlOperator::output_sum: processOutputSum(payload); break;
            case CimControlOperator::output_sum_move: processOutputSumMove(payload); break;
            default: break;
        }

        finishInstruction();

        execute_socket_.finish();
    }
}

void CimControlUnit::processSetActivation(const CimControlInsPayload &payload) {
    // read mask
    int mask_size_byte =
        IntDivCeil(1 * macro_size_.element_cnt_per_compartment * config_.macro_group_size, BYTE_TO_BIT);
    auto mask_byte_data = memory_socket_.readLocal(payload.ins, payload.mask_addr_byte, mask_size_byte);

    releaseResource(payload.ins.ins_id);

    if (cim_unit_ != nullptr) {
        cim_unit_->setMacroGroupActivationElementColumn(mask_byte_data, payload.group_broadcast, payload.group_id);
    }
}

void CimControlUnit::processOnlyOutput(const CimControlInsPayload &payload) {
    releaseResource(payload.ins.ins_id);

    int size_byte =
        IntDivCeil(payload.output_bit_width * payload.output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    memory_socket_.writeLocal(payload.ins, payload.output_addr_byte, size_byte, {});
}

void CimControlUnit::processOutputSum(const CimControlInsPayload &payload) {
    // read and process sum mask
    int mask_size_byte = IntDivCeil(payload.output_cnt_per_group, BYTE_TO_BIT);
    auto mask_byte_data = memory_socket_.readLocal(payload.ins, payload.output_mask_addr_byte, mask_size_byte);
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
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW,
                                                    {.core_id = core_id_,
                                                     .ins_id = payload.ins.ins_id,
                                                     .inst_opcode = payload.ins.inst_opcode,
                                                     .inst_group_tag = payload.ins.inst_group_tag,
                                                     .inst_profiler_operator = "result_adder"});

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    releaseResource(payload.ins.ins_id);

    // write to memory
    int valid_output_cnt_per_group = payload.output_cnt_per_group - sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    memory_socket_.writeLocal(payload.ins, payload.output_addr_byte, size_byte, {});
}

void CimControlUnit::processOutputSumMove(const CimControlInsPayload &payload) {
    int sum_times_per_group = payload.output_cnt_per_group;

    double sum_latency = config_.result_adder.latency_cycle * period_ns_;
    double sum_dynamic_power_mW =
        config_.result_adder.dynamic_power_mW * sum_times_per_group * payload.activation_group_num;
    result_adder_energy_counter_.addDynamicEnergyPJ(sum_latency, sum_dynamic_power_mW,
                                                    {.core_id = core_id_,
                                                     .ins_id = payload.ins.ins_id,
                                                     .inst_opcode = payload.ins.inst_opcode,
                                                     .inst_group_tag = payload.ins.inst_group_tag,
                                                     .inst_profiler_operator = "result_adder"});

    // need not wait for result adder finish, because result is written to memory instead of registers in result adder
    double sum_stall_ns = (config_.result_adder.latency_cycle - 1) * period_ns_;
    wait(sum_stall_ns, SC_NS);

    releaseResource(payload.ins.ins_id);

    // write to memory
    int valid_output_cnt_per_group = sum_times_per_group;
    int size_byte =
        IntDivCeil(payload.output_bit_width * valid_output_cnt_per_group * payload.activation_group_num, BYTE_TO_BIT);
    CORE_LOG(fmt::format("size_byte: {}", size_byte));
    memory_socket_.writeLocal(payload.ins, payload.output_addr_byte, size_byte, {});
}

ResourceAllocatePayload CimControlUnit::getDataConflictInfo(const CimControlInsPayload &payload) const {
    ResourceAllocatePayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::cim_control};
    switch (payload.op) {
        case CimControlOperator::set_activation: {
            conflict_payload.addReadMemoryId(as_.getLocalMemoryId(payload.mask_addr_byte), cim_unit_->getMemoryID());
            break;
        }
        case CimControlOperator::only_output:
        case CimControlOperator::output_sum:
        case CimControlOperator::output_sum_move: {
            conflict_payload.addReadMemoryId(cim_unit_->getMemoryID());
            conflict_payload.addWriteMemoryId(as_.getLocalMemoryId(payload.output_addr_byte));
            if (payload.op == +CimControlOperator::output_sum) {
                conflict_payload.addReadMemoryId(as_.getLocalMemoryId(payload.output_mask_addr_byte));
            }
        }
        default: break;
    }

    return conflict_payload;
}

ResourceAllocatePayload CimControlUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload> &payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<CimControlInsPayload>(payload));
}

}  // namespace cimsim
