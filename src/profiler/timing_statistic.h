//
// Created by wyk on 2025/4/15.
//

#pragma once

#include <string>

#include "config/config.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "systemc.h"

namespace cimsim {

struct TimeSegment {
    sc_time start;
    sc_time end;

    friend void to_json(nlohmann::ordered_json& j, const TimeSegment& t);
};

class TimingStatistic {
public:
    explicit TimingStatistic(bool record_time_segment);

    void addActivityTime(double latency);
    void finishRun();

    void report(std::ostream& ofs, double total_latency);
    friend void to_json(nlohmann::ordered_json& j, const TimingStatistic& t);

private:
    const bool record_time_segment_;

    double activity_time_{0.0};  // ns
    sc_time start_time_tag_{0.0, SC_NS};
    sc_time end_time_tag_{0.0, SC_NS};
    std::vector<TimeSegment> time_segment_list_{};
};

class HardwareTimingStatistic {
public:
    static bool record_time_segments_;

public:
    explicit HardwareTimingStatistic(std::string name);

    void addActivityTime(double latency);
    void finishRun();

    void addSub(const std::shared_ptr<HardwareTimingStatistic>& sub);
    void setParent(const std::shared_ptr<HardwareTimingStatistic>& parent);

    void report(std::ostream& ofs, int level, int remain_level_cnt, double total_latency);
    [[nodiscard]] const std::string& getName() const;

    friend void to_json(nlohmann::ordered_json& j, const HardwareTimingStatistic& t);

private:
    std::string name_;

    TimingStatistic timing_statistic_;

    std::shared_ptr<HardwareTimingStatistic> parent_{nullptr};
    std::vector<std::shared_ptr<HardwareTimingStatistic>> sub_list_{};
};

class InstTimingStatistic {
public:
    explicit InstTimingStatistic(std::string name);

    void addActivityTime(double latency, const std::string& inst_profiler_operator);
    void finishRun();

    void addSub(const std::shared_ptr<InstTimingStatistic>& sub);
    void setParent(const std::shared_ptr<InstTimingStatistic>& parent);

    void report(std::ostream& ofs, int level, double total_latency);
    [[nodiscard]] const std::string& getName() const;

    friend void to_json(nlohmann::ordered_json& j, const InstTimingStatistic& t);

private:
    std::string name_;

    std::unordered_map<std::string, std::shared_ptr<TimingStatistic>> timing_statistic_map_{};
    std::vector<std::pair<std::string, std::shared_ptr<TimingStatistic>>> timing_statistic_list_{};

    std::shared_ptr<InstTimingStatistic> parent_{nullptr};
    std::vector<std::shared_ptr<InstTimingStatistic>> sub_list_{};
};

}  // namespace cimsim
