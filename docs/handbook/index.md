# Engine OS user handbook

Start with **M5.1 v0.5.1**. A guided keyboard menu offers Try, Install and Repair without terminal commands. Type a displayed number and press Enter. The installed menu provides a persistent sample-file preview. This is not a desktop yet.

- [Step-by-step guided VMware installation and recovery](install-m51.md)
- [Build and customize your own distribution](distributions.md)
- [Product principles: convenience, clear interaction and distribution freedom](../architecture/product-principles.md)
- [M5.1 implementation and qualification](../milestones/m5.1.md)
- [Current status](../roadmap/status.md)

Use a new disposable SATA disk and disable Secure Boot. The native data partition remains 16 MiB, with 48-KiB files; remaining disk capacity stays unallocated. Live sample files disappear on reboot; installed sample files persist.

[Online documentation and downloads](https://engineos.brixcanvas.com) update from canonical GitHub Markdown and release assets. [Website maintenance](../../website/README.md).

## Historical and advanced references

[M5 command-based installation](install.md), [advanced recovery](recovery.md), [terminal command reference](m5-console.md). In M5.1 choose Advanced deliberately and type `menu` to return; these commands are not installation prerequisites. [M4.5 storage](vmware-storage.md) and [console](console.md) describe the older release without an installer. Old releases/evidence remain immutable.
