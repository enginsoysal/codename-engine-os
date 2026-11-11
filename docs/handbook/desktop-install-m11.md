# M11 installation and desktop guide — staged development

M11.0 through M11.4 are implemented and qualified within their individual stage
scopes. Use the [M11.4 installation tutorial](install-m114.md) for its actual
seven-step wizard and downloads. [Files](files-m11.md), [Start/taskbar](start-m11.md)
and [windows/appearance](windows-m11.md) describe current native behavior.
The expanded installation journey below is the ordered M11.5 contract;
full final regression is M11.6. Each remaining stage has its own public release,
manuals and fresh website/download verification before proceeding.

## Planned M11.5 installation journey

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
Menus now expose genuine provider-dependent actions. In Settings, click the double-click
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

## Implemented through M11.4

The interaction stage, [native Files](files-m11.md), [Start/taskbar](start-m11.md)
and [windows/appearance](windows-m11.md) have actual guest and saved-disk evidence.
[M11.4 release](../releases/m114-v0.11.4.md) includes its exact tested VMware
starter. The expanded setup and complete final regression remain M11.5 and M11.6.
