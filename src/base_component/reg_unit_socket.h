//
// Created by wyk on 2024/7/19.
//

#pragma once
#include "core/payload/payload.h"

namespace pimsim {

class RegUnit;

class RegUnitSocket {
public:
    RegUnitSocket() = default;

    void bindRegUnit(RegUnit* reg_unit);

    int getSpecialBoundGeneralId(int special_id);

    void writeRegister(const RegUnitWritePayload& write_req);

private:
    RegUnit* reg_unit_{nullptr};
};

}  // namespace pimsim
