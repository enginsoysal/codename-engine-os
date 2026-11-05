# M0 → M1 handoff and current continuation

Historical handoff, now implemented by M1. M2 builds on it; this document preserves the sequence and its boundaries.

1. Establish `boot/uefi`, `kernel/arch/x86_64`, `kernel/core`, public headers, runtime, tools and tests with explicit freestanding targets and pinned dependencies.
2. Validate firmware protocols, a restricted ELF64 kernel/initmanifest and all boot segment/ownership ranges. An EFI-only test did not count as this loader.
3. Allocate the own stack/page tables, capture the final memory map, perform bounded ExitBootServices retry, adapt MS-x64 → System V and enter the own kernel.
4. Prove build-bound serial output, own exception handling and a controlled real CPU fault. Retain loader resources until ownership can safely be released.
5. Package optical EFI ISO and actual GPT/FAT32 USB-storage separately; harden the restricted image author instead of calling it a general filesystem driver.
6. Supply a VM launcher, debug symbols, checksums and explicit hypervisor/platform limits.

M1 completed these virtual boot/diagnostic gates. [M1 instructions](m1.md), [M1 evidence](../../evidence/M1/README.md).

## Beyond the first boot

M2 provides limited isolated native processes and resource accounting. M3 adds general memory/scheduling/concurrency. M4 adds persistent storage/input. M5 supplies real live/install/recovery workflows and installed-disk boot without the media. A simulated install screen would not replace these prerequisites.

The current latest route is [M2](m2.md). A user-observed VMware M1 boot is additional narrow evidence; other VMware configurations, physical devices and future milestones still need tests.

## Windows 95–11 invariants

Keep guest binary formats/profiles out of the native boot ABI; reserve Win16/NE, PE32 and PE32+ execution directions, versioned semantics, thunks, runtimes/COM/services, historical/modern graphics and explicit driver dependencies. DOS or x86 compatibility mode alone does not implement Win16. Native kernel/process success does not qualify Windows apps. Record exact original installer/version/architecture/tasks before publishing compatibility.
