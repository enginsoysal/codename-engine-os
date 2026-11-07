# Architecture decision record

A selected approach is not a runtime claim. [Status](../roadmap/status.md) and [milestone evidence](../../evidence/M2/README.md) state what is implemented. Changes need rationale, affected contracts, migration and regression results.

| ID | Choice and implementation | Why | Alternative/tradeoff | Proof or decision gate |
|---|---|---|---|---|
| 001 | Own kernel/system services | Independent platform/resource policy | Existing kernel accelerates drivers but changes the product requirement | Product scope |
| 002 | Modular kernel, substantial policy in user services | Small trusted core without compulsory round trips | Pure microkernel adds IPC; large monolith broadens fault impact | Measure service/IPC costs at M3/M10 |
| 003 | Per-environment ABI adapters | Ecosystems differ in semantics, not just names | Superficial universal API loses behavior | Semantics matrix; guest implementation at M8+ |
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
| 050 | AHCI transport before writable filesystem | Exercise original PCI/DMA/flush ownership first | PIO avoids DMA ownership; NVMe/virtio widen initial scope | Historical alpha used sector 1 only; current version-2 profile validates ext2 before broader writes |
| 051 | Bounded synchronous first storage increment | Establish failure behavior before completion IRQs | Polling costs CPU during I/O | Superseded in the qualified MSI path by one outstanding IRQ/HLT request; polling fallback remains explicit |
| 052 | PS/2/COM1 diagnostics before USB HID | Test editing and transport interactively first | Kernel diagnostics are not a user shell | Current M4 adds own xHCI boot keyboard/mouse; ASCII console remains separate from UTF-8 storage |
| 053 | Bounded ext2 with explicit feature allowlist | Original writable filesystem before guest storage semantics | NTFS/ext4/new journaling format substantially expand initial scope | One group, 4 KiB blocks, 512 inodes, 12 direct pointers; independent host graph/map checker |
| 054 | Dirty-marker flush and offline recovery to a new copy | Refuse unsafe further writes and preserve the source image | No journal means arbitrary crashes cannot be promised atomic | Error after dirty marker, full disk, delayed-write quarantine and recovered boots; ambiguous graph damage refused |
| 055 | Exact UTF-8/case and centralized inode handles | Avoid accidental collision/lock bypass before guest adapters | Guest normalization/ACL/link rules differ and remain separate work | Case/NFC/NFD names, malformed paths, stale/quota/share handles, rename/delete while open |
| 056 | xHCI boot HID, MSI-X/MSI and high MMIO mapping | Own USB controller and event-driven input with bounded DMA | Hubs/hotplug/arbitrary reports and physical controllers widen lifetime/routing scope | Two devices/four attempts, supervisor/NX MMIO window, key/mouse reports and HLT observations |
| 057 | Four-KiB DMA batches and temporary verification mappings | Reduce sector command overhead and retained test RAM | Permanent large buffers waste boot reservations | 16 KiB AHCI buffers; self-test payload/copy pages unmapped, TLB invalidated and reclaimed |

| 058 | Original BIOS loader normalizes to the existing boot contract | Share one protected kernel without requiring UEFI firmware | Third-party bootloaders would reduce work but add an external runtime dependency | Dual El Torito ISO/GPT USB, EDD sector-size checks, bounded E820 and pre-copy memory validation; named runtime and negative reports |
| 059 | Release temporary UEFI ELF input after segment copying | Remove duplicated retained file/image memory | Retaining the input simplifies loader lifetime but costs RAM | Exact reservation removal before ExitBootServices; signed-payload verification precedes the release |
| 060 | Separate read-only protocol drivers and partition/filesystem parsers | Ordinary media inspection must not imply write authority | A marker-only driver hides usable disks; arbitrary write support is premature | AHCI/NVMe/ATA/VirtIO first-sector and boundary reads, GPT/MBR checks and unchanged ordinary backing hashes |
| 061 | Small modern VirtIO split queues and MSI-X/HLT completion | Efficient virtual storage with bounded per-device DMA state | Packed queues and deeper concurrency expand ownership complexity | Eight descriptors, one request, explicit version/feature negotiation, 512/4096-byte reads and repeated queue wrap |
| 062 | Owner-enrolled disposable VM trust qualification | Prove firmware-to-payload authentication without modifying host policy | Self-signing is not factory trust; production enrollment needs an explicit owner workflow | Valid signed loader, unsigned/tampered PE rejection and external kernel/init digest rejection; the original pre-closure scope remained open at that checkpoint; decision 071 governs current completion |

| 063 | USB BOT read-only transport before UAS/hubs | Add common removable-storage reads with bounded protocol ownership | More classes and topology require separate endpoint/lifetime qualification | CBW/CSW validation, 512/4096-byte reads, IRQ/HLT, native GPT/FAT32 and EIO tests |
| 064 | Deferred root-port work, Disable Slot before free and generation-tagged readers | Reclaim unplugged DMA without aliasing old disk references to a new device | Reusing disk numbers hides stale-reference bugs; freeing before hardware acknowledgement is unsafe | Repeated storage/HID reconnect, seven-page reclamation, stale reference refusal and healthy replacement after EIO |
| 065 | Separate extended read-only ext2 reader | Widen geometry and indirect/sparse reads without silently widening write authorization | General writable metadata/recovery needs a distinct integrity design | Native multiple-group/geometry tests and corrupted metadata rejection; original writable development profile unchanged |

| 066 | Bounded interruptible AP pressure-response check | A brief delayed VM CPU must not be mistaken for a permanently missing CPU after only three ticks | An immediate timer-count assertion has a hidden host scheduling bound; unbounded waiting hides failure | Exact pre-fix source reproduction, short owned-AP pause succeeds, long pause still fails, no pages allocated and scheduler lock released across HLT |

| 067 | Shared USB protocol folder, bounded hub tree and lazy device DMA | HID and storage share controller ownership; allocation should follow actual attached devices | Fixed eight-context admission and five route nibbles limit topology; physical TT/SS hubs remain unqualified | Direct/five-hub native reads and input, leaf/whole-tree reconnect, exact child-before-parent reclamation |
| 068 | Serialized read-only UAS with independent data/status streams and explicit task abort | Support common UAS devices without unbounded queues or retained failed SCSI commands | A timeout alone does not release device/controller ownership; old USB transfers must stop before task cancellation | High/super-speed 512/4096 reads, queue wrap, EIO/deadline injection, abort response, safe replacement, exact eight-page lifetime and idle checks |

| 069 | Pin and reuse an encrypted owner signing identity across payload builds | A fresh self-signed identity per build cannot support an enrolled update policy | A trusted signing host is required; one-use private PFX has an owner-only DACL and cleanup but no secure-erasure guarantee | Wrong pin/password refusal, changed signed init boots under unchanged VM policy, old digest rejects it, private staging cleanup after failure |
| 070 | Authenticated db rotation and explicit authority-role checks in disposable firmware | Prove firmware authorization, retired signer rejection and anti-replay beyond initial boot | The helper is TCG-only; physical enrollment/recovery and factory trust require external qualification | Exact db readback, unchanged PK/KEK, new signer boot, retired signer/foreign update/replay refusal and original-policy snapshot recovery |

| 071 | User-approved M45-VM-v1 completion contract (2026-11-06) | Protocol behavior and failure/resource handling can be qualified against named virtual endpoints without physical hardware blocking milestone closure | Explicitly excludes physical/factory-trust/production-key acceptance, ARM64 and arbitrary hardware support; older preview scope remains historical | Exact final ISO tested in five VMware profiles, all 29 gates and 23 trust cases; completion flag carries scope ID/exclusions and evidence hashes |

## Detailed contracts and remaining choices

[Kernel](kernel-contracts.md), [filesystem](filesystem.md), [build/platform](build-platform.md), [compatibility](compatibility.md), [M2 implementation](../milestones/m2.md), [M3 implementation](../milestones/m3.md).

Before physical tests: select an explicit test computer; never use the development disk as an implicit target. Post-M3: migration/finer locking, extended XSAVE and file-backed reclaim. M4 has a bounded writable ext2 profile and offline recovery; M5 adds installation/update activation and broader recovery, while production journaling/integrity remains additional work. M10: GPU family/driver/API chain. M11–M15: exact app versions, accounts, tool/SDK access and external dependencies. M16: exact Pi model/firmware. Component revisions are selected before actual ports; none is invented simply to close a planning checkbox.

## M5 decisions

| ID | Choice | How and why | Alternatives and limits | Verification |
|---|---|---|---|---|
| 072 | Original sparse live installer bundle and smaller installed kernel | Encode zero pages without literal storage; share source identity and write bounded four-KiB batches | Reading optical media from a new ATAPI driver or retaining a full disk image adds code/RAM; public media remain unsigned | Enforced BIOS image bound, payload CRC/bounds, independent installed ELF SHA256 and raw BIOS bytes |
| 073 | Blank whole-disk SATA plan and exact confirmation | Bind disk index, PCI/port/capacity and sector-zero snapshot; revalidate before first write | General partition editing, force wipe and dual boot substantially widen data-loss risk | Whole-image no-write hashes for refusal/cancel and wrong-disk confirmation |
| 074 | 128-page volatile overlay plus bounded persistent ext2 | Allocate changed live pages only; reuse the qualified native filesystem in a separate partition | Larger files/volumes and journaling remain necessary before real application packages | Real quota/reuse/reclamation and low-RAM/idle observations; detached-boot file persistence |
| 075 | Pending/complete records and same-build boot repair | Flush intent, verify copied bytes and publish completion last; keep initialized user data outside repair extents | Does not make boot writes atomic or implement cross-version upgrades/rollback | Real SATA copy EIO/resume, entire data-partition hash before/after repair, recovered installed boot |
| 076 | Refuse automatic writable selection with multiple eligible SATA targets | The existing boot header has no firmware boot-disk identity; do not guess another disk after damaged metadata | Carrying authenticated volume identity through both firmware paths is future work | Multiple-disk negative guest leaves both images unchanged |

## M5.1 decisions

| ID | Choice | How and why | Alternatives/limits | Verification |
|---|---|---|---|---|
| 077 | Guided finite-state text setup before a graphical desktop | Real numbered tasks over the existing IRQ/HLT loop remove mandatory terminal syntax without a GUI stack | Keyboard text UI, not a human usability study or complete accessibility solution | Menu-only Try/install/cancel/persistent boot/repair and readable error/details tests |
| 078 | Strict data-only distribution recipes | Compile selected branding/colour and populate actual live/installed files; hash exact inputs into identity | No inert desktop/plugin choices or executable install hooks; richer packages require their own runtime | Independently built/installed Forge ISO, pixel accent/content checks and unsafe/unknown recipe refusal |
| 079 | Preserve the existing destructive-operation boundary under the wizard | Generate the internal sector confirmation from a revalidated selected plan; back invalidates it | Convenience must not force-wipe unknown existing disks | Full backing hashes on cancellation/refusal; real write EIO; recovery preserving the data partition |

## Owner roadmap revision — 6 November 2026

**Choice:** Engine OS delivers M6 real product installation, then M7 real desktop, then M8 original MS-DOS and Windows 1.x–11 applications. Earlier future milestone numbers are superseded by the [current contracts](../roadmap/milestones.md).

**How:** remove prototype storage limits, qualify persistent independent installation and recovery, then qualify complete graphical daily tasks before original application compatibility. DOS, early Windows, 9x and NT each require their own runtime evidence.

**Why:** the owner prioritizes a usable installed OS and desktop over expanding compatibility on a console prototype. Existing completed technical gates remain historical evidence, while future completion requires real user workflows, resource measurements and declared hardware limits. This revision implements no new runtime and changes no released ISO.

## M6 initial capacity planner

**Choice/how:** a freestanding, allocation-free C planner produces a versioned BIOS/EFI, two-system-slot and remaining-capacity data layout. Support starts with 512-byte SATA targets from 2 GiB to 2 TiB. It performs no I/O, refuses invalid targets without changing its output, and accounts for every sector.

**Why/alternative:** continuing the fixed 16-MiB demonstration volume cannot support useful desktop documents or application downloads. Two 512-MiB base-system slots prepare an inactive-copy recovery mechanism without duplicating user data; a single mutable system partition would make update recovery harder. These are initial base-image budgets, not implemented activation/rollback or unlimited image capacity. The shared native planner avoids diverging host/kernel arithmetic.

**Verification/limits:** actual C host execution across 10,005 geometries and refusal cases, plus freestanding kernel compilation; [M6 contract](../milestones/m6.md). This was the initial planner checkpoint. Installer integration, scalable storage, boot/update recovery and runtime resource qualification were subsequently completed in M6; see [exact acceptance](../../evidence/M6/acceptance-results.json).

## M6 native implementation decisions

**Wizard:** use original framebuffer rendering and actual keyboard/USB mouse input in the kernel, following the accepted design. The browser prototype remains a reference; it is not bundled as a browser dependency. Choices persist across Back/Next, writes begin only at Install/Confirm, and errors cannot display Finish success. Optional terminal/details are retained.

**Storage:** use a bounded, original copy-on-write extent volume with 4-KiB streaming, two metadata generations and explicit integrity checks. This replaces the fixed 16-MiB/direct-file demonstration writer. It trades an initial 1,024-entry/contiguous-extent limit for constant metadata RAM and independently testable recovery; broad POSIX/Windows semantics remain future adapters.

**Updates:** keep verified boot bundles in two system slots and redundant activation records. Update/rollback/repair replay verified boot assets from the ISO without formatting data. An interrupted replay may require ISO repair rather than automatic boot fallback. Known failed/torn-write models and actual guest transitions govern claims. [Mechanism, alternatives and limitations](storage-m6.md).

**Customization:** persist the chosen local name/accent, make guide data optional/removable and carry supported distro data into the selected user namespace. Do not label a guide checkbox an executable package manager, or a local profile an authenticated multi-user system.

## M7 native desktop decisions

**Rendering:** use an original eight-slot, generation-bound window model and CPU framebuffer regional repainting. A full backing surface for every window would multiply RAM; damage clipping keeps the initial path bounded. Idle uses interrupts/HLT, without animation polling. GPU acceleration remains separate device work.

**Applications:** install separate original Editor/Calculator ELF programs with private CPL3 address spaces, guarded stacks, checked five-page mailboxes and a timer-bound synchronous event ABI. Share immutable code and reclaim private pages after faults. Fully trusted callbacks were rejected as isolation proof; a general scheduler/third-party service ABI remains beyond this shipped worker model. Trusted UI/network/file services are explicitly identified.

**Files and packages:** broker current-directory creation, validated Save as and safe explicit deletion; protect changed/open files and nonempty folders. Optional Calculator owns no shared system dependency and can be truly removed, with an explicit retained offline cache. Arbitrary hooks/dependency migration are not implied. Schema-2 profiles change actual installed defaults and optional selection; a separately booted Night image proves the behavior.

**Networking/security:** original bounded Intel 8254x/82574 DMA, IPv4 configuration and TCP/HTTP integrate a nonblocking unchanged BearSSL TLS 1.2 engine. RDRAND and explicit UTC/root/hostname checks fail closed. MSI is the qualified VMware route; unsupported legacy routing is refused. Bounded retries and a 4-KiB COW volume stage trade throughput for predictable RAM; no whole-file managed download buffer is needed. Public third-party licenses/source remain available alongside binary releases.

**Qualification:** exact committed source and media digests, automated complete native tasks and independent disk/packet/cryptographic adapters distinguish implementation from claims. VMware visual input/capture targets only owned loopback guests, never the host desktop. All 26 final gates pass, and 146 old accepted proofs remain unchanged. Named VM scope is deliberate; universal hardware, human usability and original DOS/Windows execution require their own evidence.


## M7.1 installer disk management

**Choice/how:** original bounded GPT/primary-MBR backend with read-only plans, device/metadata revalidation, verified writes, explicit review/consent and safe cancel. FAT32 metadata is built directly; Engine data formatting reuses the existing native-volume workspace. The GUI separates individual data operations from whole-disk replacement.

**Why/alternatives:** refusing every existing disk forced users into external tools and did not satisfy convenient installation. Silently treating formatting one partition as permission to repartition a whole disk would lose unrelated data. Retaining the qualified boot/system-slot layout and providing an explicit replacement route avoids claiming unimplemented dual-boot placement or filesystem relocation. Noncanonical/ambiguous layouts and protected entries fail closed.

**Verification/limits:** 15 exact-artifact gates include 902 failed/torn writes, independent table/filesystem decoding, native operation/cancel tasks, two equal-capacity disks, actual VMware BIOS/UEFI and retained desktop/custom/update behavior. All 322 previous frozen proofs remain unchanged. Quick format/metadata clear are not secure erasure; torn writes may require explicit repair. [Design](disk-manager-m71.md), [user manual](../handbook/disk-manager-m71.md), [acceptance](../../evidence/M7.1/acceptance-results.json).
