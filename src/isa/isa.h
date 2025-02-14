//
// Created by wyk on 2024/7/19.
//

#pragma once

#include "better-enums/enum.h"

namespace pimsim {

BETTER_ENUM(InstClass, int,  // NOLINT(*-explicit-constructor)
            pim = 0b00, simd = 0b01, scalar = 0b10, transfer = 0b110, control = 0b111)

BETTER_ENUM(PIMInstType, int,  // NOLINT(*-explicit-constructor)
            compute = 0b00, set = 0b01, output = 0b10)

BETTER_ENUM(ScalarInstType, int,  // NOLINT(*-explicit-constructor)
            RR = 0b00, RI = 0b01, SL = 0b10, Assign = 0b11)

BETTER_ENUM(TransferInstType, int,  // NOLINT(*-explicit-constructor)
            trans = 0b0, send = 0b10, receive = 0b11)

BETTER_ENUM(ControlInstType, int,  // NOLINT(*-explicit-constructor)
            beq = 0b000, bne = 0b001, bgt = 0b010, blt = 0b011, jmp = 0b100, wait = 0b101, barrier = 0b110)

BETTER_ENUM(ScalarRRInstOpcode, int,  // NOLINT(*-explicit-constructor)
            add = 0b0000, sub = 0b0001, mul = 0b0010, div = 0b0011, sll = 0b0100, srl = 0b0101, sra = 0b0110,
            mod = 0b0111, min = 0b1000, max = 0b1001, s_and = 0b1010, s_or = 0b1011, eq = 0b1100, ne = 0b1101,
            gt = 0b1110, lt = 0b1111)

BETTER_ENUM(ScalarRIInstOpcode, int,  // NOLINT(*-explicit-constructor)
            addi = 0b0000, subi = 0b0001, muli = 0b0010, divi = 0b0011, slli = 0b0100, srli = 0b0101, srai = 0b0110,
            modi = 0b0111, mini = 0b1000, maxi = 0b1001, andi = 0b1010, ori = 0b1011, eqi = 0b1100, nei = 0b1101,
            gti = 0b1110, lti = 0b1111)

BETTER_ENUM(ScalarSLInstOpcode, int,  // NOLINT(*-explicit-constructor)
            load_local = 0b00, store_local = 0b01, load_global = 0b10, store_global = 0b11)

BETTER_ENUM(ScalarAssignInstOpcode, int,  // NOLINT(*-explicit-constructor)
            li_general = 0b00, li_special = 0b01, assign_general_to_special = 0b10, assign_special_to_general = 0b11)

}  // namespace pimsim