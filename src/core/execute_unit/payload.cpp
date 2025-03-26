//
// Created by wyk on 2025/3/10.
//

#include "payload.h"

namespace cimsim {

void to_json(nlohmann::ordered_json& j, const ScalarOperator& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, ScalarOperator& m) {
    const auto str = j.get<std::string>();
    m = ScalarOperator::_from_string(str.c_str());
}

void to_json(nlohmann::ordered_json& j, const TransferType& m) {
    j = m._to_string();
}

void from_json(const nlohmann::ordered_json& j, TransferType& m) {
    const auto str = j.get<std::string>();
    m = TransferType::_from_string(str.c_str());
}

}  // namespace cimsim
