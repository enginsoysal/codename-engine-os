# M4.5 console commands

Commands are case-sensitive. Type `help` for the available syntax. `disk` and `storage` are not commands; use `disks`. There is no partition-create, format or install command in this release.

## Ordinary disk inspection

| Command | Purpose |
|---|---|
| `help` | List current command syntax |
| `disks` | Show discovered disk number, capacity, sector size and read status |
| `hardware` | Show PCI discovery, supported bindings and USB driver/topology/rejection status |
| `info` | Show transport/input counters and managed retained/peak memory |
| `parts N` | Inspect an existing MBR/GPT table on disk N; never create a partition |
| `fatls N P` | List the root of supported FAT32 partition P on disk N |
| `fatcat N P /PATH` | Read a FAT32 file, at most 4 KiB through the console |
| `extls N P /PATH` | List a directory in the supported read-only ext2 profile |
| `extcat N P /PATH` | Read an ext2 file, at most 4 KiB through the console |
| `halt` | Halt the guest CPU; it does not power off the VMware VM |

Disk and partition numbers are zero-based and must refer to entries actually reported by the console. A blank detected disk is inspectable without any partitions; it has no partition P to read. NTFS/exFAT are recognized but not mounted. These ordinary-disk commands do not grant write access.

## Marked M4 development volume only

`ls [PATH]`, `cat PATH`, `mkdir PATH`, `write PATH TEXT`, `mv FROM TO` and `rm PATH` use the separately mounted native development filesystem. Without that marked volume, they cannot create files on an ordinary SATA/NVMe disk. `store TEXT` and `show` are legacy development-record diagnostics; `sync` flushes the development block path. They are not partitioning or installation utilities.

[Handbook](index.md), [missing/blank VMware disks](vmware-storage.md), [exact filesystem limits](../milestones/m4-contract.md).
