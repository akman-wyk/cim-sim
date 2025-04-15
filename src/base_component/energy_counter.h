//
// Created by wyk on 11/1/23.
//

#pragma once

#include <set>
#include <stack>

#include "profiler/timing_statistic.h"
#include "systemc.h"

namespace cimsim {

class EnergyReporter;
class HardwareProfiler;

class EnergyCounter {
    // energy unit -- pJ
    // power unit  -- mW
    // time unit   -- ns
    friend HardwareProfiler;

public:
    struct DynamicEnergyTag {
        sc_time end_time{0.0, SC_NS};
        double power{0.0};
    };

public:
    static void setRunningTimeNS(double time);
    static void setRunningTimeNS(const sc_time& time);
    static double getRunningTimeNS();

private:
    static double running_time_;  // ns
    static bool set_running_time_;

public:
    explicit EnergyCounter(bool mult_pipeline_stage = false);
    ~EnergyCounter();

    void setStaticPowerMW(double power);
    void addDynamicEnergyPJ(double energy);
    void addDynamicEnergyPJ(double latency, double power);
    void addActivityTime(double latency);

    [[nodiscard]] double getStaticEnergyPJ() const;
    [[nodiscard]] double getDynamicEnergyPJ() const;
    [[nodiscard]] double getTotalEnergyPJ() const;
    [[nodiscard]] double getAveragePowerMW() const;

    void addSubEnergyCounter(const std::string_view& name, EnergyCounter* sub_energy_counter);
    [[nodiscard]] EnergyReporter getEnergyReporter() const;

private:
    void addPipelineStageDynamicEnergyPJ(double latency, double power);

private:
    const bool mult_pipeline_stage_;
    double static_power_ = 0.0;    // mW
    double dynamic_energy_ = 0.0;  // pJ

    std::stack<DynamicEnergyTag>* dynamic_tag_stack_{};
    sc_time activity_time_tag_{0.0, SC_NS};

    EnergyCounter* parent_energy_counter_{nullptr};
    std::vector<std::pair<std::string_view, EnergyCounter*>> sub_energy_counter_list_{};

    std::set<std::shared_ptr<HardwareTimingStatistic>> hardware_timing_statistic_list{};
};

}  // namespace cimsim
