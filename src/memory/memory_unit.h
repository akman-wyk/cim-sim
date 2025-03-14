//
// Created by wyk on 2024/7/4.
//

#pragma once
#include <cstdint>
#include <vector>

#include "address_space/address_space.h"
#include "base_component/base_module.h"
#include "core/cim_unit/cim_unit.h"
#include "memory.h"

namespace cimsim {

class MemoryUnit : public BaseModule {
public:
    SC_HAS_PROCESS(MemoryUnit);

    MemoryUnit(const char* name, const MemoryUnitConfig& config, const SimConfig& sim_config, Core* core, Clock* clk,
               bool is_global);

    std::vector<uint8_t> read_data(const InstructionPayload& ins, int address_byte, int size_byte,
                                   sc_core::sc_event& finish_access);

    void write_data(const InstructionPayload& ins, int address_byte, int size_byte, std::vector<uint8_t> data,
                    sc_core::sc_event& finish_access);

    void access(const std::shared_ptr<MemoryAccessPayload>& payload);

    EnergyReporter getEnergyReporter() override;

    void bindCimUnit(CimUnit* cim_unit);

    int getMemoryDataWidthById(int memory_id, MemoryAccessType access_type) const;
    int getMemorySizeById(int memory_id) const;

private:
    std::shared_ptr<Memory> getMemoryByAddress(int address_byte);

private:
    const MemoryUnitConfig& config_;
    const SimConfig& sim_config_;
    const AddressSapce& as_;

    bool is_global_;
    std::vector<std::shared_ptr<Memory>> memory_list_;
};

}  // namespace cimsim
