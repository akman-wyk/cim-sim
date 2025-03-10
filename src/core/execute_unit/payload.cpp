//
// Created by wyk on 2025/3/10.
//

#include "payload.h"

namespace pimsim {

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

std::stringstream& operator<<(std::stringstream& out, const std::array<int, 4>& arr) {
    out << arr[0];
    for (int i = 1; i < arr.size(); i++) {
        out << ", " << arr[i];
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::vector<int>& list) {
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it != list.begin()) {
            out << ", ";
        }
        out << *it;
    }
    return out;
}

std::stringstream& operator<<(std::stringstream& out, const std::unordered_map<int, int>& map) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) {
            out << ", ";
        }
        out << it->first << ": " << it->second;
    }
    return out;
}

}  // namespace pimsim
