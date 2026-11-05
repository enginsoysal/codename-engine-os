# Changelog

## Projectorganisatie — 2026-11-05

De oude OS-planningskopie is uit BrixCanvas verplaatst naar een genegeerd lokaal archief binnen de zelfstandige Engine OS-repository. Werkdirectory- en publicatieafspraken voorkomen dat de twee projecten opnieuw door elkaar raken. Kernel en M1-release zijn ongewijzigd.

## M1 v0.1.0 — 2026-11-05

Own UEFI loader and ELF64 kernel now boot in QEMU from ISO and GPT-USB. Firmware services are stopped before the kernel takes control using its own page tables, stack, GDT/TSS/IDT and framebuffer console. Negative load tests, real CPU panic/symbolization and debugger connectivity have evidence. Normal boot media, debug files, checksums and [release instructions](M1.md) are available.

29 M1 checks and all 51 M0 regression checks pass. No installer, usermode or existing Windows app support yet. Windows 95–11 compatibility remains a separate staged requirement.

## M0 baseline — 2026-11-05

Architecture, directory namespaces, kernel/ABI/boot contracts, Windows generation inventory and pinned build/firmware tools established. 51 technical checks, including EFI-only environment fixtures, passed before real kernel implementation.
