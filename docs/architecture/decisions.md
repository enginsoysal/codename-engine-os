# Architecture decision record

A selected approach is not a runtime claim. [Status](../roadmap/status.md) and [milestone evidence](../../evidence/M2/README.md) state what is implemented. Changes need rationale, affected contracts, migration and regression results.

| ID | Choice and implementation | Why | Alternative/tradeoff | Proof or decision gate |
|---|---|---|---|---|
| 001 | Own kernel/system services | Independent platform/resource policy | Existing kernel accelerates drivers but changes the product requirement | Product scope |
| 002 | Modular kernel, substantial policy in user services | Small trusted core without compulsory round trips | Pure microkernel adds IPC; large monolith broadens fault impact | Measure service/IPC costs at M3/M10 |
| 003 | Per-environment ABI adapters | Ecosystems differ in semantics, not just names | Superficial universal API loses behavior | Semantics matrix; guest implementation at M7+ |
| 004 | Windows first, complete 7-Zip path | Tangible original-installer proof | Four ecosystems at once spread effort | M8 end-to-end |
| 005 | x86-64 UEFI first | Matches initial Windows targets, bounded boot matrix | BIOS/ARM immediately multiply platform work | ARM boundaries retained for M16 |
| 006 | Own versioned native ABI | Internal evolution independent of guests | Making everything NT-shaped couples future platforms | Fixed headers/register/status contracts |
| 007 | Object rights and explicit environments | Compatibility must not grant all host authority | Global shared state is simpler but riskier | M2 native rights; future identity/namespace tests |
| 008 | Common VFS with semantic adapters | Shared storage without erasing differences | Global POSIX/Windows behavior breaks other expectations | M4/M8 path/locking/metadata tests |
| 009 | Existing documented writable format first | Storage integrity before inventing a filesystem | New format creates another research project | Bounded ext2 development profile; not implemented yet |
| 010 | Measure RAM categories before product budgets | Avoid misleading idle numbers | Arbitrary low targets can reward bad behavior | M2 owned/boot/idle reports, broader M3 workloads |
| 011 | Compression/swap after correct VM | Extra tiers must not mask ownership faults | Early complexity adds deadlocks/data-loss risk | M3+ pressure/recovery tests |
| 012 | Reuse appropriate open-source components | Own OS does not require reinventing every library | Everything-original increases maintenance/time | Record provenance/license/dependencies before ports |
| 013 | Shared live/install/recovery base | Avoid driver/configuration drift | Separate installer OS adds another stack | M5 installs the tested build |
| 014 | Version/task-specific app qualification | Reproducible failures and honest support claims | Generic 'app works' statements are not testable | WC1/WC7 records |
| 015 | No full guest-driver support in initial release | Drivers and user APIs are separate large contracts | Reusing `.sys` requires deep NT kernel support | WC6 keeps blockers visible in ultimate scope |
| 016 | C17 plus small assembly, Clang/LLD | Available cross-toolchain and direct ABI/porting fit | Rust improves some safety properties but adds tooling/FFI | ELF/EFI builds; own kernel/user runtime tests |
| 017 | Native root plus guest filesystem views | Independent layout and expected app paths | One global guest root couples all environments | Approved namespace contract; VFS later |
| 018 | Versioned read-only code, separate durable/cache/temp | Controlled updates and retention | Legacy writes need explicit compatibility layers | M5/M8 recovery/uninstall |
| 019 | Small own UEFI loader | Bounded control of handoff | Limine/GRUB reduce boot work | M1/M2 real ExitBootServices + retry |
| 020 | FAT32 EFI, bounded ext2 development storage | Standard boot plus manageable first persistence | NTFS/ext4/new format are larger first projects | Boot media tested; ext2/fsck/crash work M4/M5 |
| 021 | Pinned QEMU q35/TCG with OVMF | Repeatable without host acceleration | Passthrough couples to host | Binary/DLL/firmware hashes and actual media loads |
| 022 | Trusted process-profile dispatch | Same syscall number can mean different things | Imports-only interception misses direct calls | Native M2 dispatch; guest decoders later |
| 023 | x86 compatibility mode for future PE32 | Original 32-bit code on x64 | CPU emulation costs more; x64-only excludes apps | Thunks/TLS/stack proof before Steam |
| 024 | Conservative initial backing | Do not promise unbacked memory | Aggressive overcommit defers failures | M2 fixed backed pages/quotas; general M3 policy |
| 025 | pycdlib host-only ISO author | Small pure-Python EFI El Torito route | xorriso needs extra current-host setup | Pinned wheel, genuine firmware loads; LGPL tool |
| 026 | Explicit EFI fixture before kernel | Separate tool/firmware from kernel faults | Immediate combined boot obscures diagnosis | M0 marker is not called a kernel |
| 027 | Generation inventory + later runtime gates | Preserve broad Windows scope without invented support | 'All apps' from samples is invalid | Eight generation groups, empty third-party register |
| 028 | Fixed 2 MiB ELF bootstrap/low identity map | Simple first handoff and diagnostics | Higher-half/general mapping adds initial complexity | M1 bootstrap broad RWX; M2 hardens before user entry |
| 029 | Assembly exception entry with TSS/IST | Explicit CPU context/error-code handling | Printed stubs alone prove no real faults | M1 #UD; M2 user #PF/#GP isolation |
| 030 | Fixed initmanifest with build/test mode | Small validated boot input | General archive parser unnecessary initially | Normal mode 0; test modes explicit |
| 031 | Single-CPU cooperative native M2 | Small understandable context/cleanup proof | Preemption/SMP immediately enlarge race surface | Non-yield starvation remains an M3 limitation |
| 032 | Shared immutable user code, private data/stack | Lower per-process RAM without losing isolation | Duplicate code wastes pages; shared private data leaks | Same-VA sentinel tests and measured cleanup |
| 033 | Two bitmap page maps, conventional memory only | 64 KiB fixed metadata, no heap per page | Reclaiming firmware early is unsafe; broad frame objects cost RAM | Zeroing, bounded allocation and page balance |
| 034 | Temporary native `int 0x80` gateway | Correct privilege/context boundary before fast-path tuning | `SYSCALL` needs additional MSR/per-CPU entry design | Checked native ABI; future transport/guest decoders separate |
| 035 | Fixed quotas and explicit nonblocking IPC | Bound resident resources and remove polling | Unbounded queues hide exhaustion; waits require scheduler work | Handle/queue saturation, rights/type/pointer failures |
| 036 | HLT after finite verification, separate memory/CPU metrics | Idle cycles must not be wasted; guest RAM is not footprint | Busy loops or one combined number conceal cost | QMP halt state, measured host CPU interval and page categories |
| 037 | English docs in structured directories | Consistent public project and reviewable navigation | Mixed languages/root clutter drift | Current-doc language/link/status gate; no rewriting historical tags |
| 038 | Native user VA above 4 GiB and upper-half entry stack | Preserve boot mappings across larger VM memory settings; switch off the firmware stack before process CR3 | Reusing a low identity range can shadow retained boot resources | 64/512/2048 MiB boot scenarios; 32-bit guest layout remains future work |
| 039 | Disable unconfigured SYSCALL/SYSENTER entry | No unintended privilege gateway before fast-path design | Leaving firmware MSRs implicit broadens the attack surface | Real native instructions fault in user mode; future gateways require deliberate setup |
| 040 | Separate single-CPU PIC/PIT preemption increment | Prove involuntary switching and flags before concurrent kernel entry | APIC/SMP immediately complicates entry/locking; cooperative yield cannot prove preemption | Two non-yielding native probes and carry-state checks; timer masked at final idle; APIC/SIMD/threads/waits/VM remain M3 gates |
| 041 | Eager FXSAVE64/FXRSTOR64 and per-thread FS TLS | Preserve CPU ownership without lazy-FPU leakage; bounded 512-byte save area | XSAVE/AVX requires larger negotiated state and feature policy | x87/XMM0/XMM15/MXCSR/TLS probes; extended XSAVE disabled |
| 042 | Fixed process affinity, per-CPU entry stacks, serialized kernel | Safe page/handle teardown and simultaneous native execution before scalable locking | Migration/fine-grained locks add remote lifetime races | Native execution mask on 1/2/4 CPUs; lock order scheduler then allocator |
| 043 | HPET elapsed time plus PIT/APIC delivery | Monotonic deadlines survive missed/coalesced timer interrupts | Counting IRQ deliveries loses elapsed time; TSC needs broader frequency/synchronization qualification | Masked-interrupt catch-up, actual waits and reported bounded lateness |
| 044 | Atomic wait state and referenced objects | Eliminate the test/enqueue gap and keep waited objects alive | Returning would-block alone is not a wait implementation | Signal-before/after, reset, timeout, close/cancel, manual wake-all and blocking mailbox |
| 045 | Backed anonymous commit, COW and admission-budget pressure | Explicit failure before success; zeroed reuse and bounded recovery without exhausting host RAM | Overcommit hides failure; storage paging requires M4 | Commit/thread/snapshot rollback, CPU/copy-out COW, contained OOM, intact four-page reserve |
| 046 | ACPI MADT INIT/SIPI with per-CPU GDT/TSS/IST/APIC | Real secondary execution and independent interrupt state | Configured vCPUs alone prove no SMP; x2APIC/hotplug enlarge initial scope | 1/2/4 actual native CPU mask, AP SIMD resume, guarded entry/IST stacks |
| 047 | Acknowledged IPI invalidation before backing release | Avoid stale shared supervisor translations after replacement/unmap | Bare PTE writes do not invalidate remote TLBs | Warm/read/replace/read/unmap on APs; user spaces remain affinity-bound |
| 048 | Measured bounded native M3 profile | Keep resource/latency claims reproducible and distinct from guest compatibility | Unbounded APIs or a splash screen conceal missing behavior | 8 threads, 2 spaces, 16 anonymous pages/space; exact counts/budgets, 256 warm dispatch samples |

| 049 | Feature-detected RDTSCP/TSC_AUX CPU index and demand-driven HPET reads | Remove per-yield APIC/HPET MMIO overhead while retaining hardware-clock wait semantics | Cached clock ticks alone lose deadline accuracy; GS-based entry would add SWAPGS complexity | CPUID-gated MSR readback on each CPU, APIC fallback, unchanged 64-sample user round-trip benchmark and full regressions |

## Detailed contracts and remaining choices

[Kernel](kernel-contracts.md), [filesystem](filesystem.md), [build/platform](build-platform.md), [compatibility](compatibility.md), [M2 implementation](../milestones/m2.md), [M3 implementation](../milestones/m3.md).

Before physical tests: select an explicit test computer; never use the development disk as an implicit target. Post-M3: migration/finer locking, extended XSAVE and file-backed reclaim after storage. M4/M5: writable format implementation, integrity/recovery and update transactions. M10: GPU family/driver/API chain. M11–M15: exact app versions, accounts, tool/SDK access and external dependencies. M16: exact Pi model/firmware. Component revisions are selected before actual ports; none is invented simply to close a planning checkbox.
