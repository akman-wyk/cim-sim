//
// Created by wyk on 2024/8/13.
//

#include "network_simulator.h"

#include "systemc.h"
#include "util/util.h"

#if defined(WIN32)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFEXITED(status)   (((status) & 0x7f) == 0)
#elif defined(__linux__)
#include <sys/wait.h>
#endif

namespace pimsim {

Reporter test_network(const std::string& data_root_dir, const std::string& report_root_dir, const std::string& network,
                      const TestCaseConfig& test_case_config, const std::vector<LayerConfig>& layer_config,
                      bool& all_tests_passed, double OP_count) {
    auto data_dir = fmt::format("{}/{}/{}", data_root_dir, network, test_case_config.test_case_name);
    auto report_dir = fmt::format("{}/{}", report_root_dir, TEMP_REPORT_DIR_NAME);

    std::size_t execute_times = layer_config.size();
    Reporter total_reporter;
    for (std::size_t i = 0; i < execute_times; i++) {
        std::cout << fmt::format("    execute file{}: {}, ", i, layer_config[i].sub_dir_name);

        auto code_file = fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, CODE_FILE_NAME);
        auto global_image_file =
            fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, GLOBAL_IMAGE_FILE_NAME);
        auto report_json_file = fmt::format("{}/report{}.json", report_dir, i);
        auto expected_ins_stat_file =
            fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, EXPECTED_INS_STAT_FILE_NAME);
        auto expected_reg_file =
            fmt::format("{}/{}/{}", data_dir, layer_config[i].sub_dir_name, EXPECTED_REG_FILE_NAME);
        auto actual_reg_file = fmt::format("{}/{}", report_dir, ACTUAL_REG_FILE_NAME);

        // execute and generate json file
        auto cmd = fmt::format("./LayerSimulator {} {} {} {} {} {} -j {} >> ./log.txt 2>&1",
                               test_case_config.config_file_path, code_file, global_image_file, expected_ins_stat_file,
                               expected_reg_file, actual_reg_file, report_json_file);
        int status = system(cmd.c_str());
        if (status == -1) {
            all_tests_passed = false;
            std::cout << "Fork Error!" << std::endl;
        } else if (!WIFEXITED(status)) {
            all_tests_passed = false;
            std::cout << "Abnormal Exit!" << std::endl;
        } else {
            if (int result = WEXITSTATUS(status); result == TEST_PASSED) {
                std::cout << "Passed" << std::endl;
            } else {
                all_tests_passed = false;
                if (result == TEST_FAILED) {
                    std::cout << "Failed" << std::endl;
                } else if (result == INVALID_CONFIG) {
                    std::cout << "Invalid Config" << std::endl;
                } else if (result == INVALID_USAGE) {
                    std::cout << "Invalid Usage" << std::endl;
                } else if (result == CHECK_INS_STAT_FAILED) {
                    std::cout << "Check ins stat failed" << std::endl;
                } else if (result == CHECK_REG_FAILED) {
                    std::cout << "Check reg failed" << std::endl;
                }
            }
        }

        // read json file, get reporter and add reporter
        auto reporter = readTypeFromJsonFile<Reporter>(report_json_file);
        total_reporter += reporter;

        // delete json file
        remove(report_json_file.c_str());
    }

    total_reporter.setOPCount(OP_count);

    auto total_report_file_path = fmt::format("{}/{}/{}", report_root_dir, network, test_case_config.report_file_name);
    std::ofstream ofs(total_report_file_path);
    total_reporter.report(ofs);
    ofs.close();

    return std::move(total_reporter);
}

void test_wrap(const std::string& test_config_file, bool& all_tests_passed) {
    auto test_config = readTypeFromJsonFile<TestConfig>(test_config_file);
    std::map<std::string, Reporter> reporters;

    if (test_config.generate_report) {
        for (int i = 0; i < test_config.test_case_config.size(); i++) {
            if (const auto& test_case = test_config.test_case_config[i]; test_case.test) {
                std::cout << fmt::format("Testing case {}: {}", i, test_case.test_case_name) << std::endl;
                auto reporter =
                    test_network(test_config.data_root_dir, test_config.report_root_dir, test_config.network, test_case,
                                 test_config.layer_config, all_tests_passed, test_config.OP_count);
                reporters.emplace(test_case.test_case_name, std::move(reporter));
                std::cout << fmt::format("Finish test case {}\n", i) << std::endl;
            }
        }
    }

    auto network_report_file =
        fmt::format("{}/{}/{}_report.txt", test_config.report_root_dir, test_config.network, test_config.network);
    std::ofstream network_report_ofs(network_report_file);
    for (auto& [name, reporter] : reporters) {
        network_report_ofs << fmt::format("{}:\n", name);
        reporter.report(network_report_ofs, false);
        network_report_ofs << "\n";
    }

    if (test_config.compare) {
        std::cout << "Generating compare report" << std::endl;
        for (auto& [compare, test_case_1, test_case_2] : test_config.compare_config) {
            if (!compare) {
                continue;
            }

            if (reporters.find(test_case_1) == reporters.end() || reporters.find(test_case_2) == reporters.end()) {
                std::cout << fmt::format("    No input data for comparing {} with {}", test_case_1, test_case_2)
                          << std::endl;
                continue;
            }

            std::cout << fmt::format("    Comparing {} with {}", test_case_1, test_case_2) << std::endl;
            auto& r1 = reporters[test_case_1];
            auto& r2 = reporters[test_case_2];
            auto compare_r = r1.compare(r2);

            network_report_ofs << fmt::format("{} compare with {}:\n", test_case_1, test_case_2);
            compare_r.report(network_report_ofs, false);
            network_report_ofs << "\n";
        }
    }

    network_report_ofs.close();
}

}  // namespace pimsim

int sc_main(int argc, char** argv) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    if (argc != 2) {
        std::cerr << "Usage: ./NetworkSimulator <test_config_file>" << std::endl;
        return EXIT_FAILURE;
    }
    bool all_tests_passed = true;
    pimsim::test_wrap(argv[1], all_tests_passed);
    if (all_tests_passed) {
        std::cout << "All Tests Passed!" << std::endl;
    } else {
        std::cout << "Some Tests Failed!!!!" << std::endl;
    }
    return 0;
}
