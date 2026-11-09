# VMware startup correction: Engine OS M9.1

M9.1 corrects the VMware starter configuration. The included **EngineOS-M9.iso is unchanged**. The original M9 ZIP enabled an e1000e network card without the required PCIe root-port configuration. VMware can report **Transport (VMDB) error -14** after failing to create that device. This was a packaging defect, not a partitioning task for the user.

## Start with the corrected package

1. Download **EngineOS-M91-VMware.zip** from the [correction release](../releases/m91-v0.9.1.md).
2. Extract the complete ZIP into a new folder. Open **EngineOS.vmx** and choose **I Copied It** if asked.
3. Start the VM. The package includes a new blank growable **20 GiB SATA disk**, 128 MiB RAM, two CPUs, enabled NAT Ethernet and sound, and the required PCIe bridge configuration. Keep Secure Boot off.
4. Select the disk row, follow Next, review and install. Choose **Restart to Engine OS**. The matching ISO can remain attached.
5. To check independent startup, power off, disconnect only CD/DVD and keep the SATA disk connected. Start again.

The installer and desktop controls are unchanged. [Full installation and graphics guide](install-m9.md). A fresh starter does not migrate data from an earlier VM.

## Why a 20-GB disk can still be unavailable

Capacity, partitions and the controller interface are separate properties.

| VMware disk connection | Current M9 behavior |
|---|---|
| SATA/AHCI, supported geometry | Installation and persistent data supported in the named VM profile |
| IDE | May appear for inspection; not an installation target in this release |
| Unqualified SCSI controller | May not appear because its controller driver is unsupported |

Formatting an IDE/SCSI disk does not add a controller driver. **Disk tools** cannot turn an inspection-only device into a writable installation target. For a custom VM, power it off and attach a supported SATA test disk; existing disk files need not be deleted. Prefer the corrected prepared package for the first test.

This correction does not add IDE/SCSI write drivers, broader GPU support or 3D acceleration. It fixes the startup configuration and closes the enabled-device test gap. The owner's existing VM configuration and host settings were not modified.
