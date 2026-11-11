# Engacer OS architecture

M11.1–M11.4 add shared release-driven interaction, native Files transactions,
customizable Start/taskbar and bounded windows/appearance to the independent
kernel. [Current release](../releases/m114-v0.11.4.md), [M11 contract](desktop-m11.md),
[Files](files-m11.md), [Start](start-m11.md), [windows](windows-m11.md).
Each stage has separate exact evidence. Expanded setup is M11.5; full new-artifact
controller/application qualification is M11.6. Older records below are baselines,
not automatic qualification of newly built media.

M10 phases 1/2 add bounded hardware status, input layouts and guarded removable file transfers. [Design and limits](hardware-m10.md), [native user guide](../handbook/hardware-m10.md). The 27-gate M10-DEVICES-VM-v1 qualification binds these tasks to the published ISO and exact VMware ZIP.

M9 now prioritizes automatic supported VMware graphics/input and window movement. One admitted RAM screen buffer, cached client movement, batched presentation and a separate cursor overlay supplement the earlier renderer. [Implementation decisions, memory accounting and limits](graphics-m9.md). No 3D or universal GPU claim is made.

The M7.5 installer adds explicit disk lists, partial redraw and actual installed restart. [Installer design and source comparison](installer-m75.md), [current manual](../handbook/install-m75.md). The shared M7.3 native UI uses smooth measured text, shared logical/physical scaling and bounded direct framebuffer rendering. [Graphics decisions](graphics-m73.md), [five-question review](usability-review-m73.md), [current delivery status](../roadmap/status.md). M8 implements a first separately qualified original Win64 console/MSI runtime; the wider foreign layers below remain planned. [Windows runtime](windows-runtime-m8.md), [current installation guide](../handbook/install-m8.md).

Baseline: M0 contracts, M1 boot and M2 protected native processes. [M3](../milestones/m3.md) qualifies native threads, waits, anonymous VM/COW/pressure and 1/2/4 active CPUs in a bounded VM profile. Planned interfaces are distinguished from implementation. See [status](../roadmap/status.md) and [evidence](../../evidence/M2/README.md).

## Layers

```text
Native apps     Windows PE/NE     Linux ELF     macOS Mach-O     DOS
     |                |               |             |            |
Native runtime   Windows profile  Linux profile  Darwin profile Legacy runtime
     +----------------+---------------+-------------+------------+
                              System services
               files, network, audio, compositor, identities, packages
                                    |
                         Versioned native kernel ABI
                                    |
             objects, virtual memory, threads, IPC, rights, exceptions
                                    |
                      Platform code and modular drivers
                                    |
                                 Hardware
```

Only native verification programs currently run, through the preserved M2 baseline and M3 thread runtime. Windows/Linux/macOS adapters are future loaders, libraries, servers and narrowly scoped kernel support. A same-architecture application may execute CPU instructions directly while still needing its expected OS interfaces. Win16/DOS and cross-architecture targets may need explicit emulation; this is never described as universal native execution.

## Kernel boundary

Keep memory/address spaces, scheduling mechanisms, object references/rights, controlled IPC, exceptions and interrupt/DMA fundamentals in the kernel. Put complex policy, desktop, installation and compatibility frameworks in user services. The selected modular kernel avoids assuming every operation needs a server round trip; performance/isolation decisions require measurement.

M2 implements a conventional-memory bitmap allocator, private CR3 address spaces, one native context per process, generation-tagged handles, events/shared-memory objects, bounded messages, controlled copy-in/out and cooperative switching. It runs one bootstrap CPU with maskable interrupts disabled. M3 adds preemption, independent thread CPU/TLS/FX state, atomic blocking waits and active SMP. Processes are assigned fixed CPU affinity; native programs execute on every active CPU while kernel dispatch is serialized. Wait-any/all and guest-specific synchronization remain future adapter work.

## Memory and isolation

M1 uses a broad supervisor identity map for initial firmware handoff. Before user entry, M2 replaces it with kernel RX text, RO/NX constants, RW/NX data, a missing null page, a guarded kernel entry stack and private user mappings. CR0.WP and EFER.NXE are enabled. User code/constant pages are shared immutable allocations; data and stacks are private. Shared pages require an explicit handle/map operation and enforce read-only receiver rights in hardware.

The early allocator manages only UEFI conventional pages below 1 GiB. Loader/firmware allocations are retained and separately accounted for. This is a development limit, not a final physical-RAM ceiling. M3 implements bounded anonymous reservation, backed commit/decommit, COW snapshots/write faults and budget pressure/OOM cleanup. File-backed demand paging, cache reclaim, compression and swap remain future work after reliable storage.

## Execution environments

The trusted loader assigns an ABI profile; user arguments cannot switch it. M2 reserves native64, Win16, Windows x86/x64, Linux and Darwin IDs, but dispatches only native64. Its temporary `int 0x80` gateway uses the documented native register layout. Future `SYSCALL`, 32-bit thunks and NT/Linux/Mach decoders remain separate contracts. M2 does not load arbitrary programs from disk or provide Windows APIs.

## Files, namespaces and identity

The approved native root is `/System`, `/Applications`, `/Users`, `/Data`, `/Cache`, `/Logs`, `/Temporary`, `/Volumes`. No mandatory native drive letters or `/var`. Future VFS namespaces expose the Windows, Linux and macOS paths required by each profile while sharing authorized user documents. [Detailed mapping](filesystem.md).

Kernel authorization must use object identity and explicit rights, not string aliases. Native names are case-sensitive UTF-8; guest lookup, locking, metadata and deletion rules need separate tested adapters. Persistent VFS/filesystem services are M4 work; the firmware file protocol used by the bootloader is not a kernel driver.

## Boot and recovery

UEFI loads the original PE32+ loader. It validates firmware tables and a restricted ELF64 kernel, records allocations, loads a build-bound manifest, obtains a final memory map and exits boot services with bounded stale-key retry. Assembly adapts the MS-x64 caller to System V kernel entry, changes stack/page tables and establishes a GDT. Kernel diagnostics use direct serial/framebuffer access thereafter.

USB GPT/FAT32 and optical El Torito are distinct tested media. M5 live/installed/recovery share the original kernel source with different embedded payloads. Native installation, persistent files and same-build boot repair are implemented; arbitrary update/rollback and atomic activation remain future work. Normal public media remain unsigned. M4.5 separately qualifies an owner-enrolled VM trust chain; this does not authenticate the public unsigned M5 release or establish factory trust.

## Graphics and hardware

Discover bus/protocol identity, bind a supported driver, initialize and publish capabilities. A device class alone cannot supply its register protocol. M2 retains GOP's framebuffer and draws through the CPU; there is no GPU acceleration. MMIO/DMA, residency and hardware faults require driver-specific handling. Initial platform: pinned QEMU q35/OVMF; physical GPU and ARM/Pi support are later milestones.

## Evidence and change control

M4 adds class-bound AHCI with one outstanding MSI/HLT request and 4 KiB DMA batches, bounded writable ext2/VFS and own xHCI boot keyboard/mouse input. Four AHCI pages and fixed filesystem/input limits bound resources; uncertain DMA ownership is quarantined. Dirty-volume write refusal and offline copy recovery are explicit rather than a journaling guarantee. See the [M4 contract and named profiles](../milestones/m4-contract.md).

Cross-compile without a host CRT; retain symbols, build IDs, source/artifact hashes, negative tests and public sanitized logs. Record CPU/guest/host configuration and separate managed pages from retained boot memory and framebuffer reservations. The M3 preemption and SMP suite proves its specified native workloads, not universal starvation/race freedom or arbitrary-app security. Exact limits and resource measurements remain linked to fresh reports.

Primary sources: [UEFI 2.11](https://uefi.org/specs/UEFI/2.11/), [Intel system programming](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf), [AMD system programming](https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2), [Clang](https://clang.llvm.org/docs/UsersManual.html). These define interfaces; our tests establish only explicitly reported behavior.

## M4.5 firmware increment

The completed M45-VM-v1 build adds an original BIOS EDD/E820/VBE path beside UEFI. Both produce the same normalized boot contract and enter the same kernel. BIOS conventional E820 ranges are normalized to the existing allocator descriptor types; this does not require UEFI firmware on the BIOS path. M45 UEFI releases the temporary ELF file allocation after segment copying. See [boot design and tests](../milestones/m4.5-boot.md) and [storage/trust boundaries](../milestones/m4.5-storage.md). Earlier milestone reservation evidence remains historical.

## M4.5 USB topology and transport

The shared original M45 USB stack is organized under `kernel/usb/`. Eight lazily allocated device contexts bound hub/HID/storage state. Class-9 hub routing, child-before-parent retirement and generation-tagged disk readers protect reconnect lifetime. Read-only UAS adds high-speed Ready/Sense and SuperSpeed stream handling with bounded command abort before DMA reclamation. See [protocol, memory and qualification boundaries](../milestones/m4.5-usb-topology.md).

M4.5 completion is governed by the [user-approved VM contract](../milestones/m4.5-scope.json) and [fresh acceptance](../../evidence/M4.5/acceptance-results.json). Physical hardware, factory trust and production key operations remain separate unqualified tracks. M5 implements native installation and same-build boot recovery under its separate [VM contract](../milestones/m5.md); general updates and automatic rollback remain future work.

## Guided interaction and distribution identity

M5.1 adds a bounded keyboard menu over the actual native installation path. End-user tasks must follow the [product principles](product-principles.md); terminal/verbose diagnostics remain optional. Validated profiles compile branding/colour and selected data content into live and installed images. See [M5.1](../milestones/m5.1.md) and [distribution recipes](../handbook/distributions.md). Future desktop/package-runtime configuration must represent real implemented components.

## Current product delivery order

The owner revision of 6 November 2026 sets M6 real product installation, M7 real desktop experience, then M8 original MS-DOS and Windows 1.x through Windows 11 application compatibility. M6 is complete within M6-VM-v1, with a native graphical installer, scalable volume and tested recovery; M7 is complete within M7-VM-v1 with a native installed desktop and checked HTTPS downloads; M8 now delivers one bounded original 7-Zip x64 MSI/console profile; the wider DOS/Windows program remains in progress. See [the runtime architecture](windows-runtime-m8.md). Previous milestone evidence remains bounded historical proof. The current product name is Engacer OS; earlier release identities remain historical. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply to every stage. See [the current contracts](../roadmap/milestones.md).
