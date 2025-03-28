//
// Created by wyk on 2024/8/10.
//

#include "conflict_handler.h"

#include "util/log.h"

namespace cimsim {

ConflictHandler::ConflictHandler(const sc_module_name& name, const sc_event& decode_new_ins_trigger,
                                 ExecuteUnitType execute_unit_type)
    : sc_module(name), execute_unit_type_(execute_unit_type) {
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
        if (auto data_path_id = payload.data_path_payload.getUniqueId();
            data_path_resource_allocate_map_.count(data_path_id) == 0) {
            data_path_resource_allocate_map_.emplace(data_path_id, payload);
        } else {
            data_path_resource_allocate_map_[data_path_id] += payload;
        }

        conflict_trigger_.notify(SC_ZERO_TIME);
    }
}

void ConflictHandler::processUnitResourceRelease() {
    int erase_id_cnt = 0;
    for (int ins_id : unit_ins_resource_release_.read().ins_id_list_) {
        if (auto node = unit_ins_resource_allocate_map_.extract(ins_id); !node.empty()) {
            auto data_path_id = node.mapped().data_path_payload.getUniqueId();
            data_path_resource_allocate_map_[data_path_id] = ResourceAllocatePayload{.unit_type = execute_unit_type_};
            erase_id_cnt++;
        }
    }

    if (erase_id_cnt > 0) {
        for (const auto& [ins_pc, ins_data_conflict_payload] : unit_ins_resource_allocate_map_) {
            auto data_path_id = ins_data_conflict_payload.data_path_payload.getUniqueId();
            data_path_resource_allocate_map_[data_path_id] += ins_data_conflict_payload;
        }
        conflict_trigger_.notify(SC_ZERO_TIME);
    }
}

void ConflictHandler::processUnitResourceConflict() {
    bool unit_conflict =
        (execute_unit_type_ == next_ins_resource_allocate_->unit_type && !ready_.read()) ||
        std::any_of(data_path_resource_allocate_map_.begin(), data_path_resource_allocate_map_.end(),
                    [&](const auto& p) { return p.second.conflictWithIns(*next_ins_resource_allocate_); });
    conflict_.write(unit_conflict);
}

}  // namespace cimsim
