//
// Created by wyk on 2024/7/19.
//

#include "reg_unit_socket.h"

#include "core/reg_unit/reg_unit.h"

namespace pimsim {

void RegUnitSocket::bindRegUnit(pimsim::RegUnit *reg_unit) {
    reg_unit_ = reg_unit;
}

int RegUnitSocket::getSpecialBoundGeneralId(int special_id) {
    return reg_unit_->getSpecialBoundGeneralId(special_id);
}

void RegUnitSocket::writeRegister(const pimsim::RegUnitWritePayload &write_req) {
    reg_unit_->writeRegister(write_req);
}

}  // namespace pimsim
