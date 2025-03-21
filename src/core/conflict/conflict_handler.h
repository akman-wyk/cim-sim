//
// Created by wyk on 2024/8/10.
//

#pragma once
#include <unordered_map>

#include "core/execute_unit/execute_unit.h"
#include "payload.h"
#include "systemc.h"

namespace cimsim {

class ConflictHandler : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(ConflictHandler);

    ConflictHandler(const sc_core::sc_module_name& name, const sc_core::sc_event& decode_new_ins_trigger,
                    ExecuteUnitType execute_unit_type);

    void bind(ExecuteUnitSignalPorts& signals, sc_core::sc_signal<bool>& conflict_signal,
              ResourceAllocatePayload* next_ins_resource_allocate_) {
        ready_.bind(signals.ready_);
        unit_ins_resource_allocate_.bind(signals.resource_allocate_);
        unit_ins_resource_release_.bind(signals.resource_release_);
        conflict_.bind(conflict_signal);

        this->next_ins_resource_allocate_ = next_ins_resource_allocate_;
    }

private:
    void processUnitResourceAllocate();
    void processUnitResourceRelease();
    void processUnitResourceConflict();

public:
    sc_core::sc_in<bool> ready_{"ready"};
    sc_core::sc_in<ResourceAllocatePayload> unit_ins_resource_allocate_{"unit_ins_resource_allocate"};
    sc_core::sc_in<ResourceReleasePayload> unit_ins_resource_release_{"unit_ins_resource_release"};

    sc_core::sc_out<bool> conflict_{"conflict"};

private:
    const ExecuteUnitType execute_unit_type_;
    ResourceAllocatePayload* next_ins_resource_allocate_{nullptr};

    std::unordered_map<int, ResourceAllocatePayload> unit_ins_resource_allocate_map_{};
    ResourceAllocatePayload unit_resource_allocate_;
    sc_core::sc_event conflict_trigger_;
};

}  // namespace cimsim
