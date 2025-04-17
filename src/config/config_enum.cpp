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

void to_json(nlohmann::ordered_json& j, const InstProfilerOperator& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, InstProfilerOperator& m) {
    const auto str = j.get<std::string>();
    m = InstProfilerOperator::_from_string(str.c_str());
}

}  // namespace cimsim