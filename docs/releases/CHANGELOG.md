# Changelog

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
