# Aethel Linux

A minimal Linux distribution built from scratch.

Version: **0.5.0**

## Components

- **Kernel** — Linux 6.6 (custom config, from kernel.org)
- **Userland** — BusyBox
- **Init** — custom C program with signal handling (`src/init.c`)
- **Login** — custom `aethel-login` with `/etc/shadow` support
- **First-boot setup** — `aethel-setup` script
- **Network** — `aethel-net` DHCP manager
- **Package manager** — `gris`
- **System info** — `fastfetch` (custom)

## Features

- Custom PID 1: halt/poweroff/reboot, Ctrl+Alt+Del
- Multi-user: login with password verification
- First-boot wizard: hostname, root password, user creation
- DHCP networking with one config file
- Package manager with dependency resolution
- Bootable ISO ~14 MB

## Build

Requires: `gcc`, `make`, `cpio`, `xorriso`, `syslinux`, `libcrypt-dev`.

    ./build.sh

Output: `aethel-linux-0.5.0.iso`

## Run

Boot the ISO in QEMU or VirtualBox.
