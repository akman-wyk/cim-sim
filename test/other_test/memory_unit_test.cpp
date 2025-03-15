//
// Created by wyk on 2024/7/5.
//
#include <iostream>

#include "config/config.h"
#include "memory/memory_unit.h"
#include "systemc.h"
#include "util/util.h"

const std::string CONFIG_FILE = "../config/config_template.json";

namespace cimsim {

class TestModule : public sc_core::sc_module {
public:
    SC_HAS_PROCESS(TestModule);

    TestModule(const char* name, const Config& config)
        : sc_core::sc_module(name)
        , local_memory_unit_("local_memory_unit", config.chip_config.core_config.local_memory_unit_config,
                             config.sim_config, nullptr, nullptr, true) {
        SC_THREAD(process1)
        SC_THREAD(process2)
    }

    void process1() {
        wait(10, SC_NS);
        std::cout << sc_core::sc_time_stamp() << ", process1 start access memory" << std::endl;
        InstructionPayload ins{.pc = 1};
        auto payload = std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                                 .access_type = MemoryAccessType::read,
                                                                                 .address_byte = 1024,
                                                                                 .size_byte = 33,
                                                                                 .finish_access = event1});
        local_memory_unit_.access(payload);
        std::cout << sc_core::sc_time_stamp() << ", process1 finish access memory" << std::endl;
    }

    void process2() {
        wait(15, SC_NS);
        std::cout << sc_core::sc_time_stamp() << ", process2 start access memory" << std::endl;
        InstructionPayload ins{.pc = 2};
        std::vector<uint8_t> data{};
        auto payload = std::make_shared<MemoryAccessPayload>(MemoryAccessPayload{.ins = ins,
                                                                                 .access_type = MemoryAccessType::read,
                                                                                 .address_byte = 2048,
                                                                                 .size_byte = 33,
                                                                                 .finish_access = event2});
        local_memory_unit_.access(payload);
        std::cout << sc_core::sc_time_stamp() << ", process2 finish access memory" << std::endl;
    }

private:
    MemoryUnit local_memory_unit_;

    sc_core::sc_event event1;
    sc_core::sc_event event2;
};

}  // namespace cimsim

using namespace cimsim;

int sc_main(int argc, char* argv[]) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    auto config = readTypeFromJsonFile<Config>(CONFIG_FILE);
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return 1;
    }
    AddressSapce::initialize(config.chip_config);

    TestModule test_module{"test_local_memory_unit_module", config};
    sc_start(500, SC_NS);
    return 0;
}
