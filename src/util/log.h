//
// Created by wyk on 2024/7/9.
//

#pragma once
#include <string>

namespace cimsim {

class Core;

void log(const std::string& msg, Core* core);

}  // namespace cimsim

#define LOG(msg) log(msg, core_)
