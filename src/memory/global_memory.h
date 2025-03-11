//
// Created by wyk on 2024/11/11.
//

#pragma once
#include "memory.h"
#include "network/switch.h"

namespace cimsim {

class GlobalMemory {
public:
    GlobalMemory(const char* name, const GlobalMemoryConfig& config, const SimConfig& sim_config, Clock* clk);

    EnergyReporter getEnergyReporter();

    void bindNetwork(Network* network);

private:
    void switchReceiveHandler(const std::shared_ptr<NetworkPayload>& payload);

private:
    Memory memory_;
    Switch switch_;
};

}  // namespace cimsim
