# M5 troubleshooting and recovery

**M5 reference / M5.1 advanced workflow.** For normal use, follow the [M5.1 guided-menu tutorial](install-m51.md); terminal commands are optional.

## No disk or no installation plan

Run `disks` and `hardware`. M5 installs only to readable SATA/AHCI disks with 512-byte sectors, at least 256 MiB. A new blank disk needs no pre-created partition. SCSI/LSI is not supported; choose a new SATA virtual disk. A missing disk is a driver/controller issue, not an empty partition table.

Installer status `2` means refused: missing plan, wrong confirmation/capacity, unsupported target, nonblank reserved metadata, wrong build, or insufficient size. `3` indicates I/O/verification failure; `4` indicates timeout. The failed controller can remain quarantined for that boot. Power off and restart the test VM before retrying with a healthy backend.

## Installed disk does not boot

First confirm the ISO is disconnected, the intended disk is connected, firmware mode is supported and Secure Boot is disabled. Keep the disk intact. Boot the **same M5 release ISO** and run:

```text
disks
repairplan 0
```

Check its target and capacity. It prints the exact repair confirmation, for example for a 256-MiB disk:

```text
repair 0 REPAIR-524288
```

Use your own displayed disk number/sector count. Expect `M5_REPAIR_PASS` and status `0`. Power off, detach live media and boot the disk again.

Repair restores the same-build GPT/BIOS/EFI system boot content and preserves the **entire initialized native data partition**. It requires a valid matching installation intent/completion record; it refuses unrelated disks and other builds. It is not a cross-version rollback or a way to salvage arbitrary existing partitions. Before a risky manual repair, preserve a copy of your test VMDK through VMware's normal powered-off workflow.

## Copy failed during the first installation

There may be no bootable system yet. Restart from the same ISO with the disk connected through a healthy SATA backend, then use `repairplan` and the printed `repair` confirmation. If the initial data partition was never initialized, resume initializes it from the release template. Once initialization is recorded, repair preserves it. An invalid/missing intent is refused; there is no blind force-recovery option.

## Boot works but files are missing or read-only

Run `volume`. Live mode is volatile; its changes disappear on reboot. Installed mode should identify a persistent native volume. Keep only the intended SATA installation disk attached. More than one eligible SATA target causes an explicit refusal rather than guessing which disk supplied the running kernel. Invalid GPT or dirty/corrupt ext2 can prevent a writable mount.

Boot repair does **not** repair damaged personal filesystem metadata. Stop writing and preserve an offline image. The existing conservative [filesystem recovery tool](../../tools/README.md) operates on a standalone native volume image and writes a new repair copy; it is not a command to run on a whole GPT/VMDK disk. Ambiguous corruption may be unrecoverable. M5 does not promise journaling or arbitrary power-cut safety.

## Storage and memory limits

The filesystem has 16 MiB capacity, 512 inodes and a maximum 48 KiB per file. A 20-GB virtual disk does not enlarge this profile. `parts N` shows the actual partitions; remaining space is unallocated. Live changes have an additional 128-page/512-KiB copy-on-write budget. Exhaustion refuses further writes; restart restores the clean live template. Do not mistake temporary files for installed persistence.
