//
// Created by wyk on 2024/11/11.
//

#include "global_memory.h"

namespace cimsim {

GlobalMemory::GlobalMemory(const char* name, const GlobalMemoryConfig& config, const SimConfig& sim_config, Clock* clk)
    : memory_unit_(name, config.global_memory_unit_config, sim_config, nullptr, clk, true)
    , switch_("GlobalMemoryConfig", sim_config, nullptr, clk, config.global_memory_switch_id) {
    switch_.registerReceiveHandler([this](const std::shared_ptr<NetworkPayload>& payload) {
        memory_unit_.access(payload->getRequestPayload<MemoryAccessPayload>());
    });
}

void GlobalMemory::bindNetwork(Network* network) {
    switch_.bindNetwork(network);
}

EnergyReporter GlobalMemory::getEnergyReporter() {
    return memory_unit_.getEnergyReporter();
}

}  // namespace cimsim
