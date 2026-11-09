# M9.2 storage and window-boundary decisions

## Capability-based installation

Earlier installation, disk management and installed-volume paths explicitly required `STORAGE_AHCI`. Read-only discovery of IDE/NVMe/VirtIO could therefore never produce an installable target. M9.2 introduces common bounded transfer and flush callbacks and asks whether the actual discovered device is readable, writable and compatible with the installed layout. The UI does not ask an inexperienced user to select a bus as an installation technique.

The registry retains stable controller/unit identity, verifies a real first-sector read and publishes write capability only when both a transfer implementation and a flush implementation exist. Existing disk selection, consent, verified writes, GPT guards and copy-on-write volume transactions remain above that interface. A flush callback may not report success for a command that was never sent or failed.

IDE uses bounded ATA PIO reads/writes and cache flush. NVMe reuses its existing queues and interrupt waits for NVM writes and flush. Modern VirtIO Block negotiates write-cache flush and refuses write admission for a read-only device or missing flush support. Controller DMA buffers are fixed and page aligned; transfers are bounded to one 4-KiB buffer.

## SCSI is a command set, not one controller register map

SCSI inquiry, capacity, read/write and synchronize-cache tasks share a command layer. LSI MPI, VMware PVSCSI and BusLogic use different submission/completion protocols. Original Engine OS implementations match the appropriate PCI interface automatically. No user driver installer or imported upstream driver is added.

The initial implementations serialize requests, use bounded command deadlines, validate completion identity/range/status and use private DMA/sense buffers. A timed-out adapter is quarantined and bus mastering is disabled. Potentially referenced DMA pages remain owned rather than being reused while hardware may still reference them. MPI/PVSCSI use supported MSI/MSI-X completion interrupts with bounded halt waits; legacy PIO/mailbox paths retain bounded polling. Retained controller pages and CPU wait behavior must be measured; they are not inferred from a small guest-RAM setting.

Public protocol references used for field/register semantics: [LSI MPI interface definitions](https://github.com/torvalds/linux/tree/master/drivers/message/fusion/lsi), [VMware PVSCSI device specification](https://www.qemu.org/docs/master/specs/vmw_pvscsi-spec.html), [BusLogic interface definitions](https://github.com/torvalds/linux/blob/master/drivers/scsi/BusLogic.h), [VirtIO specification](https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html). These references are distinct from the project's original implementation and runtime proof.

## Partially off-screen windows

The old model constrained the complete window rectangle inside the desktop. M9.2 permits side and bottom overlap while retaining a useful title-bar grip. This avoids losing a window entirely and preserves taskbar access. Signed viewport clipping must be applied before pixel addressing; merely relaxing window coordinates would let unsigned origins wrap and corrupt drawing.

The existing cached movement path is used only where its source/destination pixels are valid. At boundaries, reuse is restricted to the client-pixel intersection visible at both the old and new positions. Newly revealed regions are composed exactly. Empty intersections and unsuitable fractional sampling phases fall back to exact clipped composition. Native comparisons against a complete repaint, moves out and back, ordinary/HiDPI captures, stable page ownership and idle halts are required. No cosmetic mockup can prove these properties.

## Controller discovery and verification limits

ATA software reset now respects the required assertion and post-reset settle intervals before probing both channels and units. The earlier four I/O-read delay alone was insufficient for secondary-channel discovery. Native disk-management tests cover separate IDE channels, and a mixed SATA keeper/LSI SAS installation target provides an additional identity-isolation case.

Completion interrupts are configured only when the adapter exposes a usable MSI/MSI-X capability. A bounded timer wake also checks completion/deadline state while the processor halts. An enabled interrupt flag does not prove that each command generated an MSI; reports retain observed interrupt and halt counters. VMware's legacy LSI Logic and BusLogic paths can use bounded polling. Idle desktop behavior is measured independently.

QEMU PVSCSI returns a successful 36-byte inquiry completion with zero reported transferred bytes in the tested implementation. Engine OS refuses this inconsistent completion; VMware PVSCSI has separate real installation and persistence qualification. The validation is not weakened to make the other emulator appear supported.

QEMU/OVMF completed native disk changes but returned to setup after retained-ISO restart in two tested LSI SAS multi-disk topologies: targets 0/1 on one adapter, and a separate SATA keeper plus SAS target 0. Root cause is not conclusively diagnosed; firmware visibility is a hypothesis. That topology remains an explicit failed/unqualified case. The qualified mixed-controller test and VMware single-target controller matrix do not erase this failure.

The first SCSI capacity implementation uses READ CAPACITY(10), LUN 0 and a bounded target scan; the extended-capacity sentinel is rejected. Installation remains limited to 512-byte logical sectors and the existing layout bounds. Hotplug, all physical HBAs and arbitrary RAID/USB bridge protocols remain separate work.

## Hardware interrupts while applications execute

The exact enabled-device starter regression exposed a real keyboard IRQ arriving during protected Windows application removal. The legacy input handler previously rejected every CPL3 interrupt frame, allowing a genuine device interrupt to fall into the old process-fault path. M9.2 services initialized hardware input independently of the interrupted privilege level, queues bounded input and returns to the same protected frame. User software still cannot invoke privileged interrupt gates or access device ports.

The regression injects neutral Shift key edges during real Windows workers and requires the native `M92_INPUT_IRQ_CPL3_SERVICED` observation, successful removal/reinstall and subsequent disk-only startup. Waiting until workers finish or disabling keyboard/network devices would hide the failure and is not an acceptable fix.

## Exact compiled and executed source bytes

Native qualification binds every declared input to both the working bytes and the published Git blobs. Six native sources were compiled with CRLF endings on Windows; explicit attributes preserve those exact bytes across checkouts. Qualification harness sources are preserved as executed as well. No post-build normalization, source-hash substitution or changed ISO is accepted. A future formatting conversion requires its own new build and qualification; it cannot silently change the accepted delivery.
