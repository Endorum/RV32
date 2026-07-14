#!/bin/bash
set -e
set -v

CC="riscv64-unknown-elf-gcc"
LD="riscv64-unknown-elf-ld"
OBJCOPY="riscv64-unknown-elf-objcopy"
OBJDUMP="riscv64-unknown-elf-objdump"

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
