# Update and recover Engine OS 0.7.2

Boot the M7.2 ISO and choose **Recovery** at Welcome. Technical details open with **Repair**, **Update** and **Rollback** choices. Select your installation disk, choose Next, read the review and choose Confirm. None of these operations uses Erase and install or formats the user data volume.

| Action | What it changes |
|---|---|
| Update | Stages this ISO's verified boot bundle in the inactive system slot, then activates it |
| Rollback | Restores the previous verified system slot, when one exists |
| Repair | Replays the active verified slot's boot files |

After maintenance, power off the VM, disconnect **only CD/DVD**, keep the SATA disk connected and start it again. The immediate Open desktop action is for a fresh successful installation; maintenance of an older slot finishes with restart instructions.

M7.2 tests an actual M7.1 installation, native Update to M7.2, detached new desktop, then native Rollback to the exact M7.1 desktop using the firmware variables saved by M7.2. Independently CRC-checked user file contents remain unchanged. The firmware entry uses the stable fallback loader path shared by these slots; it must not point to a stale version-specific copied loader. Earlier M7.1 evidence separately qualifies damaged-BIOS-payload repair and torn-write behavior. This does not qualify arbitrary future format migration or rollback into a system lacking the compatible native desktop ABI.

Keep the ISO available. System slots and activation records are checksum-verified; they are not a cryptographic update signature. A power failure while replaying boot files can require ISO repair. There is no guarantee of automatic rollback after every interrupted firmware or storage write. A failed maintenance operation does not justify erasing your personal files.

If VMware shows No Media before Engine OS starts, first check the installed SATA disk is present and connected. For UEFI, the loader registers an Engine OS fallback boot entry and requires writable firmware variables. A multi-OS/ESP machine and physical installation are outside this release's qualified scope. [Installation and boot troubleshooting](install-m72.md), [original M7.1 recovery limits](recovery-m71.md), [release identity](../releases/m72-v0.7.2.md).

Updating keeps personal files, including your original Welcome.txt. Fresh installation includes the new folder-focused guide; use the online M7.2 manual for current instructions after an update instead of overwriting your existing personal copy.
