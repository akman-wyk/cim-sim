//
// Created by wyk on 2024/8/2.
//

#pragma once
#include "config/constant.h"
#include "util/macro_scope.h"

namespace cimsim {

struct TestExpectedInfo {
    double time_ns{0.0};
    double energy_pj{0.0};
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(TestExpectedInfo)

struct CoreTestRegisterInfo {
    bool check{false};
    std::array<int, GENERAL_REG_NUM> general_reg_expected_values{};
    std::array<int, SPECIAL_REG_NUM> special_reg_expected_values{};
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(CoreTestRegisterInfo)

}  // namespace cimsim
