//
// Created by Akman on 2023/11/14.
//

#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace pimsim {

#define ENERGY_REPORT_INDENT_SPACE_NUM 4

class EnergyCounter;

struct EnergyReportItem {
    std::string name;
    std::string total_energy, static_energy, dynamic_energy;  // pJ(%all)
    std::string activity_time;                                // ns(%all)
};

struct EnergyReporterCompare {
    double total_energy_diff{0.0}, total_energy_radio{1.0};
    double static_energy_diff{0.0}, static_energy_radio{1.0};
    double dynamic_energy_diff{0.0}, dynamic_energy_radio{1.0};
    double activity_time_diff{0.0}, activity_time_radio{1.0};

    std::map<std::string, EnergyReporterCompare> sub_modules;

    std::vector<EnergyReportItem> getEnergyReportItem(const std::string& module_name, int indent_level);
};

struct ReporterCompare {
    double latency_diff{0.0}, latency_radio{1.0};
    double average_power_diff{0.0}, average_power_radio{1.0};
    double total_energy_diff{0.0}, total_energy_radio{1.0};
    double TOPS_diff{0.0}, TOPS_radio{1.0};
    double TOPS_per_W_diff{0.0}, TOPS_per_W_radio{1.0};

    std::string module_name;
    EnergyReporterCompare energy_reporter_compare;

    void report(std::ostream& os, bool detail = true);
};

class EnergyReporter {
public:
    EnergyReporter() = default;
    EnergyReporter(double total_energy, double static_energy, double dynamic_energy, double activity_time);
    explicit EnergyReporter(const EnergyCounter& energy_counter);

    void addSubModule(std::string name, EnergyReporter sub_module);

    std::vector<EnergyReportItem> getEnergyReportItem(const std::string& module_name, double all_energy,
                                                      double total_latency, int indent_level);

    [[nodiscard]] double getTotalEnergyPJ() const;
    [[nodiscard]] double getDynamicEnergyPJ() const;

    void accumulate(const EnergyReporter& another, bool same_simulation);

    [[nodiscard]] EnergyReporterCompare compare(const EnergyReporter& r2) const;

private:
    double total_energy_{0.0};    // pJ
    double static_energy_{0.0};   // pJ
    double dynamic_energy_{0.0};  // pJ
    double activity_time_{0.0};   // ns

    std::map<std::string, EnergyReporter> sub_modules_;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(EnergyReporter, total_energy_, static_energy_, dynamic_energy_,
                                                activity_time_, sub_modules_)
};

class Reporter {
public:
    Reporter() = default;

    Reporter(double latency_ms, std::string module_name, const EnergyReporter& energy_reporter, int OP_count);

    void report(std::ostream& os, bool detail = true);

    void reportEnergyForm(std::ostream& os);

    [[nodiscard]] double getAveragePowerMW() const;
    [[nodiscard]] double getTotalEnergyPJ() const;
    [[nodiscard]] double getLatencyNs() const;

    [[nodiscard]] double getDynamicEnergyPJ() const;

    Reporter& operator+=(const Reporter& another);

    [[nodiscard]] ReporterCompare compare(const Reporter& r2) const;

    void setOPCount(double OP_count);

private:
    double latency_{0.0};        // ms
    double average_power_{0.0};  // mW
    double total_energy_{0.0};   // pJ
    double TOPS_{0.0};
    double TOPS_per_W_{0.0};
    int OP_count_{0};

    std::string module_name_;
    EnergyReporter energy_reporter_;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Reporter, latency_, average_power_, total_energy_, TOPS_, TOPS_per_W_,
                                                OP_count_, module_name_, energy_reporter_)
};

}  // namespace pimsim
