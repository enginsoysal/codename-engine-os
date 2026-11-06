# Milestones and acceptance contract

Updated 2026-11-06. M0–M4.5 are complete within their documented acceptance scopes; [status](status.md) and fresh evidence govern each claim. M4.5 completion uses the explicitly user-approved M45-VM-v1 scope. Later milestones are planned, not implicit capabilities.

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

## M6 — desktop, network and downloads

**Depends on:** M5. **What:** compositor/window/input/focus/fonts/clipboard, shell/file manager, one network driver, IP/DNS and TLS-backed downloads with valid time/certificates.

**How/why:** CPU framebuffer first, bounded buffers/caches; separate protocol stack from driver. A download utility supplies the installer workflow without requiring an entire browser engine immediately.

**Acceptance:** multiple native windows, documents, correct HTTPS download/hash, certificate rejection and recovery from network loss while UI remains responsive under pressure. **Risk:** Wi-Fi, accessibility, power and language coverage need explicit follow-ups; Unicode extensibility starts early.

## M7 — Windows binary foundation

**Depends on:** M3/M4/M6. **What:** PE loader, imports/relocations, DLLs, TLS/exceptions, NT/Win32 memory/files/threads/synchronization, registry/services and versioned syscall decoding.

**How/why:** compare identical small binaries on Windows and Engine OS, including failures/last-error. Audit any Wine port's host dependencies. Implement 64-bit first but prove 32-bit before Steam; Win16 is WC2, not automatically covered by DOS or PE32.

**Acceptance:** named unmodified console binaries, correct output/file/thread behavior, DLL failure paths, direct-syscall routing and isolated crashes. **Risk:** no-op missing functions and a working loader are not app support; guest kernel drivers remain WC6.

## M8 — original 7-Zip installer through uninstall

**Depends on:** M7. **What:** pin version/architecture, implement needed GUI/configuration/path/install/shell behavior, track installation state.

**How/why:** download the real installer, trace dependencies and implement general semantics, using varied archives, Unicode names and content hashes. This proves subsystem integration without an adapted demo bypass.

**Acceptance:** download → install → GUI → archive/extract/hash → OS reboot → use again → uninstall, including corrupt archive/full-disk/restricted-right failures. **Risk:** only the tested version/tasks are qualified; removal must not delete unrelated data.

## M9 — integrated DOS runtime

**Depends on:** M4/M6. **What:** isolated COM/EXE execution, disk views, timing, input/video/audio, DOS extenders where targets require them.

**How/why:** explicit CPU/device emulation within a protected process, granted game-directory mapping and per-game tempo/audio profiles. Historical real-mode hardware assumptions must not gain host-kernel authority.

**Acceptance:** named lawful game builds start, play, save/load and terminate without host damage; no access outside grants. **Risk:** emulation is reported honestly and is not Win16 GUI compatibility.

## M10 — accelerated graphics/API path

**Depends on:** M3/M4/M6. **What:** choose GPU/firmware/driver and underlying graphics API; implement command submission, buffers/address spaces, fences/residency/faults, user driver and shader compiler; then required Direct3D translation.

**How/why:** dependency/provenance audit first; known render/readback comparisons before compositor or game integration. CPU desktop rendering is not a modern gaming backend; API translation still requires a hardware driver.

**Acceptance:** correct frames, stable synchronization/long runs, bounded RAM/VRAM, reported hang detection/recovery and device isolation limits. **Risk:** virtual GPU proof does not qualify a physical card; unavailable documentation/firmware can require changing target hardware.

## M11 — Steam client

**Depends on:** M8, necessary 32-bit/runtime support and a qualified graphics route. **What:** installer, embedded web UI, helpers/services, secure networking, fonts, login, persistence and updates.

**How/why:** inventory subprocesses/dependencies, use an authorized account without logging secrets, separate client from game support, regression-test self-updates.

**Acceptance:** install/login/library/tasks/download-pause-resume-update/reboot/logout/uninstall on a pinned client version. **Risk:** external changes can regress support; MFA remains a user action; client launch does not imply every game works.

## M12 — first Steam games

**Depends on:** M10/M11. **What:** a named selection, beginning without mandatory kernel anticheat, then a heavier 3D workload; required media/input/runtimes/saves.

**How/why:** fix hardware, build, resolution/settings and scenes; distinguish launchable, usable and performant; measure frame-time distributions and memory pressure rather than only average FPS.

**Acceptance:** agreed play session, correct graphics/audio/input, save/load after reboot and workload budgets. GTA requires its own edition/mode record. **Risk:** offline success does not clear DRM/anticheat/service blockers; updates remain version-specific.

## M13 — first Adobe package

**Depends on:** M8 plus actual GUI/network/GPU/runtime needs, not inherently Steam. **What:** one exact package/version, installer/licensing/helpers/fonts/color/GPU/files and defined editing tasks.

**How/why:** lawful access and dependency chain first; CPU routes where supported, then GPU; compare content/color/export results, not only a splash screen. Other packages are separate qualification targets.

**Acceptance:** install/activate/open/edit/export/save/reopen with output validation and peak RAM/scratch/latency measurements. **Risk:** unavailable service/SDK/license access is an explicit blocker; kernel control cannot replace external authority.

## M14 — Linux binaries

**Depends on:** stable core/ABI/files/desktop; independent of Adobe. **What:** ELF/interpreter, Linux syscall ABI, TLS/signals/futexes, process/file/socket/pseudo-filesystem behavior and a fixed libc/distribution profile.

**How/why:** static CLI first, then dynamic binaries and a specified GUI display route. Use unmodified binaries; recompiling a POSIX demo does not prove Linux binary support.

**Acceptance:** named static/dynamic programs with pipes, signals, threads/files and later one GUI target; simultaneous Windows/Linux profiles cannot confuse syscall routing or configuration. **Risk:** ELF loading alone does not support arbitrary distribution packages.

## M15 — macOS binaries and Xcode gates

**Depends on:** core/desktop/ABI and reusable POSIX mechanisms. **What:** Mach-O/dynamic loading, Mach IPC, Darwin and actual app frameworks; target architecture/OS/SDK generation.

**How/why:** M15a CLI, M15b GUI, M15c separate Xcode editor/project/build/debugger/simulator gates. Audit Darling's Linux dependencies and lawful SDK/tool availability before ports.

**Acceptance:** named unchanged apps and exact Xcode tasks, with frameworks/version evidence. A separate compiler is not Xcode and an editor without builds is not full Xcode. **Risk:** external services and inaccessible tools can block individual tasks; no universal promise.

## M16 — ARM64 and a selected Raspberry Pi

**Depends on:** stable core, architectural separation from M0; can begin before M15 once core is ready. **What:** ARM entry/page tables/exceptions/timers/interrupts/atomics/context and board drivers/firmware/image.

**How/why:** QEMU ARM64 native programs before the named physical Pi. Isolate platform code and validate memory ordering, cache/DMA differences. Set console/light-desktop budgets from measured reality.

**Acceptance:** same codebase boots virtually and on that Pi, native ARM programs/storage/input and agreed pressure behavior. **Risk:** an x86 ISO is not a Pi image; x86 Windows apps on ARM need separate instruction translation and app tests.

## Cross-cutting release gates

Every relevant milestone tracks source/artifact identity, negative inputs, crash logs, rights/isolation, resource categories and regressions. Storage adds fault/reboot/recovery tests; apps add exact installer/architecture/version/task records. Firmware/driver/third-party provenance is explicit. English documentation and executable command/link checks stay current in the same change.

Windows 95–11 breadth is [WC1–WC7](windows-compatibility.md), not a guarantee that M0–M16 automatically support all historical binaries. Secure Boot, Wi-Fi, power/suspend, encryption, accessibility and production integrity require explicit work packages before a consumer release; this is an incremental development roadmap.
