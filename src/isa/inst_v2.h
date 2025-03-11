//
// Created by wyk on 2025/3/3.
//

#pragma once
#include <string>

#include "isa_v2.h"
#include "util/macro_scope.h"

namespace cimsim {

struct InstV2 {
    int opcode{0};
    int rs{-1}, rt{-1}, rd{-1}, re{-1}, rf{-1};
    int funct{0}, imm{0};

    bool GRP{false}, GRP_I{false}, SP_V{false}, SP_B{false}, GRP_B{false}, OSUM{false}, OSUM_MOV{false};

    [[nodiscard]] OPCODE_CLASS getOpcodeClass() const;

    [[nodiscard]] OPCODE getOpcodeEnum() const;

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] std::string toJsonString() const;
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(InstV2)

}  // namespace cimsim
