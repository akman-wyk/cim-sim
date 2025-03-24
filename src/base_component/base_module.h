//
// Created by wyk on 11/3/23.
//

#pragma once
#include "config/config.h"
#include "energy_counter.h"
#include "systemc.h"
#include "util/reporter.h"

namespace cimsim {

class Core;

struct BaseInfo {
    const SimConfig& sim_config;
    int core_id{-1};
};

class BaseModule : public sc_module {
public:
    BaseModule(const sc_module_name& name, const BaseInfo& base_info);

    virtual EnergyReporter getEnergyReporter();

    const std::string& getName() const;
    const char* getFullName() const;

protected:
    const double period_ns_;
    const SimMode sim_mode_;
    const DataMode data_mode_;

    int core_id_;

    EnergyCounter energy_counter_;

private:
    const std::string name_;
};

}  // namespace cimsim
