//
// Created by wyk on 2024/6/14.
//

#pragma once

#include <array>
#include <string>
#include <vector>

#include "config_enum.h"
#include "constant.h"
#include "nlohmann/json.hpp"
#include "util/macro_scope.h"

namespace cimsim {

using SIMDInputsArray = std::array<int, SIMD_MAX_INPUT_NUM>;

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

[[nodiscard]] static bool checkDataWidth(int width);

struct SIMDDataWidthConfig {
    // data bit-width of input and output of SIMD functor
    SIMDInputsArray inputs{0, 0, 0, 0};
    int output{0};  // bit

    [[nodiscard]] bool inputBitWidthMatch(const SIMDDataWidthConfig& other) const;
    [[nodiscard]] bool checkValid(unsigned int input_cnt, bool check_output) const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(SIMDDataWidthConfig)
};

struct SIMDFunctorConfig {
    std::string name{};
    unsigned int input_cnt{2};
    SIMDDataWidthConfig data_bit_width{};

    int functor_cnt{32};
    int latency_cycle{1};                      // cycle
    int pipeline_stage_cnt{1};                 // pipeline stage count
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

struct ReduceFunctorConfig {
    std::string name{};
    unsigned int funct{0};

    int input_bit_width{0};
    int output_bit_width{0};

    int reduce_input_cnt{32};
    int latency_cycle{1};
    int pipeline_stage_cnt{1};
    double static_power_mW{1.0};
    double dynamic_power_mW{1.0};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ReduceFunctorConfig)
};

struct ReduceUnitConfig {
    bool pipeline{false};
    std::vector<ReduceFunctorConfig> functor_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ReduceUnitConfig)
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
    int pipeline_stage_cnt{1};     // count of pipeline stages
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

struct CimUnitConfig {
    // macro scale config
    int macro_total_cnt{0};
    int macro_group_size{0};
    CimMacroSizeConfig macro_size{};

    // address space
    std::string name_as_memory{"cim_unit"};

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

    // memory interface
    [[nodiscard]] int getByteSize() const;
    [[nodiscard]] int getBitWidth() const;
    [[nodiscard]] int getByteWidth() const;
    [[nodiscard]] std::string getMemoryName() const;

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

struct MemoryConfig {
    std::string name{};
    MemoryType type{MemoryType::ram};
    int duplicate_cnt{1};

    RAMConfig ram_config{};
    RegBufferConfig reg_buffer_config{};

    // memory interface
    [[nodiscard]] int getByteSize() const;
    [[nodiscard]] std::string getMemoryName() const;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(MemoryConfig)
};

struct MemoryUnitConfig {
    std::vector<MemoryConfig> memory_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(MemoryUnitConfig)
};

struct DataPathMemoryConfig {
    std::string name{};
    int duplicate_id{0};

    [[nodiscard]] bool isSameWith(const DataPathMemoryConfig& another) const;
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(DataPathMemoryConfig)
};

struct DataPathMemoryPairConfig {
    DataPathMemoryConfig memory_1;
    DataPathMemoryConfig memory_2;

    [[nodiscard]] bool isSameWith(const DataPathMemoryPairConfig& another) const;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(DataPathMemoryPairConfig)
};

struct LocalDedicatedDataPathConfig {
    unsigned int id{0};
    std::vector<DataPathMemoryPairConfig> memory_pair_list;

    [[nodiscard]] bool conflictWith(const LocalDedicatedDataPathConfig& another) const;

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(LocalDedicatedDataPathConfig)
};

struct TransferUnitConfig {
    bool pipeline{false};
    std::vector<LocalDedicatedDataPathConfig> local_dedicated_data_path_list{};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(TransferUnitConfig)
};

struct CoreConfig {
    ControlUnitConfig control_unit_config{};
    RegisterUnitConfig register_unit_config{};
    ScalarUnitConfig scalar_unit_config{};
    SIMDUnitConfig simd_unit_config{};
    ReduceUnitConfig reduce_unit_config{};
    CimUnitConfig cim_unit_config{};
    MemoryUnitConfig local_memory_unit_config{};
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
    MemoryUnitConfig global_memory_unit_config{};
    int global_memory_switch_id{-10};

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(GlobalMemoryConfig)
};

struct AddressSpaceElementConfig {
    std::string name{};
    int size{-1};  // negative size means that size of memory with same name will be used as size of this address space

    [[nodiscard]] bool checkValid() const;
    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(AddressSpaceElementConfig)
};

struct ChipConfig {
    struct MemoryInfo {
        int size{-1};
        bool is_global{false};
        int duplicate_cnt{1};
    };

    using MemoryInfoMap = std::unordered_map<std::string, MemoryInfo>;

    int core_cnt{1};
    CoreConfig core_config{};
    GlobalMemoryConfig global_memory_config{};
    NetworkConfig network_config{};
    std::vector<AddressSpaceElementConfig> address_space_config{};

    [[nodiscard]] static bool checkAddressSpaceWithMemory(
        const std::string& mem_name, int mem_size,
        const std::unordered_map<std::string, AddressSpaceElementConfig>& as_map);
    [[nodiscard]] MemoryInfoMap getMemoryInfoMap(bool check = false) const;

    [[nodiscard]] bool checkAddressSpace(const MemoryInfoMap& mem_map) const;
    [[nodiscard]] bool checkLocalDedicatedDataPath(const MemoryInfoMap& mem_map) const;

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

struct HardwareProfilerConfig {
    bool profiling{false};
    bool record_timing_segments{false};
    bool each_core_profiling{false};
    int report_level_cnt_{0};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(HardwareProfilerConfig)
};

struct InstProfilerGroupConfig {
    std::string name{};
    std::vector<InstProfilerGroupConfig> sub_groups{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(InstProfilerGroupConfig)
};

struct InstProfilerConfig {
    bool single_inst_profiling{false};
    bool inst_type_profiling{false};
    bool inst_group_profiling{false};

    std::vector<InstProfilerGroupConfig> inst_groups{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(InstProfilerConfig)
};

struct ProfilerConfig {
    bool profiling{false};
    bool report_to_json{false};
    bool json_flat{false};
    std::string json_file{};

    HardwareProfilerConfig hardware_profiler_config{};
    InstProfilerConfig inst_profiler_config{};

    DECLARE_TYPE_FROM_TO_JSON_FUNCTION_INTRUSIVE(ProfilerConfig)
};

}  // namespace cimsim