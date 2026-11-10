# Install Engacer OS M10 in VMware

Candidate guide: use the M10 release only after its acceptance report is published.
The prepared ZIP contains a new blank disk, not a preinstalled demonstration.

1. Download **EngacerOS-M10-VMware.zip** from the official
   [release page](https://github.com/enginsoysal/codename-engine-os/releases/tag/m10-v0.10.0).
   Extract the whole ZIP to a local folder. Keep its files together.
2. In VMware Workstation, open **EngacerOS.vmx**. Choose **I Copied It** if asked.
   The FreeBSD guest-type setting selects VMware's hardware profile; Engacer OS
   runs its own kernel. The profile supplies two CPUs, 128 MiB, a growable 20-GiB
   SATA disk, xHCI USB, NAT Ethernet and HDA audio.
3. Start the VM. The welcome screen explains the guided installation.
4. Choose Next, then click **Disk 1** inside the bordered installation list.
   The highlighted row confirms your choice. A connected USB file-transfer drive
   is not an installation target for this phase.
5. Choose apps and appearance. Back lets you change choices before installation.
6. Review the selected disk and choose **Install Engacer OS**. Replacing existing
   disk data requires explicit erasure consent. Use disposable disks for testing.
7. Wait for completion, then choose **Restart to Engacer OS**. The matching ISO
   can remain connected. The real installed desktop should open automatically.
8. Open My files or Documents and browse or create folders. Settings → Devices
   shows built-in hardware status. [Try USB file copies](hardware-m10.md).

For an independent boot check, shut down the VM, disconnect **only CD/DVD** and
start again. Keep the same hard disk connected. Your folders and saved settings
should remain. Do not create another VM or detach its system disk for this check.

## Using your own VM

Use x86-64, BIOS or UEFI, two CPUs and the qualified controller/firmware matrix
in the release report. Public development media is unsigned; Secure Boot is off.
Installation requires a supported writable/flush-capable fixed disk with 512-byte
logical sectors, from 2 GiB through 2 TiB. IDE, SATA, SCSI and NVMe need their
specific supported controller interfaces; their generic class names are not a
promise about every physical controller.

The low-memory 64-MiB OVMF test connects USB storage after the installed desktop
starts. Booting that firmware with USB storage already attached can exhaust its
contiguous allocation for the original MSI boot bundle; that combination remains
unqualified. Use the 128-MiB prepared VMware profile for the ordinary setup.

Attach **EngacerOS-M10.iso** to CD/DVD and connect it at power on. For USB tasks
enable a supported xHCI controller. Ordinary 4K/scaling depends on a firmware
mode available before boot; it does not provide runtime monitor mode changes.

## If something fails

A disabled Next explains why the selected disk is unavailable. Disk tools can
manage supported fixed disks with separate destructive review. Do not select a
valuable host disk. If firmware reports No Media after installation, check that
the same installed hard disk is connected and retain the matching ISO for
recovery. UEFI needs writable firmware variables. A screenshot alone does not
prove which controller or boot route failed; retain the matching build and logs.

Start with a fresh M10 package. Cross-release installed application migration is
unqualified. The historical M9.2 Engine OS packages remain separate downloads.
General Windows installers, DOS, physical GPU acceleration and universal device
support are not supplied by the first hardware phases.
