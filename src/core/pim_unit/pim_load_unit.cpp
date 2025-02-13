//
// Created by wyk on 2024/8/8.
//

#include "pim_load_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

PimLoadUnit::PimLoadUnit(const char *name, const pimsim::PimUnitConfig &config, const pimsim::SimConfig &sim_config,
                         pimsim::Core *core, pimsim::Clock *clk)
    : BaseModule(name, sim_config, core, clk)
    , config_(config)
    , macro_size_(config.macro_size)
    , fsm_("PimLoadFSM", clk) {
    fsm_.input_.bind(fsm_in_);
    fsm_.enable_.bind(ports_.id_ex_enable_port_);
    fsm_.output_.bind(fsm_out_);

    SC_METHOD(checkPimLoadInst)
    sensitive << ports_.id_ex_payload_port_;

    SC_THREAD(processIssue)
    SC_THREAD(processExecute)

    SC_METHOD(finishInstruction)
    sensitive << finish_ins_trigger_;

    SC_METHOD(finishRun)
    sensitive << finish_run_trigger_;
}

void PimLoadUnit::bindLocalMemoryUnit(pimsim::LocalMemoryUnit *local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

EnergyReporter PimLoadUnit::getEnergyReporter() {
    EnergyReporter reporter;
    reporter.addSubModule("sram write", EnergyReporter{sram_write_energy_counter_});
    reporter.addSubModule("pim load", EnergyReporter{this->energy_counter_});
    return std::move(reporter);
}

void PimLoadUnit::checkPimLoadInst() {
    if (const auto &payload = ports_.id_ex_payload_port_.read(); payload.ins.valid()) {
        fsm_in_.write({payload, true});
    } else {
        fsm_in_.write({{}, false});
    }
}

void PimLoadUnit::processIssue() {
    while (true) {
        wait(fsm_.start_exec_);

        ports_.busy_port_.write(true);

        const auto &payload = fsm_out_.read();
        LOG(fmt::format("Pim load start, pc: {}", payload.ins.pc));

        DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::pim_load};
        conflict_payload.use_pim_unit = true;
        conflict_payload.addReadMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.src_address_byte));
        ports_.data_conflict_port_.write(conflict_payload);

        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = payload;
        execute_socket_.start_exec.notify();

        ports_.busy_port_.write(false);
        fsm_.finish_exec_.notify(SC_ZERO_TIME);
    }
}

void PimLoadUnit::processExecute() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        LOG(fmt::format("Pim load start execute, pc: {}", payload.ins.pc));
        auto start_exec_time = sc_core::sc_time_stamp();

        // read weight data
        local_memory_socket_.readData(payload.ins, payload.src_address_byte, payload.size_byte);

        // calculate config
        int macro_bit_width = macro_size_.bit_width_per_row * macro_size_.element_cnt_per_compartment;
        int pim_bit_width = config_.sram.as_mode == +CimASMode::intergroup
                                ? macro_bit_width * config_.macro_total_cnt
                                : macro_bit_width * config_.macro_group_size;
        int weight_bit_size = payload.size_byte * BYTE_TO_BIT;
        int process_times = IntDivCeil(weight_bit_size, pim_bit_width);

        // load weight
        double dynamic_power_mW = config_.sram.write_dynamic_power_per_bit_mW * pim_bit_width;
        double latency = config_.sram.write_latency_cycle * period_ns_;
        for (int i = 0; i < process_times; i++) {
            sram_write_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);

            if (i == process_times - 1) {
                finish_ins_ = true;
                finish_ins_id_ = payload.ins.ins_id;
                finish_ins_trigger_.notify(SC_ZERO_TIME);
            }

            wait(latency, SC_NS);
        }

        if (isEndPC(payload.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            finish_run_ = true;
            finish_run_trigger_.notify(SC_ZERO_TIME);
        }

        auto exec_time = sc_time_stamp() - start_exec_time;
        this->energy_counter_.addDynamicEnergyPJ(exec_time.to_seconds() * 1e9, 0);

        execute_socket_.finish();
    }
}

void PimLoadUnit::finishInstruction() {
    ports_.finish_ins_port_.write(finish_ins_);
    ports_.finish_ins_id_port_.write(finish_ins_id_);
}

void PimLoadUnit::finishRun() {
    ports_.finish_run_port_.write(finish_run_);
}

}  // namespace pimsim
