//
// Created by wyk on 2024/6/14.
//

#include "config.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

#include "fmt/format.h"
#include "util/util.h"

namespace cimsim {

template <class T>
bool check_positive(const T& t) {
    return t > 0;
}

template <class T>
bool check_positive(const std::vector<T>& list) {
    return std::all_of(list.begin(), list.end(), [](const T& t) { return check_positive(t); });
}

template <class T, class... Args>
bool check_positive(const T& t, const Args&... args) {
    return check_positive(t) && check_positive(args...);
}

template <class T>
[[maybe_unused]] bool check_not_negative(const T& t) {
    return t >= 0;
}

template <class T, class... Args>
bool check_not_negative(const T& t, const Args&... args) {
    return check_not_negative(t) && check_not_negative(args...);
}

template <class T, class... Args>
bool check_vector_valid(const std::vector<T>& configs, [[maybe_unused]] const Args&... args) {
    return std::all_of(configs.begin(), configs.end(), [&](const T& config) { return config.checkValid(args...); });
}

// ControlUnit
bool ControlUnitConfig::checkValid() const {
    if (!check_not_negative(controller_static_power_mW, controller_dynamic_power_mW, fetch_static_power_mW,
                            fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW)) {
        std::cerr << "ControlUnitConfig not valid, 'controller_static_power_mW, controller_dynamic_power_mW, "
                     "fetch_static_power_mW, fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW' "
                     "must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ControlUnitConfig, controller_static_power_mW,
                                               controller_dynamic_power_mW, fetch_static_power_mW,
                                               fetch_dynamic_power_mW, decode_static_power_mW, decode_dynamic_power_mW)

// RegisterUnit
bool SpecialRegisterBindingConfig::checkValid() const {
    if (!(0 <= special && special < SPECIAL_REG_NUM)) {
        std::cerr
            << fmt::format(
                   "SpecialRegisterBindingConfig not valid, 'special' must be in [0, {}), while actually it is {}",
                   SPECIAL_REG_NUM, special)
            << std::endl;
        return false;
    }
    if (!(0 <= general && general < GENERAL_REG_NUM)) {
        std::cerr
            << fmt::format(
                   "SpecialRegisterBindingConfig not valid, 'general' must be in [0, {}), while actually it is {}",
                   GENERAL_REG_NUM, general)
            << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SpecialRegisterBindingConfig, special, general)

bool RegisterUnitConfig::checkValid() const {
    if (!check_not_negative(static_power_mW, dynamic_power_mW)) {
        std::cerr << "RegisterUnitConfig not valid, 'static_power_mW, dynamic_power_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    if (!check_vector_valid(special_register_binding)) {
        std::cerr << "RegisterUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(RegisterUnitConfig, static_power_mW, dynamic_power_mW,
                                               special_register_binding)

// ScalarUnit
bool ScalarFunctorConfig::checkValid() const {
    if (inst_name.empty()) {
        std::cerr << "ScalarFunctorConfig not valid, 'inst_name' must be non-empty" << std::endl;
        return false;
    }
    if (!check_not_negative(static_power_mW, dynamic_power_mW)) {
        std::cerr
            << fmt::format(
                   "ScalarFunctorConfig of '{}' not valid, 'static_power_mW, dynamic_power_mW' must be non-negative",
                   inst_name)
            << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ScalarFunctorConfig, inst_name, static_power_mW, dynamic_power_mW)

bool ScalarUnitConfig::checkValid() const {
    if (!check_not_negative(default_functor_static_power_mW, default_functor_dynamic_power_mW)) {
        std::cerr << "ScalarUnitConfig not valid, 'default_functor_static_power_mW, default_functor_dynamic_power_mW' "
                     "must be non-negative"
                  << std::endl;
        return false;
    }
    if (!check_vector_valid(functor_list)) {
        std::cerr << "ScalarUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ScalarUnitConfig, default_functor_static_power_mW,
                                               default_functor_dynamic_power_mW, functor_list)

// SIMDUnit
bool checkDataWidth(const int width) {
    return width == 1 || width == 2 || width == 4 || width == 8 || width == 16 || width == 32 || width == 64;
}

bool SIMDDataWidthConfig::inputBitWidthMatch(const cimsim::SIMDDataWidthConfig& other) const {
    return inputs == other.inputs;
}

bool SIMDDataWidthConfig::checkValid(const unsigned int input_cnt, const bool check_output) const {
    bool valid = std::transform_reduce(
        inputs.begin(), inputs.begin() + input_cnt, true, [](bool v1, bool v2) { return v1 && v2; },
        [](int input_bit_width) { return checkDataWidth(input_bit_width); });
    if (check_output) {
        valid = valid && checkDataWidth(output);
    }
    if (!valid) {
        std::cerr << "SIMDDataWidthConfig not valid, data width must be one of the following values: "
                     "ont-bit(1b), two-bit(2b), half-byte(4b), byte(8b), half-word(16b), word(32b), double-word(64b)"
                  << std::endl;
        return false;
    }
    return true;
}

void to_json(nlohmann::ordered_json& j, const SIMDDataWidthConfig& t) {
    for (int i = 0; i < SIMD_MAX_INPUT_NUM; i++) {
        if (t.inputs[i] != 0) {
            j[fmt::format("input{}", i + 1)] = t.inputs[i];
        }
    }
    if (t.output != 0) {
        j["output"] = t.output;
    }
}

void from_json(const nlohmann::ordered_json& nlohmann_json_j, SIMDDataWidthConfig& nlohmann_json_t) {
    for (int i = 0; i < SIMD_MAX_INPUT_NUM; i++) {
        nlohmann_json_t.inputs[i] = nlohmann_json_j.value(fmt::format("input{}", i + 1), 0);
    }
    nlohmann_json_t.output = nlohmann_json_j.value("output", 0);
}

bool SIMDFunctorConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "SIMDFunctorConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (input_cnt > SIMD_MAX_INPUT_NUM) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'input_cnt' must be not greater than {}", name,
                                 SIMD_MAX_INPUT_NUM)
                  << std::endl;
        return false;
    }
    if (!check_positive(functor_cnt, pipeline_stage_cnt)) {
        std::cerr << fmt::format(
                         "SIMDFunctorConfig of '{}' not valid, 'functor_cnt, pipeline_stage_cnt' must be positive",
                         name)
                  << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_per_functor_mW, dynamic_power_per_functor_mW)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'latency_cycle, static_power_per_functor_mW, "
                                 "dynamic_power_per_functor_mW' must be non-negative",
                                 name)
                  << std::endl;
        return false;
    }
    if (latency_cycle % pipeline_stage_cnt != 0) {
        std::cerr
            << fmt::format(
                   "SIMDFunctorConfig of '{}' not valid, 'latency_cycle' must be divisible by 'pipeline_stage_cnt'",
                   name)
            << std::endl;
        return false;
    }
    if (!data_bit_width.checkValid(input_cnt, true)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid", name) << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDFunctorConfig, name, input_cnt, data_bit_width, functor_cnt,
                                               latency_cycle, pipeline_stage_cnt, static_power_per_functor_mW,
                                               dynamic_power_per_functor_mW)

bool SIMDInstructionFunctorBindingConfig::checkValid(const unsigned int input_cnt) const {
    if (functor_name.empty()) {
        std::cerr << "SIMDInstructionFunctorBindingConfig not valid, 'functor_name' must be non-empty" << std::endl;
        return false;
    }
    if (!input_bit_width.checkValid(input_cnt, false)) {
        std::cerr << fmt::format("SIMDInstructionFunctorBindingConfig with functor '{}' not valid", functor_name)
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDInstructionFunctorBindingConfig, input_bit_width, functor_name)

bool SIMDInstructionConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "SIMDInstructionConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (input_cnt > SIMD_MAX_INPUT_NUM) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'input_cnt' must be not greater than {}",
                                 name, SIMD_MAX_INPUT_NUM)
                  << std::endl;
        return false;
    }
    if (opcode > SIMD_MAX_OPCODE) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'opcode' must be not greater than {}", name,
                                 SIMD_MAX_OPCODE)
                  << std::endl;
        return false;
    }

    if (bool invalid_input_type = std::transform_reduce(
            inputs_type.begin(), inputs_type.begin() + input_cnt, false, [](bool v1, bool v2) { return v1 || v2; },
            [](SIMDInputType input_type) { return input_type == +SIMDInputType::other; });
        invalid_input_type) {
        std::cerr << fmt::format("SIMDInstructionConfig of '{}' not valid, 'input_type' must be 'vector' or 'scalar'",
                                 name)
                  << std::endl;
        return false;
    }

    if (!check_vector_valid(functor_binding_list, input_cnt)) {
        std::cerr << "SIMDInstructionConfig not valid" << std::endl;
        return false;
    }
    return true;
}

void to_json(nlohmann::ordered_json& j, const SIMDInstructionConfig& t) {
    j["name"] = t.name;
    j["input_cnt"] = t.input_cnt;

    std::stringstream ss;
    ss << std::hex << t.opcode;
    j["opcode"] = "0x" + ss.str();

    for (unsigned int i = 0; i < t.input_cnt; i++) {
        j[fmt::format("input{}_type", i + 1)] = t.inputs_type[i];
    }
    j["functor_binding_list"] = t.functor_binding_list;
}

void from_json(const nlohmann::ordered_json& j, SIMDInstructionConfig& t) {
    const SIMDInstructionConfig default_obj{};
    t.name = j.value("name", default_obj.name);
    t.input_cnt = j.value("input_cnt", default_obj.input_cnt);

    std::string opcode_str = j.value("opcode", "0x0");
    opcode_str = opcode_str.substr(2);
    t.opcode = std::stoul(opcode_str, nullptr, 16);

    for (unsigned int i = 0; i < SIMD_MAX_INPUT_NUM; i++) {
        JSON_VALUE_ENUM(t, inputs_type[i], fmt::format("input{}_type", i + 1))
    }
    t.functor_binding_list = j.value("functor_binding_list", default_obj.functor_binding_list);
}

bool SIMDUnitConfig::checkValid() const {
    if (const bool valid = check_vector_valid(functor_list) && check_vector_valid(instruction_list); !valid) {
        std::cerr << "SIMDUnitConfig not valid" << std::endl;
        return false;
    }

    // check instruction functor binding
    std::unordered_map<std::string, const SIMDFunctorConfig*> functor_map;
    for (auto& functor_cfg : functor_list) {
        if (functor_map.count(functor_cfg.name) == 1) {
            std::cerr << fmt::format("SIMDUnitConfig not valid, duplicate functor '{}'", functor_cfg.name) << std::endl;
            return false;
        }
        functor_map.emplace(functor_cfg.name, &functor_cfg);
    }

    std::unordered_set<unsigned int> instruction_opcode_set;
    for (const auto& instruction : instruction_list) {
        if (instruction_opcode_set.count(instruction.opcode) == 1) {
            std::cerr << fmt::format("SIMDUnitConfig not valid, duplicate instruction opcode '{}'", instruction.opcode)
                      << std::endl;
            return false;
        }
        instruction_opcode_set.emplace(instruction.opcode);

        for (const auto& functor_binding : instruction.functor_binding_list) {
            auto functor_found = functor_map.find(functor_binding.functor_name);
            // check functor exist
            if (functor_found == functor_map.end()) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("\tFunctor '{}' not exist", functor_binding.functor_name) << std::endl;
                return false;
            }

            const auto& functor = functor_found->second;

            // check input cnt match
            if (instruction.input_cnt != functor->input_cnt) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("\tInput count not match between instruction '{}' and functor '{}'",
                                         instruction.name, functor->name)
                          << std::endl;
                return false;
            }

            // check input bit width match
            if (!functor_binding.input_bit_width.inputBitWidthMatch(functor->data_bit_width)) {
                std::cerr << "SIMDUnitConfig not valid, instruction functor binding error" << std::endl;
                std::cerr << fmt::format("\tInput bit-width not match between instruction '{}' and functor '{}'",
                                         instruction.name, functor->name)
                          << std::endl;
                return false;
            }
        }
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SIMDUnitConfig, pipeline, functor_list, instruction_list)

bool ReduceFunctorConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "ReduceFunctorConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (funct > REDUCE_MAX_FUNCT) {
        std::cerr << fmt::format("ReduceFunctorConfig of '{}' not valid, 'funct' must be not greater than {}", name,
                                 REDUCE_MAX_FUNCT)
                  << std::endl;
        return false;
    }
    if (const bool valid = checkDataWidth(input_bit_width) && checkDataWidth(output_bit_width); !valid) {
        std::cerr << "ReduceFunctorConfig not valid, input and output width must be one of the following values: "
                     "ont-bit(1b), two-bit(2b), half-byte(4b), byte(8b), half-word(16b), word(32b), double-word(64b)"
                  << std::endl;
        return false;
    }
    if (!check_positive(reduce_input_cnt, pipeline_stage_cnt)) {
        std::cerr << fmt::format(
                         "SIMDFunctorConfig of '{}' not valid, 'reduce_input_cnt, pipeline_stage_cnt' must be positive",
                         name)
                  << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr << fmt::format("SIMDFunctorConfig of '{}' not valid, 'latency_cycle, static_power_mW, "
                                 "dynamic_power_mW' must be non-negative",
                                 name)
                  << std::endl;
        return false;
    }
    if (latency_cycle % pipeline_stage_cnt != 0) {
        std::cerr
            << fmt::format(
                   "SIMDFunctorConfig of '{}' not valid, 'latency_cycle' must be divisible by 'pipeline_stage_cnt'",
                   name)
            << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ReduceFunctorConfig, name, funct, input_bit_width, output_bit_width,
                                               reduce_input_cnt, latency_cycle, pipeline_stage_cnt, static_power_mW,
                                               dynamic_power_mW)

bool ReduceUnitConfig::checkValid() const {
    if (!check_vector_valid(functor_list)) {
        std::cerr << "ReduceUnitConfig not valid" << std::endl;
        return false;
    }

    std::unordered_set<unsigned int> functor_funct_set;
    for (auto& functor_cfg : functor_list) {
        if (functor_funct_set.count(functor_cfg.funct) == 1) {
            std::cerr << fmt::format("ReduceUnitConfig not valid, duplicate functor funct code '{}'", functor_cfg.funct)
                      << std::endl;
            return false;
        }
        functor_funct_set.emplace(functor_cfg.funct);
    }

    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ReduceUnitConfig, pipeline, functor_list)

// CimUnit
bool CimMacroSizeConfig::checkValid() const {
    if (!check_positive(compartment_cnt_per_macro, element_cnt_per_compartment, row_cnt_per_element,
                        bit_width_per_row)) {
        std::cerr << "CimMacroSizeConfig not valid, 'compartment_cnt_per_macro, element_cnt_per_compartment, "
                     "row_cnt_per_element, bit_width_per_row' must be positive"
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimMacroSizeConfig, compartment_cnt_per_macro,
                                               element_cnt_per_compartment, row_cnt_per_element, bit_width_per_row)

bool CimModuleConfig::checkValid(const std::string& module_name) const {
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr << fmt::format("CimModuleConfig of '{}' not valid, 'latency_cycle, static_power_mW, "
                                 "dynamic_power_mW' must be non-negative",
                                 module_name)
                  << std::endl;
        return false;
    }
    if (!check_positive(pipeline_stage_cnt)) {
        std::cerr << fmt::format("CimModuleConfig of '{}' not valid, 'pipeline_stage_cnt' must be positive",
                                 module_name)
                  << std::endl;
        return false;
    }
    if (latency_cycle % pipeline_stage_cnt != 0) {
        std::cerr << fmt::format(
                         "CimModuleConfig of '{}' not valid, 'latency_cycle' must be divisible by 'pipeline_stage_cnt'",
                         module_name)
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimModuleConfig, latency_cycle, pipeline_stage_cnt, static_power_mW,
                                               dynamic_power_mW)

bool CimSRAMConfig::checkValid() const {
    if (as_mode == +CimASMode::other) {
        std::cerr << "CimSRAMConfig not valid, 'as_mode' must be 'intergroup' or 'intragroup'" << std::endl;
        return false;
    }
    if (!check_not_negative(write_latency_cycle, read_latency_cycle, static_power_mW, write_dynamic_power_per_bit_mW,
                            read_dynamic_power_per_bit_mW)) {
        std::cerr << "CimSRAMConfig not valid, 'write_latency_cycle, read_latency_cycle, static_power_mW, "
                     "write_dynamic_power_per_bit_mW, read_dynamic_power_per_bit_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimSRAMConfig, as_mode, write_latency_cycle, read_latency_cycle,
                                               static_power_mW, write_dynamic_power_per_bit_mW,
                                               read_dynamic_power_per_bit_mW)

bool CimValueSparseConfig::checkValid() const {
    if (!check_positive(mask_bit_width, output_macro_group_cnt)) {
        std::cerr << "CimValueSparseConfig not valid, 'mask_bit_width, output_macro_group_cnt' must be positive"
                  << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW)) {
        std::cerr
            << "CimValueSparseConfig not valid, 'latency_cycle, static_power_mW, dynamic_power_mW' must be non-negative"
            << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimValueSparseConfig, mask_bit_width, latency_cycle, static_power_mW,
                                               dynamic_power_mW, output_macro_group_cnt)

bool CimBitSparseConfig::checkValid() const {
    if (!check_positive(mask_bit_width, unit_byte)) {
        std::cerr << "CimBitSparseConfig not valid, 'mask_bit_width, unit_byte' must be positive" << std::endl;
        return false;
    }
    if (!check_not_negative(latency_cycle, static_power_mW, dynamic_power_mW, reg_buffer_static_power_mW,
                            reg_buffer_dynamic_power_mW_per_unit)) {
        std::cerr << "CimBitSparseConfig not valid, 'latency_cycle, static_power_mW, dynamic_power_mW, "
                     "reg_buffer_static_power_mW, reg_buffer_dynamic_power_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CimBitSparseConfig, mask_bit_width, latency_cycle, static_power_mW,
                                               dynamic_power_mW, unit_byte, reg_buffer_static_power_mW,
                                               reg_buffer_dynamic_power_mW_per_unit)

int CimUnitConfig::getByteSize() const {
    return IntDivCeil(macro_size.bit_width_per_row * macro_size.row_cnt_per_element *
                          macro_size.element_cnt_per_compartment * macro_size.compartment_cnt_per_macro *
                          macro_total_cnt,
                      BYTE_TO_BIT);
}

int CimUnitConfig::getBitWidth() const {
    return macro_size.bit_width_per_row * macro_size.element_cnt_per_compartment *
           (sram.as_mode == +CimASMode::intergroup ? macro_total_cnt : macro_group_size);
}

int CimUnitConfig::getByteWidth() const {
    return IntDivCeil(getBitWidth(), BYTE_TO_BIT);
}

std::string CimUnitConfig::getMemoryName() const {
    return name_as_memory;
}

bool CimUnitConfig::checkValid() const {
    if (!check_positive(macro_total_cnt, macro_group_size)) {
        std::cerr << "CimUnitConfig not valid, 'macro_total_cnt, macro_group_size_configurable_values' must be positive"
                  << std::endl;
        return false;
    }

    if (macro_group_size == 0 || macro_total_cnt % macro_group_size != 0) {
        std::cerr << fmt::format("CimUnitConfig not valid, macro group size '{}' cannot divide macro total count '{}'",
                                 macro_group_size, macro_total_cnt)
                  << std::endl;
        return false;
    }

    if (name_as_memory.empty()) {
        std::cerr << "CimUnitConfig not valid, 'name_as_memory' must be non-empty" << std::endl;
        return false;
    }
    if (name_as_memory.find(DUPLICATE_MEMORY_NAME_DELIMITER) != std::string::npos) {
        std::cerr << fmt::format("CimUnitConfig of {} not valid, 'name_as_memory' has invalid char {} which is "
                                 "duplicate memory name delimiter",
                                 name_as_memory, DUPLICATE_MEMORY_NAME_DELIMITER)
                  << std::endl;
        return false;
    }

    if (const bool valid = macro_size.checkValid() && ipu.checkValid("ipu") && sram.checkValid() &&
                           adder_tree.checkValid("adder_tree") && shift_adder.checkValid("shift_adder") &&
                           result_adder.checkValid("result_adder") &&
                           (!value_sparse || value_sparse_config.checkValid()) &&
                           (!bit_sparse || bit_sparse_config.checkValid());
        !valid) {
        std::cerr << "CimUnitConfig not valid" << std::endl;
        return false;
    }

    return true;
}

void to_json(nlohmann::ordered_json& j, const CimUnitConfig& t) {
    j["macro_total_cnt"] = t.macro_total_cnt;
    j["macro_group_size"] = t.macro_group_size;
    j["macro_size"] = t.macro_size;
    j["ipu"] = t.ipu;
    j["sram"] = t.sram;
    j["adder_tree"] = t.adder_tree;
    j["shift_adder"] = t.shift_adder;
    j["result_adder"] = t.result_adder;
    j["value_sparse"] = t.value_sparse;
    if (t.value_sparse) {
        j["value_sparse_config"] = t.value_sparse_config;
    }
    j["bit_sparse"] = t.bit_sparse;
    if (t.bit_sparse) {
        j["bit_sparse_config"] = t.bit_sparse_config;
    }
    j["input_bit_sparse"] = t.input_bit_sparse;
}

DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(CimUnitConfig, macro_total_cnt, macro_group_size, macro_size,
                                            name_as_memory, ipu, sram, adder_tree, shift_adder, result_adder,
                                            value_sparse, value_sparse_config, bit_sparse, bit_sparse_config,
                                            input_bit_sparse)

// MemoryUnit
bool RAMConfig::checkValid() const {
    if (!check_positive(size_byte, width_byte)) {
        std::cerr << "RAMConfig not valid, 'size_byte, width_byte' must be positive" << std::endl;
        return false;
    }
    if (size_byte % width_byte != 0) {
        std::cerr << "RAMConfig not valid, 'width_byte' cannot divide 'size_byte'" << std::endl;
        return false;
    }
    if (!check_not_negative(write_latency_cycle, read_latency_cycle, static_power_mW, write_dynamic_power_mW,
                            read_dynamic_power_mW)) {
        std::cerr << "RAMConfig not valid, 'write_latency_cycle, read_latency_cycle, static_power_mW, "
                     "write_dynamic_power_mW, read_dynamic_power_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    if (has_image && image_file.empty()) {
        std::cerr << "RAMConfig not valid, 'image_file' must be non-empty when RAM has a image file." << std::endl;
        return false;
    }
    return true;
}

void to_json(nlohmann::ordered_json& j, const RAMConfig& t) {
    j["size_byte"] = t.size_byte;
    j["width_byte"] = t.width_byte;
    j["write_latency_cycle"] = t.write_latency_cycle;
    j["read_latency_cycle"] = t.read_latency_cycle;
    j["static_power_mW"] = t.static_power_mW;
    j["write_dynamic_power_mW"] = t.write_dynamic_power_mW;
    j["read_dynamic_power_mW"] = t.read_dynamic_power_mW;
    j["has_image"] = t.has_image;
    if (t.has_image) {
        j["image_file"] = t.image_file;
    }
}

DEFINE_TYPE_FROM_JSON_FUNCTION_WITH_DEFAULT(RAMConfig, size_byte, width_byte, write_latency_cycle, read_latency_cycle,
                                            static_power_mW, write_dynamic_power_mW, read_dynamic_power_mW, has_image,
                                            image_file)

bool RegBufferConfig::checkValid() const {
    if (!check_positive(size_byte, read_max_width_byte, write_max_width_byte, rw_min_unit_byte)) {
        std::cerr << "RegBufferConfig not valid, 'size_byte, read_max_width_byte, write_max_width_byte, "
                     "rw_min_unit_byte' must be positive"
                  << std::endl;
        return false;
    }
    if (size_byte % read_max_width_byte != 0 || size_byte % write_max_width_byte != 0) {
        std::cerr
            << "RegBufferConfig not valid, 'read_max_width_byte' and 'write_max_width_byte' cannot divide 'size_byte'"
            << std::endl;
        return false;
    }
    if (read_max_width_byte % rw_min_unit_byte != 0 || write_max_width_byte % rw_min_unit_byte != 0) {
        std::cerr << "RegBufferConfig not valid, 'rw_min_unit_byte' cannot divide 'read_max_width_byte' and "
                     "'write_max_width_byte'"
                  << std::endl;
        return false;
    }
    if (!check_not_negative(static_power_mW, rw_dynamic_power_per_unit_mW)) {
        std::cerr << "RegBufferConfig not valid, 'static_power_mW, rw_dynamic_power_per_unit_mW' must be non-negative"
                  << std::endl;
        return false;
    }
    if (has_image && image_file.empty()) {
        std::cerr << "RegBufferConfig not valid, 'image_file' must be non-empty when RegBuffer has a image file."
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(RegBufferConfig, size_byte, read_max_width_byte, write_max_width_byte,
                                               rw_min_unit_byte, static_power_mW, rw_dynamic_power_per_unit_mW,
                                               has_image, image_file)

int MemoryConfig::getByteSize() const {
    return type == +MemoryType::ram ? ram_config.size_byte : reg_buffer_config.size_byte;
}

std::string MemoryConfig::getMemoryName() const {
    return name;
}

bool MemoryConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "MemoryConfig not valid, 'name' must be non-empty" << std::endl;
        return false;
    }
    if (name.find(DUPLICATE_MEMORY_NAME_DELIMITER) != std::string::npos) {
        std::cerr << fmt::format("MemoryConfig of {} not valid, 'name' has invalid char {} which is duplicate "
                                 "memory name delimiter",
                                 name, DUPLICATE_MEMORY_NAME_DELIMITER)
                  << std::endl;
        return false;
    }
    if (type == +MemoryType::other) {
        std::cerr << fmt::format("MemoryConfig of '{}' not valid, 'type' must be 'ram' or 'reg_buffer'", name)
                  << std::endl;
        return false;
    }
    if (duplicate_cnt <= 0) {
        std::cerr << fmt::format("MemoryConfig of '{}' not valid, 'duplicate_cnt' must be positive", name) << std::endl;
        return false;
    }
    if (const bool valid = ((type == +MemoryType::ram && ram_config.checkValid()) ||
                            (type == +MemoryType::reg_buffer && reg_buffer_config.checkValid()));
        !valid) {
        std::cerr << fmt::format("MemoryConfig of '{}' not valid", name) << std::endl;
        return false;
    }

    return true;
}

void to_json(nlohmann::ordered_json& j, const MemoryConfig& config) {
    j["name"] = config.name;
    j["type"] = config.type;
    j["duplicate_cnt"] = config.duplicate_cnt;
    if (config.type == +MemoryType::ram) {
        j["hardware_config"] = config.ram_config;
    } else if (config.type == +MemoryType::reg_buffer) {
        j["hardware_config"] = config.reg_buffer_config;
    }
}

void from_json(const nlohmann::ordered_json& j, MemoryConfig& config) {
    const MemoryConfig default_obj{};
    config.name = j.value("name", default_obj.name);
    JSON_VALUE_ENUM(config, type, "type")
    config.duplicate_cnt = j.value("duplicate_cnt", default_obj.duplicate_cnt);
    if (config.type == +MemoryType::ram) {
        config.ram_config = j.value("hardware_config", default_obj.ram_config);
    } else if (config.type == +MemoryType::reg_buffer) {
        config.reg_buffer_config = j.value("hardware_config", default_obj.reg_buffer_config);
    }
}

bool MemoryUnitConfig::checkValid() const {
    if (!check_vector_valid(memory_list)) {
        std::cerr << "MemoryUnitConfig not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(MemoryUnitConfig, memory_list)

bool DataPathMemoryConfig::isSameWith(const DataPathMemoryConfig& another) const {
    return name == another.name && duplicate_id == another.duplicate_id;
}

std::string DataPathMemoryConfig::toString() const {
    return fmt::format("name: {}, id: {}", name, duplicate_id);
}

bool DataPathMemoryConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "DataPathMemoryConfig not valid, name is empty" << std::endl;
        return false;
    }
    if (duplicate_id < 0) {
        std::cerr << fmt::format("DataPathMemoryConfig of '{}' not valid, 'duplicate_id' mush be not negative", name)
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(DataPathMemoryConfig, name, duplicate_id)

bool DataPathMemoryPairConfig::isSameWith(const DataPathMemoryPairConfig& another) const {
    if (memory_1.isSameWith(another.memory_1) && memory_2.isSameWith(another.memory_2)) {
        return true;
    }
    if (memory_1.isSameWith(another.memory_2) && memory_2.isSameWith(another.memory_1)) {
        return true;
    }
    return false;
}

bool DataPathMemoryPairConfig::checkValid() const {
    if (const bool valid = memory_1.checkValid() && memory_2.checkValid(); !valid) {
        std::cerr << "DataPathMemoryPairConfig not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(DataPathMemoryPairConfig, memory_1, memory_2)

bool LocalDedicatedDataPathConfig::conflictWith(const LocalDedicatedDataPathConfig& another) const {
    if (id == another.id) {
        std::cerr << fmt::format("LocalDedicatedDataPathConfig not valid, id '{}' is duplicate", id) << std::endl;
        return true;
    }
    for (auto& mem_pair : memory_pair_list) {
        for (auto& ano_mem_pair : another.memory_pair_list) {
            if (mem_pair.isSameWith(ano_mem_pair)) {
                std::cerr << fmt::format(
                                 "LocalDedicatedDataPathConfigs with id '{}' and '{}' are not valid, id '{}', they "
                                 "have same memory pair: ['{}', '{}']",
                                 id, another.id, mem_pair.memory_1.toString(), mem_pair.memory_2.toString())
                          << std::endl;
                return true;
            }
        }
    }

    return false;
}

bool LocalDedicatedDataPathConfig::checkValid() const {
    if (!check_vector_valid(memory_pair_list)) {
        std::cerr << fmt::format("LocalDedicatedDataPathConfig of '{}' not valid", id) << std::endl;
        return false;
    }

    for (int i = 0; i < memory_pair_list.size(); i++) {
        for (int j = i + 1; j < memory_pair_list.size(); j++) {
            if (memory_pair_list[i].isSameWith(memory_pair_list[j])) {
                std::cerr
                    << fmt::format(
                           "LocalDedicatedDataPathConfig of '{}' not valid, memory pair ['{}', '{}'] is duplicate", id,
                           memory_pair_list[i].memory_1.toString(), memory_pair_list[i].memory_2.toString())
                    << std::endl;
                return false;
            }
        }
    }

    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(LocalDedicatedDataPathConfig, id, memory_pair_list)

bool TransferUnitConfig::checkValid() const {
    // check LocalDedicatedDataPathConfig
    for (auto& data_path_cfg : local_dedicated_data_path_list) {
        if (!data_path_cfg.checkValid()) {
            std::cerr << "TransferUnitConfig not valid" << std::endl;
            return false;
        }
    }

    // check LocalDedicatedDataPathConfig confilct
    for (int i = 0; i < local_dedicated_data_path_list.size(); i++) {
        for (int j = i + 1; j < local_dedicated_data_path_list.size(); j++) {
            if (local_dedicated_data_path_list[i].conflictWith(local_dedicated_data_path_list[j])) {
                std::cerr << "TransferUnitConfig not valid" << std::endl;
                return false;
            }
        }
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(TransferUnitConfig, pipeline, local_dedicated_data_path_list)

// CoreConfig
bool CoreConfig::checkValid() const {
    if (const bool valid = control_unit_config.checkValid() && register_unit_config.checkValid() &&
                           scalar_unit_config.checkValid() && simd_unit_config.checkValid() &&
                           reduce_unit_config.checkValid() && cim_unit_config.checkValid() &&
                           local_memory_unit_config.checkValid() && transfer_unit_config.checkValid();
        !valid) {
        std::cerr << "CoreConfig not valid" << std::endl;
        return false;
    }

    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(CoreConfig, control_unit_config, register_unit_config,
                                               scalar_unit_config, simd_unit_config, reduce_unit_config,
                                               cim_unit_config, local_memory_unit_config, transfer_unit_config)

// NetworkConfig
bool NetworkConfig::checkValid() const {
    if (const bool valid = bus_width_byte > 0 && !network_config_file_path.empty(); !valid) {
        std::cerr << "NetworkConfig not valid, 'bus_width_byte' must be positive and 'network_config_file_path' must "
                     "not be empty"
                  << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(NetworkConfig, bus_width_byte, network_config_file_path);

bool GlobalMemoryConfig::checkValid() const {
    if (!global_memory_unit_config.checkValid()) {
        std::cerr << "GlobalMemoryConfig not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(GlobalMemoryConfig, global_memory_unit_config, global_memory_switch_id)

bool AddressSpaceElementConfig::checkValid() const {
    if (name.empty()) {
        std::cerr << "AddressSpaceConfig not valid, name must not be empty" << std::endl;
        return false;
    }
    if (name.find(DUPLICATE_MEMORY_NAME_DELIMITER) != std::string::npos) {
        std::cerr << fmt::format("AddressSpaceConfig of {} not valid, 'name' has invalid char {} which is duplicate "
                                 "memory name delimiter",
                                 name, DUPLICATE_MEMORY_NAME_DELIMITER)
                  << std::endl;
        return false;
    }
    if (size == 0) {
        std::cerr << fmt::format("AddressSpaceConfig of {} not valid, size must not be zero", name) << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(AddressSpaceElementConfig, name, size)

// ChipConfig
bool ChipConfig::checkAddressSpaceWithMemory(const std::string& mem_name, int mem_size,
                                             const std::unordered_map<std::string, AddressSpaceElementConfig>& as_map) {
    auto found = as_map.find(mem_name);
    if (found == as_map.end()) {
        std::cerr << fmt::format("Memory \"{}\" not in address space", mem_name) << std::endl;
        return false;
    }

    auto& address_space = found->second;
    if (address_space.size > 0 && address_space.size < mem_size) {
        std::cerr << fmt::format("Size of Memory \"{}\" is bigger than size of address space", mem_name) << std::endl;
        return false;
    }

    return true;
}

ChipConfig::MemoryInfoMap ChipConfig::getMemoryInfoMap(bool check) const {
    std::unordered_map<std::string, MemoryInfo> mem_map;
    auto trans_mem = [&](const auto& mem_config, bool is_global, int duplicate_cnt) {
        mem_map.insert(std::make_pair<std::string, MemoryInfo>(mem_config.getMemoryName(),
                                                               {mem_config.getByteSize(), is_global, duplicate_cnt}));
    };

    // add all local memory
    auto& local_mems = core_config.local_memory_unit_config.memory_list;
    std::for_each(local_mems.begin(), local_mems.end(),
                  [&](const MemoryConfig& c) { return trans_mem(c, false, c.duplicate_cnt); });

    // add all global memory
    auto& global_mems = global_memory_config.global_memory_unit_config.memory_list;
    std::for_each(global_mems.begin(), global_mems.end(),
                  [&](const MemoryConfig& c) { return trans_mem(c, true, c.duplicate_cnt); });

    // all all mounted memory
    constexpr int mounted_memory_cnt = 1;
    trans_mem(core_config.cim_unit_config, false, 1);

    if (check && mem_map.size() != local_mems.size() + global_mems.size() + mounted_memory_cnt) {
        return {};
    }
    return std::move(mem_map);
}

bool ChipConfig::checkAddressSpace(const MemoryInfoMap& mem_map) const {
    // Check AddressSpace, and check if AddressSpace and Memory correspond one to one
    // transfer address space to map, and check address space config
    std::unordered_map<std::string, AddressSpaceElementConfig> as_map;
    std::transform(address_space_config.begin(), address_space_config.end(), std::inserter(as_map, as_map.end()),
                   [](const AddressSpaceElementConfig& as) { return std::make_pair(as.name, as); });
    if (as_map.size() != address_space_config.size()) {
        std::cerr << "Address space not valid, it has duplicate parts with same name" << std::endl;
        return false;
    }

    // check address space config with memory
    if (mem_map.size() != as_map.size()) {
        std::cerr << "Address space not valid, count of memory and address space is not same" << std::endl;
        return false;
    }
    for (const auto& [name, info] : mem_map) {
        if (!checkAddressSpaceWithMemory(name, info.size, as_map)) {
            std::cerr << "Address space not valid" << std::endl;
            return false;
        }
    }
    return true;
}

bool ChipConfig::checkLocalDedicatedDataPath(const MemoryInfoMap& mem_map) const {
    auto check_data_path_mem_cfg = [&mem_map](const DataPathMemoryConfig& mem_cfg) -> bool {
        auto found = mem_map.find(mem_cfg.name);
        if (found == mem_map.end()) {
            std::cerr << fmt::format("DataPathMemoryConfig not valid, memory with name '{}' does not exist",
                                     mem_cfg.name)
                      << std::endl;
            return false;
        }
        auto& mem_info = found->second;
        if (mem_info.is_global) {
            std::cerr << fmt::format("DataPathMemoryConfig not valid, memory with name '{}' is global memory",
                                     mem_cfg.name)
                      << std::endl;
            return false;
        }
        if (mem_cfg.duplicate_id >= mem_info.duplicate_cnt) {
            std::cerr << fmt::format("DataPathMemoryConfig not valid, memory with name '{}' and duplicate id '{}' does "
                                     "not exist, the max duplicate id is {}",
                                     mem_cfg.name, mem_cfg.duplicate_id, mem_info.duplicate_cnt - 1)
                      << std::endl;
            return false;
        }
        return true;
    };

    // check local decicated data path could correspond to real memory
    for (auto& data_path_cfg : core_config.transfer_unit_config.local_dedicated_data_path_list) {
        for (auto& memory_pair_cfg : data_path_cfg.memory_pair_list) {
            if (const bool valid = check_data_path_mem_cfg(memory_pair_cfg.memory_1) &&
                                   check_data_path_mem_cfg(memory_pair_cfg.memory_2);
                !valid) {
                std::cerr << fmt::format("LocalDedicatedDataPathConfig with id '{}' not valid", data_path_cfg.id)
                          << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool ChipConfig::checkValid() const {
    if (!check_positive(core_cnt)) {
        std::cerr << "ChipConfig not valid, 'core_cnt' must be positive" << std::endl;
        return false;
    }
    if (const bool valid = core_config.checkValid() && global_memory_config.checkValid() &&
                           network_config.checkValid() && check_vector_valid(address_space_config);
        !valid) {
        std::cerr << "ChipConfig not valid" << std::endl;
        return false;
    }
    // check if the memory name is repeated
    auto mem_map = getMemoryInfoMap(true);
    if (mem_map.empty()) {
        std::cerr << "Local and global memory not valid, there are repeated names" << std::endl;
        return false;
    }
    if (!checkAddressSpace(mem_map) || !checkLocalDedicatedDataPath(mem_map)) {
        std::cerr << "ChipConfig not valid" << std::endl;
        return false;
    }

    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ChipConfig, core_cnt, core_config, global_memory_config, network_config,
                                               address_space_config)

// SimConfig
bool SimConfig::checkValid() const {
    if (!check_positive(period_ns)) {
        std::cerr << "SimConfig not valid, 'period_ns' must be positive" << std::endl;
        return false;
    }
    if (sim_mode == +SimMode::other) {
        std::cerr << "SimConfig not valid, 'sim_mode' must be 'run_until_time' or 'run_one_round'" << std::endl;
        return false;
    }
    if (data_mode == +DataMode::other) {
        std::cerr << "SimConfig not valid, 'data_mode' must be 'real_data' or 'not_real_data'" << std::endl;
        return false;
    }
    if (sim_mode == +SimMode::run_until_time && !check_positive(sim_time_ms)) {
        std::cerr << "SimConfig not valid, 'sim_time_ms' must be positive" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(SimConfig, period_ns, sim_mode, data_mode, sim_time_ms)

// Config
bool Config::checkValid() const {
    if (!(chip_config.checkValid() && sim_config.checkValid())) {
        std::cerr << "Config not valid" << std::endl;
        return false;
    }
    return true;
}

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(Config, chip_config, sim_config)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(HardwareProfilerConfig, profiling, record_timing_segments,
                                               each_core_profiling, report_level_cnt_)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(InstProfilerGroupConfig, name, sub_groups)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(InstProfilerConfig, single_inst_profiling, inst_type_profiling,
                                               inst_group_profiling, inst_groups)

DEFINE_TYPE_FROM_TO_JSON_FUNCTION_WITH_DEFAULT(ProfilerConfig, profiling, report_to_json, json_flat, json_file,
                                               hardware_profiler_config, inst_profiler_config)

}  // namespace cimsim
