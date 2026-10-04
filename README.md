# Aethel Linux

A minimal Linux distribution built from scratch.

Version: **0.2.0**

## Components

- **Kernel** — Linux 6.6 (custom config, from kernel.org)
- **Userland** — BusyBox
- **Init** — custom C program (`src/init.c`)
- **Package manager** — `gris`
- **System info** — `fastfetch` (custom)

## Features

- Custom PID 1 with signal handling (halt/poweroff/reboot)
- Zombie process reaping
- Auto-restart of shell
- Ctrl+Alt+Del reboot
- Package manager with dependency resolution
- Bootable ISO ~15 MB

## Build

Requires: `gcc`, `make`, `cpio`, `xorriso`, `syslinux`.

    ./build.sh

Output: `aethel-linux-0.2.0.iso`

## Run

Boot the ISO in QEMU or VirtualBox.
