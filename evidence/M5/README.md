# M5 native installation and recovery qualification

The [acceptance report](acceptance-results.json) governs completion. It binds the source build, exact public ISO and nine required gates. [Build manifest](build-manifest.json), [M5 contract](../../docs/milestones/m5.md), [installation tutorial](../../docs/handbook/install.md), [recovery](../../docs/handbook/recovery.md).

| Profile | Runtime checks |
|---|---|
| QEMU UEFI ISO, two CPUs / 128 MiB | Live file changes; plan/cancel and wrong confirmation; native install; independent GPT/FAT/ELF/BIOS/ext2 inspection; halted CPUs and memory/CPU sample; detached boot twice; whole-data-preserving repair |
| QEMU BIOS ISO, two CPUs / 128 MiB | Same complete workflow through the original BIOS loader |
| QEMU UEFI USB, one CPU / 64 MiB | Same workflow from USB live media; installed boot has no USB/ISO |
| QEMU BIOS USB, four CPUs / 128 MiB | Same workflow with all four CPUs observed halted at console idle |
| Native overlay test / 64 MiB | 128 changed pages, denial of page 129, allocation-free reuse and exact page-allocator baseline after cleanup |
| Negative inputs / copy EIO | Nonblank/undersized refusal hashes, actual SATA write failure without false success, incomplete-install resume, multiple-target refusal, confirmation cannot switch disks |
| VMware Workstation UEFI and BIOS | Exact public ISO, COM1-driven installer, real SATA VMDK, two fresh-firmware boots without CD, persistent file, same-build repair and unchanged whole data partition; pre-existing running VM inventory preserved |
| Repository gate | English documents, valid local links/commands, parseable Python, clean root layout and unchanged Windows generation inventory |

The test disks are new 256-MiB regular image files under ignored build output. A 20-GB SATA disk is a documented manual setup, not a claim that every disk capacity/controller has been qualified. No physical device is opened. Firmware/media names and transport are explicit in each report.

Resource reports distinguish configured guest RAM, managed current/peak pages, retained boot/image memory and a two-second host-QEMU CPU interval. These are named VM observations, not a universal physical minimum or benchmark. The live overlay's separate 512-KiB cap is exercised by real kernel operations. There is no host swap dependency.

Run `python tests/m5/verify.py` from the repository root to rebuild and rerun. `--collect` validates existing successful reports against current source/artifact identity; it does not invent runtime evidence. Public report bytes are normalized to LF before their hashes are frozen. Development failures remain in [development](development/README.md) and are excluded from successful qualification.

Earlier M4.5 acceptance reports and release assets remain unchanged. M5 does not qualify physical hardware, factory Secure Boot, arbitrary storage, dual boot, atomic filesystem/boot updates, general upgrades/rollback, desktop/network or Windows applications. The activation record defines a versioned format; current recovery restores the same installed build.
