# VMware storage: why a blank disk does not appear

## Disk discovery comes before partitions

A disk with no partition table must still appear under `disks` when its controller is supported and a first-sector read succeeds. For a 20-GiB disk, M4.5 prints approximately `20480 MiB` and its logical sector size. `parts N` then reports no MBR/GPT table. An empty partition table is different from an undetected disk.

`No readable storage devices discovered` with no disk entries means no storage record was registered. It does not mean that the OS examined a detected disk and found no partitions. `No writable development filesystem mounted` separately means that no marked M4 development volume is mounted; ordinary disks are deliberately read-only.

## VMware controller selection

| Virtual disk controller | M4.5 status |
|---|---|
| SATA / AHCI | Qualified on VMware EFI and BIOS |
| IDE / ATA | Qualified on VMware EFI and BIOS |
| NVMe | Qualified on VMware EFI |
| SCSI LSI Logic, LSI Logic SAS, BusLogic or VMware Paravirtual SCSI | No driver in M4.5; a connected disk can remain undetected |

The FreeBSD guest preset can choose SCSI. Its virtual hardware selection is separate from Engine OS support. A controller that works for another guest needs an Engine OS driver too. A USB device count of zero is unrelated to whether a SATA disk is readable; VMware USB 2 controllers are also outside the qualified xHCI input profile.

## Safe setup for a new 20-GB test disk

1. Power off this test VM normally from VMware. Changing a controller requires a powered-off VM; Engine OS `halt` only halts the guest CPU and is not a VMware power-off operation.
2. Open **VM settings > Add > Hard Disk** and select **SATA**. Select a new virtual disk and a maximum size of **20 GB**. Do not select a physical/raw host disk.
3. Keep existing SCSI disk files intact. For a simple test, add the new SATA disk alongside them; there is no need to delete or overwrite the original VMDK. This procedure does not convert an existing SCSI disk.
4. Keep the Engine OS ISO connected to CD/DVD at power-on and boot it with Secure Boot disabled.
5. Run `disks`. Identify the disk by its displayed controller and capacity; disk numbering depends on discovery order.
6. Run `parts N`, replacing `N` with that disk's number. For a new blank disk, `No MBR/GPT partition table - unknown or empty filesystem` is expected. Status zero means inspection succeeded; it does not mean a partition was created.

If the SATA disk still does not appear, run `hardware` and `info`, and record the complete output and virtual controller configuration. Do not format the disk as a workaround for failed controller detection.

## Creating a partition

M4.5 has **no partition-create or format command**. `parts N` only inspects. Partitioning, formatting, target confirmation, installation and independently booting an installed disk belong to M5 and have not been implemented. There is no Engine OS command to turn this 20-GB disk into an install target yet.

M4.5 can inspect supported existing FAT32/ext2 filesystems on ordinary disks read-only. Its legacy writable filesystem is a separately marked, bounded M4 development fixture; it is not a general formatter for arbitrary disks. Creating a partition elsewhere therefore does not turn an ordinary disk into the writable development volume.

[Handbook](index.md), [console commands](console.md), [VMware release guide](../releases/m45-v0.4.5.md).
