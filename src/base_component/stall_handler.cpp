//
// Created by wyk on 2024/8/10.
//

#include "stall_handler.h"

#include "fmt/format.h"
#include "util/log.h"

namespace pimsim {

StallHandler::StallHandler(sc_core::sc_event& decode_new_ins_trigger, ExecuteUnitType execute_unit_type)
    : sc_core::sc_module("StallHandler"), execute_unit_type_(execute_unit_type) {
    SC_METHOD(processAddUnitDataConflict)
    sensitive << data_conflict_;

    SC_METHOD(processDeleteUnitDataConflict)
    sensitive << finish_ins_ << finish_ins_id_;

    SC_METHOD(processUnitDataConflict)
    sensitive << trigger_ << busy_ << decode_new_ins_trigger;
}

void StallHandler::processAddUnitDataConflict() {
    const auto& unit_conflict_payload = data_conflict_.read();
    if (unit_conflict_payload.ins_id != -1) {
        auto found = ins_data_conflict_info_map_.find(unit_conflict_payload.ins_id);
        if (found == ins_data_conflict_info_map_.end()) {
            ins_data_conflict_info_map_.emplace(unit_conflict_payload.ins_id, unit_conflict_payload);
            trigger_.notify(SC_ZERO_TIME);
        }
    }
}

void StallHandler::processDeleteUnitDataConflict() {
    bool finish = finish_ins_.read();
    int finish_pc = finish_ins_id_.read();

    if (finish) {
        auto found = ins_data_conflict_info_map_.find(finish_pc);
        if (found != ins_data_conflict_info_map_.end()) {
            ins_data_conflict_info_map_.erase(finish_pc);
            trigger_.notify(SC_ZERO_TIME);
        }
    }
}

void StallHandler::processUnitDataConflict() {
    unit_data_conflict_info_ = DataConflictPayload{};
    for (const auto& [ins_pc, ins_data_conflict_payload] : ins_data_conflict_info_map_) {
        unit_data_conflict_info_ += ins_data_conflict_payload;
    }

    bool busy = busy_.read();
    bool unit_conflict = DataConflictPayload::checkDataConflict(*cur_ins_conflict_info_, unit_data_conflict_info_) ||
                         (cur_ins_conflict_info_->unit_type == unit_data_conflict_info_.unit_type && busy);
    conflict_.write(unit_conflict);
}

}  // namespace pimsim
