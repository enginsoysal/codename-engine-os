# M7.1 disk management design

The original freestanding `kernel/install/disk71.c` backend consumes bounded sector callbacks. Its GUI adapter belongs to the trusted live wizard. It invokes no host disk utility and exposes no new disk permission to third-party applications.

## Inspect and plan

Scan captures constant-size snapshots: primary/protective MBR, canonical GPT headers and 128 × 128-byte tables, partition rows, aligned gaps and a CRC fingerprint of first 64/last 33 sectors. Headers/tables require valid CRC, supported revision/size/locations, in-range bounds, nonzero/nonduplicate GUIDs and nonoverlapping entries. Damaged/disagreeing copies block ordinary edits. Noncanonical layouts are not guessed writable.

The GUI displays disk/controller/port/capacity and clears selection when switching devices. Prepare is read-only; plans record operation, gap/partition, exact range and format boot-sector CRC. Apply repeats semantic validation and metadata/boot checks. The adapter checks the registered device descriptor on each transfer. This is bounded change detection, not cryptographic device identity or support for hotplug/concurrent writers.

Required/read-only GPT entries, Engine OS system layouts and extended MBR containers are protected. Primary MBR edits preserve boot code/unrelated entries; logical/extended mutation is refused. Existing GPT UTF-16 names are retained verbatim; new names are bounded ASCII. MBR offers no name field. New GUIDs use local timestamp/identity seeds with within-table uniqueness checks, not authentication/cryptographic entropy.

## Apply and format

GPT writes/verifies backup table/header, flushes, then primary and flushes. Failed/torn writes can leave inconsistent copies. There is no fictitious atomic multi-sector commit or guessed winner. Explicit repair uses the chosen valid copy's bounds/GUID/entries. If both valid copies disagree, expert knowledge of the intended layout is required. MBR mutation writes/verifies sector zero only; deletion removes the selected entry.

FAT32 computes bounded geometry, clears reserved/allocation/root metadata, writes two FATs and matching FSInfo/backups, then publishes the primary boot sector after flushing. It does not securely erase payloads. Engine formatting reuses the installer's existing `v6_volume` workspace instead of allocating another 864-KiB metadata object. Slice callbacks clamp accesses to the selected partition.

Transfers are at most 4 KiB. Generic callbacks permit sector multiples; the AHCI adapter splits tails into supported 512/4096-byte commands. The first native tail-size refusal is retained as failed evidence, not success. Whole-disk replacement is a separate installer plan: Components/Style/Review remain read-only until explicit final consent, then boot/table metadata is cleared and verified installation begins. Neighboring partitions are deliberately not preserved in that route.

## Resources and qualification

Snapshots occupy 46,664 bytes and plans 88 bytes each. Scratch storage is bounded, with no disk-sized allocation or permanent polling. Formatting writes metadata proportional to allocation tables. [Resource evidence](../../evidence/M7.1/regression/resources/results.json) distinguishes managed allocations, static kernel, framebuffer and configured guest RAM.

[Acceptance](../../evidence/M7.1/acceptance-results.json) requires actual C host execution with independent GPT/FAT decoders, 902 refused/torn write/flush positions, corrupt/changed tables, protected entries and foreign-byte retention. Native tasks cover all ordinary operations, cancel/unchecked consent, GPT initialization/repair, equal-capacity disks, existing-disk installation and detached desktop. Real VMware uses only owned authenticated loopback guest framebuffer/PS2 input. Night distro, BIOS/UEFI USB and app/pressure/TLS regressions retain desktop behavior. This is no human study or physical power-loss certification.

Primary references: [UEFI GPT specification](https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html), [Microsoft FAT structures](https://github.com/Microsoft/Windows-driver-samples/blob/main/filesys/fastfat/fat.h). No third-party partition editor/filesystem library is bundled; original contributions remain MIT.
