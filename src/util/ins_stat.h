//
// Created by wyk on 2024/8/18.
//

#pragma once
#include <string>
#include <unordered_map>

#include "config/config.h"
#include "macro_scope.h"
#include "nlohmann/json.hpp"

namespace pimsim {

struct ScalarInsStat {
    int total{0};
    int rr{0}, ri{0}, store{0}, load{0};
    int general_li{0}, special_li{0}, special_general_assign{0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarInsStat);
    bool operator==(const ScalarInsStat& another) const;

    void addInsCount(int type, int opcode);
};

struct SIMDInsStat {
    int total{0};
    std::unordered_map<std::string, int> ins_count{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInsStat);
    bool operator==(const SIMDInsStat& another) const;

    void addInsCount(int opcode, const SIMDUnitConfig& config);
};

struct TransferInsStat {
    int total{0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TransferInsStat);
    bool operator==(const TransferInsStat& another) const;

    void addInsCount();
};

struct PimInsStat {
    int total{0};
    int pim_compute{0}, pim_set{0}, pim_output{0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(PimInsStat);
    bool operator==(const PimInsStat& another) const;

    void addInsCount(int type);
};

struct ControlInsStat {
    int total{0};
    int branch{0}, jump{0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ControlInsStat);
    bool operator==(const ControlInsStat& another) const;

    void addInsCount(int type);
};

struct InsStat {
    int total{0};

    ScalarInsStat scalar{};
    TransferInsStat trans{};
    ControlInsStat ctr{};
    PimInsStat pim{};
    SIMDInsStat simd{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(InsStat);
    bool operator==(const InsStat& another) const;

    void addInsCount(int class_code, int type, int opcode, const CoreConfig& core_config);
};

}  // namespace pimsim
