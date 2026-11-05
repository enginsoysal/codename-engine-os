# M3 preemption increment — alpha.1

Historical prerelease, superseded by [M3 v0.3.0](m3-v0.3.0.md). The text below describes the original alpha scope.

This is the first verified M3 increment, **not the completed M3 milestone**. It adds timer-driven involuntary switching between two non-yielding native processes on one active CPU after the inherited M2 isolation suite. Arithmetic flags survive return; temporary process/shared pages are reclaimed and the timer is masked at final HLT.

Use `EngineOS-M3.iso` in a 64-bit UEFI VM with Secure Boot disabled, one vCPU and 128 MiB RAM. The result screen identifies `M3 PREEMPTION INCREMENT` and reports timer preemption plus memory totals. The USB image is a disposable VM/device image, not an instruction to overwrite a host disk. Validate downloads with `M3-SHA256SUMS.txt`.

Expected workload: 47 starts, 37 exits, ten contained hostile faults and at least 16 timer preemptions; actual counts/resources and exact source/artifact hashes are in [M3 evidence](../../evidence/M3/README.md). The acceptance report explicitly leaves full M3 incomplete. Qualification uses pinned QEMU/OVMF, including ISO/USB, 64/512/2048 MiB and diagnosed failure paths; VMware is supplied for a separate user test.

Still pending in M3: SIMD state, real threads/TLS, blocking waits/deadlines, VM/COW/pressure and active SMP. PIT/PIC is a temporary PC-VM baseline. There is no installer, desktop, arbitrary program loading or existing Windows application support. Windows 95–11 remains the version/task-qualified product target. See [M3 implementation and remaining work](../milestones/m3.md).
