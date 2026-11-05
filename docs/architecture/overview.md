# Architecture

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

USB GPT/FAT32 and optical EFI El Torito are distinct tested media. Live/installed/recovery should share the same kernel. A real installer, persistent system image, update activation and rollback are M5 work. Normal preview media remain unsigned. M4.5 separately qualifies an owner-enrolled VM trust chain with a signed loader and SHA-256-pinned external payloads; factory trust and physical owner enrollment remain open.

## Graphics and hardware

Discover bus/protocol identity, bind a supported driver, initialize and publish capabilities. A device class alone cannot supply its register protocol. M2 retains GOP's framebuffer and draws through the CPU; there is no GPU acceleration. MMIO/DMA, residency and hardware faults require driver-specific handling. Initial platform: pinned QEMU q35/OVMF; physical GPU and ARM/Pi support are later milestones.

## Evidence and change control

M4 adds class-bound AHCI with one outstanding MSI/HLT request and 4 KiB DMA batches, bounded writable ext2/VFS and own xHCI boot keyboard/mouse input. Four AHCI pages and fixed filesystem/input limits bound resources; uncertain DMA ownership is quarantined. Dirty-volume write refusal and offline copy recovery are explicit rather than a journaling guarantee. See the [M4 contract and named profiles](../milestones/m4-contract.md).

Cross-compile without a host CRT; retain symbols, build IDs, source/artifact hashes, negative tests and public sanitized logs. Record CPU/guest/host configuration and separate managed pages from retained boot memory and framebuffer reservations. The M3 preemption and SMP suite proves its specified native workloads, not universal starvation/race freedom or arbitrary-app security. Exact limits and resource measurements remain linked to fresh reports.

Primary sources: [UEFI 2.11](https://uefi.org/specs/UEFI/2.11/), [Intel system programming](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf), [AMD system programming](https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2), [Clang](https://clang.llvm.org/docs/UsersManual.html). These define interfaces; our tests establish only explicitly reported behavior.

## M4.5 firmware increment

The in-progress M4.5 build adds an original BIOS EDD/E820/VBE path beside UEFI. Both produce the same normalized boot contract and enter the same kernel. BIOS conventional E820 ranges are normalized to the existing allocator descriptor types; this does not require UEFI firmware on the BIOS path. M45 UEFI releases the temporary ELF file allocation after segment copying. See [boot design and tests](../milestones/m4.5-boot.md) and [storage/trust boundaries](../milestones/m4.5-storage.md). Earlier milestone reservation evidence remains historical.

## M4.5 USB topology and transport

The shared original M45 USB stack is organized under `kernel/usb/`. Eight lazily allocated device contexts bound hub/HID/storage state. Class-9 hub routing, child-before-parent retirement and generation-tagged disk readers protect reconnect lifetime. Read-only UAS adds high-speed Ready/Sense and SuperSpeed stream handling with bounded command abort before DMA reclamation. See [protocol, memory and qualification boundaries](../milestones/m4.5-usb-topology.md).
