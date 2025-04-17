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

InstProfiler::InstProfiler(const InstProfilerConfig& config) : config_(config) {
    for (auto& group_config : config_.inst_groups) {
        addInstGroup(group_config, nullptr);
    }
}

void InstProfiler::bindEnergyCounter(EnergyCounter* energy_counter) {
    if (energy_counter->inst_profiler_ == nullptr) {
        energy_counter->inst_profiler_ = this;
    }
    for (auto& [sub_name, sub_energy_counter] : energy_counter->sub_energy_counter_list_) {
        bindEnergyCounter(sub_energy_counter);
    }
}

void InstProfiler::addActivityTime(double latency, const ProfilerTag& profiler_tag) {
    if (config_.single_inst_profiling) {
        auto inst_timing_statistic = getSingleInstTimingStatistic(profiler_tag);
        inst_timing_statistic->addActivityTime(latency, profiler_tag.inst_profiler_operator);
    }
    if (config_.inst_type_profiling) {
        auto inst_timing_statistic = getInstTypeTimingStatistic(profiler_tag);
        inst_timing_statistic->addActivityTime(latency, profiler_tag.inst_profiler_operator);
    }
    if (config_.inst_group_profiling && !profiler_tag.inst_group_tag.empty()) {
        auto inst_timing_statistic = getInstGroupTimingStatistic(profiler_tag);
        inst_timing_statistic->addActivityTime(latency, profiler_tag.inst_profiler_operator);
    }
}

void InstProfiler::finishRun() {
    for (auto& [name, timing_statistic] : timing_statistic_map_) {
        timing_statistic->finishRun();
    }
}

void InstProfiler::report(std::ostream& ofs, double total_latency) {
    for (auto& top_timing_statistic : top_timing_statistic_list_) {
        top_timing_statistic->report(ofs, 0, total_latency);
    }
}

void to_json(nlohmann::ordered_json& j, const InstProfiler& t) {
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

void InstProfiler::addInstGroup(const InstProfilerGroupConfig& group_config,
                                const std::shared_ptr<InstTimingStatistic>& parent) {
    std::shared_ptr<InstTimingStatistic> inst_group_timing_statistic;
    if (parent == nullptr) {
        inst_group_timing_statistic = std::make_shared<InstTimingStatistic>(group_config.name);
        top_timing_statistic_list_.emplace_back(inst_group_timing_statistic);
    } else {
        auto full_name = parent->getName() + "." + group_config.name;
        inst_group_timing_statistic = std::make_shared<InstTimingStatistic>(full_name);

        parent->addSub(inst_group_timing_statistic);
        inst_group_timing_statistic->setParent(parent);
    }

    timing_statistic_map_.emplace(inst_group_timing_statistic->getName(), inst_group_timing_statistic);
    for (auto& sub_group : group_config.sub_groups) {
        addInstGroup(sub_group, inst_group_timing_statistic);
    }
}

std::shared_ptr<InstTimingStatistic> InstProfiler::getSingleInstTimingStatistic(const ProfilerTag& profiler_tag) {
    auto single_inst_name =
        fmt::format("Core_{}_{}_{}", profiler_tag.core_id, profiler_tag.inst_opcode._to_string(), profiler_tag.ins_id);
    std::shared_ptr<InstTimingStatistic> single_inst_timing_statistic;
    if (auto found = timing_statistic_map_.find(single_inst_name); found == timing_statistic_map_.end()) {
        single_inst_timing_statistic = std::make_shared<InstTimingStatistic>(single_inst_name);
        timing_statistic_map_.emplace(single_inst_name, single_inst_timing_statistic);
        top_timing_statistic_list_.emplace_back(single_inst_timing_statistic);
    } else {
        single_inst_timing_statistic = found->second;
    }
    return single_inst_timing_statistic;
}

std::shared_ptr<InstTimingStatistic> InstProfiler::getInstTypeTimingStatistic(const ProfilerTag& profiler_tag) {
    std::string inst_type_name = profiler_tag.inst_opcode._to_string();
    std::shared_ptr<InstTimingStatistic> inst_type_timing_statistic;
    if (auto found = timing_statistic_map_.find(inst_type_name); found == timing_statistic_map_.end()) {
        inst_type_timing_statistic = std::make_shared<InstTimingStatistic>(inst_type_name);
        timing_statistic_map_.emplace(inst_type_name, inst_type_timing_statistic);
        top_timing_statistic_list_.emplace_back(inst_type_timing_statistic);
    } else {
        inst_type_timing_statistic = found->second;
    }
    return inst_type_timing_statistic;
}

std::shared_ptr<InstTimingStatistic> InstProfiler::getInstGroupTimingStatistic(const ProfilerTag& profiler_tag) {
    return timing_statistic_map_[std::string{profiler_tag.inst_group_tag}];
}

bool Profiler::json_flat = false;

Profiler::Profiler(const ProfilerConfig& config) : config_(config), inst_profiler_(config_.inst_profiler_config) {
    Profiler::json_flat = config_.json_flat;
    HardwareTimingStatistic::record_time_segments_ = config_.hardware_profiler_config.record_timing_segments;
}

void Profiler::finishRun() {
    hardware_profiler_.finishRun();
    inst_profiler_.finishRun();
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
    ofs << "Instruction Profiling:\n";
    inst_profiler_.report(ofs, total_latency);

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
    }
    if (config_.inst_profiler_config.single_inst_profiling || config_.inst_profiler_config.inst_type_profiling ||
        config_.inst_profiler_config.inst_group_profiling) {
        inst_profiler_.bindEnergyCounter(chip_energy_counter);
    }
}

void to_json(nlohmann::ordered_json& j, const Profiler& t) {
    if (t.config_.profiling) {
        if (t.config_.hardware_profiler_config.profiling) {
            j["hardware_profiling"] = t.hardware_profiler_;
        }
        j["instruction_profiling"] = t.inst_profiler_;
    }
}

}  // namespace cimsim
