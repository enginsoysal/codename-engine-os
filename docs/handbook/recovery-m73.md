# Engine OS M7.3 recovery and updates

Keep the matching ISO available. Recovery is graphical and separate from a
fresh whole-disk installation. It supports the existing compatible M6/M7 native
slot layout; it does not migrate an unrelated OS or arbitrary disk layout.

Boot the ISO and choose Recovery on Welcome. Choose Repair, Update or Roll back,
select the existing Engine OS disk, then Next. Read Review and choose Confirm
only when the intended disk and operation are correct. Personal files are not
formatted by these maintenance choices. On completion, power off, disconnect
only the CD/DVD installation ISO and start from the same SATA disk.

- **Repair** replays the current verified boot slot. It is useful when the boot
  files need recovery; it cannot guarantee repair of every storage failure.
- **Update** stages this ISO's verified system bundle in the inactive slot.
  The existing account and personal file contents remain. Its older welcome
  guide may remain unchanged too; consult the current online handbook.
- **Roll back** restores the previous verified slot. It starts that slot's
  original version, including its original UI. It is not a fresh M7.3 install.

Do not choose Erase and install to preserve an existing workspace: that is
whole-disk replacement. Manage disks has its own reviewed partition operations;
quick formatting destroys the selected partition's files and is not secure
erasure. Back/Keep/Cancel is the safe route before applying a disk operation.

The UEFI loader requires writable firmware variables for its startup entry.
The entry can remain if setup is cancelled. Preserve the same VM configuration
and its NVRAM when checking a normal restart. If firmware reports No Media,
check that the SATA hard disk is connected; keep the ISO for diagnostics.
The precise cause of the owner's historical VM failure was not conclusively
established. Qualification covers named owned test VMs, not every machine.

Read [the installation guide](install-m73.md),
[partition instructions](disk-manager-m71.md), and
[the qualified graphics/boot limits](../architecture/graphics-m73.md).
