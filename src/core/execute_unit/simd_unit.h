//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <string>
#include <unordered_map>
#include <utility>

#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/socket/memory_socket.h"
#include "execute_unit.h"
#include "payload.h"

namespace cimsim {

class MemoryUnit;

struct SIMDInputOutputInfo {
    int data_bit_width{0};
    int start_address_byte{0};
};

struct SIMDInstructionInfo {
    InstructionPayload ins{};

    std::vector<SIMDInputOutputInfo> scalar_inputs{};
    std::vector<SIMDInputOutputInfo> vector_inputs{};
    SIMDInputOutputInfo output{};

    int functor_cnt{0};
    bool use_pipeline{false};
};

struct SIMDBatchInfo {
    int batch_vector_len{0};
    int batch_num{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct SIMDStagePayload {
    SIMDInstructionInfo ins_info;
    SIMDBatchInfo batch_info;
};

using SIMDStageSocket = SubmoduleSocket<SIMDStagePayload>;

void waitAndStartNextStage(const SIMDStagePayload& cur_payload, SIMDStageSocket& next_stage_socket);

class SIMDFunctorPipelineStage : public BaseModule {
public:
    SC_HAS_PROCESS(SIMDFunctorPipelineStage);

    explicit SIMDFunctorPipelineStage(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core,
                                      Clock* clk, const SIMDFunctorConfig& config,
                                      EnergyCounter& functor_energy_counter);

    SIMDStageSocket* getExecuteSocket();
    void setNextStageSocket(SIMDStageSocket* next_stage_socket);
    void clearNextStageSocket();

    [[noreturn]] void processExecute();

private:
    const double dynamic_power_per_functor_mW_{1.0};
    int pipeline_stage_latency_cycle_{1};

    SIMDStageSocket exec_socket_;
    SIMDStageSocket* next_stage_socket_{nullptr};

    EnergyCounter& functor_energy_counter_;
};

class SIMDFunctor : public BaseModule {
public:
    explicit SIMDFunctor(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core, Clock* clk,
                         const SIMDFunctorConfig& functor_config, SIMDStageSocket* next_stage_socket);

    SIMDStageSocket* getExecuteSocket();
    [[nodiscard]] const SIMDFunctorConfig* getFunctorConfig() const;

    [[nodiscard]] EnergyReporter getEnergyReporter() const;

private:
    const SIMDFunctorConfig& functor_config_;

    std::vector<std::shared_ptr<SIMDFunctorPipelineStage>> stage_list_{};

    EnergyCounter functor_energy_counter_;
};

class SIMDUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(SIMDUnit);

    SIMDUnit(const sc_core::sc_module_name& name, const SIMDUnitConfig& config, const SimConfig& sim_config, Core* core,
             Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadStage();
    [[noreturn]] void processWriteStage();

    ResourceAllocatePayload getDataConflictInfo(const SIMDInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

    EnergyReporter getEnergyReporter() override;

private:
    std::pair<SIMDInstructionInfo, ResourceAllocatePayload> decodeAndGetInfo(const SIMDInsPayload& payload) const;

private:
    const SIMDUnitConfig& config_;

    std::unordered_map<const SIMDFunctorConfig*, std::shared_ptr<SIMDFunctor>> functor_map_{};
    std::shared_ptr<SIMDFunctor> executing_functor_{nullptr};

    sc_core::sc_event cur_ins_next_batch_;
    SIMDStageSocket read_stage_socket_{};
    SIMDStageSocket write_stage_socket_{};
};

}  // namespace cimsim
