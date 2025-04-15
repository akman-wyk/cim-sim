//
// Created by wyk on 2025/4/15.
//

#pragma once

#include <string>

#include "nlohmann/json.hpp"
#include "systemc.h"

namespace cimsim {

struct TimeSegment {
    sc_time start;
    sc_time end;

    friend void to_json(nlohmann::ordered_json& j, const TimeSegment& t);
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
    std::string name_{};

    double activity_time_{0.0};  // ns
    sc_time start_time_tag_{0.0, SC_NS};
    sc_time end_time_tag_{0.0, SC_NS};
    std::vector<TimeSegment> time_segment_list_{};

    std::shared_ptr<HardwareTimingStatistic> parent_{nullptr};
    std::vector<std::shared_ptr<HardwareTimingStatistic>> sub_list_{};
};

}  // namespace cimsim
