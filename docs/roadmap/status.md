# Current status

## Documentation website

The public [Engine OS website](https://engineos.brixcanvas.com) uses native editable BrixCanvas blocks and the preferred local design. It publishes canonical Markdown guides, the documentation index and official downloads, with source commits. Automatic GitHub synchronization uses a site-specific content-only encrypted Actions secret. The first publication verified 55 articles; new M5 installation/recovery guides join the current index while earlier release guides remain historical. See [publisher evidence](../../evidence/documentation/github-sync-results.json) and [maintenance](../../website/README.md).

Updated 2026-11-06. Canonical project: [enginsoysal/codename-engine-os](https://github.com/enginsoysal/codename-engine-os), public/MIT, independent from BrixCanvas. All current documents/UI/comments are English; code/tests and documentation are updated together. Every coherent verified update is committed and pushed.

| Milestone | Status | Evidence/boundary |
|---|---|---|
| M0 architecture/toolchain | Complete | [51 technical checks](../../evidence/M0/README.md); no app compatibility claim |
| M1 own-kernel boot | Complete, virtual scope | [Historical evidence](../../evidence/M1/README.md); one VMware boot user-observed |
| M2 protected native processes | Complete within native-only, single-active-CPU scope | [Current M2 evidence](../../evidence/M2/README.md), [fresh acceptance](../../evidence/M2/acceptance-results.json) |
| M3 scheduling/general VM/concurrency | Complete within documented native VM scope | [Contracts and limits](../milestones/m3.md), [fresh acceptance](../../evidence/M3/acceptance-results.json); native execution on 1/2/4 CPUs, bounded threads/waits/COW/pressure |
| M4 persistent storage/input | Complete within documented native filesystem/virtual-device scope | [Fresh acceptance](../../evidence/M4/acceptance-results.json), [AHCI/ext2/VFS and USB/PS/2 contracts](../milestones/m4-contract.md) |
| M4.5 broader boot/hardware | Complete within M45-VM-v1 | [Fresh acceptance](../../evidence/M4.5/acceptance-results.json), [approved scope](../milestones/m4.5.md), [final ISO and VMware guide](../releases/m45-v0.4.5.md); dual BIOS/UEFI, ordinary storage, checked filesystems, driver status/input/hubs/UAS, faults/resource/SMP and 23 owner-trust checks; physical/factory trust and production key operations are separate unqualified follow-ups |
| M5 live/install/recovery | Complete within M5-VM-v1 | [Contract](../milestones/m5.md), [acceptance](../../evidence/M5/acceptance-results.json), [installation tutorial](../handbook/install.md); native installer, detached boot, bounded live/persistent volume and same-build repair |
| M5.1 guided UX/distributions | Complete within M51-VM-v1 | [Contract](../milestones/m5.1.md), [guided tutorial](../handbook/install-m51.md), [custom distributions](../handbook/distributions.md), [acceptance](../../evidence/M5.1/acceptance-results.json) |
| M6 desktop/network/downloads | Not started | Boot console only |
| M7 Windows binary foundation | Not started | No NT/Win32 APIs |
| M8 original 7-Zip workflow | Not started | Third-party app register empty |
| M9 DOS runtime | Not started | Not supplied by native usermode |
| M10 GPU/API acceleration | Not started | CPU framebuffer only |
| M11 Steam client | Not started | Version-specific future target |
| M12 Steam games | Not started | Includes separate driver/service blockers |
| M13 Adobe target | Not started | Package/version/task gates required |
| M14 Linux binaries | Not started | Reserved ABI IDs are not implementation |
| M15 macOS/Xcode | Not started | Separate framework/tool/service gates |
| M16 ARM64/Pi | Not started | Cross-compilation alone does not prove boot |

## M2 delivered scope

Private user address spaces, CPL3 entry/checked return, native syscalls, controlled copy-in/out, bounded generation/rights/type-checked objects, mailbox IPC and explicit shared memory. Shared code reduces duplication; private pages are zeroed and reaped. Thirteen initial roles plus 32 fresh survivors yield 45 starts, 35 exits and ten contained failures in the specified workload. The normal ISO runs these checks then shows results and halts.

Resource reports separate peak/retained managed pages from retained boot resources. Qualification includes low guest RAM, quotas/reuse and actual CPU halt/idle-time observations. Exact final values/build ID are in the acceptance/boot/visual reports, not guessed from guest RAM settings.

## M4 delivered scope

Original class-bound AHCI reset, one outstanding MSI/HLT request, 4 KiB DMA batches and timeout/error quarantine. Bounded writable ext2 and native root, 48 KiB direct files, exact case/UTF-8 and kernel file-handle mediation. Dirty-volume write refusal and independent offline recovery to a new copy are explicit; no journal or arbitrary power-loss guarantee. Own xHCI boot keyboard/mouse uses a supervisor MMIO window and MSI-X/MSI; PS/2/COM1 remain diagnostic fallbacks. Named QEMU ISO/USB, low-RAM, input/resource/error/recovery and VMware SATA persistence tests pass. Installer/live/install/recovery workflow is M5; physical devices, hotplug/hubs, guest semantics and untrusted file syscalls are outside this bounded profile.

## Limits and next work

M2 has one active CPU, cooperative trusted native probes, no preemption/starvation defense, no general pressure/reclaim, no arbitrary app/file loading and no Windows APIs. Boot/firmware resources remain reserved. M3 retains that regression phase and adds real preemptible threads, x87/SSE/FS-base TLS, hardware-clock deadlines, event/message waits, cancellation, anonymous reserve/commit/decommit/COW, admission-budget/OOM recovery and active SMP. Native user execution is checked on every discovered qualified CPU. Processes have fixed affinity; extended XSAVE, migration, fine-grained locking, file paging/reclaim and physical qualification remain future work. M4 adds the bounded storage/input profile above. M4.5 closes the named VM boot/storage/input/trust acceptance contract; M5 implements installation/recovery; desktop/network work remains next in M6.

Windows 95–11 remains a broad target qualified per original installer/version/architecture/task through WC1–WC7. It is not a universal flawless-start guarantee. Physical hardware and arbitrary VMware-version/device coverage remain unqualified and outside M45-VM-v1. Additional [VMware Workstation 26.0.1 serial/runtime cases](../../evidence/M3/vmware/README.md) pass on 1/2/4 CPUs with the same normal ISO at 128 MiB; broader version/device coverage and VMware framebuffer capture are not claimed.

Only `Complete` plus successful fresh linked evidence counts as milestone completion; mocks, generated files or a splash screen are insufficient. Failed checks/blockers must remain visible. Earlier local planning is archived under ignored `tools/local/legacy-planning`; never place new OS output back in BrixCanvas.

## M5 implementation and resource boundary

Original sparse-bundle installer, SATA/512-byte blank-target GPT/EFI/BIOS setup, explicit confirmation and copy readback, installed boot and persistent native files. The live overlay allocates changed pages on demand, capped at 512 KiB. The installed ext2 profile is 16 MiB with 48-KiB files; other disk capacity remains unallocated. Same-build boot repair preserves initialized data. Arbitrary upgrades/rollback and power-loss atomicity are not implemented. Multiple eligible SATA targets refuse automatic persistent selection. [Installation](../handbook/install.md), [commands](../handbook/m5-console.md), [recovery](../handbook/recovery.md), [exact contract](../milestones/m5.md).
