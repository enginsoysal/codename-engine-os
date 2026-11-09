# MS-DOS and Windows 1.x–11 application requirement

The product goal is a standalone bootable/installable OS in original source code that can install and run existing MS-DOS programs and applications made for Windows 1.x through Windows 11 using their original installers. The first major app path remains 7-Zip, followed by larger client/game/creative targets. This is an ambition with explicit qualification, not a universal error-free guarantee.

C17/assembly is the current implementation language; no new programming language is planned. No Microsoft/FreeBSD/Linux kernel is required underneath. Appropriate open-source components may be ported after provenance/license review. App compatibility is behavioral; pixel-identical Windows UI and all Microsoft cloud/server/management features are not automatically included.

## Coverage audit

| Requirement | Route | Current truth |
|---|---|---|
| ISO/USB boot | M1 | Own kernel tested in QEMU; user-observed VMware M1 boot |
| Install OS to disk and boot independently | M5 | Implemented within M5-VM-v1; [VM qualification](../../evidence/M5/acceptance-results.json). This does not implement Windows app installation. |
| Protected process foundation | M2/M3 | Native64 isolation, threads/waits/VM/SMP verification only; no Windows APIs |
| Win32/Win64/NT interfaces | M8.4/M8.5 | Selected Win64 interfaces qualify one original 7-Zip console profile; general Win32/NT remains planned |
| DOS programs/games | M8.1 | Separate future legacy runtime |
| Windows 1.x/2.x/3.x and 16-bit installers | M8.2/WC2 | Explicit separate route; not supplied by PE32/DOS alone |
| 9x versus NT behavior | WC3 | Profiles/semantics must be implemented |
| Modern runtimes/package forms | WC4 | Dependency inventory and task qualification needed |
| Old/modern graphics and media | WC5/M8.6 | CPU boot framebuffer is not DirectX support |
| Guest drivers/DRM/anticheat/services | WC6 | Separate feasibility/access blockers |
| Every app without defects | WC7 | Not an available or verifiable global guarantee |

## WC1 — application/installer inventory

Record exact app/version/hash, original target generation, installer and payload CPU architectures separately, APIs/runtimes, drivers, external services/hardware, installation/start/use/reboot/uninstall tasks, provenance and evidence. [Machine-readable inventory](windows-targets.json) contains thirteen generation groups, including separate DOS, Windows 1.x, 2.x, 3.x and NT 3.x profiles; one exact original 7-Zip 26.06 x64 console/MSI profile is qualified within M8-MSI1-VM-v1; general generation profiles remain planned. M2 native probes are not Windows applications.

Why: a 32-bit application can use a 16-bit installer. Version labels do not identify dependencies. Acceptance requires a complete record for each published claim; a compatibility percentage needs a defined population. Proprietary installers do not enter the public repository automatically.

## WC2 — Win16 and historical installation chains

Implement/investigate NE loading, Win16 APIs, segmented pointers, message/thread semantics, 16↔32-bit thunks and the needed isolated CPU/device runtime. Share DOS machinery only where behavior matches; it does not provide Win16 GUI interfaces.

Why: modern PE32/PE32+ coverage misses old installers and apps. Qualification requires an original Win16 app and a 32-bit payload with a 16-bit installer, including reboot/start/uninstall and bounded file access. Legacy code never receives uncontrolled host-kernel privileges.

## WC3 — Windows 9x and NT generation profiles

Compare and implement real ANSI/Unicode, paths, registry, IPC, timers/shared-state and graphics differences. Profiles must provide behavior, not merely report a different OS version. Keep conflicting expectations isolated while preserving intentional app cooperation.

Acceptance: named targets across generation groups install/start with an explicit profile and visible deviations. Initial UEFI-only host boot remains valid; supporting a 1995 app does not require booting on a 1995 motherboard.

## WC4 — runtimes, services and package forms

Inventory EXE/MSI, mixed 16/32/64-bit installers, AppX/MSIX, COM/OLE/ActiveX, side-by-side assemblies, VB/VC runtimes, .NET Framework/modern .NET, browser/web helpers, services/tasks and elevation. WinRT/UWP/Windows App SDK require their own dependency chains and tests.

Why: the binary loader is a small part of an application's environment. Repair/update/uninstall matter as much as initial setup. Every claimed route needs a real app/task; missing services must not be no-op success stubs. Distribution and external-access conditions remain explicit.

## WC5 — historical and modern multimedia

Map GDI/DirectDraw/Direct3D/audio/input/media generations to native compositor/drivers, with format/timing/fullscreen and modern shader tests. API translation does not replace a GPU driver.

Acceptance: correct output/input/audio and stable start on named hardware per route, with task-specific frame-time/resource budgets. Hardware requirements remain physical constraints.

## WC6 — driver-bound apps and external authority

Inventory VxD/NT/filter/anticheat/DRM and device dependencies. Select an original functional alternative, a real guest-driver compatibility implementation, or an isolated legacy machine environment where feasible. There is no proven solution for every class.

Kernel ownership cannot recreate a closed activation server, unavailable physical device or provider permission. An unresolved mandatory dependency means the app is not supported. Initial-release exclusion of full guest-driver execution does not silently remove these apps from the ultimate product goal.

## WC7 — qualification and regressions

Use the original installer in a clean environment, install, first launch, OS reboot, repeated launch, defined core functions, save/reopen and uninstall. Inspect child processes/services, error paths, data integrity and isolation. Automate reproducible parts and state manual tasks.

Acceptance for a published app/version/task matrix: no known critical or blocking defect in those tests; other limitations remain visible. Installed, launchable, usable and performant are separate results. Unknown apps/inputs remain unknown, not implicitly qualified.

## Implementation order

Complete the revised M6 real product installation and M7 real desktop contracts first. M8 then qualifies original DOS and Windows application workflows through the separately tracked M8.1–M8.7 routes. See [active milestone contracts](milestones.md). The product remains Engine OS; the first bounded delivery is documented in the [M8 MSI contract](../milestones/m8-msi.md) and its exact acceptance report.

Early Windows executable formats, segmented memory and APIs must be inspected per original version. Windows 95 compatibility does not establish Windows 1.x–3.x or MS-DOS compatibility. Installer and payload architectures are recorded separately.
