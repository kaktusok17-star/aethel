#!/bin/sh
# build.sh — build Aethel Linux ISO
set -e

VERSION="0.7.0"

echo "==> Aethel Linux ${VERSION} build"

# 1. Init
echo "  [1/6] Building init..."
cd "$HOME/aethel-build/src"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o "$HOME/initramfs/init" init.c
chmod +x "$HOME/initramfs/init"

# 2. Gris
echo "  [2/6] Building gris..."
cd "$HOME/gris"
make clean >/dev/null
make CFLAGS="-std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static" >/dev/null

# 3. Network
echo "  [3/6] Building aethel-net..."
cd "$HOME/aethel-extras/aethel-net"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o aethel-net aethel-net.c

# 4. Login
echo "  [4/6] Building aethel-login..."
cd "$HOME/aethel-extras/aethel-login"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o aethel-login aethel-login.c -lcrypt

# 5. Pack initramfs
echo "  [5/6] Packing initramfs..."
cp "$HOME/gris/gris"                                "$HOME/initramfs/usr/bin/"
cp "$HOME/aethel-extras/aethel-net/aethel-net"      "$HOME/initramfs/usr/bin/"
cp "$HOME/aethel-extras/aethel-login/aethel-login"  "$HOME/initramfs/usr/bin/"

# su as copy of busybox (symlink won't work with setuid)
rm -f "$HOME/initramfs/bin/su"
cp "$HOME/initramfs/bin/busybox" "$HOME/initramfs/bin/su"
chmod 4755 "$HOME/initramfs/bin/su"

cd "$HOME/initramfs"
find . | cpio -o -H newc --owner=0:0 2>/dev/null | gzip > "$HOME/initramfs.cpio.gz"
cp "$HOME/initramfs.cpio.gz" "$HOME/iso/boot/initramfs.gz"

# 6. ISO
echo "  [6/6] Building ISO..."
cd "$HOME"
xorriso -as mkisofs -o "$HOME/aethel-linux-${VERSION}.iso" \
    -b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -V "AETHEL_${VERSION}" "$HOME/iso" >/dev/null

echo ""
echo "==> Built: aethel-linux-${VERSION}.iso"
ls -lh "$HOME/aethel-linux-${VERSION}.iso"