//
// Created by wyk on 2024/7/15.
//

#pragma once

#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/socket/transmit_socket.h"
#include "execute_unit.h"
#include "payload.h"

namespace cimsim {

struct LocalTransferInsInfo {
    InstructionPayload ins{};

    int src_start_address_byte{0};
    int dst_start_address_byte{0};
    int batch_max_data_size_byte{0};

    bool use_pipeline{false};
};

struct LocalTransferBatchInfo {
    int batch_num{0};
    int batch_data_size_byte{0};
    bool last_batch{false};
    std::vector<unsigned char> data{};
};

struct LocalTransferDataPathPayload {
    std::shared_ptr<LocalTransferInsInfo> ins_info;
    int process_times{0};
    int data_size_byte{0};
};

struct LocalTransferStagePayload {
    std::shared_ptr<LocalTransferInsInfo> ins_info;
    std::shared_ptr<LocalTransferBatchInfo> batch_info;
};

struct GlobalTransferInsInfo {
    InstructionPayload ins{};
    TransferType type{TransferType::global_load};

    int src_start_address_byte{0};
    int dst_start_address_byte{0};
    int data_size_byte{0};

    int src_id{0};
    int dst_id{0};
    int transfer_id_tag{0};
};

struct GlobalTransferDataPathPayload {
    std::shared_ptr<GlobalTransferInsInfo> ins_info;
};

struct GlobalTransferStagePayload {
    std::shared_ptr<GlobalTransferInsInfo> ins_info;
};

using LocalTransferStageSocket = SubmoduleSocket<LocalTransferStagePayload>;
using GlobalTransferStageSocket = SubmoduleSocket<GlobalTransferStagePayload>;

class TransferUnit;

class LocalTransferDataPath : public BaseModule {
public:
    SC_HAS_PROCESS(LocalTransferDataPath);

    LocalTransferDataPath(const sc_module_name& name, const BaseInfo& base_info, TransferUnit& transfer_unit,
                          bool pipeline);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadStage();
    [[noreturn]] void processWriteStage();

    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);

public:
    SubmoduleSocket<LocalTransferDataPathPayload> exec_socket_;

private:
    TransferUnit& transfer_unit_;
    const bool pipeline_;

    sc_event cur_ins_next_batch_;
    LocalTransferStageSocket read_stage_socket_;
    LocalTransferStageSocket write_stage_socket_;

    MemorySocket memory_socket_;
};

class GlobalTransferDataPath : public BaseModule {
public:
    SC_HAS_PROCESS(GlobalTransferDataPath);

    GlobalTransferDataPath(const sc_module_name& name, const BaseInfo& base_info, TransferUnit& transfer_unit);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadStage();
    [[noreturn]] void processWriteStage();

    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit);
    void bindSwitchAndGlobalMemory(Switch* switch_, int global_memory_switch_id);

public:
    SubmoduleSocket<GlobalTransferDataPathPayload> exec_socket_;

private:
    TransferUnit& transfer_unit_;

    GlobalTransferStageSocket read_stage_socket_;
    GlobalTransferStageSocket write_stage_socket_;

    MemorySocket memory_socket_;
    TransmitSocket transmit_socket_;
};

class TransferUnit : public ExecuteUnit {
public:
    friend LocalTransferDataPath;
    friend GlobalTransferDataPath;

public:
    SC_HAS_PROCESS(TransferUnit);

    TransferUnit(const sc_module_name& name, const TransferUnitConfig& config, const BaseInfo& base_info, Clock* clk,
                 int global_memory_switch_id = -10);

    [[noreturn]] void processIssue();

    void bindSwitch(Switch* switch_);
    void bindLocalMemoryUnit(MemoryUnit* local_memory_unit) override;

    ResourceAllocatePayload getDataConflictInfo(const TransferInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    LocalTransferDataPathPayload decodeLocalTransferInfo(const TransferInsPayload& payload) const;
    static GlobalTransferDataPathPayload decodeGlobalTransferInfo(const TransferInsPayload& payload);

private:
    const TransferUnitConfig& config_;

    LocalTransferDataPath intra_core_bus_;
    GlobalTransferDataPath inter_core_bus_;
    std::unordered_map<unsigned int, std::shared_ptr<LocalTransferDataPath>> local_dedicated_data_path_map_{};

    const int global_memory_switch_id_;
};

}  // namespace cimsim
