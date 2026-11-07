# Install Engine OS 0.7.2 in VMware

This release focuses on two tasks: a guided graphical installation and a real desktop for navigating your folders. No network, terminal commands or optional applications are required.

## Easiest route: open the prepared VM

1. [Download the VMware starter ZIP](https://github.com/enginsoysal/codename-engine-os/releases/download/m72-v0.7.2/EngineOS-M72-VMware.zip).
2. Extract the **entire ZIP** into a new local folder. Open **EngineOS.vmx** in VMware Workstation. If asked, choose **I Copied It**.
3. Start the VM. Its included growable 20-GiB SATA disk is new and empty. The installer ISO is already connected. It is an actual installation, not a preinstalled image.
4. Choose **Next**, select **Disk 1**, then choose **Next**. Keep the default setup or change the welcome guide. Enter your name and choose your accent.
5. Review the disk and file capacity, then choose **Install**. Wait for **Welcome home** and **Open desktop**.
6. Choose **Open desktop**. Files opens your home folder immediately. Open Documents, create a folder, and explore. Changes are stored on the installed disk.

The package uses two CPUs, 128 MiB RAM, UEFI with Secure Boot off, and an ordinary connected SATA disk. Networking is off because these tasks do not need it. Files and the desktop operate without Calculator. The ZIP contains no remote-control settings or account credentials.

On UEFI startup the loader adds an Engine OS firmware fallback boot entry, keeping other operating systems' entries. It may remain if you cancel installation; no disk erasure is implied. Firmware variable storage must be writable. BIOS startup does not add this entry. Use a separate single-OS test VM; multi-OS/ESP boot priority and physical installation are unqualified.

## Use your existing VM instead

[Download the standalone ISO](https://github.com/enginsoysal/codename-engine-os/releases/download/m72-v0.7.2/EngineOS-M72.iso). Power off the test VM. Keep a **SATA** hard disk of 2 GiB to 2 TiB, with 512-byte logical sectors, connected at power on. Connect the ISO as CD/DVD. BIOS and UEFI are tested; Secure Boot must be off. Other controllers can be inspectable without being supported installation targets.

For a disk containing data, select it and choose **Erase and install: ON**, then Next. Review requires a separate unchecked **Erase ALL data on the selected disk** confirmation. Installation replaces the whole selected disk. Back retains your choices; no installation write happens before confirmed Install. Use **Manage disks** for individual partition operations. [Disk-manager reference](disk-manager-m71.md).

Do not erase a disk to fix a firmware boot error. The installer should detect and display the target first. Existing data requires your deliberate replacement choice.

## Next boot: disconnect only the ISO

1. Power off the VM through VMware.
2. In VM settings, disconnect **CD/DVD**, or clear its Connect at power on setting.
3. Leave **Hard Disk (SATA)** connected. Keep its VMDK file in place.
4. Start the same VM. The installed disk opens the desktop.

The first Open desktop runs the native desktop from the installation kernel against the persistent installed volume. It avoids a mandatory restart to see your desktop. Separate acceptance tests power off, disconnect the optical media and start the same VMware VM twice, retaining its NVRAM and without forcing disk boot priority. Immediate desktop entry alone is not evidence of independent disk boot.

If the ISO starts again, the welcome page offers **Open installed desktop**. It opens exactly one valid same-build Engine OS installation without formatting. It refuses a different version, damaged layout/settings, or multiple matching installations. For an older compatible install, use Recovery instead.

## If VMware says No Media or starts EFI Network

This screen is VMware firmware; Engine OS has not started. Check **Hard Disk (SATA)** is still present and connected at power on, and its VMDK exists. Disconnecting the ISO must not disconnect the installed hard disk. If the disk uses NVMe or SCSI, this release's installer does not support that target. The starter ZIP supplies a known SATA configuration.

The reported user screenshot did not establish the exact cause. The previous M7.1 installed disk also booted in a fresh unforced VMware UEFI test; that does not diagnose the user's particular VM. Do not label this fixed by a guessed firmware setting. The new tests cover ordinary persistent-VM boot discovery as well as an immediately available desktop.

## Preserve an existing M7.1 installation

Boot this ISO and choose **Recovery → Update**. Select the compatible Engine OS disk, Next, review and Confirm. This stages verified boot files while keeping the user volume. Power off, disconnect only the ISO, then restart. Do not select Erase and install for an update. [Recovery boundaries](recovery-m71.md). M7.2 tests an actual M7.1 installation through this native update path and independently verifies retained file contents.

[Desktop and folder controls](desktop-m72.md) · [Release and checksums](../releases/m72-v0.7.2.md) · [Exact acceptance](../../evidence/M7.2/acceptance-results.json).
