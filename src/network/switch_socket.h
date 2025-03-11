//
// Created by wyk on 2024/11/7.
//

#pragma once
#include <string>

#include "switch.h"
#include "systemc.h"

namespace cimsim {

class SwitchSocket {
public:
    SwitchSocket() = default;

    void bindSwitch(Switch* _switch_);

    // two mode:
    // load and store are transport mode sends data and requires response, block (use event_ptr)
    // send_message is only send mode just sends data and non-block
    void load(const std::shared_ptr<NetworkPayload>& payload);
    void store(const std::shared_ptr<NetworkPayload>& payload);
    void send_message(const std::shared_ptr<NetworkPayload>& payload);

private:
    sc_core::sc_event finish_load_;
    sc_core::sc_event finish_store_;
    Switch* switch_{nullptr};
};

}  // namespace cimsim
