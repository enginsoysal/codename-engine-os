# M4 storage-console alpha.1

Historical transport-only increment. Superseded by [M4 v0.4.0](m4-v0.4.0.md); the statements below describe this alpha's original scope.

M4 is in progress. This release delivers the first original AHCI storage transport and PS/2/COM1 diagnostic console increment. It does not complete filesystem/input acceptance.

The own kernel discovers PCI AHCI by protocol class, identifies a marked 16 MiB test disk, and writes only its sector-1 CRC/generation record with flush and full readback. Four fixed DMA pages cost 16 KiB; input buffers are bounded and all four CPUs are observed halted while the console waits. Disk polling is bounded during commands; asynchronous block queues are pending.

QEMU tests cover reboot persistence, host-side CRC/payload verification, write-error injection and rejected/absent/corrupt volumes. Real PS/2 key events exercise editing and storage commands. Two additional VMware Workstation 26.0.1 SATA smoke boots verify persisted generations while preserving existing VM inventory. The full M3 regression suite is rerun separately.

Build/download assets include the bootable ISO, USB image, kernel/user ELF, map, source/artifact manifest and checksums. The extra data-template IMG/VMDK pair must be copied together into a disposable VM folder before attaching it as a writable SATA disk. The ISO alone opens the console with storage unavailable if no marked disk is attached. Read the [M4 usage and exact limits](../milestones/m4.md) and [test evidence](../../evidence/M4/README.md).

Still pending: writable filesystem/VFS and namespace semantics, file operations/locks/full-disk/crash recovery, native USB HID and mouse, broader queue/device/resource qualification. No installer, desktop or Windows application support is included. M3 remains the latest completed milestone and the default local launcher target.
