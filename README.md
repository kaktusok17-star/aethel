# Aethel Linux

A minimal Linux distribution built from scratch.

**Version: 1.0.0**

## Philosophy

KISS. Everything is simple, readable, replaceable.
No systemd, no dbus, no python. Just the essentials.

## Components

| Component | Language | Notes |
|---|---|---|
| Kernel | Linux 6.6 | custom config, ~5 MB |
| Userland | BusyBox | ~400 commands in one binary |
| Init | C | PID 1, signal handling, boot services |
| Login | C | `/etc/shadow`, banner |
| Network | C | DHCP via `udhcpc` + own config |
| Services | Shell | `aethel service` start/stop/status |
| Installer | Shell | `aethel-install /dev/sda` |
| Package manager | C | `gris`, dependency resolver, sha256 |
| Compiler | tcc + musl | static C toolchain inside Aethel |
| HTTP client | static curl | HTTPS with CA certificates |

**ISO: ~26 MB**

## Package repository

    https://kaktusok17-star.github.io/aethel-repo/

Packages available:

- `fastfetch` — system information tool
- `musl` — C standard library (static)
- `tcc` — Tiny C Compiler
- `ncurses` — terminal UI library
- `nano` — text editor

## Features

- Custom PID 1: halt/poweroff/reboot/Ctrl+Alt+Del, boot services
- Multi-user login with `/etc/shadow` password verification
- DHCP networking (`aethel-net up`)
- Service manager with boot autostart (`aethel service`)
- **Disk installer** — `aethel-install /dev/sda`
- **Remote package repository** over HTTPS
- **C toolchain inside Aethel** — compile C programs with `tcc`
- Runs in QEMU and VirtualBox

## Build

Requires: `gcc`, `make`, `cpio`, `xorriso`, `syslinux`, `extlinux`, `libcrypt-dev`, `wget`.

    ./build.sh

Output: `aethel-linux-1.0.0.iso`

## Usage

Live mode:

    # aethel-net up
    # gris sync
    # gris search nano
    # gris install nano
    # nano /tmp/file.txt
    # poweroff

Compile C inside Aethel:

    # gris install tcc
    # echo 'int main(){return 0;}' > /tmp/h.c
    # tcc -static -o /tmp/h /tmp/h.c
    # /tmp/h

Install to disk:

    # aethel-install /dev/sda
    # poweroff
    (remove ISO, boot from disk)

## Roadmap

- [x] 0.1.0 — Kernel + BusyBox + bootable ISO
- [x] 0.2.0 — Custom init in C
- [x] 0.3.0 — Custom kernel config
- [x] 0.4.0 — Network manager
- [x] 0.5.0 — Login system
- [x] 0.6.0 — Service manager
- [x] 0.7.0 — Disk installer
- [x] 0.8.0 — HTTPS package repository
- [x] 0.9.0 — C toolchain (tcc + musl)
- [x] **1.0.0 — Stable release**
- [ ] 1.1.0 — More packages (tree, htop, git, lua)
- [ ] 2.0.0 — X11 base
- [ ] 2.2.0 — i3 window manager

## License

MIT