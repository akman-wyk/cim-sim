//
// Created by wyk on 2025/3/16.
//

#pragma once
#include <unordered_map>

#include "core/payload.h"
#include "network/payload.h"
#include "systemc.h"

namespace cimsim {

class Switch;

class TransmitSocket {
public:
    TransmitSocket() = default;

    void bindSwitchAndGlobalMemory(Switch* _switch, int core_id, int global_memory_switch_id);

    void switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload);

    std::vector<uint8_t> loadGlobal(const InstructionPayload& ins, int address_byte, int size_byte);
    void storeGlobal(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data);

    void sendHandshake(const InstructionPayload& ins, int dst_id, int transfer_id_tag);
    void sendData(const InstructionPayload& ins, int dst_id, int transfer_id_tag, int dst_address_byte,
                  int data_size_byte);
    void receiveHandshake(int src_id, int transfer_id_tag);
    void receiveData(const InstructionPayload& ins, int src_id);

private:
    Switch* switch_{nullptr};
    int core_id_{0};
    int global_memory_switch_id_{-1};

    // load and store global memory
    sc_event finish_load_;
    sc_event finish_store_;
    sc_event finish_load_trans_;
    sc_event finish_store_trans_;

    // send and receive
    int expected_receiver_core_id_{-1};  // -1 for not ready, same below
    int expected_sender_core_id_{-1};    // execute recv and recv sender_core_id's send inst
    int expected_transfer_id_tag_{-1};

    sc_event sender_wait_receiver_ready_;
    sc_event receiver_wait_sender_ready_;
    sc_event receiver_wait_data_ready_;

    std::unordered_map<int, int> receiver_waiting_sender_map;  // <core_id_, transfer_id_tag>
};

}  // namespace cimsim
