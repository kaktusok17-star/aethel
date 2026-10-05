# Aethel Linux

A minimal Linux distribution built from scratch.

**Version: 0.8.0**

## Philosophy

KISS. Everything is simple, readable, replaceable.
No systemd, no dbus, no python. Just the essentials.

## Components

| Component | Language | Notes |
|---|---|---|
| Kernel | Linux 6.6 | custom config, ~5 MB |
| Userland | BusyBox | ~400 commands in one binary |
| Init | C | PID 1, signal handling, boot services |
| Login | C | `/etc/shadow`, banner, empty password support |
| Network | C | DHCP via `udhcpc` + own config |
| Services | Shell | `aethel service` start/stop/status |
| Installer | Shell | `aethel-install /dev/sda` |
| Package manager | C | `gris`, dependency resolver, sha256 |
| HTTP client | static curl | HTTPS, CA certificates included |

**ISO: ~31 MB**

## Features

- Custom PID 1: halt/poweroff/reboot/Ctrl+Alt+Del, boot services
- Multi-user login with `/etc/shadow` password verification
- DHCP networking (`aethel-net up`)
- Service manager with boot autostart (`aethel service`)
- **Disk installer** — `aethel-install /dev/sda`
- Package manager with **remote repository** support
- **HTTPS package downloads** from GitHub Pages
- Runs in QEMU and VirtualBox

## Package repository

    https://kaktusok17-star.github.io/aethel-repo/

Packages: `fastfetch`, `musl`, `tcc`, and growing.

## Build

Requires: `gcc`, `make`, `cpio`, `xorriso`, `syslinux`, `extlinux`, `libcrypt-dev`, `wget`.

    ./build.sh

Output: `aethel-linux-0.8.0.iso`

## Usage

Live mode:

    # aethel-net up
    # gris sync
    # gris search fastfetch
    # gris install fastfetch
    # fastfetch
    # poweroff

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
- [x] 0.8.0 — **HTTPS package repository**
- [ ] 0.9.0 — C toolchain (tcc + musl) inside Aethel
- [ ] 1.0.0 — Stable release

## License

MIT