//
// Created by wyk on 2024/11/11.
//

#include "global_memory.h"

namespace cimsim {

GlobalMemory::GlobalMemory(const char* name, const GlobalMemoryConfig& config, const SimConfig& sim_config, Clock* clk)
    : memory_(name, config.hardware_config, config.addressing, sim_config, nullptr, clk)
    , switch_("GlobalMemoryConfig", sim_config, nullptr, clk, config.global_memory_switch_id) {
    switch_.registerReceiveHandler(
        [this](const std::shared_ptr<NetworkPayload>& payload) { this->switchReceiveHandler(payload); });
}

void GlobalMemory::bindNetwork(Network* network) {
    switch_.bindNetwork(network);
}

EnergyReporter GlobalMemory::getEnergyReporter() {
    return memory_.getEnergyReporter();
}

void GlobalMemory::switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload) {
    auto global_trans = payload->getRequestPayload<MemoryAccessPayload>();
    memory_.access(global_trans);
    wait(global_trans->finish_access);
}

}  // namespace cimsim
