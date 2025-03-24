//
// Created by wyk on 2025/3/24.
//

#pragma once
#include "base_component/base_module.h"
#include "base_component/submodule_socket.h"
#include "payload.h"

namespace cimsim {

using MacroGroupStageSocket = SubmoduleSocket<MacroGroupSubmodulePayload>;

class MacroGroupPipelineStage : public BaseModule {
public:
    MacroGroupPipelineStage(const sc_module_name& name, const BaseInfo& base_info, int latency_cycle);

    [[noreturn]] virtual void processExecute() = 0;

public:
    MacroGroupStageSocket exec_socket_;
    MacroGroupStageSocket* next_stage_socket_{nullptr};
    bool last_batch_trigger_next_{false};

protected:
    int latency_cycle_;
};

class MacroGroupPipelineNormalStage : public MacroGroupPipelineStage {
public:
    SC_HAS_PROCESS(MacroGroupPipelineNormalStage);

    MacroGroupPipelineNormalStage(const sc_module_name& name, const BaseInfo& base_info, int latency_cycle);

    [[noreturn]] void processExecute() override;
};

class MacroGroupPipelineLastStage : public MacroGroupPipelineStage {
public:
    SC_HAS_PROCESS(MacroGroupPipelineLastStage);

    MacroGroupPipelineLastStage(const sc_module_name& name, const BaseInfo& base_info, int latency_cycle);

    [[noreturn]] void processExecute() override;

public:
    std::function<void(int ins_id)> release_resource_func_;
    std::function<void()> finish_ins_func_;
};

class MacroGroupModule : public BaseModule {
public:
    MacroGroupModule(const sc_module_name& name, const BaseInfo& base_info, int latency_cycle, int pipeline_stage_cnt,
                     bool last_module);

    MacroGroupModule(const sc_module_name& name, const BaseInfo& base_info, const CimModuleConfig& module_config,
                     bool last_module);

    MacroGroupStageSocket* getExecuteSocket() const;
    void bindNextStageSocket(MacroGroupStageSocket* next_stage_socket, bool last_batch_trigger);

    void setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func);
    void setFinishInsFunc(std::function<void()> finish_ins_func);

private:
    std::vector<std::shared_ptr<MacroGroupPipelineStage>> stage_list_{};
    bool last_module_;
};

}  // namespace cimsim
