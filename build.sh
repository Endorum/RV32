#!/bin/bash
set -e
set -v

# Toolchain prefix. Homebrew installs the bare-metal RISC-V GNU toolchain as
# "riscv64-elf-*" (riscv64-elf-binutils / riscv64-elf-gcc). The official
# riscv-gnu-toolchain source build uses "riscv64-unknown-elf-*" instead —
# just change this one line if you use that.
PREFIX="${RISCV_PREFIX:-riscv64-elf-}"

CC="${PREFIX}gcc"
LD="${PREFIX}ld"
OBJCOPY="${PREFIX}objcopy"
OBJDUMP="${PREFIX}objdump"

COMMON_FLAGS="
-march=rv32i
-mabi=ilp32
-ffreestanding
-nostdlib
-nostdinc
-fno-builtin
-O0
-fno-omit-frame-pointer
-fno-inline
"

BUILD_DIR="."
mkdir -p $BUILD_DIR

############################################################
# 1. Compile C and Assembly as individual objects
############################################################

# $CC $COMMON_FLAGS -c main.c -o $BUILD_DIR/main.o

$CC $COMMON_FLAGS -c test.S -o $BUILD_DIR/test.o

############################################################
# 2. Link objects into final ELF
############################################################
# If you have a linker script (recommended), replace with: -T linker.ld
$LD -m elf32lriscv -nostdlib -o $BUILD_DIR/rom.elf $BUILD_DIR/test.o

############################################################
# 3. Convert ELF → binary
############################################################
$OBJCOPY -O binary $BUILD_DIR/rom.elf $BUILD_DIR/rom.bin

############################################################
# 4. Optional disassembly dump
############################################################
$OBJDUMP -D -S $BUILD_DIR/rom.elf > $BUILD_DIR/rom.asm


rm rom.elf
rm test.o
