# Install Engacer OS M11.4 in VMware

M11.4 is the windows/appearance stage delivery, qualified within
M11-STAGE4-VM-v1. Use its exact new starter and matching ISO. Full M11 regression
is M11.6; do not substitute an older ISO or assume an old matrix qualifies it.

## Download and start

1. Open the [M11.4 release](https://github.com/enginsoysal/engacer-os/releases/tag/m114-v0.11.4).
   Download `EngacerOS-M11STAGE4-VMware.zip` and its checksum list. Compare the
   complete SHA256 value before opening it.
2. Extract the **whole ZIP** into a local folder with sufficient free space.
   Open **EngacerOS.vmx**. Choose **I Copied It** if VMware asks.
3. Start the VM. The package has an empty growable **20-GiB SATA** disk, **128 MiB**
   RAM, two CPUs, UEFI, networking, USB and sound enabled. Keep Secure Boot off.
   It performs a real installation; the disk is not already installed.

## Follow setup

Choose Next, click the disk's entire row in the bordered list, and continue.
Choose optional apps and appearance, review the named disk and installation
choices, then Install. This release uses the existing seven-step wizard; the
expanded installation journey is M11.5. No terminal command, online account,
network connection or separate guest-driver installer is required.

Existing data needs separate whole-disk erasure consent. Use **Disk tools** to
inspect supported partition/format operations and their target-specific review.
Installation uses the supported whole-disk layout. Selected-partition OS install,
partition resizing and dual boot are not implemented. Never use a host disk as
the test target.

Wait for completed installation, then choose **Restart to Engacer OS**. The
matching ISO may remain connected. To check a separate disk-only start, shut down
the VM, disconnect **only CD/DVD**, keep the same installed disk connected and
start again. A live-session desktop is not proof of installed startup.

## Explore the desktop

- [Click/select/open and real Files operations](files-m11.md).
- [Start search, categories, pins and taskbar](start-m11.md).
- [Maximize, snap, Alt+Tab and appearance](windows-m11.md).
- [Optional standard apps](apps-m74.md) and [distribution profiles](distributions.md).
- [Release scope, hashes and remaining limits](../releases/m114-v0.11.4.md).

If setup refuses a disk, read the row's reason and Details. It needs a supported
writable controller with verified read/write/flush, 512-byte logical sectors and
a target of 2 GiB to 2 TiB. A capacity alone does not prove writable support.
The release guide identifies the newly tested M11.4 profiles. M10's broader old
matrix does not by itself qualify new M11.4 bytes.
