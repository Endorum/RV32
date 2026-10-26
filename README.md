# RV32 stack

A RISC-V RV32 machine built from scratch: the emulator ("hardware") and the
software that runs on it (firmware, kernel). Two RAD C++ projects, one shared
hardware contract.

```
rv32/
├── platform/          hardware contract: memory_map.h, device registers
│                      used by the emulator AND the software
├── emulator/          the machine, Qt app       -> emulator/build/rv32emu
└── software/          RISC-V cross build        -> software/build/
    ├── cmake/         toolchain file + rv32_image()
    ├── tools/mkimage/ kernel.bin -> disk.img (magic + size header)
    └── src/
        ├── common/    shared by firmware and kernel, compiled into each:
        │              abi/ (firmware <-> kernel contract), arch/rv32/,
        │              drivers/uart/, libk/ (string, printf)
        ├── firmware/  ROM image, M-mode, monitor shell + kernel loader
        └── kernel/    loaded from the HD into RAM
```

## Who may see what

| part              | include paths                                   |
|-------------------|-------------------------------------------------|
| emulator          | platform                                        |
| software/firmware | platform, src/common/*, src/firmware/*          |
| software/kernel   | platform, src/common/*, src/kernel/*            |

Firmware and kernel never include each other -- they only share the
contracts in `platform/` and `src/common/abi/`. Each image links its own copy
of libk and the drivers. Headers live next to their sources.

## Working on it

Open both projects in RAD C++:

1. `emulator/rv32emu.radcpp` -- build it once (Play runs it with the
   software's `rom.bin` and `disk.img`).
2. `software/software.radcpp` -- Play builds firmware + kernel and starts the
   emulator with them (via the generated script `software/build/software`).
   Extra emulator flags go into `[run] args` in `software.radcpp`.

`software/CMakeLists.txt` is written by hand (RAD C++ cannot set a cross
toolchain), so RAD C++ shows a "foreign CMakeLists" note on every build.
That is expected.

From the terminal:

```sh
cd software
cmake --preset default
cmake --build --preset default   # rom.bin, kernel.bin, disk.img
cmake --build --preset run       # ... and start the emulator
```

## Emulator without a window

`-headless` runs the machine in the console, UART on stdout/stdin:

```sh
software/build/software -headless
emulator/build/rv32emu -headless -firmware=software/build/firmware/rom.bin \
                                 -harddisk=software/build/disk.img
```

`emulator/build/rv32emu` without arguments lists all options.

Toolchain: Homebrew `riscv64-elf-*`; override with `RISCV_PREFIX=...`.
