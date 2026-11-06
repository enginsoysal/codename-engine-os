# Superseded future roadmap

Historical plan before the owner changed priorities to real installation, real desktop, then MS-DOS/Windows 1–11. These future numbers are not the active schedule. Completed milestones and release evidence are unaffected.

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

Windows 95–11 breadth is [WC1–WC7](../windows-compatibility.md), not a guarantee that M0–M16 automatically support all historical binaries. Secure Boot, Wi-Fi, power/suspend, encryption, accessibility and production integrity require explicit work packages before a consumer release; this is an incremental development roadmap.
