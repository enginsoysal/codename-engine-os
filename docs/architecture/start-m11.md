# M11.3 — Start, catalogue and taskbar

Follow the ordered [M11 contract](../milestones/m11.md). M11.0–M11.2 and their
public publication gates are closed. The native M11.3 stage passes
[22 native tasks and production checks](../../evidence/M11/3/acceptance-results.json).
M10 is the tested download until M11.6. Public publication is a separate gate
before M11.4.

## Model and persistence

Start uses native installed-package catalogue IDs, including required Files and
Settings. It does not store an executable path for each app. A removed optional
package disappears from search and both pin sets. A failed preferences save
cannot reactivate it: availability is checked again at launch. Windows Apps opens
the existing bounded original-app integration; arbitrary Windows registration
and compatibility are not claimed.

One versioned per-user record stores up to twelve categories, 32 Start pins,
eight taskbar app pins, category assignments/order/visibility, optional pin labels,
list/grid mode, selected tab and alignment. Validate magic, version, counts,
category permutation, strings, duplicates and IDs before use. Corrupt records
activate defaults with an explanation. There is no growing query/history heap.
Search compares local catalogue names and pinned-folder labels; it never starts
a process per character or sends the query to a search service.

Default categories are Accessories, System, Internet, Media and Development.
Users can create, rename, hide, show and reorder categories and reassign an app
through its context menu. Hiding does not uninstall or make an app unsearchable.
Pins can be renamed, reordered and removed without deleting their targets.
Folder pins point to real native directories, follow native rename/move and are
removed when their targets are recycled. Missing folders explain a failed open
and can be unpinned. Pinning does not copy shortcuts or folder contents.

Start items launch on one click; desktop/Files retain selection/double-open.
Pin identity and catalogue IDs guard release-time activation against changed
lists. Launch retains eight-window admission. A ninth launch reports the limit
and preserves current windows.

## Taskbar and keyboard

The taskbar separates pins from actual window state. A pin launches an app or
restores/minimizes its most recent open window. A window button uses its
generation-checked token. Active/running/minimized indicators are distinct.
Narrow layouts expose bounded Pins/Windows overflow menus. Left/centered alignment
persists. A modal owner can be restored without closing its unfinished dialog.

Start stays open while its context/category menus are used. An outside click
dismisses it without opening a covered desktop item. Both Windows keys, F10 and
Ctrl+Escape have implemented input routes. Search, arrows, Tab/Shift+Tab, Enter,
Escape and Shift+F10 have native routes. Mixed mouse-button chords cancel capture.

## Actual power operations and explicit boundary

Power opens a separate cancel-first review. Unsaved work, active file transfers,
Windows installation jobs, downloads, a read-only system volume or a failed disk
flush prevent a successful power claim. Writable registered disks must confirm
cache flush. Restart uses a supported ACPI I/O reset register when available,
then existing PC reset paths. Shutdown is offered only after a bounded virtual
provider validates ACPI root/FADT/DSDT and a static S5 package.

This is not a general ACPI/AML interpreter or physical power qualification. The
provider accepts recognized QEMU/KVM/VMware CPUID identities, checksum-checked
tables below 4 GiB, bounded static S5 integer packages and supported legacy or
System-I/O PM1 control registers. Dynamic AML, HW-reduced and unsupported MMIO
power interfaces are refused. Arbitrary firmware methods, physical sleep,
battery, lock and account switching are not implemented here. Qualification is
limited to executed tests; broader VMware tests remain M11.6.

The design follows the primary [ACPI 6.6 table model](https://uefi.org/specs/ACPI/6.6/05_ACPI_Software_Programming_Model.html)
and [sleep/shutdown model](https://uefi.org/specs/ACPI/6.6/16_Waking_and_Sleeping.html).
Full firmware-method evaluation is deferred to the hardware program. Restricting
the provider avoids offering generic shutdown based on enumeration alone.
Native tests must observe actual reset/new kernel and QEMU S5 exit, rather than
a simulated success label or halted GUI.

## Closing evidence

Production preference/package validation; actual native search/category/pin/
removal/restore/window/power tasks; independent stored preferences and installed/
detached reboot; keyboard, quota and pagination checks; exact source/artifacts.
Record fixed BSS/data separately from managed pages/framebuffers. Then update
English guides and GitHub, publish the native homepage and verify fresh public
articles before M11.4.

A folder transaction and the following preferences save are separate durable
updates. Interruption between them can leave an unavailable pin after restart;
repinning repairs the menu without rolling back or losing the folder. Launch
always checks the real target. The native catalogue currently has twenty ID slots
(including zero); adding app kinds requires revising the record bounds/schema,
not inserting executable paths into preferences.

Build `491380a834029bdeb591520e97dd512a`, ISO SHA256
`9467bd0c4e62668e82f54295d79f30624d10544b922541472efdf0e5c46dce6f`,
passes 22 native tasks, with 551 source inputs and thirteen artifact digests
verified. Actual installed restart, QEMU S5 exit, detached boot, USB-HID event
counters, PS/2-only input, optional removal, pin order, quota and stored settings
are distinct checks. [Exact evidence](../../evidence/M11/3/acceptance-results.json).

The installed desktop object's fixed BSS/data grows by 12,256 bytes from M11.2,
below the 64-KiB stage budget. Native typing leaves retained managed pages
unchanged and starts no app process until launch. This is separate from total
kernel/static/firmware/framebuffer consumption and configured guest RAM.
