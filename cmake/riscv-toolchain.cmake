# ============================================================
# Cross toolchain for all RV32 software (firmware, kernel, tests)
#
# Homebrew installs the bare-metal RISC-V GNU toolchain as "riscv64-elf-*".
# The riscv-gnu-toolchain source build uses "riscv64-unknown-elf-*"; override
# with the environment variable before the first configure:
#   RISCV_PREFIX=riscv64-unknown-elf- cmake --preset default
# ============================================================

set(CMAKE_SYSTEM_NAME      Generic)   # bare metal, no OS
set(CMAKE_SYSTEM_PROCESSOR riscv32)

if(DEFINED ENV{RISCV_PREFIX})
  set(RISCV_PREFIX "$ENV{RISCV_PREFIX}")
else()
  set(RISCV_PREFIX "riscv64-elf-")
endif()

set(CMAKE_C_COMPILER   "${RISCV_PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${RISCV_PREFIX}gcc")
set(CMAKE_OBJCOPY      "${RISCV_PREFIX}objcopy")
set(CMAKE_OBJDUMP      "${RISCV_PREFIX}objdump")
set(CMAKE_SIZE         "${RISCV_PREFIX}size")

# CMake test-compiles a program at configure time; without a C runtime that
# link would fail, so only test-compile a static library.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Never pick up host headers/libraries.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
