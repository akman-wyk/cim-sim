//
// Created by wyk on 2024/11/12.
//

#include "log.h"

#include <iostream>

#include "systemc.h"

namespace cimsim {

// #define ENABLE_LOG

void log(const std::string& msg) {
#ifdef ENABLE_LOG
    std::cout << sc_time_stamp() << ", " << msg << std::endl;
#endif
}

void core_log(const std::string& msg, int core_id) {
#ifdef ENABLE_LOG
    std::cout << sc_time_stamp() << ", core id: " << core_id << ", " << msg << std::endl;
#endif
}

#undef ENABLE_LOG

}  // namespace cimsim
