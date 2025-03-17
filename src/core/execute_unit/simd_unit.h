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

    const SIMDFunctorConfig* functor_config{nullptr};
    bool use_pipeline{false};
};

struct SIMDBatchInfo {
    int batch_vector_len{0};
    int batch_num{0};
    bool first_batch{false};
    bool last_batch{false};
};

struct SIMDSubmodulePayload {
    SIMDInstructionInfo ins_info;
    SIMDBatchInfo batch_info;
};

class SIMDUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(SIMDUnit);

    SIMDUnit(const char* name, const SIMDUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk);

    [[noreturn]] void processIssue();
    [[noreturn]] void processReadSubmodule();
    [[noreturn]] void processExecuteSubmodule();
    [[noreturn]] void processWriteSubmodule();

    ResourceAllocatePayload getDataConflictInfo(const SIMDInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    static void waitAndStartNextSubmodule(const SIMDSubmodulePayload& cur_payload,
                                          SubmoduleSocket<SIMDSubmodulePayload>& next_submodule_socket);

    std::pair<SIMDInstructionInfo, ResourceAllocatePayload> decodeAndGetInfo(const SIMDInstructionConfig* instruction,
                                                                             const SIMDFunctorConfig* functor,
                                                                             const SIMDInsPayload& payload) const;

private:
    const SIMDUnitConfig& config_;

    sc_core::sc_event cur_ins_next_batch_;
    SubmoduleSocket<SIMDSubmodulePayload> read_submodule_socket_{};
    SubmoduleSocket<SIMDSubmodulePayload> execute_submodule_socket_{};
    SubmoduleSocket<SIMDSubmodulePayload> write_submodule_socket_{};
};

}  // namespace cimsim
