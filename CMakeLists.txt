
# project
cmake_minimum_required(VERSION 3.20)
#cmake_policy(SET CMP0091 NEW)
project(pim-sim LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Use C++ 17")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# dependency
add_subdirectory(packages/fmt)
add_subdirectory(packages/systemc)
add_subdirectory(packages/json)
add_subdirectory(packages/zlib)
add_subdirectory(packages/filesystem)

add_library(pim-simulator STATIC ""
        src/config/config.cpp
        src/config/config.h
        src/config/config_enum.cpp
        src/config/config_enum.h
        src/config/config_enum.cpp
        src/config/config_enum.h
        src/util/macro_scope.h
        src/memory/memory.cpp
        src/memory/memory.h
        src/core/payload/payload.h
        src/core/payload/payload_enum.h
        src/base_component/base_module.cpp
        src/base_component/base_module.h
        src/base_component/clock.cpp
        src/base_component/clock.h
        src/base_component/energy_counter.cpp
        src/base_component/energy_counter.h
        src/util/reporter.cpp
        src/util/reporter.h
        src/memory/ram.cpp
        src/memory/ram.h
        src/util/util.cpp
        src/util/util.h
        src/memory/reg_buffer.cpp
        src/memory/reg_buffer.h
        src/core/local_memory_unit/local_memory_unit.cpp
        src/core/local_memory_unit/local_memory_unit.h
        src/core/simd_unit/simd_unit.cpp
        src/core/simd_unit/simd_unit.h
        src/base_component/fsm.h
        src/base_component/submodule_socket.h
        src/base_component/memory_socket.cpp
        src/base_component/memory_socket.h
        src/core/payload/payload.cpp
        src/util/log.h
        src/core/transfer_unit/transfer_unit.cpp
        src/core/transfer_unit/transfer_unit.h
        src/isa/isa.h
        src/core/payload/payload_enum.cpp
        src/core/scalar_unit/scalar_unit.cpp
        src/core/scalar_unit/scalar_unit.h
        src/core/reg_unit/reg_unit.cpp
        src/core/reg_unit/reg_unit.h
        src/base_component/register.h
        src/base_component/reg_unit_socket.cpp
        src/base_component/reg_unit_socket.h
        src/core/pim_unit/macro.cpp
        src/core/pim_unit/macro.h
        src/core/pim_unit/pim_payload.h
        src/core/pim_unit/macro_group.cpp
        src/core/pim_unit/macro_group.h
        src/core/pim_unit/macro_group_controller.cpp
        src/core/pim_unit/macro_group_controller.h
        src/core/pim_unit/pim_compute_unit.cpp
        src/core/pim_unit/pim_compute_unit.h
        src/core/pim_unit/pim_control_unit.cpp
        src/core/pim_unit/pim_control_unit.h
        src/isa/instruction.h
        src/isa/instruction.cpp
        src/core/core.cpp
        src/core/core.h
        src/base_component/stall_handler.cpp
        src/base_component/stall_handler.h
        src/util/ins_stat.cpp
        src/util/ins_stat.h
        src/network/network.cpp
        src/network/network.h
        src/network/payload.h
        src/network/switch.cpp
        src/network/switch.h
        src/network/switch_socket.cpp
        src/network/switch_socket.h
        src/memory/memory_hardware.h
        src/memory/global_memory.cpp
        src/memory/global_memory.h
        src/chip/chip.cpp
        src/chip/chip.h
        src/util/log.cpp
        src/core/pim_unit/cim_unit.cpp
        src/core/pim_unit/cim_unit.h
)
set_target_properties(pim-simulator PROPERTIES OUTPUT_NAME "pim-simulator")
target_include_directories(pim-simulator PRIVATE
        src
)

# header-only dependency
target_include_directories(pim-simulator PUBLIC packages/header-only)
target_include_directories(pim-simulator PUBLIC packages/header-only/zstr/src)

add_dependencies(pim-simulator systemc zlibstatic nlohmann_json fmt ghc_filesystem)

target_link_libraries(pim-simulator PUBLIC
        systemc
        zlibstatic
        nlohmann_json
        fmt
        ghc_filesystem
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

add_executable(LocalMemoryUnitTest "" test/other_test/local_memory_unit_test.cpp)
add_dependencies(LocalMemoryUnitTest pim-simulator)
target_link_libraries(LocalMemoryUnitTest PRIVATE pim-simulator)
target_include_directories(LocalMemoryUnitTest PRIVATE src)

add_executable(SIMDUnitTest "" test/execute_unit_test/simd_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(SIMDUnitTest pim-simulator)
target_link_libraries(SIMDUnitTest PRIVATE pim-simulator)
target_include_directories(SIMDUnitTest PRIVATE src)

add_executable(TransferUnitTest "" test/execute_unit_test/transfer_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(TransferUnitTest pim-simulator)
target_link_libraries(TransferUnitTest PRIVATE pim-simulator)
target_include_directories(TransferUnitTest PRIVATE src)

add_executable(MacroTest "" test/other_test/macro_test.cpp)
add_dependencies(MacroTest pim-simulator)
target_link_libraries(MacroTest PRIVATE pim-simulator)
target_include_directories(MacroTest PRIVATE src)

add_executable(MacroGroupTest "" test/other_test/macro_group_test.cpp)
add_dependencies(MacroGroupTest pim-simulator)
target_link_libraries(MacroGroupTest PRIVATE pim-simulator)
target_include_directories(MacroGroupTest PRIVATE src)

add_executable(PimComputeUnitTest "" test/execute_unit_test/pim_compute_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h)
add_dependencies(PimComputeUnitTest pim-simulator)
target_link_libraries(PimComputeUnitTest PRIVATE pim-simulator)
target_include_directories(PimComputeUnitTest PRIVATE src)

add_executable(PimControlUnitTest "" test/execute_unit_test/pim_control_unit_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h
        test/execute_unit_test/execute_unit_test.h
)
add_dependencies(PimControlUnitTest pim-simulator)
target_link_libraries(PimControlUnitTest PRIVATE pim-simulator)
target_include_directories(PimControlUnitTest PRIVATE src)

add_executable(CoreTest "" test/core_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h)
add_dependencies(CoreTest pim-simulator)
target_link_libraries(CoreTest PRIVATE pim-simulator)
target_include_directories(CoreTest PRIVATE src)

add_executable(ChipTest "" test/chip_test.cpp
        test/base/test_payload.cpp
        test/base/test_payload.h)
add_dependencies(ChipTest pim-simulator)
target_link_libraries(ChipTest PRIVATE pim-simulator)
target_include_directories(ChipTest PRIVATE src)

add_executable(LayerSimulator "" src/simulator/layer_simulator.cpp src/simulator/layer_simulator.h)
add_dependencies(LayerSimulator pim-simulator)
target_link_libraries(LayerSimulator PRIVATE pim-simulator)
target_include_directories(LayerSimulator PRIVATE src)

add_executable(NetworkSimulator "" src/simulator/network_simulator.cpp src/simulator/network_simulator.h)
add_dependencies(NetworkSimulator pim-simulator)
target_link_libraries(NetworkSimulator PRIVATE pim-simulator)
target_include_directories(NetworkSimulator PRIVATE src)

add_executable(TestWrap "" test/test_wrap.cpp)
add_dependencies(TestWrap nlohmann_json fmt)
target_link_libraries(TestWrap PUBLIC nlohmann_json fmt)
target_include_directories(TestWrap PRIVATE src)
target_include_directories(TestWrap PUBLIC packages/header-only)

add_executable(Test "" test/test.cpp)
