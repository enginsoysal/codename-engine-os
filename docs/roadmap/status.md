# Current status

Updated 2026-11-06. Canonical project: [enginsoysal/codename-engine-os](https://github.com/enginsoysal/codename-engine-os), public/MIT, independent from BrixCanvas. All current documents/UI/comments are English; code/tests and documentation are updated together. Every coherent verified update is committed and pushed.

| Milestone | Status | Evidence/boundary |
|---|---|---|
| M0 architecture/toolchain | Complete | [51 technical checks](../../evidence/M0/README.md); no app compatibility claim |
| M1 own-kernel boot | Complete, virtual scope | [Historical evidence](../../evidence/M1/README.md); one VMware boot user-observed |
| M2 protected native processes | Complete within native-only, single-active-CPU scope | [Current M2 evidence](../../evidence/M2/README.md), [fresh acceptance](../../evidence/M2/acceptance-results.json) |
| M3 scheduling/general VM/concurrency | Complete within documented native VM scope | [Contracts and limits](../milestones/m3.md), [fresh acceptance](../../evidence/M3/acceptance-results.json); native execution on 1/2/4 CPUs, bounded threads/waits/COW/pressure |
| M4 persistent storage/input | Complete within documented native filesystem/virtual-device scope | [Fresh acceptance](../../evidence/M4/acceptance-results.json), [AHCI/ext2/VFS and USB/PS/2 contracts](../milestones/m4-contract.md) |
| M4.5 broader boot/hardware | In progress; required before M5 | [Agreed tracks and acceptance boundaries](../milestones/m4.5.md); Increment evidence covers dual BIOS/UEFI media, owner-key VM trust and broader read-only storage; USB BOT/UAS, hub trees/reconnect, deadline/EIO containment and broader read-only ext2 now have named tests; the premature SMP pressure assertion has a [causally tested bounded-wait correction](../milestones/m4.5-smp-pressure.md); persistent encrypted signing and authenticated db rotation/replay tests are implemented; physical/factory trust and production owner-key operational qualification remain open |
| M5 live/install/recovery | Not started | No installer in the current ISO |
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

M2 has one active CPU, cooperative trusted native probes, no preemption/starvation defense, no general pressure/reclaim, no arbitrary app/file loading and no Windows APIs. Boot/firmware resources remain reserved. M3 retains that regression phase and adds real preemptible threads, x87/SSE/FS-base TLS, hardware-clock deadlines, event/message waits, cancellation, anonymous reserve/commit/decommit/COW, admission-budget/OOM recovery and active SMP. Native user execution is checked on every discovered qualified CPU. Processes have fixed affinity; extended XSAVE, migration, fine-grained locking, file paging/reclaim and physical qualification remain future work. M4 adds the bounded storage/input profile above; installation is next in M5.

Windows 95–11 remains a broad target qualified per original installer/version/architecture/task through WC1–WC7. It is not a universal flawless-start guarantee. Physical hardware and complete VMware support remain unqualified. Additional [VMware Workstation 26.0.1 serial/runtime cases](../../evidence/M3/vmware/README.md) pass on 1/2/4 CPUs with the same normal ISO at 128 MiB; broader version/device coverage and VMware framebuffer capture are not claimed.

Only `Complete` plus successful fresh linked evidence counts as milestone completion; mocks, generated files or a splash screen are insufficient. Failed checks/blockers must remain visible. Earlier local planning is archived under ignored `tools/local/legacy-planning`; never place new OS output back in BrixCanvas.
