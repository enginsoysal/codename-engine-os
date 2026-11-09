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
