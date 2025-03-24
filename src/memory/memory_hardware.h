//
// Created by wyk on 2024/11/10.
//

#pragma once
#include "base_component/base_module.h"
#include "payload.h"

namespace cimsim {

class MemoryHardware : public BaseModule {
public:
    MemoryHardware(const sc_module_name& name, const BaseInfo& base_info)
        : BaseModule(name, base_info) {}

    virtual sc_time accessAndGetDelay(MemoryAccessPayload& payload) = 0;

    EnergyReporter getEnergyReporter() override = 0;

    [[nodiscard]] virtual int getMemoryDataWidthByte(MemoryAccessType access_type) const = 0;
    [[nodiscard]] virtual int getMemorySizeByte() const = 0;

    [[nodiscard]] virtual const std::string& getMemoryName() {
        return getName();
    }

    void setMemoryID(int mem_id) {
        mem_id_ = mem_id;
    }

    [[nodiscard]] int getMemoryID() const {
        return mem_id_;
    }

private:
    int mem_id_{-1};
};

}  // namespace cimsim
