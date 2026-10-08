# ============================================================
# ISA tests -- one bare ROM image per instruction group, they test the CPU
# (emulator), not the OS.
#
#   cmake --preset default            configure  -> rv32/build/tests/isa/
#   cmake --build --preset default    build      -> <name>.bin, <name>.elf, dis/ ...
#
# Every subfolder is one test (e.g. alu_imm/test_alu_imm.S). Run one with
#   rv32/build/emulator/rv32emu -firmware=rv32/build/tests/isa/<name>.bin -d
#
# Result convention (registers at 'halt: j halt'):
#   x31 = 0x600DC0DE pass  /  0x0BADC0DE fail
#   x28 = check number, x29 = actual, x30 = expected
# ============================================================

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/riscv-toolchain.cmake"
    CACHE FILEPATH "RISC-V cross toolchain")

project(rv32_isa_tests LANGUAGES C ASM)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/rv32-image.cmake")

file(GLOB children LIST_DIRECTORIES true CONFIGURE_DEPENDS
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT children)
foreach(test IN LISTS children)
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${test}" AND NOT test MATCHES "^\\.")
    rv32_image(${test}
      LINKER_SCRIPT isa.ld.S
      MODULES  ${test}
      INCLUDES "${RV32_PLATFORM}")
  endif()
endforeach()
