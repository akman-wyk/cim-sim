//
// Created by wyk on 11/3/23.
//

#include "base_module.h"

namespace cimsim {

BaseModule::BaseModule(const sc_module_name& name, const BaseInfo& base_info)
    : sc_module(name)
    , period_ns_(base_info.sim_config.period_ns)
    , sim_mode_(base_info.sim_config.sim_mode)
    , data_mode_(base_info.sim_config.data_mode)
    , core_id_(base_info.core_id)
    , name_(name) {}

EnergyCounter* BaseModule::getEnergyCounterPtr() {
    return &energy_counter_;
}

const std::string& BaseModule::getName() const {
    return name_;
}

const char* BaseModule::getFullName() const {
    return name();
}

}  // namespace cimsim
