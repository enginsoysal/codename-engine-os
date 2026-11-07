# Codename: Engine OS

An independent, MIT-licensed operating system with its own UEFI loader, kernel and native user runtime. MS-DOS and Windows 1.x–11 application compatibility is a long-term requirement, followed by Linux/macOS execution environments and ARM64. Those compatibility layers are not implemented yet.

**Visual refresh in progress: M7.3.** Native antialiased text, HiDPI sizing and a five-question usability review are being qualified. [Scope](docs/milestones/m7.3.md), [graphics decisions](docs/architecture/graphics-m73.md). M7.2 remains the accepted release during qualification.

**Current completed follow-up: M7.2 — desktop-first installation and real folder navigation.** [Installer](docs/handbook/install-m72.md), [desktop controls](docs/handbook/desktop-m72.md), [scope](docs/milestones/m7.2.md), [exact acceptance](evidence/M7.2/acceptance-results.json). Named virtual profiles only; physical hardware and factory Secure Boot remain unqualified.

The historical [M5 release](https://github.com/enginsoysal/codename-engine-os/releases/tag/m5-v0.5.0) provides BIOS/UEFI live media, a native text installer for a new SATA disk, media-detached installed boot, persistent native files and same-build boot recovery. There is no desktop, network stack or existing Windows app compatibility yet. That historical native data partition is 16 MiB with 48-KiB files; other disk capacity remains unallocated.

## Next product milestones

**M6: real installation → M7: real desktop → M8: MS-DOS and Windows 1.x–11 applications.** M6 delivers a native graphical wizard, capacity-aware data volume, persistent local setup and tested ISO-assisted update, rollback and repair. M7 and M7.1 retain their historical named-scope acceptance. M7.2 prioritizes installation and a visible folder desktop; M8 is not started. [Desktop implementation status](docs/milestones/m7.md). [M6 implementation status](docs/milestones/m6.md). Existing technical acceptance remains historical; M5.1 does not satisfy the new complete product installation/desktop requirements. [Acceptance gates and choices](docs/roadmap/milestones.md) · [Current status](docs/roadmap/status.md). The product name remains **Engine OS**.

## Install and open your desktop

[Download the prepared VMware VM](https://github.com/enginsoysal/codename-engine-os/releases/download/m72-v0.7.2/EngineOS-M72-VMware.zip) · [Standalone ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m72-v0.7.2/EngineOS-M72.iso) · [Step-by-step guide](docs/handbook/install-m72.md) · [Release and checksum](docs/releases/m72-v0.7.2.md).

Extract the entire ZIP, open EngineOS.vmx and start. It includes a **new empty** growable 20-GiB SATA disk and the installer ISO. Follow Next, review and Install, then choose **Open desktop**. Files opens your real saved home folder immediately. Browse Documents, create a named folder, use Back/Forward/Up and customize appearance. No network, terminal or optional apps are needed. For future disk boots, disconnect only the CD/DVD ISO and keep the SATA hard disk connected. Existing M7.1 installations can use Recovery/Update to preserve data. [Folder guide](docs/handbook/desktop-m72.md).

The UEFI loader registers an Engine OS fallback startup option while firmware services are active; writable firmware variable storage is required. The entry can remain after cancelled installation. The immediate desktop uses the installation kernel; separate unforced same-VM/NVRAM tests qualify actual detached disk startup. [Boot and UI decisions](docs/architecture/desktop-m72.md). DOS and Windows compatibility remain later work.

## Historical M7.1 installation


[Download the M7.1 ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m71-v0.7.1/EngineOS-M71.iso) · [VMware tutorial](docs/handbook/install-m71.md) · [Disk manager](docs/handbook/disk-manager-m71.md) · [Release and checksum](docs/releases/m71-v0.7.1.md). For an existing disk you intend to replace: select it, enable Erase and install, follow Next and explicitly consent at final review. Manage disks handles individual data partitions while keeping others. OS installation still replaces the whole disk. The real desktop appears after powering off, detaching the ISO and booting SATA.

## Historical M7 installation

[Download the M7 ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m7-v0.7.0/EngineOS-M7.iso) · [VMware tutorial](docs/handbook/install-m7.md) · [Desktop controls](docs/handbook/desktop-m7.md) · [Release/checksum](docs/releases/m7-v0.7.0.md). Follow the native wizard on a new SATA disk, power off, detach the ISO and boot into the real desktop. Use two CPUs, 128 MiB, Secure Boot off and e1000e NAT for downloads. Files, Editor, Settings, optional Calculator and HTTPS downloads work without terminal commands. Original DOS/Windows application execution remains M8.

## Historical M6 installation

[Native installation handbook](docs/handbook/install-m6.md) · [Implementation and qualification](docs/milestones/m6.md) · [Storage/recovery design](docs/architecture/storage-m6.md). The native implementation follows the accepted wizard design; it is not the HTML preview. [Download the M6 ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m6-v0.6.0/EngineOS-M6.iso). Create a new SATA disk of at least 2 GiB, use two CPUs and 128 MiB RAM, disable Secure Boot, then follow Next, review and Install. After Finish, power off, detach the ISO and boot the installed disk. No manual partition commands are required.

The data partition uses the remaining aligned disk capacity after boot/system partitions. Files stream in 4-KiB buffers; a file over 2 MiB survives detached boots. Keyboard, USB mouse and PS/2 mouse are supported on the named profiles. [Recovery/update guide](docs/handbook/recovery-m6.md) · [Release notes and exact limits](docs/releases/m6-v0.6.0.md).

## Historical M5.1 guided setup

[Guided VMware installation](docs/handbook/install-m51.md) · [Build your own distribution](docs/handbook/distributions.md) · [Product principles](docs/architecture/product-principles.md) · [M5.1 contract](docs/milestones/m5.1.md).

The historical M5.1 welcome menu offers Try, Install, Repair, Advanced, Details and distribution information. Ordinary installation/repair requires displayed choices only, with real disk capacities, review/cancel and progress. Profiles choose actual branding, colour and data packages; the Forge example is a separately booted/installable ISO. This is a keyboard-driven text interface, not a desktop or general application package manager.

[Download M5.1 ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m51-v0.5.1/EngineOS-M51.iso). Use a new SATA disk, two CPUs/128 MiB and Secure Boot disabled. Storage remains 16 MiB native data/48-KiB files with other capacity unallocated. Choose **2 - Install**, your displayed disk, then review and confirm. No `plan N` or sector token is required.

```powershell
python tools/build/m51.py
python tools/vm/run.py --create-install-disk
```

## Historical M5 user handbook

**[Documentation & downloads website](https://engineos.brixcanvas.com)** — English guides and official release downloads, built in BrixCanvas. [Website source and maintenance](website/README.md).

**[Start here](docs/handbook/index.md)** · [Install in VMware](docs/handbook/install.md) · [Recovery](docs/handbook/recovery.md) · [M5 commands](docs/handbook/m5-console.md). M5 installs to a new SATA/512-byte-sector disk of at least 256 MiB after an explicit plan and confirmation. Live changes are temporary and are not copied into installation.

## Historical M5 build and test

[Download M5 ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m5-v0.5.0/EngineOS-M5.iso). Use two CPUs, 128 MiB RAM, a new SATA disk and Secure Boot disabled. Run `disks`, then `plan N`; use only the exact confirmation printed for your target. After verified success, power off VMware, detach ISO/USB and boot the disk.

```powershell
python tools/build/m5.py
python tools/vm/run.py --milestone M5 --create-install-disk
python tests/m5/verify.py
```

Select `--milestone M5` for that historical workflow; the current launcher defaults to M51. `--create-install-disk --installed` boots its owned target without installation media after successful installation. Explicit M5 `--data-disk` targets are writable; use disposable image files. No physical host disk is used by these tests.

## Historical M4.5 boot instructions

[Download the final ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m45-v0.4.5/EngineOS-M45.iso) · [VMware setup and expected screen](docs/releases/m45-v0.4.5.md) · [hashes and all assets](https://github.com/enginsoysal/codename-engine-os/releases/tag/m45-v0.4.5).

Use a separate x86-64 VMware VM with two vCPUs, 128 MiB RAM, ISO-connected CD/DVD and **Secure Boot disabled**. UEFI and BIOS are qualified; SATA/IDE and EFI NVMe have named native read tests. A disk is optional for boot. Expect **M4.5 DEVICE AND FILE CONSOLE** and `engine>`. Blank disks/no writable development filesystem are valid states. The guest profile label FreeBSD 64-bit does not supply a FreeBSD kernel.

```powershell
python tools/build/m45.py
python tools/vm/run.py --milestone M45
python tests/m45/verify.py
```

For M45, `--data-disk` uses an overlay so an ordinary backing image stays unchanged. Select historical milestones explicitly. Public media are unsigned. [M4.5 owner-signed Secure Boot](docs/milestones/m4.5-owner-trust.md) is separately qualified in disposable QEMU policy; it is not factory trust and does not authenticate the public unsigned M5 ISO.

Earlier M4 and M4.5 preview tags remain immutable historical checkpoints. The final completion claim is bounded by [the versioned contract](docs/milestones/m4.5-scope.json).

## Repository layout

```text
boot/                  Original UEFI/BIOS loaders and firmware-to-kernel transition
kernel/                Core, x86-64 exceptions, memory and process mechanisms
include/engine/        Versioned boot/native ABI and shared headers
runtime/               Minimal freestanding memory routines
user/                  Native ring-3 verification programs and linker layout
tools/build/           Separate M1/M2/M3/M4/M45/M5 build entry points and boot-media authoring
tools/vm/              Pinned VM setup and user launcher
tests/m0/              Architecture/toolchain and Windows-host references
tests/m1/              Original kernel-boot/parser/visual regression tools
tests/m2/              Isolation, cleanup, low-RAM and idle tests
tests/m3/              Scheduling, VM, SMP and resource qualification
tests/m4/              Filesystem, DMA/IRQ, recovery and PS/2/USB input qualification
tests/m45/             Broader boot/storage/USB and owner-trust qualification
tests/m5/              Live/install/recovery, overlay/failure and owned VMware qualification
website/               Native BrixCanvas documentation converter, design and publisher
docs/architecture/     Design contracts and decision record
docs/roadmap/          Milestones, current status and Windows generation inventory
docs/milestones/       Per-milestone usage, implementation and handoff notes
docs/releases/         Changelog and release notes
evidence/M0..M5/        Public test reports, sanitized logs and screenshots
build/, dist/          Generated files; ignored by Git
tools/local/           Private/local dependencies and historical archive; ignored
```

The repository is independent from BrixCanvas. Do not put OS sources or outputs in that unrelated project. The initial planning copy was moved to the ignored local archive; GitHub contains the canonical current source.

## Architecture and roadmap

- [Architecture](docs/architecture/overview.md), [decisions](docs/architecture/decisions.md), [kernel contracts](docs/architecture/kernel-contracts.md).
- [OS directory namespace](docs/architecture/filesystem.md), [build/platform choices](docs/architecture/build-platform.md), [compatibility matrix](docs/architecture/compatibility.md).
- [Detailed milestones](docs/roadmap/milestones.md), [Windows 95–11 requirement](docs/roadmap/windows-compatibility.md), [generation inventory](docs/roadmap/windows-targets.json).
- [M0](docs/milestones/m0.md), [M1](docs/milestones/m1.md), [M2](docs/milestones/m2.md), [M3](docs/milestones/m3.md), [M4](docs/milestones/m4.md), [M4.5](docs/milestones/m4.5.md), [M5](docs/milestones/m5.md), [changelog](docs/releases/CHANGELOG.md).
- [Contributing](CONTRIBUTING.md), [agent instructions](AGENTS.md), [host tools](tools/README.md).

## Development commitments

All current documentation, comments, UI strings and new release notes are English. Behavior changes must update architecture/status, verification and user instructions together. After each coherent update, run relevant checks, review, commit and push to [GitHub](https://github.com/enginsoysal/codename-engine-os). Never publish credentials, raw host-user paths, proprietary installers or ignored local archives.

Resource efficiency is a primary requirement: shared immutable code, bounded allocation and queues, separate peak/retained/boot memory accounting, and HLT when idle. A small guest-RAM test does not establish a universal product minimum or remove application resource requirements.

Original contributions use the [MIT license](LICENSE). External build tools and future reused components retain their own licenses and require recorded provenance. Existing binaries remain unmodified compatibility targets; claims are qualified per version, architecture and task, never inferred from a boot screen.
