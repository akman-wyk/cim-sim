//
// Created by wyk on 2025/3/25.
//

#include "reduce_unit.h"

#include "fmt/format.h"
#include "profiler/profiler.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

ReduceFunctorPipelineStage::ReduceFunctorPipelineStage(const sc_module_name& name, const BaseInfo& base_info,
                                                       const ReduceFunctorConfig& functor_config,
                                                       EnergyCounter& functor_energy_counter)
    : BaseModule(name, base_info)
    , dynamic_power_mW_(functor_config.dynamic_power_mW)
    , pipeline_stage_latency_cycle_(functor_config.latency_cycle / functor_config.pipeline_stage_cnt)
    , functor_energy_counter_(functor_energy_counter) {
    SC_THREAD(processExecute);
}

ReduceStageSocket* ReduceFunctorPipelineStage::getExecuteSocket() {
    return &exec_socket_;
}

void ReduceFunctorPipelineStage::setNextStageSocket(ReduceStageSocket* next_stage_socket) {
    next_stage_socket_ = next_stage_socket;
}

void ReduceFunctorPipelineStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        CORE_LOG(fmt::format("{} start, pc: {}, ins id: {}, batch: {}", getFullName(), payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        double latency = pipeline_stage_latency_cycle_ * period_ns_;
        functor_energy_counter_.addDynamicEnergyPJ(latency, dynamic_power_mW_,
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

ReduceFunctor::ReduceFunctor(const sc_module_name& name, const BaseInfo& base_info,
                             const ReduceFunctorConfig& functor_config, ReduceStageSocket* next_stage_socket)
    : BaseModule(name, base_info)
    , functor_config_(functor_config)
    , functor_energy_counter_(functor_config_.pipeline_stage_cnt > 1) {
    stage_list_.emplace_back(std::make_shared<ReduceFunctorPipelineStage>("Pipeline_0", base_info, functor_config_,
                                                                          functor_energy_counter_));
    for (int i = 1; i < functor_config_.pipeline_stage_cnt; i++) {
        auto stage_ptr = std::make_shared<ReduceFunctorPipelineStage>(fmt::format("Pipeline_{}", i).c_str(), base_info,
                                                                      functor_config_, functor_energy_counter_);
        stage_list_[i - 1]->setNextStageSocket(stage_ptr->getExecuteSocket());
        stage_list_.emplace_back(stage_ptr);
    }
    stage_list_[stage_list_.size() - 1]->setNextStageSocket(next_stage_socket);

    functor_energy_counter_.setStaticPowerMW(functor_config_.static_power_mW);
}

ReduceStageSocket* ReduceFunctor::getExecuteSocket() const {
    return stage_list_[0]->getExecuteSocket();
}

const ReduceFunctorConfig* ReduceFunctor::getFunctorConfig() const {
    return &functor_config_;
}

EnergyCounter* ReduceFunctor::getEnergyCounterPtr() {
    return &functor_energy_counter_;
}

ReduceUnit::ReduceUnit(const sc_module_name& name, const ReduceUnitConfig& config, const BaseInfo& base_info,
                       Clock* clk)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::reduce), config_(config) {
    SC_THREAD(processIssue)
    SC_THREAD(processReadStage)
    SC_THREAD(processWriteStage)

    for (const auto& functor_config : config_.functor_list) {
        auto functor = std::make_shared<ReduceFunctor>(fmt::format("Functor_{}", functor_config.name).c_str(),
                                                       base_info, functor_config, &write_stage_socket_);
        functor_map_.emplace(&functor_config, functor);
        energy_counter_.addSubEnergyCounter(functor_config.name, functor->getEnergyCounterPtr());
    }
}

void ReduceUnit::processIssue() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<ReduceInsPayload>();

        auto ins_info = decodeAndGetInfo(*payload);
        auto conflict_payload = getDataConflictInfo(*payload);
        ports_.resource_allocate_.write(conflict_payload);

        if (executing_functor_ == nullptr || executing_functor_->getFunctorConfig() != payload->func_cfg) {
            executing_functor_ = functor_map_[payload->func_cfg];
        }

        int process_times = IntDivCeil(payload->length, payload->func_cfg->reduce_input_cnt);
        ReduceStagePayload stage_payload{.ins_info = std::make_shared<ReduceInstructionInfo>(ins_info)};
        for (int batch = 0; batch < process_times; batch++) {
            stage_payload.batch_info = std::make_shared<ReduceBatchInfo>(ReduceBatchInfo{
                .batch_vector_len = (batch == process_times - 1)
                                        ? (payload->length - batch * payload->func_cfg->reduce_input_cnt)
                                        : payload->func_cfg->reduce_input_cnt,
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

void ReduceUnit::processReadStage() {
    while (true) {
        read_stage_socket_.waitUntilStart();

        const auto& payload = read_stage_socket_.payload;
        CORE_LOG(fmt::format("Reduce read start, pc: {}, ins id: {}, batch: {}", payload.ins_info->ins.pc,
                             payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

        int address_byte = payload.ins_info->input_start_address_byte +
                           (payload.batch_info->batch_num * payload.ins_info->functor_config->input_bit_width *
                            payload.ins_info->functor_config->reduce_input_cnt / BYTE_TO_BIT);
        int size_byte =
            payload.ins_info->functor_config->input_bit_width * payload.batch_info->batch_vector_len / BYTE_TO_BIT;
        memory_socket_.readLocal(payload.ins_info->ins, address_byte, size_byte);

        waitAndStartNextStage(payload, *(executing_functor_->getExecuteSocket()));

        if (payload.ins_info->use_pipeline && !payload.batch_info->last_batch) {
            cur_ins_next_batch_.notify();
        }

        read_stage_socket_.finish();
    }
}

void ReduceUnit::processWriteStage() {
    int output_cumulative_cnt{0}, write_cumulative_cnt{0};
    while (true) {
        write_stage_socket_.waitUntilStart();

        const auto& payload = write_stage_socket_.payload;
        output_cumulative_cnt++;

        if (output_cumulative_cnt == payload.ins_info->write_batch_vector_len || payload.batch_info->last_batch) {
            CORE_LOG(fmt::format("Reduce write start, pc: {}, ins id: {}, batch: {}", payload.ins_info->ins.pc,
                                 payload.ins_info->ins.ins_id, payload.batch_info->batch_num));

            if (payload.batch_info->last_batch) {
                releaseResource(payload.ins_info->ins.ins_id);
            }

            int address_byte = payload.ins_info->output_start_address_byte +
                               (write_cumulative_cnt * payload.ins_info->functor_config->output_bit_width *
                                payload.ins_info->write_batch_vector_len / BYTE_TO_BIT);
            int size_byte = payload.ins_info->functor_config->output_bit_width * output_cumulative_cnt / BYTE_TO_BIT;
            memory_socket_.writeLocal(payload.ins_info->ins, address_byte, size_byte, {});

            CORE_LOG(fmt::format("Reduce write end, pc: {}, ins id: {}, batch: {}, the {}th writting, data cnt: {}",
                                 payload.ins_info->ins.pc, payload.ins_info->ins.ins_id, payload.batch_info->batch_num,
                                 write_cumulative_cnt + 1, output_cumulative_cnt));
            output_cumulative_cnt = 0;
            write_cumulative_cnt++;
        }

        if (!payload.ins_info->use_pipeline && !payload.batch_info->last_batch) {
            cur_ins_next_batch_.notify();
        }

        if (payload.batch_info->last_batch) {
            write_cumulative_cnt = 0;
            finishInstruction();
        }

        write_stage_socket_.finish();
    }
}

ResourceAllocatePayload ReduceUnit::getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) {
    return getDataConflictInfo(*std::dynamic_pointer_cast<ReduceInsPayload>(payload));
}

ResourceAllocatePayload ReduceUnit::getDataConflictInfo(const ReduceInsPayload& payload) const {
    ResourceAllocatePayload cur_ins_allocate_payload{.ins_id = payload.ins.ins_id,
                                                     .unit_type = ExecuteUnitType::reduce};
    cur_ins_allocate_payload.addReadMemoryId(as_.getLocalMemoryId(payload.input_address_byte));
    cur_ins_allocate_payload.addWriteMemoryId(as_.getLocalMemoryId(payload.output_address_byte));
    cur_ins_allocate_payload.reduce_functor_cfg = payload.func_cfg;
    return cur_ins_allocate_payload;
}

ReduceInstructionInfo ReduceUnit::decodeAndGetInfo(const ReduceInsPayload& payload) const {
    int read_memory_id = as_.getLocalMemoryId(payload.input_address_byte);
    int write_memory_id = as_.getLocalMemoryId(payload.output_address_byte);

    int write_memory_bit_width =
        memory_socket_.getLocalMemoryDataWidthById(write_memory_id, MemoryAccessType::write) * BYTE_TO_BIT;
    int write_batch_vector_len = IntDivCeil(write_memory_bit_width, payload.output_bit_width);

    bool use_pipeline = config_.pipeline && (read_memory_id != write_memory_id);

    return {.ins = payload.ins,
            .functor_config = payload.func_cfg,
            .input_start_address_byte = payload.input_address_byte,
            .output_start_address_byte = payload.output_address_byte,
            .write_batch_vector_len = write_batch_vector_len,
            .use_pipeline = use_pipeline};
}

}  // namespace cimsim
