# Devices and USB files in Engacer OS M10

M10 is qualified within M10-DEVICES-VM-v1. Use a fresh disposable VM and the
[official release](../releases/m10-v0.10.0.md). Historical Engine OS releases
retain their original names.

## See what is usable

Open Start → Settings → Devices. Ready means the indicated native function is
available. Limited means a useful subset, such as software/VMware 2D display.
Detected means recognition without a usable function claim. Unavailable means
the device has no accessible function at present. Refresh updates provider status.
No manufacturer installer is needed for the supported profiles.

Select US QWERTY or US Dvorak here; the choice saves automatically. Files accepts
scroll-wheel input while focused. International input methods and all HID report
formats are not part of this release.

## Copy from a USB drive

1. Connect disposable FAT32 USB media to your test VM through VMware's removable
   device menu. Connect it to the guest, not both the host and guest at once.
2. Open Files and choose USB drives in the left sidebar.
3. Choose the drive row, then click folders to browse. Up returns to its parent.
4. Select a file and choose Copy selected to Downloads. Keep the drive connected.
5. Choose Your folders, then Downloads to find the copy. Existing files are kept;
   a repeated name receives a numbered alternative.

## Copy to USB and disconnect

1. In your native Documents or Downloads folder, choose To USB beside a file.
2. Choose a drive and destination folder, then Copy here.
3. Wait for the copied-and-flushed confirmation. An existing destination name is
   refused; use another folder or rename your native source.
4. Choose Safe remove. Only unplug after the Safe to remove confirmation.
5. Reconnect the drive to use it again. The supported driver binds automatically.

Do not disconnect during a transfer. If an I/O error occurs, the volume may need
an offline FAT check before it accepts another write. A failed action does not
mean safe removal. Copies display a keep-connected message and currently cannot
be cancelled midway.

## Supported limits and troubleshooting

The supported write profile is USB BOT with 512-byte sectors, flush support and
a clean mirrored FAT32 volume. UAS and write-protected media can remain read only.
Only the first supported FAT32 volume is opened. ExFAT, NTFS and arbitrary USB
bridges are not implied. Each transfer is one file, up to 64 MiB; export names
are printable ASCII up to 63 bytes and graphical import names up to 55 bytes.
Free-space fragmentation or a full directory can prevent a copy even when the
drive appears to have capacity. The broker does not grow directories or repair
damaged FAT metadata. Partition formatting and OS installation on USB are separate
features and are refused here.

If no drive appears, check the VM's USB connection and controller, then reopen
USB drives. Settings → Devices identifies the bound controller and device counts.
Do not format a valuable drive just to test support. The downloadable starter
uses a blank SATA disk for installation and enables its USB controller.

This phase does not deliver Wi-Fi setup, comprehensive audio, vendor GPU 3D,
multi-monitor support or Steam gaming. Those remain later hardware phases.
