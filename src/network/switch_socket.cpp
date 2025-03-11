//
// Created by wyk on 2024/11/7.
//

#include "switch_socket.h"

namespace cimsim {

void SwitchSocket::bindSwitch(Switch* _switch_) {
    this->switch_ = _switch_;
}

void SwitchSocket::load(const std::shared_ptr<NetworkPayload>& payload) {
    payload->finish_network_trans = &finish_load_;
    switch_->transportHandler(payload);
    wait(finish_load_);
}

void SwitchSocket::store(const std::shared_ptr<NetworkPayload>& payload) {
    payload->finish_network_trans = &finish_store_;
    switch_->transportHandler(payload);
    wait(finish_store_);
}

void SwitchSocket::send_message(const std::shared_ptr<NetworkPayload>& payload) {
    switch_->sendHandler(payload);
}

}  // namespace cimsim
