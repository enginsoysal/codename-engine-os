# Install Engine OS M7.5 in VMware

Use the complete matching VMware starter ZIP from the M7.5 release. It includes the installer and a **new empty** growable 20-GiB SATA disk. It is not preinstalled. Cross-release migration of old application packages is not qualified: use the fresh starter for this release.

## Before you start

1. Extract the entire ZIP into one local folder. Open **EngineOS.vmx** in VMware Workstation.
2. If VMware asks, choose **I Copied It**. Start the VM. The prepared configuration uses UEFI with Secure Boot off.
3. For a custom VM, keep its hard disk connected as SATA. This installer supports readable SATA disks with 512-byte logical sectors, from 2 GiB through 2 TiB. Other hardware is not universally supported.

The native installer needs no terminal commands, network or online account. The local account name is a profile name; it does not create an authenticated multi-user security boundary.

## Follow the seven screens

1. **Welcome:** choose **Next**. Recovery and opening an existing system are separate secondary actions.
2. **Choose a disk:** click the disk's **entire row inside the bordered list**. The row is highlighted and its radio mark fills. Capacity belongs to the clickable row too. Next stays unavailable until a usable choice has been made. An empty disk is prepared automatically. A disk containing files requires **Erase this disk and install**, followed by a separate final permission at Review. Back up those files first. OS installation uses the whole selected disk; this is not dual boot.
3. **Your apps:** keep or remove the getting-started guide. Desktop and Files are included. Optional apps can be removed and reinstalled later in Settings.
4. **Your style:** click the account-name box to edit it. Use letters, numbers, a dash or underscore. Backspace removes a letter. Choose Mint, Lavender or Amber. Back retains choices.
5. **Review:** check the disk number, capacity, account, guide and colour. If existing data will be erased, explicitly agree. **Install Engine OS** begins writes; entering Review does not.
6. **Installing:** keep the computer on and the disk connected. The displayed stages and progress come from actual copying and verification. A failed operation displays a failure, not completion.
7. **Ready:** choose **Restart to Engine OS**. This performs an actual restart into the installed hard-disk system. The matching M7.5 installer ISO can stay attached. **Open desktop now** is a secondary shortcut into the current installation session; it is not the independent-startup test.

Mouse, Tab, Shift+Tab and Enter are supported. Next keeps the window frame in place. Focus movement changes only focus borders. Use Back before installation to change your decisions.

## After restart

Engine OS opens its real persistent Files desktop. Open Documents, create a folder and use an app. The account and colour should match your choices. To verify the disk works alone, power off the VM, disconnect **only CD/DVD**, and start again. Keep **Hard Disk (SATA)** connected. Do not delete its VMDK file or create a replacement disk.

The matching ISO detects a single matching Engine OS installation and runs its actual installed loader/bootstrap. It does not silently reopen the live kernel. With no or multiple matching installations it stays in setup. Earlier releases do not acquire this convenience retroactively. Press Escape during early matching-media startup to bypass redirection for recovery/reinstallation; firmware may intercept Escape, so repeat it while the installation media starts.

## If something goes wrong

| What you see | What to do |
|---|---|
| No disk found | Power off. Add/connect a SATA hard disk in VMware and enable connection at power on. Restart. |
| Next is unavailable | Select a row; read the explanation below the list. Unsupported/small/unreadable disks cannot be installed. |
| Disk already has files | Back them up. Choose erase only if you want to replace that entire disk. Disk tools offers individual data-partition operations; it does not turn OS installation into a partition-preserving operation. |
| Installation fails | Keep the ISO connected. Use Back to return to disk choice with your personal choices retained, or open details/recovery. Do not assume the disk is untouched after writes started. |
| VMware reports No Media | Confirm the same SATA hard disk remains present and connected. Attach the matching ISO for recovery. This firmware message is not proof of an installed desktop. |
| Old installer appears again | Confirm the ISO/release matches this guide. Old media lacks M7.5 redirection. Disconnect only CD/DVD to test the installed disk. |
| Installed settings/volume fail validation | Keep the matching ISO and use its recovery tools. Do not erase user data merely to silence a boot error. |

Report the visible screen, release/build, firmware type, SATA configuration and whether CD/DVD is connected. Avoid sending private VM paths, identifiers or unrelated disk contents. [App guide and format limits](apps-m74.md), [partition tools](disk-manager-m71.md), [startup architecture and exact limits](../architecture/installer-m75.md).
