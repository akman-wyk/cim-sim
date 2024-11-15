//
// Created by wyk on 2024/7/4.
//

#pragma once
#include "better-enums/enum.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace pimsim {

BETTER_ENUM(MemoryAccessType, int, read = 1, write = 2)  // NOLINT(*-explicit-constructor, *-no-recursion)

BETTER_ENUM(ScalarOperator, int,  // NOLINT(*-explicit-constructor)
            add, sub, mul, div, sll, srl, sra, mod, min, max, s_and, s_or, eq, ne, gt, lt, lui, load, store, assign)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(ScalarOperator)

BETTER_ENUM(PimOutputType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            only_output = 1, output_sum, output_sum_move)

BETTER_ENUM(ExecuteUnitType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            none = 0, scalar, simd, transfer, pim_compute, pim_output, pim_set, pim_transfer, pim_load, control)

BETTER_ENUM(TransferType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            local_trans = 0, global_load, global_store, send, receive)
DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(TransferType)

}  // namespace pimsim
