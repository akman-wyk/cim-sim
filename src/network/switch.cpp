//
// Created by wyk on 2024/11/7.
//

#include "switch.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

Switch::Switch(const sc_module_name& name, const BaseInfo& base_info) : BaseModule(name, base_info) {
    SC_THREAD(processTransport);
}

void Switch::processTransport() {
    while (true) {
        while (pending_queue_.empty()) {
            wait(trigger_);
        }

        auto payload = pending_queue_.front().first;
        auto mode = pending_queue_.front().second;
        pending_queue_.pop();

        CORE_LOG(fmt::format("mode: {}, src: {}, dst: {}, req size: {}, rsp size: {}", mode._to_string(),
                             payload->src_id, payload->dst_id, payload->request_data_size_byte,
                             payload->response_data_size_byte));

        ProfilerTag profiler_tag = {.core_id = core_id_,
                                    .ins_id = payload->ins.ins_id,
                                    .inst_opcode = payload->ins.inst_opcode,
                                    .inst_group_tag = payload->ins.inst_group_tag,
                                    .inst_profiler_operator = "transport"};

        auto send_delay = network_->transferAndGetDelay(payload->src_id, payload->dst_id,
                                                        payload->request_data_size_byte, profiler_tag);
        wait(send_delay);

        auto target_switch = network_->getSwitch(payload->dst_id);
        target_switch->receiveHandler(payload);
        if (mode == +NetworkTransferMode::transport) {
            auto receive_delay = network_->transferAndGetDelay(payload->dst_id, payload->src_id,
                                                               payload->response_data_size_byte, profiler_tag);
            wait(receive_delay);
        }

        if (payload->finish_network_trans != nullptr) {
            payload->finish_network_trans->notify(SC_ZERO_TIME);
        }
    }
}

void Switch::transportHandler(const std::shared_ptr<NetworkPayload>& payload) {
    pending_queue_.emplace(payload, NetworkTransferMode::transport);
    trigger_.notify();
}

void Switch::sendHandler(const std::shared_ptr<NetworkPayload>& payload) {
    pending_queue_.emplace(payload, NetworkTransferMode::only_send);
    trigger_.notify();
}

void Switch::registerReceiveHandler(
    const std::function<void(const std::shared_ptr<NetworkPayload>&)>& reveive_handler) {
    receive_handler_ = reveive_handler;
}

void Switch::receiveHandler(const std::shared_ptr<NetworkPayload>& payload) {
    receive_handler_(payload);
}

void Switch::bindNetwork(Network* network) {
    network_ = network;
    network_->registerSwitch(core_id_, this);
}

}  // namespace cimsim
