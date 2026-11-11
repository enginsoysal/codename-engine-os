# Engacer OS user handbook

Current delivery: **M11.4 / 0.11.4**, windows and appearance within M11-STAGE4-VM-v1. [Release scope and exact evidence](../releases/m114-v0.11.4.md). Expanded setup and full regression follow as M11.5 and M11.6. Older releases remain historical.

Start with **Engacer OS 0.11.4**. Extract the complete prepared VMware ZIP, open EngacerOS.vmx and choose I Copied It if asked. Start, select the disk row in the bordered list, follow Next, review and install, then choose **Restart to Engacer OS**. The matching ISO may stay connected. No terminal command, network connection or guest driver installer is needed.

The new stage starter is tested with VMware SATA/UEFI and all shipped devices enabled. Native ordinary/150%/4K window behavior and USB/PS2 keyboard routes have separate evidence. The inherited device/USB implementation and older M10 controller matrix remain scoped historical records; they do not automatically qualify these new bytes. Full new controller/application regression is M11.6.

- [Download and install M11.4](install-m114.md)
- [Select, open and manage real Files items](files-m11.md)
- [Use Start search, categories, pins and taskbar](start-m11.md)
- [Arrange windows and save your appearance](windows-m11.md)
- [Install, run and remove the original Windows 7-Zip console app](windows-apps-m8.md)
- [Use the standard apps and manage optional packages](apps-m74.md)
- [Explore the desktop and navigate your folders](desktop-m73.md)
- [Current release, checksums and qualified limits](../releases/m114-v0.11.4.md)
- [Repair boot files and review recovery limits](apps-m74.md#disks-and-recovery)
- [Manage partitions and format safely](disk-manager-m71.md)
- [Build and customize your own distribution](distributions.md)
- [Product principles](../architecture/product-principles.md) and [current status](../roadmap/status.md)

The prepared VM has a new empty growable 20-GiB SATA disk, two CPUs, 128 MiB RAM, NAT networking, HDA audio and Secure Boot off. Use this fresh starter for this storage release; cross-version package-cache migration is not qualified. UEFI startup adds a dedicated boot entry and requires writable firmware variables. Existing-disk replacement requires separate final erasure consent. For a separate disk-only test disconnect only CD/DVD and keep the SATA hard disk connected. The new tests retain the same VMware configuration/NVRAM and use ordinary disk discovery, without forced boot priority.

[Online documentation and downloads](https://engaceros.brixcanvas.com) update from canonical GitHub Markdown and official release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5.1 numbered setup](install-m51.md), [M5 command-based installation](install.md), [old recovery](recovery.md), [old command reference](m5-console.md), [M4.5 storage](vmware-storage.md) and [console](console.md) describe their respective historical releases. M5/M5.1 data formats are not migrated by M6. Old releases and accepted evidence remain immutable.

M11 desktop and installation work now precedes further hardware and Windows expansion.
M11.0 design, M11.1 input, M11.2 Files, M11.3 Start/taskbar and M11.4 windows/appearance are complete
within their stage scopes. [M11 progress](../milestones/m11.md) ·
[Files user guide](files-m11.md) · [Start/taskbar guide](start-m11.md) · [setup/desktop contract](desktop-install-m11.md).
Every remaining stage has its own visible release, matching test media, English manuals and complete website update before the next starts. Full final regression remains M11.6.
The earlier [M9 graphics record](../milestones/m9.md) remains historical; it does
not claim 3D acceleration or runtime physical-mode switching.

## Current product delivery order

M6 real installation is complete within its named VM scope. M7 real desktop/networking is complete within M7-VM-v1; M8 qualifies one original Windows console/MSI profile; the wider MS-DOS and Windows 1.x through Windows 11 tracks remain in progress. Engacer OS is the current product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply throughout. See [the contracts](../roadmap/milestones.md).

[Native display sizing and design decisions](../architecture/graphics-m73.md). The 4K evidence uses an actual framebuffer; firmware mode availability still applies. Choose Appearance for Auto/100%/150%/200% and save preferences.
