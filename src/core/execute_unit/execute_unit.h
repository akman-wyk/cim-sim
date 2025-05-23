
//
// Created by wyk on 2025/2/25.
//

#pragma once
#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "base_component/fsm.h"
#include "core/conflict/payload.h"
#include "core/socket/memory_socket.h"
#include "payload.h"

namespace cimsim {

struct ExecuteUnitSignalPorts {
    explicit ExecuteUnitSignalPorts(ExecuteUnitType unit_type);

    sc_signal<ExecuteUnitPayload> id_ex_payload_;
    sc_signal<bool> id_ex_enable_;
    sc_signal<bool> ready_;
    sc_signal<ResourceAllocatePayload> resource_allocate_;
    sc_signal<ResourceReleasePayload> resource_release_;

    sc_signal<bool> unit_finish_;
};

struct ExecuteUnitRequestIOPorts {
    sc_out<ExecuteUnitPayload> id_ex_payload_port_;
    sc_out<bool> id_ex_enable_port_;
    sc_out<bool> id_finish_port_;

    sc_in<bool> ready_port_;
    sc_in<ResourceAllocatePayload> resource_allocate_;
    sc_in<ResourceReleasePayload> resource_release_;
    sc_in<bool> unit_finish_port_;
};

struct ExecuteUnitResponseIOPorts {
    sc_in<ExecuteUnitPayload> id_ex_payload_port_{"id_ex_payload_port"};
    sc_in<bool> id_ex_enable_port_{"id_ex_enable_port"};
    sc_in<bool> id_finish_port_{"id_finish_port"};

    sc_out<bool> ready_port_{"ready_port"};
    sc_out<ResourceAllocatePayload> resource_allocate_{"resource_allocate"};
    sc_out<ResourceReleasePayload> resource_release_{"resource_release"};
    sc_out<bool> unit_finish_port_{"unit_finish_port"};

    void bind(ExecuteUnitSignalPorts& signals) {
        id_ex_payload_port_.bind(signals.id_ex_payload_);
        id_ex_enable_port_.bind(signals.id_ex_enable_);
        ready_port_.bind(signals.ready_);
        resource_allocate_.bind(signals.resource_allocate_);
        resource_release_.bind(signals.resource_release_);
        unit_finish_port_.bind(signals.unit_finish_);
    }
};

class ExecuteUnit : public BaseModule {
public:
    SC_HAS_PROCESS(ExecuteUnit);

    ExecuteUnit(const sc_module_name& name, const BaseInfo& base_info, Clock* clk, ExecuteUnitType type);

    void checkInst();
    void processReleaseResource();
    void processFinishRun();
    void processIdFinish();

    virtual void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);

    virtual ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload);

protected:
    template <class InsPayload>
    std::shared_ptr<InsPayload> waitForExecuteAndGetPayload() {
        wait(fsm_.start_exec_);
        ports_.ready_port_.write(false);
        running_ins_cnt_++;

        return std::dynamic_pointer_cast<InsPayload>(fsm_out_.read().payload);
    }

    void readyForNextExecute();

    void releaseResource(int ins_id);
    void finishInstruction(double t = 0);

public:
    ExecuteUnitResponseIOPorts ports_;

protected:
    const AddressSapce& as_;
    MemorySocket memory_socket_;

private:
    const ExecuteUnitType type_;

    FSM<ExecuteUnitPayload> fsm_;
    sc_signal<ExecuteUnitPayload> fsm_out_{"fsm_out"};
    sc_signal<FSMPayload<ExecuteUnitPayload>> fsm_in_{"fsm_in"};

    sc_event release_resource_trigger_;
    std::vector<int> release_resource_ins_id_list_{};

    int running_ins_cnt_{0};
    bool finish_decode_{false};
    sc_event finish_run_trigger_;
    bool finish_run_{false};
};

}  // namespace cimsim
