//
// Created by wyk on 2024/11/7.
//

#pragma once

#include <string>
#include <unordered_map>

#include "base_component/energy_counter.h"
#include "config/config.h"
#include "nlohmann/json.hpp"
#include "util/reporter.h"

namespace cimsim {

class Switch;

class Network {
public:
    Network(std::string name, const NetworkConfig& config, const SimConfig& sim_config);

    sc_time transferAndGetDelay(int src_id, int dst_id, int data_size_byte);

    Switch* getSwitch(int id);
    void registerSwitch(int id, Switch* switch_ptr);

    void readLatencyEnergyFile(const std::string& file_path);
    void setLatencyEnergy(const nlohmann::json& j);

    EnergyCounter* getEnergyCounterPtr();

private:
    const NetworkConfig& config_;
    const SimConfig& sim_config_;
    std::string name_;

    std::unordered_map<int, Switch*> switch_map_;

    std::unordered_map<int, std::unordered_map<int, double>> latency_map_;  // cycle
    std::unordered_map<int, std::unordered_map<int, double>> energy_map_;   // pJ

    EnergyCounter energy_counter_;
};

}  // namespace cimsim