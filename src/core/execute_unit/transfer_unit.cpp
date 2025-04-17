//
// Created by wyk on 2024/7/15.
//

#include "transfer_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

LocalTransferDataPath::LocalTransferDataPath(const sc_module_name& name, const BaseInfo& base_info,
                                             TransferUnit& transfer_unit, bool pipeline)
    : BaseModule(name, base_info), transfer_unit_(transfer_unit), pipeline_(pipeline) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadStage)
    SC_THREAD(processWriteStage)
}

void LocalTransferDataPath::processIssue() {
    while (true) {
        exec_socket_.waitUntilStart();

        auto& payload = exec_socket_.payload;
        LocalTransferStagePayload stage_payload{.ins_info = payload.ins_info};
        for (int batch = 0; batch < payload.process_times; batch++) {
            stage_payload.batch_info = std::make_shared<LocalTransferBatchInfo>(LocalTransferBatchInfo{
                .batch_num = batch,
                .batch_data_size_byte =
                    (batch == payload.process_times - 1)
                        ? payload.data_size_byte - batch * payload.ins_info->batch_max_data_size_byte
                        : payload.ins_info->batch_max_data_size_byte,
                .last_batch = (batch == payload.process_times - 1)});
            waitAndStartNextStage(stage_payload, read_stage_socket_);

            if (!stage_payload.batch_info->last_batch) {
                wait(cur_ins_next_batch_);
            }
        }

        exec_socket_.finish();
    }
}

void LocalTransferDataPath::processReadStage() {
    while (true) {
        read_stage_socket_.waitUntilStart();

        auto& payload = read_stage_socket_.payload;
        CORE_LOG(fmt::format("{} read start, pc: {}, batch: {}", getName(), payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        int address_byte = payload.ins_info->src_start_address_byte +
                           payload.batch_info->batch_num * payload.ins_info->batch_max_data_size_byte;
        int size_byte = payload.batch_info->batch_data_size_byte;
        payload.batch_info->data = memory_socket_.readLocal(payload.ins_info->ins, address_byte, size_byte);

        waitAndStartNextStage(payload, write_stage_socket_);

        if (!payload.batch_info->last_batch && payload.ins_info->use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        read_stage_socket_.finish();
    }
}

void LocalTransferDataPath::processWriteStage() {
    while (true) {
        write_stage_socket_.waitUntilStart();

        const auto& payload = write_stage_socket_.payload;
        CORE_LOG(fmt::format("{} write start, pc: {}, batch: {}", getName(), payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        if (payload.batch_info->last_batch) {
            transfer_unit_.releaseResource(payload.ins_info->ins.ins_id);
        }

        int address_byte = payload.ins_info->dst_start_address_byte +
                           payload.batch_info->batch_num * payload.ins_info->batch_max_data_size_byte;
        int size_byte = payload.batch_info->batch_data_size_byte;
        memory_socket_.writeLocal(payload.ins_info->ins, address_byte, size_byte, payload.batch_info->data);

        CORE_LOG(fmt::format("{} write end, pc: {}, batch: {}", getName(), payload.ins_info->ins.pc,
                             payload.batch_info->batch_num));

        if (!payload.batch_info->last_batch && !payload.ins_info->use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        if (payload.batch_info->last_batch) {
            transfer_unit_.finishInstruction();
        }

        write_stage_socket_.finish();
    }
}

void LocalTransferDataPath::bindLocalMemoryUnit(MemoryUnit* local_memory_unit) {
    memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

GlobalTransferDataPath::GlobalTransferDataPath(const sc_module_name& name, const BaseInfo& base_info,
                                               TransferUnit& transfer_unit)
    : BaseModule(name, base_info), transfer_unit_(transfer_unit) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadStage)
    SC_THREAD(processWriteStage)
}

void GlobalTransferDataPath::processIssue() {
    while (true) {
        exec_socket_.waitUntilStart();

        auto& payload = exec_socket_.payload;

        if (payload.ins_info->type == +TransferType::send) {
            transmit_socket_.sendHandshake(payload.ins_info->ins, payload.ins_info->dst_id,
                                           payload.ins_info->transfer_id_tag);
        } else if (payload.ins_info->type == +TransferType::receive) {
            transmit_socket_.receiveHandshake(payload.ins_info->src_id, payload.ins_info->transfer_id_tag);
        }

        GlobalTransferStagePayload stage_payload{.ins_info = payload.ins_info};
        waitAndStartNextStage(stage_payload, read_stage_socket_);

        exec_socket_.finish();
    }
}

void GlobalTransferDataPath::processReadStage() {
    while (true) {
        read_stage_socket_.waitUntilStart();

        auto& payload = read_stage_socket_.payload;
        CORE_LOG(fmt::format("{} read start, pc: {}", getName(), payload.ins_info->ins.pc));

        int address_byte = payload.ins_info->src_start_address_byte;
        int size_byte = payload.ins_info->data_size_byte;
        if (auto type = payload.ins_info->type; type == +TransferType::receive) {
            transmit_socket_.receiveData(payload.ins_info->ins, payload.ins_info->src_id);
        } else if (type == +TransferType::global_load) {
            transmit_socket_.loadGlobal(payload.ins_info->ins, address_byte, size_byte);
        } else {
            memory_socket_.readLocal(payload.ins_info->ins, address_byte, size_byte);
        }

        waitAndStartNextStage(payload, write_stage_socket_);

        read_stage_socket_.finish();
    }
}

void GlobalTransferDataPath::processWriteStage() {
    while (true) {
        write_stage_socket_.waitUntilStart();

        const auto& payload = write_stage_socket_.payload;
        CORE_LOG(fmt::format("{} write start, pc: {}", getName(), payload.ins_info->ins.pc));

        transfer_unit_.releaseResource(payload.ins_info->ins.ins_id);

        int address_byte = payload.ins_info->dst_start_address_byte;
        int size_byte = payload.ins_info->data_size_byte;
        if (auto type = payload.ins_info->type; type == +TransferType::send) {
            transmit_socket_.sendData(payload.ins_info->ins, payload.ins_info->dst_id,
                                      payload.ins_info->transfer_id_tag, address_byte, size_byte);
        } else if (type == +TransferType::global_store) {
            transmit_socket_.storeGlobal(payload.ins_info->ins, address_byte, size_byte, {});
        } else {
            memory_socket_.writeLocal(payload.ins_info->ins, address_byte, size_byte, {});
        }

        CORE_LOG(fmt::format("{} write end, pc: {}", getName(), payload.ins_info->ins.pc));

        transfer_unit_.finishInstruction();

        write_stage_socket_.finish();
    }
}

void GlobalTransferDataPath::bindLocalMemoryUnit(MemoryUnit* local_memory_unit) {
    memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void GlobalTransferDataPath::bindSwitchAndGlobalMemory(Switch* switch_, int global_memory_switch_id) {
    transmit_socket_.bindSwitchAndGlobalMemory(switch_, core_id_, global_memory_switch_id);
}

TransferUnit::TransferUnit(const sc_module_name& name, const TransferUnitConfig& config, const BaseInfo& base_info,
                           Clock* clk, int global_memory_switch_id)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::transfer)
    , config_(config)
    , intra_core_bus_("IntraCoreBus", base_info, *this, config_.pipeline)
    , inter_core_bus_("InterCoreBus", base_info, *this)
    , global_memory_switch_id_(global_memory_switch_id) {
    SC_THREAD(processIssue)

    for (auto& local_dedicated_data_path_cfg : config_.local_dedicated_data_path_list) {
        auto data_path_name = fmt::format("LocalDedicatedDataPath_{}", local_dedicated_data_path_cfg.id);
        auto data_path_ptr =
            std::make_shared<LocalTransferDataPath>(data_path_name.c_str(), base_info, *this, config_.pipeline);
        local_dedicated_data_path_map_.emplace(local_dedicated_data_path_cfg.id, data_path_ptr);
    }
}

void TransferUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<TransferInsPayload>();
        auto& data_path_payload = payload->data_path_payload;

        ports_.resource_allocate_.write(getDataConflictInfo(*payload));
        if (data_path_payload.type == +DataPathType::intra_core_bus) {
            auto local_trans_payload = decodeLocalTransferInfo(*payload);
            waitAndStartNextStage(local_trans_payload, intra_core_bus_.exec_socket_);
        } else if (data_path_payload.type == +DataPathType::inter_core_bus) {
            auto global_trans_payload = decodeGlobalTransferInfo(*payload);
            waitAndStartNextStage(global_trans_payload, inter_core_bus_.exec_socket_);
        } else if (data_path_payload.type == +DataPathType::local_dedicated_data_path) {
            auto local_trans_payload = decodeLocalTransferInfo(*payload);
            auto data_path_ptr = local_dedicated_data_path_map_[data_path_payload.local_dedicated_data_path_id];
            waitAndStartNextStage(local_trans_payload, data_path_ptr->exec_socket_);
        }

        readyForNextExecute();
    }
}

void TransferUnit::bindSwitch(Switch* switch_) {
    inter_core_bus_.bindSwitchAndGlobalMemory(switch_, global_memory_switch_id_);
}

void TransferUnit::bindLocalMemoryUnit(MemoryUnit* local_memory_unit) {
    intra_core_bus_.bindLocalMemoryUnit(local_memory_unit);
    inter_core_bus_.bindLocalMemoryUnit(local_memory_unit);
    for (auto& [id, data_path_ptr] : local_dedicated_data_path_map_) {
        data_path_ptr->bindLocalMemoryUnit(local_memory_unit);
    }
    this->memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

LocalTransferDataPathPayload TransferUnit::decodeLocalTransferInfo(const TransferInsPayload& payload) const {
    LocalTransferDataPathPayload local_transfer_payload;
    int src_memory_id = as_.getLocalMemoryId(payload.src_address_byte);
    int dst_memory_id = as_.getLocalMemoryId(payload.dst_address_byte);

    int data_width_byte = std::max(memory_socket_.getLocalMemoryDataWidthById(src_memory_id, MemoryAccessType::read),
                                   memory_socket_.getLocalMemoryDataWidthById(dst_memory_id, MemoryAccessType::write));
    bool use_pipeline = config_.pipeline && (src_memory_id != dst_memory_id);

    local_transfer_payload.ins_info =
        std::make_shared<LocalTransferInsInfo>(LocalTransferInsInfo{.ins = payload.ins,
                                                                    .src_start_address_byte = payload.src_address_byte,
                                                                    .dst_start_address_byte = payload.dst_address_byte,
                                                                    .batch_max_data_size_byte = data_width_byte,
                                                                    .use_pipeline = use_pipeline});
    local_transfer_payload.data_size_byte = payload.size_byte;
    local_transfer_payload.process_times =
        IntDivCeil(payload.size_byte, local_transfer_payload.ins_info->batch_max_data_size_byte);

    return std::move(local_transfer_payload);
}

GlobalTransferDataPathPayload TransferUnit::decodeGlobalTransferInfo(const TransferInsPayload& payload) {
    return {.ins_info = std::make_shared<GlobalTransferInsInfo>(
                GlobalTransferInsInfo{.ins = payload.ins,
                                      .type = payload.type,
                                      .src_start_address_byte = payload.src_address_byte,
                                      .dst_start_address_byte = payload.dst_address_byte,
                                      .data_size_byte = payload.size_byte,
                                      .src_id = payload.src_id,
                                      .dst_id = payload.dst_id,
                                      .transfer_id_tag = payload.transfer_id_tag})};
}

ResourceAllocatePayload TransferUnit::getDataConflictInfo(const TransferInsPayload& payload) const {
    ResourceAllocatePayload conflict_payload{.ins_id = payload.ins.ins_id,
                                             .unit_type = ExecuteUnitType::transfer,
                                             .data_path_payload = payload.data_path_payload};
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
