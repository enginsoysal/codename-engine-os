# M4 qualification fixes

These observations describe development failures, not passing acceptance evidence.

- The original transport-only report was superseded by writable ext2. Its unchanged-outside-sector-1 assertion correctly failed after filesystem writes were introduced; the current test instead independently validates ext2 allocation, graph and payload semantics.
- The first USB attempt refused a 64-bit controller BAR above 4 GiB. A dedicated supervisor/NX MMIO mapping now supports it. QEMU keyboard/mouse reports subsequently reached the original xHCI driver. A separate test harness failure used an invalid QMP slash key name; the harness now emits the documented qcode.
- Initial USB media boot left SATA uninitialized by firmware. The kernel now resets its own AHCI controller. A fresh selected USB boot passed after that change.
- The subsequent wrong-capacity rejection test timed out with the CPU repeatedly in the AHCI IRQ handler. A pending interrupt from a previously probed port was not acknowledged after the candidate port pointer changed. The handler now acknowledges every indicated source port and disables/clears a rejected port. Fresh selected rejection and full M4 acceptance runs pass after the fix; current JSON reports carry their own tested build identities. The [earlier timeout log](history/wrong-capacity-timeout-before-irq-fix.log) is retained.
- Fault injection also showed that an ATA task-file error may leave the command-issue bit set. The driver now recognizes the reported error immediately, offlines the controller and quarantines buffers, rather than waiting for the deadline and mislabeling the error as a timeout. Delayed requests without an error indication still exercise the separate timeout path.

The earlier alpha's host idle-sample excursion is separately retained in [regression-note.md](regression-note.md). A passing later sample does not explain that earlier host-process measurement.
