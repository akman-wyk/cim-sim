//
// Created by wyk on 2024/8/3.
//

#pragma once
#include <fstream>
#include <functional>
#include <vector>

#include "../base/test_macro.h"
#include "base_component/base_module.h"
#include "config/config.h"
#include "core/conflict/conflict_handler.h"
#include "core/execute_unit/execute_unit.h"
#include "fmt/format.h"
#include "memory/memory_unit.h"
#include "systemc.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

template <class TestUnitModule, class TestUnit, class TestUnitConfig, class InsPayload, class TestInstruction,
          class TestExpectedInfo, class TestInfo>
class ExecuteUnitTestModule : public BaseModule {
public:
    using TestInfoType = TestInfo;
    using TestBaseModule = ExecuteUnitTestModule<TestUnitModule, TestUnit, TestUnitConfig, InsPayload, TestInstruction,
                                                 TestExpectedInfo, TestInfo>;

    SC_HAS_PROCESS(TestUnitModule);

public:
    ExecuteUnitTestModule(const sc_core::sc_module_name& name, const char* test_unit_name,
                          const TestUnitConfig& test_unit_config, const Config& config, Clock* clk,
                          std::vector<TestInstruction> codes, ExecuteUnitType type)
        : BaseModule(name, BaseInfo{config.sim_config})
        , type_(type)
        , test_unit_config_(test_unit_config)
        , local_memory_unit_("LocalMemoryUnit", config.chip_config.core_config.local_memory_unit_config,
                             BaseInfo{config.sim_config}, false)
        , test_unit_(test_unit_name, test_unit_config, BaseInfo{config.sim_config}, clk)
        , unit_stall_handler_("stall_handler", decode_new_ins_trigger_, type)

        , signals_(type) {
        test_unit_.ports_.bind(signals_);
        test_unit_.ports_.id_finish_port_.bind(id_finish_);
        unit_stall_handler_.bind(signals_, unit_conflict_, &cur_ins_conflict_info_);

        SC_THREAD(issue)

        SC_METHOD(processStall)
        sensitive << unit_conflict_;

        SC_METHOD(processIdExEnable)
        sensitive << id_stall_;

        SC_METHOD(processResourceRelease)
        sensitive << signals_.resource_release_;

        SC_METHOD(processFinishRun)
        sensitive << signals_.unit_finish_;

        test_unit_.bindLocalMemoryUnit(&local_memory_unit_);
        ins_list_ = std::move(codes);
    }

    virtual EnergyReporter getEnergyReporter() = 0;

    Reporter getReporter() {
        EnergyCounter::setRunningTimeNS(running_time_);
        return Reporter{running_time_.to_seconds() * 1000, getName(), getEnergyReporter(), 0};
    }

    virtual bool checkTestResult(const TestExpectedInfo& expected) {
        return true;
    }

    virtual InsPayload decode(const InsPayload& payload) {
        return payload;
    }

private:
    [[noreturn]] void issue() {
        wait(8, SC_NS);

        while (true) {
            if (cur_ins_conflict_info_.unit_type == +ExecuteUnitType::none) {
                if (ins_index_ < ins_list_.size()) {
                    ins_list_[ins_index_].payload.ins.ins_id = ins_id++;
                    ins_list_[ins_index_].payload.ins.unit_type = type_;
                    cur_ins_conflict_info_ = test_unit_.getDataConflictInfo(decode(ins_list_[ins_index_].payload));
                    decode_new_ins_trigger_.notify();
                } else {
                    id_finish_.write(true);
                }
            }
            wait(0.1, SC_NS);

            if (!id_stall_.read() && cur_ins_conflict_info_.unit_type != +ExecuteUnitType::none) {
                signals_.id_ex_payload_.write(
                    ExecuteUnitPayload{.payload = std::make_shared<InsPayload>(decode(ins_list_[ins_index_].payload))});
                ins_index_++;

                cur_ins_conflict_info_ = ResourceAllocatePayload{.ins_id = -1, .unit_type = ExecuteUnitType::none};
            } else {
                signals_.id_ex_payload_.write(ExecuteUnitPayload{.payload = nullptr});
            }
            wait(period_ns_ - 0.1, SC_NS);
        }
    }

    void processStall() {
        bool stall = unit_conflict_.read();
        id_stall_.write(stall);
    }

    void processIdExEnable() {
        signals_.id_ex_enable_.write(!id_stall_.read());
    }

    void processResourceRelease() {
        for (int ins_id : signals_.resource_release_.read().ins_id_list_) {
            if (ins_id != -1) {
                CORE_LOG(fmt::format("{} ins finish, ins id: {}", type_._to_string(), ins_id));
            }
        }
    }

    void processFinishRun() {
        if (signals_.unit_finish_.read()) {
            running_time_ = sc_core::sc_time_stamp();
            sc_stop();
        }
    }

protected:
    // config
    const ExecuteUnitType type_;
    const TestUnitConfig& test_unit_config_;

    // instruction list
    std::vector<TestInstruction> ins_list_;
    int ins_index_{0};
    int ins_id{0};
    ResourceAllocatePayload cur_ins_conflict_info_;
    sc_core::sc_event decode_new_ins_trigger_;

    // modules
    MemoryUnit local_memory_unit_;
    TestUnit test_unit_;

    // stall
    ConflictHandler unit_stall_handler_;
    sc_core::sc_signal<bool> unit_conflict_{"unit_conflict"};
    sc_core::sc_signal<bool> id_stall_{"id_stall"};
    sc_core::sc_signal<bool> id_finish_{"id_finish"};

    // id ex signals
    ExecuteUnitSignalPorts signals_;

    sc_core::sc_time running_time_;
};

template <class TestUnitModule>
int cimsim_unit_test(
    int argc, char* argv[],
    std::function<TestUnitModule*(const Config& config, Clock* clk, typename TestUnitModule::TestInfoType& test_info)>
        test_module_initializer) {
    sc_core::sc_report_handler::set_actions(sc_core::SC_WARNING, sc_core::SC_DO_NOTHING);

    std::string exec_file_name{argv[0]};
    if (argc != 4) {
        std::cout << fmt::format("Usage: {} [config_file] [instruction_file] [report_file]", exec_file_name)
                  << std::endl;
        return INVALID_USAGE;
    }

    auto* config_file = argv[1];
    auto* instruction_file = argv[2];
    auto* report_file = argv[3];

    auto config = readTypeFromJsonFile<Config>(config_file);
    if (!config.checkValid()) {
        std::cout << "Config not valid" << std::endl;
        return INVALID_CONFIG;
    }
    AddressSapce::initialize(config.chip_config);

    auto test_info = readTypeFromJsonFile<typename TestUnitModule::TestInfoType>(instruction_file);
    Clock clk{"clock", config.sim_config.period_ns};
    auto* test_module = test_module_initializer(config, &clk, test_info);
    sc_start();

    std::ofstream ofs;
    ofs.open(report_file);
    auto reporter = test_module->getReporter();
    reporter.report(ofs);
    ofs.close();

    if (DoubleEqual(reporter.getLatencyNs(), test_info.expected.time_ns) &&
        DoubleEqual(reporter.getTotalEnergyPJ(), test_info.expected.energy_pj) &&
        test_module->checkTestResult(test_info.expected)) {
        std::cout << "Test Pass" << std::endl;
        return TEST_PASSED;
    } else {
        std::cout << "Test Failed" << std::endl;
        return TEST_FAILED;
    }
}

}  // namespace cimsim
