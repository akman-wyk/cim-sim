//
// Created by wyk on 2025/3/16.
//

#pragma once
#include <unordered_map>

#include "network/payload.h"
#include "systemc.h"

namespace cimsim {

class Switch;

class TransmitSocket {
public:
    TransmitSocket() = default;

    ~TransmitSocket();

    void bindSwitch(Switch* _switch, int core_id);

    void switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload);

    void sendHandshake(int dst_id, int transfer_id_tag);
    void sendData(int dst_id, int transfer_id_tag, int dst_address_byte, int data_size_byte);
    void receiveHandshake(int src_id, int transfer_id_tag);
    void receiveData(int src_id);

private:
    Switch* switch_{nullptr};
    int core_id_{0};

    // -1 for not ready
    int expected_receiver_core_id_{-1};
    int expected_sender_core_id_{-1};  // execute recv and recv sender_core_id's send inst
    int expected_transfer_id_tag_{-1};

    sc_event* sender_wait_receiver_ready_{nullptr};
    sc_event* receiver_wait_sender_ready_{nullptr};
    sc_event* receiver_wait_data_ready_{nullptr};

    std::unordered_map<int, int> receiver_waiting_sender_map;  // <core_id_, transfer_id_tag>
};

}  // namespace cimsim
