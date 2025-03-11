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
    sc_core::sc_event start_exec;
    sc_core::sc_event finish_exec;

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

}  // namespace cimsim
