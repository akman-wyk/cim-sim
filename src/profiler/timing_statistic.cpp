//
// Created by wyk on 2025/4/15.
//

#include "timing_statistic.h"

#include <utility>

#include "fmt/format.h"
#include "profiler.h"
#include "util/util.h"

namespace cimsim {

void to_json(nlohmann::ordered_json& j, const TimeSegment& t) {
    j["start"] = t.start.to_seconds() * 1e9;
    j["end"] = t.end.to_seconds() * 1e9;
}

TimingStatistic::TimingStatistic(bool record_time_segment) : record_time_segment_(record_time_segment) {}

void TimingStatistic::addActivityTime(double latency) {
    auto& now_time = sc_time_stamp();
    auto end_time = now_time + sc_time{latency, SC_NS};

    if (end_time_tag_ < end_time) {
        if (now_time > end_time_tag_) {
            if (end_time_tag_ > start_time_tag_) {
                activity_time_ += (end_time_tag_ - start_time_tag_).to_seconds() * 1e9;
                if (record_time_segment_) {
                    time_segment_list_.push_back({start_time_tag_, end_time_tag_});
                }
            }
            start_time_tag_ = now_time;
        }
        end_time_tag_ = end_time;
    }
}

void TimingStatistic::finishRun() {
    if (end_time_tag_ > start_time_tag_) {
        activity_time_ += (end_time_tag_ - start_time_tag_).to_seconds() * 1e9;
        if (record_time_segment_) {
            time_segment_list_.push_back({start_time_tag_, end_time_tag_});
        }
    }
}

void TimingStatistic::report(std::ostream& ofs, double total_latency) {
    ofs << fmt::format("{:.3f}ns ({:.2f}%)", activity_time_,
                       (total_latency == 0.0 ? 0.0 : (activity_time_ / total_latency) * 100));
    if (record_time_segment_) {
        ofs << ", [";
        for (int i = 0; i < time_segment_list_.size(); i++) {
            auto& [start, end] = time_segment_list_[i];
            ofs << fmt::format("[{:.3f}->{:.3f}]", start.to_seconds() * 1e9, end.to_seconds() * 1e9);
            if (i != time_segment_list_.size() - 1) {
                ofs << ", ";
            }
        }
        ofs << "]";
    }
}

void to_json(nlohmann::ordered_json& j, const TimingStatistic& t) {
    j["activity_time"] = t.activity_time_;
    j["time_segment_list"] = t.time_segment_list_;
}

bool HardwareTimingStatistic::record_time_segments_ = false;

HardwareTimingStatistic::HardwareTimingStatistic(std::string name)
    : name_(std::move(name)), timing_statistic_(record_time_segments_) {}

void HardwareTimingStatistic::addActivityTime(double latency) {
    timing_statistic_.addActivityTime(latency);
    if (parent_ != nullptr) {
        parent_->addActivityTime(latency);
    }
}

void HardwareTimingStatistic::finishRun() {
    timing_statistic_.finishRun();
}

void HardwareTimingStatistic::addSub(const std::shared_ptr<HardwareTimingStatistic>& sub) {
    sub_list_.emplace_back(sub);
}

void HardwareTimingStatistic::setParent(const std::shared_ptr<HardwareTimingStatistic>& parent) {
    if (parent_ != nullptr) {
        throw std::runtime_error{"addSubHardwareTimingStatistic duplicate"};
    }
    parent_ = parent;
}

void HardwareTimingStatistic::report(std::ostream& ofs, int level, int remain_level_cnt, double total_latency) {
    auto leaf_name = splitAndGetLastPart(name_, ".");
    printTab(ofs, level);
    ofs << fmt::format("{}: ", leaf_name);
    timing_statistic_.report(ofs, total_latency);
    ofs << "\n";

    if (remain_level_cnt > 0) {
        for (auto& sub : sub_list_) {
            sub->report(ofs, level + 1, remain_level_cnt - 1, total_latency);
        }
    }
}

const std::string& HardwareTimingStatistic::getName() const {
    return name_;
}

void to_json(nlohmann::ordered_json& j, const HardwareTimingStatistic& t) {
    j["timing"] = t.timing_statistic_;

    if (!Profiler::json_flat) {
        for (auto& sub : t.sub_list_) {
            auto leaf_name = splitAndGetLastPart(sub->getName(), ".");
            j["sub"][leaf_name] = *sub;
        }
    }
}

InstTimingStatistic::InstTimingStatistic(std::string name)
    : name_(std::move(name))
    , timing_statistics_{TimingStatistic{true}, TimingStatistic{true}, TimingStatistic{true}, TimingStatistic{true}} {}

void InstTimingStatistic::addActivityTime(double latency, InstProfilerOperator inst_profiler_operator) {
    timing_statistics_[inst_profiler_operator._to_integral()].addActivityTime(latency);
    if (parent_ != nullptr) {
        parent_->addActivityTime(latency, inst_profiler_operator);
    }
}

void InstTimingStatistic::finishRun() {
    for (auto& timing_statistic : timing_statistics_) {
        timing_statistic.finishRun();
    }
}

void InstTimingStatistic::addSub(const std::shared_ptr<InstTimingStatistic>& sub) {
    sub_list_.emplace_back(sub);
}

void InstTimingStatistic::setParent(const std::shared_ptr<InstTimingStatistic>& parent) {
    if (parent_ != nullptr) {
        throw std::runtime_error{"addSubHardwareTimingStatistic duplicate"};
    }
    parent_ = parent;
}

void InstTimingStatistic::report(std::ostream& ofs, int level, double total_latency) {
    auto leaf_name = splitAndGetLastPart(name_, ".");
    printTab(ofs, level);
    ofs << fmt::format("{}: \n", leaf_name);
    for (int i = 0; i < INST_PROFILER_TYPE_COUNT; i++) {
        printTab(ofs, level + 1);
        ofs << fmt::format("{}: ", InstProfilerOperator::_from_integral(i)._to_string());
        timing_statistics_[i].report(ofs, total_latency);
        ofs << '\n';
    }

    for (auto& sub : sub_list_) {
        sub->report(ofs, level + 1, total_latency);
    }
}

const std::string& InstTimingStatistic::getName() const {
    return name_;
}

void to_json(nlohmann::ordered_json& j, const InstTimingStatistic& t) {
    for (int i = 0; i < INST_PROFILER_TYPE_COUNT; i++) {
        j[InstProfilerOperator::_from_integral(i)._to_string()] = t.timing_statistics_[i];
    }

    if (!Profiler::json_flat) {
        for (auto& sub : t.sub_list_) {
            auto leaf_name = splitAndGetLastPart(sub->getName(), ".");
            j["sub"][leaf_name] = *sub;
        }
    }
}

}  // namespace cimsim
