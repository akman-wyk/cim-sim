//
// Created by wyk on 2025/3/25.
//

#pragma once
#include <string>

namespace cimsim {

#define TEST_PASSED           0
#define TEST_FAILED           1
#define INVALID_USAGE         2
#define INVALID_CONFIG        3
#define CHECK_INS_STAT_FAILED 4
#define CHECK_REG_FAILED      5

const std::string TEMP_REPORT_DIR_NAME = "temp";
const std::string CODE_FILE_NAME = "final_code.json";
const std::string GLOBAL_IMAGE_FILE_NAME = "global_image";
const std::string EXPECTED_INS_STAT_FILE_NAME = "stats.json";
const std::string EXPECTED_REG_FILE_NAME = "regs.json";
const std::string ACTUAL_REG_FILE_NAME = "regs.txt";
const std::string GLOBAL_MEMORY_NAME = "global";

}  // namespace cimsim
