# M11 installation and desktop guide — staged development

This describes the approved M11 workflow. Check the
[stage status](../milestones/m11.md) before expecting a feature in downloadable
media. M10 remains the qualified download until M11.6. M11.0 provides designs;
M11.1 now implements native selection, double-click, context/focus and release
behavior in a separately tested development candidate. File operations, Start,
window/visual changes and the expanded setup below belong to M11.2–M11.6 and
remain staged work. New media identify their exact qualified scope.

## Installing

Start from the prepared disposable VMware package, choose I Copied It if asked,
and boot the matching ISO. Follow Welcome, keyboard, disk, components, user/style
and Review. Click the disk row in the list; selection must be visibly highlighted.
Use Disk tools for supported partition tasks. Read the final target/erase review,
then choose Install Engacer OS. Keep other disks and valuable host storage outside
the test VM. Finish with Restart to Engacer OS and reach the installed desktop.

Back preserves your choices. Unsupported settings are explained. Format and
replacement require explicit confirmation for the named target. The wizard is
usable offline. Recovery details are optional; normal use requires no terminal.

## Selecting and opening

Click a file, folder or desktop icon once to select it. Double-click or press
Enter to open it. Buttons and Start entries run with one click. Ctrl adds/removes
individual selected items; Shift extends a range. Drag empty space to select a
rectangle. Right-click the selection for its actions; Escape closes the menu.
M11.1 provides the initial Open/help/Close menu. In Settings, click the double-click
interval to cycle 200, 500 and 900 ms; the choice is saved and restored on restart.

Use Copy/Cut then Paste, or drag with a visible copy/move indication. Name conflicts
must present a choice rather than overwrite silently. Recycle native user files
to the Recycle Bin; restore puts them back after any collision review. Permanent
deletion is a separate explicit action. System paths and USB media offer only
their supported operations. Read the confirmation before disconnecting USB.

## Start, windows and your style

Open Start to search locally or browse Pinned, All apps and Categories. Pin/unpin
installed apps and folders; rename/reorder your categories. Taskbar entries open
or restore windows and show the active one. Double-click a title bar to maximize
or restore; drag its edges to resize and use Alt+Tab to switch.

Personalize chooses light/dark, accent, wallpaper, scaling and taskbar alignment.
Choices, pins and view settings survive restart. Removing an optional app must
not remove Files, Settings or the desktop. Only working power operations appear.

## What qualifies completion

An installed reboot, persistent files/preferences, actual mouse/keyboard tasks,
ordinary/4K readability and error-path tests are required. Design images and a
live installer desktop are not completion evidence. Original Windows application
compatibility retains its separate bounded profile and roadmap.

## Implemented through M11.2

The interaction stage and [native Files guide](files-m11.md) now have real guest
and persistent-disk evidence. The Start/window/setup sections above remain the
ordered contract for M11.3–M11.5. Downloadable M11 media follow M11.6; use the
current M10 installation manual for the existing public release.
