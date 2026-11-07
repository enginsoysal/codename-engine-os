# Repair, update or roll back Engine OS M7

Use the M7 ISO matching your distribution and compatible system-slot layout. Power off the VM, attach the ISO and boot it. At Welcome choose the operation, select your installed SATA disk, review the proposed action and Confirm. Keep the disk attached until success. Then power off, detach the ISO and boot the installed disk.

| Goal | Wizard action | Result |
|---|---|---|
| Restore damaged active boot files | Repair | Replays the verified active bundle without formatting user data |
| Activate another compatible M7 build | Update | Stages the ISO bundle in the inactive system slot, verifies it and activates it |
| Restore the previous verified build | Rollback | Reactivates the previous valid slot; one must exist |

These are explicit ISO-assisted operations. Interrupted boot-file replay can require another ISO Repair; this is not automatic A/B boot fallback. A failed operation must not be interpreted as successful installation. Read Details for the actual error.

## What remains yours

The native maintenance gate creates a real document and preferences, updates from standard M7 to the distinct Night kernel, boots its desktop, rolls back and independently checks the complete data partition SHA-256 at every stage. It then deliberately corrupts the owned test disk's BIOS payload, observes CRC boot refusal, repairs from ISO and boots the original desktop with exactly the same data-partition bytes.

Personal preferences and already installed optional programs are retained during this same-ABI update. Night's default Calculator omission applies to fresh installation; an update does not silently remove your existing Calculator or overwrite your saved appearance. Updates do not implement general dependency resolution, app/data schema migration or arbitrary incompatible kernels.

## Fresh installation versus update

For first testing, use a **new separate SATA disk**, following the [M7 VMware tutorial](install-m7.md). This release does not qualify migrating the earlier M5/M5.1 layouts or upgrading an M6 installation without its required M7 apps. Preserve an older test disk as a backup. Do not force-format an unknown existing disk or choose another disk merely because its capacity looks similar.

The optical ISO and raw USB image are different release assets. Factory Secure Boot and physical-hardware recovery are outside M7-VM-v1. [Exact release limits](../releases/m7-v0.7.0.md), [desktop tasks](desktop-m7.md).
