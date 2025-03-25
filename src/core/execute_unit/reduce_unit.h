//
// Created by wyk on 2025/3/25.
//

#pragma once

#include "base_component/base_module.h"
#include "base_component/submodule_socket.h"
#include "execute_unit.h"
#include "payload.h"

namespace cimsim {

struct ReduceInstructionInfo {
    InstructionPayload ins{};

    const ReduceFunctorConfig* functor_config{nullptr};
    int input_start_address_byte{0}, output_start_address_byte{0};
    int write_batch_vector_len{0};

    bool use_pipeline{false};
};

struct ReduceBatchInfo {
    int batch_vector_len{0};
    int batch_num{0};
    bool last_batch{false};
};

struct ReduceStagePayload {
    std::shared_ptr<ReduceInstructionInfo> ins_info;
    std::shared_ptr<ReduceBatchInfo> batch_info;
};

using ReduceStageSocket = SubmoduleSocket<ReduceStagePayload>;

class ReduceFunctorPipelineStage : public BaseModule {
public:
    SC_HAS_PROCESS(ReduceFunctorPipelineStage);

    ReduceFunctorPipelineStage(const sc_module_name& name, const BaseInfo& base_info,
                               const ReduceFunctorConfig& functor_config, EnergyCounter& functor_energy_counter);

    ReduceStageSocket* getExecuteSocket();
    void setNextStageSocket(ReduceStageSocket* next_stage_socket);

    [[noreturn]] void processExecute();

private:
    const double dynamic_power_mW_{1.0};
    const int pipeline_stage_latency_cycle_{1};

    ReduceStageSocket exec_socket_;
    ReduceStageSocket* next_stage_socket_{nullptr};

    EnergyCounter& functor_energy_counter_;
};

class ReduceFunctor : public BaseModule {
public:
    ReduceFunctor(const sc_module_name& name, const BaseInfo& base_info, const ReduceFunctorConfig& functor_config,
                  ReduceStageSocket* next_stage_socket);

    [[nodiscard]] ReduceStageSocket* getExecuteSocket() const;
    [[nodiscard]] const ReduceFunctorConfig* getFunctorConfig() const;
    [[nodiscard]] EnergyReporter getEnergyReporter() const;

private:
    const ReduceFunctorConfig& functor_config_;

    std::vector<std::shared_ptr<ReduceFunctorPipelineStage>> stage_list_{};

    EnergyCounter functor_energy_counter_;
};

class ReduceUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(ReduceUnit);

    ReduceUnit(const sc_module_name& name, const ReduceUnitConfig& config, const BaseInfo& base_info, Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadStage();
    [[noreturn]] void processWriteStage();

    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

    EnergyReporter getEnergyReporter() override;

private:
    ResourceAllocatePayload getDataConflictInfo(const ReduceInsPayload& payload) const;
    ReduceInstructionInfo decodeAndGetInfo(const ReduceInsPayload& payload) const;

private:
    const ReduceUnitConfig& config_;

    std::unordered_map<const ReduceFunctorConfig*, std::shared_ptr<ReduceFunctor>> functor_map_{};
    std::shared_ptr<ReduceFunctor> executing_functor_{nullptr};

    sc_event cur_ins_next_batch_;
    ReduceStageSocket read_stage_socket_{};
    ReduceStageSocket write_stage_socket_{};
};

}  // namespace cimsim
