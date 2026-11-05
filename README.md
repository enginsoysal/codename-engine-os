# Codename: Engine OS

An independent, MIT-licensed operating system with its own UEFI loader, kernel and native user runtime. Windows 95–11 application compatibility is a long-term requirement, followed by Linux/macOS execution environments and ARM64. Those compatibility layers are not implemented yet.

**M4.5 is in progress before M5:** [broader boot/hardware scope](docs/milestones/m4.5.md), [ordinary-storage/trust increment](docs/milestones/m4.5-storage.md). The [M4.5 preview 2](https://github.com/enginsoysal/codename-engine-os/releases/tag/m45-hardware-alpha.2) provides a test ISO/USB image with dual BIOS/UEFI boot and broader read-only storage; use Secure Boot disabled for these unsigned preview downloads. See [USB/filesystem scope](docs/milestones/m4.5-usb-filesystems.md). The M0–M4 completion labels below describe their bounded published test profiles, not universal hardware support.

**Current completed milestone: M4 — persistent storage, bounded ext2/VFS and input, within its named native virtual-device scope.** M0 established architecture/toolchain, M1 own-kernel boot, M2 protected native processes and M3 scheduling/VM/SMP. M4 adds original AHCI and xHCI drivers, a persistent native directory namespace and interactive file console. See the [status](docs/roadmap/status.md), [M4 contracts](docs/milestones/m4-contract.md) and [test evidence](evidence/M4/README.md). There is no installer, desktop or existing Windows application support yet.

## Boot in a VM

[Download M4 prerelease](https://github.com/enginsoysal/codename-engine-os/releases/tag/m4-v0.4.0) · [M4 instructions](docs/milestones/m4-contract.md) · [test evidence](evidence/M4/README.md). Earlier [M3](https://github.com/enginsoysal/codename-engine-os/releases/tag/m3-v0.3.0) and [transport-only alpha](https://github.com/enginsoysal/codename-engine-os/releases/tag/m4-storage-alpha.1) releases remain immutable historical snapshots.

```powershell
python tools/build/m4.py
python tools/vm/run.py --create-data-disk
```

Use a 64-bit UEFI VM with Secure Boot disabled. QEMU defaults to two vCPUs/128 MiB; `--create-data-disk` preserves a marked disposable working image between boots. For VMware copy both filesystem-template IMG/VMDK assets together and attach the copied VMDK as SATA; see the [instructions](docs/milestones/m4-contract.md). M4 VMware qualification covers SATA persistence at two CPUs/128 MiB; USB keyboard/mouse and framebuffer/idle evidence come from pinned QEMU. Neither test qualifies all versions/devices or physical hardware.

## Repository layout

```text
boot/                  Original UEFI/BIOS loaders and firmware-to-kernel transition
kernel/                Core, x86-64 exceptions, memory and process mechanisms
include/engine/        Versioned boot/native ABI and shared headers
runtime/               Minimal freestanding memory routines
user/                  Native ring-3 verification programs and linker layout
tools/build/           Separate M1/M2/M3/M4 build entry points and boot-media authoring
tools/vm/              Pinned VM setup and user launcher
tests/m0/              Architecture/toolchain and Windows-host references
tests/m1/              Original kernel-boot/parser/visual regression tools
tests/m2/              Isolation, cleanup, low-RAM and idle tests
tests/m3/              Scheduling, VM, SMP and resource qualification
tests/m4/              Filesystem, DMA/IRQ, recovery and PS/2/USB input qualification
docs/architecture/     Design contracts and decision record
docs/roadmap/          Milestones, current status and Windows generation inventory
docs/milestones/       Per-milestone usage, implementation and handoff notes
docs/releases/         Changelog and release notes
evidence/M0..M4/       Public test reports, sanitized logs and screenshots
build/, dist/          Generated files; ignored by Git
tools/local/           Private/local dependencies and historical archive; ignored
```

The repository is independent from BrixCanvas. Do not put OS sources or outputs in that unrelated project. The initial planning copy was moved to the ignored local archive; GitHub contains the canonical current source.

## Architecture and roadmap

- [Architecture](docs/architecture/overview.md), [decisions](docs/architecture/decisions.md), [kernel contracts](docs/architecture/kernel-contracts.md).
- [OS directory namespace](docs/architecture/filesystem.md), [build/platform choices](docs/architecture/build-platform.md), [compatibility matrix](docs/architecture/compatibility.md).
- [Detailed milestones](docs/roadmap/milestones.md), [Windows 95–11 requirement](docs/roadmap/windows-compatibility.md), [generation inventory](docs/roadmap/windows-targets.json).
- [M0](docs/milestones/m0.md), [M1](docs/milestones/m1.md), [M2](docs/milestones/m2.md), [M3](docs/milestones/m3.md), [M4](docs/milestones/m4.md), [changelog](docs/releases/CHANGELOG.md).
- [Contributing](CONTRIBUTING.md), [agent instructions](AGENTS.md), [host tools](tools/README.md).

## Development commitments

All current documentation, comments, UI strings and new release notes are English. Behavior changes must update architecture/status, verification and user instructions together. After each coherent update, run relevant checks, review, commit and push to [GitHub](https://github.com/enginsoysal/codename-engine-os). Never publish credentials, raw host-user paths, proprietary installers or ignored local archives.

Resource efficiency is a primary requirement: shared immutable code, bounded allocation and queues, separate peak/retained/boot memory accounting, and HLT when idle. A small guest-RAM test does not establish a universal product minimum or remove application resource requirements.

Original contributions use the [MIT license](LICENSE). External build tools and future reused components retain their own licenses and require recorded provenance. Existing binaries remain unmodified compatibility targets; claims are qualified per version, architecture and task, never inferred from a boot screen.
