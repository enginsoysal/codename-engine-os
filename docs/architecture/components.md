# Independent components and customizable setup

Owner requirement, 6 November 2026: Engine OS must offer a graphical Back/Next/Finish installer, extensive customization, and removable optional applications without breaking unrelated system functions. This is a delivery requirement, not a claim that a package manager or graphical installer is implemented.

## Component boundaries

The dependency direction is applications → public services → kernel mechanisms. The desktop, Settings, installer and recovery must never depend on Calculator or another optional end-user app. Shared libraries/services belong to separately identified packages; they must not be hidden files owned exclusively by an optional app. A native application cannot obtain unrestricted kernel access merely because it ships with the distro.

Record component identity/version, required dependencies, optional integrations, owned files, resources, supported replacement providers and removal class. Core boot/storage/recovery components are required for this installed profile. Protect them from ordinary removal; support versioned replacement with a verified fallback rather than promising the running kernel can disappear. Alternative shells/providers are allowed when the replacement satisfies the declared interface. A theme cannot bypass integrity checks or destructive-operation confirmation.

## Removal behavior

Before changing anything, compute the required reverse-dependency closure and display the exact affected components. Removing Calculator must remove only Calculator and its app-owned installation files. Desktop, Settings, file manager, installed user documents, boot and recovery must remain usable. Optional launchers/integrations disappear cleanly; unavailable file associations offer another provider rather than crash. Keep a shared dependency while any installed component requires it. Never silently cascade from a small optional app to the desktop or base system.

A component required by another installed package needs an explicit resolution: retain it, replace its provider, or present the exact dependent removals for review. Uninstall defaults to retaining personal app data; a separate clear choice may remove that app's private data, never shared documents. Updating/removing executable code requires a versioned plan, active-process handling and a recoverable durable transition. These operations must not share an unrestricted arbitrary shell-hook mechanism.

## Required evidence

After implementing native package operations, test install-without and install-then-remove for each optional shipped app. For Calculator: uninstall, relaunch the desktop, open Settings and file manager, edit/save/reopen a document, reboot and repeat. Test shared dependency retention, rejected core removal, missing optional services, stale launchers, replacement-provider failure, cancellation and interrupted uninstall/update. Compare retained/peak memory before and after and confirm no idle polling. These are future acceptance tests; a browser checkbox does not satisfy them.

Distribution recipes may select implemented components and presentation defaults. Unsupported fields/features must be rejected. Current M5.1 recipes remain data-only and have not acquired executable package support through this document. [M6 wizard requirements](../handbook/installer-design.md), [product principles](product-principles.md).

## M7 implemented native component slice

The M7 desktop installs separate original Editor and Calculator ELF programs. Its bounded package operation permits actual Calculator executable removal and reinstalls from a retained offline cache; it refuses required Editor removal. Native tests retain Files/Settings/Editor, clipboard and personal documents across removal and restart. This is a fixed shipped-component policy, not a general dependency solver or arbitrary package installer. User applications have actual CPL3 state and failure boundaries; the UI/file broker remains trusted supervisor code. [Detailed implementation and limits](desktop-m7.md), [native evidence](../../evidence/M7/isolation/results.json). M7 networking and complete named-scope acceptance now pass; arbitrary third-party package support remains future work.

## M7.2 focused presentation

The standard workspace profile omits Calculator and foregrounds Files, Home and Documents. The launcher presents Files, Settings and optional diagnostics; existing app code is retained without expanding it. Removing or omitting Calculator does not prevent installation, immediate persistent desktop entry, folder navigation or later disk boot. [M7.2 implementation and actual native tasks](desktop-m72.md).

## M7.4 component classes

The new catalogue distinguishes actual CPL3 event programs from fixed declarative
broker frontends. Both have independent app-owned installation files and retained
offline reinstall sources; required shared code remains protected. Frontends do
not allocate a no-op process. Static broker memory is explicitly separate from
unloadable worker pages. [How, why and current limits](applications-m74.md).
