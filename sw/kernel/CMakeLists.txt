# ============================================================
# RV32 kernel -- loaded by the firmware from the HD into RAM_KERN_START
#
#   cmake --preset default            configure  -> rv32/build/kernel/
#   cmake --build --preset default    build      -> kernel.bin, kernel.elf, dis/ ...
#                                     and rv32/build/disk.img (header + kernel.bin)
#
# Every subfolder of sw/kernel/ is a module: its *.c / *.S are compiled and
# it is an include path. Add e.g. sw/kernel/boot/ with a _start in
# .section .text.init and it builds. As long as there are no sources the
# image is skipped.
#
# Sees: platform/ (hardware), sw/common/ (abi, libk, drivers, arch) and
# itself. Never the firmware.
# ============================================================

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/riscv-toolchain.cmake"
    CACHE FILEPATH "RISC-V cross toolchain")

project(rv32_kernel LANGUAGES C ASM)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/rv32-image.cmake")

# every non-hidden subfolder is a module
file(GLOB children LIST_DIRECTORIES true CONFIGURE_DEPENDS
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
set(kernel_modules "")
foreach(child IN LISTS children)
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}" AND NOT child MATCHES "^\\.")
    list(APPEND kernel_modules "${child}")
  endif()
endforeach()

if(NOT kernel_modules)
  message(STATUS "")
  message(STATUS "==== image 'kernel' ====")
  message(STATUS "  -> no module folders in sw/kernel/ yet, nothing to build")
  add_custom_target(kernel_empty ALL
    COMMAND "${CMAKE_COMMAND}" -E echo "[kernel] no sources yet -- disk.img not touched")
  return()
endif()

rv32_image(kernel
  LINKER_SCRIPT kernel.ld.S
  MODULES
    ${kernel_modules}
    "${RV32_SW_COMMON}/libk"
    "${RV32_SW_COMMON}/drivers/uart"
  INCLUDES
    "${RV32_PLATFORM}"
    "${RV32_SW_COMMON}/abi"
    "${RV32_SW_COMMON}/arch/rv32"
)

# Pack kernel.bin into the disk image the firmware boots from.
if(TARGET kernel)
  add_custom_command(TARGET kernel POST_BUILD
    COMMAND "${RV32_MKIMAGE}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin" "${RV32_DISK_IMAGE}"
    VERBATIM)
endif()
