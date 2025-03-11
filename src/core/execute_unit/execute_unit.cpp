//
// Created by wyk on 2025/2/25.
//

#include "execute_unit.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

ExecuteUnit::ExecuteUnit(const char* name, const SimConfig& sim_config, Core* core, Clock* clk, ExecuteUnitType type)
    : BaseModule(name, sim_config, core, clk), type_(type), fsm_("FSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_METHOD(processReleaseResource)
    sensitive << release_resource_trigger_;

    SC_METHOD(processFinishRun)
    sensitive << finish_run_trigger_;

    SC_METHOD(processIdFinish)
    sensitive << ports_.id_finish_port_;
}

void ExecuteUnit::processIdFinish() {
    if (ports_.id_finish_port_.read()) {
        finish_decode_ = true;
        if (running_ins_cnt_ == 0) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }
    }
}

void ExecuteUnit::checkInst() {
    const auto& payload = ports_.id_ex_payload_port_.read();
    if (payload.payload != nullptr && payload.payload->ins.valid() && payload.payload->ins.unit_type == type_) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void ExecuteUnit::processReleaseResource() {
    ports_.resource_release_.write(ResourceReleasePayload{.ins_id = release_resource_ins_id_});
}

void ExecuteUnit::processFinishRun() {
    ports_.unit_finish_port_.write(finish_run_);
}

ResourceAllocatePayload ExecuteUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return {.ins_id = payload->ins.ins_id, .unit_type = payload->ins.unit_type};
}

void ExecuteUnit::readyForNextExecute() {
    ports_.ready_port_.write(true);
    fsm_.finish_exec_.notify(SC_ZERO_TIME);
}

void ExecuteUnit::releaseResource(int ins_id) {
    release_resource_ins_id_ = ins_id;
    release_resource_trigger_.notify(SC_ZERO_TIME);
}

void ExecuteUnit::finishInstruction(double t) {
    wait(t, SC_NS);
    running_ins_cnt_--;
    if (running_ins_cnt_ == 0 && finish_decode_) {
        finish_run_ = true;
        finish_run_trigger_.notify(SC_ZERO_TIME);
    }
}

}  // namespace cimsim
