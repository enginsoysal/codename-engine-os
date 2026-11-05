# M4 first increment evidence

M4 is **in progress**. These tests qualify the AHCI sector-record and PS/2 console increment, not a filesystem or full M4 acceptance. The build identity in each JSON report links its logs to the generated source manifest. Full M4 requirements remain in the [milestone](../../docs/milestones/m4.md).

[Increment summary](increment-results.json), [source/artifact manifest](build-manifest.json), [M3 regression sampling note](regression-note.md).

- [Storage report](storage-report.json): real QEMU SATA DMA, two boots of the same disposable image, independent host generation/CRC/payload checks, unchanged bytes outside sector 1, injected backend write error, absent disk, corrupt header/record and wrong capacity. All rejected-write cases compare the complete backing file before/after. No host block device is exposed.
- [Console report](console-report.json): actual QMP key events through the emulated PS/2 controller, Shift/Backspace, store/flush/show, independent host payload check and four CPUs observed in HLT at input idle.
- [Console serial log](console.log), [idle registers](console-idle-registers.log), [framebuffer](console-screen.png).
- [VMware report](vmware-report.json): two real AHCI smoke boots on Workstation 26.0.1, two vCPUs/128 MiB, EFI FreeBSD-64 virtual profile. Generations 1 and 2 are independently verified after stopping the owned test VM. Existing VM inventory is unchanged. VMware keyboard/framebuffer and physical controllers are not qualified by this test.

Four fixed managed DMA pages add 16 KiB beyond the retained M3 baseline. `M4_MEMORY` records total managed current/peak bytes after driver initialization; `M2_BOOT_RESERVED_BYTES` separately records boot reservations. Console/ring state is fixed kernel BSS and therefore included in the boot-reserved kernel image, not counted again as managed DMA memory. No universal RAM minimum, physical-device or power-loss durability claim follows from these VM tests.

The writable record has no journal; CRC detects some corruption but does not recover it. Timeout quarantine exists in code but requires additional injected-timeout qualification. The injected write-error test exercises an ATA error response, not power loss. USB/mouse, ext2/VFS, file operations, disk-full and crash recovery are pending.
