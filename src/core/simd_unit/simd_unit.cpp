//
// Created by wyk on 2024/7/5.
//

#include "simd_unit.h"

#include "fmt/core.h"
#include "util/log.h"
#include "util/util.h"

namespace pimsim {

SIMDUnit::SIMDUnit(const char* name, const SIMDUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk)
    : ExecuteUnit(name, sim_config, core, clk, ExecuteUnitType::simd), config_(config) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadSubmodule)
    SC_THREAD(processExecuteSubmodule)
    SC_THREAD(processWriteSubmodule)

    for (const auto& ins_config : config_.instruction_list) {
        instruction_config_map_.emplace(getSIMDInstructionIdentityCode(ins_config.input_cnt, ins_config.opcode),
                                        &ins_config);
    }

    double SIMD_functors_total_static_power_mW = 0.0;
    for (const auto& functor_config : config_.functor_list) {
        SIMD_functors_total_static_power_mW += functor_config.static_power_per_functor_mW * functor_config.functor_cnt;
        functor_config_map_.emplace(functor_config.name, &functor_config);
    }

    energy_counter_.setStaticPowerMW(SIMD_functors_total_static_power_mW);
}

void SIMDUnit::processIssue() {
    while (true) {
        auto payload = waitForExecuteAndGetPayload<SIMDInsPayload>();

        // Find instruction and functor
        const auto [instruction, functor] = getSIMDInstructionAndFunctor(*payload);
        if (instruction == nullptr || functor == nullptr) {
            if (instruction == nullptr) {
                throw std::runtime_error(
                    fmt::format("No match inst, Invalid SIMD instruction: \n{}", payload->toString()));
            } else {
                throw std::runtime_error(
                    fmt::format("No match functor, Invalid SIMD instruction: \n{}", payload->toString()));
            }
        }

        // Decode instruction
        const auto& [ins_info, conflict_payload] = decodeAndGetInfo(instruction, functor, *payload);
        ports_.data_conflict_port_.write(conflict_payload);

        int vector_total_len = ins_info.vector_inputs.empty() ? 1 : payload->len;
        int process_times = IntDivCeil(vector_total_len, functor->functor_cnt);
        SIMDSubmodulePayload submodule_payload{.ins_info = ins_info};
        for (int batch = 0; batch < process_times; batch++) {
            submodule_payload.batch_info = {.batch_vector_len = (batch == process_times - 1)
                                                                    ? (vector_total_len - batch * functor->functor_cnt)
                                                                    : functor->functor_cnt,
                                            .batch_num = batch,
                                            .first_batch = (batch == 0),
                                            .last_batch = (batch == process_times - 1)};
            waitAndStartNextSubmodule(submodule_payload, read_submodule_socket_);

            if (!submodule_payload.batch_info.last_batch) {
                wait(cur_ins_next_batch_);
            }
        }

        readyForNextExecute();
    }
}

void SIMDUnit::processReadSubmodule() {
    while (true) {
        read_submodule_socket_.waitUntilStart();

        const auto& payload = read_submodule_socket_.payload;
        LOG(fmt::format("simd read start, pc: {}, ins id: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.ins_info.ins.ins_id, payload.batch_info.batch_num));

        if (payload.batch_info.first_batch) {
            for (const auto& scalar_input : payload.ins_info.scalar_inputs) {
                local_memory_socket_.readData(payload.ins_info.ins, scalar_input.start_address_byte,
                                              scalar_input.data_bit_width / BYTE_TO_BIT);
            }
        }

        for (const auto& vector_input : payload.ins_info.vector_inputs) {
            int address_byte =
                vector_input.start_address_byte + (payload.batch_info.batch_num * vector_input.data_bit_width *
                                                   payload.ins_info.functor_config->functor_cnt / BYTE_TO_BIT);
            int size_byte = vector_input.data_bit_width * payload.batch_info.batch_vector_len / BYTE_TO_BIT;
            local_memory_socket_.readData(payload.ins_info.ins, address_byte, size_byte);
        }

        waitAndStartNextSubmodule(payload, execute_submodule_socket_);

        if (payload.ins_info.use_pipeline && !payload.batch_info.last_batch) {
            cur_ins_next_batch_.notify();
        }

        read_submodule_socket_.finish();
    }
}

void SIMDUnit::processExecuteSubmodule() {
    while (true) {
        execute_submodule_socket_.waitUntilStart();

        const auto& payload = execute_submodule_socket_.payload;
        LOG(fmt::format("simd execute start, pc: {}, ins id: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.ins_info.ins.ins_id, payload.batch_info.batch_num));

        double dynamic_power_mW =
            payload.ins_info.functor_config->dynamic_power_per_functor_mW * payload.batch_info.batch_vector_len;
        double latency = payload.ins_info.functor_config->latency_cycle * period_ns_;
        energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW);
        wait(latency, SC_NS);

        waitAndStartNextSubmodule(payload, write_submodule_socket_);

        execute_submodule_socket_.finish();
    }
}

void SIMDUnit::processWriteSubmodule() {
    while (true) {
        write_submodule_socket_.waitUntilStart();

        const auto& payload = write_submodule_socket_.payload;
        LOG(fmt::format("simd write start, pc: {}, ins id: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.ins_info.ins.ins_id, payload.batch_info.batch_num));

        if (payload.batch_info.last_batch) {
            triggerFinishInstruction(payload.ins_info.ins.ins_id);
        }

        int address_byte = payload.ins_info.output.start_address_byte +
                           (payload.batch_info.batch_num * payload.ins_info.output.data_bit_width *
                            payload.ins_info.functor_config->functor_cnt / BYTE_TO_BIT);
        int size_byte = payload.ins_info.output.data_bit_width * payload.batch_info.batch_vector_len / BYTE_TO_BIT;
        local_memory_socket_.writeData(payload.ins_info.ins, address_byte, size_byte, {});

        LOG(fmt::format("simd write end, pc: {}, ins id: {}, batch: {}", payload.ins_info.ins.pc,
                        payload.ins_info.ins.ins_id, payload.batch_info.batch_num));

        if (!payload.ins_info.use_pipeline && !payload.batch_info.last_batch) {
            cur_ins_next_batch_.notify();
        }

        write_submodule_socket_.finish();

        if (payload.batch_info.last_batch && isEndPC(payload.ins_info.ins.pc) && sim_mode_ == +SimMode::run_one_round) {
            triggerFinishRun();
        }
    }
}

void SIMDUnit::bindLocalMemoryUnit(LocalMemoryUnit* local_memory_unit) {
    local_memory_socket_.bindLocalMemoryUnit(local_memory_unit);
}

unsigned int SIMDUnit::getSIMDInstructionIdentityCode(unsigned int input_cnt, unsigned int opcode) {
    return ((input_cnt << SIMD_INSTRUCTION_OPCODE_BIT_LENGTH) | opcode);
}

void SIMDUnit::waitAndStartNextSubmodule(const pimsim::SIMDSubmodulePayload& cur_payload,
                                         SubmoduleSocket<pimsim::SIMDSubmodulePayload>& next_submodule_socket) {
    next_submodule_socket.waitUntilFinishIfBusy();
    if (cur_payload.batch_info.first_batch) {
        next_submodule_socket.payload.ins_info = cur_payload.ins_info;
    }
    next_submodule_socket.payload.batch_info = cur_payload.batch_info;
    next_submodule_socket.start_exec.notify();
}

std::pair<const SIMDInstructionConfig*, const SIMDFunctorConfig*> SIMDUnit::getSIMDInstructionAndFunctor(
    const SIMDInsPayload& payload) {
    auto ins_found = instruction_config_map_.find(getSIMDInstructionIdentityCode(payload.input_cnt, payload.opcode));
    if (ins_found == instruction_config_map_.end()) {
        return {nullptr, nullptr};
    }

    auto* ins_config = ins_found->second;
    for (const auto& bind_info : ins_config->functor_binding_list) {
        if (bind_info.input_bit_width.inputs == payload.inputs_bit_width) {
            if (auto functor_found = functor_config_map_.find(bind_info.functor_name);
                functor_found != functor_config_map_.end()) {
                if (auto* functor_config = functor_found->second;
                    payload.input_cnt == functor_config->input_cnt &&
                    payload.inputs_bit_width == functor_config->data_bit_width.inputs) {
                    return {ins_config, functor_config};
                }
            }
        }
    }

    return {ins_config, nullptr};
}

std::pair<SIMDInstructionInfo, DataConflictPayload> SIMDUnit::decodeAndGetInfo(const SIMDInstructionConfig* instruction,
                                                                               const SIMDFunctorConfig* functor,
                                                                               const SIMDInsPayload& payload) const {
    SIMDInputOutputInfo output = {payload.output_bit_width, payload.output_address_byte};
    std::vector<SIMDInputOutputInfo> vector_inputs;
    std::vector<SIMDInputOutputInfo> scalar_inputs;
    for (unsigned int i = 0; i < payload.input_cnt; i++) {
        if (instruction->inputs_type[i] == +SIMDInputType::vector) {
            vector_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        } else {
            scalar_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        }
    }

    DataConflictPayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::simd};
    for (const auto& vector_input : vector_inputs) {
        conflict_payload.addReadMemoryId(
            local_memory_socket_.getLocalMemoryIdByAddress(vector_input.start_address_byte));
    }
    conflict_payload.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(output.start_address_byte));

    bool use_pipeline =
        config_.pipeline && !SetsIntersection(conflict_payload.write_memory_id, conflict_payload.read_memory_id);
    SIMDInstructionInfo ins_info{.ins = payload.ins,
                                 .scalar_inputs = scalar_inputs,
                                 .vector_inputs = vector_inputs,
                                 .output = output,
                                 .functor_config = functor,
                                 .use_pipeline = use_pipeline};

    return {ins_info, std::move(conflict_payload)};
}

DataConflictPayload SIMDUnit::getDataConflictInfo(const SIMDInsPayload& payload) const {
    DataConflictPayload cur_ins_conflict_info{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::simd};
    for (unsigned int i = 0; i < payload.input_cnt; i++) {
        cur_ins_conflict_info.addReadMemoryId(
            local_memory_socket_.getLocalMemoryIdByAddress(payload.inputs_address_byte[i]));
    }
    cur_ins_conflict_info.addWriteMemoryId(local_memory_socket_.getLocalMemoryIdByAddress(payload.output_address_byte));
    return std::move(cur_ins_conflict_info);
}

DataConflictPayload SIMDUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<SIMDInsPayload>(payload));
}

}  // namespace pimsim
