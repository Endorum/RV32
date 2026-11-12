#!/bin/bash
#
# Build the hard disk image: a real FAT12 volume whose reserved sectors carry
# the kernel the firmware boots.
#
#   mkimage.sh <kernel.bin> <disk.img> [files_dir]
#
# Disk layout (boot contract, see software/src/common/abi/info.md):
#
#   sector 0              FAT boot sector / BPB (written by mkfs.fat)
#   sector 1 = HD + 0x200 magic  "RVOS"  bytes 52 56 4F 53  (read_word -> 0x534F5652)
#             HD + 0x204  size of the kernel image in bytes, 32 bit little endian
#             HD + 0x208  kernel image  -> copied by the firmware to RAM_KERN_START
#   ...                   rest of the reserved sectors (kernel area)
#   after reserved        FAT, FAT copy, root directory, data -- normal FAT12
#
# The magic and the offset must match the firmware loader.
#
# The image is created ONCE. Later builds keep it and only rewrite the kernel
# area, so files the OS wrote onto the disk survive a rebuild.
#   RV32_FRESH_DISK=1      throw the image away and create a new, empty one
# Files from [files_dir] (default: software/disk/) are copied into the root
# directory on every build (host version wins). Needs mtools (brew install mtools).
#
set -euo pipefail

MAGIC="RVOS"
SECTOR=512
KERNEL_SECTOR=1        # header starts at sector 1 = byte 0x200
RESERVED_MIN=2050      # sector 0 + header + 1 MiB kernel (RAM_KERN_SIZE); mkfs may round up
DISK_KIB=16384         # 16 MiB, comfortably FAT12
LABEL="RV32HD"

[ $# -ge 2 ] && [ $# -le 3 ] || { echo "usage: $0 <kernel.bin> <disk.img> [files_dir]" >&2; exit 1; }
IN="$1"
OUT="$2"
FILES_DIR="${3:-$(cd "$(dirname "$0")/../.." && pwd)/disk}"
[ -f "$IN" ] || { echo "error: $IN not found" >&2; exit 1; }

command -v mkfs.fat >/dev/null || { echo "error: mkfs.fat not found (brew install dosfstools)" >&2; exit 1; }

# little endian u16 at byte offset $2 of file $1
u16() { od -An -tu2 -j "$2" -N2 "$1" | tr -d ' '; }

# ---- 1. FAT12 volume: create if missing / wrong shape / forced --------------
need_new=0
reason=""
if [ "${RV32_FRESH_DISK:-0}" = "1" ]; then
    need_new=1; reason="RV32_FRESH_DISK=1"
elif [ ! -f "$OUT" ]; then
    need_new=1; reason="no image yet"
elif [ "$(wc -c < "$OUT" | tr -d ' ')" != "$((DISK_KIB * 1024))" ]; then
    need_new=1; reason="wrong size (old raw image?)"
elif [ "$(u16 "$OUT" 11)" != "$SECTOR" ] || [ "$(u16 "$OUT" 14)" -lt "$RESERVED_MIN" ]; then
    need_new=1; reason="not our FAT12 layout"
fi

mkdir -p "$(dirname "$OUT")"
if [ "$need_new" = "1" ]; then
    echo "[img]   new FAT12 volume ($reason)"
    rm -f "$OUT"
    mkfs.fat -F 12 -S "$SECTOR" -R "$RESERVED_MIN" -n "$LABEL" -C "$OUT" "$DISK_KIB" >/dev/null
else
    echo "[img]   keeping existing FAT12 volume, only the kernel area is rewritten"
fi

RESERVED=$(u16 "$OUT" 14)   # what mkfs.fat actually chose (rounded to cluster alignment)

# ---- 2. kernel into the reserved sectors --------------------------------
SIZE=$(wc -c < "$IN" | tr -d ' ')
ROOM=$(( (RESERVED - KERNEL_SECTOR) * SECTOR - 8 ))
if [ "$SIZE" -gt "$ROOM" ]; then
    echo "error: kernel ($SIZE bytes) does not fit into the reserved sectors ($ROOM bytes)" >&2
    exit 1
fi

# clear the whole kernel area first, so a shrinking kernel leaves no stale bytes
dd if=/dev/zero of="$OUT" bs="$SECTOR" seek="$KERNEL_SECTOR" count="$((RESERVED - KERNEL_SECTOR))" conv=notrunc status=none
{
    printf '%s' "$MAGIC"
    perl -e 'print pack("V", shift)' "$SIZE"   # V = 32 bit little endian
    cat "$IN"
} | dd of="$OUT" bs="$SECTOR" seek="$KERNEL_SECTOR" conv=notrunc status=none

printf '        sector 0        BPB, %d reserved sectors\n' "$RESERVED"
printf '        HD + 0x%03X     magic  %s\n' $((KERNEL_SECTOR * SECTOR))     "$MAGIC"
printf '        HD + 0x%03X     size   %d bytes (0x%X), room %d bytes\n' $((KERNEL_SECTOR * SECTOR + 4)) "$SIZE" "$SIZE" "$ROOM"
printf '        HD + 0x%03X     kernel image\n' $((KERNEL_SECTOR * SECTOR + 8))

# ---- 3. files from the host into the root directory ---------------------
if [ -d "$FILES_DIR" ] && [ -n "$(ls -A "$FILES_DIR" 2>/dev/null)" ]; then
    if command -v mcopy >/dev/null; then
        echo "[img]   files: $FILES_DIR -> ::/"
        MTOOLS_SKIP_CHECK=1 mcopy -i "$OUT" -o -s -Q "$FILES_DIR"/* ::/
    else
        echo "warning: $FILES_DIR has files but mcopy is missing (brew install mtools) -- skipped" >&2
    fi
fi

# ---- 4. sanity: the kernel write must not have touched the file system -----
if command -v fsck.fat >/dev/null; then
    fsck.fat -n "$OUT" >/dev/null || { echo "error: fsck.fat reports a damaged volume" >&2; exit 1; }
fi

echo "OK      $(wc -c < "$OUT" | tr -d ' ') bytes -> $OUT"
