# Manual CMake additions for rv32emu-qt. Included at the end of the generated CMakeLists.txt.
# Compiler flags, linker scripts, anything RAD C++ does not know about goes here.

# platform/ = hardware contract with the software (memory_map.h, device registers), shared with emulator/
target_include_directories(rv32emu-qt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

# emulator core: -std=c++23 instead of gnu++23, like emulator/
set_target_properties(rv32emu-qt PROPERTIES CXX_EXTENSIONS OFF)
