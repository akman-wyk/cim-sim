//
// Created by wyk on 24-6-17.
//

#include "config_enum.h"

#include <string>

namespace cimsim {

DEFINE_ENUM_FROM_TO_JSON_FUNCTION(SimMode, run_until_time, run_one_round, other)

DEFINE_ENUM_FROM_TO_JSON_FUNCTION(DataMode, real_data, not_real_data, other)

DEFINE_ENUM_FROM_TO_JSON_FUNCTION(MemoryType, ram, reg_buffer, other)

DEFINE_ENUM_FROM_TO_JSON_FUNCTION(SIMDInputType, vector, scalar, other)

DEFINE_ENUM_FROM_TO_JSON_FUNCTION(CimASMode, intergroup, intragroup, other)

}  // namespace cimsim