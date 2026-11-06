# M5 console command reference

**M5 reference / M5.1 advanced workflow.** For normal use, follow the [M5.1 guided-menu tutorial](install-m51.md); terminal commands are optional.

The console is a trusted diagnostic interface, not a Windows command prompt or Unix shell. Commands below use ASCII text and exact syntax; native filenames preserve UTF-8 through the underlying filesystem, but this console does not provide full Unicode input. Type `help` for syntax. Installation and repair require the live ISO/USB; the installed kernel intentionally omits the installer bundle.

| Command | Behavior |
|---|---|
| `disks` / `hardware` / `info` | Device inventory, bindings and runtime counters |
| `volume` | Volatile versus persistent storage and live overlay pages |
| `plan N` | Read-only first-install preflight and exact target/change plan |
| `install N ERASE-SECTORS` | Execute that outstanding plan; substitute the exact printed sector count |
| `cancel` | Discard the outstanding plan; no disk writes |
| `repairplan N` | Read-only same-build system-repair preflight |
| `repair N REPAIR-SECTORS` | Restore boot content, preserving initialized native data |
| `parts N` | Inspect partitions; never changes them |
| `ls` or `ls /PATH` | List the native volume |
| `cat /PATH` | Read at most four KiB through the console |
| `mkdir /PATH` | Create a native directory |
| `write /PATH TEXT` | Create/replace a native text file |
| `mv /FROM /TO` / `rm /PATH` | Rename/remove native files under the existing bounded VFS rules |
| `sync` | Flush the native volume; live RAM remains volatile |
| `fatls N P`, `fatcat N P /PATH` | Read-only inspection of a supported FAT32 partition |
| `extls N P /PATH`, `extcat N P /PATH` | Read-only inspection of a supported ext2 partition |
| `halt` | Halt guest CPUs; use VMware to power off |

Disk/partition numbers are zero-based. Installation is a whole-disk blank-target workflow, not an arbitrary partition-create or format command. Read discovery remains read-only; only the confirmed installer target and validated persistent native volume gain write access. `show` and `store TEXT` retain their legacy development-record diagnostics; they are not installer commands.

See [installation tutorial](install.md), [recovery](recovery.md), [M5 contract](../milestones/m5.md). The [older M4.5 command reference](console.md) remains release-specific and does not describe the M5 installer.
