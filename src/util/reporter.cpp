//
// Created by Akman on 2023/11/14.
//
#include "reporter.h"

#include "base_component/energy_counter.h"
#include "fmt/core.h"

namespace pimsim {

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define TERA      1e12

std::vector<EnergyReportItem> EnergyReporterCompare::getEnergyReportItem(const std::string& module_name,
                                                                         int indent_level) {
    std::vector<EnergyReportItem> items;

    std::string name;
    for (int i = 0; i < indent_level * ENERGY_REPORT_INDENT_SPACE_NUM; i++) {
        name += ' ';
    }
    name += module_name;

    std::string total_energy_str = fmt::format("{:.3f}pJ ({:.3f})", total_energy_diff, total_energy_radio);
    std::string static_energy_str = fmt::format("{:.3f}pJ ({:.3f})", static_energy_diff, static_energy_radio);
    std::string dynamic_energy_str = fmt::format("{:.3f}pJ ({:.3f})", dynamic_energy_diff, dynamic_energy_radio);
    std::string activity_time_str = fmt::format("{:.3f}ns ({:.3f})", activity_time_diff, activity_time_radio);
    items.emplace_back(
        EnergyReportItem{name, total_energy_str, static_energy_str, dynamic_energy_str, activity_time_str});

    for (auto& [sub_module_name, sub_module] : sub_modules) {
        auto sub_module_items = sub_module.getEnergyReportItem(sub_module_name, indent_level + 1);
        items.insert(items.end(), sub_module_items.begin(), sub_module_items.end());
    }

    return std::move(items);
}

void ReporterCompare::report(std::ostream& os, bool detail) {
    os << "Simulation Compare Result:\n";
    os << fmt::format("  - {:<25}{:.6f} ms\n", "latency diff:", latency_diff);
    os << fmt::format("  - {:<25}{:.3f}\n", "latency radio:", latency_radio);
    os << fmt::format("  - {:<25}{:.4f} mW\n", "average power diff:", average_power_diff);
    os << fmt::format("  - {:<25}{:.3f}\n", "average power radio:", average_power_radio);
    os << fmt::format("  - {:<25}{:.4f} pJ/it\n", "total energy diff:", total_energy_diff);
    os << fmt::format("  - {:<25}{:.3f}\n", "total energy radio:", total_energy_radio);
    os << fmt::format("  - {:<25}{:.4f}\n", "TOPS diff:", TOPS_diff);
    os << fmt::format("  - {:<25}{:.3f}\n", "TOPS radio:", TOPS_radio);
    os << fmt::format("  - {:<25}{:.4f}\n", "TOPS/W diff:", TOPS_per_W_diff);
    os << fmt::format("  - {:<25}{:.3f}\n", "TOPS/W radio:", TOPS_per_W_radio);

    if (detail) {
        auto energy_report_items = energy_reporter_compare.getEnergyReportItem(module_name, 0);
        energy_report_items.insert(energy_report_items.begin(),
                                   EnergyReportItem{"module", "total diff(radio)", "static diff(radio)",
                                                    "dynamic diff(radio)", "activity time diff(radio)"});

        unsigned int name_width = 0, total_width = 0, static_width = 0, dynamic_width = 0, activity_width = 0;
        for (const auto& [name, total_energy, static_energy, dynamic_energy, activity_time] : energy_report_items) {
            name_width = MAX(name_width, name.length());
            total_width = MAX(total_width, total_energy.length());
            static_width = MAX(static_width, static_energy.length());
            dynamic_width = MAX(dynamic_width, dynamic_energy.length());
            activity_width = MAX(activity_width, activity_time.length());
        }

        os << "Energy Compare report form:\n";
        for (const auto& [name, total_energy, static_energy, dynamic_energy, activity_time] : energy_report_items) {
            os << fmt::format("    {:<{}}", name, name_width);
            os << fmt::format("    {:<{}}", total_energy, total_width);
            os << fmt::format("    {:<{}}", static_energy, static_width);
            os << fmt::format("    {:<{}}", dynamic_energy, dynamic_width);
            os << fmt::format("    {:<{}}\n", activity_time, activity_width);
        }
    }
}

EnergyReporter::EnergyReporter(double total_energy, double static_energy, double dynamic_energy, double activity_time)
    : total_energy_(total_energy)
    , static_energy_(static_energy)
    , dynamic_energy_(dynamic_energy)
    , activity_time_(activity_time) {}

EnergyReporter::EnergyReporter(const EnergyCounter& energy_counter)
    : total_energy_(energy_counter.getTotalEnergyPJ())
    , static_energy_(energy_counter.getStaticEnergyPJ())
    , dynamic_energy_(energy_counter.getDynamicEnergyPJ())
    , activity_time_(energy_counter.getActivityTime()) {}

void EnergyReporter::addSubModule(std::string name, EnergyReporter sub_module) {
    total_energy_ += sub_module.total_energy_;
    static_energy_ += sub_module.static_energy_;
    dynamic_energy_ += sub_module.dynamic_energy_;
    activity_time_ += sub_module.activity_time_;
    if (auto sub_module_found = sub_modules_.find(name); sub_module_found != sub_modules_.end()) {
        sub_module_found->second.accumulate(sub_module, true);
    } else {
        sub_modules_.emplace(std::move(name), std::move(sub_module));
    }
}

std::vector<EnergyReportItem> EnergyReporter::getEnergyReportItem(const std::string& module_name, double all_energy,
                                                                  double total_latency, int indent_level) {
    std::vector<EnergyReportItem> items;

    std::string name;
    for (int i = 0; i < indent_level * ENERGY_REPORT_INDENT_SPACE_NUM; i++) {
        name += ' ';
    }
    name += module_name;

    std::string total_energy_str = fmt::format("{:.3f}pJ ({:.2f}%)", total_energy_,
                                               (all_energy == 0.0 ? 0.0 : (total_energy_ / all_energy) * 100));
    std::string static_energy_str = fmt::format("{:.3f}pJ ({:.2f}%)", static_energy_,
                                                (all_energy == 0.0 ? 0.0 : (static_energy_ / all_energy) * 100));
    std::string dynamic_energy_str = fmt::format("{:.3f}pJ ({:.2f}%)", dynamic_energy_,
                                                 (all_energy == 0.0 ? 0.0 : (dynamic_energy_ / all_energy) * 100));
    std::string activity_time_str = fmt::format("{:.3f}ns ({:.2f}%)", activity_time_,
                                                (total_latency == 0.0 ? 0.0 : (activity_time_ / total_latency) * 100));
    items.emplace_back(
        EnergyReportItem{name, total_energy_str, static_energy_str, dynamic_energy_str, activity_time_str});

    for (auto& [sub_module_name, sub_module] : sub_modules_) {
        auto sub_module_items =
            sub_module.getEnergyReportItem(sub_module_name, all_energy, total_latency, indent_level + 1);
        items.insert(items.end(), sub_module_items.begin(), sub_module_items.end());
    }

    return std::move(items);
}

double EnergyReporter::getTotalEnergyPJ() const {
    return total_energy_;
}

double EnergyReporter::getDynamicEnergyPJ() const {
    return dynamic_energy_;
}

void EnergyReporter::accumulate(const EnergyReporter& another, bool same_simulation) {
    total_energy_ += another.total_energy_;
    static_energy_ += another.static_energy_;
    dynamic_energy_ += another.dynamic_energy_;
    if (same_simulation) {
        activity_time_ = std::max(activity_time_, another.activity_time_);
    } else {
        activity_time_ += another.activity_time_;
    }

    for (auto& [name, sub_module] : another.sub_modules_) {
        if (auto sub_module_found = sub_modules_.find(name); sub_module_found != sub_modules_.end()) {
            sub_module_found->second.accumulate(sub_module, same_simulation);
        } else {
            sub_modules_.emplace(name, sub_module);
        }
    }
}

Reporter::Reporter(double latency_ms, std::string module_name, const EnergyReporter& energy_reporter, int OP_count)
    : latency_(latency_ms)
    , total_energy_(energy_reporter.getTotalEnergyPJ())
    , module_name_(std::move(module_name))
    , energy_reporter_(energy_reporter)
    , OP_count_(OP_count) {
    average_power_ = (latency_ms == 0.0 ? 0.0 : (energy_reporter.getTotalEnergyPJ() / (latency_ms * 1e6)));
    TOPS_ = (latency_ms == 0.0 ? 0.0 : (1.0 * OP_count / (latency_ms / 1e3) / TERA));
    TOPS_per_W_ = (average_power_ == 0.0 ? 0.0 : (TOPS_ / (average_power_ / 1e3)));
}

void Reporter::report(std::ostream& os, bool detail) {
    os << "Simulation Result:\n";
    os << fmt::format("  - {:<20}{:.6} ms\n", "latency:", latency_);
    os << fmt::format("  - {:<20}{:.4f} mW\n", "average power:", average_power_);
    os << fmt::format("  - {:<20}{:.4f} pJ/it\n", "total energy:", total_energy_);
    os << fmt::format("  - {:<20}{:.4f}\n", "TOPS:", TOPS_);
    os << fmt::format("  - {:<20}{:.4f}\n", "TOPS/W:", TOPS_per_W_);
    os << fmt::format("  - {:<20}{}\n", "OP_count:", OP_count_);

    if (detail) {
        os << "Energy report form:\n";
        reportEnergyForm(os);
    }
}

void Reporter::reportEnergyForm(std::ostream& os) {
    auto energy_report_items = energy_reporter_.getEnergyReportItem(module_name_, total_energy_, latency_ * 1e6, 0);
    energy_report_items.insert(
        energy_report_items.begin(),
        EnergyReportItem{"module", "total energy", "static energy", "dynamic energy", "activity time"});

    unsigned int name_width = 0, total_width = 0, static_width = 0, dynamic_width = 0, activity_width = 0;
    for (const auto& [name, total_energy, static_energy, dynamic_energy, activity_time] : energy_report_items) {
        name_width = MAX(name_width, name.length());
        total_width = MAX(total_width, total_energy.length());
        static_width = MAX(static_width, static_energy.length());
        dynamic_width = MAX(dynamic_width, dynamic_energy.length());
        activity_width = MAX(activity_width, activity_time.length());
    }
    for (const auto& [name, total_energy, static_energy, dynamic_energy, activity_time] : energy_report_items) {
        os << fmt::format("    {:<{}}", name, name_width);
        os << fmt::format("    {:<{}}", total_energy, total_width);
        os << fmt::format("    {:<{}}", static_energy, static_width);
        os << fmt::format("    {:<{}}", dynamic_energy, dynamic_width);
        os << fmt::format("    {:<{}}\n", activity_time, activity_width);
    }
}


double Reporter::getAveragePowerMW() const {
    return average_power_;
}

double Reporter::getTotalEnergyPJ() const {
    return total_energy_;
}

double Reporter::getLatencyNs() const {
    return latency_ * 1e6;
}

double Reporter::getDynamicEnergyPJ() const {
    return energy_reporter_.getDynamicEnergyPJ();
}

Reporter& Reporter::operator+=(const Reporter& another) {
    latency_ += another.latency_;
    total_energy_ += another.total_energy_;
    OP_count_ += another.OP_count_;

    average_power_ = (latency_ == 0.0 ? 0.0 : (total_energy_ / (latency_ * 1e6)));
    TOPS_ = (latency_ == 0.0 ? 0.0 : (1.0 * OP_count_ / (latency_ / 1e3) / TERA));
    TOPS_per_W_ = (average_power_ == 0.0 ? 0.0 : (TOPS_ / (average_power_ / 1e3)));

    energy_reporter_.accumulate(another.energy_reporter_, false);

    return *this;
}

EnergyReporterCompare EnergyReporter::compare(const EnergyReporter& r2) const {
    EnergyReporterCompare c;

    // diff
    c.total_energy_diff = total_energy_ - r2.total_energy_;
    c.static_energy_diff = static_energy_ - r2.static_energy_;
    c.dynamic_energy_diff = dynamic_energy_ - r2.dynamic_energy_;
    c.activity_time_diff = activity_time_ - r2.activity_time_;

    // radio
    c.total_energy_radio = r2.total_energy_ == 0.0 ? 0.0 : total_energy_ / r2.total_energy_;
    c.static_energy_radio = r2.static_energy_ == 0.0 ? 0.0 : static_energy_ / r2.static_energy_;
    c.dynamic_energy_radio = r2.dynamic_energy_ == 0.0 ? 0.0 : dynamic_energy_ / r2.dynamic_energy_;
    c.activity_time_radio = r2.activity_time_ == 0.0 ? 0.0 : activity_time_ / r2.activity_time_;

    for (auto& [name1, sub_module1] : sub_modules_) {
        if (auto sub_module2_found = r2.sub_modules_.find(name1); sub_module2_found != r2.sub_modules_.end()) {
            auto& sub_module2 = sub_module2_found->second;
            auto sub_module_cmp = sub_module1.compare(sub_module2);
            c.sub_modules.emplace(name1, std::move(sub_module_cmp));
        }
    }

    return std::move(c);
}

ReporterCompare Reporter::compare(const Reporter& r2) const {
    ReporterCompare r;

    // diff
    r.latency_diff = latency_ - r2.latency_;
    r.average_power_diff = average_power_ - r2.average_power_;
    r.total_energy_diff = total_energy_ - r2.total_energy_;
    r.TOPS_diff = TOPS_ - r2.TOPS_;
    r.TOPS_per_W_diff = TOPS_per_W_ - r2.TOPS_per_W_;

    // radio
    r.latency_radio = r2.latency_ == 0.0 ? 0.0 : latency_ / r2.latency_;
    r.average_power_radio = r2.average_power_ == 0.0 ? 0.0 : average_power_ / r2.average_power_;
    r.total_energy_radio = r2.total_energy_ == 0.0 ? 0.0 : total_energy_ / r2.total_energy_;
    r.TOPS_radio = r2.TOPS_ == 0 ? 0.0 : TOPS_ / r2.TOPS_;
    r.TOPS_per_W_radio = r2.TOPS_per_W_ == 0 ? 0.0 : TOPS_per_W_ / r2.TOPS_per_W_;

    r.module_name = module_name_;
    r.energy_reporter_compare = energy_reporter_.compare(r2.energy_reporter_);

    return std::move(r);
}

void Reporter::setOPCount(double OP_count) {
    this->OP_count_ = static_cast<int>(OP_count);
    this->TOPS_ = (latency_ == 0.0 ? 0.0 : (1.0 * OP_count / (latency_ / 1e3) / TERA));
    this->TOPS_per_W_ = (average_power_ == 0.0 ? 0.0 : (TOPS_ / (average_power_ / 1e3)));
}

#undef MAX

}  // namespace pimsim
