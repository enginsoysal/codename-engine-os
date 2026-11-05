# Context-switch measurement and CPU optimization

The same native benchmark ELF (SHA-256 `c82c7f44394c65fbd9295a1b30d32e1684a243932ba7ffc9aa098c9b87de963f`) was used before and after the kernel optimization. Its two threads share one CPU/address space, pass a manual-event gate, exchange 40 yields each and record 64 middle round-trip samples after excluding four initial/final iterations per thread. Values include both gateway/scheduler/context paths, peer instrumentation and any interrupt overhead; they are TSC deltas, not calibrated nanoseconds or isolated one-way switches.

| VMware 26.0.1, build 25688693; 2 vCPUs, 128 MiB | Minimum | p50 | p95 | Maximum |
|---|---:|---:|---:|---:|
| [Baseline](https://github.com/enginsoysal/codename-engine-os/blob/8163b8249ff93d36595c72b7dc985a7e66359851/evidence/M3/vmware/2cpu-results.json) | 237297 | 256253 | 319801 | 356035 |
| [Optimized](../vmware/2cpu-results.json) | 2858 | 2880 | 2983 | 3643 |

The observed median ratio is approximately 89.0 for this specific yield benchmark. It is not an OS/application-wide speedup or a cross-machine performance guarantee. Host scheduling, timer delivery and virtualization can affect samples. Both reports retain build IDs, full counters, memory categories and tool identity.

The baseline read the APIC ID and HPET through device mappings on every yield path. The optimized kernel uses CPUID-advertised RDTSCP plus programmed/read-back TSC_AUX for CPU identity, with APIC lookup as a fallback. It reads HPET for timer delivery, time/deadline requests and wake/cancel ordering; a plain yield needs neither device read. Same-address-space selections also avoid unnecessary CR3 writes. Eager FPU isolation, checked return, rights and conservative mapping lifetimes remain enforced.

The complete boot/SMP/negative-control/low-RAM/clock/isolation suite, M0/M2 regressions and VMware 1/2/4-CPU profiles pass after the change. The stale-TLB negative-control still fails at its expected diagnostic, so removing incidental flushes/device accesses does not replace required explicit invalidation. Managed peak remains 456 KiB; exact boot-reserved changes and runtime-only peaks are reported separately.
