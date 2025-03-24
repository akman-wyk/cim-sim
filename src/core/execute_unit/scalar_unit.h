//
// Created by wyk on 2024/7/19.
//

#pragma once
#include <unordered_map>

#include "base_component/submodule_socket.h"
#include "config/config.h"
#include "core/reg_unit/reg_unit.h"
#include "execute_unit.h"
#include "payload.h"

namespace cimsim {

class ScalarUnit : public ExecuteUnit {
public:
    SC_HAS_PROCESS(ScalarUnit);

    ScalarUnit(const sc_module_name& name, const ScalarUnitConfig& config, const BaseInfo& base_info, Clock* clk);

    void bindRegUnit(RegUnit* reg_unit);

private:
    [[noreturn]] void process();
    [[noreturn]] void executeInst();
    RegUnitWritePayload executeAndWriteRegister(const ScalarInsPayload& payload);

private:
    const ScalarUnitConfig& config_;
    std::unordered_map<std::string, const ScalarFunctorConfig*> functor_config_map_;

    SubmoduleSocket<ScalarInsPayload> execute_socket_;

    RegUnit* reg_unit_{nullptr};
};

}  // namespace cimsim
