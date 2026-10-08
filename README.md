# RV32 stack

A RISC-V RV32 machine built from scratch: emulator ("hardware"), firmware,
kernel. Every part is its own CMake project — open the folder in VS Code
and work on just that part.

```
rv32/
├── platform/          hardware contract: memory_map.h (+ device registers)
│                      used by the emulator AND the software
├── emulator/          the machine (C++)            -> build/emulator/rv32emu
├── sw/
│   ├── common/        shared by all software images, compiled into each one
│   │   ├── abi/       firmware <-> kernel contract: ram_layout.h, boot image
│   │   ├── libk/      freestanding C library (string, printf)
│   │   ├── drivers/   device drivers (uart)
│   │   └── arch/rv32/ csr.h
│   ├── firmware/      ROM image, M-mode, monitor shell + kernel loader
│   │                                               -> build/firmware/rom.bin
│   └── kernel/        loaded from the HD into RAM  -> build/kernel/kernel.bin
│                                                      build/disk.img
├── tests/isa/         CPU instruction tests (bare ROMs) -> build/tests/isa/
├── tools/mkimage/     kernel.bin -> disk.img (magic + size header)
└── cmake/             toolchain file + rv32_image() shared by all sw projects
```

## Who may see what

| project       | include paths                                          |
|---------------|--------------------------------------------------------|
| emulator      | platform                                               |
| sw/firmware   | platform, sw/common/*, sw/firmware/*                   |
| sw/kernel     | platform, sw/common/*, sw/kernel/*                     |
| tests/isa     | platform                                               |

Firmware and kernel never include each other — they only share the
contracts in `platform/` and `sw/common/abi/`. Each image links its own copy
of libk and the drivers.

## Build

Each project (emulator, sw/firmware, sw/kernel, tests/isa):

```sh
cmake --preset default           # configure  -> rv32/build/<project>/
cmake --build --preset default   # build (verbose: every command is printed)
```

`sw/firmware` also has `cmake --build --preset run`: build and start the
emulator with `rom.bin` and `build/disk.img`.

Everything at once, from `rv32/`:

```sh
cmake --preset default
cmake --build --preset default   # emulator, firmware, kernel, ISA tests
cmake --build --preset run       # ... and run
```

Emulator checks: `cmake --build --preset default -t tests` in `emulator/`,
then `../build/emulator/tests`; official suite: `emulator/tests/run_riscv_tests.sh`
(expects riscv-tests in `../third_party/riscv-tests`).

Toolchain: Homebrew `riscv64-elf-*`; override with `RISCV_PREFIX=...`.
