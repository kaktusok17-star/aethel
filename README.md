# Aethel Linux

A minimal Linux distribution built from scratch.

**Version: 0.7.0**

## Philosophy

KISS. Everything is simple, readable, replaceable.
No systemd, no dbus, no python. Just the essentials.

## Components

| Component | Language | Size |
|---|---|---|
| **Kernel** | Linux 6.6 (custom config) | ~5 MB |
| **Userland** | BusyBox | ~2 MB |
| **Init** | C (src/init.c) | 800 lines |
| **Login** | C (extras/aethel-login) | 200 lines |
| **Network** | C (extras/aethel-net) | 250 lines |
| **Services** | Shell (aethel service) | 150 lines |
| **Installer** | Shell (extras/aethel-install) | 160 lines |
| **Package manager** | C (gris) | 1600 lines |

**Total ISO: ~26 MB**

## Features

- Custom PID 1 with signal handling (halt/poweroff/reboot/Ctrl+Alt+Del)
- Multi-user login with `/etc/shadow` password verification
- DHCP networking (`aethel-net up`)
- Service manager with boot autostart (`aethel service`)
- **Disk installer** (`aethel-install /dev/sda`)
- Package manager with dependency resolution (`gris`)
- Static initramfs, no external dependencies at runtime
- Runs in QEMU and VirtualBox

## Build

Requires: `gcc`, `make`, `cpio`, `xorriso`, `syslinux`, `extlinux`, `libcrypt-dev`.

    ./build.sh

Output: `aethel-linux-0.7.0.iso`

## Usage

Boot the ISO. On first boot, login as `root` (no password).

Live mode:

    # aethel-net up               # DHCP network
    # aethel service list         # show services
    # gris --help                 # package manager
    # poweroff                    # shutdown

Install to disk:

    # aethel-install /dev/sda     # wipe disk, install system
    # poweroff
    # (remove ISO, boot from disk)

## Roadmap

- [x] 0.1.0 — Kernel + BusyBox + bootable ISO
- [x] 0.2.0 — Custom init in C
- [x] 0.3.0 — Custom kernel config (5 MB)
- [x] 0.4.0 — Network manager (aethel-net)
- [x] 0.5.0 — Login system
- [x] 0.6.0 — Service manager
- [x] 0.7.0 — **Disk installer**
- [ ] 0.8.0 — HTTP client + package repository
- [ ] 0.9.0 — First 15 packages
- [ ] 1.0.0 — Stable release

## License

MIT