# Engine OS M7 desktop — development guide

M7 is in development, not an official release. The stable published ISO remains M6. The development desktop and secure-download path still require the complete native qualification matrix before a final M7 ISO is advertised. [Current status](../milestones/m7.md).

## Starting the desktop

Build separately with `python tools/build/m7.py`. Use a disposable VM with a new SATA/AHCI disk, two xAPIC CPUs, 128 MiB RAM and Secure Boot disabled. Boot `dist/EngineOS-M7.iso`, follow the native installation wizard, then power off and detach the ISO. Boot the installed disk to enter the actual desktop. No partition commands are needed. This is not yet a completed VMware desktop qualification claim. Keep the stable recovery media and never use a real personal disk as a development target.

## Everyday tasks

Open Files, Text Editor, Settings, Calculator or Downloads from the desktop icons or Start. Drag a title bar to move a window, drag its bottom-right corner to resize, use the underscore to minimize and the taskbar to restore it. The close button closes the selected window. The current model permits eight windows and keeps the taskbar reachable.

Text Editor creates a uniquely named document in Documents. Type and choose Save, or press Ctrl+S. Copy all, Cut all and Paste are available in the toolbar and through Ctrl+C/X/V. Arrows, Home, End, Backspace and Delete edit text. Closing a changed document asks whether to keep editing or discard changes. Reopen a saved file through Files after restart. The current editor has an explicit 8,191-byte bound and refuses overwriting a file changed by another open editor.

Files lists the actual installed volume. Open a folder or document by clicking its row. Up returns to the parent folder; New text opens an editor and New folder creates a uniquely named Documents folder. This bounded development file manager does not yet implement a general desktop trash, drag-and-drop, permissions UI or arbitrary app associations.

Settings changes the accent and dark/light appearance. Save preferences persists these choices. Removing Calculator deletes its actual optional executable without removing the desktop or documents. Reinstall uses the retained offline package cache. The base and Text Editor are required in this bounded package operation. System details exposes optional diagnostics; ordinary file tasks do not require the terminal.

## Downloads under qualification

Downloads shows actual Ethernet configuration and offers Retry network and an optional DNS check. The initial device family is Intel 8254x; runtime device qualification is recorded separately. Hardware RDRAND is currently required for secure randomization. Set the virtual RTC to correct UTC. Engine OS refuses invalid clocks and cannot safely bypass a certificate error. Proxy, Wi-Fi, IPv6 and captive-portal support are not implemented.

The default HTTPS address points to the public Engine OS MIT license. Enter downloads, or choose Download. Ctrl+L selects the address for replacement. Files are saved to a unique `DownloadN.bin` name in your Downloads folder; Open Downloads opens that folder. Only one transfer runs at a time, with at most four checked HTTPS redirects and a 64-MiB body bound. Cancel or closing Downloads cancels the transfer; a partial file is not added to the namespace. HTTP downgrade, credentials in URLs, unsupported encoding and invalid TLS are refused. The first native public-license download and restart persistence pass; negative security/network cases and the final platform matrix are still under qualification.

The current desktop is original native Engine OS code. It does not yet install or run original DOS/Windows applications; that is M8. [Architecture and limitations](../architecture/desktop-m7.md), [network/security implementation](../architecture/network-m7.md).
