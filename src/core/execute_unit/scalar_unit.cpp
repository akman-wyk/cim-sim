//
// Created by wyk on 2024/7/19.
//

#include "scalar_unit.h"

#include "fmt/format.h"
#include "util/log.h"
#include "util/util.h"

namespace cimsim {

ScalarUnit::ScalarUnit(const sc_module_name &name, const ScalarUnitConfig &config, const BaseInfo &base_info,
                       Clock *clk)
    : ExecuteUnit(name, base_info, clk, ExecuteUnitType::scalar), config_(config) {
    SC_THREAD(process)
    SC_THREAD(executeInst)

    double scalar_functors_total_static_power_mW = config_.default_functor_static_power_mW;
    for (const auto &scalar_functor_config : config_.functor_list) {
        scalar_functors_total_static_power_mW += scalar_functor_config.static_power_mW;
        functor_config_map_.emplace(scalar_functor_config.inst_name, &scalar_functor_config);
    }
    energy_counter_.setStaticPowerMW(scalar_functors_total_static_power_mW);
}

void ScalarUnit::process() {
    ports_.ready_port_.write(true);
    while (true) {
        auto payload = waitForExecuteAndGetPayload<ScalarInsPayload>();

        ResourceAllocatePayload conflict_payload{.ins_id = payload->ins.ins_id, .unit_type = ExecuteUnitType::scalar};
        ports_.resource_allocate_.write(conflict_payload);

        CORE_LOG(fmt::format("scalar {} start, pc: {}", payload->op._to_string(), payload->ins.pc));

        // statistic energy
        auto functor_found = functor_config_map_.find(payload->op._to_string());
        double dynamic_power_mW = functor_found == functor_config_map_.end() ? config_.default_functor_dynamic_power_mW
                                                                             : functor_found->second->dynamic_power_mW;
        energy_counter_.addDynamicEnergyPJ(period_ns_, dynamic_power_mW,
                                           {.core_id = core_id_,
                                            .ins_id = payload->ins.ins_id,
                                            .inst_opcode = payload->ins.inst_opcode,
                                            .inst_group_tag = payload->ins.inst_group_tag,
                                            .inst_profiler_operator = InstProfilerOperator::control});

        // execute instruction
        execute_socket_.waitUntilFinishIfBusy();
        execute_socket_.payload = *payload;
        execute_socket_.start_exec.notify();

        readyForNextExecute();
    }
}

void ScalarUnit::bindRegUnit(cimsim::RegUnit *reg_unit) {
    reg_unit_ = reg_unit;
}

void ScalarUnit::executeInst() {
    while (true) {
        execute_socket_.waitUntilStart();

        const auto &payload = execute_socket_.payload;
        CORE_LOG(fmt::format("Scalar start execute, pc: {}", payload.ins.pc));

        if (payload.op == +ScalarOperator::store) {
            releaseResource(payload.ins.ins_id);

            int address_byte = payload.src1_value + payload.offset;
            int size_byte = WORD_BYTE_SIZE;
            auto write_data = IntToBytes(payload.src2_value, true);
            memory_socket_.writeLocal(payload.ins, address_byte, size_byte, std::move(write_data));
        } else {
            reg_unit_->writeRegister(executeAndWriteRegister(payload));

            releaseResource(payload.ins.ins_id);
        }

        // check if last pc
        if (payload.op == +ScalarOperator::store) {
            finishInstruction();
        } else {
            finishInstruction(period_ns_);
        }

        execute_socket_.finish();
    }
}

RegUnitWritePayload ScalarUnit::executeAndWriteRegister(const cimsim::ScalarInsPayload &payload) {
    RegUnitWritePayload write_payload{.id = payload.dst_reg, .special = false};
    switch (payload.op) {
        case ScalarOperator::add: {
            write_payload.value = payload.src1_value + payload.src2_value;
            break;
        }
        case ScalarOperator::sub: {
            write_payload.value = payload.src1_value - payload.src2_value;
            break;
        }
        case ScalarOperator::mul: {
            write_payload.value = payload.src1_value * payload.src2_value;
            break;
        }
        case ScalarOperator::div: {
            write_payload.value = payload.src1_value / payload.src2_value;
            break;
        }
        case ScalarOperator::sll: {
            write_payload.value = (payload.src1_value << payload.src2_value);
            break;
        }
        case ScalarOperator::srl: {
            unsigned int result =
                (static_cast<unsigned int>(payload.src1_value) >> static_cast<unsigned int>(payload.src2_value));
            write_payload.value = static_cast<int>(result);
            break;
        }
        case ScalarOperator::sra: {
            write_payload.value = (payload.src1_value >> payload.src2_value);
            break;
        }
        case ScalarOperator::mod: {
            write_payload.value = payload.src1_value % payload.src2_value;
            break;
        }
        case ScalarOperator::min: {
            write_payload.value = std::min(payload.src1_value, payload.src2_value);
            break;
        }
        case ScalarOperator::max: {
            write_payload.value = std::max(payload.src1_value, payload.src2_value);
            break;
        }
        case ScalarOperator::s_and: {
            write_payload.value = (payload.src1_value & payload.src2_value);
            break;
        }
        case ScalarOperator::s_or: {
            write_payload.value = (payload.src1_value | payload.src2_value);
            break;
        }
        case ScalarOperator::eq: {
            write_payload.value = (payload.src1_value == payload.src2_value) ? 1 : 0;
            break;
        }
        case ScalarOperator::ne: {
            write_payload.value = (payload.src1_value != payload.src2_value) ? 1 : 0;
            break;
        }
        case ScalarOperator::gt: {
            write_payload.value = (payload.src1_value > payload.src2_value) ? 1 : 0;
            break;
        }
        case ScalarOperator::lt: {
            write_payload.value = (payload.src1_value < payload.src2_value) ? 1 : 0;
            break;
        }
        case ScalarOperator::lui: {
            write_payload.value = (payload.src2_value << 16);
            break;
        }
        case ScalarOperator::load: {
            int address_byte = payload.src1_value + payload.offset;
            int size_byte = WORD_BYTE_SIZE;
            auto read_result = memory_socket_.readLocal(payload.ins, address_byte, size_byte);
            write_payload.value = BytesToInt(read_result, true);
            break;
        }
        case ScalarOperator::assign: {
            write_payload.value = payload.src1_value;
            write_payload.special = payload.write_special_register;
            if (payload.write_special_register) {
                int special_bound_general_id = reg_unit_->getSpecialBoundGeneralId(write_payload.id);
                if (special_bound_general_id != -1) {
                    write_payload.id = special_bound_general_id;
                    write_payload.special = false;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return write_payload;
}

}  // namespace cimsim
