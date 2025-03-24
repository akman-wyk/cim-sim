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

struct TransferInstructionInfo {
    InstructionPayload ins{};

    TransferType type{TransferType::local_trans};

    int src_start_address_byte{0};
    int dst_start_address_byte{0};
    int batch_max_data_size_byte{0};

    int src_id{0};
    int dst_id{0};
    int transfer_id_tag{0};

    bool use_pipeline{false};
};

struct TransferBatchInfo {
    int batch_num{0};
    int batch_data_size_byte{0};
    bool last_batch{false};
    std::vector<unsigned char> data{};
};

struct TransferSubmodulePayload {
    std::shared_ptr<TransferInstructionInfo> ins_info;
    std::shared_ptr<TransferBatchInfo> batch_info;
};

class TransferUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(TransferUnit);

    TransferUnit(const sc_module_name& name, const TransferUnitConfig& config, const BaseInfo& base_info, Clock* clk,
                 int global_memory_switch_id = -10);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadSubmodule();
    [[noreturn]] void processWriteSubmodule();

    void bindSwitch(Switch* switch_);

    ResourceAllocatePayload getDataConflictInfo(const TransferInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    std::pair<TransferInstructionInfo, ResourceAllocatePayload> decodeAndGetInfo(
        const TransferInsPayload& payload) const;

private:
    const TransferUnitConfig& config_;

    sc_event cur_ins_next_batch_;
    SubmoduleSocket<TransferSubmodulePayload> read_submodule_socket_{};
    SubmoduleSocket<TransferSubmodulePayload> write_submodule_socket_{};

    // send receive
    const int global_memory_switch_id_;
    TransmitSocket transmit_socket_;
};

}  // namespace cimsim
