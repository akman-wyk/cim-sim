//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <ostream>

#include "better-enums/enum.h"
#include "config/config_enum.h"
#include "isa/isa_v2.h"
#include "util/macro_scope.h"

namespace cimsim {

BETTER_ENUM(ExecuteUnitType, int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            none = 0, scalar, simd, reduce, transfer, cim_compute, cim_control, control)

struct InstructionPayload {
    int pc{-1};
    int ins_id{-1};
    ExecuteUnitType unit_type{ExecuteUnitType::none};

    OPCODE inst_opcode{OPCODE::CIM_MVM};
    std::string_view inst_group_tag{};

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

BETTER_ENUM(DataPathType, unsigned int,  // NOLINT(*-explicit-constructor, *-no-recursion)
            none = 0, intra_core_bus = 1, inter_core_bus = 2, local_dedicated_data_path = 3)

struct DataPathPayload {
    DataPathType type{DataPathType::none};
    unsigned int local_dedicated_data_path_id{0};

    [[nodiscard]] unsigned int getUniqueId() const {
        return (local_dedicated_data_path_id << 2) | type._to_integral();
    }

    friend std::ostream& operator<<(std::ostream& out, const DataPathPayload& data_path_payload) {
        out << "type: " << data_path_payload.type
            << ", local_dedicated_data_path_id: " << data_path_payload.local_dedicated_data_path_id << "\n";
        return out;
    }

    bool operator==(const DataPathPayload& another) const {
        return type == another.type && local_dedicated_data_path_id == another.local_dedicated_data_path_id;
    }

    [[nodiscard]] bool conflictWith(const DataPathPayload& another) const {
        if (type == +DataPathType::local_dedicated_data_path &&
            another.type == +DataPathType::local_dedicated_data_path) {
            return local_dedicated_data_path_id == another.local_dedicated_data_path_id;
        }
        return type == another.type;
    }
};

}  // namespace cimsim
