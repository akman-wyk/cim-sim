//
// Created by wyk on 24-6-17.
//

#pragma once
#include "better-enums/enum.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace cimsim {

BETTER_ENUM(SimMode, int,  // NOLINT(*-no-recursion, *-explicit-constructor)
            run_until_time = 0, run_one_round = 1, other = 2)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(SimMode)

BETTER_ENUM(DataMode, int,  // NOLINT(*-no-recursion, *-explicit-constructor)
            real_data = 0, not_real_data = 1, other = 2)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(DataMode)

BETTER_ENUM(MemoryType, int,  // NOLINT(*-no-recursion, *-explicit-constructor)
            ram = 0, reg_buffer = 1, other = 2)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(MemoryType)

BETTER_ENUM(SIMDInputType, int,  // NOLINT(*-no-recursion, *-explicit-constructor)
            vector = 0, scalar = 1, other = 2)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(SIMDInputType)

BETTER_ENUM(CimASMode, int,  // NOLINT(*-no-recursion, *-explicit-constructor)
            intergroup = 1, intragroup = 2, other = 3)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(CimASMode)

}  // namespace cimsim