# M4.5 evidence — in progress

These are increment reports, not full M4.5 completion. [Agreed acceptance](../../docs/milestones/m4.5.md), [implemented increment/limits](../../docs/milestones/m4.5-storage.md).

[Increment qualification](increment-results.json), [source/artifact manifest](build-manifest.json). The full milestone flag deliberately remains false.

- [Storage/partition report](storage-results.json): real AHCI/NVMe reads, ordinary capacities, 512/4096-byte sectors, primary/backup GPT and MBR validation, NTFS/exFAT recognition, corruption rejection, NVMe queue wrap and mixed-disk selection/write isolation. Ordinary backing files are independently hashed before/after.
- [FAT32 report](fat32-results.json): real native AHCI/NVMe reads of short-name and UTF-16 long-name files, malformed LFN checksum rejection and unchanged backing files.
- [SHA-256 report](sha256-results.json): exact original C implementation compared with independent known/boundary digests.
- [Secure Boot report](secure-boot-results.json): owner-enrolled disposable TCG firmware policy; genuine SecureBoot/SetupMode readback, valid signed loader, unsigned/tampered loader rejection, tampered external kernel/init rejection. The private test key/enrollment media are not published.

Every runtime report has its own tested build identity or source/provenance hashes. Native drivers are tested after firmware handoff; host-only parsers or a boot screen are not substituted for transfers. Physical hardware, factory trust, Legacy BIOS, USB mass storage/virtio and broader input/filesystem profiles are still open unless separately evidenced here.

Development observations: SATA signatures initially remained unknown until our driver enabled receive FIS and initialized the link. The driver now owns that step. The pinned QEMU ATA model refuses 4096-byte logical sectors; NVMe supplies that test profile. OVMF repaired a primary-only GPT corruption before kernel entry, so the negative fixture now damages both copies. Newer firmware required proof-of-possession for initial PK enrollment; the test helper now uses a signed authenticated-variable update. These are resolved development observations, not hidden passing tests.
