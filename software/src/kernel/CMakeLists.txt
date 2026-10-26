# ============================================================
# RV32 kernel -- loaded by the firmware from the HD into RAM_KERN_START
#
# Built as part of software/ (RAD C++ or `cmake --preset default` there):
#   software/build/kernel/   kernel.bin, kernel.elf, dis/ ...
#   software/build/disk.img  header + kernel.bin, what the firmware boots
#
# Every subfolder of src/kernel/ is a module: its *.c / *.S are compiled and
# it is an include path. Add e.g. src/kernel/boot/ with a _start in
# .section .text.init and it builds. As long as there are no sources the
# image is skipped.
#
# Sees: platform/ (hardware), src/common/ (abi, libk, drivers, arch) and
# itself. Never the firmware.
# ============================================================

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
  message(STATUS "  -> no module folders in src/kernel/ yet, nothing to build")
  add_custom_target(kernel_empty ALL
    COMMAND "${CMAKE_COMMAND}" -E echo "[kernel] no sources yet -- disk.img not touched")
  return()
endif()

rv32_image(kernel
  LINKER_SCRIPT kernel.ld.S
  MODULES
    ${kernel_modules}
  LIBS                                    # shared code, linked as needed
    "${RV32_SW_COMMON}/libk"              # kprintf, mem*, str*
    "${RV32_SW_COMMON}/drivers/uart"
    "${RV32_SW_COMMON}/arch/rv32"         # csr.h, tf.h, trap.S (trap_entry)
  INCLUDES
    "${RV32_PLATFORM}"
    "${RV32_SW_COMMON}/abi"
)

# Pack kernel.bin into the disk image the firmware boots from.
if(TARGET kernel)
  add_custom_command(
    OUTPUT  "${RV32_DISK_IMAGE}"
    COMMAND "${RV32_MKIMAGE}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin" "${RV32_DISK_IMAGE}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin" "${RV32_MKIMAGE}"
    COMMENT "[img]   kernel.bin -> disk.img"
    VERBATIM)
  add_custom_target(disk ALL DEPENDS "${RV32_DISK_IMAGE}")
endif()
