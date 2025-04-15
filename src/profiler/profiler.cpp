//
// Created by wyk on 2025/4/15.
//

#include "profiler.h"
#include "core/core.h"

namespace cimsim {

void HardwareProfiler::bindEnergyCounter(const std::string& name, EnergyCounter* energy_counter,
                                         const std::shared_ptr<HardwareTimingStatistic>& parent) {
    std::shared_ptr<HardwareTimingStatistic> timing_statistic_ptr;
    if (auto found = timing_statistic_map_.find(name); found == timing_statistic_map_.end()) {
        timing_statistic_ptr = std::make_shared<HardwareTimingStatistic>(name);
        if (parent != nullptr) {
            parent->addSub(timing_statistic_ptr);
            timing_statistic_ptr->setParent(parent);
        } else {
            top_timing_statistic_list_.emplace_back(timing_statistic_ptr);
        }

        timing_statistic_map_.emplace(name, timing_statistic_ptr);
    } else {
        timing_statistic_ptr = found->second;
    }

    if (energy_counter->hardware_timing_statistic_list.count(timing_statistic_ptr) == 0) {
        energy_counter->hardware_timing_statistic_list.emplace(timing_statistic_ptr);
        for (auto& [sub_name, sub_energy_counter] : energy_counter->sub_energy_counter_list_) {
            std::string sub_full_name = name + "." + std::string{sub_name};
            bindEnergyCounter(sub_full_name, sub_energy_counter, timing_statistic_ptr);
        }
    }
}

void HardwareProfiler::finishRun() {
    for (auto& [name, timing_statistic] : timing_statistic_map_) {
        timing_statistic->finishRun();
    }
}

void HardwareProfiler::report(std::ostream& ofs, int level_cnt, double total_latency) {
    for (auto& top_timing_statistic : top_timing_statistic_list_) {
        top_timing_statistic->report(ofs, 0, level_cnt, total_latency);
    }
}

void to_json(nlohmann::ordered_json& j, const HardwareProfiler& t) {
    if (Profiler::json_flat) {
        for (auto& [name, timing_statistic] : t.timing_statistic_map_) {
            j[name] = *timing_statistic;
        }
    } else {
        for (auto& top_timing_statistic : t.top_timing_statistic_list_) {
            j[top_timing_statistic->getName()] = *top_timing_statistic;
        }
    }
}

bool Profiler::json_flat = false;

Profiler::Profiler(const ProfilerConfig& config) : config_(config) {
    Profiler::json_flat = config_.json_flat;
}

void Profiler::finishRun() {
    hardware_profiler_.finishRun();
}

void Profiler::report(std::ostream& ofs, double total_latency) {
    if (!config_.profiling) {
        return;
    }

    ofs << "\nProfiling:\n";
    if (config_.hardware_profiler_config.profiling) {
        ofs << "Hardware Profiling:\n";
        hardware_profiler_.report(ofs, config_.hardware_profiler_config.report_level_cnt_, total_latency);
    }

    if (config_.report_to_json) {
        nlohmann::ordered_json profiling_json = *this;
        std::ofstream json_ofs;
        json_ofs.open(config_.json_file);
        json_ofs << profiling_json;
        json_ofs.close();
    }
}

void Profiler::bindHardware(EnergyCounter* chip_energy_counter, std::vector<std::shared_ptr<Core>>& core_list) {
    if (config_.hardware_profiler_config.profiling) {
        hardware_profiler_.bindEnergyCounter("Chip", chip_energy_counter, nullptr);
        if (config_.hardware_profiler_config.each_core_profiling) {
            for (auto& core : core_list) {
                hardware_profiler_.bindEnergyCounter(core->getName(), core->getEnergyCounterPtr(), nullptr);
            }
        }
        HardwareTimingStatistic::record_time_segments_ = config_.hardware_profiler_config.record_timing_segments;
    }
}

void to_json(nlohmann::ordered_json& j, const Profiler& t) {
    j["hardware_profiling"] = t.hardware_profiler_;
}

}  // namespace cimsim
