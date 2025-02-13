//
// Created by wyk on 2024/8/8.
//

#pragma once
#include <iostream>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/stall_handler.h"
#include "core/payload/execute_unit_payload.h"
#include "core/pim_unit/cim_unit.h"
#include "core/pim_unit/pim_compute_unit.h"
#include "core/pim_unit/pim_load_unit.h"
#include "core/pim_unit/pim_output_unit.h"
#include "core/pim_unit/pim_set_unit.h"
#include "core/pim_unit/pim_transfer_unit.h"
#include "core/reg_unit/reg_unit.h"
#include "core/scalar_unit/scalar_unit.h"
#include "core/simd_unit/simd_unit.h"
#include "core/transfer_unit/transfer_unit.h"
#include "isa/instruction.h"
#include "local_memory_unit/local_memory_unit.h"
#include "payload/payload.h"
#include "util/ins_stat.h"

namespace pimsim {

class Core : public BaseModule {
public:
    SC_HAS_PROCESS(Core);

    Core(int core_id, const char* name, const Config& config, Clock* clk, std::vector<Instruction> ins_list,
         std::function<void()> finish_run_call, bool check = false, std::ostream& reg_stat_os = std::cout);
    void bindNetwork(Network* network);

    EnergyReporter getEnergyReporter() override;

    bool checkRegValues(const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                        const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values);

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    [[nodiscard]] int getCoreId() const;

private:
    [[noreturn]] void issue();
    void processStall();
    void processIdExEnable();
    void processFinishRun();

    int decodeAndGetPCIncrement();
    void decodeScalarIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodeSIMDIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodeTransferIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodePimComputeIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodePimOutputIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodePimSetIns(const Instruction& ins, const InstructionPayload& ins_payload);
    void decodePimTransferIns(const Instruction& ins, const InstructionPayload& ins_payload);
    int decodeControlInsAndGetPCIncrement(const Instruction& ins, const InstructionPayload& ins_payload);

private:
    const int core_id_;
    const CoreConfig& core_config_;
    const AddressSpaceConfig& global_memory_addressing_;

    InsStat ins_stat_{};
    bool check{false};
    std::ostream& reg_stat_os_;

    // instruction
    std::vector<Instruction> ins_list_;
    int ins_index_{0};
    int ins_id_{0};
    DataConflictPayload cur_ins_conflict_info_;
    sc_core::sc_event decode_new_ins_trigger_;

    // payloads to execute units
    ScalarInsPayload scalar_payload_;
    SIMDInsPayload simd_payload_;
    TransferInsPayload transfer_payload_;

    CimUnit cim_unit_;
    PimComputeInsPayload pim_compute_payload_;
    PimLoadInsPayload pim_load_payload_;
    PimOutputInsPayload pim_output_payload_;
    PimSetInsPayload pim_set_payload_;
    PimTransferInsPayload pim_transfer_payload_;

    // modules
    // execute units
    ScalarUnit scalar_unit_;
    SIMDUnit simd_unit_;
    TransferUnit transfer_unit_;

    PimComputeUnit pim_compute_unit_;
    PimLoadUnit pim_load_unit_;
    PimOutputUnit pim_output_unit_;
    PimSetUnit pim_set_unit_;
    PimTransferUnit pim_transfer_unit_;
    // other modules
    LocalMemoryUnit local_memory_unit_;
    RegUnit reg_unit_;
    Switch core_switch_;

    // signals
    ExecuteUnitSignalPorts<ScalarInsPayload> scalar_signals_;
    ExecuteUnitSignalPorts<SIMDInsPayload> simd_signals_;
    ExecuteUnitSignalPorts<TransferInsPayload> transfer_signals_;
    ExecuteUnitSignalPorts<PimComputeInsPayload> pim_compute_signals_;
    ExecuteUnitSignalPorts<PimLoadInsPayload> pim_load_signals_;
    ExecuteUnitSignalPorts<PimOutputInsPayload> pim_output_signals_;
    ExecuteUnitSignalPorts<PimSetInsPayload> pim_set_signals_;
    ExecuteUnitSignalPorts<PimTransferInsPayload> pim_transfer_signals_;

    sc_core::sc_signal<RegUnitReadRequest> read_req_signal_;
    sc_core::sc_signal<RegUnitReadResponse> read_rsp_signal_;
    sc_core::sc_signal<RegUnitWriteRequest> write_req_signal_;

    // stall
    StallHandler scalar_stall_handler_, simd_stall_handler_, transfer_stall_handler_, pim_compute_stall_handler_,
        pim_load_stall_handler_, pim_output_stall_handler_, pim_set_stall_handler_, pim_transfer_stall_handler_;
    sc_core::sc_signal<bool> scalar_conflict_, simd_conflict_, transfer_conflict_, pim_compute_conflict_,
        pim_load_conflict_, pim_output_conflict_, pim_set_conflict_, pim_transfer_conflict_;
    sc_core::sc_signal<bool> id_stall_;

    // finish run
    std::function<void()> finish_run_call_;
};

}  // namespace pimsim
