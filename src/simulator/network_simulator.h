//
// Created by wyk on 2024/8/13.
//

#pragma once
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "util/reporter.h"

#define TEST_PASSED           0
#define TEST_FAILED           1
#define INVALID_USAGE         2
#define INVALID_CONFIG        3
#define CHECK_INS_STAT_FAILED 4
#define CHECK_REG_FAILED      5

namespace cimsim {

const std::string TEMP_REPORT_DIR_NAME = "temp";
const std::string CODE_FILE_NAME = "final_code.json";
const std::string GLOBAL_IMAGE_FILE_NAME = "global_image";
const std::string EXPECTED_INS_STAT_FILE_NAME = "stats.json";
const std::string EXPECTED_REG_FILE_NAME = "regs.json";
const std::string ACTUAL_REG_FILE_NAME = "regs.txt";

struct LayerConfig {
    std::string sub_dir_name{};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LayerConfig, sub_dir_name);
};

struct TestCaseConfig {
    bool test = false;
    std::string test_case_name;
    std::string config_file_path;
    std::string report_file_name;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TestCaseConfig, test, test_case_name, config_file_path,
                                                report_file_name);
};

struct CompareConfig {
    bool compare = false;
    std::string test_case_1;
    std::string test_case_2;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(CompareConfig, compare, test_case_1, test_case_2);
};

struct TestConfig {
    std::string data_root_dir, report_root_dir;
    std::string network;
    double OP_count;

    bool generate_report = false;
    std::vector<TestCaseConfig> test_case_config;
    std::vector<LayerConfig> layer_config;

    bool compare = false;
    std::vector<CompareConfig> compare_config;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TestConfig, data_root_dir, report_root_dir, network, OP_count,
                                                generate_report, test_case_config, layer_config, compare,
                                                compare_config);
};

struct CompareResult {
    std::string compare_name;
    ReporterCompare compare_result;
};

}  // namespace cimsim
