# M1 evidence

Historical M1 release qualification. [Acceptance](acceptance-results.json), [build manifest](build-manifest.json), [ELF parser](elf-results.json), [boot scenarios](boot-results.json), [visual/debugger report](visual-results.json).

The reports identify the original source paths/build ID at the [M1 tag](https://github.com/enginsoysal/codename-engine-os/tree/m1-v0.1.0). Current repository paths are reorganized; historical source hashes are not silently rewritten to pretend they came from today's code. Current development is [M2](../../docs/milestones/m2.md).

M1 passed 19 actual C ELF parser checks, seven kernel boot/error scenarios and three normal-screen/debugger checks, plus 51 M0 regressions. Firmware exit and kernel entry must carry the same build ID. A real #UD was symbolized; the screenshot comes from the actual framebuffer.

[ISO](iso-smoke.log), [USB](usb-smoke.log), [128 MiB](iso-lowram.log), [retry](iso-retry.log), [panic](iso-panic.log), [bad header](iso-badboot.log), [bad ELF](iso-badelf.log), [persistent normal boot](normal-iso.log), [symbolized fault](panic-symbol.txt).

![M1 own kernel after firmware exit](boot-screen.png)

Only PPM→PNG conversion was applied. This proves the tested virtual boot path, not installer/app support, physical hardware, ARM, Secure Boot, scheduler or kernel drivers. M1's broad supervisor RWX mapping is hardened by M2 before user execution. Double-fault IST was configured, not separately forced. [M1 choices](../../docs/milestones/m1.md).
