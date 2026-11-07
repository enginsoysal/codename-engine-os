# Current status

**M7.4 standard applications are in progress.** The requested full set and
independent removal/resource criteria are tracked in the [M7.4 contract](../milestones/m7.4.md).
No M7.4 ISO has been qualified or published. M7.3 remains the current release.

**M7.3 is complete within M73-VM-v1.** Seventeen exact-artifact gates cover the smooth native wizard/folder desktop, actual 4K, scaling/persistence, disk safety, error recovery, resource idle and VMware lifecycles. [Scope](../milestones/m7.3.md), [acceptance](../../evidence/M7.3/acceptance-results.json), [release](../releases/m73-v0.7.3.md), [graphics](../architecture/graphics-m73.md), [installation guide](../handbook/install-m73.md). Physical/ARM qualification and M8 remain separate.

## Historical M7.2 delivery — 7 November 2026

**M7.2 is complete within M72-VM-v1: installation and a visible folder desktop before more applications.** The native wizard now ends with Open desktop against the persistent installed volume. Files foregrounds Home, sidebar locations, Back/Forward/Up, named folders and protected system browsing. A new empty-disk VMware starter ZIP removes VM setup ambiguity. UEFI boot registration addresses a reproduced retained-NVRAM USB boot failure; exact-artifact gates include unforced same-VM VMware boots. [Installer](../handbook/install-m72.md), [desktop](../handbook/desktop-m72.md), [scope](../milestones/m7.2.md), [acceptance](../../evidence/M7.2/acceptance-results.json). The original user VM failure is not conclusively diagnosed. M8 remains deferred.

## Owner priority revision — 6 November 2026

The product remains **Engine OS**. The next delivery order is:

1. **M6 — real product installation:** guided ISO/USB installation, useful disk capacity and large files, persistent users/settings/data, independent disk boot, tested update/recovery behavior and clear failures.
2. **M7 — real desktop experience:** a native graphical session with windows, launcher, file manager, editor, settings, networking and real downloads; normal tasks require no terminal commands.
3. **M8 — original MS-DOS and Windows 1.x through Windows 11 applications:** separate DOS, early Windows, 9x and NT/Win32/Win64 tracks, including original installers and useful application tasks.

**M6 is complete within M6-VM-v1 and M7 within M7-VM-v1**; M8 is not started under these revised acceptance contracts. The [M7 desktop](../milestones/m7.md) passes 26 exact-artifact gates, including native documents, networking/HTTPS, app containment, resource pressure, custom distro and VMware BIOS/UEFI tasks. [Acceptance](../../evidence/M7/acceptance-results.json), [VMware tutorial](../handbook/install-m7.md). Completed M0–M5.1 remain historical technical achievements within their named scopes. The historical M5.1 installer is a working bounded prototype, not completion of the new product installation or desktop requirements. The native M6 wizard, scalable installed volume, user preferences, data distribution selection and ISO-assisted update/rollback/repair are implemented. The historical M6 artifact passes the [M6 acceptance gates](../../evidence/M6/acceptance-results.json); [release and VMware instructions](../releases/m6-v0.6.0.md) identify that earlier ISO. The current M7 ISO and desktop guide are linked above. Completion is bounded by the named virtual profiles. See [M6 delivery slices](../milestones/m6.md). [Detailed gates and implementation choices](milestones.md).

**M7.1 is complete within M71-VM-v1:** native disk/partition management and explicit existing-disk replacement, qualified by 15 gates and 322 unchanged prior proofs. [Acceptance](../../evidence/M7.1/acceptance-results.json), [VMware tutorial](../handbook/install-m71.md), [disk-manager manual](../handbook/disk-manager-m71.md), [release](../releases/m71-v0.7.1.md). M8 remains not started.

Convenience, optional advanced diagnostics, distribution freedom and measured CPU/RAM efficiency remain requirements throughout. A splash screen, loader, theme or successful launch alone cannot close these milestones.

## Documentation website

The public [Engine OS website](https://engineos.brixcanvas.com) uses native editable BrixCanvas blocks and the preferred local design. It publishes canonical Markdown guides, the documentation index and official downloads, with source commits. Automatic GitHub synchronization uses a site-specific content-only encrypted Actions secret. The first publication verified 55 articles; new M5 installation/recovery guides join the current index while earlier release guides remain historical. See [publisher evidence](../../evidence/documentation/github-sync-results.json) and [maintenance](../../website/README.md).

Updated 2026-11-07. Canonical project: [enginsoysal/codename-engine-os](https://github.com/enginsoysal/codename-engine-os), public/MIT, independent from BrixCanvas. All current documents/UI/comments are English; code/tests and documentation are updated together. Every coherent verified update is committed and pushed.

| Milestone | Status | Evidence/boundary |
|---|---|---|
| M0 architecture/toolchain | Complete | [51 technical checks](../../evidence/M0/README.md); no app compatibility claim |
| M1 own-kernel boot | Complete, virtual scope | [Historical evidence](../../evidence/M1/README.md); one VMware boot user-observed |
| M2 protected native processes | Complete within native-only, single-active-CPU scope | [Current M2 evidence](../../evidence/M2/README.md), [fresh acceptance](../../evidence/M2/acceptance-results.json) |
| M3 scheduling/general VM/concurrency | Complete within documented native VM scope | [Contracts and limits](../milestones/m3.md), [fresh acceptance](../../evidence/M3/acceptance-results.json); native execution on 1/2/4 CPUs, bounded threads/waits/COW/pressure |
| M4 persistent storage/input | Complete within documented native filesystem/virtual-device scope | [Fresh acceptance](../../evidence/M4/acceptance-results.json), [AHCI/ext2/VFS and USB/PS/2 contracts](../milestones/m4-contract.md) |
| M4.5 broader boot/hardware | Complete within M45-VM-v1 | [Fresh acceptance](../../evidence/M4.5/acceptance-results.json), [approved scope](../milestones/m4.5.md), [final ISO and VMware guide](../releases/m45-v0.4.5.md); dual BIOS/UEFI, ordinary storage, checked filesystems, driver status/input/hubs/UAS, faults/resource/SMP and 23 owner-trust checks; physical/factory trust and production key operations are separate unqualified follow-ups |
| M5 live/install/recovery | Complete within M5-VM-v1 | [Contract](../milestones/m5.md), [acceptance](../../evidence/M5/acceptance-results.json), [installation tutorial](../handbook/install.md); native installer, detached boot, bounded live/persistent volume and same-build repair |
| M6 real product installation | Complete within M6-VM-v1 | [Native wizard, storage and recovery](../milestones/m6.md), [exact acceptance](../../evidence/M6/acceptance-results.json), [VMware handbook](../handbook/install-m6.md) |
| M7.3 native visual quality and HiDPI | Complete within M73-VM-v1 | [Current evidence](../../evidence/M7.3/acceptance-results.json), [VMware ZIP and ISO](../releases/m73-v0.7.3.md); measured typography, actual 4K, safe scaling and structured design review |
| M7.2 installation and folder desktop | Complete within M72-VM-v1 | [Historical evidence](../../evidence/M7.2/acceptance-results.json), [VMware starter and ISO](../releases/m72-v0.7.2.md); immediate real persistent desktop plus separately tested independent boot |
| M7.1 installer disk manager | Complete within M71-VM-v1 | [Exact acceptance](../../evidence/M7.1/acceptance-results.json); GPT/primary-MBR data operations, FAT32/Engine quick formats, explicit whole-disk replacement, equal-capacity disk isolation and detached desktop |
| M7 real desktop | Complete within M7-VM-v1 | [Exact acceptance](../../evidence/M7/acceptance-results.json), [delivery/limits](../milestones/m7.md), [VMware installation](../handbook/install-m7.md), [desktop controls](../handbook/desktop-m7.md); original DOS/Windows execution remains M8 |
| M5.1 guided UX/distributions | Complete within M51-VM-v1 | [Contract](../milestones/m5.1.md), [guided tutorial](../handbook/install-m51.md), [custom distributions](../handbook/distributions.md), [acceptance](../../evidence/M5.1/acceptance-results.json) |

## M2 delivered scope

Private user address spaces, CPL3 entry/checked return, native syscalls, controlled copy-in/out, bounded generation/rights/type-checked objects, mailbox IPC and explicit shared memory. Shared code reduces duplication; private pages are zeroed and reaped. Thirteen initial roles plus 32 fresh survivors yield 45 starts, 35 exits and ten contained failures in the specified workload. The normal ISO runs these checks then shows results and halts.

Resource reports separate peak/retained managed pages from retained boot resources. Qualification includes low guest RAM, quotas/reuse and actual CPU halt/idle-time observations. Exact final values/build ID are in the acceptance/boot/visual reports, not guessed from guest RAM settings.

## M4 delivered scope

Original class-bound AHCI reset, one outstanding MSI/HLT request, 4 KiB DMA batches and timeout/error quarantine. Bounded writable ext2 and native root, 48 KiB direct files, exact case/UTF-8 and kernel file-handle mediation. Dirty-volume write refusal and independent offline recovery to a new copy are explicit; no journal or arbitrary power-loss guarantee. Own xHCI boot keyboard/mouse uses a supervisor MMIO window and MSI-X/MSI; PS/2/COM1 remain diagnostic fallbacks. Named QEMU ISO/USB, low-RAM, input/resource/error/recovery and VMware SATA persistence tests pass. Installer/live/install/recovery workflow is M5; physical devices, hotplug/hubs, guest semantics and untrusted file syscalls are outside this bounded profile.

## Limits and next work

M2 has one active CPU, cooperative trusted native probes, no preemption/starvation defense, no general pressure/reclaim, no arbitrary app/file loading and no Windows APIs. Boot/firmware resources remain reserved. M3 retains that regression phase and adds real preemptible threads, x87/SSE/FS-base TLS, hardware-clock deadlines, event/message waits, cancellation, anonymous reserve/commit/decommit/COW, admission-budget/OOM recovery and active SMP. Native user execution is checked on every discovered qualified CPU. Processes have fixed affinity; extended XSAVE, migration, fine-grained locking, file paging/reclaim and physical qualification remain future work. M4 adds the bounded storage/input profile above. M4.5 closes the named VM boot/storage/input/trust acceptance contract; M5 implements installation/recovery; the next product delivery order is real installation in M6, desktop/networking in M7, then DOS/Windows compatibility in M8.

MS-DOS and Windows 1.x–11 remain a broad target qualified per original installer/version/architecture/task through WC1–WC7. It is not a universal flawless-start guarantee. Physical hardware and arbitrary VMware-version/device coverage remain unqualified and outside M45-VM-v1. Additional [VMware Workstation 26.0.1 serial/runtime cases](../../evidence/M3/vmware/README.md) pass on 1/2/4 CPUs with the same normal ISO at 128 MiB; broader version/device coverage and VMware framebuffer capture are not claimed.

Only `Complete` plus successful fresh linked evidence counts as milestone completion; mocks, generated files or a splash screen are insufficient. Failed checks/blockers must remain visible. Earlier local planning is archived under ignored `tools/local/legacy-planning`; never place new OS output back in BrixCanvas.

## M5 implementation and resource boundary

Original sparse-bundle installer, SATA/512-byte blank-target GPT/EFI/BIOS setup, explicit confirmation and copy readback, installed boot and persistent native files. The live overlay allocates changed pages on demand, capped at 512 KiB. The installed ext2 profile is 16 MiB with 48-KiB files; other disk capacity remains unallocated. Same-build boot repair preserves initialized data. Arbitrary upgrades/rollback and power-loss atomicity are not implemented. Multiple eligible SATA targets refuse automatic persistent selection. [Installation](../handbook/install.md), [commands](../handbook/m5-console.md), [recovery](../handbook/recovery.md), [exact contract](../milestones/m5.md).
