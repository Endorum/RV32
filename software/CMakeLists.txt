# ============================================================
# RV32 software -- firmware + kernel, one RISC-V cross build
#
# RAD C++ project (software.radcpp). This file is written by hand, not
# generated: RAD C++ cannot set a cross toolchain before project(), so it
# leaves this file alone and just builds it.
#
#   RAD C++:   open software.radcpp, Play
#   terminal:  cmake --preset default && cmake --build --preset default
#              cmake --build --preset run       ... and start the emulator
#
# Outputs (software/build/, the same folder for RAD C++ and the presets):
#   firmware/rom.bin   kernel/kernel.bin   disk.img   compile_commands.json
#   software           start script, see below
# ============================================================

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/riscv-toolchain.cmake"
    CACHE FILEPATH "RISC-V cross toolchain")

project(rv32_software LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/rv32-image.cmake)

add_subdirectory(src/firmware firmware)
add_subdirectory(src/kernel   kernel)

# ---- Play ------------------------------------------------------------
# RAD C++ runs build/<project name> on Play. That is this script: it starts
# the emulator (emulator/, its own RAD C++ project) with the fresh ROM and
# disk image. Extra arguments ([run] args in software.radcpp) are passed on.
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/software"
  CONTENT "#!/bin/sh
EMU=\"${RV32_EMULATOR}\"
[ -x \"$EMU\" ] || { echo \"emulator not built: $EMU -- open emulator/rv32emu.radcpp in RAD C++ and build it\" >&2; exit 1; }
exec \"$EMU\" -firmware=\"${CMAKE_BINARY_DIR}/firmware/rom.bin\" -harddisk=\"${RV32_DISK_IMAGE}\" \"$@\"
"
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

add_custom_target(launcher ALL DEPENDS rom_bin disk)

add_custom_target(run
  COMMAND "${CMAKE_BINARY_DIR}/software"
  DEPENDS launcher
  USES_TERMINAL
  VERBATIM)
