//
// Created by wyk on 2024/11/7.
//

#pragma once
#include <queue>

#include "network.h"
#include "payload.h"
#include "base_component/base_module.h"

namespace pimsim {

class SwitchSocket;

class Switch : public BaseModule {
    SC_HAS_PROCESS(Switch);

public:
    Switch(const char* name, const SimConfig& sim_config, Core* core, Clock* clk, int core_id);

    [[noreturn]] void processTransport();

    // two mode :
    // transport mode not only sends to dst,but also requires response from dst
    // send mode just sends data to dst without demands of response
    void transportHandler(const std::shared_ptr<NetworkPayload>& payload);
    void sendHandler(const std::shared_ptr<NetworkPayload>& payload);

    void registerReceiveHandler(const std::function<void(const std::shared_ptr<NetworkPayload>&)>& reveive_handler);
    void receiveHandler(const std::shared_ptr<NetworkPayload>& payload);  // when recv data from network,call this

    void bindNetwork(Network* network);

private:
    sc_core::sc_event trigger_;

    std::queue<std::pair<std::shared_ptr<NetworkPayload>, NetworkTransferMode>> pending_queue_;
    std::function<void(const std::shared_ptr<NetworkPayload>&)> receive_handler_;

    int core_id_;
    Network* network_{nullptr};
};

}  // namespace pimsim
