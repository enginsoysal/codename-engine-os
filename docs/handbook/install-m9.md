# Install Engine OS M9 and check window movement

M9 starts with automatic supported VMware graphics and pointer setup. Use the matching M9 ISO and VMware package together. Qualification status and remaining work are recorded in the [M9 contract](../milestones/m9.md).

## Easiest VMware start

1. Download **EngineOS-M9-VMware.zip** from the official GitHub release and extract the entire ZIP into a new local folder.
2. Open **EngineOS.vmx** in VMware Workstation. Choose **I Copied It** if asked.
3. Start the VM. Its new growable 20 GiB SATA disk is blank. The prepared configuration uses two virtual CPUs, 128 MiB RAM, x64 UEFI and Secure Boot off.
4. Choose **Next**. Click the disk row inside the bordered list and check its selected appearance. Choose **Next**.
5. Review Components and Your style. Set the account name, accent and options you want; Back keeps your choices.
6. Read Review and choose **Install Engine OS**. Existing-disk replacement requires separate erasure consent. The prepared empty disk is the simplest test target.
7. Choose **Restart to Engine OS**. This starts the installed system. The matching ISO may remain connected.

No network connection, terminal commands or guest driver installer are required. The FreeBSD configuration label is a VMware device preset; Engine OS uses its own kernel. Keep this unsigned development build's Secure Boot setting off.

## Try the desktop

Files opens Home. Click Documents, create a named folder, open it and use Back or Up to navigate. Your files are stored on the installed disk. Drag Files by its title bar. The supported VMware graphics and absolute-pointer paths bind automatically.

Open Appearance to choose light/dark style, accent and Auto/100%/150%/200% scaling. Scaling changes text and controls; it does not change the firmware's physical display resolution. Stretching a low-resolution guest image in VMware can still look blurry. Runtime resolution changes and 3D acceleration are outside this first graphics delivery.

System Monitor reports the selected graphics backend. A buffered framebuffer fallback can be valid even if the SVGA protocol is unavailable. The optional composition buffer uses approximately 3 MiB at 1024×768 or 31.6 MiB at 3840×2160, with a 32 MiB cap and a free-memory reserve. This buffer is retained RAM, not free VRAM.

## Check independent startup

Power off. Disconnect only the CD/DVD ISO and keep Hard Disk (SATA) connected. Start again: the installed desktop and your folder should remain. Reconnect the matching ISO for reinstall or ISO-assisted recovery.

For a custom VM use a qualified SATA/AHCI disk with 512-byte sectors and at least 2 GiB. A firmware **No Media** message means no bootable device was found; it is not fixed by installing a Windows application. Prefer the prepared VM for the first test. Existing-disk choices are explained in the [disk-manager guide](disk-manager-m71.md).

The original 7-Zip console profile remains available through **Start → Windows Apps**. Its [app guide](windows-apps-m8.md) still describes the supported original MSI/executable and its limits. M9 does not add arbitrary Windows GUI applications or DOS support.
