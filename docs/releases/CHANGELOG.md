# Changelog

## M5.1 v0.5.1 — 2026-11-06

Guided numbered welcome/Try/Install/Repair routes, real disk review/back, bounded progress and readable errors with optional diagnostics. Normal tasks no longer require terminal commands or sector tokens. Strict profiles build actual differently branded/coloured/populated ISO/USB distributions; Forge has independent installed-boot evidence. Product principles make convenience and distribution freedom ongoing acceptance requirements. [Release notes](m51-v0.5.1.md), [guided tutorial](../handbook/install-m51.md), [distribution tutorial](../handbook/distributions.md).


## M5 v0.5.0 — 2026-11-06

Native BIOS/UEFI live installation to a blank SATA target: confirmed GPT/ESP/16-MiB native-volume setup, verified placement, media-detached boot and durable files. Bounded live overlay, same-build boot repair and failed-copy resume. English installation/recovery tutorials and command reference. No desktop, Windows apps, general upgrades/rollback or power-loss atomicity. [Release notes](m5-v0.5.0.md), [scope and evidence](../milestones/m5.md).

## 2026-11-06 — native BrixCanvas website correction

- Build the website in the owner's BrixCanvas workspace at `engineos.brixcanvas.com`, using native editable Brix and article content. Remove the incorrectly introduced separate-hosting workflow and restore original BrixCanvas Front Door route settings.
- Add committed-Markdown conversion to native articles, a documentation index and official downloads/checksums. Keep OS source separate and frozen M4.5 evidence unchanged.

## 2026-11-06 — initial documentation website prototype

- Add the English documentation/download hub at `engineos.brixcanvas.com`, with current GitHub Markdown, full-text search, release-tag documentation, official assets/digests and clearly dated fallback data.
- Keep frontend source in `website/`, deployment separate from BrixCanvas, and M4.5 runtime evidence unchanged. Documentation and releases update on load/refresh; frontend changes require a Site redeploy.

## M4.5 handbook clarification — 2026-11-06

Add a prominent user handbook, actual console command reference and a step-by-step VMware 20-GB SATA setup. Clarify that LSI/other SCSI controllers are unsupported, blank disks should still be discovered, `parts` only inspects, and ordinary writable partitioning/formatting/installing is M5. No kernel, ISO or existing VM/disk is changed by this documentation update.

## M4.5 v0.4.5 — 2026-11-06

Final user-approved M45-VM-v1 milestone: fresh 29-gate qualification and 23 trust checks; exact ISO hash in five VMware profiles; normal launcher defaults to M45; console identifies M4.5 without an in-progress banner. All current contracts/status/usage distinguish VM completion from unqualified physical/factory-trust follow-ups. [Final ISO and VMware instructions](m45-v0.4.5.md). M5 installation remains not started.

## M4.5 hardware preview 4 — 2026-11-06

Persistent pinned/encrypted owner signing, authenticated payload update and db rotation/authority/replay/recovery VM tests; private staging cleanup on failure; actual USB hardware binding/error inventory. [Release notes and remaining external qualification](m45-hardware-alpha.4.md). Full M4.5 remains in progress.

## M4.5 previews 1–3 — 2026-11-05

Original BIOS/UEFI shared-kernel media, ordinary AHCI/NVMe/ATA/VirtIO/USB read discovery, GPT/MBR and FAT32/extended-ext2 validation, owner-enrolled VM trust, bounded USB hub trees/UAS, reconnect/EIO/deadline recovery and the SMP pressure-response correction. [Increment evidence and named limits](../../evidence/M4.5/README.md). These were partial previews, not universal hardware or full M4.5 completion.

## M4 v0.4.0 — 2026-11-05

Original AHCI reset/IRQ-completed DMA with HLT waits and 4 KiB batching; bounded writable ext2/VFS and native namespace; strict UTF-8/case and generation/share handles; dirty-volume refusal and offline copy recovery. Own xHCI 64-bit MMIO, MSI-X/MSI, boot keyboard/mouse and PS/2/COM1 diagnostics. Reboot, full-disk, error/timeout/recovery, low-RAM, ISO/USB and named VMware qualification are recorded in [M4 evidence](../../evidence/M4/README.md). [Release and exact limits](m4-v0.4.0.md). No installer, desktop or Windows application support.

## M4 first increment — 2026-11-05

Original class-bound AHCI driver, marked disposable volume, fixed DMA buffers, CRC/generation sector record with flush/readback, and interrupt-driven PS/2/COM1 diagnostic console. [Scope and pending M4 work](../milestones/m4.md). No filesystem, installer or USB input yet.

## M3 v0.3.0 — 2026-11-05

Preemptible native threads with guarded stacks, FS-base TLS and x87/SSE/MXCSR state; atomic event/mailbox waits, join/cancel/deadlines; HPET monotonic time across delayed interrupts; backed anonymous commit/decommit, COW CPU/copy-out paths and bounded OOM recovery. ACPI INIT/SIPI starts additional CPUs with per-CPU GDT/TSS/IST/stacks and APIC timers; native user execution is verified on 1/2/4 CPUs. Concurrent allocator and acknowledged TLB replacement/unmap are tested. M2 handle rights/mapping lifetime and earlier isolation regressions remain required.

Feature-detected per-CPU identity, demand-driven hardware-clock reads and same-space CR3 reuse reduce avoidable switch overhead. An unchanged native two-thread round-trip benchmark and component timings are published with a before/after comparison.

Current launcher/build/English contracts, roadmap, evidence and release instructions move to M3. Fixed affinity, serialized kernel dispatch, xAPIC/HPET VM scope, quotas and unsupported AVX/migration/file paging are explicit. No Windows apps, installer or desktop. [M3](../milestones/m3.md), [release notes](m3-v0.3.0.md).

## M3 first increment — 2026-11-05

Single-active-CPU timer preemption with an explicit PIC/PIT baseline, safe arithmetic-flag preservation, two non-yielding native probes and bounded cleanup. Separate build/test/ISO path preserves the M2 baseline. At that alpha release, full M3 was still in progress; the completed scope is recorded in the v0.3.0 entry above. [Details](../milestones/m3.md).

## M2 v0.2.0 — 2026-11-05

Protected native ring-3 processes, private address spaces, shared immutable code, NX/write protection, guarded stacks, checked syscall buffers, generation/type/rights-checked handles, bounded messages and explicit shared memory. Real hostile user faults are isolated; repeated process/object cleanup returns page counts to baseline. Normal ISO displays measured results and halts instead of polling. Windows 95–11 remains planned compatibility, not an existing capability.

All current documentation is English. Architecture/roadmap/milestone/release documents, per-milestone tests and build/VM tools now have dedicated folders. The OS repository remains independent of BrixCanvas; ignored historical local archives are not public source. See [M2](../milestones/m2.md) and [current status](../roadmap/status.md).

## M1 v0.1.0 — 2026-11-05

Own UEFI loader and ELF64 kernel boot through ISO and GPT-USB, with firmware exit, own page tables/stack/GDT/TSS/IDT, framebuffer/serial diagnostics, symbolized CPU panic and debugger connectivity. 29 M1 checks plus 51 M0 regressions passed. No installer or app execution. [Historical release notes](m1-v0.1.0.md).

## M0 baseline — 2026-11-05

Architecture, native/guest filesystem namespaces, object/ABI/boot contracts, Windows generation inventory and pinned compiler/VM/firmware/ISO tools. 51 technical checks, including EFI-only environment fixtures, passed before real kernel implementation.
