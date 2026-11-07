# Engine OS user handbook

Start with **M7.1 v0.7.1**. The native graphical installer offers Back, Next, review, Install and Finish. You choose the disk, optional guide/Calculator, local name and accent; the installer creates its partitions. Existing-disk replacement requires final whole-disk consent; Manage disks handles individual data operations. No terminal commands are required for ordinary setup.

- [Download and install in VMware, step by step](install-m71.md)
- [Repair, update or roll back compatible system slots](recovery-m71.md)
- [Manage data partitions and format safely](disk-manager-m71.md)
- [Build and customize your own distribution](distributions.md)
- [Current release, checksum and qualified limits](../releases/m71-v0.7.1.md)
- [Product principles](../architecture/product-principles.md) and [current status](../roadmap/status.md)

Use a disposable SATA disk of at least 2 GiB; existing contents require the explicit replacement route. Use two CPUs and 128 MiB RAM. Disable Secure Boot. After successful installation, power off, disconnect the ISO and restart. The installed disk enters the real desktop and retains your choices/files. Use e1000e NAT for downloads. [Desktop controls and everyday tasks](desktop-m7.md). MS-DOS/Windows application execution remains M8.

[Online documentation and downloads](https://engineos.brixcanvas.com) update from canonical GitHub Markdown and official release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5.1 numbered setup](install-m51.md), [M5 command-based installation](install.md), [old recovery](recovery.md), [old command reference](m5-console.md), [M4.5 storage](vmware-storage.md) and [console](console.md) describe their respective historical releases. M5/M5.1 data formats are not migrated by M6. Old releases and accepted evidence remain immutable.

## Current product delivery order

M6 real installation is complete within its named VM scope. M7 real desktop/networking is complete within M7-VM-v1; M8 implements separately qualified original MS-DOS and Windows 1.x through Windows 11 application tracks. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply throughout. See [the contracts](../roadmap/milestones.md).
