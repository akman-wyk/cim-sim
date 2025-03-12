//
// Created by wyk on 2024/7/15.
//

#include "transfer_unit.h"

#include "fmt/format.h"
#include "network/switch.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

TransferUnit::TransferUnit(const char* name, const TransferUnitConfig& config, const SimConfig& sim_config, Core* core,
                           Clock* clk, int core_id, int global_memory_switch_id)
    : ExecuteUnit(name, sim_config, core, clk, ExecuteUnitType::transfer)
    , config_(config)
    , core_id_(core_id)
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
            processSendHandshake(payload->dst_id, payload->transfer_id_tag);
        } else if (payload->type == +TransferType::receive) {
            processReceiveHandshake(payload->src_id, payload->transfer_id_tag);
        }

        int process_times = IntDivCeil(payload->size_byte, ins_info.batch_max_data_size_byte);
        TransferSubmodulePayload submodule_payload{.ins_info = ins_info};
        for (int batch = 0; batch < process_times; batch++) {
            submodule_payload.batch_info = {
                .batch_num = batch,
                .batch_data_size_byte = (batch == process_times - 1)
                                            ? payload->size_byte - batch * ins_info.batch_max_data_size_byte
                                            : ins_info.batch_max_data_size_byte,
                .first_batch = (batch == 0),
                .last_batch = (batch == process_times - 1)};
            waitAndStartNextSubmodule(submodule_payload, read_submodule_socket_);

            if (!submodule_payload.batch_info.last_batch) {
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
        LOG(fmt::format("transfer read start, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        int address_byte = payload.ins_info.src_start_address_byte +
                           payload.batch_info.batch_num * payload.ins_info.batch_max_data_size_byte;
        int size_byte = payload.batch_info.batch_data_size_byte;
        if (auto type = payload.ins_info.type; type == +TransferType::receive) {
            processReceiveData(payload.ins_info.src_id);
        } else if (type == +TransferType::global_load) {
            processLoadGlobalData(payload.ins_info.ins, address_byte, size_byte);
        } else {
            payload.batch_info.data = local_memory_socket_.readData(payload.ins_info.ins, address_byte, size_byte);
        }

        waitAndStartNextSubmodule(payload, write_submodule_socket_);

        if (!payload.batch_info.last_batch && payload.ins_info.use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        read_submodule_socket_.finish();
    }
}

void TransferUnit::processWriteSubmodule() {
    while (true) {
        write_submodule_socket_.waitUntilStart();

        const auto& payload = write_submodule_socket_.payload;
        LOG(fmt::format("transfer write start, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        if (payload.batch_info.last_batch) {
            releaseResource(payload.ins_info.ins.ins_id);
        }

        int address_byte = payload.ins_info.dst_start_address_byte +
                           payload.batch_info.batch_num * payload.ins_info.batch_max_data_size_byte;
        int size_byte = payload.batch_info.batch_data_size_byte;
        if (auto type = payload.ins_info.type; type == +TransferType::send) {
            processSendData(payload.ins_info.dst_id, payload.ins_info.transfer_id_tag, address_byte, size_byte);
        } else if (type == +TransferType::global_store) {
            processStoreGlobalData(payload.ins_info.ins, address_byte, size_byte);
        } else {
            local_memory_socket_.writeData(payload.ins_info.ins, address_byte, size_byte, payload.batch_info.data);
        }

        LOG(fmt::format("transfer write end, pc: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.batch_info.batch_num));

        if (!payload.batch_info.last_batch && !payload.ins_info.use_pipeline) {
            cur_ins_next_batch_.notify();
        }

        if (payload.batch_info.last_batch) {
            finishInstruction();
        }

        write_submodule_socket_.finish();
    }
}

void TransferUnit::bindLocalMemoryUnit(MemoryUnit* local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void TransferUnit::bindSwitch(Switch* switch_) {
    switch_socket_.bindSwitch(switch_);
    switch_->registerReceiveHandler(
        [this](const std::shared_ptr<NetworkPayload>& payload) { this->switchReceiveHandler(payload); });
}

void TransferUnit::waitAndStartNextSubmodule(cimsim::TransferSubmodulePayload& cur_payload,
                                             SubmoduleSocket<cimsim::TransferSubmodulePayload>& next_submodule_socket) {
    next_submodule_socket.waitUntilFinishIfBusy();
    if (cur_payload.batch_info.first_batch) {
        next_submodule_socket.payload.ins_info = cur_payload.ins_info;
    }
    next_submodule_socket.payload.batch_info = std::move(cur_payload.batch_info);
    next_submodule_socket.start_exec.notify();
}

std::pair<TransferInstructionInfo, ResourceAllocatePayload> TransferUnit::decodeAndGetInfo(
    const TransferInsPayload& payload) const {
    TransferInstructionInfo ins_info;
    if (payload.type == +TransferType::local_trans) {
        int src_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.src_address_byte);
        int dst_memory_id = local_memory_socket_.getLocalMemoryIdByAddress(payload.dst_address_byte);

        int data_width_byte =
            std::max(local_memory_socket_.getMemoryDataWidthById(src_memory_id, MemoryAccessType::read),
                     local_memory_socket_.getMemoryDataWidthById(dst_memory_id, MemoryAccessType::write));
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
        conflict_payload.addReadMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.src_address_byte));
    }
    if (payload.type == +TransferType::local_trans || payload.type == +TransferType::receive ||
        payload.type == +TransferType::global_load) {
        conflict_payload.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.dst_address_byte));
    }
    return std::move(conflict_payload);
}

ResourceAllocatePayload TransferUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<TransferInsPayload>(payload));
}

void TransferUnit::switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload) {
    auto data_transfer_payload = payload->getRequestPayload<DataTransferInfo>();
    auto remote_is_sender = data_transfer_payload->is_sender;

    LOG(fmt::format(
        "receive network message from {}, remote is {}, status: {}, sender_core_id: {}, expected_sender_core_id: {}",
        payload->src_id, (remote_is_sender ? "sender" : "receiver"), data_transfer_payload->status._to_string(),
        data_transfer_payload->sender_id, expected_sender_core_id_));

    if (remote_is_sender) {
        // remote core execute send inst to this core
        auto status = data_transfer_payload->status;
        auto sender_core_id = data_transfer_payload->sender_id;
        if (status == +DataTransferStatus::sender_ready) {
            // remote core send handshake
            receiver_waiting_sender_map[sender_core_id] = data_transfer_payload->id_tag;
            if (sender_core_id == expected_sender_core_id_) {
                // this core already execute recv and is waiting for remote core
                receiver_wait_sender_ready_.notify(SC_ZERO_TIME);
            }
        } else if (status == +DataTransferStatus::send_data) {
            receiver_wait_data_ready_.notify(SC_ZERO_TIME);
        }
    } else {
        // remote core recv this core
        auto receiver_core_id = data_transfer_payload->receiver_id;
        if (receiver_core_id == expected_receiver_core_id_ &&
            data_transfer_payload->id_tag == expected_transfer_id_tag_ &&
            data_transfer_payload->status == +DataTransferStatus::receiver_ready) {
            sender_wait_receiver_ready_.notify(SC_ZERO_TIME);
        } else {
            std::cerr << fmt::format("TransferUnit: send-recv not match") << std::endl;
        }
    }
}

void TransferUnit::processSendHandshake(int dst_id, int transfer_id_tag) {
    expected_receiver_core_id_ = dst_id;
    expected_transfer_id_tag_ = transfer_id_tag;
    LOG(fmt::format("send handshake start, dst_id: {}, transfer_id_tag: {}", dst_id, transfer_id_tag));

    auto request = std::make_shared<DataTransferInfo>(DataTransferInfo{.sender_id = core_id_,
                                                                       .receiver_id = dst_id,
                                                                       .is_sender = true,
                                                                       .status = DataTransferStatus::sender_ready,
                                                                       .id_tag = transfer_id_tag,
                                                                       .data_size_byte = 0});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = dst_id,
                                                                           .request_data_size_byte = 1,
                                                                           .request_payload = request,
                                                                           .response_data_size_byte = 1,
                                                                           .response_payload = nullptr});
    switch_socket_.send_message(network_payload);
    wait(sender_wait_receiver_ready_);

    LOG(fmt::format("send handshake end, dst_id: {}, transfer_id_tag: {}", dst_id, transfer_id_tag));
    expected_receiver_core_id_ = -1;
    expected_transfer_id_tag_ = -1;
}

void TransferUnit::processSendData(int dst_id, int transfer_id_tag, int dst_address_byte, int data_size_byte) {
    LOG(fmt::format("send data start, dst_id: {}, transfer_id_tag: {}", dst_id, transfer_id_tag));
    auto resuest = std::make_shared<DataTransferInfo>(DataTransferInfo{.sender_id = core_id_,
                                                                       .receiver_id = dst_id,
                                                                       .is_sender = true,
                                                                       .status = DataTransferStatus::send_data,
                                                                       .id_tag = transfer_id_tag,
                                                                       .data_size_byte = data_size_byte});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = dst_id,
                                                                           .request_data_size_byte = data_size_byte,
                                                                           .request_payload = resuest,
                                                                           .response_data_size_byte = 0,
                                                                           .response_payload = nullptr});
    switch_socket_.send_message(network_payload);
    LOG(fmt::format("send data end, dst_id: {}, transfer_id_tag: {}", dst_id, transfer_id_tag));
}

void TransferUnit::processReceiveHandshake(int src_id, int transfer_id_tag) {
    expected_sender_core_id_ = src_id;
    LOG(fmt::format("receive handshake start, src_id: {}, transfer_id_tag: {}", src_id, transfer_id_tag));

    if (auto found = receiver_waiting_sender_map.find(src_id);
        found == receiver_waiting_sender_map.end() || found->second != transfer_id_tag) {
        wait(receiver_wait_sender_ready_);
    }

    expected_sender_core_id_ = -1;
}

void TransferUnit::processReceiveData(int src_id) {
    auto data_transfer_response =
        std::make_shared<DataTransferInfo>(DataTransferInfo{.sender_id = src_id,
                                                            .receiver_id = core_id_,
                                                            .is_sender = false,
                                                            .status = DataTransferStatus::receiver_ready,
                                                            .id_tag = receiver_waiting_sender_map[src_id],
                                                            .data_size_byte = 0});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = src_id,
                                                                           .request_data_size_byte = 1,
                                                                           .request_payload = data_transfer_response,
                                                                           .response_data_size_byte = 0,
                                                                           .response_payload = nullptr});
    int transfer_id_tag = receiver_waiting_sender_map[src_id];
    receiver_waiting_sender_map[src_id] = -1;
    switch_socket_.send_message(network_payload);
    LOG(fmt::format("receive handshake end, src_id: {}, transfer_id_tag: {}", src_id, transfer_id_tag));

    LOG(fmt::format("receive data start, src_id: {}, transfer_id_tag: {}", src_id,
                    receiver_waiting_sender_map[src_id]));
    wait(receiver_wait_data_ready_);
    LOG(fmt::format("receive data end, src_id: {}, transfer_id_tag: {}", src_id, transfer_id_tag));
}

void TransferUnit::processLoadGlobalData(const InstructionPayload& ins, int src_address_byte, int data_size_byte) {
    LOG(fmt::format("load global data start, pc: {}", ins.pc));
    auto global_trans =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::read,
                                                                  .address_byte = src_address_byte,
                                                                  .size_byte = data_size_byte,
                                                                  .finish_access = finish_read_global_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .request_data_size_byte = 1,
                                                                           .request_payload = global_trans,
                                                                           .response_data_size_byte = data_size_byte,
                                                                           .response_payload = nullptr});
    switch_socket_.load(network_payload);
    LOG(fmt::format("load global data end, pc: {}", ins.pc));
}

void TransferUnit::processStoreGlobalData(const InstructionPayload& ins, int dst_address_byte, int data_size_byte) {
    LOG(fmt::format("store global data start, pc: {}", ins.pc));
    auto global_trans =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::write,
                                                                  .address_byte = dst_address_byte,
                                                                  .size_byte = data_size_byte,
                                                                  .finish_access = finish_write_global_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .request_data_size_byte = data_size_byte,
                                                                           .request_payload = global_trans,
                                                                           .response_data_size_byte = 1,
                                                                           .response_payload = nullptr});
    switch_socket_.store(network_payload);
    LOG(fmt::format("store global data end, pc: {}", ins.pc));
}

}  // namespace cimsim
