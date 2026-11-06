# Install Engine OS with the guided menu

For **M5.1 v0.5.1**. No terminal commands are needed for installation. The interface uses your keyboard: type a displayed number and press **Enter**. It is a text wizard; a desktop is not included yet.

## Prepare VMware

1. Download `EngineOS-M51.iso` and `M51-SHA256SUMS.txt` from the [official release](https://github.com/enginsoysal/codename-engine-os/releases/tag/m51-v0.5.1). Compare its SHA256 using PowerShell `Get-FileHash .\EngineOS-M51.iso -Algorithm SHA256`.
2. Create a disposable x86-64 VM with two CPUs and 128 MiB RAM. BIOS and UEFI are supported in the qualified profiles; turn **Secure Boot off** for this unsigned image.
3. Add a **new SATA hard disk**, at least 256 MiB. A new **20-GB** VMware disk is tested. Do not use a physical host disk or a VMDK containing files you need. SCSI/LSI disks are not install targets.
4. Connect the ISO to CD/DVD at power-on and start from it. If VMware asks for a guest preset, FreeBSD 64-bit is a hardware preset only; Engine OS supplies its own kernel.

## What to choose

The welcome page offers:

- **1 — Try / open my files:** create and read a sample note or list Documents. Live changes disappear on reboot; installed changes persist.
- **2 — Install:** choose the actual disk number displayed beside its capacity.
- **3 — Repair:** restore a matching same-build installation's boot files.
- **4 — Advanced tools / terminal:** optional diagnostic commands. Type `menu` to return.
- **5 — Technical details:** optional device/build information.
- **6 — About this distribution:** identity and profile information.

To install, choose **2**, select your SATA disk, then read **Review installation**. It names the selected disk and its actual capacity. The current layout is a 64-MiB EFI partition plus a **16-MiB native data partition**, with **48-KiB maximum files**; most of a 20-GB disk stays unallocated. This is an early installable system, not application-ready storage.

Choose **0** to go back without changing the disk. Choose **1 — Install on this disk now** only after verifying the target. Wait for progress and **Installation complete**. Do not disconnect the disk while copying.

After completion:

1. Power off the VM through VMware.
2. Disconnect the ISO/CD/DVD at power-on; remove USB live media too if used.
3. Start the VM again. The disk now starts the installed menu.
4. Choose **1**, then **1** to create a sample note. Restart without the ISO and choose **1**, then **2** to read it again.

The guest does not control VMware's power/media settings automatically. `plan N`, `ERASE-SECTORS` and terminal syntax belong to the older advanced workflow and are not required here.

## Repair or a message you do not understand

Boot this **same distribution/build ISO**, choose **3**, select the installed SATA disk, review and choose **1** to repair. Initialized user data is preserved. After completion disconnect the ISO and boot the disk again. Repair does not fix corrupt personal filesystem metadata or upgrade an older build.

An unavailable disk needs the displayed supported controller/sector/capacity profile. An installation refusal can indicate nonblank boot metadata; create a new disposable SATA disk rather than deleting an existing VMDK. A write/verification failure means the install has not completed: restart this ISO after checking the disk and choose Repair. **D — Technical details** is optional. Record the message if reporting a problem.

See [exact limits](../milestones/m5.1.md), [advanced commands](m5-console.md) and [custom distributions](distributions.md). Older [M5 command-based instructions](install.md) remain historical.
