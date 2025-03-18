//
// Created by wyk on 11/1/23.
//

#include "energy_counter.h"

namespace cimsim {

double EnergyCounter::running_time_ = 0.0;
bool EnergyCounter::set_running_time_ = false;

EnergyCounter::EnergyCounter(const EnergyCounter& another)
    : static_power_(another.static_power_)
    , dynamic_energy_(another.dynamic_energy_)
    , activity_time_(another.activity_time_) {}

void EnergyCounter::clear() {
    static_power_ = 0.0;
    dynamic_energy_ = 0.0;
    activity_time_ = 0.0;
    dynamic_end_time_tag_map_.clear();
}

void EnergyCounter::setStaticPowerMW(double power) {
    static_power_ = power;
}

void EnergyCounter::addDynamicEnergyPJ(double energy) {
    dynamic_energy_ += energy;
}

void EnergyCounter::addDynamicEnergyPJ(double latency, double power) {
    activity_time_ += latency;
    dynamic_energy_ += latency * power;
}

void EnergyCounter::addDynamicEnergyPJWithTime(double latency, double power, int id_tag) {
    auto& now_time = sc_core::sc_time_stamp();
    auto end_time_tag = now_time + sc_time{latency, SC_NS};
    auto found = dynamic_end_time_tag_map_.find(id_tag);

    if (found == dynamic_end_time_tag_map_.end()) {
        dynamic_energy_ += latency * power;
        dynamic_end_time_tag_map_.emplace(id_tag, end_time_tag);
    } else if (auto& lastend_time = found->second; lastend_time < end_time_tag) {
        auto new_dynamic_latency = latency;
        if (now_time < lastend_time) {
            auto overlap_time = lastend_time - now_time;
            new_dynamic_latency -= (overlap_time.to_seconds() * 1e9);
        }
        dynamic_energy_ += new_dynamic_latency * power;
        dynamic_end_time_tag_map_[id_tag] = end_time_tag;
    }

    if (activity_time_tag_ < end_time_tag) {
        auto new_activity_latency = latency;
        if (now_time < activity_time_tag_) {
            auto overlap_time = activity_time_tag_ - now_time;
            new_activity_latency -= (overlap_time.to_seconds() * 1e9);
        }
        activity_time_ += new_activity_latency;
        activity_time_tag_ = end_time_tag;
    }
}

void EnergyCounter::addPipelineDynamicEnergyPJ(int unit_latency_cycle, int pipeline_length, double period,
                                               double power) {
    if (pipeline_length <= 0) {
        return;
    }

    int total_cycle = (unit_latency_cycle == 0) ? pipeline_length : (unit_latency_cycle - 1 + pipeline_length);
    double total_latency = total_cycle * period;
    addDynamicEnergyPJ(total_latency, power);
}

void EnergyCounter::setRunningTimeNS(double time) {
    running_time_ = time;
    set_running_time_ = true;
}

void EnergyCounter::setRunningTimeNS(const sc_core::sc_time& time) {
    setRunningTimeNS(time.to_seconds() * 1e9);
}

double EnergyCounter::getRunningTimeNS() {
    if (!set_running_time_) {
        throw std::runtime_error("No running time has been set yet");
    }
    return running_time_;
}

double EnergyCounter::getStaticEnergyPJ() const {
    return static_power_ * getRunningTimeNS();  // mW * ns = pJ
}

double EnergyCounter::getDynamicEnergyPJ() const {
    return dynamic_energy_;
}

double EnergyCounter::getActivityTime() const {
    return activity_time_;
}

double EnergyCounter::getTotalEnergyPJ() const {
    return getStaticEnergyPJ() + getDynamicEnergyPJ();
}

double EnergyCounter::getAveragePowerMW() const {
    return getTotalEnergyPJ() / getRunningTimeNS();  // pJ / ns = mW
}

EnergyCounter& EnergyCounter::operator+=(const EnergyCounter& another) {
    activity_time_ = std::max(activity_time_, another.activity_time_);
    dynamic_energy_ += another.dynamic_energy_;
    static_power_ += another.static_power_;
    return *this;
}

}  // namespace cimsim
