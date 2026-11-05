# Codename: Engine OS

An independent, MIT-licensed operating system with its own UEFI loader, kernel and native user runtime. Windows 95–11 application compatibility is a long-term requirement, followed by Linux/macOS execution environments and ARM64. Those compatibility layers are not implemented yet.

**Current completed milestone: M3 — native scheduling, memory policy and concurrency.** M0 established the architecture/toolchain and M1 delivered kernel boot. M2 adds ring-3 isolation, rights-checked handles, bounded IPC/shared memory and measured page cleanup. See the [current status](docs/roadmap/status.md) for verified results. [M3](docs/milestones/m3.md) qualifies preemptible threads, x87/SSE/TLS, waits/deadlines, anonymous COW/pressure and 1/2/4 active CPUs within its documented native VM limits. There is no installer, shell, desktop or existing Windows application support yet.

## Boot in a VM

[Download M3 prerelease](https://github.com/enginsoysal/codename-engine-os/releases/tag/m3-v0.3.0) · [M3 instructions](docs/milestones/m3.md) · [test evidence](evidence/M3/README.md)

```powershell
python tools/build/m3.py
python tools/vm/run.py
```

Use a 64-bit UEFI VM with Secure Boot disabled. The default QEMU launcher uses two vCPUs and 128 MiB guest RAM; `--cpus 1`, `2` or `4` selects the qualified CPU profiles. Automated qualification uses pinned QEMU/OVMF. Additional [VMware Workstation 26.0.1 serial/runtime tests](evidence/M3/vmware/README.md) pass at 1/2/4 CPUs with 128 MiB; framebuffer capture/idle-register evidence is from QEMU. This does not qualify every VMware version/device profile or physical hardware.

## Repository layout

```text
boot/                  UEFI loader and firmware-to-kernel transition
kernel/                Core, x86-64 exceptions, memory and process mechanisms
include/engine/        Versioned boot/native ABI and shared headers
runtime/               Minimal freestanding memory routines
user/                  Native ring-3 verification programs and linker layout
tools/build/           Separate M1/M2/M3 build entry points and boot-media authoring
tools/vm/              Pinned VM setup and user launcher
tests/m0/              Architecture/toolchain and Windows-host references
tests/m1/              Original kernel-boot/parser/visual regression tools
tests/m2/              Isolation, cleanup, low-RAM and idle tests
tests/m3/              Scheduling, VM, SMP and resource qualification
docs/architecture/     Design contracts and decision record
docs/roadmap/          Milestones, current status and Windows generation inventory
docs/milestones/       Per-milestone usage, implementation and handoff notes
docs/releases/         Changelog and release notes
evidence/M0..M3/       Public test reports, sanitized logs and screenshots
build/, dist/          Generated files; ignored by Git
tools/local/           Private/local dependencies and historical archive; ignored
```

The repository is independent from BrixCanvas. Do not put OS sources or outputs in that unrelated project. The initial planning copy was moved to the ignored local archive; GitHub contains the canonical current source.

## Architecture and roadmap

- [Architecture](docs/architecture/overview.md), [decisions](docs/architecture/decisions.md), [kernel contracts](docs/architecture/kernel-contracts.md).
- [OS directory namespace](docs/architecture/filesystem.md), [build/platform choices](docs/architecture/build-platform.md), [compatibility matrix](docs/architecture/compatibility.md).
- [Detailed milestones](docs/roadmap/milestones.md), [Windows 95–11 requirement](docs/roadmap/windows-compatibility.md), [generation inventory](docs/roadmap/windows-targets.json).
- [M0](docs/milestones/m0.md), [M1](docs/milestones/m1.md), [M2](docs/milestones/m2.md), [M3](docs/milestones/m3.md), [changelog](docs/releases/CHANGELOG.md).
- [Contributing](CONTRIBUTING.md), [agent instructions](AGENTS.md), [host tools](tools/README.md).

## Development commitments

All current documentation, comments, UI strings and new release notes are English. Behavior changes must update architecture/status, verification and user instructions together. After each coherent update, run relevant checks, review, commit and push to [GitHub](https://github.com/enginsoysal/codename-engine-os). Never publish credentials, raw host-user paths, proprietary installers or ignored local archives.

Resource efficiency is a primary requirement: shared immutable code, bounded allocation and queues, separate peak/retained/boot memory accounting, and HLT when idle. A small guest-RAM test does not establish a universal product minimum or remove application resource requirements.

Original contributions use the [MIT license](LICENSE). External build tools and future reused components retain their own licenses and require recorded provenance. Existing binaries remain unmodified compatibility targets; claims are qualified per version, architecture and task, never inferred from a boot screen.
