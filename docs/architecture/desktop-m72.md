# Desktop-first installation and Files

Owner priority, 7 November 2026: deliver a beautiful, convenient native installer and a visible, usable desktop with folder navigation before adding applications or Windows compatibility. This follow-up is M7.2; historical M7/M7.1 proofs remain frozen.

## Why the installation now ends inside the desktop

The old Finish page requested a manual power-off/media-disconnect/restart and its Finish button only advanced a page. It did not take the owner to a desktop. The new native wizard keeps the verified storage transaction, retained Back/Next choices, explicit destructive consent and capacity checks, but presents a new light content surface, dark step rail, native desktop illustration, consistent spacing and clearer progress/copy.

After successful installation, the installer already has a mounted persistent user volume and saved preferences. **Open desktop** passes those verified preferences to the same native compositor used on installed boot. It does not invent an in-memory directory tree. The installation kernel remains resident for that first session; independent installed boot uses the smaller installed kernel and is tested separately. This avoids a restart prerequisite for seeing the result while preserving honest boot evidence.

The ISO's explicit **Open installed desktop** action permits read-only discovery and mounting of exactly one matching installation, including build, activation, GPT, volume and settings checks. The same-build check prevents opening unsupported mixed-version user state. Automatic unqualified version migration or guessing among multiple installs was rejected. Recovery/Update remains available separately; it is not a silent erase-and-reinstall path.

## Boot evidence and the reported firmware screen

Earlier VMware tests forced `bios.bootOrder` and created a fresh VM configuration for each installed phase. That was a coverage gap. M7.2 keeps the same VMX and saved NVRAM, leaves the optical device present but disconnected, and tests two normal disk starts with no forced order. QEMU tests reuse installation firmware variables and remove both order forcing and device bootindex. A fresh M7.1 UEFI baseline also boots unforced; the user's particular firmware No Media cause remains unproven without that VM's connection/controller information.

The expanded test reproduced a distinct failure after UEFI USB installation: the installed ESP was readable but the retained firmware BootOrder returned to Shell. Relying only on removable-media fallback did not register the fixed-disk OS. The original loader now creates/readbacks an active Engine OS Boot#### short-form File Path option for the stable `\EFI\BOOT\BOOTX64.EFI`, and puts only that option first while preserving every foreign option and their relative ordering. The same fallback path is retained by older verified system slots, avoiding a stale version-specific loader after rollback. A separate duplicated loader at an Engine OS-only path was rejected because an old-slot replay removes that path. [UEFI 2.11 boot-manager load-option processing](https://uefi.org/specs/UEFI/2.11/03_Boot_Manager.html) specifies short-form file-path search across system partitions.

This first scope supports a single Engine OS target VM. A short-form fallback option searches available system partitions rather than binding one disk GUID; multiple operating systems/ESPs and dual-boot priority are unqualified. Disconnect installation media for ordinary installed boot. Booting the ISO on a physical/multi-OS machine is outside this qualified scope and would still add the disclosed firmware entry.

Registration happens in the UEFI loader before ExitBootServices, using firmware variable services while their environment is valid. It is idempotent, bounded to 256 BootOrder entries and a project-selected 256-ID search range, skips occupied foreign identifiers, rejects malformed/full state and verifies writes by reading them back. A failed registration refuses UEFI kernel entry with a diagnostic rather than pretending startup was configured. Writable NVRAM is therefore required for this UEFI profile. BIOS is unchanged. The entry can remain after cancelling installation; this firmware mutation is distinct from disk erasure, disclosed on Welcome and in the guide. A post-ExitBootServices runtime bridge was rejected here because it would require a separately qualified firmware ABI/memory/interrupt boundary. No boot-services call is made by the installed kernel.

Host tests execute this same C with independent variable storage: they decode the resulting device path, verify zero writes on a second call, preserve occupied/oversized foreign options and duplicate foreign ordering, and reject read-only/malformed/exhausted cases without changing foreign data. Native USB/ISO and persistent VMware startup remain the actual boot proof.

The prepared VMware package adds a new, empty growable SATA disk and relative ISO/VMX paths. It does not ship a preinstalled desktop, change an existing VM or require a remote-control setting. Its exact archive is extracted, instrumented only with owned COM1/loopback display observation, installed and booted in the acceptance suite. Networking is disabled for this test to demonstrate the requested tasks have no network dependency.

## Files instead of optional-app expansion

The desktop foregrounds Home and Documents, with Settings for appearance. Files opens larger by default and provides a places sidebar, visible path, Back/Forward/Up, explicit list paging, folder icons, readable names, item count and a real named-folder dialog. Personal folders use the existing copy-on-write extent volume. System browsing uses the same mounted volume but blocks system item modification through Files, including launching an editable system file. This is a trusted UI policy, not a new ACL/security boundary.

Eight per-window histories retain eight 256-byte paths plus counters each: 16,448 static bytes total. State is reset when a Files window is created, including reused slots. No heap allocation occurs on navigation; counting children uses one bounded 1,024-entry pass, with visible row lookups separately bounded. Long names are shortened visually; navigation always uses the full underlying path. New names reject collisions, invalid endings and paths reaching 256 bytes. Cancel and failed creation do not delete existing content.

The existing eight-window generation model, clipping, damage painting, input interrupts and halt-at-idle remain. Original Editor and Calculator implementations are retained without feature expansion. Calculator is absent by default; the desktop and folder tasks do not require its package. Network functionality remains present but is unnecessary for these acceptance tasks.

## Customization and limits

The standard profile is [engineos-workspace.json](../../distros/engineos-workspace.json), using the validated schema 2. `python tools/build/m72.py --profile <profile> --revision <name>` writes separate named media. Profile choices affect implemented defaults and content; they cannot remove disk confirmation, storage checks or required components. Installed personal preferences survive an update rather than being overwritten by distro defaults.

No new foreign application compatibility, arbitrary hardware support, resize/move of partitions, selected-partition OS installation, general theme engine, multiple user authentication or mounted foreign volumes is claimed. M72-VM-v1 qualifies the exact x86-64 BIOS/UEFI guest artifacts with unsigned boot and SATA/AHCI 512-byte sectors. Automated task completion and captured native frames are not a human usability study. [Milestone and tests](../milestones/m7.2.md).
