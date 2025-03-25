//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <ostream>

#include "better-enums/enum.h"
#include "util/macro_scope.h"

namespace cimsim {

BETTER_ENUM(ExecuteUnitType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            none = 0, scalar, simd, reduce, transfer, cim_compute, cim_control, control)

struct InstructionPayload {
    int pc{-1};
    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};

    [[nodiscard]] bool valid() const {
        return pc != -1 && ins_id != -1;
    }

    void clear() {
        pc = -1;
        ins_id = -1;
    }

    friend std::ostream& operator<<(std::ostream& out, const InstructionPayload& ins) {
        out << "pc: " << ins.pc << ", ins id: " << ins.ins_id << ", unit type: " << ins.unit_type << "\n";
        return out;
    }

    bool operator==(const InstructionPayload& another) const {
        return pc == another.pc && ins_id == another.ins_id && unit_type == another.unit_type;
    }

    DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT_INTRUSIVE(InstructionPayload, pc, ins_id)
};

}  // namespace cimsim
