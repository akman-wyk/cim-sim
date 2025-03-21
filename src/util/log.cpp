//
// Created by wyk on 2024/11/12.
//

#include "log.h"

#include "core/core.h"

namespace cimsim {

// #define ENABLE_LOG

void log(const std::string& msg) {
#ifdef ENABLE_LOG
    std::cout << sc_core::sc_time_stamp() << ", " << msg << std::endl;
#endif
}

void core_log(const std::string& msg, Core* core) {
#ifdef ENABLE_LOG
    std::cout << sc_core::sc_time_stamp() << ", core id: " << (core != nullptr ? core->getCoreId() : -1) << ", " << msg
              << std::endl;
#endif
}

#undef ENABLE_LOG

}  // namespace cimsim
