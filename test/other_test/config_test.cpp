//
// Created by wyk on 2024/6/20.
//

#include <fstream>
#include <iostream>
#include <string>

#include "config/config.h"

using namespace pimsim;

const std::string CONFIG_FILE = "../config/config_template.json";
const std::string TEST_CONFIG_FILE = "../config/config1.json";

Config get_config() {
    Config config;

    config.chip_config.core_config.register_unit_config.special_register_binding.emplace_back(
        SpecialRegisterBindingConfig{});
    config.chip_config.core_config.scalar_unit_config.functor_list.emplace_back(ScalarFunctorConfig{});

    SIMDDataWidthConfig functor_data_width_config{.inputs = {32, 32, 0, 0}, .output = 8};
    SIMDFunctorConfig functor_config{.name = "quantify", .data_bit_width = functor_data_width_config};
    config.chip_config.core_config.simd_unit_config.functor_list.emplace_back(functor_config);

    SIMDDataWidthConfig input_data_width_config{.inputs = {32, 32, 0, 0}};
    SIMDInstructionFunctorBindingConfig binding_config{.input_bit_width = input_data_width_config,
                                                       .functor_name = "quantify"};
    SIMDInstructionConfig instruction_config{.name = "vqv", .input_cnt = 2};
    instruction_config.functor_binding_list.emplace_back(binding_config);
    config.chip_config.core_config.simd_unit_config.instruction_list.emplace_back(instruction_config);

    config.chip_config.core_config.pim_unit_config.address_space =
        AddressSpaceConfig{.offset_byte = 0, .size_byte = 1024};
    config.chip_config.core_config.pim_unit_config.macro_group_size = 16;
    config.chip_config.core_config.pim_unit_config.value_sparse = true;
    config.chip_config.core_config.pim_unit_config.bit_sparse = true;

    LocalMemoryConfig l1{.name = "l1",
                         .type = LocalMemoryType::ram,
                         .addressing = AddressSpaceConfig{.offset_byte = 1024, .size_byte = 1024},
                         .ram_config = RAMConfig{}};
    LocalMemoryConfig l2{.name = "l2",
                         .type = LocalMemoryType::reg_buffer,
                         .addressing = AddressSpaceConfig{.offset_byte = 2048, .size_byte = 1024},
                         .reg_buffer_config = RegBufferConfig{}};
    config.chip_config.core_config.local_memory_unit_config.local_memory_list.emplace_back(l1);
    config.chip_config.core_config.local_memory_unit_config.local_memory_list.emplace_back(l2);

    return std::move(config);
}

void write_config() {
    auto config = get_config();

    nlohmann::ordered_json j = config;

    std::ofstream ofs;
    ofs.open(CONFIG_FILE);
    ofs << j;
    ofs.close();
}

void read_config() {
    std::ifstream ifs;
    ifs.open(CONFIG_FILE);
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);
    auto config = j.get<Config>();

    nlohmann::ordered_json j1 = config;
    std::ofstream ofs;
    ofs.open(TEST_CONFIG_FILE);
    ofs << j1;
    ofs.close();
}

void check_config() {
    std::ifstream ifs;
    ifs.open(CONFIG_FILE);
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);

    if (auto config = j.get<Config>(); config.checkValid()) {
        std::cout << "Config valid";
    }
}

int main() {
    check_config();
}
