# Install Engine OS and try the original Windows app

The M8 first profile adds original 7-Zip 26.06 x64 console execution to the native Engine OS desktop. Use the [matching release and checksums](../releases/m8-v0.8.0.md); a locally built ISO alone is not acceptance.

## Start in VMware

1. Extract the complete matching **EngineOS-M8-VMware.zip** into a new local folder.
2. Open **EngineOS.vmx** in VMware Workstation. If VMware asks, choose **I Copied It**.
3. Start the VM. The package contains a new, empty, growable **20 GiB SATA disk**, two virtual CPUs and 128 MiB RAM. Keep Secure Boot off.
4. Choose **Next** on Welcome. On Your disk, click the disk row inside the bordered list; its selected appearance confirms your choice. Then choose **Next**.
5. Review Components and Your style. Choose an account name, accent and the options you want. Back keeps your choices.
6. Read Review, then choose **Install Engine OS**. Existing-disk replacement has separate erasure consent; use the included empty test disk for the easiest start.
7. When installation completes, choose **Restart to Engine OS**. This starts the actual installed system. The matching ISO may remain connected.

No network, terminal commands or host Windows installer are needed. `freebsd-64` in the VMware configuration is a virtual-device preset; Engine OS runs its own kernel.

## Install and use 7-Zip Console

1. Open **Start → Windows Apps**.
2. Optionally choose **Review installer** to inspect the included original MSI.
3. Choose **Install 7-Zip**, read the confirmation and confirm **Install 7-Zip**.
4. Choose **Open 7-Zip**. The output comes from the original Windows executable and its original `7z.dll`.
5. **Create test archive** creates a separate sample document in Documents and asks the original program to archive it. **Extract test archive** extracts it into `Documents/EngineOS-Windows-Extracted`. Repeating the examples creates new numbered archives/folders and keeps earlier results.
6. **Remove app** asks for confirmation and keeps your documents. You can install the same app again.

This is the **console** application. Its Windows file-manager GUI and Explorer integration are unavailable. Other MSI packages are refused in this first profile. [Detailed app guide and limits](windows-apps-m8.md).

## Check independent startup

Power the VM off. Disconnect only the CD/DVD ISO, keeping Hard Disk (SATA) connected, then start again. The desktop, original app and your documents should remain available. To reinstall Engine OS, reconnect its matching ISO.

For an existing custom VM, select BIOS or x64 UEFI, disable Secure Boot and attach a SATA/AHCI disk with 512-byte sectors, at least 2 GiB. VMware SCSI and other unqualified device combinations are not interchangeable with the supplied profile. A No Media message is a firmware/device discovery problem, not a prompt to run the MSI on the host. Prefer the prepared VM when checking the release.

## Appearance, optional apps and recovery

Use Appearance for the accent, light/dark style and Auto/100%/150%/200% display size. Files browses real folders. Software Manager removes optional native apps without removing shared core services; Windows Apps manages the foreign product separately. Removing the Windows Apps frontend does not remove unrelated applications or user documents.

Cancel stops a Windows job. A fresh failed installation attempts to clear its new files. If cleanup was interrupted, **Recover / remove** uses the surviving ownership plan; an empty incomplete product directory can also be cleared. Invalid ownership records are refused. Repairing the OS boot files remains an ISO-assisted operation with its own limits. [Disk management](disk-manager-m71.md), [native app capabilities](apps-m74.md), [first-profile scope](../milestones/m8-msi.md).
