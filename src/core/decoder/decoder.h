//
// Created by wyk on 2025/2/15.
//

#pragma once
#include <memory>

#include "base_component/base_module.h"
#include "core/execute_unit/execute_unit.h"
#include "core/payload/payload.h"
#include "core/reg_unit/reg_unit.h"
#include "isa/instruction.h"
#include "util/ins_stat.h"

namespace pimsim {

struct DecodeResult {
    int pc_increment{};
    std::shared_ptr<ExecuteInsPayload> payload{};
    DataConflictPayload conflict_info{};
};

class Decoder : BaseModule {
public:
    Decoder(const char* name, const ChipConfig& chip_config, const SimConfig& sim_config, Core* core, Clock* clk);

    void bindRegUnit(RegUnit* reg_unit);
    void bindExecuteUnit(ExecuteUnitType type, ExecuteUnit* execute);

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    std::shared_ptr<ExecuteInsPayload> decode(const Instruction& ins, int pc, int& pc_increment,
                                              DataConflictPayload& conflict_info);

private:
    std::shared_ptr<ExecuteInsPayload> decodeScalarIns(const Instruction& ins, int pc) const;
    std::shared_ptr<ExecuteInsPayload> decodeSIMDIns(const Instruction& ins, int pc) const;
    std::shared_ptr<ExecuteInsPayload> decodeTransferIns(const Instruction& ins, int pc) const;
    std::shared_ptr<ExecuteInsPayload> decodePimComputeIns(const Instruction& ins, int pc) const;
    std::shared_ptr<ExecuteInsPayload> decodePimControlIns(const Instruction& ins, int pc) const;
    int decodeControlInsAndGetPCIncrement(const Instruction& ins, int pc) const;

    static ScalarOperator decodeScalarRROpcode(int opcode);
    static ScalarOperator decodeScalarRIOpcode(int opcode);

private:
    const AddressSpaceConfig& global_as_;
    const SIMDUnitConfig& simd_unit_config_;

    int ins_id_{0};
    InsStat ins_stat_{};

    RegUnit* reg_unit_{};
    std::unordered_map<int, ExecuteUnit*> execute_unit_map_;
};

}  // namespace pimsim
