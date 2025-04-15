//
// Created by wyk on 11/1/23.
//

#include "energy_counter.h"

#include "util/reporter.h"

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
    for (auto& timing_statistic : hardware_timing_statistic_list) {
        timing_statistic->addActivityTime(latency);
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

double EnergyCounter::getTotalEnergyPJ() const {
    return getStaticEnergyPJ() + getDynamicEnergyPJ();
}

double EnergyCounter::getAveragePowerMW() const {
    return getTotalEnergyPJ() / getRunningTimeNS();  // pJ / ns = mW
}

void EnergyCounter::addSubEnergyCounter(const std::string_view& name, EnergyCounter* sub_energy_counter) {
    sub_energy_counter_list_.emplace_back(name, sub_energy_counter);
    if (sub_energy_counter->parent_energy_counter_ != nullptr) {
        throw std::runtime_error{"addSubEnergyCounter duplicate"};
    }
    sub_energy_counter->parent_energy_counter_ = this;
}

EnergyReporter EnergyCounter::getEnergyReporter() const {
    EnergyReporter reporter{*this};
    for (const auto& [name, sub] : sub_energy_counter_list_) {
        reporter.addSubModule(std::string{name}, sub->getEnergyReporter());
    }
    return std::move(reporter);
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
