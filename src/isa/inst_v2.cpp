//
// Created by wyk on 2025/3/3.
//

#include "inst_v2.h"

#include <bitset>

#include "fmt/format.h"

namespace cimsim {

#define TO_STRING_WRITE_FLAG(flag) \
    if (flag) {                    \
        ss << " " << #flag;        \
    }

#define TO_STRING_WRITE_FLAGS(...) CIM_PASTE(TO_STRING_WRITE_FLAG, DELIMITER_SPACE, __VA_ARGS__)

#define TYPE_TO_JSON_WRITE_FLAG(flag)                  \
    if (nlohmann_json_t.flag) {                        \
        nlohmann_json_j[#flag] = nlohmann_json_t.flag; \
    }

#define TYPE_TO_JSON_WRITE_FLAGS(...) CIM_PASTE(TYPE_TO_JSON_WRITE_FLAG, DELIMITER_SPACE, __VA_ARGS__)

#define INST_V2_TO_JSON_STR_WRITE_REG(reg)            \
    if (reg >= 0) {                                   \
        ss << fmt::format(", \"{}\": {}", #reg, reg); \
    }

#define INST_V2_TO_JSON_STR_WRITE_REGS(...) CIM_PASTE(INST_V2_TO_JSON_STR_WRITE_REG, DELIMITER_SPACE, __VA_ARGS__)

#define INST_V2_TO_JSON_STR_WRITE_FLAG(flag)            \
    if (flag) {                                         \
        ss << fmt::format(", \"{}\": {}", #flag, flag); \
    }

#define INST_V2_TO_JSON_STR_WRITE_FLAGS(...) CIM_PASTE(INST_V2_TO_JSON_STR_WRITE_FLAG, DELIMITER_SPACE, __VA_ARGS__)

OPCODE_CLASS InstV2::getOpcodeClass() const {
    if ((opcode & OPCODE_MASK::INST_CLASS_2BIT) != OPCODE_CLASS::TRANS) {
        return OPCODE_CLASS::_from_integral(opcode & OPCODE_MASK::INST_CLASS_2BIT);
    }
    return OPCODE_CLASS::_from_integral(opcode & OPCODE_MASK::INST_CLASS_3BIT);
}

OPCODE InstV2::getOpcodeEnum() const {
    if ((opcode & OPCODE_MASK::VEC_TYPE) == OPCODE::VEC_OP) {
        return OPCODE::VEC_OP;
    }
    if ((opcode & OPCODE_MASK::TRANS_TYPE_4BIT) == OPCODE::MEM_CPY) {
        return OPCODE::MEM_CPY;
    }
    if ((opcode & OPCODE_MASK::TRANS_TYPE_5BIT) == OPCODE::SEND) {
        return OPCODE::SEND;
    }
    if ((opcode & OPCODE_MASK::TRANS_TYPE_5BIT) == OPCODE::RECV) {
        return OPCODE::RECV;
    }
    return OPCODE::_from_integral(opcode);
}

std::string InstV2::toString() const {
    auto op = getOpcodeEnum();
    std::stringstream ss;
    ss << op._to_string() << " ";
    switch (op) {
        case OPCODE::CIM_MVM: {
            ss << fmt::format("${} ${} ${}, flags:", rs, rt, re);
            TO_STRING_WRITE_FLAGS(GRP, GRP_I, SP_V, SP_B)
            break;
        }
        case OPCODE::CIM_CFG: {
            ss << fmt::format("${} ${}, flags:", rs, rt);
            TO_STRING_WRITE_FLAG(GRP_B)
            break;
        }
        case OPCODE::CIM_OUT: {
            ss << fmt::format("${} ${} to ${}, flags:", rs, rt, rd);
            TO_STRING_WRITE_FLAGS(OSUM, OSUM_MOV)
            break;
        }
        case OPCODE::VEC_OP: {
            ss << fmt::format("${} ", rs);
            int input_cnt = ((opcode >> 2) & 0x11) + 1;
            if (input_cnt >= 2) {
                ss << fmt::format("${} ", rt);
            }
            ss << fmt::format("to ${}, i_cnt: {}, len: ${}, func: {}", rd, input_cnt, re, funct);
            break;
        }
        case OPCODE::REDUCE: ss << fmt::format("${} to ${}, len: ${}, func: {}", rs, rd, rt, funct); break;
        case OPCODE::SC_RR: {
            ss.str(fmt::format("{} ${} ${} to ${}", SC_RR_FUNCT::_from_integral(funct)._to_string(), rs, rt, rd));
            break;
        }
        case OPCODE::SC_RI: {
            ss.str(fmt::format("{} ${} {} to ${}", SC_RI_FUNCT::_from_integral(funct)._to_string(), rs, imm, rd));
            break;
        }
        case OPCODE::SC_LD:
        case OPCODE::SC_LDG: ss << fmt::format("{}(${}) to ${}", imm, rs, rd); break;
        case OPCODE::SC_ST:
        case OPCODE::SC_STG: ss << fmt::format("${} to {}(${})", rt, imm, rs); break;
        case OPCODE::G_LI:
        case OPCODE::S_LI: ss << fmt::format("{} to ${}", imm, rd); break;
        case OPCODE::GS_MOV:
        case OPCODE::SG_MOV: ss << fmt::format("${} to ${}", rs, rd); break;
        case OPCODE::MEM_CPY: {
            std::bitset<2> mask_bin(opcode & 0b11);
            ss << fmt::format("${} to ${}, size: ${}, off: {}, mask: {}", rs, rd, rt, imm, mask_bin.to_string());
            break;
        }
        case OPCODE::SEND: ss << fmt::format("${} to [${}]${}, size: ${}, trans-id: ${}", rs, rt, rd, re, rf); break;
        case OPCODE::RECV: ss << fmt::format("[${}]${} to ${}, size: ${}, trans-id: ${}", rs, rt, rd, re, rf); break;
        case OPCODE::BEQ:
        case OPCODE::BNE:
        case OPCODE::BGT:
        case OPCODE::BLT: ss << fmt::format("${} ${}, off: {}", rs, rt, imm); break;
        case OPCODE::JMP: ss << fmt::format("{}", imm); break;
        case OPCODE::WAIT:
        case OPCODE::BARRIER: ss << fmt::format("${} ${}", rs, rt); break;
        default: break;
    }
    return ss.str();
}

std::string InstV2::toJsonString() const {
    std::stringstream ss;
    ss << "{";

    ss << fmt::format("\"{}\": {}", "opcode", opcode);
    INST_V2_TO_JSON_STR_WRITE_REGS(rs, rt, rd, re, rf);

    switch (getOpcodeEnum()) {
        case OPCODE::VEC_OP:
        case OPCODE::REDUCE:
        case OPCODE::SC_RR: ss << fmt::format(", \"{}\": {}", "funct", funct); break;
        case OPCODE::SC_RI: {
            ss << fmt::format(", \"{}\": {}", "funct", funct);
            ss << fmt::format(", \"{}\": {}", "imm", imm);
            break;
        }
        case OPCODE::SC_LD:
        case OPCODE::SC_LDG:
        case OPCODE::SC_ST:
        case OPCODE::SC_STG:
        case OPCODE::G_LI:
        case OPCODE::S_LI:
        case OPCODE::MEM_CPY:
        case OPCODE::BEQ:
        case OPCODE::BNE:
        case OPCODE::BGT:
        case OPCODE::BLT:
        case OPCODE::JMP: ss << fmt::format(", \"{}\": {}", "imm", imm); break;
        default: break;
    }

    INST_V2_TO_JSON_STR_WRITE_FLAGS(GRP, GRP_I, SP_V, SP_B, GRP_B, OSUM, OSUM_MOV)

    ss << fmt::format(R"(, "{}": "{}")", "asm", toString());
    ss << "}";
    return ss.str();
}

DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(InstV2, opcode, rs, rt, rd, re, rf, funct, imm, GRP, GRP_I, SP_V, SP_B,
                                            GRP_B, OSUM, OSUM_MOV, inst_group_tag)

void to_json(nlohmann::ordered_json& nlohmann_json_j, const InstV2& nlohmann_json_t) {
    nlohmann_json_j["asm"] = nlohmann_json_t.toString();

    std::bitset<6> binary(nlohmann_json_t.opcode);
    nlohmann_json_j["opcode_binary"] = fmt::format("0b{}", binary.to_string());

    TYPE_TO_JSON_FIELD_ASSIGN(opcode, rs, rt, rd, re, rf, funct, imm)
    TYPE_TO_JSON_WRITE_FLAGS(GRP, GRP_I, SP_V, SP_B, GRP_B, OSUM, OSUM_MOV)
}

}  // namespace cimsim
