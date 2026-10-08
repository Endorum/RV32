#!/bin/bash
#
# Run the official riscv-tests ISA suites (or single tests) against the
# emulator and print a result table.
#
#   tests/run_riscv_tests.sh                     # all suites (RV32G_Zifencei)
#   tests/run_riscv_tests.sh rv32ui              # one suite
#   tests/run_riscv_tests.sh rv32mi-p-mcsr       # a single test
#   tests/run_riscv_tests.sh rv32ui rv32mi-p-csr # mix is fine
#   TRACE=1 tests/run_riscv_tests.sh rv32mi-p-mcsr   # run with -l, trace -> <elf>.log
#   RISCV_TESTS=/path/to/riscv-tests/isa tests/run_riscv_tests.sh
#
# Per test: objcopy ELF -> flat .bin, read the tohost symbol address via nm,
# run the emulator with the riscv-tests memory layout. The TOHOST device
# prints PASS/"FAIL test n" and exits; a wall-clock alarm catches hangs.
# On a non-PASS result the last lines of emulator output are shown.

# set -x

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMU="$ROOT/build/main"
ISA_DIR="${RISCV_TESTS:-$ROOT/../riscv-tests/isa}"
OBJCOPY="${RISCV_PREFIX:-riscv64-elf-}objcopy"
NM="${RISCV_PREFIX:-riscv64-elf-}nm"
TIMEOUT_S=10
TRACE="${TRACE:-}"

# Deliberately not implemented (decision 2026-08-14) — expected to fail:
#   breakpoint:        hardware debug triggers (tselect/tdata*) — the emulator
#                      has host-side breakpoints instead
#   instret_overflow:  64-bit minstret counting + overflow semantics
SKIP_LIST=" rv32mi-p-breakpoint rv32mi-p-instret_overflow "



[ -x "$EMU" ] || { echo "error: emulator not built: $EMU" >&2; exit 1; }
[ -d "$ISA_DIR" ] || { echo "error: riscv-tests not found: $ISA_DIR" >&2; exit 1; }

# resolve args (suites or single test names) into a list of ELF paths
elfs=()
for arg in "${@:-rv32ui rv32um rv32ua rv32uf rv32ud rv32mi}"; do
  # shellcheck disable=SC2086
  for word in $arg; do
    if [ -f "$ISA_DIR/$word" ]; then
      elfs+=("$ISA_DIR/$word")               # single test, e.g. rv32mi-p-mcsr
    else
      found=0
      for elf in "$ISA_DIR/$word"-p-*; do    # suite, e.g. rv32ui
        case "$elf" in *.dump|*.bin|*.log) continue ;; esac
        [ -f "$elf" ] && { elfs+=("$elf"); found=1; }
      done
      [ $found -eq 1 ] || { echo "error: no such test or suite: $word" >&2; exit 1; }
    fi
  done
done

pass=0; fail=0; err=0; skip=0

for elf in "${elfs[@]}"; do
  name="$(basename "$elf")"

  "$OBJCOPY" -O binary "$elf" "$elf.bin"
  tohost="0x$("$NM" "$elf" | awk '$3=="tohost"{print $1}')"

  trace_flag=()
  [ -n "$TRACE" ] && trace_flag=(-l)

  # perl alarm = portable timeout (macOS has no coreutils timeout)
  out="$(perl -e 'alarm shift @ARGV; exec @ARGV' "$TIMEOUT_S" \
    "$EMU" "-firmware=$elf.bin" "${trace_flag[@]+"${trace_flag[@]}"}" \
    -reset_vector=0x80000000 -ram_start=0x80000000 "-tohost=$tohost" 2>&1)"
  code=$?

  if [ -n "$TRACE" ]; then
    printf '%s\n' "$out" > "$elf.log"
  fi

  # classify first ...
  if echo "$out" | grep -q "^PASS"; then
    status="PASS"
  elif echo "$out" | grep -q "FAIL"; then
    # tolerate any FAIL wording ("FAIL test n", "TOHOST FAIL WITH CODE n", ...)
    n="$(echo "$out" | grep -m1 'FAIL' | grep -o '[0-9]*' | tail -1)"
    status="FAIL (test ${n:-?})"
  elif [ $code -eq 142 ]; then
    status="TIMEOUT"                        # 128+14: our SIGALRM watchdog
  elif [ $code -ge 128 ]; then
    status="CRASH (signal $((code-128)))"   # e.g. 133 = SIGTRAP = UB brk-pad
  else
    reason="$(echo "$out" | grep -m1 -o 'ERROR: .*' || echo "exit $code")"
    status="ERROR ($reason)"
  fi

  # ... then apply expected-fail: skipped features don't fail the run, but an
  # unexpected PASS is flagged so the skip list gets cleaned up
  case "$SKIP_LIST" in *" $name "*)
    if [ "$status" = "PASS" ]; then
      status="PASS (unexpected — remove from SKIP_LIST)"
    else
      status="SKIP (deliberately not implemented)"
    fi
  ;; esac

  # ... then count
  case "$status" in
    PASS*) pass=$((pass+1)) ;;
    SKIP*) skip=$((skip+1)) ;;
    FAIL*) fail=$((fail+1)) ;;
    *)     err=$((err+1))  ;;
  esac

  printf '%-28s %s\n' "$name" "$status"
  [ -n "$TRACE" ] && printf '%-28s trace: %s\n' "" "$elf.log"

  # for a failing single test, show the tail of the output right away
  if [ "${#elfs[@]}" -eq 1 ] && [ "${status%% *}" != "PASS" ] && [ "${status%% *}" != "SKIP" ]; then
    echo "--- last output lines:"
    printf '%s\n' "$out" | tail -8
  fi
done

echo "----------------------------------------"
echo "PASS: $pass   FAIL: $fail   ERROR/TIMEOUT: $err   SKIP: $skip"
[ $((fail+err)) -eq 0 ]
