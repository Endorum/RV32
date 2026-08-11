#!/bin/bash
#
# Run the official riscv-tests ISA suites (rv32ui-p-*, rv32mi-p-*) against
# the emulator and print a result table.
#
#   tests/run_riscv_tests.sh              # both suites
#   tests/run_riscv_tests.sh rv32ui       # one suite
#   RISCV_TESTS=/path/to/riscv-tests/isa tests/run_riscv_tests.sh
#
# Per test: objcopy ELF -> flat .bin, read the tohost symbol address via nm,
# run the emulator with the riscv-tests memory layout. The TOHOST device
# prints PASS/"FAIL test n" and exits; a wall-clock alarm catches hangs.

set -x

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMU="$ROOT/build/main"
ISA_DIR="${RISCV_TESTS:-$ROOT/../riscv-tests/isa}"
OBJCOPY="${RISCV_PREFIX:-riscv64-elf-}objcopy"
NM="${RISCV_PREFIX:-riscv64-elf-}nm"
TIMEOUT_S=10

[ -x "$EMU" ] || { echo "error: emulator not built: $EMU" >&2; exit 1; }
[ -d "$ISA_DIR" ] || { echo "error: riscv-tests not found: $ISA_DIR" >&2; exit 1; }

SUITES=("${@:-rv32ui rv32mi}")
# shellcheck disable=SC2068
read -ra SUITES <<< "${SUITES[@]}"

pass=0; fail=0; err=0
results=""

for suite in "${SUITES[@]}"; do
  for elf in "$ISA_DIR/$suite"-p-*; do
    case "$elf" in *.dump|*.bin) continue ;; esac
    name="$(basename "$elf")"

    "$OBJCOPY" -O binary "$elf" "$elf.bin"
    tohost="0x$("$NM" "$elf" | awk '$3=="tohost"{print $1}')"

    # perl alarm = portable timeout (macOS has no coreutils timeout)
    out="$(perl -e 'alarm shift @ARGV; exec @ARGV' "$TIMEOUT_S" \
      "$EMU" "-firmware=$elf.bin" -l -reset_vector=0x80000000 \
      -ram_start=0x80000000 "-tohost=$tohost" 2>&1)"
    code=$?

    if echo "$out" | grep -q "^PASS"; then
      status="PASS"; pass=$((pass+1))
    elif echo "$out" | grep -q "FAIL test"; then
      n="$(echo "$out" | grep -o 'FAIL test [0-9]*' | awk '{print $3}')"
      status="FAIL (test $n)"; fail=$((fail+1))
    elif [ $code -ge 128 ]; then
      status="TIMEOUT"; err=$((err+1))
    else
      reason="$(echo "$out" | grep -m1 -o 'ERROR: .*' || echo "exit $code")"
      status="ERROR ($reason)"; err=$((err+1))
    fi
    results+="$(printf '%-28s %s' "$name" "$status")"$'\n'
  done
done

echo "$results"
echo "----------------------------------------"
echo "PASS: $pass   FAIL: $fail   ERROR/TIMEOUT: $err"
[ $((fail+err)) -eq 0 ]
