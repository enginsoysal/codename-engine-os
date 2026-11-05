# Additional VMware Workstation qualification

Observed VMware Workstation 26.0.1, build 25688693, with the exact normal M3 ISO/build identity in each report. A separate diskless/networkless EFI VM uses virtual hardware 20, FreeBSD-64 guest profile, Secure Boot disabled, HPET enabled and 128 MiB RAM. Existing running VM inventory is checked before/after; only the newly owned test VM is stopped.

| Active CPUs | Report | Guest serial evidence |
|---|---|---|
| 1 | [Four checks](1cpu-results.json) | [Log](1cpu-serial.log) |
| 2 | [Four checks](2cpu-results.json) | [Log](2cpu-serial.log) |
| 4 | [Four checks](4cpu-results.json) | [Log](4cpu-serial.log) |

Every case reaches `M3_COMPLETE_PASS`, records native execution on each CPU, checks exact workload/resource budgets and has zero observed deadline lateness in these samples. Peak managed pages are 456 KiB; boot reservations 676 KiB; retained managed pages 60/92/156 KiB respectively. Guest RAM, firmware, framebuffer and host memory are different categories. The reports preserve raw values and return-preparation TSC samples; these are not calibrated wall-time or cross-hypervisor performance rankings.

No VMware Tools are installed. The CLI framebuffer capture is unavailable (`screenshot_captured: false`), so this is serial/runtime qualification; the actual captured framebuffer/idle-register checks are supplied by QEMU. It is additional evidence for these configurations, not every VMware release/device profile or physical hardware.

Optional rerun when VMware is installed: `python tests/m3/vmware.py --cpus 2` (also 1 or 4). It creates/reuses only its dedicated ignored `build/m3/vmware/<cpu-count>cpu/` workspace, starts with `nogui` and leaves existing VMs untouched. VMware binaries/configuration/private VM logs are not redistributed.
