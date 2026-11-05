# Changelog

## M3 first increment — 2026-11-05

Single-active-CPU timer preemption with an explicit PIC/PIT baseline, safe arithmetic-flag preservation, two non-yielding native probes and bounded cleanup. Separate build/test/ISO path preserves the M2 baseline. Full M3 remains in progress: SIMD/thread/TLS, waits/deadlines, general VM/pressure and SMP acceptance are pending. [Details](../milestones/m3.md).

## M2 v0.2.0 — 2026-11-05

Protected native ring-3 processes, private address spaces, shared immutable code, NX/write protection, guarded stacks, checked syscall buffers, generation/type/rights-checked handles, bounded messages and explicit shared memory. Real hostile user faults are isolated; repeated process/object cleanup returns page counts to baseline. Normal ISO displays measured results and halts instead of polling. Windows 95–11 remains planned compatibility, not an existing capability.

All current documentation is English. Architecture/roadmap/milestone/release documents, per-milestone tests and build/VM tools now have dedicated folders. The OS repository remains independent of BrixCanvas; ignored historical local archives are not public source. See [M2](../milestones/m2.md) and [current status](../roadmap/status.md).

## M1 v0.1.0 — 2026-11-05

Own UEFI loader and ELF64 kernel boot through ISO and GPT-USB, with firmware exit, own page tables/stack/GDT/TSS/IDT, framebuffer/serial diagnostics, symbolized CPU panic and debugger connectivity. 29 M1 checks plus 51 M0 regressions passed. No installer or app execution. [Historical release notes](m1-v0.1.0.md).

## M0 baseline — 2026-11-05

Architecture, native/guest filesystem namespaces, object/ABI/boot contracts, Windows generation inventory and pinned compiler/VM/firmware/ISO tools. 51 technical checks, including EFI-only environment fixtures, passed before real kernel implementation.
