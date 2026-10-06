#!/bin/sh
# build.sh — build Aethel Linux ISO
set -e

VERSION="1.0.0"

echo "==> Aethel Linux ${VERSION} build"

# 1. Init
echo "  [1/7] Building init..."
cd "$HOME/aethel-build/src"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o "$HOME/initramfs/init" init.c
chmod +x "$HOME/initramfs/init"

# 2. Gris
echo "  [2/7] Building gris..."
cd "$HOME/gris"
make clean >/dev/null
make CFLAGS="-std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static" >/dev/null
cp "$HOME/gris/gris" "$HOME/initramfs/usr/bin/gris"

# 3. Network
echo "  [3/7] Building aethel-net..."
cd "$HOME/aethel-extras/aethel-net"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o aethel-net aethel-net.c
cp "$HOME/aethel-extras/aethel-net/aethel-net" "$HOME/initramfs/usr/bin/aethel-net"

# 4. Login
echo "  [4/7] Building aethel-login..."
cd "$HOME/aethel-extras/aethel-login"
cc -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -static -o aethel-login aethel-login.c -lcrypt
cp "$HOME/aethel-extras/aethel-login/aethel-login" "$HOME/initramfs/usr/bin/aethel-login"

# 5. External tools
echo "  [5/7] Fetching external tools..."

# static curl (HTTPS)
if [ ! -f "$HOME/static-curl" ]; then
    wget -q -O "$HOME/static-curl" \
        https://github.com/moparisthebest/static-curl/releases/latest/download/curl-amd64
    chmod +x "$HOME/static-curl"
fi
cp "$HOME/static-curl" "$HOME/initramfs/usr/bin/curl"
chmod +x "$HOME/initramfs/usr/bin/curl"

# extlinux (from syslinux)
cp /usr/bin/extlinux "$HOME/initramfs/usr/bin/extlinux"
chmod +x "$HOME/initramfs/usr/bin/extlinux"

# glibc for extlinux
mkdir -p "$HOME/initramfs/lib/x86_64-linux-gnu"
mkdir -p "$HOME/initramfs/lib64"
cp /lib/x86_64-linux-gnu/libc.so.6 "$HOME/initramfs/lib/x86_64-linux-gnu/"
cp /lib64/ld-linux-x86-64.so.2    "$HOME/initramfs/lib64/"

# CA certificates for curl HTTPS
mkdir -p "$HOME/initramfs/etc/ssl/certs"
cp /etc/ssl/certs/ca-certificates.crt "$HOME/initramfs/etc/ssl/certs/"

# 6. Pack initramfs
echo "  [6/7] Packing initramfs..."
# su as copy of busybox (symlink won't work with setuid)
rm -f "$HOME/initramfs/bin/su"
cp "$HOME/initramfs/bin/busybox" "$HOME/initramfs/bin/su"
chmod 4755 "$HOME/initramfs/bin/su"

cd "$HOME/initramfs"
find . | cpio -o -H newc --owner=0:0 2>/dev/null | gzip > "$HOME/initramfs.cpio.gz"
cp "$HOME/initramfs.cpio.gz" "$HOME/iso/boot/initramfs.gz"

# 7. ISO
echo "  [7/7] Building ISO..."
cd "$HOME"
xorriso -as mkisofs -o "$HOME/aethel-linux-${VERSION}.iso" \
    -b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -V "AETHEL_${VERSION}" "$HOME/iso" >/dev/null

echo ""
echo "==> Built: aethel-linux-${VERSION}.iso"
ls -lh "$HOME/aethel-linux-${VERSION}.iso"