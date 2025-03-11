//
// Created by wyk on 2024/8/10.
//

#include "conflict_handler.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

ConflictHandler::ConflictHandler(sc_core::sc_event& decode_new_ins_trigger, ExecuteUnitType execute_unit_type)
    : sc_core::sc_module("StallHandler"), execute_unit_type_(execute_unit_type) {
    SC_METHOD(processUnitResourceAllocate)
    sensitive << unit_ins_resource_allocate_;

    SC_METHOD(processUnitResourceRelease)
    sensitive << unit_ins_resource_release_;

    SC_METHOD(processUnitResourceConflict)
    sensitive << trigger_ << ready_ << decode_new_ins_trigger;
}

void ConflictHandler::processUnitResourceAllocate() {
    const auto& unit_conflict_payload = unit_ins_resource_allocate_.read();
    if (unit_conflict_payload.ins_id != -1) {
        auto found = unit_ins_resource_allocate_map_.find(unit_conflict_payload.ins_id);
        if (found == unit_ins_resource_allocate_map_.end()) {
            unit_ins_resource_allocate_map_.emplace(unit_conflict_payload.ins_id, unit_conflict_payload);
            trigger_.notify(SC_ZERO_TIME);
        }
    }
}

void ConflictHandler::processUnitResourceRelease() {
    int ins_id = unit_ins_resource_release_.read().ins_id;
    auto found = unit_ins_resource_allocate_map_.find(ins_id);
    if (found != unit_ins_resource_allocate_map_.end()) {
        unit_ins_resource_allocate_map_.erase(ins_id);
        trigger_.notify(SC_ZERO_TIME);
    }
}

void ConflictHandler::processUnitResourceConflict() {
    unit_resource_allocate_ = ResourceAllocatePayload{};
    for (const auto& [ins_pc, ins_data_conflict_payload] : unit_ins_resource_allocate_map_) {
        unit_resource_allocate_ += ins_data_conflict_payload;
    }

    bool unit_conflict =
        ResourceAllocatePayload::checkDataConflict(*next_ins_resource_allocate_, unit_resource_allocate_) ||
        (next_ins_resource_allocate_->unit_type == unit_resource_allocate_.unit_type && !ready_.read());
    conflict_.write(unit_conflict);
}

}  // namespace cimsim
