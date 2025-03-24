//
// Created by wyk on 2024/7/5.
//

#pragma once
#include "systemc.h"

namespace cimsim {

template <class PayloadType>
struct SubmoduleSocket {
    PayloadType payload;
    bool busy{false};
    sc_event start_exec;
    sc_event finish_exec;

    void waitUntilStart() {
        wait(start_exec);
        busy = true;
    }

    void waitUntilFinishIfBusy() const {
        if (busy) {
            wait(finish_exec);
        }
    }

    void finish() {
        busy = false;
        finish_exec.notify();
    }
};

template <class PayloadType>
void waitAndStartNextStage(const PayloadType& cur_payload, SubmoduleSocket<PayloadType>& next_stage_socket) {
    next_stage_socket.waitUntilFinishIfBusy();
    next_stage_socket.payload = cur_payload;
    next_stage_socket.start_exec.notify();
}

}  // namespace cimsim
