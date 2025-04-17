//
// Created by wyk on 2024/7/5.
//

#include "simd_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

SIMDFunctorPipelineStage::SIMDFunctorPipelineStage(const sc_module_name& name, const BaseInfo& base_info,
                                                   const SIMDFunctorConfig& config,
                                                   EnergyCounter& functor_energy_counter)
    : BaseModule(name, base_info)
    , dynamic_power_per_functor_mW_(config.dynamic_power_per_functor_mW)
    , pipeline_stage_latency_cycle_(config.latency_cycle / config.pipeline_stage_cnt)
    , functor_energy_counter_(functor_energy_counter) {
    SC_THREAD(processExecute);
}

SIMDStageSocket* SIMDFunctorPipelineStage::getExecuteSocket() {
    return &exec_socket_;
}

void SIMDFunctorPipelineStage::setNextStageSocket(SIMDStageSocket* next_stage_socket) {
    next_stage_socket_ = next_stage_socket;
}

void SIMDFunctorPipelineStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        CORE_LOG(fmt::format("{} start, pc: {}, ins id: {}, batch: {}", getFullName(), payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        double dynamic_power_mW = dynamic_power_per_functor_mW_ * payload.batch_info->batch_vector_len;
        double latency = pipeline_stage_latency_cycle_ * period_ns_;
        functor_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW,
                                                   {.core_id = core_id_,
                                                    .ins_id = payload.ins_info->ins.ins_id,
                                                    .inst_opcode = payload.ins_info->ins.inst_opcode,
                                                    .inst_group_tag = payload.ins_info->ins.inst_group_tag,
                                                    .inst_profiler_operator = InstProfilerOperator::computation});
        wait(latency, SC_NS);

        waitAndStartNextStage(payload, *next_stage_socket_);

        exec_socket_.finish();
    }
}

SIMDFunctor::SIMDFunctor(const sc_module_name& name, const BaseInfo& base_info, const SIMDFunctorConfig& functor_config,
                         SIMDStageSocket* next_stage_socket)
    : BaseModule(name, base_info)
    , functor_config_(functor_config)
    , functor_energy_counter_(functor_config_.pipeline_stage_cnt > 1) {
    stage_list_.emplace_back(
        std::make_shared<SIMDFunctorPipelineStage>("Pipeline_0", base_info, functor_config_, functor_energy_counter_));

    for (int i = 1; i < functor_config_.pipeline_stage_cnt; i++) {
        auto stage_ptr = std::make_shared<SIMDFunctorPipelineStage>(fmt::format("Pipeline_{}", i).c_str(), base_info,
                                                                    functor_config_, functor_energy_counter_);
        stage_list_[i - 1]->setNextStageSocket(stage_ptr->getExecuteSocket());
        stage_list_.emplace_back(stage_ptr);
    }
    stage_list_[stage_list_.size() - 1]->setNextStageSocket(next_stage_socket);

    functor_energy_counter_.setStaticPowerMW(functor_config_.static_power_per_functor_mW * functor_config_.functor_cnt);
}

SIMDStageSocket* SIMDFunctor::getExecuteSocket() const {
    return stage_list_[0]->getExecuteSocket();
}

const SIMDFunctorConfig* SIMDFunctor::getFunctorConfig() const {
    return &functor_config_;
}

EnergyCounter* SIMDFunctor::getEnergyCounterPtr() {
    return &functor_energy_counter_;
}

SIMDUnit::SIMDUnit(const sc_module_name& name, const SIMDUnitConfig& config, const BaseInfo& base_info, Clock* clk)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::simd), config_(config) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadStage)
    SC_THREAD(processWriteStage)

    for (const auto& functor_config : config_.functor_list) {
        auto functor = std::make_shared<SIMDFunctor>(fmt::format("Functor_{}", functor_config.name).c_str(), base_info,
                                                     functor_config, &write_stage_socket_);
        functor_map_.emplace(&functor_config, functor);
        energy_counter_.addSubEnergyCounter(functor_config.name, functor->getEnergyCounterPtr());
    }
}

void SIMDUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<SIMDInsPayload>();

        // Decode instruction
        const auto& [ins_info, conflict_payload] = decodeAndGetInfo(*payload);
        ports_.resource_allocate_.write(conflict_payload);

        if (executing_functor_ == nullptr || executing_functor_->getFunctorConfig() != payload->func_cfg) {
            executing_functor_ = functor_map_[payload->func_cfg];
        }

        for (const auto& scalar_input : ins_info.scalar_inputs) {
            memory_socket_.readLocal(ins_info.ins, scalar_input.start_address_byte,
                                     scalar_input.data_bit_width / BYTE_TO_BIT);
        }

        int vector_total_len = ins_info.vector_inputs.empty() ? 1 : payload->len;
        int process_times = IntDivCeil(vector_total_len, payload->func_cfg->functor_cnt);
        SIMDStagePayload stage_payload{.ins_info = std::make_shared<SIMDInstructionInfo>(ins_info)};
        for (int batch = 0; batch < process_times; batch++) {
            stage_payload.batch_info = std::make_shared<SIMDBatchInfo>(
                SIMDBatchInfo{.batch_vector_len = (batch == process_times - 1)
                                                      ? (vector_total_len - batch * payload->func_cfg->functor_cnt)
                                                      : payload->func_cfg->functor_cnt,
                              .batch_num = batch,
                              .last_batch = (batch == process_times - 1)});
            waitAndStartNextStage(stage_payload, read_stage_socket_);

            if (!stage_payload.batch_info->last_batch) {
                wait(cur_ins_next_batch_);
            }
        }

        readyForNextExecute();
    }
}

void SIMDUnit::processReadStage() {
    while (true) {
        read_stage_socket_.waitUntilStart();

        const auto& payload = read_stage_socket_.payload;
        CORE_LOG(fmt::format("SIMD read start, pc: {}, ins id: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        for (const auto& vector_input : payload.ins_info->vector_inputs) {
            int address_byte =
                vector_input.start_address_byte + (payload.batch_info->batch_num * vector_input.data_bit_width *
                                                   payload.ins_info->functor_cnt / BYTE_TO_BIT);
            int size_byte = vector_input.data_bit_width * payload.batch_info->batch_vector_len / BYTE_TO_BIT;
            memory_socket_.readLocal(payload.ins_info->ins, address_byte, size_byte);
        }

        waitAndStartNextStage(payload, *(executing_functor_->getExecuteSocket()));

        if (payload.ins_info->use_pipeline && !payload.batch_info->last_batch) {
            cur_ins_next_batch_.notify();
        }

        read_stage_socket_.finish();
    }
}

void SIMDUnit::processWriteStage() {
    while (true) {
        write_stage_socket_.waitUntilStart();

        const auto& payload = write_stage_socket_.payload;
        CORE_LOG(fmt::format("SIMD write start, pc: {}, ins id: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        if (payload.batch_info->last_batch) {
            releaseResource(payload.ins_info->ins.ins_id);
        }

        int address_byte = payload.ins_info->output.start_address_byte +
                           (payload.batch_info->batch_num * payload.ins_info->output.data_bit_width *
                            payload.ins_info->functor_cnt / BYTE_TO_BIT);
        int size_byte = payload.ins_info->output.data_bit_width * payload.batch_info->batch_vector_len / BYTE_TO_BIT;
        memory_socket_.writeLocal(payload.ins_info->ins, address_byte, size_byte, {});

        CORE_LOG(fmt::format("simd write end, pc: {}, ins id: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        if (!payload.ins_info->use_pipeline && !payload.batch_info->last_batch) {
            cur_ins_next_batch_.notify();
        }

        if (payload.batch_info->last_batch) {
            finishInstruction();
        }

        write_stage_socket_.finish();
    }
}

std::pair<SIMDInstructionInfo, ResourceAllocatePayload> SIMDUnit::decodeAndGetInfo(
    const SIMDInsPayload& payload) const {
    SIMDInputOutputInfo output = {payload.output_bit_width, payload.output_address_byte};
    std::vector<SIMDInputOutputInfo> vector_inputs;
    std::vector<SIMDInputOutputInfo> scalar_inputs;
    for (unsigned int i = 0; i < payload.ins_cfg->input_cnt; i++) {
        if (payload.ins_cfg->inputs_type[i] == +SIMDInputType::vector) {
            vector_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        } else {
            scalar_inputs.emplace_back(
                SIMDInputOutputInfo{payload.inputs_bit_width[i], payload.inputs_address_byte[i]});
        }
    }

    ResourceAllocatePayload conflict_payload{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::simd};
    for (const auto& vector_input : vector_inputs) {
        conflict_payload.addReadMemoryId(as_.getLocalMemoryId(vector_input.start_address_byte));
    }
    conflict_payload.addWriteMemoryId(as_.getLocalMemoryId(output.start_address_byte));
    conflict_payload.simd_functor_cfg = payload.func_cfg;

    bool use_pipeline =
        config_.pipeline && !conflict_payload.write_memory_id.intersectionWith(conflict_payload.read_memory_id);
    SIMDInstructionInfo ins_info{.ins = payload.ins,
                                 .scalar_inputs = scalar_inputs,
                                 .vector_inputs = vector_inputs,
                                 .output = output,
                                 .functor_cnt = payload.func_cfg->functor_cnt,
                                 .use_pipeline = use_pipeline};

    return {ins_info, conflict_payload};
}

ResourceAllocatePayload SIMDUnit::getDataConflictInfo(const SIMDInsPayload& payload) const {
    ResourceAllocatePayload cur_ins_conflict_info{.ins_id = payload.ins.ins_id, .unit_type = ExecuteUnitType::simd};
    for (unsigned int i = 0; i < payload.ins_cfg->input_cnt; i++) {
        cur_ins_conflict_info.addReadMemoryId(as_.getLocalMemoryId(payload.inputs_address_byte[i]));
    }
    cur_ins_conflict_info.addWriteMemoryId(as_.getLocalMemoryId(payload.output_address_byte));
    cur_ins_conflict_info.simd_functor_cfg = payload.func_cfg;
    return cur_ins_conflict_info;
}

ResourceAllocatePayload SIMDUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<SIMDInsPayload>(payload));
}

}  // namespace cimsim
