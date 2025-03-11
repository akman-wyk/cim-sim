//
// Created by wyk on 11/1/23.
//

#pragma once

#include <unordered_map>

#include "systemc.h"

namespace cimsim {

class EnergyCounter {
    // energy unit -- pJ
    // power unit  -- mW
    // time unit   -- ns

public:
    static void setRunningTimeNS(double time);
    static void setRunningTimeNS(const sc_core::sc_time& time);
    static double getRunningTimeNS();

private:
    static double running_time_;  // ns
    static bool set_running_time_;

public:
    EnergyCounter() = default;
    EnergyCounter(const EnergyCounter& another);

    void clear();

    void setStaticPowerMW(double power);

    void addDynamicEnergyPJ(double energy);
    void addDynamicEnergyPJ(double latency, double power);
    void addDynamicEnergyPJ(double latency, double power, const sc_core::sc_time& time_tag, int id_tag);

    void addPipelineDynamicEnergyPJ(int unit_latency_cycle, int pipeline_length, double period, double power);

    [[nodiscard]] double getStaticEnergyPJ() const;
    [[nodiscard]] double getDynamicEnergyPJ() const;
    [[nodiscard]] double getActivityTime() const;
    [[nodiscard]] double getTotalEnergyPJ() const;
    [[nodiscard]] double getAveragePowerMW() const;

    EnergyCounter& operator+=(const EnergyCounter& another);

private:
    double static_power_ = 0.0;    // mW
    double dynamic_energy_ = 0.0;  // pJ
    double activity_time_ = 0.0;   // ns

    std::unordered_map<int, sc_core::sc_time> dynamic_time_tag_map_{};
    sc_core::sc_time activity_time_tag_{0.0, SC_NS};
};

}  // namespace cimsim
