//
// Created by wyk on 2024/11/12.
//

#include "log.h"

#include "core/core.h"

namespace cimsim {

void log(const std::string& msg, Core* core) {
    // std::cout << sc_core::sc_time_stamp() << ", core id: " << (core != nullptr ? core->getCoreId() : -1) << ", " << msg
    //           << std::endl;
}

}  // namespace cimsim
