//
// Created by wyk on 2024/11/11.
//

#pragma once
#include "base_component/base_module.h"
#include "config/config.h"
#include "memory_unit.h"
#include "network/network.h"
#include "network/switch.h"

namespace cimsim {

class GlobalMemory : public BaseModule {
public:
    GlobalMemory(const sc_module_name& name, const GlobalMemoryConfig& config, const SimConfig& sim_config);

    EnergyReporter getEnergyReporter() override;

    void bindNetwork(Network* network);

private:
    MemoryUnit memory_unit_;
    Switch switch_;
};

}  // namespace cimsim
