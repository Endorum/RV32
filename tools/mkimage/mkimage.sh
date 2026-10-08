#!/bin/bash
#
# Pack a flat kernel binary into a bootable disk image.
#
#   mkimage.sh <kernel.bin> <disk.img>
#
# Disk layout (boot contract, see sw/common/abi/info.md):
#   HD + 0   magic  "RVOS"  bytes 52 56 4F 53  (read_word -> 0x534F5652)
#   HD + 4   size of the kernel image in bytes, 32 bit little endian
#   HD + 8   kernel image  -> copied by the firmware to RAM_KERN_START
#
# The magic must match the one the firmware loader checks.
#
set -euo pipefail

MAGIC="RVOS"

[ $# -eq 2 ] || { echo "usage: $0 <kernel.bin> <disk.img>" >&2; exit 1; }
IN="$1"
OUT="$2"
[ -f "$IN" ] || { echo "error: $IN not found" >&2; exit 1; }

SIZE=$(wc -c < "$IN" | tr -d ' ')

echo "[img]   $(basename "$IN") -> $OUT"
echo "        +0  magic  '$MAGIC'"
echo "        +4  size   $SIZE bytes ($(printf '0x%X' "$SIZE"))"
echo "        +8  image"

mkdir -p "$(dirname "$OUT")"
{
  printf '%s' "$MAGIC"
  perl -e 'print pack("V", shift)' "$SIZE"   # V = 32 bit little endian
  cat "$IN"
} > "$OUT"

echo "OK      $(wc -c < "$OUT" | tr -d ' ') bytes -> $OUT"
