
# project
cmake_minimum_required(VERSION 3.20)
#cmake_policy(SET CMP0091 NEW)
project(cim-sim LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Use C++ 17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# dependency
add_subdirectory(packages/fmt)
add_subdirectory(packages/systemc)
add_subdirectory(packages/json)

add_library(cim-simulator STATIC ""
        src/address_space/address_space.cpp
        src/address_space/address_space.h

        src/base_component/base_module.cpp
        src/base_component/base_module.h
        src/base_component/clock.cpp
        src/base_component/clock.h
        src/base_component/energy_counter.cpp
        src/base_component/energy_counter.h
        src/base_component/fsm.h
        src/base_component/submodule_socket.h

        src/chip/chip.cpp
        src/chip/chip.h

        src/config/config.cpp
        src/config/config.h
        src/config/config_enum.cpp
        src/config/config_enum.h
        src/config/constant.h

        src/core/cim_unit/cim_unit.cpp
        src/core/cim_unit/cim_unit.h
        src/core/cim_unit/macro.cpp
        src/core/cim_unit/macro.h
        src/core/cim_unit/macro_group.cpp
        src/core/cim_unit/macro_group.h
        src/core/cim_unit/macro_group_controller.cpp
        src/core/cim_unit/macro_group_controller.h
        src/core/cim_unit/payload.h

        src/core/conflict/conflict_handler.cpp
        src/core/conflict/conflict_handler.h
        src/core/conflict/payload.cpp
        src/core/conflict/payload.h

        src/core/decoder/decoder.h
        src/core/decoder/decoder_v1.cpp
        src/core/decoder/decoder_v2.cpp
        src/core/decoder/decoder_v3.cpp

        src/core/execute_unit/cim_compute_unit.cpp
        src/core/execute_unit/cim_compute_unit.h
        src/core/execute_unit/cim_control_unit.cpp
        src/core/execute_unit/cim_control_unit.h
        src/core/execute_unit/execute_unit.cpp
        src/core/execute_unit/execute_unit.h
        src/core/execute_unit/payload.cpp
        src/core/execute_unit/payload.h
        src/core/execute_unit/scalar_unit.cpp
        src/core/execute_unit/scalar_unit.h
        src/core/execute_unit/simd_unit.cpp
        src/core/execute_unit/simd_unit.h
        src/core/execute_unit/transfer_unit.cpp
        src/core/execute_unit/transfer_unit.h

        src/core/reg_unit/reg_unit.cpp
        src/core/reg_unit/reg_unit.h

        src/core/socket/memory_socket.cpp
        src/core/socket/memory_socket.h
        src/core/socket/transmit_socket.cpp
        src/core/socket/transmit_socket.h

        src/core/payload.h
        src/core/core.cpp
        src/core/core.h

        src/isa/inst_v1.cpp
        src/isa/inst_v1.h
        src/isa/inst_v2.cpp
        src/isa/inst_v2.h
        src/isa/inst_v3.cpp
        src/isa/inst_v3.h
        src/isa/isa.h
        src/isa/isa_v2.h

        src/memory/global_memory.cpp
        src/memory/global_memory.h
        src/memory/memory.cpp
        src/memory/memory.h
        src/memory/memory_hardware.h
        src/memory/memory_unit.cpp
        src/memory/memory_unit.h
        src/memory/payload.h
        src/memory/ram.cpp
        src/memory/ram.h
        src/memory/reg_buffer.cpp
        src/memory/reg_buffer.h

        src/network/network.cpp
        src/network/network.h
        src/network/payload.h
        src/network/switch.cpp
        src/network/switch.h

        src/util/ins_stat.cpp
        src/util/ins_stat.h
        src/util/log.cpp
        src/util/log.h
        src/util/macro_scope.h
        src/util/reporter.cpp
        src/util/reporter.h
        src/util/util.cpp
        src/util/util.h
        src/core/cim_unit/macro_module.cpp
        src/core/cim_unit/macro_module.h
        src/core/cim_unit/macro_group_module.cpp
        src/core/cim_unit/macro_group_module.h
)
set_target_properties(cim-simulator PROPERTIES OUTPUT_NAME "cim-simulator")
target_include_directories(cim-simulator PRIVATE
        src
)

# header-only dependency
target_include_directories(cim-simulator PUBLIC packages/header-only)
target_include_directories(cim-simulator PUBLIC packages/header-only/zstr/src)

add_dependencies(cim-simulator systemc nlohmann_json fmt)

target_link_libraries(cim-simulator PUBLIC
        systemc
        nlohmann_json
        fmt
)

add_executable(ConfigTest test/other_test/config_test.cpp
        src/config/config.h
        src/config/config.cpp
        src/config/config_enum.h
        src/config/config_enum.cpp
        src/util/macro_scope.h)
add_dependencies(ConfigTest nlohmann_json fmt)
target_link_libraries(ConfigTest PUBLIC nlohmann_json fmt)
target_include_directories(ConfigTest PRIVATE src)
target_include_directories(ConfigTest PUBLIC packages/header-only)

add_executable(MemoryUnitTest "" test/other_test/memory_unit_test.cpp)
add_dependencies(MemoryUnitTest cim-simulator)
target_link_libraries(MemoryUnitTest PRIVATE cim-simulator)
target_include_directories(MemoryUnitTest PRIVATE src)

add_executable(SIMDUnitTest "" test/execute_unit_test/simd_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(SIMDUnitTest cim-simulator)
target_link_libraries(SIMDUnitTest PRIVATE cim-simulator)
target_include_directories(SIMDUnitTest PRIVATE src)

add_executable(TransferUnitTest "" test/execute_unit_test/transfer_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(TransferUnitTest cim-simulator)
target_link_libraries(TransferUnitTest PRIVATE cim-simulator)
target_include_directories(TransferUnitTest PRIVATE src)

add_executable(MacroTest "" test/other_test/macro_test.cpp)
add_dependencies(MacroTest cim-simulator)
target_link_libraries(MacroTest PRIVATE cim-simulator)
target_include_directories(MacroTest PRIVATE src)

add_executable(MacroGroupTest "" test/other_test/macro_group_test.cpp)
add_dependencies(MacroGroupTest cim-simulator)
target_link_libraries(MacroGroupTest PRIVATE cim-simulator)
target_include_directories(MacroGroupTest PRIVATE src)

add_executable(CimComputeUnitTest "" test/execute_unit_test/cim_compute_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(CimComputeUnitTest cim-simulator)
target_link_libraries(CimComputeUnitTest PRIVATE cim-simulator)
target_include_directories(CimComputeUnitTest PRIVATE src)

add_executable(CimControlUnitTest "" test/execute_unit_test/cim_control_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h
)
add_dependencies(CimControlUnitTest cim-simulator)
target_link_libraries(CimControlUnitTest PRIVATE cim-simulator)
target_include_directories(CimControlUnitTest PRIVATE src)

add_executable(CoreTest "" test/core_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h)
add_dependencies(CoreTest cim-simulator)
target_link_libraries(CoreTest PRIVATE cim-simulator)
target_include_directories(CoreTest PRIVATE src)

add_executable(ChipTest "" test/chip_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h)
add_dependencies(ChipTest cim-simulator)
target_link_libraries(ChipTest PRIVATE cim-simulator)
target_include_directories(ChipTest PRIVATE src)

add_executable(LayerSimulator "" src/simulator/layer_simulator.cpp src/simulator/layer_simulator.h)
add_dependencies(LayerSimulator cim-simulator)
target_link_libraries(LayerSimulator PRIVATE cim-simulator)
target_include_directories(LayerSimulator PRIVATE src)

add_executable(NetworkSimulator "" src/simulator/network_simulator.cpp src/simulator/network_simulator.h)
add_dependencies(NetworkSimulator cim-simulator)
target_link_libraries(NetworkSimulator PRIVATE cim-simulator)
target_include_directories(NetworkSimulator PRIVATE src)

add_executable(UnitTest "" test/unit_test.cpp)
add_dependencies(UnitTest nlohmann_json fmt)
target_link_libraries(UnitTest PUBLIC nlohmann_json fmt)
target_include_directories(UnitTest PRIVATE src)
target_include_directories(UnitTest PUBLIC packages/header-only)

add_executable(InstConvert "" test/inst_convert.cpp
        src/isa/inst_v1.cpp
        src/isa/inst_v1.h
        src/isa/inst_v2.cpp
        src/isa/inst_v2.h
        src/isa/inst_v3.cpp
        src/isa/inst_v3.h
        src/isa/isa.h
        src/isa/isa_v2.h

        test/base/test_payload.cpp
        test/base/test_payload.h)
add_dependencies(InstConvert nlohmann_json fmt)
target_link_libraries(InstConvert PUBLIC nlohmann_json fmt)
target_include_directories(InstConvert PRIVATE src)
target_include_directories(InstConvert PUBLIC packages/header-only)

add_executable(Test "" test/test.cpp)
