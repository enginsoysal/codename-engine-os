# Kernel contracts

These contracts distinguish the preserved M2 baseline from the current M3 native thread runtime. [Public headers](../../include/engine/process.h), [native operation constants](../../include/engine/user.h), [M3 mechanisms/limits](../milestones/m3.md), [fresh evidence](../../evidence/M3/README.md). Neither native profile implements Windows/Linux/Darwin calls.

## Objects and authority

Process-wide handles carry slot/generation, object type and read/write/transfer rights. Lookup checks each before use; duplicate/transfer can reduce rights but never increase them. Generation wrap retires a slot. A handle is neither a global pointer nor a cryptographic secret. Native threads share their process handle table and mailbox. Mapping references keep shared memory alive independently of handles; blocked event waits hold an object reference until signal/timeout/cancel/close.

Both M2 and M3 enforce sixteen handles/process and 32 referenced event/memory objects. M2 has thirteen cooperative test-process slots; M3 has two address spaces/eight native thread slots and four 32-byte messages per process mailbox. M3 also has 64 bounded anonymous/shared backing records and one explicit shared-object mapping per address space. Limits return quota/no-memory statuses. Security identities/ACLs, file/device objects and arbitrary process loading are not implemented; fixtures run in one verification security group.

Closing a handle cancels waits on that handle before dropping its object reference; other aliases remain valid. Process teardown drops all handles, explicit mapping, anonymous pages and private tables only after its threads are no longer runnable. Per-CPU guarded kernel stacks remain mapped/retained and separately counted.

## Process/thread identity and ABI profiles

The trusted loader selects the execution environment; users cannot turn native dispatch into a Windows/Linux/Mach decoder. Reserved profile IDs do not implement those environments. M2 has one context per process. M3 separates address spaces, process IDs and generation-tagged thread IDs: PID names the process; TID names a thread. Send/transfer may name a process or an explicit thread token identifying its shared mailbox/table.

A thread has private general-register/return state, x87/MMX/XMM/MXCSR state, stack, FS-base TLS page and lifecycle/wait state. Process code is shared immutable; data, handles and anonymous mappings are shared among its threads. Each process has a fixed CPU affinity for its lifetime; distinct processes execute native user instructions on distinct CPUs. Migration, credentials and executable discovery/loading are later work.

Future PE32 needs hardware compatibility mode and 32-bit structure/address translation; Win16 needs a segmented legacy runtime, DOS a separate execution environment. Guest TLS/SEH/APCs and exception behavior require guest adapters even though native TLS/thread faults now work. Cross-architecture execution still requires explicit instruction translation.

## Native gateway

The provisional development ABI uses a DPL3 `int 0x80` gate: RAX operation/status, RDI/RSI/RDX/R10 arguments, R8/R9 reserved. Operation/status constants are in the shared native header; M0 request wire layouts are separate forward contracts, not Windows structures. RCX/R11 may be treated as clobbered even though the current transport restores them. Unknown operations return unsupported status. SYSCALL remains disabled and SYSENTER has no valid selector. M3 explicitly initializes and reads back those MSRs on every native execution CPU, disables direct FSGSBASE/OSXSAVE exposure, and atomically publishes runtime activation/configuration state.

Buffers are checked for overflow, native address range and per-page user/present/write permissions. Copies use physical backing under the scheduler lock. Kernel copy-out to anonymous COW backing resolves private ownership first, returning no-memory if it cannot obtain backing. Code/constants remain read-only; shared-object rights still govern writable mapping. No unchecked user pointer is dereferenced as a kernel pointer.

Thread create takes executable entry, output TID pointer and entry argument. It rejects kernel/NX entry mappings and invalid output buffers before publication, allocates independent zeroed stack/TLS and initializes CPU state. Native entries exit through thread-exit rather than return. Join consumes a zombie token or blocks with a deadline; the privileged snapshot launch owner may join its cross-CPU child, and terminal records are reaped on owner exit; cancellation and process exit release waiting references. The bootstrap snapshot launcher and budget/clock injection are explicitly granted harness privileges; ordinary created threads do not inherit them. This is not a general user-admin/credential model.

The retained M2 operations (diagnostic write, yield/exit/PID, events/query, duplicate/close, messages, memory create/map and rights-reduced transfer) remain tested. M3 adds TID/thread lifecycle, TLS, monotonic time, sleep, event reset/wait, blocking receive and anonymous VM. Future fast transports must preserve these semantics and add per-environment decoding. Native numbers/statuses are not NT/Linux/Mach numbers/statuses.

## Memory, commitment and pressure

Managed granularity is 4 KiB. RX shared native text, RO/NX constants, private RW/NX data/TLS/stacks, missing guards/null page and supervisor kernel mappings are enforced with CR0.WP/EFER.NXE. The low identity development window remains; native users start above 4 GiB. A temporary supervisor startup page at 0x8000 is made executable for AP initialization then restored NX. Common upper-half mappings hold per-CPU entry/IST stacks and the controlled TLB test window.

M3 provides a bounded sixteen-page anonymous window per address space. Reservation marks an allowed virtual extent without physical backing. Commit allocates zeroed pages before promising success, rolls back newly allocated pages on failure and rejects overlap/range errors. Decommit drops references; recommit returns zeros. Snapshot references committed pages in a new address space, makes both leaf maps read-only and copies on the first private CPU/kernel-copy write. This clones the explicit anonymous window, not an entire process/fork image.

Allocator bitmap updates/statistics are locked, with zeroing outside the lock. Only UEFI conventional memory below 1 GiB is managed; loader/firmware allocations remain reserved and separately reported. A privileged test admission limit forces real no-memory returns without allocating all configured guest RAM. Failed thread/snapshot/commit allocation is rolled back; an unresolved COW fault terminates that thread while survivors continue. Four preallocated emergency pages are kept outside ordinary allocation and checked intact before release. Bootstrap exhaustion remains a diagnosed kernel failure.

Cache reclaim, file-backed demand paging, compression, swap and long-running production OOM policy are later work after reliable storage. Conservative anonymous commit and tested fault cleanup are not evidence of disk paging or universal memory-pressure handling.

## Scheduling, clocks and failure isolation

M3 uses bounded per-CPU round robin and fixed process affinity. Kernel dispatch holds one scheduler lock with IF cleared and cannot be preempted on that CPU. Lock order is scheduler before allocator; the allocator does not call back into scheduling. A blocked scheduler releases the lock before atomic interrupt-enable/HLT, rechecks state after wake and never waits for another CPU while holding the scheduler lock.

Time comes from a checksummed ACPI-discovered 64-bit HPET, with its comparator interrupts disabled. Approximately 10 ms monotonic ticks are derived from elapsed counter values; missed interrupt delivery does not stop elapsed time. PIT IRQ0 wakes the BSP; calibrated local APIC timers wake APs. Timeouts use hardware time and are checked atomically with waiting state. Qualification enforces a five-tick lateness ceiling in an additional instruction-counted one-CPU run. Normal SMP runs report raw lateness, including emulator/host scheduling variability, without a hard real-time claim.

Manual-reset event signal wakes all waiters; auto-reset wakes one or retains a pending signal. Sleep, join and mailbox receive have real blocked states and deadlines. Signal-before-wait, wait-before-signal, reset, timeout, cancellation and handle close have explicit tested outcomes. Wait-any/all, futex operations and guest APC/signal/Mach exception semantics remain future work.

User faults and invalid return mappings terminate the affected thread, drop its wait/resources and preserve unrelated processes. Kernel invariant failures are diagnosed separately. M2's original ten hostile cases and the M3 fault/OOM/return cases remain acceptance gates; the finite suite does not prove universal malicious-code/race freedom.

## SMP and TLB lifecycle

Qualified CPU discovery uses bounded/checksummed RSDT/XSDT and MADT parsing. INIT/SIPI enters the recorded low-memory trampoline, loads kernel CR3 and a private stack, and installs per-CPU GDT/TSS/IST/IDT state. Up to four xAPIC CPUs are active; native CPL3 execution is checked on each. x2APIC, hotplug, migration and physical-platform compatibility remain unqualified.

Shared supervisor mapping replacement/unmap sends IPIs, invalidates the local page and waits for work acknowledgements before freeing backing. CPU-local user address-space edits invalidate on their owner CPU; no other CPU executes that address space. This fixed-affinity invariant avoids claiming general migrating-address-space shootdowns. DMA requires separate pin/lifetime/IOMMU work and is not implied by CPU TLB invalidation.

After the finite workload, AP timers are masked and APs acknowledge quiescence on retained stacks; BSP timer is masked too. Actual HLT on all four CPUs and host-QEMU idle time are observed. During blocking waits, timers remain active and schedulers sleep interruptibly.

## Evidence boundary

M0 validates wire layouts, negative inputs and host reference semantics. M1 proves firmware handoff/diagnostics; M2 its native isolation/rights/cleanup paths. M3 adds its bounded thread/wait/VM/SMP scope with actual guest execution, rollback, time/latency/resource measurements and fresh linked evidence. Guest compatibility, persistent storage, installation, desktop, hardware drivers and physical qualification remain separate milestones. No app target is promoted by a native probe.

Mailbox publication can precede IPI delivery. Each AP therefore performs the requested shared-page invalidation in the worker before acknowledging it, even if an earlier interrupt already invalidated the page. The SMP test deliberately delays doorbells and reports completions before delivery, exercising this ordering rather than assuming every worker woke from that IPI.

## Bounded pressure-response verification

The privileged native pressure probe keeps its allocator admission budget active while checking fresh AP timer progress. Missing progress starts a bounded 100-tick wait; the scheduler releases its lock and halts interruptibly between checks. COW-OOM teardown uses the same path before restoring the budget. A brief delayed VM CPU is tolerated, while the deadline negative still fails. [Mechanism and causal evidence](../milestones/m4.5-smp-pressure.md). This is not a guest ABI or a hard real-time guarantee.

## M4.5 USB DMA ownership

The shared original stack is under `kernel/usb/`. Device contexts admit at most eight live devices with lazily owned pages. Hub descendants retire before their parent, and DMA pages are reclaimed only after successful Disable Slot. UAS timeout stops old USB transfers before task abort; failures retain ownership and quarantine rather than freeing uncertain memory. Old disk generation references cannot access replacement contexts. [Exact profiles and limits](../milestones/m4.5-usb-topology.md).

The M45 `hardware` inventory queries the selected xHCI driver state, live route/class bindings, rejected ports and retained-DMA quarantine. Class discovery alone remains separate from initialization and verified storage transfer. The status reader allocates no pages or new queues.
