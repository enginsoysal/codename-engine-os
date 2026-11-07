# Install Engine OS M7.4 in VMware

M7.4 is being qualified. Do not treat a development ISO as an accepted release. [Application guide](apps-m74.md).

## The convenient route

Extract the complete EngineOS-M74-VMware.zip into a local folder. Open
EngineOS.vmx in VMware Workstation. If asked whether the VM was moved or copied,
choose **I Copied It**. The package includes a new, blank, growable 20-GiB SATA
disk and the matching ISO. It is not a preinstalled desktop.

The starter configures NAT networking and HDA audio. Neither is required for installation. Keep Secure Boot disabled for this unsigned build.
The UEFI loader can create a startup entry even if you later cancel setup.

## Follow the wizard

1. **Welcome:** choose Next to install. Open installed desktop is for an
   existing compatible installation; Recovery exposes repair/update/rollback.
2. **Your disk:** select Disk 1. Check its capacity. Manage disks is optional.
   Installation uses the whole disk. For a disk containing data, enable Erase
   and install; you must also confirm its consequences at Review.
3. **Your setup:** Desktop and Files are included. Choose the optional Calculator and
   welcome guide. Other standard apps are selected by the distribution profile. Removing optional content does not remove the desktop.
4. **Your style:** keep Owner or edit the account name. Choose an accent.
5. **Review:** check the disk, account and choices. Back preserves your choices.
   Only choose Install when the whole selected disk may be used. An existing
   disk requires the separate erase checkbox; simply clicking Install does not
   bypass this guard.
6. **Installing:** keep the disk connected. Progress represents native copying
   and verification. A failed operation is not reported as successful.
7. **Your desktop:** choose Open desktop. Files opens on the real persistent
   installed volume, in the same session.

The ISO has a graphical wizard, not a command prompt requirement. Mouse works;
Tab and Shift+Tab move between controls, and Enter activates the selected one.

## Check independent startup

Power off the VM. Disconnect **only the CD/DVD ISO**. Keep the SATA hard disk
connected, then power on again. Engine OS should start from its installed
disk. Keep the ISO available for recovery. The exact accepted virtual profiles
are listed in the release evidence; universal physical hardware is not claimed.

## Readable display sizing

UEFI uses the framebuffer supplied by firmware. Where the active EDID protocol
is available, the loader can select its matching preferred GOP mode up to
3840×2160. Otherwise configure the preferred resolution in the VM's firmware
if necessary. A host monitor being 4K does not automatically make the guest
framebuffer 4K. BIOS uses the existing 1024×768 mode.

Auto scales the UI for high-resolution framebuffers. On the desktop choose
Appearance, then Display size: Auto, 100%, 150% or 200%. A setting that leaves
too little logical workspace is refused. Choose Save preferences to retain the
choice after restart. Text is rasterized at the actual physical size, rather
than enlarging the original pixel font.

[Application guide](apps-m74.md) · [Disk manager](disk-manager-m71.md) ·
[Graphics scope](../architecture/graphics-m73.md) ·
[M7.4 qualification](../milestones/m7.3.md).
