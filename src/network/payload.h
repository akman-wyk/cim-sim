//
// Created by wyk on 2024/11/7.
//

#pragma once
#include <memory>

#include "better-enums/enum.h"
#include "systemc.h"

namespace cimsim {

BETTER_ENUM(DataTransferStatus, int,  // NOLINT(*-explicit-constructor)
            sender_ready, receiver_ready, send_data)

BETTER_ENUM(NetworkTransferMode, int,  // NOLINT(*-explicit-constructor)
            transport, only_send)

struct NetworkPayload {
    int src_id;
    int dst_id;

    sc_event* finish_network_trans{nullptr};

    // one payload contains request and its response(optional)
    int request_data_size_byte;
    std::shared_ptr<void> request_payload;

    int response_data_size_byte;
    std::shared_ptr<void> response_payload;

    template <typename T>
    std::shared_ptr<T> getRequestPayload() {
        return std::static_pointer_cast<T>(request_payload);
    }

    template <typename T>
    std::shared_ptr<T> getResponsePayload() {
        return std::static_pointer_cast<T>(response_payload);
    }
};

struct DataTransferInfo {
    int sender_id;
    int receiver_id;

    bool is_sender;
    DataTransferStatus status;

    int id_tag;
    int data_size_byte;
};

}  // namespace cimsim
