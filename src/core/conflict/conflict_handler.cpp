//
// Created by wyk on 2024/8/10.
//

#include "conflict_handler.h"

#include "util/log.h"

namespace cimsim {

ConflictHandler::ConflictHandler(const sc_core::sc_event& decode_new_ins_trigger, ExecuteUnitType execute_unit_type)
    : sc_core::sc_module("StallHandler")
    , execute_unit_type_(execute_unit_type)
    , unit_resource_allocate_({.unit_type = execute_unit_type}) {
    SC_METHOD(processUnitResourceAllocate)
    sensitive << unit_ins_resource_allocate_;

    SC_METHOD(processUnitResourceRelease)
    sensitive << unit_ins_resource_release_;

    SC_METHOD(processUnitResourceConflict)
    sensitive << conflict_trigger_ << ready_ << decode_new_ins_trigger;
}

void ConflictHandler::processUnitResourceAllocate() {
    if (const auto& payload = unit_ins_resource_allocate_.read();
        payload.ins_id != -1 && unit_ins_resource_allocate_map_.count(payload.ins_id) == 0) {
        unit_ins_resource_allocate_map_.emplace(payload.ins_id, payload);
        unit_resource_allocate_ += payload;

        conflict_trigger_.notify(SC_ZERO_TIME);
    }
}

void ConflictHandler::processUnitResourceRelease() {
    int ins_id = unit_ins_resource_release_.read().ins_id;
    if (unit_ins_resource_allocate_map_.count(ins_id) != 0) {
        unit_ins_resource_allocate_map_.erase(ins_id);
        unit_resource_allocate_ = ResourceAllocatePayload{.unit_type = execute_unit_type_};
        for (const auto& [ins_pc, ins_data_conflict_payload] : unit_ins_resource_allocate_map_) {
            unit_resource_allocate_ += ins_data_conflict_payload;
        }

        conflict_trigger_.notify(SC_ZERO_TIME);
    }
}

void ConflictHandler::processUnitResourceConflict() {
    bool unit_conflict = (execute_unit_type_ == next_ins_resource_allocate_->unit_type && !ready_.read()) ||
                         unit_resource_allocate_.conflictWithIns(*next_ins_resource_allocate_);
    conflict_.write(unit_conflict);
}

}  // namespace cimsim
