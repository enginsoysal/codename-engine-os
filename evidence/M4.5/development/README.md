# Prequalification observation

`prequalification-smp-pressure.log` records a failed exploratory storage run on build `2ec4f11df792aec558368a4786617958`. The existing M3 workload reported `CPU unresponsive under pressure` before storage initialization. Other exploratory transports passed. The log is retained instead of being presented as passing evidence or silently removed.

Current acceptance uses a later identified build and fresh complete suites. No scheduler condition was weakened to make this observation disappear. This single observation does not prove its cause or establish that all SMP timing faults have been eliminated; repeated failures must block qualification and receive investigation.

The first VMware NVMe attempt failed before kernel boot because the generated test VM lacked a PCIe root port (`No PCIe slot available for nvme0`). The isolated test configuration now explicitly provides that bridge. This is a VM-fixture correction, not evidence of an NVMe driver fix. Final per-profile results record the rerun.
