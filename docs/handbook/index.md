# Engine OS user handbook

Start with **Engine OS 0.9.2**. Extract the complete prepared VMware ZIP, open EngineOS.vmx and choose I Copied It if asked. Start, select the disk row in the bordered list, follow Next, review and install, then choose **Restart to Engine OS**. The matching ISO may stay connected. No terminal command, network connection or guest driver installer is needed.

The installer now uses actual write/flush capability. The VMware controller matrix includes IDE, SATA, supported LSI/PVSCSI/BusLogic SCSI and NVMe, with firmware limits stated per row. Broader physical hardware and two recorded QEMU SAS restart topologies remain unqualified.

- [Download, install and check controller support](install-m92.md)
- [Install, run and remove the original Windows 7-Zip console app](windows-apps-m8.md)
- [Use the standard apps and manage optional packages](apps-m74.md)
- [Explore the desktop and navigate your folders](desktop-m73.md)
- [Current release, checksums and qualified limits](../releases/m92-v0.9.2.md)
- [Repair boot files and review recovery limits](apps-m74.md#disks-and-recovery)
- [Manage partitions and format safely](disk-manager-m71.md)
- [Build and customize your own distribution](distributions.md)
- [Product principles](../architecture/product-principles.md) and [current status](../roadmap/status.md)

The prepared VM has a new empty growable 20-GiB SATA disk, two CPUs, 128 MiB RAM, NAT networking, HDA audio and Secure Boot off. Use this fresh starter for this storage release; cross-version package-cache migration is not qualified. UEFI startup adds a dedicated boot entry and requires writable firmware variables. Existing-disk replacement requires separate final erasure consent. For a separate disk-only test disconnect only CD/DVD and keep the SATA hard disk connected. The new tests retain the same VMware configuration/NVRAM and use ordinary disk discovery, without forced boot priority.

[Online documentation and downloads](https://engineos.brixcanvas.com) update from canonical GitHub Markdown and official release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5.1 numbered setup](install-m51.md), [M5 command-based installation](install.md), [old recovery](recovery.md), [old command reference](m5-console.md), [M4.5 storage](vmware-storage.md) and [console](console.md) describe their respective historical releases. M5/M5.1 data formats are not migrated by M6. Old releases and accepted evidence remain immutable.

M9 now has priority over further Windows expansion. Its first slice adds bounded buffered 2D graphics and automatic supported VMware pointer/video binding. It does not add 3D acceleration or change the physical display mode. [Graphics scope and remaining hardware stages](../milestones/m9.md).

## Current product delivery order

M6 real installation is complete within its named VM scope. M7 real desktop/networking is complete within M7-VM-v1; M8 qualifies one original Windows console/MSI profile; the wider MS-DOS and Windows 1.x through Windows 11 tracks remain in progress. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply throughout. See [the contracts](../roadmap/milestones.md).

[Native display sizing and design decisions](../architecture/graphics-m73.md). The 4K evidence uses an actual framebuffer; firmware mode availability still applies. Choose Appearance for Auto/100%/150%/200% and save preferences.
