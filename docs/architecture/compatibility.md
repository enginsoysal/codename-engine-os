# Compatibility semantics and proof boundaries

| Area | Windows | Linux | macOS/Darwin | Native mechanism/consequence |
|---|---|---|---|---|
| Loader | PE/NE imports and relocations | ELF/interpreter/auxv | Mach-O/dyld commands | Separate validated loaders; a format alone does not identify the complete ABI |
| Calls | MS-x64/x86 and NT | Target ABI and syscalls | Target ABI, Mach/Darwin | Wrappers and trusted process-profile routing |
| Threads/TLS | TEB/PEB, TLS, exceptions | Thread pointer/libc/signals | pthread/runtime state | Save/translate actual expected state; not implemented by a native PID |
| Waiting | Events, mutexes, multiwait/APC | Futex/poll/signals | Mach ports and primitives | Preserve semantic differences; bounded messages alone are insufficient |
| Errors | NTSTATUS/last-error | errno/syscall errors | Mach results/errno | Translate at boundaries; missing functions never silently succeed |
| Memory | Reserve/commit/granularity | mmap/protection flags | Mach VM | Backed memory objects and explicit mapping rules |
| Names | Commonly case-insensitive Unicode lookup | Byte paths, usually case-sensitive | Volume/Unicode rules | Native UTF-8 plus tested guest adapters |
| Delete/share | Share/delete flags | Unlink/open-fd semantics | POSIX plus platform details | Separate name lifetime from open objects |
| Identity | Tokens/ACLs | UID/GID | POSIX/platform security | Controlled mapping to native authorization |
| UI/media | Win32/GDI/DirectX and more | Display/toolkit alternatives | AppKit/CoreGraphics and more | One compositor with adapters; API translation still needs hardware drivers |
| Drivers | VxD/NT contracts | Kernel modules | Platform extensions | Native drivers first; guest-driver support is a separate project |

## Evidence progression

M0 has 11 toolchain/layout/Windows checks, 30 actual C boot/request validator checks, eight additional Windows-host references and two EFI firmware loads. References cover auto/manual events, TLS, basic multiwait, file sharing/rename/delete, case/Unicode and continuable vectored exception delivery. These are host observations, not guest API implementations. Full concurrent close/wait races, Linux signals/futexes and Mach semantics remain later gates.

M1 adds real kernel ExitBootServices handoff, ISO/USB loads, stale-key retry, #UD diagnostics/symbolization and persistent framebuffer/debugger observation. It does not prove app execution.

M2 adds actual native64 ring-3 contexts, private address spaces, hardware NX/RO/supervisor faults, checked syscall copies, handle generations/rights/quotas, bounded messages, explicit shared mappings and repeated cleanup. Only the fixed native verification ELF is executed. Windows/Linux/Darwin profile IDs are reserved but not dispatched; Win16 and DOS are not implemented.

See [M0 evidence](../../evidence/M0/README.md), [historical M1 evidence](../../evidence/M1/README.md), [M2 evidence](../../evidence/M2/README.md), [Windows target inventory](../roadmap/windows-targets.json).

## Component reuse

Wine is a candidate source of Windows behavior/library implementations, not a drop-in kernel. POSIX/thread/signal/memory/driver dependencies need actual ports or replacements. A host-internal POSIX porting interface does not require adopting Linux as the kernel or automatically provide Linux binary compatibility.

Darling is a candidate for Mach/Darwin/framework knowledge and code, but its Linux dependencies need a real port. Adding its source does not prove Xcode or GUI support. Select concrete revisions, provenance, licenses, distribution obligations, security ownership and regression plans before incorporating any component. No Wine/Darling runtime code is currently included.

## Windows 95–11

[WC1–WC7](../roadmap/windows-compatibility.md) require separate installer/app architectures, Win16/NE, 9x versus NT profiles, runtime/package coverage, graphics generations and driver/service dependencies. The app qualification register remains empty because native probes are not third-party Windows applications. Claims require original installer, exact version/hash, CPU architecture, tasks and evidence.

Primary sources: [Wine API](https://source.winehq.org/WineAPI/), [Microsoft events](https://learn.microsoft.com/en-us/windows/win32/sync/using-event-objects), [Linux ABI](https://docs.kernel.org/admin-guide/abi.html), [Darling syscall emulation](https://docs.darlinghq.org/internals/basics/system-call-emulation.html), [Darling source](https://github.com/darlinghq/darling).

## Current product delivery order

The owner revision of 6 November 2026 sets M6 real product installation, M7 real desktop experience, then M8 original MS-DOS and Windows 1.x through Windows 11 application compatibility. M6 is in progress with its initial capacity planner; M7 and M8 are not started. Previous milestone evidence remains bounded historical proof. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply to every stage. See [the current contracts](../roadmap/milestones.md).
