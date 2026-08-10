# ============================================================
# RV32 — RISC-V RV32 emulator
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



add_executable(main ${SOURCES})

# ============================================================
# Include paths
# ============================================================

target_include_directories(main PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(main PRIVATE "${CMAKE_SOURCE_DIR}/include/devices")


# ============================================================
# Tests — separate target, not part of the normal build.
#   build:  cmake --build build --target tests
#   run:    ./build/tests
# Links only the modules under test (decoder + utils), so it
# builds even while CPU.cpp is mid-rewrite.
# ============================================================

add_executable(tests EXCLUDE_FROM_ALL
  "${CMAKE_SOURCE_DIR}/tests/test_main.cpp"
  "${CMAKE_SOURCE_DIR}/tests/test_decode.cpp"
  "${CMAKE_SOURCE_DIR}/tests/test_cpu.cpp"
  "${CMAKE_SOURCE_DIR}/src/DECODE.cpp"
  "${CMAKE_SOURCE_DIR}/src/CPU.cpp"
  "${CMAKE_SOURCE_DIR}/src/BUS.cpp"
  "${CMAKE_SOURCE_DIR}/src/CONFIG.cpp"
  "${CMAKE_SOURCE_DIR}/src/UTILS.cpp"
)

target_include_directories(tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

# ============================================================
# Warnings
# ============================================================

