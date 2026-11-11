# Milestones and acceptance contract

## Current owner priority - 11 November 2026

[M11.0 through M11.6](../milestones/m11.md) refresh installation and the desktop:
selection and double-click, real Files operations, Start/categories/taskbar,
window behavior and visuals, guided setup, then exact final media qualification.
Each stage must pass its checks, update English documentation, push to GitHub
and verify the public website before the next begins. M11.0 through M11.4 are
complete within their stage criteria: [input](../../evidence/M11/1/acceptance-results.json),
[Files](../../evidence/M11/2/acceptance-results.json),
[Start/taskbar](../../evidence/M11/3/acceptance-results.json) and
[windows/appearance](../../evidence/M11/4/acceptance-results.json).
The owner's M11.4 clarification requires a visible release, exact test media,
English manuals and fresh complete site/download verification at each remaining
stage. [M11.4 delivery](../releases/m114-v0.11.4.md) precedes M11.5 expanded setup
and M11.6 full regression. M10 remains an immutable historical qualification.

## Historical owner priority - 10 November 2026

M10 delivers the first two hardware phases before further Windows expansion:
a bounded capability inventory and Settings > Devices, everyday input/display,
FAT32 USB file copies, safe removal and reconnect. Exact artifact installation,
independent startup, ordinary/4K presentation, resource and error tests govern
acceptance. [Scope and later phases](../milestones/m10.md), [design](../architecture/hardware-m10.md), [user guide](../handbook/hardware-m10.md).
The current product name is Engacer OS; earlier release identities remain historical.

## Historical owner priority - 9 November 2026

M9 automatic hardware support precedes further Windows application expansion. The first graphics slice is qualified within M9-GFX-VM-v1: supported VMware video/pointer binding, buffered cached window movement, resource/idle behavior, real installation and existing-app regression. The broader hardware program remains in progress. [Scope and remaining stages](../milestones/m9.md), [graphics decisions](../architecture/graphics-m9.md), [guide](../handbook/install-m9.md).

M9.2 now delivers the named automatic writable-controller matrix, independent VMware startup and clipped off-screen windows. Broader hardware work stays ahead of Windows expansion: resolve known boot-topology gaps, truthful active/limited device inventory, additional useful storage/network/input/audio support, safe reconnect and GPU/display work. [Current controller scope](../milestones/m9.2.md), [guide](../handbook/install-m92.md).

## Historical owner priority — 8 November 2026

M7.5 follows owner-reported installation confusion and startup failure. The novice path now uses a visible disk list, retained choices, bounded partial redraw and actual installed Restart. [Nine-gate acceptance](../../evidence/M7.5/acceptance-results.json), [manual](../handbook/install-m75.md), [scope and limits](../milestones/m7.5.md). Exact original-owner VM diagnosis and human studies remain separate.

## Historical owner priority — 7 November 2026

**M7.3 is complete within M73-VM-v1:** native visual quality, clear guided steps, readable text and actual 4K sizing. Clarity, convenience, user friendliness, graphical quality and readability remain mandatory for later changes. [Delivery](../milestones/m7.3.md), [review](../architecture/usability-review-m73.md), [current tutorial](../handbook/install-m73.md). M8 now delivers its first separately bounded original MSI/console profile; the wider program remains in progress.

## Historical M7.2 priority — 7 November 2026

Deliver **M7.2: convenient graphical installation → visible real desktop and folder navigation**, before expanding applications or starting M8. The installer must open the persistent desktop immediately, and independent disk boot must pass ordinary firmware discovery using the same VMware configuration and retained NVRAM. [Delivery and acceptance](../milestones/m7.2.md), [design decisions](../architecture/desktop-m72.md), [tutorial](../handbook/install-m72.md). Historical milestone acceptance is retained; it is not a claim that the user has successfully booted the current build.

Updated 2026-11-06. M0–M6 are complete within their documented technical acceptance scopes; [status](status.md) and fresh evidence govern each claim. M4.5 completion uses the explicitly user-approved M45-VM-v1 scope. Later milestones are planned, not implicit capabilities.

The implementation owner is the coding agent; the user sets product scope and accepts results. Each milestone needs reproducible commands, build/source/artifact identity, negative cases, resource accounting and visible limitations. A mock/stub or screenshot alone never proves a complete feature. CPU/RAM efficiency is measured from M2 onward; it is not inferred from configured guest memory.

## M0 — architecture and technical feasibility

**Depends on:** product requirements. **What:** define boot/native ABI/object/memory/driver contracts, generation semantics, directory views and the first build/test platform. Compare C17/assembly with alternatives; choose toolchain, UEFI strategy, development storage and 16/32/64-bit direction.

**How/why:** compile minimal x64/ARM64 ELF and EFI fixtures, assert layouts, run actual C malformed-input validators, observe Windows-host semantics and test firmware loading through ISO and GPT-USB. Small reference experiments reveal contradictions before a desktop is built. Record alternatives and separate documentation from runtime evidence.

**Acceptance:** pinned tools/firmware, 51 technical checks plus document/inventory gate; architecture sufficiently precise for M1. **Risk/limit:** actual guest syscalls, TLS, races and Linux/Mach behavior remain later gates; unavailable physical hardware does not block virtual M1. [M0](../milestones/m0.md).

## M1 — own bootable kernel and diagnostics

**Depends on:** M0. **What:** cross-build, original UEFI loader, ELF validation/link layout, initmanifest, memory ownership/handoff, own stack/page tables, GDT/TSS/IDT, serial/framebuffer and debugger/symbols.

**How/why:** load through UEFI, obtain a final map, exit boot services with bounded retry and enter our own kernel through an explicit ABI adapter. Separate optical ISO and USB-device routes catch packaging assumptions. Diagnostics precede complex services so crashes are actionable.

**Acceptance:** real kernel boots from both media in QEMU, matching loader/kernel build IDs; genuine #UD reports RIP/error/symbol/source; corrupt inputs are rejected; normal framebuffer persists. Original suite: 29 M1 checks plus M0 regression. **Risk/limit:** M1's broad bootstrap map is not safe for untrusted apps; physical boot is unqualified. One VMware M1 boot was user-observed. [M1](../milestones/m1.md).

## M2 — isolated native processes and controlled objects

**Depends on:** M1. **What:** backed page allocation, separate address spaces, CPL3 entry/return, native syscall gateway, generation/type/rights-checked handles, bounded messages, explicit shared memory and complete process cleanup.

**How/why:** start a small independent native ELF in private contexts, share immutable code but give each process data/stack pages. Harden the kernel mapping before user execution. Use software validation and hardware page faults together. Initially keep one active CPU and cooperative switching to prove ownership before timer/SMP races.

**Acceptance:** actual private same-VA sentinels, IPC/shared-page round trip, reduced rights, stale handles, quotas and invalid pointers; deliberate user supervisor-write, NX, guard, code-write, I/O/CLI and bad-return faults do not take down kernel/survivors. Repeated fresh processes restore allocator/object baselines. Publish a new ISO, screenshot and measured low-RAM/idle results. **Risk/limit:** trusted native probes only; no arbitrary Windows programs, preemption, starvation safety, general VM, or installer. [M2](../milestones/m2.md).

## M3 — memory policy, scheduling and concurrency

**Depends on:** M2. **What:** timer interrupts/preemption, deadlines, thread state/TLS, cancellation and correct waits, additional CPUs, commit/reservation policy, pressure/OOM, shared/COW mappings and page faults.

**How/why:** deterministic concurrency scenarios before stress; monotonic time; atomically test/enqueue wake conditions; fixed lock ordering, backing guarantees and emergency reserves. Extend M2's single-CPU/limited allocator rather than claiming it is already general. File-backed demand paging follows reliable M4 storage; compression/swap are separately benchmarked additions.

**Acceptance:** no observed lost wakeups in the specified suite, correct COW, bounded allocation failures, responsive operation under pressure and multiple CPUs; measured budgets/latency. **Risk:** tests do not prove absence of races; review and fault injection remain essential. Future user idle uses interrupt-aware sleep rather than permanent demo HLT.

M3 closes this gate in the bounded native VM profile documented in [M3](../milestones/m3.md): actual CPL3 execution on 1/2/4 CPUs, fixed process affinity, serialized kernel dispatch, x87/SSE/TLS, hardware-clock waits, anonymous COW/commit and tested budget pressure. Native thread/space/page quotas and unsupported AVX/migration/file paging are explicit. [Fresh acceptance](../../evidence/M3/acceptance-results.json) must pass; this is not general hardware or Windows app qualification.

## M4 — storage, discovery and input

**Depends on:** M3. **What:** bus/protocol driver binding, block queues/DMA, the chosen writable filesystem/VFS, buffering/flush, USB controller and keyboard/mouse.

**How/why:** virtual drivers first, then named physical controllers; content hashes and fault injection; define atomicity, Unicode/case, locks, rename/delete and metadata. Detection is separate from successful data transfer. Attach file-backed paging only after storage correctness.

**Acceptance:** persistent create/read/modify/rename across reboot, disk-full/I/O errors, documented crash recovery, working input on named configurations. **Risk:** destructive tests use disposable images/explicit test disks; one controller is not universal USB/NVMe coverage.

The implemented acceptance profile and explicit limits are in the [M4 contract](../milestones/m4-contract.md): bounded ext2/UTF-8 namespace, kernel file handles, AHCI MSI/HLT requests, conservative offline copy recovery and directly attached QEMU xHCI boot HID. Unsupported filesystem features and controllers are refused; completion is not a consumer filesystem durability or universal hardware claim.

## M4.5 — broader virtual boot, device protocols and trust

**Depends on:** M4. **Status:** complete within the user-approved M45-VM-v1 contract. **What:** own BIOS/UEFI ISO/USB paths, ordinary AHCI/NVMe/ATA/VirtIO/BOT/UAS inspection, checked GPT/MBR/FAT32/ext2, explicit binding/error inventory, input/hub reconnect, fault/deadline containment, bounded resources and owner-enrolled VM trust/update/rotation.

**How/why:** qualify real protocol transfers and negative cases on named QEMU/VMware endpoints. The OS binds supported bus/class/protocol interfaces rather than brand names. Physical qualification and factory trust are separate follow-ups; their absence does not block this approved virtual milestone and is never presented as passing physical evidence.

**Acceptance:** 29 gates against one exact build/media identity, 23 trust checks, five VMware profiles, managed-page/HLT/idle observations and M4/M3/M0 regressions. [Contract](../milestones/m4.5-scope.json), [fresh acceptance](../../evidence/M4.5/acceptance-results.json), [final release](../releases/m45-v0.4.5.md). **Limits:** no installer/installed boot, desktop/network/GPU acceleration, ARM64 or Windows app execution. Public media are unsigned; the enabled-Secure-Boot profile uses separately enrolled QEMU owner policy.

## M5 — live boot, real installation and recovery

**Depends on:** M4.5. **What:** live/write-layer profile, target disk inventory, partition/EFI setup, verified OS placement, independent installed boot, recovery mode and update activation/rollback format.

**How/why:** reuse the same kernel/drivers; start with a text installer if appropriate. Show exact target/change plan before writing; preflight capacity/driver support; verify copied content and boot registration. Use a separate blank virtual disk first; dual boot is an additional test package.

**Acceptance:** start ISO/USB live, install, detach media, boot installed disk, cancel before writes, handle copy failure and demonstrate recovery. **Risk:** first-install success does not prove atomic updates; non-journaled storage needs an explicit integrity boundary. No simulated install screen counts. See the [M5 implementation](../milestones/m5.md), [installation tutorial](../handbook/install.md) and [acceptance](../../evidence/M5/acceptance-results.json) for actual runtime behavior and exact limits.

## M5.1 — guided setup and distribution profiles

**Depends on:** M5. **What:** discoverable numbered Try/Install/Repair routes, disk review/back, readable errors/progress and optional diagnostics; actual profile-based custom ISO builds. **How/why:** a bounded state machine in the existing IRQ/HLT console avoids a heavyweight GUI dependency while removing mandatory terminal syntax. Versioned recipes separate identity/content from the original kernel and mandatory safety guards. **Acceptance:** full menu-only install/cancel/persistence/repair tasks, actual alternative distro boot/install, unsafe recipe refusal, resource checks and VMware 20-GiB SATA profiles. [Contract](../milestones/m5.1.md), [product principles](../architecture/product-principles.md). **Limits:** keyboard text UI, data packages only, no human study/desktop/app runtime or universal hardware claim.

## Current product priorities — owner revision 2026-11-06

The next three milestones replace the previous future numbering. Completed M0–M5.1 scopes and immutable releases remain historical technical achievements. M5/M5.1 demonstrate native installation and a guided prototype; they do not satisfy the broader product installation/desktop requirements below. The [previous future schedule](history/2026-11-06-previous-future-milestones.md) is retained as history only.

## M6 — real product installation

**Status:** complete within [M6-VM-v1](../milestones/m6-scope.json), with [exact acceptance](../../evidence/M6/acceptance-results.json) and [release instructions](../releases/m6-v0.6.0.md).

**Priority 1. Depends on:** M5.1. Deliver a complete guided installation from ISO/USB onto the selected supported target, meaningful use of the selected disk capacity, application-ready large-file storage, a real native system/user layout and first-run user setup. Remove the fixed 16-MiB/48-KiB product bottlenecks. The installed system must own its boot/runtime assets and persistent configuration, rather than relying on installation media or a demonstration data volume.

**How/why:** extend storage/integrity before placing a desktop or Windows packages on it. Show exact partitions/capacity and destructive changes, preserve cancellation and target revalidation, verify placed files and boot setup, and keep recovery understandable. Provide a versioned update/activation/recovery design with actual implemented rollback/repair behavior and named crash/fault boundaries; declaring record fields is insufficient. Do not silently claim arbitrary power-loss atomicity or every physical controller.

**Acceptance:** a new user completes installation using screen guidance; available capacity is accounted for; large files and non-demo user data survive multiple independent boots with ISO/USB detached; user/settings setup persists; cancellation/refusal does not change unrelated data; interrupted copy/write/activation paths have demonstrated recovery, integrity checks and readable errors. Qualify exact VMware BIOS/UEFI media/controllers and resource measurements. No desktop or Windows app claim is inferred from passing this milestone.

## M7 — real desktop experience

**Priority 2. Depends on:** M6. Deliver an actual native graphical session with independently managed windows, mouse/keyboard focus, fonts, clipboard, launcher, file manager, settings and real native applications for opening/editing/saving documents. Desktop/theme choices must be implemented profile options, not ignored configuration fields. Keep Advanced/verbose available without requiring them for ordinary work.

Include supported networking, automatic setup, name resolution, valid-time/certificate handling and real HTTPS downloads. Storage must support useful documents/downloads; an attractive static screen or menu is not desktop acceptance.

**Acceptance:** boot an installed system into the desktop; open/move/resize/switch several windows; create/edit/save/reopen files across restart; change persistent settings; download and verify a real file, reject invalid certificates and recover from network loss; contain an app crash while the desktop remains usable. Measure idle CPU, damage-driven repainting, peak/retained RAM/VRAM and responsiveness under memory pressure. Start with a bounded CPU-rendered path if appropriate; GPU acceleration is added when a target needs it. Publish exact tested limits. A native text editor is not evidence of Windows application compatibility.

## M7.1 — installer disk and partition management

The owner added this before M8 after testing M7 on an existing disk. Deliver native guided partition inspection, GPT/primary-MBR creation/deletion, FAT32/Engine quick formatting, safe cancel/consent and explicit whole-disk replacement through the installer. Preserve other partitions in individual operations, and qualify existing-disk installation through detached desktop boot. Logical partitions, resize/move, non-destructive conversion, secure erase and dual-boot installation remain outside this bounded release. [Contract, choices and acceptance](../milestones/m7.1.md).

## M8 — original MS-DOS and Windows 1.x through Windows 11 applications

The first **M8-MSI1-VM-v1** delivery covers original 7-Zip 26.06 x64 MSI installation, console execution/use, removal and reinstall. [Exact scope](../milestones/m8-msi.md), [release](../releases/m8-v0.8.0.md). This does not close the tracks below.

**Priority 3. Depends on:** M6, M7 and M7.1. Windows remains the first foreign application ecosystem. Explicitly include original MS-DOS programs/games, Windows 1.x/2.x/3.x applications, Windows 95/98/Me and the NT generations through Windows 11. Windows 95 support does not imply early Windows or DOS support. Installer and payload formats/architectures/version expectations are separate inventory fields.

This is a substantial program of independently qualified submilestones, not one loader or a universal flawless-start promise:

| Track | Required behavior and proof |
|---|---|
| M8.1 DOS | Original COM/MZ/setup workflows, required extenders, isolated CPU/device/timing/input/video/audio, file grants, play/use/save/load/exit |
| M8.2 early Windows | Separate 1.x, 2.x and 3.x profiles; actual original executable/setup formats, segmented memory, Windows APIs, GUI/message behavior and mixed 16/32-bit installation chains |
| M8.3 Windows 9x | Original 95/98/Me app workflows and actual generation semantics, including ANSI/Unicode, files/configuration, graphics and applicable service/driver dependencies |
| M8.4 Windows NT/Win32/Win64 | NT 3.x onward through Windows 11: validated PE loading, imports/DLLs, exceptions/TLS, memory/files/threads/waits, registry, services and generation-specific behavior |
| M8.5 runtimes/installers | Original EXE/MSI/mixed-bitness/AppX/MSIX where required; COM, VB/VC, .NET and modern helper/runtime dependencies; actual install/update/repair/uninstall |
| M8.6 graphics/audio/drivers | Required GDI/DirectDraw/Direct3D/media/input plus a real native acceleration path when needed; explicit device, DRM, anticheat and external-service gates |
| M8.7 real application tasks | Original 7-Zip installer-to-uninstall first, then separately pinned Steam client/games and Adobe targets; no adapted demos or success-only stubs |

**Acceptance:** per original app/version/architecture/profile: clean setup/install, first launch, useful tasks, restart, repeated launch, save/reopen and removal where applicable; failures and isolation/resource bounds tested. The [expanded inventory](windows-targets.json), [WC1–WC7](windows-compatibility.md) and exact evidence govern claims. Unknown apps remain unqualified. External authority, unavailable devices/services and unimplemented guest drivers are explicit blockers, not reasons to fake success.

## Deferred tracks and cross-cutting gates

Linux binaries, macOS/Xcode and ARM/Raspberry Pi stay on the backlog after these priorities; they do not replace the Windows/DOS program. Their old future numbers are superseded, not completion claims. GPU/driver work is performed as required by the desktop/application targets, not inferred from a framebuffer.

All three priorities retain convenience, readable errors, distribution freedom, measured CPU/RAM efficiency, supported-hardware declarations and automatically current English documentation. A fresh tested ISO and an end-to-end user task are required for a release. No screenshot, wire format, native probe or unimplemented profile option closes a product milestone.
