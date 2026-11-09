# Engine OS M9.2 installation and hardware guide

M9.2 qualification is in progress. Use the release only after its exact-artifact acceptance report is published. The broader hardware program remains open.

## Start in VMware

1. Extract the complete **EngineOS-M92-VMware.zip** into a new folder. Open **EngineOS.vmx** and choose **I Copied It** if VMware asks.
2. Start the VM. The prepared package supplies a new blank growable 20-GiB SATA disk, 128 MiB RAM, two CPUs, NAT Ethernet, sound and the required PCIe root ports. Keep Secure Boot off for this unsigned development build.
3. Choose Next. Click the whole **Disk 1** row in the bordered disk list; the highlight and selection circle confirm your choice. Supported controllers bind automatically.
4. For an existing test disk, open **Disk tools** to inspect partitions or prepare it. Review the selected disk identity and explicit erase confirmation. Installation replaces the selected whole disk; it is not a dual-boot installer.
5. Choose applications and appearance, review the summary, then Install Engine OS. Wait for successful completion and choose **Restart to Engine OS**.
6. The matching ISO can remain connected. The installed desktop opens Files on the persistent native volume. Create a folder in Documents and restart to check persistence.
7. To test independent disk startup, power off and disconnect only CD/DVD. Keep the hard disk connected, then start again. No terminal command or forced boot order is required for a qualified profile.

## Controllers and geometry

IDE, SATA and SCSI are different hardware interfaces. Engine OS supplies its own controller implementations. The interface name is not an installer requirement: an eligible disk needs successful native transfer, supported geometry and working write/flush operations. SCSI controller models have different protocols; support for one does not imply all SCSI hardware.

The qualification matrix is maintained in [M9.2](../milestones/m9.2.md). Installation uses 512-byte logical sectors and a whole target from 2 GiB through the documented 2-TiB limit. A discovered 4-KiB-sector disk is not an installable target. Additional partitions are not automatically mounted in Files. VMware BusLogic requires a compatible device preset and BIOS; a hypervisor refusal before the guest runs cannot be repaired by a guest driver.

An empty list indicates no usable device was discovered. An inspection-only row means the disk cannot safely be used for installation. Formatting does not repair a missing controller implementation. Open Details for the actual condition; do not erase unrelated disks or disable safeguards.

## Windows and desktop movement

Move a Files window by its title bar. It can overlap the left, right and bottom edges, while a useful title-bar grip remains visible. Drag the visible caption back onto the desktop. Multiple-monitor movement and dragging file icons between screens are separate features.

The display remains bounded software 2D composition with supported VMware SVGA presentation. Device recognition is not proof of 3D acceleration or support for every monitor, GPU, network card or physical computer. See the [hardware roadmap](../milestones/m9.md).

Open Start > Windows Apps for the existing original 7-Zip console profile. General Windows GUI/MSI and DOS compatibility are not implied by storage support. [Windows application tutorial](windows-apps-m8.md).
