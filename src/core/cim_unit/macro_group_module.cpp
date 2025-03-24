//
// Created by wyk on 2025/3/24.
//

#include "macro_group_module.h"

#include "fmt/format.h"
#include "util/log.h"

namespace cimsim {

MacroGroupPipelineStage::MacroGroupPipelineStage(const sc_core::sc_module_name& name, const SimConfig& sim_config,
                                                 Core* core, Clock* clk, const std::string& macro_group_name,
                                                 const std::string& module_name, int pipeline_stage_latency_cycle)
    : BaseModule(name, sim_config, core, clk)
    , macro_group_name_(macro_group_name)
    , module_name_(module_name)
    , pipeline_stage_latency_cycle_(pipeline_stage_latency_cycle) {}

MacroGroupPipelineNormalStage::MacroGroupPipelineNormalStage(const sc_core::sc_module_name& name,
                                                             const SimConfig& sim_config, Core* core, Clock* clk,
                                                             const std::string& macro_group_name,
                                                             const std::string& module_name,
                                                             int pipeline_stage_latency_cycle)
    : MacroGroupPipelineStage(name, sim_config, core, clk, macro_group_name, module_name,
                              pipeline_stage_latency_cycle) {
    SC_THREAD(processExecute)
}

[[noreturn]] void MacroGroupPipelineNormalStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        const auto& cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} {} {} start, ins pc: {}, sub ins num: {}, batch: {}", macro_group_name_, module_name_,
                             getName(), cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        double latency = pipeline_stage_latency_cycle_ * period_ns_;
        wait(latency, SC_NS);

        if (next_stage_socket_ != nullptr && (!last_batch_trigger_next_ || payload.batch_info->last_batch)) {
            waitAndStartNextStage(payload, *next_stage_socket_);
        }

        exec_socket_.finish();
    }
}

MacroGroupPipelineLastStage::MacroGroupPipelineLastStage(const sc_core::sc_module_name& name,
                                                         const SimConfig& sim_config, Core* core, Clock* clk,
                                                         const std::string& macro_group_name,
                                                         const std::string& module_name,
                                                         int pipeline_stage_latency_cycle)
    : MacroGroupPipelineStage(name, sim_config, core, clk, macro_group_name, module_name,
                              pipeline_stage_latency_cycle) {
    SC_THREAD(processExecute)
}

[[noreturn]] void MacroGroupPipelineLastStage::processExecute() {
    while (true) {
        exec_socket_.waitUntilStart();

        const auto& payload = exec_socket_.payload;
        const auto& cim_ins_info = payload.sub_ins_info->cim_ins_info;
        CORE_LOG(fmt::format("{} {} {} start, ins pc: {}, sub ins num: {}, batch: {}", macro_group_name_, module_name_,
                             getName(), cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        if (release_resource_func_ && payload.sub_ins_info->last_group && cim_ins_info.last_sub_ins) {
            release_resource_func_(cim_ins_info.ins_id);
        }

        double latency = pipeline_stage_latency_cycle_ * period_ns_;
        wait(latency, SC_NS);

        if (finish_ins_func_ && payload.sub_ins_info->last_group && cim_ins_info.last_sub_ins) {
            finish_ins_func_();
        }

        CORE_LOG(fmt::format("{} {} {} end, ins pc: {}, sub ins num: {}, batch: {}", macro_group_name_, module_name_,
                             getName(), cim_ins_info.ins_pc, cim_ins_info.sub_ins_num, payload.batch_info->batch_num));

        exec_socket_.finish();
    }
}

MacroGroupModule::MacroGroupModule(const sc_core::sc_module_name& name, const SimConfig& sim_config, Core* core,
                                   Clock* clk, const std::string& macro_group_name, int latency_cycle,
                                   int pipeline_stage_cnt, bool last_module)
    : BaseModule(name, sim_config, core, clk), last_module_(last_module) {
    int pipeline_stage_latency_cycle = latency_cycle / pipeline_stage_cnt;
    for (int i = 0; i < pipeline_stage_cnt; i++) {
        std::shared_ptr<MacroGroupPipelineStage> stage_ptr;
        if (last_module_ && i == pipeline_stage_cnt - 1) {
            stage_ptr = std::make_shared<MacroGroupPipelineLastStage>(fmt::format("pipeline_{}", i).c_str(), sim_config,
                                                                      core, clk, macro_group_name, getName(),
                                                                      pipeline_stage_latency_cycle);
        } else {
            stage_ptr = std::make_shared<MacroGroupPipelineNormalStage>(fmt::format("pipeline_{}", i).c_str(),
                                                                        sim_config, core, clk, macro_group_name,
                                                                        getName(), pipeline_stage_latency_cycle);
        }

        if (i > 0) {
            stage_list_[i - 1]->next_stage_socket_ = &(stage_ptr->exec_socket_);
        }

        stage_list_.emplace_back(stage_ptr);
    }
}

MacroGroupStageSocket* MacroGroupModule::getExecuteSocket() const {
    return &(stage_list_[0]->exec_socket_);
}

void MacroGroupModule::bindNextStageSocket(MacroGroupStageSocket* next_stage_socket, bool last_batch_trigger) {
    auto& last_stage_ptr = stage_list_[stage_list_.size() - 1];
    last_stage_ptr->next_stage_socket_ = next_stage_socket;
    last_stage_ptr->last_batch_trigger_next_ = last_batch_trigger;
}

void MacroGroupModule::setReleaseResourceFunc(std::function<void(int ins_pc)> release_resource_func) {
    if (last_module_) {
        auto stage_ptr = std::dynamic_pointer_cast<MacroGroupPipelineLastStage>(stage_list_[stage_list_.size() - 1]);
        stage_ptr->release_resource_func_ = std::move(release_resource_func);
    }
}

void MacroGroupModule::setFinishInsFunc(std::function<void()> finish_ins_func) {
    if (last_module_) {
        auto stage_ptr = std::dynamic_pointer_cast<MacroGroupPipelineLastStage>(stage_list_[stage_list_.size() - 1]);
        stage_ptr->finish_ins_func_ = std::move(finish_ins_func);
    }
}

}  // namespace cimsim
