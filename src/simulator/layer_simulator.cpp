//
// Created by wyk on 2024/8/13.
//

#include "layer_simulator.h"

#include "argparse/argparse.hpp"
#include "fmt/format.h"
#include "isa/instruction.h"
#include "util/util.h"

namespace pimsim {

const std::string GLOBAL_MEMORY_NAME = "global";

LayerSimulator::LayerSimulator(std::string config_file, std::string instruction_file, bool check)
    : config_file_(std::move(config_file))
    , instruction_file_(std::move(instruction_file))
    // , global_image_file_(std::move(global_image_file))
    // , expected_ins_stat_file_(std::move(expected_ins_stat_file))
    // , expected_reg_file_(std::move(expected_reg_file))
    // , actual_reg_file_(std::move(actual_reg_file))
    , check_(check) {}

void LayerSimulator::run() {
    std::cout << "Loading Instructions and Config" << std::endl;
    std::ifstream config_if(config_file_);
    std::ifstream instruction_if(instruction_file_);
    nlohmann::ordered_json config_json = nlohmann::ordered_json::parse(config_if);
    nlohmann::ordered_json instruction_json = nlohmann::ordered_json::parse(instruction_if);
    config_ = config_json.get<Config>();

    // for (auto& local_memory_config : config_.chip_config.core_config.local_memory_unit_config.local_memory_list) {
    //     if (local_memory_config.name == GLOBAL_MEMORY_NAME) {
    //         local_memory_config.ram_config.has_image = true;
    //         local_memory_config.ram_config.image_file = global_image_file_;
    //     }
    // }
    if (!config_.checkValid()) {
        std::cout << "Invalid config" << std::endl;
        return;
    }

    std::cout << "Load finish" << std::endl;

    std::cout << "Reading Instructions" << std::endl;
    auto core_ins_list = getCoreInstructionList(instruction_json);
    std::cout << "Read finish" << std::endl;

    std::cout << "Build Chip" << std::endl;
    chip_ = std::make_shared<Chip>("Chip", config_, core_ins_list);
    std::cout << "Build finish" << std::endl;

    std::cout << "Start Simulation" << std::endl;
    if (config_.sim_config.sim_mode == +SimMode::run_until_time) {
        sc_start(config_.sim_config.sim_time_ms, sc_core::SC_MS);
    } else {
        sc_start();
    }
    std::cout << "Simulation Finish" << std::endl;
}

void LayerSimulator::report(std::ostream& os, const std::string& report_json_file, bool report_every_core_energy) {
    os << "|*************** Simulation Report ***************|\n";
    os << "Basic Information:\n";

    std::string sub_line = "  - {:<20}{}\n";
    os << fmt::format(sub_line, "config file:", config_file_);
    os << fmt::format(sub_line, "instruction file:", instruction_file_);
    os << fmt::format(sub_line, "simulation mode:", config_.sim_config.sim_mode._to_string());
    if (config_.sim_config.sim_mode == +SimMode::run_until_time) {
        os << fmt::format("  - {:<20}{} ms\n", "simulation time:", config_.sim_config.sim_time_ms);
    }
    os << fmt::format(sub_line, "data mode:", config_.sim_config.data_mode._to_string());

    auto reporter = chip_->report(os, report_every_core_energy);

    if (!report_json_file.empty()) {
        nlohmann::json report_json = reporter;
        std::ofstream ofs;
        ofs.open(report_json_file);
        ofs << report_json;
        ofs.close();
    }
}

// bool LayerSimulator::checkInsStat() const {
//     return core_->checkInsStat(expected_ins_stat_file_);
// }
//
// bool LayerSimulator::checkReg() const {
//     return check_text_file_same(expected_reg_file_, actual_reg_file_);
// }

std::vector<std::vector<Instruction>> LayerSimulator::getCoreInstructionList(
    const nlohmann::ordered_json& instruction_json) const {
    int core_cnt = config_.chip_config.core_cnt;
    assert(instruction_json.size() == core_cnt);

    std::vector<std::vector<Instruction>> core_inst_list;
    for (int core_id = 0; core_id < core_cnt; core_id++) {
        auto core_key = fmt::format("{}", core_id);
        const auto& core_inst_json = instruction_json.at(core_key);

        std::vector<Instruction> core_inst;
        for (auto& ins_json : core_inst_json) {
            core_inst.push_back(ins_json.get<Instruction>());
        }
        core_inst_list.emplace_back(std::move(core_inst));
    }

    return std::move(core_inst_list);
}

}  // namespace pimsim

struct PimArguments {
    std::string config_file;
    std::string instruction_file;
    // std::string global_image_file;
    // std::string expected_ins_stat_file;
    // std::string expected_reg_file;
    // std::string actual_reg_file;
    bool check;

    bool report_result;
    std::string simulation_report_file;
    std::string report_json_file;

    bool list_every_core_energy;
};

PimArguments parsePimArguments(int argc, char* argv[]) {
    argparse::ArgumentParser parser("ChipTest");
    parser.add_argument("config").help("config file");
    parser.add_argument("inst").help("instruction file");
    // parser.add_argument("global").help("global image file");
    // parser.add_argument("stat").help("expected ins stat file");
    // parser.add_argument("reg").help("expected reg file");
    // parser.add_argument("actual_reg").help("actual reg file");
    parser.add_argument("-c", "--check")
        .help("whether to check reg and ins stat")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("-r", "--report")
        .help("whether to report simulation result")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("-s", "--sim_report").help("simulation report file").default_value("");
    parser.add_argument("-j", "--report_json").help("report json file").default_value("");
    parser.add_argument("-l", "--list_cores")
        .help("whether to list every core energy")
        .default_value(false)
        .implicit_value(true);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(EXIT_FAILURE);
    }

    std::string simulation_report_file = parser.is_used("--sim_report") ? parser.get("--sim_report") : "";
    std::string report_json_file = parser.is_used("--report_json") ? parser.get("--report_json") : "";
    return PimArguments{.config_file = parser.get("config"),
                        .instruction_file = parser.get("inst"),
                        // .global_image_file = parser.get("global"),
                        // .expected_ins_stat_file = parser.get("stat"),
                        // .expected_reg_file = parser.get("reg"),
                        // .actual_reg_file = parser.get("actual_reg"),
                        .check = parser.get<bool>("--check"),
                        .report_result = parser.get<bool>("--report"),
                        .simulation_report_file = simulation_report_file,
                        .report_json_file = report_json_file,
                        .list_every_core_energy = parser.get<bool>("--list_cores")};
}

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    auto args = parsePimArguments(argc, argv);

    pimsim::LayerSimulator layer_simulator{args.config_file, args.instruction_file,
                                           // args.global_image_file,
                                           // args.expected_ins_stat_file,
                                           // args.expected_reg_file,
                                           // args.actual_reg_file,
                                           args.check};
    layer_simulator.run();

    if (!args.simulation_report_file.empty()) {
        std::ofstream os;
        os.open(args.simulation_report_file);
        layer_simulator.report(os, args.report_json_file, args.list_every_core_energy);
        os.close();
    } else if (args.report_result) {
        layer_simulator.report(std::cout, args.report_json_file, args.list_every_core_energy);
    } else {
        std::stringstream ss;
        layer_simulator.report(ss, args.report_json_file, args.list_every_core_energy);
    }

    // if (!layer_simulator.checkInsStat()) {
    //     std::cerr << "check ins stat failed" << std::endl;
    //     return CHECK_INS_STAT_FAILED;
    // }
    //
    // if (args.check) {
    //     if (!layer_simulator.checkReg()) {
    //         std::cerr << "check reg failed" << std::endl;
    //         return CHECK_REG_FAILED;
    //     }
    // }

    return TEST_PASSED;
}
