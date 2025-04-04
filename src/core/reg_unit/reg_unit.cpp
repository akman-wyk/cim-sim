//
// Created by wyk on 2024/7/19.
//

#include "reg_unit.h"

#include "fmt/format.h"

namespace cimsim {

RegUnit::RegUnit(const sc_module_name &name, const RegisterUnitConfig &config, const BaseInfo &base_info)
    : BaseModule(name, base_info), config_(config) {
    for (const auto &[special, general] : config_.special_register_binding) {
        special_bind_map_.emplace(special, general);
    }

    energy_counter_.setStaticPowerMW(config_.static_power_mW);
}

int RegUnit::getSpecialBoundGeneralId(int special_id) const {
    auto found = special_bind_map_.find(special_id);
    if (found == special_bind_map_.end()) {
        return -1;
    }
    return found->second;
}

void RegUnit::writeRegister(const cimsim::RegUnitWritePayload &write_req) {
    if (write_req.special) {
        special_regs_[write_req.id] = write_req.value;
    } else {
        general_regs_[write_req.id] = write_req.value;
    }
}

int RegUnit::readRegister(int id, bool special) const {
    if (!special) {
        return general_regs_[id];
    }

    if (int special_bound_general_id = getSpecialBoundGeneralId(id); special_bound_general_id != -1) {
        return general_regs_[special_bound_general_id];
    }
    return special_regs_[id];
}

bool RegUnit::checkRegValues(const std::array<int, GENERAL_REG_NUM> &general_reg_expected_values,
                             const std::array<int, SPECIAL_REG_NUM> &special_reg_expected_values) {
    for (int i = 0; i < GENERAL_REG_NUM; i++) {
        if (general_regs_[i] != general_reg_expected_values[i]) {
            std::cout << fmt::format("index: {}, actual: {}, expected: {}", i, general_regs_[i],
                                     general_reg_expected_values[i])
                      << std::endl;
            return false;
        }
    }

    for (int i = 0; i < SPECIAL_REG_NUM; i++) {
        if (special_regs_[i] != special_reg_expected_values[i]) {
            std::cout << fmt::format("index: {}, actual: {}, expected: {}", i, special_regs_[i],
                                     special_reg_expected_values[i])
                      << std::endl;
            return false;
        }
    }
    return true;
}

std::string RegUnit::getGeneralRegistersString() const {
    std::stringstream ss;
    for (int i = 0; i < GENERAL_REG_NUM; i++) {
        ss << general_regs_[i];
        if (i != GENERAL_REG_NUM - 1) {
            ss << ", ";
        }
    }
    return ss.str();
}

}  // namespace cimsim
