# ============================================================
# RV32 — RISC-V RV32 emulator (the "hardware")
#
#   cmake --preset default                      configure -> rv32/build/emulator/
#   cmake --build --preset default              build     -> rv32emu
#   cmake --build --preset default -t tests     unit tests -> tests
#
# The memory map is shared with the software: platform/memory_map.h
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(rv32 LANGUAGES CXX)

# ============================================================
# Standard
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clangd / nvim can resolve includes.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Sources
# ============================================================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")



add_executable(rv32emu ${SOURCES})

# ============================================================
# Include paths
# ============================================================

# platform/ = hardware contract shared with the software (memory_map.h)
set(RV32_PLATFORM "${CMAKE_SOURCE_DIR}/../platform")

target_include_directories(rv32emu PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(rv32emu PRIVATE "${CMAKE_SOURCE_DIR}/include/devices")
target_include_directories(rv32emu PRIVATE "${RV32_PLATFORM}")


# ============================================================
# Tests — separate target, not part of the normal build.
#   build:  cmake --build --preset default --target tests
#   run:    ../build/emulator/tests
# Links only the modules under test (decoder + utils), so it
# builds even while CPU.cpp is mid-rewrite.
# ============================================================

add_executable(tests EXCLUDE_FROM_ALL
  "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
  "${CMAKE_SOURCE_DIR}/tests/test_decode.cpp"
  "${CMAKE_SOURCE_DIR}/tests/test_cpu.cpp"
  "${CMAKE_SOURCE_DIR}/tests/test_uart.cpp"
  "${CMAKE_SOURCE_DIR}/src/DECODE.cpp"
  "${CMAKE_SOURCE_DIR}/src/CPU.cpp"
  "${CMAKE_SOURCE_DIR}/src/CPU_exec.cpp"
  "${CMAKE_SOURCE_DIR}/src/CPU_exec_fp.cpp"
  "${CMAKE_SOURCE_DIR}/src/CPU_csr.cpp"
  "${CMAKE_SOURCE_DIR}/src/BUS.cpp"
  "${CMAKE_SOURCE_DIR}/src/CONFIG.cpp"
  "${CMAKE_SOURCE_DIR}/src/UTILS.cpp"
  "${CMAKE_SOURCE_DIR}/src/devices/CLINT.cpp"
  "${CMAKE_SOURCE_DIR}/src/devices/UART.cpp"
)

target_include_directories(tests PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(tests PRIVATE "${CMAKE_SOURCE_DIR}/include/devices")
target_include_directories(tests PRIVATE "${RV32_PLATFORM}")

# ============================================================
# Warnings
# ============================================================

