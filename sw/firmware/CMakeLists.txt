# ============================================================
# RV32 firmware -- the ROM image (runs at reset, M-mode)
#
#   cmake --preset default            configure  -> rv32/build/firmware/
#   cmake --build --preset default    build      -> rom.bin, rom.elf, dis/ ...
#   cmake --build --preset run        build + start the emulator with
#                                     rom.bin and rv32/build/disk.img
#
# Sees: platform/ (hardware), sw/common/ (abi, libk, drivers, arch) and
# itself. Never the kernel.
# ============================================================

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/riscv-toolchain.cmake"
    CACHE FILEPATH "RISC-V cross toolchain")

project(rv32_firmware LANGUAGES C ASM)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/rv32-image.cmake")

rv32_image(rom
  LINKER_SCRIPT firmware.ld.S
  MODULES
    boot                                  # boot.S (_start), trap.S
    core                                  # main, trap dispatch, panic
    loader                                # finds + starts the kernel on the HD
    monitor                               # the shell
    "${RV32_SW_COMMON}/libk"
    "${RV32_SW_COMMON}/drivers/uart"
  INCLUDES
    "${RV32_PLATFORM}"
    "${RV32_SW_COMMON}/abi"
    "${RV32_SW_COMMON}/arch/rv32"
)

# Start the emulator with this ROM and the shared disk image.
add_custom_target(run
  COMMAND "${CMAKE_COMMAND}" -E echo "[run]   ${RV32_EMULATOR} -firmware=rom.bin -harddisk=${RV32_DISK_IMAGE}"
  COMMAND "${RV32_EMULATOR}" "-firmware=${CMAKE_CURRENT_BINARY_DIR}/rom.bin" "-harddisk=${RV32_DISK_IMAGE}"
  DEPENDS rom
  USES_TERMINAL
  VERBATIM)
