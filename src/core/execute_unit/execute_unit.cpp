//
// Created by wyk on 2025/2/25.
//

#include "execute_unit.h"

#include "fmt/format.h"
#include "util/log.h"

namespace pimsim {

ExecuteUnit::ExecuteUnit(const char* name, const SimConfig& sim_config, Core* core, Clock* clk, ExecuteUnitType type)
    : BaseModule(name, sim_config, core, clk), type_(type), fsm_("FSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void ExecuteUnit::checkInst() {
    const auto& payload = ports_.id_ex_payload_port_.read();
    if (payload.payload != nullptr && payload.payload->ins.valid() && payload.payload->ins.unit_type == type_) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void ExecuteUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_id_port_.write(finish_ins_id_);
}

void ExecuteUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

DataConflictPayload ExecuteUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return {.ins_id = payload->ins.ins_id, .unit_type = payload->ins.unit_type};
}

void ExecuteUnit::readyForNextExecute() {
    ports_.busy_port_.write(false);
    fsm_.finish_exec_.notify(SC_ZERO_TIME);
}

void ExecuteUnit::triggerFinishInstruction(int ins_id) {
    finish_ins_ = true;
    finish_ins_id_ = ins_id;
    finish_ins_trigger_.notify(SC_ZERO_TIME);
}

void ExecuteUnit::triggerFinishRun() {
    finish_run_ = true;
    finish_run_trigger_.notify(SC_ZERO_TIME);
}

void ExecuteUnit::triggerFinishRun(double t) {
    finish_run_ = true;
    finish_run_trigger_.notify(t, SC_NS);
}

}  // namespace pimsim
