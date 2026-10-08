# ============================================================
# RV32 stack -- builds everything in one go
#
#   cmake --preset default              configure
#   cmake --build --preset default      build emulator, firmware, kernel, ISA tests
#   cmake --build --preset run          ... and start the emulator (rom.bin + disk.img)
#
# This file only orchestrates. Every part is its own CMake project that can
# be opened and built on its own (that's the normal way to work on it):
#
#   emulator/      host C++   -> build/emulator/rv32emu
#   sw/firmware/   RISC-V     -> build/firmware/rom.bin
#   sw/kernel/     RISC-V     -> build/kernel/kernel.bin + build/disk.img
#   tests/isa/     RISC-V     -> build/tests/isa/<name>.bin
#
# The sub-builds use the same build directories as the standalone presets,
# so it does not matter which way you build.
# ============================================================

cmake_minimum_required(VERSION 3.20)
project(rv32_stack NONE)

include(ExternalProject)

set(RV32_BUILD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/build")

function(rv32_part name source_dir binary_dir)
  ExternalProject_Add(${name}
    SOURCE_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}"
    BINARY_DIR       "${RV32_BUILD_ROOT}/${binary_dir}"
    CMAKE_GENERATOR  "${CMAKE_GENERATOR}"
    BUILD_COMMAND    "${CMAKE_COMMAND}" --build <BINARY_DIR> --verbose
    INSTALL_COMMAND  ""
    BUILD_ALWAYS     ON
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD     ON
    ${ARGN})
endfunction()

rv32_part(emulator  emulator     emulator)
rv32_part(firmware  sw/firmware  firmware)
rv32_part(kernel    sw/kernel    kernel)
rv32_part(isa_tests tests/isa    tests/isa)

add_custom_target(run
  COMMAND "${RV32_BUILD_ROOT}/emulator/rv32emu"
          "-firmware=${RV32_BUILD_ROOT}/firmware/rom.bin"
          "-harddisk=${RV32_BUILD_ROOT}/disk.img"
  DEPENDS emulator firmware kernel
  USES_TERMINAL
  VERBATIM)
