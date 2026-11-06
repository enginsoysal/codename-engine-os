# Engine OS user handbook

Start here for **M5 v0.5.0**, the current live/install/recovery release. Engine OS boots its original kernel and text console. You can install to a new supported SATA virtual disk, detach the ISO and retain native files across boots. There is no desktop, network stack or Windows app execution yet.

- [Step-by-step VMware installation, download verification and first persistent file](install.md)
- [Troubleshooting, interrupted installation and boot recovery](recovery.md)
- [Current M5 command reference](m5-console.md)
- [M5 release notes and limits](../releases/m5-v0.5.0.md)
- [Current capabilities and next milestones](../roadmap/status.md)
- [M5 implementation and exact qualification profile](../milestones/m5.md)

Use a disposable VM and a new SATA disk. The current native data partition is 16 MiB and individual files are limited to 48 KiB; remaining disk capacity is unallocated. Live edits are temporary and are not migrated during installation.

## Online documentation and downloads

[Open the English documentation website](https://engineos.brixcanvas.com) for searchable guides and official GitHub downloads. Articles identify the exact canonical source commit. Relevant documentation pushes and release changes synchronize automatically through the site-scoped content publisher. OS source remains independent of the BrixCanvas application's source. [Website maintenance](../../website/README.md).

## Historical M4.5 documentation

[M4.5 VMware storage troubleshooting](vmware-storage.md), [M4.5 console](console.md) and [M4.5 release guide](../releases/m45-v0.4.5.md) describe that older read-only discovery release. Its statement that there is no installer applies to M4.5, not M5. Historical releases remain immutable.
