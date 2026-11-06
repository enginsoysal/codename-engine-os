# Codename: Engine OS

An independent, MIT-licensed operating system with its own UEFI loader, kernel and native user runtime. Windows 95–11 application compatibility is a long-term requirement, followed by Linux/macOS execution environments and ARM64. Those compatibility layers are not implemented yet.

**Current completed milestone: M4.5 — boot, storage, input and owner-trust qualification within M45-VM-v1.** The user approved this named QEMU/VMware scope on 2026-11-06. Physical hardware, factory trust and production key operations remain separate unqualified follow-ups. [Acceptance](evidence/M4.5/acceptance-results.json), [scope and limits](docs/milestones/m4.5.md), [current status](docs/roadmap/status.md).

The final [M4.5 v0.4.5 release](https://github.com/enginsoysal/codename-engine-os/releases/tag/m45-v0.4.5) provides the same original kernel through BIOS/UEFI ISO and USB boot, ordinary read-only storage discovery, checked GPT/MBR/FAT32/ext2, bounded USB hubs/UAS and an interactive diagnostic console. There is no installer, desktop or existing Windows app compatibility yet.

## User handbook

**[Documentation & downloads website](https://codenameenginos.brixcanvas.com)** — English, searchable, automatically loaded from GitHub. [Website source and maintenance](website/README.md).

**[Start here](docs/handbook/index.md)** · [20-GB disk missing / SCSI versus SATA](docs/handbook/vmware-storage.md) · [Console commands](docs/handbook/console.md). M4.5 inspects existing disks; it cannot create partitions, format or install yet.

## Boot in VMware or QEMU

[Download the final ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m45-v0.4.5/EngineOS-M45.iso) · [VMware setup and expected screen](docs/releases/m45-v0.4.5.md) · [hashes and all assets](https://github.com/enginsoysal/codename-engine-os/releases/tag/m45-v0.4.5).

Use a separate x86-64 VMware VM with two vCPUs, 128 MiB RAM, ISO-connected CD/DVD and **Secure Boot disabled**. UEFI and BIOS are qualified; SATA/IDE and EFI NVMe have named native read tests. A disk is optional for boot. Expect **M4.5 DEVICE AND FILE CONSOLE** and `engine>`. Blank disks/no writable development filesystem are valid states. The guest profile label FreeBSD 64-bit does not supply a FreeBSD kernel.

```powershell
python tools/build/m45.py
python tools/vm/run.py
python tests/m45/verify.py
```

The QEMU launcher now defaults to M45, two vCPUs/128 MiB. `--data-disk` uses an overlay so an ordinary backing image stays unchanged. Select earlier milestones explicitly, such as `--milestone M4 --create-data-disk` for the older writable development-volume profile. Public final media are unsigned. [Owner-signed Secure Boot](docs/milestones/m4.5-owner-trust.md) is separately qualified in disposable QEMU policy; it is not factory trust. Detailed USB/input/fault/idle qualification is QEMU-specific, not a claim for every VMware configuration.

Earlier M4 and M4.5 preview tags remain immutable historical checkpoints. The final completion claim is bounded by [the versioned contract](docs/milestones/m4.5-scope.json).

## Repository layout

```text
boot/                  Original UEFI/BIOS loaders and firmware-to-kernel transition
kernel/                Core, x86-64 exceptions, memory and process mechanisms
include/engine/        Versioned boot/native ABI and shared headers
runtime/               Minimal freestanding memory routines
user/                  Native ring-3 verification programs and linker layout
tools/build/           Separate M1/M2/M3/M4/M45 build entry points and boot-media authoring
tools/vm/              Pinned VM setup and user launcher
tests/m0/              Architecture/toolchain and Windows-host references
tests/m1/              Original kernel-boot/parser/visual regression tools
tests/m2/              Isolation, cleanup, low-RAM and idle tests
tests/m3/              Scheduling, VM, SMP and resource qualification
tests/m4/              Filesystem, DMA/IRQ, recovery and PS/2/USB input qualification
tests/m45/             Broader boot/storage/USB and owner-trust qualification
website/               Public documentation frontend and snapshot builder
docs/architecture/     Design contracts and decision record
docs/roadmap/          Milestones, current status and Windows generation inventory
docs/milestones/       Per-milestone usage, implementation and handoff notes
docs/releases/         Changelog and release notes
evidence/M0..M4.5/      Public test reports, sanitized logs and screenshots
build/, dist/          Generated files; ignored by Git
tools/local/           Private/local dependencies and historical archive; ignored
```

The repository is independent from BrixCanvas. Do not put OS sources or outputs in that unrelated project. The initial planning copy was moved to the ignored local archive; GitHub contains the canonical current source.

## Architecture and roadmap

- [Architecture](docs/architecture/overview.md), [decisions](docs/architecture/decisions.md), [kernel contracts](docs/architecture/kernel-contracts.md).
- [OS directory namespace](docs/architecture/filesystem.md), [build/platform choices](docs/architecture/build-platform.md), [compatibility matrix](docs/architecture/compatibility.md).
- [Detailed milestones](docs/roadmap/milestones.md), [Windows 95–11 requirement](docs/roadmap/windows-compatibility.md), [generation inventory](docs/roadmap/windows-targets.json).
- [M0](docs/milestones/m0.md), [M1](docs/milestones/m1.md), [M2](docs/milestones/m2.md), [M3](docs/milestones/m3.md), [M4](docs/milestones/m4.md), [M4.5](docs/milestones/m4.5.md), [changelog](docs/releases/CHANGELOG.md).
- [Contributing](CONTRIBUTING.md), [agent instructions](AGENTS.md), [host tools](tools/README.md).

## Development commitments

All current documentation, comments, UI strings and new release notes are English. Behavior changes must update architecture/status, verification and user instructions together. After each coherent update, run relevant checks, review, commit and push to [GitHub](https://github.com/enginsoysal/codename-engine-os). Never publish credentials, raw host-user paths, proprietary installers or ignored local archives.

Resource efficiency is a primary requirement: shared immutable code, bounded allocation and queues, separate peak/retained/boot memory accounting, and HLT when idle. A small guest-RAM test does not establish a universal product minimum or remove application resource requirements.

Original contributions use the [MIT license](LICENSE). External build tools and future reused components retain their own licenses and require recorded provenance. Existing binaries remain unmodified compatibility targets; claims are qualified per version, architecture and task, never inferred from a boot screen.
