#!/bin/sh
# aethel-install — install Aethel Linux to disk
# Usage: aethel-install /dev/sdX

set -e

DISK="$1"
MOUNT="/mnt"

if [ -z "$DISK" ]; then
    echo "usage: aethel-install /dev/sdX"
    fdisk -l 2>/dev/null | grep '^Disk /dev/' || true
    exit 1
fi

if [ ! -b "$DISK" ]; then
    echo "aethel-install: $DISK is not a block device"
    exit 1
fi

if [ "$(id -u)" != "0" ]; then
    echo "aethel-install: must be run as root"
    exit 1
fi

SIZE=$(fdisk -l "$DISK" 2>/dev/null | head -1 | sed 's/.*: //')
echo ""
echo "=== Aethel Linux Installer ==="
echo ""
echo "Disk:    $DISK"
echo "Size:    $SIZE"
echo ""
echo "WARNING: this will ERASE everything on $DISK"
printf "Continue? [y/N] "
read ANS
case "$ANS" in
    y|Y) ;;
    *) echo "Aborted."; exit 1 ;;
esac

umount "$MOUNT" 2>/dev/null || true
umount "${DISK}1" 2>/dev/null || true

echo ""
echo "[1/7] Partitioning $DISK..."
printf 'o\nn\np\n1\n\n\na\n1\nw\n' | fdisk "$DISK" >/dev/null 2>&1 || true
sleep 2

PART="${DISK}1"
if [ ! -b "$PART" ]; then
    echo "aethel-install: partition $PART not created"
    exit 1
fi

echo "[2/7] Formatting $PART as ext2..."
mkfs.ext2 -F "$PART" >/dev/null

echo "[3/7] Mounting..."
mkdir -p "$MOUNT"
mount "$PART" "$MOUNT"

mkdir -p "$MOUNT/proc"
mkdir -p "$MOUNT/sys"
mkdir -p "$MOUNT/dev"
mkdir -p "$MOUNT/run"
mkdir -p "$MOUNT/mnt"
mkdir -p "$MOUNT/tmp"
mkdir -p "$MOUNT/boot"
mkdir -p "$MOUNT/var"
mkdir -p "$MOUNT/var/log"
mkdir -p "$MOUNT/var/www"
mkdir -p "$MOUNT/home"
mkdir -p "$MOUNT/root"

echo "[4/7] Copying system..."
for d in bin sbin usr etc var lib lib64; do
    if [ -e "/$d" ]; then
        cp -a "/$d" "$MOUNT/" 2>/dev/null || true
    fi
done

if [ -f /init ]; then
    cp /init "$MOUNT/init"
    chmod +x "$MOUNT/init"
fi

mkdir -p "$MOUNT/root"
mkdir -p "$MOUNT/home"

echo "[5/7] Installing kernel and bootloader..."
cp /boot/vmlinuz             "$MOUNT/boot/vmlinuz"
cp /boot/initramfs-boot.gz   "$MOUNT/boot/initramfs.gz"
cp /boot/ldlinux.c32         "$MOUNT/boot/"
cp /boot/mbr.bin             "$MOUNT/boot/"

/usr/bin/extlinux --install "$MOUNT/boot"
dd if="$MOUNT/boot/mbr.bin" of="$DISK" bs=440 count=1 conv=notrunc 2>/dev/null || true

echo "[6/7] Writing configuration..."

# --- fstab (by device path, no UUID) ---
cat > "$MOUNT/etc/fstab" << FSTABEOF
# Aethel Linux fstab
$PART  /  ext2  defaults  0  1
FSTABEOF

# --- syslinux.cfg (by device path) ---
cat > "$MOUNT/boot/syslinux.cfg" << SYSEOF
DEFAULT aethel
PROMPT 0
TIMEOUT 50

LABEL aethel
    LINUX /boot/vmlinuz
    APPEND initrd=/boot/initramfs.gz root=$PART quiet
SYSEOF

if [ ! -f "$MOUNT/etc/hostname" ]; then
    echo "aethel" > "$MOUNT/etc/hostname"
fi

echo ""
echo "Set root password:"
printf "Password: "
stty -echo
read PW
stty echo
echo ""
printf "Repeat:   "
stty -echo
read PW2
stty echo
echo ""

if [ "$PW" = "$PW2" ] && [ -n "$PW" ]; then
    HASH=$(echo "$PW" | cryptpw -m sha-512)
    echo "root:$HASH:20000:0:99999:7:::" > "$MOUNT/etc/shadow"
    chmod 600 "$MOUNT/etc/shadow"
    echo "Password set."
else
    echo "Passwords don't match — root will have no password."
    echo "root::20000:0:99999:7:::" > "$MOUNT/etc/shadow"
    chmod 600 "$MOUNT/etc/shadow"
fi

echo ""
echo "[7/7] Syncing..."
sync
sleep 2
umount "$MOUNT" || umount -l "$MOUNT"

echo ""
echo "=== Installation complete ==="
echo ""
echo "Remove the ISO and reboot."
echo "Then login as: root"
echo ""