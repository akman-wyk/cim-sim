//
// Created by wyk on 2025/4/15.
//

#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "base_component/energy_counter.h"
#include "config/config.h"
#include "core/payload.h"
#include "nlohmann/json.hpp"
#include "timing_statistic.h"

namespace cimsim {

class Core;
class InstProfiler;

class HardwareProfiler {
public:
    void bindEnergyCounter(const std::string& name, EnergyCounter* energy_counter,
                           const std::shared_ptr<HardwareTimingStatistic>& parent);

    void finishRun();

    void report(std::ostream& ofs, int level_cnt, double total_latency);

    friend void to_json(nlohmann::ordered_json& j, const HardwareProfiler& t);

private:
    std::unordered_map<std::string, std::shared_ptr<HardwareTimingStatistic>> timing_statistic_map_{};
    std::vector<std::shared_ptr<HardwareTimingStatistic>> top_timing_statistic_list_{};
};

class InstProfiler {
public:
    explicit InstProfiler(const InstProfilerConfig& config);

    void bindEnergyCounter(EnergyCounter* energy_counter);

    void addActivityTime(double latency, const ProfilerTag& profiler_tag);
    void finishRun();

    void report(std::ostream& ofs, double total_latency);

    friend void to_json(nlohmann::ordered_json& j, const InstProfiler& t);

private:
    void addInstGroup(const InstProfilerGroupConfig& group_config, const std::shared_ptr<InstTimingStatistic>& parent);

    std::shared_ptr<InstTimingStatistic> getSingleInstTimingStatistic(const ProfilerTag& profiler_tag);
    std::shared_ptr<InstTimingStatistic> getInstTypeTimingStatistic(const ProfilerTag& profiler_tag);
    std::shared_ptr<InstTimingStatistic> getInstGroupTimingStatistic(const ProfilerTag& profiler_tag);

private:
    const InstProfilerConfig& config_;

    std::unordered_map<std::string, std::shared_ptr<InstTimingStatistic>> timing_statistic_map_{};
    std::vector<std::shared_ptr<InstTimingStatistic>> top_timing_statistic_list_{};
};

class Profiler {
public:
    static bool json_flat;

public:
    explicit Profiler(const ProfilerConfig& config);

    void bindHardware(EnergyCounter* chip_energy_counter, std::vector<std::shared_ptr<Core>>& core_list);
    void finishRun();

    void report(std::ostream& ofs, double total_latency);

    friend void to_json(nlohmann::ordered_json& j, const Profiler& t);

private:
    const ProfilerConfig& config_;

    HardwareProfiler hardware_profiler_;
    InstProfiler inst_profiler_;
};

}  // namespace cimsim
