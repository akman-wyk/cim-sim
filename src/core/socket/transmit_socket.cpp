//
// Created by wyk on 2025/3/16.
//

#include "transmit_socket.h"

#include "fmt/format.h"
#include "memory/payload.h"
#include "network/switch.h"
#include "util/log.h"

namespace cimsim {

void TransmitSocket::bindSwitchAndGlobalMemory(Switch* _switch, int core_id, int global_memory_switch_id) {
    switch_ = _switch;
    switch_->registerReceiveHandler(
        [this](const std::shared_ptr<NetworkPayload>& payload) { this->switchReceiveHandler(payload); });

    core_id_ = core_id;
    global_memory_switch_id_ = global_memory_switch_id;
}

std::vector<uint8_t> TransmitSocket::loadGlobal(const InstructionPayload& ins, int address_byte, int size_byte) {
    LOG(fmt::format("core id: {}, load global data start, pc: {}", core_id_, ins.pc));
    auto global_payload =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::read,
                                                                  .address_byte = address_byte,
                                                                  .size_byte = size_byte,
                                                                  .finish_access = finish_load_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .finish_network_trans = &finish_load_trans_,
                                                                           .request_data_size_byte = 1,
                                                                           .request_payload = global_payload,
                                                                           .response_data_size_byte = size_byte,
                                                                           .response_payload = nullptr});
    switch_->transportHandler(network_payload);
    wait(*network_payload->finish_network_trans);

    LOG(fmt::format("core id: {}, load global data end, pc: {}", core_id_, ins.pc));
    return std::move(global_payload->data);
}

void TransmitSocket::storeGlobal(const InstructionPayload& ins, int address_byte, int size_byte,
                                 std::vector<uint8_t> data) {
    LOG(fmt::format("core id: {}, store global data start, pc: {}", core_id_, ins.pc));
    auto global_payload =
        std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                  .access_type = MemoryAccessType::write,
                                                                  .address_byte = address_byte,
                                                                  .size_byte = size_byte,
                                                                  .data = std::move(data),
                                                                  .finish_access = finish_store_});
    auto network_payload = std::make_shared<NetworkPayload>(NetworkPayload{.src_id = core_id_,
                                                                           .dst_id = global_memory_switch_id_,
                                                                           .finish_network_trans = &finish_store_trans_,
                                                                           .request_data_size_byte = size_byte,
                                                                           .request_payload = global_payload,
                                                                           .response_data_size_byte = 1,
                                                                           .response_payload = nullptr});
    switch_->transportHandler(network_payload);
    wait(*network_payload->finish_network_trans);
    LOG(fmt::format("core id: {}, store global data end, pc: {}", core_id_, ins.pc));
}

void TransmitSocket::sendHandshake(int dst_id, int transfer_id_tag) {
    expected_receiver_core_id_ = dst_id;
    expected_transfer_id_tag_ = transfer_id_tag;
    LOG(fmt::format("core id: {}, send handshake start, dst_id: {}, transfer_id_tag: {}", core_id_, dst_id,
                    transfer_id_tag));

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
    switch_->sendHandler(network_payload);
    wait(sender_wait_receiver_ready_);

    LOG(fmt::format("core id: {}, send handshake end, dst_id: {}, transfer_id_tag: {}", core_id_, dst_id,
                    transfer_id_tag));
    expected_receiver_core_id_ = -1;
    expected_transfer_id_tag_ = -1;
}

void TransmitSocket::sendData(int dst_id, int transfer_id_tag, int dst_address_byte, int data_size_byte) {
    LOG(fmt::format("core id: {}, send data start, dst_id: {}, transfer_id_tag: {}", core_id_, dst_id,
                    transfer_id_tag));
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
    switch_->sendHandler(network_payload);
    LOG(fmt::format("core id: {}, send data end, dst_id: {}, transfer_id_tag: {}", core_id_, dst_id, transfer_id_tag));
}

void TransmitSocket::receiveHandshake(int src_id, int transfer_id_tag) {
    expected_sender_core_id_ = src_id;
    LOG(fmt::format("core id: {}, receive handshake start, src_id: {}, transfer_id_tag: {}", core_id_, src_id,
                    transfer_id_tag));

    if (auto found = receiver_waiting_sender_map.find(src_id);
        found == receiver_waiting_sender_map.end() || found->second != transfer_id_tag) {
        wait(receiver_wait_sender_ready_);
    }

    expected_sender_core_id_ = -1;
}

void TransmitSocket::receiveData(int src_id) {
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
    switch_->sendHandler(network_payload);
    LOG(fmt::format("core id: {}, receive handshake end, src_id: {}, transfer_id_tag: {}", core_id_, src_id,
                    transfer_id_tag));

    LOG(fmt::format("core id: {}, receive data start, src_id: {}, transfer_id_tag: {}", core_id_, src_id,
                    receiver_waiting_sender_map[src_id]));
    wait(receiver_wait_data_ready_);
    LOG(fmt::format("core id: {}, receive data end, src_id: {}, transfer_id_tag: {}", core_id_, src_id,
                    transfer_id_tag));
}

void TransmitSocket::switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload) {
    auto data_transfer_payload = payload->getRequestPayload<DataTransferInfo>();
    auto remote_is_sender = data_transfer_payload->is_sender;

    LOG(fmt::format("core id: {}, receive network message from {}, remote is {}, status: {}, sender_core_id: {}, "
                    "expected_sender_core_id: {}",
                    core_id_, payload->src_id, (remote_is_sender ? "sender" : "receiver"),
                    data_transfer_payload->status._to_string(), data_transfer_payload->sender_id,
                    expected_sender_core_id_));

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

}  // namespace cimsim
