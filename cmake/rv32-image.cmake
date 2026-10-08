# ============================================================
# rv32_image() -- build one bootable RV32 image (firmware, kernel, test ROM)
#
# Every image is linked on its own: it gets its own copy of libk/drivers,
# its own linker script and only the include paths it lists. Two images never
# share code or symbols -- they only share the contracts in platform/ and
# sw/common/abi/.
#
#   rv32_image(<name>
#     LINKER_SCRIPT <file.ld.S>        preprocessed with cpp -> <name>.ld
#     MODULES  <dir>...                sources (*.c, *.S) compiled from these,
#                                      each dir is also an include path
#     INCLUDES <dir>...                header-only include paths
#   )
#
# Outputs, all in the project's build dir (rv32/build/<project>/):
#   <name>.elf            linked ELF (with debug info)
#   <name>.bin            flat binary, what the emulator / disk image gets
#   <name>.map            link map (where every symbol/section landed)
#   <name>.ld             the linker script after preprocessing
#   dis/<name>.dis        disassembly, source interleaved, ABI register names
#   dis/<name>.raw.dis    disassembly, numeric registers, no pseudo-instructions
#                         -> exactly the instruction stream the CPU sees
# ============================================================

include_guard(GLOBAL)

# ---- paths shared by every project -------------------------------------
get_filename_component(RV32_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(RV32_PLATFORM   "${RV32_ROOT}/platform")
set(RV32_SW_COMMON  "${RV32_ROOT}/sw/common")
set(RV32_BUILD_ROOT "${RV32_ROOT}/build")
set(RV32_EMULATOR   "${RV32_BUILD_ROOT}/emulator/rv32emu")
set(RV32_DISK_IMAGE "${RV32_BUILD_ROOT}/disk.img")
set(RV32_MKIMAGE    "${RV32_ROOT}/tools/mkimage/mkimage.sh")

# ---- compiler flags for all RV32 software ------------------------------
# -O0 + no-inline keeps the generated instruction stream close to the source,
# which is the whole point of the per-instruction disassembly.
set(RV32_ARCH_FLAGS -march=rv32g -mabi=ilp32)
set(RV32_C_FLAGS
  -ffreestanding
  -nostdlib
  -fno-builtin
  -O0
  -g
  -fno-omit-frame-pointer
  -fno-inline
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for VS Code / clangd

function(_rv32_rel out path)
  file(RELATIVE_PATH rel "${RV32_ROOT}" "${path}")
  set(${out} "${rel}" PARENT_SCOPE)
endfunction()

function(rv32_image NAME)
  cmake_parse_arguments(IMG "" "LINKER_SCRIPT" "MODULES;INCLUDES" ${ARGN})

  get_filename_component(IMG_LINKER_SCRIPT "${IMG_LINKER_SCRIPT}" ABSOLUTE)
  set(out_dir "${CMAKE_CURRENT_BINARY_DIR}")
  set(elf     "${out_dir}/${NAME}.elf")
  set(bin     "${out_dir}/${NAME}.bin")
  set(map     "${out_dir}/${NAME}.map")
  set(ld      "${out_dir}/${NAME}.ld")
  set(dis_dir "${out_dir}/dis")

  # ---- collect sources, printed per module so you see what goes in ----
  message(STATUS "")
  message(STATUS "==== image '${NAME}' ====")
  _rv32_rel(rel "${IMG_LINKER_SCRIPT}")
  message(STATUS "  linker script : ${rel}")

  set(srcs "")
  set(inc_dirs "")
  foreach(mod IN LISTS IMG_MODULES)
    get_filename_component(mod "${mod}" ABSOLUTE)
    file(GLOB_RECURSE mod_srcs CONFIGURE_DEPENDS "${mod}/*.c" "${mod}/*.S")
    list(SORT mod_srcs)
    list(APPEND srcs ${mod_srcs})
    list(APPEND inc_dirs "${mod}")

    set(names "")
    foreach(s IN LISTS mod_srcs)
      file(RELATIVE_PATH n "${mod}" "${s}")
      list(APPEND names "${n}")
    endforeach()
    list(JOIN names " " names)
    _rv32_rel(rel "${mod}")
    message(STATUS "  module        : ${rel}  [${names}]")
  endforeach()

  set(header_deps "")
  foreach(inc IN LISTS IMG_INCLUDES)
    get_filename_component(inc "${inc}" ABSOLUTE)
    list(APPEND inc_dirs "${inc}")
    file(GLOB inc_headers CONFIGURE_DEPENDS "${inc}/*.h")
    list(APPEND header_deps ${inc_headers})
    _rv32_rel(rel "${inc}")
    message(STATUS "  include       : ${rel}")
  endforeach()

  if(NOT srcs)
    message(STATUS "  -> no sources yet, image '${NAME}' is skipped")
    return()
  endif()
  message(STATUS "  output        : ${out_dir}/${NAME}.{elf,bin,map}")

  # ---- linker script: run it through the C preprocessor --------------
  # So it can #include memory_map.h / ram_layout.h instead of repeating the
  # addresses. -undef: no predefined macros (e.g. 'riscv' must stay a word).
  set(inc_flags "")
  foreach(inc IN LISTS inc_dirs)
    list(APPEND inc_flags "-I${inc}")
  endforeach()

  _rv32_rel(rel "${IMG_LINKER_SCRIPT}")
  add_custom_command(
    OUTPUT  "${ld}"
    COMMAND "${CMAKE_C_COMPILER}" -E -P -undef -x c ${inc_flags}
            "${IMG_LINKER_SCRIPT}" -o "${ld}"
    DEPENDS "${IMG_LINKER_SCRIPT}" ${header_deps}
    COMMENT "[ld.S]  ${rel} -> ${NAME}.ld"
    VERBATIM)
  add_custom_target(${NAME}_ldscript DEPENDS "${ld}")

  # ---- compile + link -------------------------------------------------
  add_executable(${NAME} ${srcs})
  set_target_properties(${NAME} PROPERTIES
    OUTPUT_NAME "${NAME}"
    SUFFIX      ".elf"
    LINK_DEPENDS "${ld}")
  add_dependencies(${NAME} ${NAME}_ldscript)

  target_include_directories(${NAME} PRIVATE ${inc_dirs})
  target_compile_options(${NAME} PRIVATE ${RV32_ARCH_FLAGS} ${RV32_C_FLAGS})
  target_link_options(${NAME} PRIVATE
    ${RV32_ARCH_FLAGS}
    -nostdlib
    -nostartfiles
    "-T${ld}"
    "-Wl,-Map=${map}")

  # ---- ELF -> binary, disassembly, size report ------------------------
  add_custom_command(TARGET ${NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${dis_dir}"

    COMMAND "${CMAKE_COMMAND}" -E echo "[bin]   ${NAME}.elf -> ${NAME}.bin"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "${elf}" "${bin}"

    COMMAND "${CMAKE_COMMAND}" -E echo "[dis]   dis/${NAME}.dis      (source + ABI names)"
    COMMAND sh -c "\"${CMAKE_OBJDUMP}\" -D -S \"${elf}\" > \"${dis_dir}/${NAME}.dis\""

    COMMAND "${CMAKE_COMMAND}" -E echo "[dis]   dis/${NAME}.raw.dis  (numeric, no aliases)"
    COMMAND sh -c "\"${CMAKE_OBJDUMP}\" -D -M numeric,no-aliases \"${elf}\" > \"${dis_dir}/${NAME}.raw.dis\""

    COMMAND "${CMAKE_COMMAND}" -E echo "[size]  sections of ${NAME}.elf:"
    COMMAND "${CMAKE_SIZE}" -A -x "${elf}"

    COMMAND sh -c "echo \"OK      $(wc -c < \"${bin}\" | tr -d ' ') bytes -> ${bin}\""
    BYPRODUCTS "${bin}" "${map}"
    VERBATIM)
endfunction()
