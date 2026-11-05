# M3 first-increment evidence

This directory qualifies **single-active-CPU native timer preemption only**. Full M3 (threads/TLS, SIMD state, waits/deadlines, general VM/COW/pressure and SMP) remains in progress. No installer or Windows application is implemented.

Run `python tests/m3/verify.py`. [Acceptance](acceptance-results.json), [boot/error cases](boot-results.json), [normal framebuffer/idle](visual-results.json), [build provenance](build-manifest.json) and [screenshot](boot-screen.png) are generated from fresh runs. The native workload combines the inherited 45-start isolation suite with two non-yielding processes, a shared 16-step handshake and carry-state checks. Counts and page budgets are asserted in every successful ISO/USB/low/large-RAM/retry case.

See [implementation, limits and remaining gates](../../docs/milestones/m3.md). A passing increment is not complete M3 acceptance. Automated VM scope is pinned QEMU/OVMF; VMware requires a separate observation. Reports contain sanitized project-relative paths. Published M2 release artifacts remain tied to their original tag; current M2 reports may be refreshed as development regressions.
