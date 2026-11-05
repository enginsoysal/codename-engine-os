# M4 implementation, usage and limits

The current acceptance state is in [M4](m4.md). The following is the bounded native virtual-device profile, not universal hardware or application compatibility.

## Controller ownership and queues

PCI discovery binds AHCI 01/06/01 and xHCI 0c/03/30 by class/protocol, without vendor IDs. The kernel owns initialization after ExitBootServices. AHCI resets its controller even when USB firmware boot did not initialize SATA. NVMe, virtio and physical controllers are unqualified.

AHCI has one outstanding slot and four managed DMA pages (16 KiB), LBA48, 512-byte metadata requests and eight-sector/4 KiB block transfers. MSI completion wakes a halted CPU; a temporarily unmasked 100 Hz PIT provides timeout wakeups. The previous PIC mask and interrupt state are restored. A bounded polling fallback remains for controllers without MSI. Tests report actual IRQ/HLT counts. Timeout offlines the controller and quarantines DMA buffers: stopping does not prove DMA completion, and the write outcome is unknown.

IDENTIFY and a CRC-checked version-2 marker must match the exact 16 MiB/512-byte-sector disposable profile. Filesystem writes become available only after geometry, features, allocation maps, directory graph, names/types/links and block ownership validate. Version-1 alpha disks are refused; preserve them. A separate sector-1 diagnostic record remains outside ext2 metadata.

## Filesystem and namespace

The ext2 allowlist is one group, 4 KiB blocks, 512 128-byte inodes, FILETYPE entries and twelve direct pointers: at most 48 KiB per file/directory. Names preserve valid UTF-8 bytes and case, up to 63 bytes; absolute paths are at most 255 bytes. NFC/NFD names remain distinct. Requested dot/dot-dot, backslashes, control characters, repeated separators and malformed UTF-8 are rejected. Hard/symbolic links, indirect blocks, xattrs/ACLs and unknown feature flags are refused.

The VFS resolves the native root and seeded System, Applications, Users/test, Data, Cache, Logs, Temporary and Volumes directories. Create/read/replace, mkdir, file rename and unlink share inode identity. Eight generation-checked kernel handles mediate read/write sharing; path operations cannot bypass exclusive handles. Rename/delete while open return busy, even with delete sharing. Directory rename/removal are unsupported. These are trusted kernel diagnostic APIs, not yet an untrusted-user file syscall/security interface or guest ACL adapter. Guest views and immutable system activation remain later work.

File replacement allocates new blocks before publishing the inode, requiring space for the new contents. Preflight space checks precede mutation. Metadata is marked unclean and flushed before mutation; data, allocation/count metadata and the clean marker are flushed in order. There is no unbounded writeback cache. All maps, buffers, handles and queues are bounded. Large verification buffers use a temporary supervisor/NX mapping and are fully unmapped/reclaimed afterward.

## Crash/error contract

There is no journal or atomicity guarantee under arbitrary power loss. A failed filesystem write disables further filesystem/record writes and leaves the volume unclean. Dirty volumes are read-only until checked. The independent host checker reconstructs maps/counts from a valid reachable graph and writes recovery only to a **new exclusive copy**. It refuses duplicate references, torn directory records, duplicate block ownership and unsupported features. Ambiguous corruption needs a known-good backup; the checker does not invent missing data. Never mark an uncertain image clean by hand.

```powershell
python tools/build/fsck.py build/m4/manual/fs-data.img
python tools/build/fsck.py build/m4/manual/fs-data.img --repair-copy build/m4/manual/recovered.img
```

Tests cover a backend failure after the durable dirty marker, delayed-write timeout/DMA quarantine, full disk, dirty mount refusal and recovered-copy boot. These qualify specific interruption points, not every crash boundary or physical power-loss behavior.

## Input and resource policy

The own xHCI driver maps controller BARs, including MMIO above 4 GiB, into a supervisor/NX window. It owns command/event/transfer rings and contexts, uses MSI-X or MSI, enumerates directly attached boot HID keyboard/mouse and submits interrupt IN transfers. Two devices/four connected-port attempts bound memory. The named QEMU profile uses fifteen driver pages (60 KiB, including its MMIO page table), separately from AHCI. Uncertain DMA ownership is retained rather than recycled.

Keyboard reports produce US ASCII/Shift/Backspace. Mouse reports move a framebuffer pointer and track buttons. Normal input is interrupt-driven; all four CPUs are observed halted at idle. A fixed 256-byte queue counts drops. PS/2 set-1 and COM1 are fallbacks. Non-US layouts, Unicode console entry/rendering, key repeat/Caps Lock, hubs, hotplug/reconnect, arbitrary HID descriptors and physical devices are unqualified. UTF-8 storage is tested through the native API independently of the ASCII console.

## Build and use

```powershell
python tools/build/m4.py
python tests/m4/verify.py
python tools/vm/run.py --milestone M4 --create-data-disk
```

The launcher creates the regular working file `build/m4/manual/fs-data.img` only if absent and preserves it. `--data-disk` selects another marked version-2 image. No host disk is exposed. Without a marked disk, the console opens with writes unavailable.

Commands: `help`, `info`, `ls PATH`, `cat PATH`, `write PATH TEXT`, `mkdir PATH`, `mv FROM TO`, `rm PATH`, `sync`, `halt`. `store TEXT`/`show` exercise the separate diagnostic record. Commands are capped at 120 ASCII characters; `cat` displays at most 4 KiB, while native tests cover 48 KiB.

```text
write /Users/test/Documents/hello.txt Hello
cat /Users/test/Documents/hello.txt
mv /Users/test/Documents/hello.txt /Users/test/Documents/saved.txt
```

Reboot with the same disk and read `saved.txt`. Rename refuses existing destinations.

For VMware: EFI x86-64, Secure Boot disabled, HPET enabled, 128 MiB RAM and SATA. Load `dist/EngineOS-M4.iso`. Copy **both** `EngineOS-M4-fs-template.vmdk` and `EngineOS-M4-fs-template.img` together into a disposable VM directory, retaining names, then attach the copied VMDK as SATA. Keep download/build templates unchanged. Workstation 26.0.1 qualification covers two-vCPU storage persistence; USB/graphical input is qualified separately on QEMU. The FreeBSD-64 VMware profile selects virtual hardware, not an OS dependency.

## Evidence and later work

[Acceptance](../../evidence/M4/acceptance-results.json), [reports/screenshots](../../evidence/M4/README.md). Named profiles: pinned QEMU q35/OVMF, 64 MiB/one CPU and 128 MiB/two or four CPUs, ISO/USB boot and direct-root xHCI boot keyboard/mouse; VMware Workstation 26.0.1 EFI/SATA at 128 MiB/two CPUs. Configured guest RAM is not kernel consumption. Reports separate managed peak/current, boot reservations, driver pages, IRQ/HLT counts and a two-second host-process CPU sample.

M5 adds live/install/recovery workflows and independent installed boot. Production durability, broader devices, file-backed paging, untrusted file syscalls and guest compatibility remain in their later scopes. [Historical alpha.1](../releases/m4-storage-alpha.1.md) contained only transport diagnostics. References: [Intel AHCI](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf), [Intel xHCI](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf), [ext2](https://docs.kernel.org/filesystems/ext2.html).
