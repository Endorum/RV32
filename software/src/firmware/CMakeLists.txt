# ============================================================
# RV32 firmware -- the ROM image (runs at reset, M-mode)
#
# Built as part of software/ (RAD C++ or `cmake --preset default` there):
#   software/build/firmware/   rom.bin, rom.elf, dis/ ...
# Play in RAD C++ starts the emulator with rom.bin and software/build/disk.img.
#
# New file in boot/ core/ loader/ monitor/ -> just build, it is picked up.
# New folder -> add it to MODULES below.
#
# Sees: platform/ (hardware), src/common/ (abi, libk, drivers, arch) and
# itself. Never the kernel.
# ============================================================

rv32_image(rom
  LINKER_SCRIPT firmware.ld.S
  MODULES
    boot                                  # boot.S (_start), trap stack
    core                                  # main, trap dispatch, panic
    loader                                # finds + starts the kernel on the HD
    monitor                               # the shell
  LIBS                                    # shared code, linked as needed
    "${RV32_SW_COMMON}/libk"              # kprintf, mem*, str*
    "${RV32_SW_COMMON}/drivers/uart"
    "${RV32_SW_COMMON}/arch/rv32"         # csr.h, tf.h, trap.S (trap_entry)
  INCLUDES
    "${RV32_PLATFORM}"
    "${RV32_SW_COMMON}/abi"
)
