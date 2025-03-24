//
// Created by wyk on 2024/7/9.
//

#pragma once
#include <string>

namespace cimsim {

void log(const std::string& msg);
void core_log(const std::string& msg, int core_id);

}  // namespace cimsim

#define LOG(msg) log(msg)
#define CORE_LOG(msg) core_log(msg, core_id_)
