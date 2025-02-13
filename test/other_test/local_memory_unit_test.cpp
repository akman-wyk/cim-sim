//
// Created by wyk on 2024/7/5.
//
#include <iostream>

#include "config/config.h"
#include "core/local_memory_unit/local_memory_unit.h"
#include "systemc.h"

const std::string CONFIG_FILE = "../config/config_template.json";

namespace pimsim {

class TestModule : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(TestModule);

    TestModule(const char* name, const Config& config)
        : sc_core::sc_module(name)
        , local_memory_unit_("local_memory_unit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, nullptr) {
        SC_THREAD(process1)
        SC_THREAD(process2)
    }

    void process1() {
        wait(10, SC_NS);
        std::cout << sc_core::sc_time_stamp() << ", process1 start access memory" << std::endl;
        InstructionPayload ins{.pc = 1};
        local_memory_unit_.read_data(ins, 1024, 33, event1);
        std::cout << sc_core::sc_time_stamp() << ", process1 finish access memory" << std::endl;
    }

    void process2() {
        wait(15, SC_NS);
        std::cout << sc_core::sc_time_stamp() << ", process2 start access memory" << std::endl;
        InstructionPayload ins{.pc = 2};
        std::vector<uint8_t> data{};
        local_memory_unit_.read_data(ins, 2048, 33, event2);
        std::cout << sc_core::sc_time_stamp() << ", process2 finish access memory" << std::endl;
    }

private:
    LocalMemoryUnit local_memory_unit_;

    sc_core::sc_event event1;
    sc_core::sc_event event2;
};

}  // namespace pimsim

using namespace pimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    std::ifstream ifs;
    ifs.open(CONFIG_FILE);
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);
    ifs.close();

    auto config = j.get<Config>();
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return 1;
    }

    TestModule test_module{"test_local_memory_unit_module", config};
    sc_start(500, SC_NS);
    return 0;
}
