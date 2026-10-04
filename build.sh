#!/bin/sh
# build.sh — build Aethel Linux ISO
set -e

VERSION="0.1.0"

echo "==> Aethel Linux ${VERSION} build"

echo "  [1/4] Building fastfetch..."
cd "$HOME/aethel-extras/fastfetch"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o fastfetch fastfetch.c

echo "  [2/4] Building gris..."
cd "$HOME/gris"
make clean >/dev/null
make CFLAGS="-std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static" >/dev/null

echo "  [3/4] Packing initramfs..."
cp "$HOME/aethel-extras/fastfetch/fastfetch" "$HOME/initramfs/usr/bin/"
cp "$HOME/gris/gris"                          "$HOME/initramfs/usr/bin/"
cd "$HOME/initramfs"
find . | cpio -o -H newc 2>/dev/null | gzip > "$HOME/initramfs.cpio.gz"
cp "$HOME/initramfs.cpio.gz" "$HOME/iso/boot/initramfs.gz"

echo "  [4/4] Building ISO..."
cd "$HOME"
xorriso -as mkisofs \
    -o "$HOME/aethel-linux-${VERSION}.iso" \
    -b boot/isolinux/isolinux.bin \
    -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -V "AETHEL_${VERSION}" \
    "$HOME/iso" >/dev/null

echo ""
echo "==> Built: aethel-linux-${VERSION}.iso"
ls -lh "$HOME/aethel-linux-${VERSION}.iso"
