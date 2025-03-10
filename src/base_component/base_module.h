//
// Created by wyk on 11/3/23.
//

#pragma once
#include "clock.h"
#include "config/config.h"
#include "energy_counter.h"
#include "systemc.h"
#include "util/reporter.h"

namespace pimsim {

class Core;

class BaseModule : public sc_core::sc_module {
public:
    BaseModule(const char* name, const SimConfig& sim_config, Core* core, Clock* clk = nullptr);

    virtual EnergyReporter getEnergyReporter();

    const std::string& getName() const;

protected:
    const double period_ns_;
    const SimMode sim_mode_;
    const DataMode data_mode_;

    Core* core_;
    Clock* clk_;

    EnergyCounter energy_counter_;

private:
    const std::string name;
};

}  // namespace pimsim
