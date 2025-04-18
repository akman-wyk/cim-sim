//
// Created by wyk on 11/1/23.
//

#include "energy_counter.h"

namespace cimsim {

double EnergyCounter::running_time_ = 0.0;
bool EnergyCounter::set_running_time_ = false;

EnergyCounter::EnergyCounter(bool mult_pipeline_stage) : mult_pipeline_stage_(mult_pipeline_stage) {
    if (mult_pipeline_stage_) {
        dynamic_tag_stack_ = new std::stack<DynamicEnergyTag>;
    }
}

EnergyCounter::~EnergyCounter() {
    delete dynamic_tag_stack_;
}

void EnergyCounter::setStaticPowerMW(double power) {
    static_power_ = power;
}

void EnergyCounter::addDynamicEnergyPJ(double energy) {
    dynamic_energy_ += energy;
}

void EnergyCounter::addDynamicEnergyPJ(double latency, double power) {
    if (mult_pipeline_stage_) {
        addPipelineStageDynamicEnergyPJ(latency, power);
    } else {
        dynamic_energy_ += latency * power;
    }
    addActivityTime(latency);
}

void EnergyCounter::addActivityTime(double latency) {
    auto& now_time = sc_time_stamp();
    auto end_time_tag = now_time + sc_time{latency, SC_NS};

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

void EnergyCounter::setRunningTimeNS(double time) {
    running_time_ = time;
    set_running_time_ = true;
}

void EnergyCounter::setRunningTimeNS(const sc_time& time) {
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

void EnergyCounter::addPipelineStageDynamicEnergyPJ(double latency, double power) {
    static std::stack<DynamicEnergyTag> temp_stack{};

    auto now_time = sc_time_stamp();
    auto end_time_tag = now_time + sc_time{latency, SC_NS};

    while (!dynamic_tag_stack_->empty() && dynamic_tag_stack_->top().end_time <= now_time) {
        dynamic_tag_stack_->pop();
    }

    while (!dynamic_tag_stack_->empty() && now_time < end_time_tag) {
        auto cur_tag = dynamic_tag_stack_->top();
        dynamic_tag_stack_->pop();

        if (power > cur_tag.power) {
            if (cur_tag.end_time > end_time_tag) {
                dynamic_energy_ += (power - cur_tag.power) * ((end_time_tag - now_time).to_seconds() * 1e9);
                temp_stack.push({.end_time = end_time_tag, .power = power});
                temp_stack.push(cur_tag);
            } else {
                dynamic_energy_ += (power - cur_tag.power) * ((cur_tag.end_time - now_time).to_seconds() * 1e9);
                temp_stack.push({.end_time = cur_tag.end_time, .power = power});
            }
        } else {
            temp_stack.push(cur_tag);
        }
        now_time = cur_tag.end_time;
    }
    if (now_time < end_time_tag) {
        dynamic_energy_ += power * ((end_time_tag - now_time).to_seconds() * 1e9);
        temp_stack.push({.end_time = end_time_tag, .power = power});
    }

    while (!temp_stack.empty()) {
        auto& cur_tag = temp_stack.top();
        if (dynamic_tag_stack_->empty() || cur_tag.power > dynamic_tag_stack_->top().power) {
            dynamic_tag_stack_->push(cur_tag);
        }
        temp_stack.pop();
    }
}

}  // namespace cimsim
