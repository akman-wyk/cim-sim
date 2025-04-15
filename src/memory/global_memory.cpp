//
// Created by wyk on 2024/11/11.
//

#include "global_memory.h"

namespace cimsim {

GlobalMemory::GlobalMemory(const sc_module_name& name, const GlobalMemoryConfig& config, const SimConfig& sim_config)
    : BaseModule(name, BaseInfo{.sim_config = sim_config})
    , memory_unit_("MemoryUnit", config.global_memory_unit_config, BaseInfo{sim_config, config.global_memory_switch_id},
                   true)
    , switch_("Switch", BaseInfo{sim_config, config.global_memory_switch_id}) {
    switch_.registerReceiveHandler([this](const std::shared_ptr<NetworkPayload>& payload) {
        memory_unit_.access(payload->getRequestPayload<MemoryAccessPayload>());
    });
}

void GlobalMemory::bindNetwork(Network* network) {
    switch_.bindNetwork(network);
}

EnergyCounter* GlobalMemory::getEnergyCounterPtr() {
    return memory_unit_.getEnergyCounterPtr();
}

}  // namespace cimsim
