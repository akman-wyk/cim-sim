//
// Created by wyk on 2025/3/5.
//

#include <iostream>

#include "base/test_macro.h"
#include "base/test_payload.h"
#include "fmt/format.h"
#include "isa/inst_v1.h"
#include "isa/inst_v2.h"
#include "isa/isa.h"
#include "util/util.h"

namespace cimsim {

InstV2 convertCimInstV1toV2(const InstV1& inst_v1) {
    if (inst_v1.type == CIMInstType::compute) {
        return InstV2{.opcode = OPCODE::CIM_MVM,
                      .rs = inst_v1.rs1,
                      .rt = inst_v1.rs2,
                      .re = inst_v1.rs3,
                      .GRP = (inst_v1.group != 0),
                      .SP_V = (inst_v1.value_sparse != 0),
                      .SP_B = inst_v1.bit_sparse != 0};
    }
    if (inst_v1.type == CIMInstType::set) {
        return InstV2{
            .opcode = OPCODE::CIM_CFG, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .GRP_B = (inst_v1.group_broadcast != 0)};
    }
    return InstV2{.opcode = OPCODE::CIM_OUT,
                  .rs = inst_v1.rs1,
                  .rt = inst_v1.rs2,
                  .rd = inst_v1.rd,
                  .OSUM = (inst_v1.outsum != 0),
                  .OSUM_MOV = (inst_v1.outsum_move != 0)};
}

InstV2 convertSIMDInstV1toV2(const InstV1& inst_v1) {
    return InstV2{.opcode = (OPCODE::VEC_OP | (inst_v1.input_num << 2)),
                  .rs = inst_v1.rs1,
                  .rt = inst_v1.rs2,
                  .rd = inst_v1.rd,
                  .re = inst_v1.rs3,
                  .funct = inst_v1.opcode};
}

InstV2 convertScalarInstV1toV2(const InstV1& inst_v1) {
    if (inst_v1.type == ScalarInstType::RR) {
        return InstV2{
            .opcode = OPCODE::SC_RR, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .rd = inst_v1.rd, .funct = inst_v1.opcode};
    }
    if (inst_v1.type == ScalarInstType::RI) {
        return InstV2{
            .opcode = OPCODE::SC_RI, .rs = inst_v1.rs1, .rd = inst_v1.rd, .funct = inst_v1.opcode, .imm = inst_v1.imm};
    }
    if (inst_v1.type == ScalarInstType::SL) {
        switch (inst_v1.opcode) {
            case ScalarSLInstOpcode::load_local:
                return InstV2{.opcode = OPCODE::SC_LD, .rs = inst_v1.rs1, .rd = inst_v1.rs2, .imm = inst_v1.offset};
            case ScalarSLInstOpcode::load_global:
                return InstV2{.opcode = OPCODE::SC_LDG, .rs = inst_v1.rs1, .rd = inst_v1.rs2, .imm = inst_v1.offset};
            case ScalarSLInstOpcode::store_local:
                return InstV2{.opcode = OPCODE::SC_ST, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
            case ScalarSLInstOpcode::store_global:
                return InstV2{.opcode = OPCODE::SC_STG, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
            default: break;
        }
    }
    if (inst_v1.type == ScalarInstType::Assign) {
        switch (inst_v1.opcode) {
            case ScalarAssignInstOpcode::li_general:
                return InstV2{.opcode = OPCODE::G_LI, .rd = inst_v1.rd, .imm = inst_v1.imm};
            case ScalarAssignInstOpcode::li_special:
                return InstV2{.opcode = OPCODE::S_LI, .rd = inst_v1.rd, .imm = inst_v1.imm};
            case ScalarAssignInstOpcode::assign_general_to_special:
                return InstV2{.opcode = OPCODE::GS_MOV, .rs = inst_v1.rs1, .rd = inst_v1.rs2};
            case ScalarAssignInstOpcode::assign_special_to_general:
                return InstV2{.opcode = OPCODE::SG_MOV, .rs = inst_v1.rs2, .rd = inst_v1.rs1};
            default: break;
        }
    }
    return InstV2{};
}

InstV2 convertTransferInstV1toV2(const InstV1& inst_v1) {
    if (inst_v1.type == TransferInstType::trans) {
        return InstV2{.opcode = (OPCODE::MEM_CPY | (inst_v1.offset_mask & 0b11)),
                      .rs = inst_v1.rs1,
                      .rt = inst_v1.rs2,
                      .rd = inst_v1.rd,
                      .imm = inst_v1.offset};
    }
    if (inst_v1.type == TransferInstType::send) {
        return InstV2{.opcode = OPCODE::SEND,
                      .rs = inst_v1.rs1,
                      .rt = inst_v1.rd1,
                      .rd = inst_v1.rd2,
                      .re = inst_v1.reg_len,
                      .rf = inst_v1.reg_id};
    }
    return InstV2{.opcode = OPCODE::RECV,
                  .rs = inst_v1.rs1,
                  .rt = inst_v1.rs2,
                  .rd = inst_v1.rd,
                  .re = inst_v1.reg_len,
                  .rf = inst_v1.reg_id};
}

InstV2 convertControlInstV1toV2(const InstV1& inst_v1) {
    switch (inst_v1.type) {
        case ControlInstType::jmp: return InstV2{.opcode = OPCODE::JMP, .imm = inst_v1.offset};
        case ControlInstType::beq:
            return InstV2{.opcode = OPCODE::BEQ, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
        case ControlInstType::bne:
            return InstV2{.opcode = OPCODE::BNE, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
        case ControlInstType::bgt:
            return InstV2{.opcode = OPCODE::BGT, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
        case ControlInstType::blt:
            return InstV2{.opcode = OPCODE::BLT, .rs = inst_v1.rs1, .rt = inst_v1.rs2, .imm = inst_v1.offset};
        default: return InstV2{};
    }
}

InstV2 convertInstV1toV2(const InstV1& inst_v1) {
    if (inst_v1.class_code == InstClass::cim) {
        return convertCimInstV1toV2(inst_v1);
    }
    if (inst_v1.class_code == InstClass::simd) {
        return convertSIMDInstV1toV2(inst_v1);
    }
    if (inst_v1.class_code == InstClass::scalar) {
        return convertScalarInstV1toV2(inst_v1);
    }
    if (inst_v1.class_code == InstClass::transfer) {
        return convertTransferInstV1toV2(inst_v1);
    }
    return convertControlInstV1toV2(inst_v1);
}

template <class Inst>
struct CoreTestData {
    std::string comments;
    std::vector<Inst> code{};
    TestExpectedInfo expected;
    CoreTestRegisterInfo reg_info;
};

template <class Inst>
struct ChipTestData {
    std::string comments;
    std::vector<std::vector<Inst>> code;
    TestExpectedInfo expected;
};

using CoreTestDataV1 = CoreTestData<InstV1>;
using ChipTestDataV1 = ChipTestData<InstV1>;

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreTestDataV1, comments, code, expected, reg_info)
DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ChipTestDataV1, code, expected);

void convertCoreCodeV1toV2(const char* v1_file, const char* v2_file) {
    auto core_code_v1 = readTypeFromJsonFile<CoreTestDataV1>(v1_file);

    std::ofstream ofs(v2_file);
    ofs << "{\n";
    ofs << fmt::format("  \"comments\": \"{}\",\n", core_code_v1.comments);

    ofs << "  \"code\": [\n";
    for (int i = 0; i < core_code_v1.code.size(); i++) {
        auto inst_v2 = convertInstV1toV2(core_code_v1.code[i]);
        ofs << "    " << inst_v2.toJsonString() << (i == core_code_v1.code.size() - 1 ? "\n" : ",\n");
    }
    ofs << "  ],\n";

    ofs << "  \"expected\": {\n";
    ofs << fmt::format("    \"time_ns\": {},\n    \"energy_pj\": {}\n", core_code_v1.expected.time_ns,
                       core_code_v1.expected.energy_pj);
    if (core_code_v1.reg_info.check) {
        ofs << "  },\n";
        nlohmann::ordered_json reg_info_j = core_code_v1.reg_info;
        ofs << "  \"reg_info\": " << reg_info_j << "\n}";
    } else {
        ofs << "  }\n}";
    }

    ofs.close();
}

void convertChipCodeV1toV2(const char* v1_file, const char* v2_file) {
    auto chip_code_v1 = readTypeFromJsonFile<ChipTestDataV1>(v1_file);

    std::ofstream ofs(v2_file);
    ofs << "{\n";
    ofs << fmt::format("  \"comments\": \"{}\",\n", chip_code_v1.comments);

    ofs << "  \"code\": [\n";
    for (int i = 0; i < chip_code_v1.code.size(); i++) {
        auto& core_code_v1 = chip_code_v1.code[i];
        ofs << "    [\n";
        for (int j = 0; j < core_code_v1.size(); j++) {
            auto inst_v2 = convertInstV1toV2(core_code_v1[j]);
            ofs << "      " << inst_v2.toJsonString() << (j == core_code_v1.size() - 1 ? "\n" : ",\n");
        }
        ofs << (i == chip_code_v1.code.size() - 1 ? "    ]\n" : "    ],\n");
    }
    ofs << "  ],\n";

    ofs << "  \"expected\": {\n";
    ofs << fmt::format("    \"time_ns\": {},\n    \"energy_pj\": {}\n", chip_code_v1.expected.time_ns,
                       chip_code_v1.expected.energy_pj);
    ofs << "  }\n}";

    ofs.close();
}

}  // namespace cimsim

int main(int argc, char* argv[]) {
    std::string exec_file_name{argv[0]};
    if (argc != 4) {
        std::cout << fmt::format("Usage: {} [chip/core] [v1_file] [v2_file]", exec_file_name) << std::endl;
        return INVALID_USAGE;
    }

    auto mode = std::string{argv[1]};
    auto* v1_file = argv[2];
    auto* v2_file = argv[3];

    if (mode == "chip") {
        cimsim::convertChipCodeV1toV2(v1_file, v2_file);
    } else {
        cimsim::convertCoreCodeV1toV2(v1_file, v2_file);
    }
    return 0;
}
