# Current status

Updated 11 November 2026. **Engacer OS M10 / 0.10.0 is the current delivery.**
Hardware phases **1 and 2 are complete within M10-DEVICES-VM-v1**, with
[27 exact-source/artifact gates](../../evidence/M10/acceptance-results.json).
[Release/downloads and controller matrix](../releases/m10-v0.10.0.md),
[VMware tutorial](../handbook/install-m10.md),
[Devices and USB tasks](../handbook/hardware-m10.md),
[architecture](../architecture/hardware-m10.md).

The new ISO and exact blank-disk VMware starter are branded Engacer OS.
The canonical public/MIT repository is
[enginsoysal/engacer-os](https://github.com/enginsoysal/engacer-os), independent
from BrixCanvas. The native English documentation site is
[engaceros.brixcanvas.com](https://engaceros.brixcanvas.com).
Current guides and official download articles update automatically from GitHub;
homepage editing and public release verification are separate required checks.
Historical Engine OS tags, binaries, evidence and internal ABI identities remain
unchanged. The previous releases below are historical qualification, not new media.

## Current development priority

[M11: familiar setup and desktop](../milestones/m11.md) proceeds strictly from
M11.0 through M11.6, with GitHub and public website verification between stages.
**M11.0 design is complete:** [interaction and visual contract](../architecture/desktop-m11.md),
four original concept screens, resource/safety bounds and the
[user-flow guide](../handbook/desktop-install-m11.md). These are design evidence.
**M11.1 native input is complete within its stage scope:** fourteen actual guest
tasks, production classifier checks, 535 source inputs and thirteen artifact
digests pass [the stage acceptance](../../evidence/M11/1/acceptance-results.json).
Selection/double-open, context/keyboard focus, Ctrl/Shift/marquee, release-outside,
drag threshold and the saved click interval include installed and detached startup.
[Implementation and decisions](../architecture/input-m11.md).
M11.2 through M11.6 implementation and final qualification remain pending.
The current tested download stays M10. Later hardware phases resume afterward.

## Current delivered scope

Common bounded device/capability reporting, Settings → Devices, saved US layouts,
working qualified keyboard/mouse/wheel/hubs/reconnect, guarded FAT32 USB file copies
and safe removal. New-artifact VMware controller installation/persistence/two
independent boots and the original 7-Zip MSI/console lifecycle pass. Native
ordinary/150%/4K movement, repaint, stable retained pages and halted idle CPUs have
separate proof. All normal tasks use the native graphical interface.

The standard VMware profile has 128 MiB and a new growable 20-GiB SATA disk.
64-MiB OVMF with boot-attached USB storage remains a recorded unqualified allocation
failure; its successful low-RAM journey connects USB after installed startup.
USB writes have bounded clean mirrored FAT32/BOT requirements. No universal
hardware, physical/ARM, vendor GPU 3D or general Windows compatibility is claimed.
See the release's exact limits; configured guest RAM is not memory consumption.

## Next hardware phases

3. Useful Ethernet/Wi-Fi configuration, downloads and reconnect.
4. Comprehensive speakers/headphones/microphone and audio device selection.
5. Qualified vendor GPU acceleration, APIs, display and power/error handling.
6. Original Windows Steam/game tasks with GPU, audio, input and persistent saves.
7. Laptops/docks/power, Bluetooth, other peripherals and separately qualified ARM.

The wider MS-DOS and Windows 1.x–11 program remains open; only the named original
7-Zip x64 console/MSI profile is qualified. Linux/macOS remain later execution
environments. Recognition is not usable hardware, and a launch is not a complete
application task. [Full hardware contract](../milestones/m10.md),
[Windows program](windows-compatibility.md).

## Historical milestone register

| Milestone | Status | Evidence/boundary |
|---|---|---|
| M10 hardware phases 1 and 2 | Complete within M10-DEVICES-VM-v1 | [27 exact-source/artifact gates](../../evidence/M10/acceptance-results.json), [release](../releases/m10-v0.10.0.md), [Devices/USB guide](../handbook/hardware-m10.md); later hardware phases remain open |
| M9.2 automatic writable storage | Historical complete within M92-STORAGE-VM-v1 | [Acceptance](../../evidence/M9.2/acceptance-results.json); its controller matrix is requalified on M10 |
| M0 architecture/toolchain | Complete | [51 technical checks](../../evidence/M0/README.md); no app compatibility claim |
| M1 own-kernel boot | Complete, virtual scope | [Historical evidence](../../evidence/M1/README.md); one VMware boot user-observed |
| M2 protected native processes | Complete within native-only, single-active-CPU scope | [Current M2 evidence](../../evidence/M2/README.md), [fresh acceptance](../../evidence/M2/acceptance-results.json) |
| M3 scheduling/general VM/concurrency | Complete within documented native VM scope | [Contracts and limits](../milestones/m3.md), [fresh acceptance](../../evidence/M3/acceptance-results.json); native execution on 1/2/4 CPUs, bounded threads/waits/COW/pressure |
| M4 persistent storage/input | Complete within documented native filesystem/virtual-device scope | [Fresh acceptance](../../evidence/M4/acceptance-results.json), [AHCI/ext2/VFS and USB/PS/2 contracts](../milestones/m4-contract.md) |
| M4.5 broader boot/hardware | Complete within M45-VM-v1 | [Fresh acceptance](../../evidence/M4.5/acceptance-results.json), [approved scope](../milestones/m4.5.md), [final ISO and VMware guide](../releases/m45-v0.4.5.md); dual BIOS/UEFI, ordinary storage, checked filesystems, driver status/input/hubs/UAS, faults/resource/SMP and 23 owner-trust checks; physical/factory trust and production key operations are separate unqualified follow-ups |
| M5 live/install/recovery | Complete within M5-VM-v1 | [Contract](../milestones/m5.md), [acceptance](../../evidence/M5/acceptance-results.json), [installation tutorial](../handbook/install.md); native installer, detached boot, bounded live/persistent volume and same-build repair |
| M6 real product installation | Complete within M6-VM-v1 | [Native wizard, storage and recovery](../milestones/m6.md), [exact acceptance](../../evidence/M6/acceptance-results.json), [VMware handbook](../handbook/install-m6.md) |
| M7.5 installer and installed restart | Qualified within M75-VM-v1 | [Nine exact-artifact gates](../../evidence/M7.5/acceptance-results.json), [novice guide](../handbook/install-m75.md); actual VMware BIOS/UEFI and retained/removed-ISO boots |
| M8 original Windows MSI/console | First app qualified within M8-MSI1-VM-v1; wider M8 in progress | [Exact evidence](../../evidence/M8/acceptance-results.json), [release](../releases/m8-v0.8.0.md); original 7-Zip x64 MSI, executable/DLL, archive tasks, removal and reinstall |
| M7.4 standard apps | First bounded delivery qualified within M74-VM-v1 | [Exact evidence](../../evidence/M7.4/acceptance-results.json), [all apps and limits](../handbook/apps-m74.md); 16 optional packages, source/data independence, real formats and audio |
| M7.3 native visual quality and HiDPI | Complete within M73-VM-v1 | [Current evidence](../../evidence/M7.3/acceptance-results.json), [VMware ZIP and ISO](../releases/m73-v0.7.3.md); measured typography, actual 4K, safe scaling and structured design review |
| M7.2 installation and folder desktop | Complete within M72-VM-v1 | [Historical evidence](../../evidence/M7.2/acceptance-results.json), [VMware starter and ISO](../releases/m72-v0.7.2.md); immediate real persistent desktop plus separately tested independent boot |
| M7.1 installer disk manager | Complete within M71-VM-v1 | [Exact acceptance](../../evidence/M7.1/acceptance-results.json); GPT/primary-MBR data operations, FAT32/Engine quick formats, explicit whole-disk replacement, equal-capacity disk isolation and detached desktop |
| M7 real desktop | Complete within M7-VM-v1 | [Exact acceptance](../../evidence/M7/acceptance-results.json), [delivery/limits](../milestones/m7.md), [VMware installation](../handbook/install-m7.md), [desktop controls](../handbook/desktop-m7.md); original DOS/Windows execution remains M8 |
| M5.1 guided UX/distributions | Complete within M51-VM-v1 | [Contract](../milestones/m5.1.md), [guided tutorial](../handbook/install-m51.md), [custom distributions](../handbook/distributions.md), [acceptance](../../evidence/M5.1/acceptance-results.json) |


Accepted historical reports and artifacts are immutable. Reproduce them only in
a separate checkout. Every new behavior updates English design, user instructions,
tests/evidence and GitHub together. No splash screen, stub or host-only probe
qualifies a runtime milestone.
