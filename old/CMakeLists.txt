# ============================================================
# RV32 — RISC-V RV32I emulator
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

# Mirror the old Makefile default (-O2 -g) when no build type is given.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# ============================================================
# Sources
# ============================================================

file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(main ${SOURCES})

# ============================================================
# Include paths
# ============================================================

target_include_directories(main PRIVATE "${CMAKE_SOURCE_DIR}/include")

# ============================================================
# Warnings
# ============================================================

target_compile_options(main PRIVATE -Wall -Wextra)
