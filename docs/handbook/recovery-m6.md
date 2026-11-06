# Recover or update an M6 installation

Boot the Engine OS M6 ISO. At Welcome, choose the operation below, select your installation disk, review it and Confirm. Keep the disk connected until the operation reports completion. Then power off, remove the ISO/USB media and start the installed disk again.

| What you need | Choose | What changes |
|---|---|---|
| Restore damaged or incomplete boot files | Repair | Replays the active verified boot bundle; the data partition is not formatted |
| Install another compatible M6 build | Update | Stages this ISO's boot bundle into the inactive system slot, verifies it and activates its version |
| Return to the previous installed build | Rollback | Restores the previous verified slot; a previous valid slot must exist |

These operations use the same M6 layout and native volume ABI. They do not migrate M5/M5.1 ext2 test disks or arbitrary future formats. Keep an earlier test disk as a backup and use a new empty SATA target for first-time M6 installation.

## If installation did not finish

A failed copy/write must not be treated as a completed installation. Reboot the same ISO, choose Install and review the identified incomplete target again. Its uncompleted setup may be recreated. A completed installation is not automatically erased by this route.

If the ownership record or necessary recovery slot cannot be validated, the wizard refuses the operation. Keep the disk unchanged for diagnosis; do not run host partition commands against it. Unknown existing partitions require another new virtual target rather than silent erasure.

## If an update was interrupted

Some or all boot files may already have changed even though activation did not finish. An explicit boot-version mismatch means the system refuses to open user data under an uncertain version. Boot the ISO and choose Repair to restore the active verified version. Automatic bootloader A/B fallback is not implemented.

Update, rollback and repair preserve the data partition; they do not repair arbitrary damaged documents. If file/volume integrity fails, keep a copy of the virtual disk before further work. The qualified injected-write model does not establish recovery from every physical power-loss or hardware fault.

## If a disk is missing

Power off VMware, check that the target is a SATA disk with 512-byte sectors and at least 2 GiB, then boot the ISO again. Do not change disks while the wizard is open. If a disk contains an older M5/M5.1 install, attach a new empty target for M6.

## Optional troubleshooting

Details shows build identity and memory information. On an installed system, the storage-test buttons create or verify a document over 2 MiB. Advanced terminal is optional; ordinary installation and recovery use the graphical choices. See [the installation guide](install-m6.md) for its commands and [the storage contract](../architecture/storage-m6.md) for exact mechanisms and limitations.
