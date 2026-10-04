#!/bin/sh
set -e
VERSION="0.2.0"
echo "==> Aethel Linux ${VERSION} build"
echo "  [1/5] Building init..."
cd "$HOME/aethel-build/src"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o "$HOME/initramfs/init" init.c
chmod +x "$HOME/initramfs/init"
echo "  [2/5] Building fastfetch..."
cd "$HOME/aethel-extras/fastfetch"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o fastfetch fastfetch.c
echo "  [3/5] Building gris..."
cd "$HOME/gris"
make clean >/dev/null
make CFLAGS="-std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static" >/dev/null
echo "  [4/5] Packing initramfs..."
cp "$HOME/aethel-extras/fastfetch/fastfetch" "$HOME/initramfs/usr/bin/"
cp "$HOME/gris/gris"                          "$HOME/initramfs/usr/bin/"
cd "$HOME/initramfs"
find . | cpio -o -H newc 2>/dev/null | gzip > "$HOME/initramfs.cpio.gz"
cp "$HOME/initramfs.cpio.gz" "$HOME/iso/boot/initramfs.gz"
echo "  [5/5] Building ISO..."
cd "$HOME"
xorriso -as mkisofs -o "$HOME/aethel-linux-${VERSION}.iso" \
    -b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -V "AETHEL_${VERSION}" "$HOME/iso" >/dev/null
echo ""
echo "==> Built: aethel-linux-${VERSION}.iso"
ls -lh "$HOME/aethel-linux-${VERSION}.iso"
