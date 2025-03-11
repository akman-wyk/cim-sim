//
// Created by wyk on 2024/6/14.
//

#pragma once

#include <array>
#include <string>
#include <vector>

#include "config_enum.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace cimsim {

static constexpr int GENERAL_REG_NUM = 32;
static constexpr int SPECIAL_REG_NUM = 32;

static constexpr int WORD_BYTE_SIZE = 4;

static constexpr int SIMD_MAX_INPUT_NUM = 4;
static constexpr int SIMD_MAX_OPCODE = 255;
static constexpr int SIMD_INSTRUCTION_OPCODE_BIT_LENGTH = 8;

static constexpr int BYTE_TO_BIT = 8;
static constexpr unsigned char BYTE_MAX_VALUE = 0xff;

struct ControlUnitConfig {
    double controller_static_power_mW{0.0};   // mW
    double controller_dynamic_power_mW{0.0};  // mW

    double fetch_static_power_mW{0.0};   // mW
    double fetch_dynamic_power_mW{0.0};  // mW

    double decode_static_power_mW{0.0};   // mW
    double decode_dynamic_power_mW{0.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ControlUnitConfig)
};

struct SpecialRegisterBindingConfig {
    int special{};  // 0-31
    int general{};  // 0-31

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SpecialRegisterBindingConfig)
};

struct RegisterUnitConfig {
    double static_power_mW{0.0};   // mW
    double dynamic_power_mW{0.0};  // mW

    std::vector<SpecialRegisterBindingConfig> special_register_binding{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RegisterUnitConfig)
};

struct ScalarFunctorConfig {
    std::string inst_name{"scalar-RR-add"};
    double static_power_mW{0.0};   // mW
    double dynamic_power_mW{0.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarFunctorConfig)
};

struct ScalarUnitConfig {
    double default_functor_static_power_mW{0.0};   // mW
    double default_functor_dynamic_power_mW{0.0};  // mW
    std::vector<ScalarFunctorConfig> functor_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ScalarUnitConfig)
};

struct SIMDDataWidthConfig {
    // data bit-width of input and output of SIMD functor
    std::array<int, SIMD_MAX_INPUT_NUM> inputs{0, 0, 0, 0};
    int output{0};  // bit

    [[nodiscard]] static bool checkDataWidth(int width);
    [[nodiscard]] bool inputBitWidthMatch(const SIMDDataWidthConfig& other) const;
    [[nodiscard]] bool checkValid(unsigned int input_cnt, bool check_output) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDDataWidthConfig)
};

struct SIMDFunctorConfig {
    std::string name{};  // need to write an execution function with the same name in file TODO
    unsigned int input_cnt{2};
    SIMDDataWidthConfig data_bit_width{};

    int functor_cnt{32};
    int latency_cycle{1};                      // cycle
    double static_power_per_functor_mW{1.0};   // mW
    double dynamic_power_per_functor_mW{1.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDFunctorConfig)
};

struct SIMDInstructionFunctorBindingConfig {
    SIMDDataWidthConfig input_bit_width{};
    std::string functor_name{};

    [[nodiscard]] bool checkValid(unsigned int input_cnt) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInstructionFunctorBindingConfig)
};

struct SIMDInstructionConfig {
    std::string name{};
    unsigned int input_cnt{2};
    unsigned int opcode{0x00};

    std::array<SIMDInputType, SIMD_MAX_INPUT_NUM> inputs_type{SIMDInputType::vector, SIMDInputType::vector,
                                                              SIMDInputType::vector, SIMDInputType::vector};

    std::vector<SIMDInstructionFunctorBindingConfig> functor_binding_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDInstructionConfig)
};

struct SIMDUnitConfig {
    bool pipeline{false};
    std::vector<SIMDFunctorConfig> functor_list{};
    std::vector<SIMDInstructionConfig> instruction_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDUnitConfig)
};

struct CimMacroSizeConfig {
    // macro 4-dim size (H, W, m, n)
    int compartment_cnt_per_macro{};    // H, element row cnt
    int element_cnt_per_compartment{};  // W, element column cnt
    int row_cnt_per_element{};          // m, element row size
    int bit_width_per_row{};            // n, element column size

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimMacroSizeConfig)
};

struct CimModuleConfig {
    int latency_cycle{1};          // cycle
    double static_power_mW{1.0};   // mW
    double dynamic_power_mW{1.0};  // mW

    [[nodiscard]] bool checkValid(const std::string& module_name) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimModuleConfig)
};

struct CimSRAMConfig {
    /* Cim SRAM hardware config
     * Cim write:
     *      All Macros can be written in parallel, each Macro can only write a whole row of one Compartment or not.
     * Cim read:
     *      All Macros can be read in parallel, each Macro can read the same whole row of 0 to all Compartments in
     * parallel.
     *
     * Then, as_mode only influences write-data-width:
     *      When as_mode is intergroup, it means all MacroGroups can be written in parallel, so that
     *      write-data-width = macro-width * macro-total-count.
     *      When as_mode is intragroup, it means only one MacroGroup can be written at a time, so that
     *      write-data-width = macro-width * macro-group-size.
     * And write-unit is always macro-width, which means the minimum write data unit width.
     */
    CimASMode as_mode{CimASMode::intergroup};

    int write_latency_cycle{1};
    int read_latency_cycle{1};
    double static_power_mW{1.0};
    double write_dynamic_power_per_bit_mW{1.0};
    double read_dynamic_power_per_bit_mW{1.0};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimSRAMConfig)
};

struct CimValueSparseConfig {
    // sparse mask config
    int mask_bit_width{1};

    // Input process module config: CimUnit only has one
    int latency_cycle{1};           // cycle
    double static_power_mW{1.0};    // mW
    double dynamic_power_mW{1.0};   // mW
    int output_macro_group_cnt{1};  // The number of macros processed at a time

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimValueSparseConfig)
};

struct CimBitSparseConfig {
    // sparse mask config
    int mask_bit_width{3};

    // post process module config: each Element has one
    int latency_cycle{1};          // cycle
    double static_power_mW{1.0};   // mW
    double dynamic_power_mW{1.0};  // mW

    // reg buffer
    int unit_byte{1};
    double reg_buffer_static_power_mW{1.0};            // mW
    double reg_buffer_dynamic_power_mW_per_unit{1.0};  // mW

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimBitSparseConfig)
};

struct AddressSpaceConfig {
    int offset_byte{};  // byte
    int size_byte{};    // byte

    [[nodiscard]] bool checkValid() const;

    [[nodiscard]] int end() const {
        return offset_byte + size_byte;
    }

    [[nodiscard]] bool contains(int address) const {
        return offset_byte <= address && address < offset_byte + size_byte;
    }

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(AddressSpaceConfig)
};

struct CimUnitConfig {
    // macro scale config
    int macro_total_cnt{0};
    int macro_group_size{0};
    CimMacroSizeConfig macro_size{};

    // address space
    AddressSpaceConfig address_space{};

    // modules config: ipu -> SRAM -> post process modules
    // ipu(input process unit) module
    CimModuleConfig ipu{};
    // SRAM module
    CimSRAMConfig sram{};
    // post process modules, each Element column has one
    CimModuleConfig adder_tree{};
    CimModuleConfig shift_adder{};
    CimModuleConfig result_adder{};

    // extensions config
    bool value_sparse{false};
    CimValueSparseConfig value_sparse_config{};

    bool bit_sparse{false};
    CimBitSparseConfig bit_sparse_config{};

    bool input_bit_sparse{false};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CimUnitConfig)
};

struct RAMConfig {
    int size_byte{1024};  // Byte, total ram size
    int width_byte{16};   // Byte, the byte-width of single read and write data

    int write_latency_cycle{1};  // cycle
    int read_latency_cycle{1};   // cycle

    double static_power_mW{1.0};         // mW
    double write_dynamic_power_mW{1.0};  // mW
    double read_dynamic_power_mW{1.0};   // mW

    bool has_image{false};     // whether RAM memory has an image file
    std::string image_file{};  // RAM memory image file path

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RAMConfig)
};

struct RegBufferConfig {
    int size_byte{1024};           // Byte, total register buffer size
    int read_max_width_byte{16};   // Byte, the max byte-width of single read data
    int write_max_width_byte{16};  // Byte, the max byte-width of single write data
    int rw_min_unit_byte{4};       // Byte, the min unit byte-width of single read and write data

    double static_power_mW{1.0};               // mW
    double rw_dynamic_power_per_unit_mW{1.0};  // mW

    bool has_image{false};     // whether RAM memory has an image file
    std::string image_file{};  // RAM memory image file path

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(RegBufferConfig)
};

struct LocalMemoryConfig {
    std::string name{};
    LocalMemoryType type{LocalMemoryType::ram};

    AddressSpaceConfig addressing{};

    RAMConfig ram_config{};
    RegBufferConfig reg_buffer_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalMemoryConfig)
};

struct LocalMemoryUnitConfig {
    std::vector<LocalMemoryConfig> local_memory_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalMemoryUnitConfig)
};

struct TransferUnitConfig {
    bool pipeline{false};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TransferUnitConfig)
};

struct CoreConfig {
    ControlUnitConfig control_unit_config{};
    RegisterUnitConfig register_unit_config{};
    ScalarUnitConfig scalar_unit_config{};
    SIMDUnitConfig simd_unit_config{};
    CimUnitConfig cim_unit_config{};
    LocalMemoryUnitConfig local_memory_unit_config{};
    TransferUnitConfig transfer_unit_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(CoreConfig)
};

struct NetworkConfig {
    int bus_width_byte{16};

    std::string network_config_file_path{"./network_config.json"};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(NetworkConfig)
};

struct GlobalMemoryConfig {
    RAMConfig hardware_config{};
    AddressSpaceConfig addressing{};
    int global_memory_switch_id{-10};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(GlobalMemoryConfig)
};

struct ChipConfig {
    int core_cnt{1};
    CoreConfig core_config{};
    GlobalMemoryConfig global_memory_config{};
    NetworkConfig network_config;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ChipConfig)
};

struct SimConfig {
    double period_ns{1.0};  // ns
    SimMode sim_mode{SimMode::run_one_round};
    DataMode data_mode{DataMode::real_data};
    double sim_time_ms{1.0};  // ms

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SimConfig)
};

struct Config {
    ChipConfig chip_config{};
    SimConfig sim_config{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(Config)
};

}  // namespace cimsim