# M4 storage/filesystem/input evidence

The [acceptance report](acceptance-results.json) records the current qualification result. Scope is the [bounded native ext2/virtual-device profile](../../docs/milestones/m4-contract.md), not universal hardware or existing application compatibility. Every runtime report carries its tested build ID; [the manifest](build-manifest.json) records source/artifact hashes.

[Development failures and fixes](development-notes.md), [historical alpha idle-sampling note](regression-note.md), [historical alpha increment](increment-results.json). Historical alpha results are not current M4 acceptance.

- [Storage report](storage-report.json): real AHCI DMA/MSI completion and HLT waits, same-disk reboot persistence, independent ext2 checking, native case/UTF-8/handles/rename/unlink probes, injected write error, rejected absent/corrupt/wrong-capacity media, 64 MiB/one-CPU and actual USB boot. Rejected-write cases compare complete backing files.
- [Filesystem fault/recovery report](filesystem-report.json): full volume, dirty mount refusal, real kernel write failure after a flushed dirty marker, delayed-write timeout/quarantine, offline recovery to exclusive copies and successful recovered boots. This is not arbitrary physical power-loss qualification.
- [PS/2 console report](console-report.json), [serial](console.log), [registers](console-idle-registers.log), [framebuffer](console-screen.png): actual key events, editing, file commands and independent ext2 payload/graph checks.
- [USB console report](usb-console-report.json), [serial](usb-console.log), [registers](usb-console-idle-registers.log), [framebuffer/pointer](usb-console-screen.png): own xHCI keyboard/mouse IRQ reports, buttons/motion, filesystem interaction, all four CPUs halted at idle, separate managed/boot accounting and host-process CPU sample.
- [VMware report](vmware-report.json): two EFI/FreeBSD-64 SATA boots on Workstation 26.0.1, two vCPUs/128 MiB, native ext2 probes and host-side validation after each stopped owned VM. Existing VM inventory is unchanged. VMware USB/graphical input and physical devices are not qualified.

Four fixed AHCI DMA pages cost 16 KiB. The named xHCI keyboard/mouse profile adds fifteen driver pages (60 KiB, including its MMIO mapping page table). Filesystem/console state is bounded BSS included in boot reservations. The 96 KiB verification buffers plus two mapping pages are temporary and reclaimed, with allocator-balance verification. Console reports separately record managed current/peak and boot reservations; guest RAM is not kernel usage. The two-second QEMU process sample is not a physical CPU benchmark.

The v0.4.0 four-CPU/128-MiB console qualification observed:

| Profile | Managed current | Managed peak | Boot reserved | Idle sample |
|---|---:|---:|---:|---|
| PS/2, no attached USB HID | 192 KiB | 456 KiB | 912 KiB | All four HLT; 0.0 QEMU CPU seconds over about two seconds |
| xHCI keyboard/mouse | 232 KiB | 456 KiB | 912 KiB | All four HLT; 0.0 QEMU CPU seconds over two seconds |

Boot plus managed peak is 1368 KiB, excluding firmware/device/framebuffer and host allocations. The 64-MiB/one-CPU case qualifies native boot/filesystem self-tests; interactive HID qualification is the 128-MiB/four-CPU profile. The USB-media case qualifies actual firmware USB boot and subsequent own AHCI/ext2 tests, not an implemented kernel USB-storage driver.

There is no ext2 journal. Dirty-volume checking can repair allocation/count bookkeeping from an intact reachable graph; ambiguous corruption is refused and requires backup recovery. Unsupported links/features, untrusted file syscalls, hubs/hotplug and guest semantics remain outside this bounded milestone profile. No installer or desktop is included.
