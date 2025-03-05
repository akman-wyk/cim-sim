//
// Created by wyk on 2024/8/8.
//

#pragma once
#include <iostream>
#include <vector>

#include "base_component/base_module.h"
#include "base_component/stall_handler.h"
#include "core/pim_unit/cim_unit.h"
#include "core/pim_unit/pim_compute_unit.h"
#include "core/pim_unit/pim_control_unit.h"
#include "core/reg_unit/reg_unit.h"
#include "core/scalar_unit/scalar_unit.h"
#include "core/simd_unit/simd_unit.h"
#include "core/transfer_unit/transfer_unit.h"
#include "decoder/decoder.h"
#include "execute_unit/execute_unit.h"
#include "local_memory_unit/local_memory_unit.h"
#include "payload/payload.h"

namespace pimsim {

using DecoderImpl = DecoderV2;
using Instruction = DecoderImpl::Instruction;

struct ExecuteUnitRegistration {
    ExecuteUnitRegistration(ExecuteUnitType type, ExecuteUnit* execute_unit, sc_event& decode_new_ins_trigger);

    ExecuteUnitType type;
    ExecuteUnit* execute_unit;
    StallHandler stall_handler;
    ExecuteUnitSignalPorts signals;
    sc_signal<bool> conflict_signal;
};

class Core : public BaseModule {
public:
    SC_HAS_PROCESS(Core);

    Core(int core_id, const char* name, const Config& config, Clock* clk, std::vector<Instruction> ins_list,
         std::function<void()> finish_run_call);
    void bindNetwork(Network* network);

    EnergyReporter getEnergyReporter() override;

    bool checkRegValues(const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                        const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values);

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    [[nodiscard]] int getCoreId() const;

private:
    [[noreturn]] void processDecode();
    [[noreturn]] void processUpdatePC();
    void processIssue();

    void processStall();
    void processIdExEnable();
    void processFinishRun();

    void registerExecuteUnit(ExecuteUnitType type, ExecuteUnit* execute_unit);

    void bindModules();
    void setThreadAndMethod();

private:
    const int core_id_;
    const CoreConfig& core_config_;

    // instruction
    std::vector<Instruction> ins_list_;
    int ins_index_{0};

    // modules
    CimUnit cim_unit_;
    LocalMemoryUnit local_memory_unit_;
    RegUnit reg_unit_;
    Switch core_switch_;
    DecoderImpl decoder_;

    // execute units
    ScalarUnit scalar_unit_;
    SIMDUnit simd_unit_;
    TransferUnit transfer_unit_;
    PimComputeUnit pim_compute_unit_;
    PimControlUnit pim_control_unit_;

    // execute unit manage
    sc_process_handle processStall_handle_;
    sc_process_handle processFinishRun_handle_;
    std::vector<std::shared_ptr<ExecuteUnitRegistration>> execute_unit_list_;

    // decode, issue and stall
    std::shared_ptr<ExecuteInsPayload> cur_ins_payload_;
    int pc_increment_{0};
    DataConflictPayload cur_ins_conflict_info_;
    sc_core::sc_event decode_new_ins_trigger_;
    sc_core::sc_signal<bool> id_finish_;
    sc_core::sc_signal<bool> id_stall_;

    // finish run
    std::function<void()> finish_run_call_;
};

}  // namespace pimsim
