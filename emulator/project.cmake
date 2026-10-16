# Hand-Datei des Emulators. Wird am Ende der generierten CMakeLists.txt includiert.
# Alles, was das Projektmodell nicht kennt, steht hier.

# platform/ = Hardware-Vertrag mit der Software (memory_map.h, Geräteregister)
set(RV32_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/../platform")
target_include_directories(rv32emu PRIVATE "${RV32_PLATFORM}")

# Unit-Tests: eigenes Ziel, nicht Teil des normalen Builds.
#   bauen:   cmake --build build --target tests
#   starten: build/tests
# Linkt nur die Module unter Test, damit es auch baut, während CPU.cpp umgebaut wird.
add_executable(tests EXCLUDE_FROM_ALL
  tests/test_main.cpp
  tests/test_decode.cpp
  tests/test_cpu.cpp
  tests/test_uart.cpp
  src/DECODE.cpp
  src/CPU.cpp
  src/CPU_exec.cpp
  src/CPU_exec_fp.cpp
  src/CPU_csr.cpp
  src/BUS.cpp
  src/CONFIG.cpp
  src/UTILS.cpp
  src/devices/CLINT.cpp
  src/devices/UART.cpp
)
target_include_directories(tests PRIVATE include "${RV32_PLATFORM}")

# -std=c++20 statt gnu++20, wie vorher
set_target_properties(rv32emu tests PROPERTIES CXX_EXTENSIONS OFF)
