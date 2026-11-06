# M6 native storage and recovery

Implementation status: native code exists; exact release qualification is tracked by [M6](../milestones/m6.md). This document describes implemented mechanisms and explicit limits, not universal device or physical power-loss certification.

## Disk layout and ownership

A common freestanding planner accounts for every 512-byte sector on eligible 2-GiB through 2-TiB SATA targets. The layout contains primary/backup GPT, alignment, a 4-MiB BIOS region, 128-MiB EFI partition, two 512-MiB system slots and a data partition covering the remaining aligned capacity. Actual runtime qualification names tested capacities/controllers; arithmetic bounds do not prove every disk size or physical device.

A new target needs blank boot metadata. The wizard reviews the whole-disk consequences, captures device/controller/context/capacity and initial boot metadata, then revalidates them at commit. An identified incomplete Engine OS install may be restarted after review. A completed installation is not silently reformatted by choosing Install. Records are checksummed, not cryptographically authenticated; unsigned media requires Secure Boot disabled.

The system/data partitions use project-specific type GUIDs derived from their Engine OS roles; they are not labelled as Linux/ext2 partitions. BIOS and EFI retain the standard firmware type GUIDs.

## Streaming copy-on-write volume

The M4/M5 ext2 demonstration remains historical. M6 uses original native extent-volume code in `kernel/storage/volume6.c`. The old one-group ext2 writer could address only 16 MiB and direct-block files. Extending its assumptions throughout the allocator/checker was an alternative; the selected backend instead makes bounded metadata and recoverable file replacement explicit before the desktop depends on it. External ext2/FAT inspection code remains separate from the native data format.

The native format has 1,024 280-byte entries, two checksummed/generation-numbered metadata banks, 4-KiB I/O and 640 KiB reserved before file data. Paths use at most 255 bytes, with 63-byte components. File sizes and offsets are 64-bit. Data capacity follows the selected partition rather than a 16-MiB template. A file occupies a contiguous extent; fragmentation or insufficient copy-on-write workspace can refuse a write even when aggregate unused space exists. Directories count toward the 1,024-entry bound. Hard links, sparse-file semantics, ACLs, multi-user authentication and arbitrary format migration are not implemented.

A replacement streams new file data into an unused extent, flushes it, writes the inactive metadata table, flushes it, writes its checksummed generation header last, then flushes. Current and previous valid snapshots protect their referenced extents from allocation. Mount validates metadata structure, checksums, parents, duplicate names, extent bounds and overlaps; it selects a valid generation. File verification streams the payload checksum. Failed metadata writes force the current mounted writer read-only until remount. Known fault-model checks require complete old or complete new data, never a claimed successful partial file.

The in-memory volume state is 864,320 bytes, independent of disk or file capacity. This static kernel memory is reported separately from managed allocator pages. Large files need a page-sized transfer buffer, not a whole-file RAM copy. The initial fixed entry count is a concrete limit, not a claim that every future Windows package fits. It must be extended before qualifying targets that exceed it.

## System slots and activation

The two system partitions store verified packed boot bundles. They are recovery/update sources; firmware boots the placed BIOS/EFI assets rather than directly choosing a slot. Updating stages this ISO's bundle into the inactive slot, verifies it, replays its boot files, reconstructs GPT and publishes a new generation through alternating checksummed activation records at LBAs 41/42. Initial installation also leaves a completed ownership record. Version identity must match the actually booted kernel before the data volume opens.

Rollback replays the previous verified slot and commits its identity. Repair replays the active verified slot. Both operate from the installer ISO and preserve the complete data partition. An interrupted boot-file replay may prevent ordinary boot or produce an explicit version-mismatch recovery screen; booting the ISO and repairing is then required. This is implemented ISO-assisted recovery, not automatic bootloader A/B failover. Cross-layout/schema migrations are not supported.

## Qualification boundaries

Host tests execute the actual native C through regular disposable files, including streamed reads, oversized/path refusals, corrupt metadata/payloads and injected failed/torn writes at data/table/header/flush boundaries. Guest tests exercise native wizard installation, media-detached BIOS/UEFI boot, a file over 2 MiB, a different kernel build, rollback, corrupted BIOS payload repair and activation-write failures. See the exact evidence linked by M6. These checks do not establish arbitrary physical power-loss durability, cryptographic boot trust, every controller or unlimited file counts.
