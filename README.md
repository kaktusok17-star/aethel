# Aethel Linux

A minimal Linux distribution built from scratch.

## Components

- Kernel: Linux 6.6 (custom config)
- Userland: BusyBox
- Init: custom shell script
- Package manager: gris
- System info: fastfetch (custom)

## Build

Requires: gcc, make, cpio, xorriso, syslinux.

    ./build.sh

Output: aethel-linux-0.1.0.iso

## Run

Boot the ISO in QEMU or VirtualBox.
