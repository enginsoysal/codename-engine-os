# Kernel contracts

Contracts describe the intended platform; implementation status is explicit below. [Public headers](../../include/engine/process.h) and [M2 evidence](../../evidence/M2/README.md) are the current native baseline.

## Objects and authority

Planned object types include processes, threads, address spaces, memory, channels, events, timers, directories, files and devices. M2 implements process-local handles for event and shared-memory objects. Tokens contain a slot and generation; lookup, type and rights are checked. A handle is not a global pointer or cryptographic secret. Read/write/transfer are explicit rights; duplication/transfer can reduce but not increase authority. Closing/reaping drops references, including persistent mapping references. Generation wrap retires a handle slot instead of silently reusing old tokens.

M2 uses fixed tables: 13 active test-process slots, 16 handles per process, 32 objects, four 32-byte messages per mailbox, one explicit shared mapping and six private allocation pages per process. Internal page-array capacity is eight. Limits return status rather than unbounded allocation. Future security identities/ACLs and file objects are not implemented.

## Process and ABI identity

Each process needs address space, credentials, namespace, budget, handles and loader-selected ABI. M2 stores a native64 ABI and reserves IDs for Win16, Windows x86/x64, Linux x64 and Darwin x64; only native64 dispatch exists. One context runs per process; threads, TLS and SMP are later work. User arguments cannot switch the trusted profile.

Future PE32 uses hardware compatibility mode with 32-bit address/structure translation; PE64 uses its matching CPU ABI. Win16 requires a distinct segmented legacy runtime; DOS emulation is separate. Cross-architecture binaries require explicit instruction translation. Shared native services must not interpret guest pointers as their own.

## Native gateway v0

M2 uses `int 0x80`, a DPL3 interrupt gate, with number in RAX and arguments RDI, RSI, RDX, R10 (R8/R9 reserved). RAX returns a native status; output values use checked user buffers. Registers/context are saved before C execution; return validates user selectors, executable RIP, mapped writable stack and safe flags. RCX/R11 may be considered clobbered by the ABI even though the current interrupt path restores them. Unconfigured `SYSCALL` is disabled and legacy `SYSENTER` has no valid selector; user attempts fault without entering an unintended kernel path. A future fast `SYSCALL` transport must preserve semantics and add per-profile decoders.

Native operations: write a bounded diagnostic, yield, exit, obtain PID, create/signal/query events, duplicate/close handles, send/receive messages, create/map shared memory and transfer a reduced-right handle. No arbitrary user allocation, file I/O or Windows syscall support exists. Buffer requests are bounded against overflow, leaf permissions and mapped pages; page-by-page physical copies avoid dereferencing unchecked guest pointers. Single-CPU/non-preemptible kernel execution currently prevents concurrent mapping changes; future threads need pinning/locking to retain that guarantee.

Windows NT numbers vary by target profile. Import wrappers alone cannot cover direct syscalls. Mach traps and Linux calls likewise need independent decoding and error translation; usermode forwarding requires per-thread request state, cancellation, reentrancy and rights mediation. None is implied by the M2 gateway.

## Memory and protection

4 KiB managed pages; byte-range public interfaces must expose supported granularity for future ARM64. M2 uses read-only executable shared program pages, RO/NX constants, private RW/NX data/stacks, missing guard/null pages and supervisor kernel mappings. Kernel text is RX with CR0.WP enabled; EFER.NXE enforces NX. An early low-memory bitmap allocator uses conventional RAM only, never reclaims loader/firmware memory, and zeroes allocations.

Reservation, commit, residency, COW and file-backed mappings remain distinct future VM concepts. Conservative backing prevents promising memory that does not exist. Cache reclaim, pressure notifications, emergency reserves, paging/compression and general OOM policy belong to M3+. Early M2 bootstrap allocation failure is a diagnosed boot failure; bounded user object creation returns quota/no-memory errors.

## Scheduling and failure isolation

M2 runs one active bootstrap CPU, interrupts masked, with explicit cooperative yield/exit. User faults are dispatched separately from kernel faults, resources are reaped and another address space resumes. It is not starvation-safe against arbitrary non-yielding code; timer preemption is an M3 prerequisite. No polling or background user loop runs after the demo: the kernel executes HLT.

Kernel entry uses a shared 16 KiB guarded stack because syscalls are non-reentrant in this limited model; user contexts live in kernel-owned process structures. Double-fault IST is separately configured. A later preemptible/SMP design needs per-CPU stacks, safe nesting and thread state, not a claim that this shared-stack design is already general.

## Future synchronization, drivers and IPC

Events currently expose state/signal/query only. Windows manual/auto-reset waits, wait-any/all, futex-like compare-and-enqueue, monotonic deadlines, cancellation, APC/signals/Mach exceptions remain future contracts. Returning `WOULD_BLOCK` is not a blocking wait implementation.

Small messages are copied into bounded mailboxes; large sharing uses explicit memory references. DMA must use pinned pages and lifecycle-aware completion; timeout does not prove hardware stopped. IOMMU-backed isolation and real drivers are later work. Device classes do not replace device protocols.

## Remaining proof

M0 validates wire layouts/negative inputs and Windows-host reference semantics. M1 proves firmware handoff/diagnostics. M2 proves its exact native ring-3, rights, fault, cleanup and budget paths. Guest TLS/SEH, preemption/races, general memory pressure, Windows/Linux/Darwin execution and physical hardware remain separate acceptance gates.
