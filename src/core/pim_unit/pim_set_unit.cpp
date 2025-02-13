//
// Created by wyk on 2024/8/1.
//

#include "pim_set_unit.h"

#include "fmt/format.h"
#include "pim_compute_unit.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimSetUnit::PimSetUnit(const char *name, const pimsim::PimUnitConfig &config, const pimsim::SimConfig &sim_config,
                       pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk), config_(config), macro_size_(config.macro_size), fsm_("PimSetFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimSetInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void PimSetUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

void PimSetUnit::bindCimUnit(CimUnit *cim_unit) {
    cim_unit_ = cim_unit;
}


void PimSetUnit::checkPimSetInst() {
    if (const auto &payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimSetUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        ports_.busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim set start, pc: {}", payload.ins.pc));

        DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_set};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addReadMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.mask_addr_byte));
        ports_.data_conflict_port_.write(conflict_payload);

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        ports_.busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void PimSetUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Pim set start execute, pc: {}", payload.ins.pc));

        // read mask
        int mask_size_byte =
            IntDivCeil(1 * macro_size_.element_cnt_per_compartment * config_.macro_group_size, BYTE_TO_BIT);
        auto mask_byte_data = local_memory_socket_.readData(payload.ins, payload.mask_addr_byte, mask_size_byte);

        finish_ins_ = true;
        finish_ins_id_ = payload.ins.ins_id;
        finish_ins_trigger_.notify(SC_ZERO_TIME);

        if (cim_unit_ != nullptr) {
            cim_unit_->setMacroGroupActivationElementColumn(mask_byte_data, payload.group_broadcast,
                                                                    payload.group_id);
        }

        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }

        execute_socket_.finish();
    }
}

void PimSetUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_id_port_.write(finish_ins_id_);
}

void PimSetUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

}  // namespace pimsim
