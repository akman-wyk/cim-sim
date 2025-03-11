//
// Created by wyk on 11/3/23.
//

#include "base_module.h"

namespace cimsim {

BaseModule::BaseModule(const char* name, const SimConfig& sim_config, Core* core, Clock* clk)
    : sc_core::sc_module(name)
    , period_ns_(sim_config.period_ns)
    , sim_mode_(sim_config.sim_mode)
    , data_mode_(sim_config.data_mode)
    , core_(core)
    , clk_(clk)
    , name(name) {}

EnergyReporter BaseModule::getEnergyReporter() {
    return EnergyReporter{energy_counter_};
}

const std::string& BaseModule::getName() const {
    return name;
}

}  // namespace cimsim
