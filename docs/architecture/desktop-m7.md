# M7 desktop and native application boundary

Status: implemented desktop/app development slice; networking and complete M7 acceptance remain pending. [Milestone](../milestones/m7.md), [native task evidence](../../evidence/M7/isolation/results.json).

## Windows and painting

An original eight-slot window model owns generation-bound identities, stacking, focus, bounds and minimization. A closed identity cannot control a reused slot. Widened geometry arithmetic clamps hostile coordinates, and the taskbar remains reachable. The native renderer clips each window to its bounds and the invalidated screen region, then draws intersecting windows in stacking order. It allocates no full-screen surface per window. The original bitmap alphabet now includes desktop punctuation and arithmetic operators. There is no permanent animation or caret timer: an unchanged desktop executes interrupt-aware HLT.

The compositor, launcher, Files and Settings are currently trusted supervisor services. This keeps the initial driver/storage/display integration small. Their faults are not contained like application faults. This is an explicit tradeoff, not a claim that every desktop service is already an isolated process. Untrusted drawing command streams, third-party executable loading and general desktop service restart are separate extensions.

## Actual user programs

Text Editor and Calculator are separately compiled original x86-64 ELF files installed under `/Applications/TextEditor/program.elf` and `/Applications/Calculator/program.elf`. They are loaded from the native volume, verified for file integrity, and parsed with strict segment/entry/size limits. Their executable pages are read-only, constants are non-executable, and private mailbox/stack pages are writable and non-executable. Kernel/device mappings remain supervisor-only. Code pages can be shared by instances of the same program; private state is zero-initialized and reaped independently.

The UI broker sends fixed-size event messages to a five-page private mailbox. The app owns its editor/calculator state there and returns only through its restricted completion trap. The broker validates lengths, cursor positions, flags and termination before accepting a bounded display snapshot. Apps receive neither arbitrary kernel pointers nor direct storage/device access. The editor's path is selected by the broker; document I/O and clipboard access occur through those controlled user actions. A CRC baseline refuses overwriting a document changed by another editor instance.

Calls run in CPL3 on the bootstrap CPU, with distinct address spaces, a guarded stack, saved x87/SSE state and a separate supervisor transition stack. A ten-tick PIT budget bounds a busy call. The watchdog must receive its user-mode timer before storage wait handlers, which also consume PIT interrupts; the failed initial routing test and correction are retained. Faults, timeouts and invalid responses terminate only that instance and free its owned pages. This synchronous event-worker ABI is deliberately narrower than a general native application scheduler. It does not implement Windows, DOS, AVX state or arbitrary syscall services.

## Optional component removal

The current package operation knows the shipped native components. Calculator is optional; Editor and the supervisor base cannot be removed through it. Removal closes Calculator instances and deletes its actual installed executable. Editor, Files, Settings and user documents remain available. Personal data is retained. An offline installer cache under `/System/Packages` permits reinstalling Calculator; keeping that cache is explicit and is not an executing app dependency. No shared runtime is owned exclusively by Calculator.

The original COW volume commits the executable namespace change. On restart the executable's actual integrity/presence governs launcher availability, so an unsaved presentation preference cannot resurrect a removed program. A corrupt/missing cache makes reinstall fail without accepting a partial app. General dependency resolution, arbitrary executable install hooks and cross-version app/package migrations are not implemented by this bounded operation.

## Qualification and identity

Native tests perform useful Editor tasks across detached restarts, Calculator arithmetic, a visible real UD2 crash, an infinite-loop deadline, forbidden kernel/code writes, malformed output, private-page reclamation, required-component refusal, executable removal, clipboard use and offline reinstall. Independent GPT/table/payload CRC checks verify actual files. These app checks do not close M7's remaining networking and resource/pressure matrix.

The live installer contains its installation payload and therefore has different data addresses from the installed kernel. M7 publishes separate installed ELF/map artifacts; debugger/resource tests must use the matching installed map. Using the live map initially produced a false empty-window observation, retained as failed harness evidence. Media identity alone is not a substitute for choosing the correct linked image.

The M7 supervisor mapping permits a bounded four-MiB kernel span with per-page W^X/NX protection. BIOS authoring independently bounds the actual raw image to its installed boot area and checks that USB boot payloads cannot overlap the EFI partition. Earlier release artifacts and their accepted evidence are unchanged.
