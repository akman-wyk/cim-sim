//
// Created by wyk on 2024/8/8.
//

#pragma once
#include <iostream>
#include <vector>

#include "base_component/base_module.h"
#include "conflict/conflict_handler.h"
#include "core/cim_unit/cim_unit.h"
#include "core/reg_unit/reg_unit.h"
#include "decoder/decoder.h"
#include "execute_unit/cim_compute_unit.h"
#include "execute_unit/cim_control_unit.h"
#include "execute_unit/execute_unit.h"
#include "execute_unit/reduce_unit.h"
#include "execute_unit/scalar_unit.h"
#include "execute_unit/simd_unit.h"
#include "execute_unit/transfer_unit.h"
#include "memory/memory_unit.h"
#include "network/switch.h"
#include "payload.h"

namespace cimsim {

using DecoderImpl = DecoderV2;
using Instruction = DecoderImpl::Instruction;

class Core : public BaseModule {
public:
    SC_HAS_PROCESS(Core);

    Core(const sc_module_name& name, const CoreConfig& config, const BaseInfo& base_info, Clock* clk, int global_id,
         std::vector<Instruction> ins_list, std::function<void()> finish_run_call);
    void bindNetwork(Network* network);

    EnergyReporter getEnergyReporter() const;

    bool checkRegValues(const std::array<int, GENERAL_REG_NUM>& general_reg_expected_values,
                        const std::array<int, SPECIAL_REG_NUM>& special_reg_expected_values);

    bool checkInsStat(const std::string& expected_ins_stat_file) const;

    [[nodiscard]] int getCoreId() const;

private:
    struct ExecuteUnitInfo {
        ExecuteUnitInfo(ExecuteUnitType type, ExecuteUnit* execute_unit, const sc_event& decode_new_ins_trigger);

        ExecuteUnitType type;
        ExecuteUnit* execute_unit;
        ConflictHandler stall_handler;
        ExecuteUnitSignalPorts signals;
        sc_signal<bool> conflict_signal;
    };

private:
    [[noreturn]] void processDecode();
    [[noreturn]] void processUpdatePC();
    void processIssue();

    void processStall();
    void processIdExEnable();
    void processFinishRun();

    void bindExecuteUnit(ExecuteUnitType type, ExecuteUnit* execute_unit);

    void bindModules();
    void setThreadAndMethod();

private:
    const CoreConfig& core_config_;

    // instruction
    std::vector<Instruction> ins_list_;
    int ins_index_{0};

    // modules
    CimUnit cim_unit_;
    MemoryUnit local_memory_unit_;
    RegUnit reg_unit_;
    Switch core_switch_;
    DecoderImpl decoder_;

    // execute units
    ScalarUnit scalar_unit_;
    SIMDUnit simd_unit_;
    ReduceUnit reduce_unit_;
    TransferUnit transfer_unit_;
    CimComputeUnit cim_compute_unit_;
    CimControlUnit cim_control_unit_;

    // execute unit manage
    sc_process_handle processStall_handle_;
    sc_process_handle processFinishRun_handle_;
    std::vector<std::shared_ptr<ExecuteUnitInfo>> execute_unit_list_;

    // decode, issue and stall
    std::shared_ptr<ExecuteInsPayload> cur_ins_payload_;
    int pc_increment_{0};
    ResourceAllocatePayload cur_ins_conflict_info_;
    sc_event decode_new_ins_trigger_;
    sc_signal<bool> id_finish_{"id_finish"};
    sc_signal<bool> id_stall_{"id_stall"};

    // finish run
    std::function<void()> finish_run_call_;
};

}  // namespace cimsim
