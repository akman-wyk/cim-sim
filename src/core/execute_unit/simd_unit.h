//
// Created by wyk on 2024/7/5.
//

#pragma once
#include <unordered_map>
#include <utility>

#include "base_component/submodule_socket.h"
#include "config/config.h"
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
    bool last_batch{false};
};

struct SIMDStagePayload {
    std::shared_ptr<SIMDInstructionInfo> ins_info;
    std::shared_ptr<SIMDBatchInfo> batch_info;
};

using SIMDStageSocket = SubmoduleSocket<SIMDStagePayload>;

class SIMDFunctorPipelineStage : public BaseModule {
public:
    SC_HAS_PROCESS(SIMDFunctorPipelineStage);

    SIMDFunctorPipelineStage(const sc_module_name& name, const BaseInfo& base_info, const SIMDFunctorConfig& config,
                             EnergyCounter& functor_energy_counter, const std::string& functor_name);

    SIMDStageSocket* getExecuteSocket();
    void setNextStageSocket(SIMDStageSocket* next_stage_socket);

    [[noreturn]] void processExecute();

private:
    const double dynamic_power_per_functor_mW_{1.0};
    const int pipeline_stage_latency_cycle_{1};

    SIMDStageSocket exec_socket_;
    SIMDStageSocket* next_stage_socket_{nullptr};

    EnergyCounter& functor_energy_counter_;
    const std::string& functor_name_;
};

class SIMDFunctor : public BaseModule {
public:
    SIMDFunctor(const sc_module_name& name, const BaseInfo& base_info, const SIMDFunctorConfig& functor_config,
                SIMDStageSocket* next_stage_socket);

    [[nodiscard]] SIMDStageSocket* getExecuteSocket() const;
    [[nodiscard]] const SIMDFunctorConfig* getFunctorConfig() const;

    EnergyCounter* getEnergyCounterPtr() override;

private:
    const SIMDFunctorConfig& functor_config_;

    std::vector<std::shared_ptr<SIMDFunctorPipelineStage>> stage_list_{};

    EnergyCounter functor_energy_counter_;
};

class SIMDUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(SIMDUnit);

    SIMDUnit(const sc_module_name& name, const SIMDUnitConfig& config, const BaseInfo& base_info, Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadStage();
    [[noreturn]] void processWriteStage();

    ResourceAllocatePayload getDataConflictInfo(const SIMDInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    std::pair<SIMDInstructionInfo, ResourceAllocatePayload> decodeAndGetInfo(const SIMDInsPayload& payload) const;

private:
    const SIMDUnitConfig& config_;

    std::unordered_map<const SIMDFunctorConfig*, std::shared_ptr<SIMDFunctor>> functor_map_{};
    std::shared_ptr<SIMDFunctor> executing_functor_{nullptr};

    sc_event cur_ins_next_batch_;
    SIMDStageSocket read_stage_socket_{};
    SIMDStageSocket write_stage_socket_{};
};

}  // namespace cimsim
