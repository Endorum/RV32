# Manual CMake additions for rv32emu. Included at the end of the generated CMakeLists.txt.
# Compiler flags, linker scripts, anything RAD C++ does not know about goes here.

# platform/ = hardware contract with the software (memory_map.h, device registers), shared with software/
target_include_directories(rv32emu PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

# emulator core: -std=c++23 instead of gnu++23
set_target_properties(rv32emu PROPERTIES CXX_EXTENSIONS OFF)
