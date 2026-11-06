# Engine OS user handbook

Start with **M6 v0.6.0**. The native graphical installer offers Back, Next, review, Install and Finish. You choose the disk, optional guide, local name and accent; the installer creates its partitions. No terminal commands are required for ordinary setup.

- [Download and install in VMware, step by step](install-m6.md)
- [Repair, update or roll back an M6 installation](recovery-m6.md)
- [Build and customize your own distribution](distributions.md)
- [Current release, checksum and qualified limits](../releases/m6-v0.6.0.md)
- [Product principles](../architecture/product-principles.md) and [current status](../roadmap/status.md)

Use a new disposable SATA disk of at least 2 GiB, two CPUs and 128 MiB RAM. Disable Secure Boot. After successful installation, power off, disconnect the ISO and restart. The installed landing screen retains your local choices and files. The full desktop is M7; MS-DOS/Windows applications are M8.

[Online documentation and downloads](https://engineos.brixcanvas.com) update from canonical GitHub Markdown and official release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5.1 numbered setup](install-m51.md), [M5 command-based installation](install.md), [old recovery](recovery.md), [old command reference](m5-console.md), [M4.5 storage](vmware-storage.md) and [console](console.md) describe their respective historical releases. M5/M5.1 data formats are not migrated by M6. Old releases and accepted evidence remain immutable.

## Current product delivery order

M6 real installation is complete within its named VM scope. M7 delivers the real desktop and networking; M8 implements separately qualified original MS-DOS and Windows 1.x through Windows 11 application tracks. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply throughout. See [the contracts](../roadmap/milestones.md).
