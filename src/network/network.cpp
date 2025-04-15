//
// Created by wyk on 2024/11/7.
//

#include "network.h"

#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

Network::Network(std::string name, const NetworkConfig& config, const SimConfig& sim_config)
    : config_(config), sim_config_(sim_config), name_(std::move(name)) {
    readLatencyEnergyFile(config.network_config_file_path);
}

sc_time Network::transferAndGetDelay(int src_id, int dst_id, int data_size_byte) {
    auto per_flit_latency_ns = latency_map_[src_id][dst_id] * sim_config_.period_ns;
    auto per_flit_energy_pj = energy_map_[src_id][dst_id];
    int times = IntDivCeil(data_size_byte, config_.bus_width_byte);
    double latency = times * per_flit_latency_ns;

    energy_counter_.addDynamicEnergyPJ(times * per_flit_energy_pj);
    energy_counter_.addActivityTime(latency);

    auto cur = sc_time_stamp().to_seconds() * 1e9;
    std::cout << fmt::format("[{}, {}]\n", cur, cur + latency);

    return sc_time{latency, SC_NS};
}

Switch* Network::getSwitch(int id) {
    return switch_map_[id];
}

void Network::registerSwitch(int id, Switch* switch_ptr) {
    if (switch_map_.find(id) == switch_map_.end()) {
        switch_map_[id] = switch_ptr;
    }
}

void Network::setLatencyEnergy(const nlohmann::json& j) {
    // set latency
    auto latency_array = j["latency"];
    for (const auto& src_array : latency_array.items()) {
        auto src_key = std::stoi(src_array.key());

        for (const auto& dst : src_array.value().items()) {
            auto dst_key = std::stoi(dst.key());
            latency_map_[src_key][dst_key] = dst.value();
        }
    }

    // set energy
    auto energy_array = j["energy"];
    for (const auto& src_array : energy_array.items()) {
        auto src_key = std::stoi(src_array.key());

        for (const auto& dst : src_array.value().items()) {
            auto dst_key = std::stoi(dst.key());
            energy_map_[src_key][dst_key] = dst.value();
        }
    }
}

void Network::readLatencyEnergyFile(const std::string& file_path) {
    std::ifstream ifs(config_.network_config_file_path);
    auto j = nlohmann::json::parse(ifs);
    setLatencyEnergy(j);
}

EnergyCounter* Network::getEnergyCounterPtr() {
    return &energy_counter_;
}

}  // namespace cimsim
