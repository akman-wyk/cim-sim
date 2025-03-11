//
// Created by wyk on 2024/8/18.
//

#include "ins_stat.h"

#include "core/core.h"
#include "isa/isa.h"

namespace cimsim {

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ScalarInsStat, total, rr, ri, store, load, general_li, special_li,
                                               special_general_assign)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDInsStat, total, ins_count)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferInsStat, total)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimInsStat, total, cim_compute, cim_set, cim_output)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ControlInsStat, total, branch, jump)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(InsStat, total, scalar, trans, ctr, cim, simd)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(ScalarInsStat, total, rr, ri, store, load, general_li, special_li,
                                  special_general_assign)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(SIMDInsStat, total)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(TransferInsStat, total)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(CimInsStat, total, cim_compute, cim_set, cim_output)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(ControlInsStat, total, branch, jump)

DEFINE_CIM_PAYLOAD_EQUAL_OPERATOR(InsStat, total, scalar, trans, ctr, cim, simd)

void ScalarInsStat::addInsCount(int type, int opcode) {
    total++;
    if (type == ScalarInstType::RR) {
        rr++;
    } else if (type == ScalarInstType::RI) {
        ri++;
    } else if (type == ScalarInstType::SL) {
        if (opcode == ScalarSLInstOpcode::load_global || opcode == ScalarSLInstOpcode::load_local) {
            load++;
        } else {
            store++;
        }
    } else {
        if (opcode == ScalarAssignInstOpcode::li_general) {
            general_li++;
        } else if (opcode == ScalarAssignInstOpcode::li_special) {
            special_li++;
        } else {
            special_general_assign++;
        }
    }
}

void SIMDInsStat::addInsCount(int opcode, const SIMDUnitConfig& config) {
    total++;

    std::string ins_name = std::to_string(opcode);
    for (auto& simd_ins_config : config.instruction_list) {
        if (simd_ins_config.opcode == static_cast<unsigned int>(opcode)) {
            ins_name = simd_ins_config.name;
            break;
        }
    }

    if (auto found = ins_count.find(ins_name); found == ins_count.end()) {
        ins_count.emplace(ins_name, 1);
    } else {
        found->second++;
    }
}

void TransferInsStat::addInsCount() {
    total++;
}

void CimInsStat::addInsCount(int type) {
    total++;
    if (type == CIMInstType::compute) {
        cim_compute++;
    } else if (type == CIMInstType::set) {
        cim_set++;
    } else if (type == CIMInstType::output) {
        cim_output++;
    }
}

void ControlInsStat::addInsCount(int type) {
    total++;
    if (type == ControlInstType::jmp) {
        jump++;
    } else {
        branch++;
    }
}

void InsStat::addInsCount(int class_code, int type, int opcode, const SIMDUnitConfig& simd_unit_config) {
    total++;
    if (class_code == InstClass::cim) {
        cim.addInsCount(type);
    } else if (class_code == InstClass::simd) {
        simd.addInsCount(opcode, simd_unit_config);
    } else if (class_code == InstClass::scalar) {
        scalar.addInsCount(type, opcode);
    } else if (class_code == InstClass::transfer) {
        trans.addInsCount();
    } else {
        ctr.addInsCount(type);
    }
}

}  // namespace cimsim
