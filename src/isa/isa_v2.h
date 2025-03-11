//
// Created by wyk on 2025/3/5.
//

#pragma once

#include "better-enums/enum.h"

namespace cimsim {

BETTER_ENUM(OPCODE_CLASS, int,  // NOLINT(*-explicit-constructor)
            CIM = 0b000000, VEC_OP = 0b010000, SC = 0b100000, TRANS = 0b110000, CONTROL = 0b111000)

BETTER_ENUM(OPCODE, int,  // NOLINT(*-explicit-constructor)
            CIM_MVM = 0b000000, CIM_CFG = 0b000100, CIM_OUT = 0b001000,

            VEC_OP = 0b010000,

            SC_RR = 0b100000, SC_RI = 0b100100, SC_LD = 0b101000, SC_ST = 0b101001, SC_LDG = 0b101010,
            SC_STG = 0b101011, G_LI = 0b101100, S_LI = 0b101101, GS_MOV = 0b101110, SG_MOV = 0b101111,

            MEM_CPY = 0b110000, SEND = 0b110100, RECV = 0b110110,

            BEQ = 0b111000, BNE = 0b111001, BGT = 0b111010, BLT = 0b111011, JMP = 0b111100, WAIT = 0b111101,
            BARRIER = 0b111110)

BETTER_ENUM(OPCODE_MASK, int,  // NOLINT(*-explicit-constructor)
            INST_CLASS_2BIT = 0b110000, INST_CLASS_3BIT = 0b111000,

            INST_TYPE_2BIT = 0b001100, INST_TYPE_3BIT = 0b000111,

            VEC_TYPE = 0b110011, TRANS_TYPE_4BIT = 0b111100, TRANS_TYPE_5BIT = 0b111110)

BETTER_ENUM(SC_RR_FUNCT, int,  // NOLINT(*-explicit-constructor)
            SC_ADD = 0b000000, SC_SUB = 0b000001, SC_MUL = 0b000010, SC_DIV = 0b000011, SC_SLL = 0b000100,
            SC_SRL = 0b000101, SC_SRA = 0b000110, SC_MOD = 0b000111, SC_MIN = 0b001000, SC_MAX = 0b001001,
            SC_AND = 0b001010, SC_OR = 0b001011, SC_EQ = 0b001100, SC_NE = 0b001101, SC_GT = 0b001110, SC_LT = 0b001111)

BETTER_ENUM(SC_RI_FUNCT, int,  // NOLINT(*-explicit-constructor)
            SC_ADDI = 0b000000, SC_SUBI = 0b000001, SC_MULI = 0b000010, SC_DIVI = 0b000011, SC_SLLI = 0b000100,
            SC_SRLI = 0b000101, SC_SRAI = 0b000110, SC_MODI = 0b000111, SC_MINI = 0b001000, SC_MAXI = 0b001001,
            SC_ANDI = 0b001010, SC_ORI = 0b001011, SC_EQI = 0b001100, SC_NEI = 0b001101, SC_GTI = 0b001110,
            SC_LTI = 0b001111)

}  // namespace cimsim
