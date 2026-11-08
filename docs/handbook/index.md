# Engine OS user handbook

Start with **Engine OS 0.7.5**. Extract the prepared VMware ZIP, open EngineOS.vmx and start. Select a disk row in the bordered list, follow Next, review, Install Engine OS and **Restart to Engine OS**. This starts the actual installed system. The matching ISO may stay connected. No commands or network are needed.

- [Download and install in VMware, step by step](install-m75.md)
- [Use the standard apps and manage optional packages](apps-m74.md)
- [Explore the desktop and navigate your folders](desktop-m73.md)
- [Current release, checksums and qualified limits](../releases/m75-v0.7.5.md)
- [Repair boot files and review recovery limits](apps-m74.md#disks-and-recovery)
- [Manage partitions and format safely](disk-manager-m71.md)
- [Build and customize your own distribution](distributions.md)
- [Product principles](../architecture/product-principles.md) and [current status](../roadmap/status.md)

The prepared VM has a new empty growable 20-GiB SATA disk, two CPUs, 128 MiB RAM, NAT networking, HDA audio and Secure Boot off. Use this fresh starter for the app release; cross-version package-cache migration is not qualified. UEFI startup adds a dedicated boot entry and requires writable firmware variables. Existing-disk replacement requires separate final erasure consent. For a separate disk-only test disconnect only CD/DVD and keep the SATA hard disk connected. The new tests retain the same VMware configuration/NVRAM and use ordinary disk discovery, without forced boot priority.

[Online documentation and downloads](https://engineos.brixcanvas.com) update from canonical GitHub Markdown and official release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5.1 numbered setup](install-m51.md), [M5 command-based installation](install.md), [old recovery](recovery.md), [old command reference](m5-console.md), [M4.5 storage](vmware-storage.md) and [console](console.md) describe their respective historical releases. M5/M5.1 data formats are not migrated by M6. Old releases and accepted evidence remain immutable.

## Current product delivery order

M6 real installation is complete within its named VM scope. M7 real desktop/networking is complete within M7-VM-v1; M8 implements separately qualified original MS-DOS and Windows 1.x through Windows 11 application tracks. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply throughout. See [the contracts](../roadmap/milestones.md).

[Native display sizing and design decisions](../architecture/graphics-m73.md). The 4K evidence uses an actual framebuffer; firmware mode availability still applies. Choose Appearance for Auto/100%/150%/200% and save preferences.
