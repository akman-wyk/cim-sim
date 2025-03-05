//
// Created by wyk on 2025/2/28.
//

#pragma once
#include <cstdint>
#include <string>

#include "util/macro_scope.h"

namespace pimsim {

struct InstV3 {
    uint32_t binary_{};
    std::string asm_{};

    [[nodiscard]] int getOpcode() const {
        return getInstField(OPCODE_OFFSET, OPCODE_LENGTH);
    }

    [[nodiscard]] int getR1() const {
        return getInstField(R1_OFFSET, REG_LENGTH);
    }

    [[nodiscard]] int getR2() const {
        return getInstField(R2_OFFSET, REG_LENGTH);
    }

    [[nodiscard]] int getR3() const {
        return getInstField(R3_OFFSET, REG_LENGTH);
    }

    [[nodiscard]] int getR4() const {
        return getInstField(R4_OFFSET, REG_LENGTH);
    }

    [[nodiscard]] int getR5() const {
        return getInstField(R5_OFFSET, REG_LENGTH);
    }

    [[nodiscard]] int getFunctType2() const {
        return getInstField(FUNCT_TYPE2_OFFSET, FUNCT_TYPE2_LENGTH);
    }

    [[nodiscard]] int getFunctType3() const {
        return getInstField(FUNCT_TYPE3_OFFSET, FUNCT_TYPE3_LENGTH);
    }

    [[nodiscard]] int getImmType3() const {
        return getInstField(IMM_OFFSET, IMM_TYPE3_LENGTH);
    }

    [[nodiscard]] int getImmType4() const {
        return getInstField(IMM_OFFSET, IMM_TYPE4_LENGTH);
    }

    [[nodiscard]] int getImmType5() const {
        return getInstField(IMM_OFFSET, IMM_TYPE5_LENGTH);
    }

    [[nodiscard]] int getImmType6() const {
        return getInstField(IMM_OFFSET, IMM_TYPE6_LENGTH);
    }

    [[nodiscard]] bool getFlag(int position) const {
        return getInstField(position, 1) == 1;
    }

private:
    enum FieldOffset : uint32_t {
        OPCODE_OFFSET = 26,
        R1_OFFSET = 21,
        R2_OFFSET = 16,
        R3_OFFSET = 11,
        R4_OFFSET = 6,
        R5_OFFSET = 1,
        FUNCT_TYPE2_OFFSET = 0,
        FUNCT_TYPE3_OFFSET = 11,
        IMM_OFFSET = 0,
    };

    enum FieldLength : uint32_t {
        OPCODE_LENGTH = 6,
        REG_LENGTH = 5,
        FUNCT_TYPE2_LENGTH = 5,
        FUNCT_TYPE3_LENGTH = 5,
        IMM_TYPE3_LENGTH = 11,
        IMM_TYPE4_LENGTH = 16,
        IMM_TYPE5_LENGTH = 21,
        IMM_TYPE6_LENGTH = 26,
    };

    [[nodiscard]] int getInstField(uint32_t offset, uint32_t length) const {
        return static_cast<int>((binary_ >> offset) & ((1 << length) - 1));
    }
};

DECLARE_TYPE_FROM_TO_JSON_FUNCTION_NON_INTRUSIVE(InstV3)

}  // namespace pimsim
