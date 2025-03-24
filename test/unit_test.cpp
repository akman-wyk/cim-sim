//
// Created by wyk on 2024/7/18.
//

#include <iostream>
#include <string>
#include <vector>

#include "argparse/argparse.hpp"
#include "base/test_macro.h"
#include "fmt/format.h"
#include "util/util.h"

#if defined(WIN32)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#elif defined(__linux__)
#include <sys/wait.h>
#endif

namespace cimsim {

struct TestArguments {
    std::string config_file;
    std::string test_unit_name;
    int test_case_num;
    bool print_cmd;
};

struct UnitTestCaseConfig {
    std::string comments{};
    std::string config_file;
    std::string instruction_file;
    std::string report_file;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UnitTestCaseConfig, comments, config_file, instruction_file,
                                                report_file)
};

struct UnitTestConfig {
    std::string name;
    std::vector<UnitTestCaseConfig> test_cases;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UnitTestConfig, name, test_cases)
};

struct TestConfig {
    std::string root_dir;
    std::vector<UnitTestConfig> unit_test_list;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TestConfig, root_dir, unit_test_list)
};

TestArguments parseTestArguments(int argc, char* argv[]) {
    argparse::ArgumentParser parser("Test");
    parser.add_argument("config").help("config file");
    parser.add_argument("-u", "--unit").help("test unit name").default_value(std::string(""));
    parser.add_argument("-c", "--case").help("test case num, from 1 to n").scan<'i', int>().default_value(0);
    parser.add_argument("-p", "--print").help("print command").default_value(false).implicit_value(true);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(EXIT_FAILURE);
    }

    return TestArguments{.config_file = parser.get("config"),
                         .test_unit_name = parser.get("--unit"),
                         .test_case_num = parser.get<int>("--case"),
                         .print_cmd = parser.get<bool>("--print")};
}

bool test_unit(const std::string& root_dir, const UnitTestConfig& unit_test_config, const TestArguments& args) {
    std::cout << fmt::format("\tStart {}", unit_test_config.name) << std::endl;

    bool all_test_cases_passed = true;
    for (int i = 0; i < unit_test_config.test_cases.size(); i++) {
        if (args.test_case_num != 0 && i + 1 != args.test_case_num) {
            continue;
        }
        const auto& test_case_config = unit_test_config.test_cases[i];
        std::cout << fmt::format("\t\tStart test case {}: {}\n\t\t\t", i + 1, test_case_config.comments);

        auto config_file = fmt::format("{}/{}", root_dir, test_case_config.config_file);
        auto instruction_file = fmt::format("{}/{}", root_dir, test_case_config.instruction_file);
        auto report_file = fmt::format("{}/{}", root_dir, test_case_config.report_file);

        auto cmd = fmt::format("./{} {} {} {} >> ./log.txt 2>&1", unit_test_config.name, config_file, instruction_file,
                               report_file);
        if (args.print_cmd) {
            std::cout << fmt::format("command: {}\n\t\t\t", cmd);
        }

        int status = system(cmd.c_str());
        if (status == -1) {
            all_test_cases_passed = false;
            std::cout << "Fork Error!" << std::endl;
        } else if (!WIFEXITED(status)) {
            all_test_cases_passed = false;
            std::cout << "Abnormal Exit!" << std::endl;
        } else {
            if (int result = WEXITSTATUS(status); result == TEST_PASSED) {
                std::cout << "Passed" << std::endl;
            } else {
                all_test_cases_passed = false;
                if (result == TEST_FAILED) {
                    std::cout << "Failed" << std::endl;
                } else if (result == INVALID_CONFIG) {
                    std::cout << "Invalid Config" << std::endl;
                } else if (result == INVALID_USAGE) {
                    std::cout << "Invalid Usage" << std::endl;
                }
            }
        }
    }

    return all_test_cases_passed;
}

bool test(const TestArguments& args) {
    auto test_config = readTypeFromJsonFile<TestConfig>(args.config_file);
    // Unit Tests
    std::cout << "Start Unit Tests" << std::endl;
    bool all_test_units_passed = true;
    for (const auto& unit_test_config : test_config.unit_test_list) {
        if (!args.test_unit_name.empty() && unit_test_config.name != args.test_unit_name) {
            continue;
        }
        if (!test_unit(test_config.root_dir, unit_test_config, args)) {
            all_test_units_passed = false;
        }
    }
    std::cout << "End Unit Tests" << std::endl;
    return all_test_units_passed;
}

}  // namespace cimsim

int main(int argc, char* argv[]) {
    if (auto args = cimsim::parseTestArguments(argc, argv); cimsim::test(args)) {
        std::cout << "All Tests Passed!" << std::endl;
    } else {
        std::cout << "Some Tests Failed!!!!" << std::endl;
    }
    return 0;
}
