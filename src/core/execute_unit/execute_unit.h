//
// Created by wyk on 2025/2/25.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "core/payload/payload.h"
#include "systemc.h"

namespace pimsim {

struct ExecuteUnitSignalPorts {
    sc_core::sc_signal<ExecuteUnitPayload> id_ex_payload_;
    sc_core::sc_signal<bool> id_ex_enable_;
    sc_core::sc_signal<bool> busy_;
    sc_core::sc_signal<DataConflictPayload> data_conflict_;

    sc_core::sc_signal<bool> finish_ins_;
    sc_core::sc_signal<int> finish_ins_id_;

    sc_core::sc_signal<bool> finish_run_;
};

struct ExecuteUnitRequestIOPorts {
    sc_core::sc_out<ExecuteUnitPayload> id_ex_payload_port_;
    sc_core::sc_out<bool> id_ex_enable_port_;
    sc_core::sc_in<bool> busy_port_;
    sc_core::sc_in<DataConflictPayload> data_conflict_port_;

    sc_core::sc_in<bool> finish_ins_port_;
    sc_core::sc_in<int> finish_ins_id_port_;

    sc_core::sc_in<bool> finish_run_port_;
};

struct ExecuteUnitResponseIOPorts {
    sc_core::sc_in<ExecuteUnitPayload> id_ex_payload_port_;
    sc_core::sc_in<bool> id_ex_enable_port_;
    sc_core::sc_out<bool> busy_port_;
    sc_core::sc_out<DataConflictPayload> data_conflict_port_;

    sc_core::sc_out<bool> finish_ins_port_;
    sc_core::sc_out<int> finish_ins_id_port_;

    sc_core::sc_out<bool> finish_run_port_;

    void bind(ExecuteUnitSignalPorts& signals) {
        id_ex_payload_port_.bind(signals.id_ex_payload_);
        id_ex_enable_port_.bind(signals.id_ex_enable_);
        busy_port_.bind(signals.busy_);
        data_conflict_port_.bind(signals.data_conflict_);
        finish_ins_port_.bind(signals.finish_ins_);
        finish_ins_id_port_.bind(signals.finish_ins_id_);
        finish_run_port_.bind(signals.finish_run_);
    }
};

class ExecuteUnit : public BaseModule {
public:
    SC_HAS_PROCESS(ExecuteUnit);

    ExecuteUnit(const char* name, const SimConfig& sim_config, Core* core, Clock* clk, ExecuteUnitType type);

    void checkInst();
    void finishInstruction();
    void finishRun();

    virtual DataConflictPayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload);

protected:
    template <class InsPayload>
    std::shared_ptr<InsPayload> waitForExecuteAndGetPayload() {
        wait(fsm_.start_exec_);
        ports_.busy_port_.write(true);

        return std::dynamic_pointer_cast<InsPayload>(fsm_out_.read().payload);
    }

    void readyForNextExecute();

    void triggerFinishInstruction(int ins_id);
    void triggerFinishRun();
    void triggerFinishRun(double t);

public:
    ExecuteUnitResponseIOPorts ports_;

protected:
    const ExecuteUnitType type_;

    FSM<ExecuteUnitPayload> fsm_;
    sc_core::sc_signal<ExecuteUnitPayload> fsm_out_;
    sc_core::sc_signal<FSMPayload<ExecuteUnitPayload>> fsm_in_;

    sc_core::sc_event finish_ins_trigger_;
    int finish_ins_id_{-1};
    bool finish_ins_{false};

    sc_core::sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace pimsim
