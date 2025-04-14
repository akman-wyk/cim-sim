//
// Created by wyk on 2024/7/29.
//

#pragma once
#include <vector>

#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/cim_unit/cim_unit.h"
#include "execute_unit.h"
#include "payload.h"

namespace cimsim {

struct CimComputeReadDataPayload {
    InstructionPayload ins{};
    int addr_byte{0};
    int size_byte{0};
    std::vector<unsigned char> data{};
};

struct CimComputeSubInsPayload {
    CimInsInfo cim_ins_info{};
    CimComputeInsPayload ins_payload;
    int group_max_activation_macro_cnt{};
};

class CimComputeUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(CimComputeUnit);

    CimComputeUnit(const sc_module_name& name, const CimUnitConfig& config, const BaseInfo& base_info, Clock* clk);

    void bindCimUnit(CimUnit* cim_unit);

    ResourceAllocatePayload getDataConflictInfo(const CimComputeInsPayload& payload) const;
    ResourceAllocatePayload getDataConflictInfo(const std::shared_ptr<ExecuteInsPayload>& payload) override;

private:
    [[noreturn]] void processIssue();
    [[noreturn]] void processSubIns();
    void processSubInsReadData(const CimComputeSubInsPayload& sub_ins_payload);
    void processSubInsCompute(const CimComputeSubInsPayload& sub_ins_payload);

    [[noreturn]] void readValueSparseMaskSubmodule();
    [[noreturn]] void readBitSparseMetaSubmodule();

    std::vector<std::vector<unsigned long long>> getMacroGroupInputs(int group_id, int addr_byte, int size_byte,
                                                                     const CimComputeSubInsPayload& sub_ins_payload);

private:
    const CimUnitConfig& config_;
    const CimMacroSizeConfig& macro_size_;

    CimUnit* cim_unit_{nullptr};

    sc_event next_sub_ins_;
    SubmoduleSocket<CimComputeSubInsPayload> process_sub_ins_socket_;
    SubmoduleSocket<CimComputeReadDataPayload> read_value_sparse_mask_socket_;
    SubmoduleSocket<CimComputeReadDataPayload> read_bit_sparse_meta_socket_;

    EnergyCounter value_sparse_network_energy_counter_;
    EnergyCounter meta_buffer_energy_counter_;
};

}  // namespace cimsim
