# Engacer OS

An independent, MIT-licensed operating system with its own BIOS/UEFI loaders,
kernel and native desktop. Convenience, readable graphics, customization and
bounded CPU/RAM use are product requirements.

**Current release: M10 / 0.10.0 — hardware phases 1 and 2**, qualified within
**M10-DEVICES-VM-v1**. [Release and checksums](docs/releases/m10-v0.10.0.md),
[27-gate acceptance](evidence/M10/acceptance-results.json),
[current status](docs/roadmap/status.md). This is a development OS qualified on
named virtual profiles; physical hardware and universal compatibility are not claimed.

**Current development: [M11 desktop and setup refresh](docs/milestones/m11.md).**
M11.0 defines the interaction and visual contract. M11.1 implements native
selection, double-click, context/focus and release-driven controls with
[14 native tasks and matching-source acceptance](evidence/M11/1/acceptance-results.json).
Later stages follow in order; the qualified download remains M10 until M11.6 passes.

## Try it in VMware

[Download the prepared VMware ZIP](https://github.com/enginsoysal/engacer-os/releases/download/m10-v0.10.0/EngacerOS-M10-VMware.zip) ·
[Standalone ISO](https://github.com/enginsoysal/engacer-os/releases/download/m10-v0.10.0/EngacerOS-M10.iso) ·
[Step-by-step installation](docs/handbook/install-m10.md) ·
[English documentation and downloads](https://engaceros.brixcanvas.com).

Extract the whole ZIP, open **EngacerOS.vmx**, and choose **I Copied It** if asked.
Start the VM, click the disk row inside the bordered list, follow Next, review
your choices and choose **Install Engacer OS**. Finish with **Restart to Engacer OS**.
The package includes a new blank growable 20-GiB SATA disk, two CPUs and 128 MiB RAM.
It performs a real installation; the disk is not preinstalled. No online account,
terminal command or guest driver installer is required. Keep Secure Boot off.
The matching ISO may remain attached. For an independent boot, disconnect only
CD/DVD and keep the same installed hard disk connected.

## What M10 delivers

- Automatic supported-device binding and common capability/status reporting in
  **Start → Settings → Devices**. Ready, Limited, Detected and Unavailable reflect
  actual providers; recognition alone is not usable functionality.
- Saved US QWERTY/Dvorak choices, qualified keyboard/mouse/wheel input, supported
  xHCI HID/hubs, disconnect and reconnect.
- **Files → USB drives**: browse supported FAT32 media, copy files to Downloads,
  choose **To USB** for native files, flush, safely remove and reconnect.
- Ordinary and actual 4K presentation, reachable partially off-screen windows,
  checked repaint, stable retained pages during movement and halted idle CPUs.
- New-artifact VMware installation, persistence and two detached boots on SATA,
  IDE, LSI Logic, LSI SAS, PVSCSI and NVMe under BIOS/UEFI, plus BusLogic BIOS.
- The exact prepared ZIP also passes native Devices/layout interaction and the
  original 7-Zip 26.06 x64 MSI install/run/archive/extract/remove/reinstall task.

[Device and USB tutorial](docs/handbook/hardware-m10.md) ·
[Hardware architecture: how and why](docs/architecture/hardware-m10.md) ·
[Standard applications](docs/handbook/apps-m74.md) ·
[Original Windows app guide](docs/handbook/windows-apps-m8.md).

## Boundaries and next work

USB writes require supported BOT media, 512-byte sectors, working flush and a
clean mirrored FAT32 volume. Copies create files without overwriting; each file
is bounded at 64 MiB. Directory growth, exFAT/NTFS writes, USB installation and
formatting are outside this slice. Failure can require an offline FAT check.
The release guide records the exact bounds and the unqualified 64-MiB OVMF
boot-attached USB allocation case. The ordinary VMware package uses 128 MiB.

Wi-Fi setup, comprehensive audio, vendor GPU 3D, multi-monitor support, laptops,
ARM/Raspberry Pi and Steam gaming remain [later hardware phases](docs/milestones/m10.md).
Original MS-DOS and Windows 1.x through Windows 11 compatibility remains a wider
program: only the named 7-Zip console/MSI profile is currently qualified.

## Build, customize and verify

Use a separate checkout to reproduce accepted releases. For new work choose a
distinct revision and preserve frozen accepted outputs.

```powershell
python tools/build/m10.py
python tools/release/vmware_starter10.py
# Run the complete frozen-artifact sequence in tests/m10/README.md.
python tests/m10/verify.py
```

[Host tools](tools/README.md) · [Qualification commands](tests/m10/README.md) ·
[Custom distributions](docs/handbook/distributions.md) ·
[Contributing](CONTRIBUTING.md) · [Architecture](docs/architecture/overview.md) ·
[OS folder structure](docs/architecture/filesystem.md).
Optional apps remain separate from required Files/Settings/services. A changed
profile needs its own qualification; the standard ISO's evidence does not
qualify an arbitrary custom build.

## Repository structure

```text
boot/                  Original BIOS/UEFI loaders
kernel/                Core, device, storage, input, network and desktop providers
include/engine/        Stable internal ABI and shared headers
runtime/, user/        Freestanding runtime and protected native/Windows workers
distros/               Validated profiles and independent data/app packages
tools/build/           Milestone builds and media authoring
tools/release/, tools/vm/ Release packages and owned guest tooling
tests/m*/              Per-milestone qualification harnesses
docs/                  Architecture, handbook, milestones, roadmap and releases
evidence/              Sanitized reports, native captures and accepted hashes
website/               Native BrixCanvas composition and GitHub article publisher
build/, dist/          Generated outputs, ignored by Git
tools/local/           Local pinned dependencies, ignored by Git
```

OS sources remain independent from BrixCanvas. The website uses editable native
BrixCanvas blocks and automatically synchronized English GitHub documentation.
The product and repository are Engacer OS / `enginsoysal/engacer-os`; internal
ABI identifiers and historical Engine OS releases remain stable for verification.
[Historical release index](docs/releases/CHANGELOG.md).

All current documentation, UI strings and comments are English. Behavior changes
update design, tests, evidence and user instructions together and are pushed to
[GitHub](https://github.com/enginsoysal/engacer-os). Never publish credentials,
private host paths or local archives. Original code uses the [MIT license](LICENSE);
third-party components retain their licenses and notices. Guest RAM is not a
measurement of total kernel consumption, and a boot screen is not app compatibility.
