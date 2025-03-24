//
// Created by wyk on 2024/7/15.
//

#include "transfer_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

TransferUnit::TransferUnit(const sc_module_name& name, const TransferUnitConfig& config, const BaseInfo& base_info,
                           Clock* clk, int global_memory_switch_id)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::transfer)
    , config_(config)
    , global_memory_switch_id_(global_memory_switch_id) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadSubmodule)
    SC_THREAD(processWriteSubmodule)
}

void TransferUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<TransferInsPayload>();

        const auto& [ins_info, conflict_payload] = decodeAndGetInfo(*payload);
        ports_.resource_allocate_.write(conflict_payload);

        if (payload->type == +TransferType::send) {
            transmit_socket_.sendHandshake(payload->dst_id, payload->transfer_id_tag);
        } else if (payload->type == +TransferType::receive) {
            transmit_socket_.receiveHandshake(payload->src_id, payload->transfer_id_tag);
        }

        int process_times = IntDivCeil(payload->size_byte, ins_info.batch_max_data_size_byte);
        TransferSubmodulePayload submodule_payload{.ins_info = std::make_shared<TransferInstructionInfo>(ins_info)};
        for (int batch = 0; batch < process_times; batch++) {
            submodule_payload.batch_info = std::make_shared<TransferBatchInfo>(TransferBatchInfo{
                .batch_num = batch,
                .batch_data_size_byte = (batch == process_times - 1)
                                            ? payload->size_byte - batch * ins_info.batch_max_data_size_byte
                                            : ins_info.batch_max_data_size_byte,
                .last_batch = (batch == process_times - 1)});
            waitAndStartNextStage(submodule_payload, read_submodule_socket_);

            if (!submodule_payload.batch_info->last_batch) {
                wait(cur_ins_next_batch_);
            }
        }

        readyForNextExecute();
    }
}

void TransferUnit::processReadSubmodule() {
    while (true) {
        read_submodule_socket_.waitUntilStart();

        auto& payload = read_submodule_socket_.payload;
        CORE_LOG(fmt::format("transfer read start, pc: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        int address_byte = payload.ins_info->src_start_address_byte +
                           payload.batch_info->batch_num * payload.ins_info->batch_max_data_size_byte;
        int size_byte = payload.batch_info->batch_data_size_byte;
        if (auto type = payload.ins_info->type; type == +TransferType::receive) {
            transmit_socket_.receiveData(payload.ins_info->src_id);
        } else if (type == +TransferType::global_load) {
            payload.batch_info->data = memory_socket_.loadGlobal(payload.ins_info->ins, address_byte, size_byte);
        } else {
            payload.batch_info->data = memory_socket_.readLocal(payload.ins_info->ins, address_byte, size_byte);
        }

        waitAndStartNextStage(payload, write_submodule_socket_);

        if (!payload.batch_info->last_batch && payload.ins_info->use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        read_submodule_socket_.finish();
    }
}

void TransferUnit::processWriteSubmodule() {
    while (true) {
        write_submodule_socket_.waitUntilStart();

        const auto& payload = write_submodule_socket_.payload;
        CORE_LOG(fmt::format("transfer write start, pc: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        if (payload.batch_info->last_batch) {
            releaseResource(payload.ins_info->ins.ins_id);
        }

        int address_byte = payload.ins_info->dst_start_address_byte +
                           payload.batch_info->batch_num * payload.ins_info->batch_max_data_size_byte;
        int size_byte = payload.batch_info->batch_data_size_byte;
        if (auto type = payload.ins_info->type; type == +TransferType::send) {
            transmit_socket_.sendData(payload.ins_info->dst_id, payload.ins_info->transfer_id_tag, address_byte,
                                      size_byte);
        } else if (type == +TransferType::global_store) {
            memory_socket_.storeGlobal(payload.ins_info->ins, address_byte, size_byte, payload.batch_info->data);
        } else {
            memory_socket_.writeLocal(payload.ins_info->ins, address_byte, size_byte, payload.batch_info->data);
        }

        CORE_LOG(fmt::format("transfer write end, pc: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        if (!payload.batch_info->last_batch && !payload.ins_info->use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        if (payload.batch_info->last_batch) {
            finishInstruction();
        }

        write_submodule_socket_.finish();
    }
}

void TransferUnit::bindSwitch(Switch* switch_) {
    memory_socket_.bindSwitchAndGlobalMemory(switch_, core_id_, global_memory_switch_id_);
    transmit_socket_.bindSwitch(switch_, core_id_);
}

std::pair<TransferInstructionInfo, ResourceAllocatePayload> TransferUnit::decodeAndGetInfo(
    const TransferInsPayload& payload) const {
    TransferInstructionInfo ins_info;
    if (payload.type == +TransferType::local_trans) {
        int src_memory_id = as_.getLocalMemoryId(payload.src_address_byte);
        int dst_memory_id = as_.getLocalMemoryId(payload.dst_address_byte);

        int data_width_byte =
            std::max(memory_socket_.getLocalMemoryDataWidthById(src_memory_id, MemoryAccessType::read),
                     memory_socket_.getLocalMemoryDataWidthById(dst_memory_id, MemoryAccessType::write));
        bool use_pipeline = config_.pipeline && (src_memory_id != dst_memory_id);

        ins_info = {.ins = payload.ins,
                    .type = TransferType::local_trans,
                    .src_start_address_byte = payload.src_address_byte,
                    .dst_start_address_byte = payload.dst_address_byte,
                    .batch_max_data_size_byte = data_width_byte,
                    .use_pipeline = use_pipeline};
    } else {
        ins_info = {.ins = payload.ins,
                    .type = payload.type,
                    .src_start_address_byte = payload.src_address_byte,
                    .dst_start_address_byte = payload.dst_address_byte,
                    .batch_max_data_size_byte = payload.size_byte,
                    .src_id = payload.src_id,
                    .dst_id = payload.dst_id,
                    .transfer_id_tag = payload.transfer_id_tag,
                    .use_pipeline = false};
    }
    return {ins_info, getDataConflictInfo(payload)};
}

ResourceAllocatePayload TransferUnit::getDataConflictInfo(const TransferInsPayload& payload) const {
    ResourceAllocatePayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::transfer};
    if (payload.type == +TransferType::local_trans || payload.type == +TransferType::send ||
        payload.type == +TransferType::global_store) {
        conflict_payload.addReadMemoryId(as_.getLocalMemoryId(payload.src_address_byte));
    }
    if (payload.type == +TransferType::local_trans || payload.type == +TransferType::receive ||
        payload.type == +TransferType::global_load) {
        conflict_payload.addWriteMemoryId(as_.getLocalMemoryId(payload.dst_address_byte));
    }
    return conflict_payload;
}

ResourceAllocatePayload TransferUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<TransferInsPayload>(payload));
}

}  // namespace cimsim
