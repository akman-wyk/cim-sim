//
// Created by wyk on 2024/8/2.
//

#include "test_payload.h"

namespace cimsim {

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TestExpectedInfo, time_ns, energy_pj)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreTestRegisterInfo, check, general_reg_expected_values,
                                               special_reg_expected_values)

}
