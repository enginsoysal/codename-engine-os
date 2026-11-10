# M10 hardware and removable-media architecture

Engacer OS supplies built-in support for qualified device interfaces. This is
the phase 1/2 implementation, not universal hardware or vendor GPU acceleration.
The [milestone](../milestones/m10.md) separates required runtime evidence from
implementation. Existing release artifacts keep their historical Engine OS name.

## Discovery and capability ownership

The hardware inventory holds at most 128 PCI functions. Initial enumeration
records class and identity; explicit refresh and USB change events observe
the active providers without allocating or periodically rescanning. Existing
native storage, graphics, Ethernet and USB implementations bind automatically.
Their actual status is authoritative: a PCI class or VRAM size grants no I/O
capability. Settings → Devices distinguishes Ready, Limited, Detected and
Unavailable. Firmware/software 2D and VMware 2D presentation do not imply 3D.
Audio discovered on PCI is not represented as verified active playback.

We reuse the existing providers rather than creating an unrestricted driver
plugin ABI. It avoids duplicate resource ownership and preserves completion,
deadline and quarantine checks. A general dynamic driver service, comprehensive
power management and physical-device qualification remain later work.

## Input

The production layout module maps both supported PS/2 and USB HID keys to US
QWERTY or US Dvorak. Shift and Caps Lock interact only for letters; control
shortcuts follow logical letters. Settings saves a small validated choice in
`/System/Settings/keyboard10.bin`. Unsupported layout values are refused.
This delivery does not include international IMEs, every national keyboard,
arbitrary HID report layouts or assistive input devices.

USB wheel report protocol is enabled only when the bounded report descriptor
proves the buttons/X/Y/relative-wheel byte layout. Report IDs and unfamiliar
layouts retain boot-protocol pointing. PS/2 IntelliMouse negotiation and VMware
absolute-pointer wheel input feed the same bounded accumulator. It retains at
most 32 steps and queues a wake only on an empty-to-nonempty transition.
Scrolling applies to the focused Files window. It is not hover scrolling.

## Removable device lifetime

The existing single-controller xHCI implementation bounds devices at eight,
handles supported hubs and receives port/endpoint changes through interrupts.
USB storage handles include a device generation. Disconnect invalidates the
registry entry; a new device can reuse that unavailable USB slot, but an old
filesystem lease cannot access it. Failed endpoint shutdown retains potentially
referenced DMA pages rather than freeing memory still owned by hardware.

BOT storage advertises writes only after a compatible 512-byte device reports
no MODE SENSE write protection and completes SYNCHRONIZE CACHE. READ/WRITE(10)
and extended-LBA commands transfer at most 4 KiB at once. UAS and other unsupported
write profiles remain read only. Readability alone never becomes write admission.

Safe remove flushes writable media, disables the device tree and invalidates
its registry identity. A failure is reported as a failure, not safe removal.
Re-enumeration is suppressed until an actual detach/reconnect. OS installation
and partition formatting on USB are deliberately refused in this phase; they
need separate boot-layout and destructive-operation qualification.

## Files and FAT32 writes

Files offers USB drives, folder navigation, copying a selected file to Downloads,
To USB on native files, an explicit destination and Safe remove. Existing names
are never overwritten. Import retains the original name when available and
chooses a numbered alternative on collision. The first supported FAT32 volume
is exposed: raw superfloppy or a supported partition among the first 16 entries.
This is not an arbitrary mount manager or exFAT/NTFS writer.

The broker reuses the strict FAT32 reader and directory validation. Writes
require 512-byte sectors, two matching mirrored FATs, valid FSInfo records and a
clean/error-free volume. File data streams through a 4-KiB buffer; files are
bounded at 64 MiB. Export names are printable ASCII up to 63 bytes. The graphical
import currently asks for names no longer than 55 bytes, leaving room for a
collision suffix in the native path component.

Exports create new files only. The writer validates and reserves bounded free
directory entries and a contiguous cluster run, invalidates advisory free-space
hints, marks the volume dirty, writes mirrored allocation/data, flushes, writes
long-name entries, flushes and publishes the short entry last. A final flush and
clean-bit update complete the operation. Deleted directory holes must not insert
an end marker that hides later files. Short aliases are checked independently
from long names. Allocation scans at most 1,048,576 clusters; directory free-slot
search visits at most 128 clusters. No directory growth, overwrite, deletion,
defragmentation or filesystem repair is supplied by this broker.

These limits avoid a full-file buffer and an unbounded allocator. A failed write
can leave allocated space or metadata requiring an offline FAT check; this is
not journaling or a power-loss atomicity guarantee. Subsequent writes to dirty
or inconsistent metadata are refused. Copies currently run synchronously after
painting a clear keep-connected message; cancellation during copying is not
implemented. Native-volume imports use the existing CRC-protected copy-on-write
transaction. No unrelated disk is a transfer target.

## Low-memory UEFI startup

A 64-MiB firmware map exposed a real overlap: the temporary whole-kernel file
allocation occupied part of the fixed physical kernel range. Claiming it later
could not succeed. M10 validates a bounded ELF/program-header prefix, reserves
the actual fixed kernel range first and reads each validated segment directly
into those pages in chunks up to 4 KiB. It needs no contiguous second copy of
the complete ELF. Header accesses, segment ranges, short reads and file seeks
are checked before handoff. BSS is explicitly cleared.

The first 4-KiB prefix is loader-owned and remains outside the allocated kernel
range. The normal init bundle is loaded only after the kernel reservation. This
reduces transient boot memory rather than raising the guest RAM requirement.
M10 public media remains unsigned; its streaming path explicitly refuses the
older whole-buffer owner-signing build mode until a streaming payload verifier
is implemented and independently qualified. Historical release loaders retain
their original whole-file path and frozen evidence.

The 64-MiB OVMF profile is qualified with USB storage connected after installed
startup. A separate boot-attached USB-storage run remains unqualified: that
firmware topology leaves no suitable contiguous allocation for the approximately
2-MiB original MSI init bundle and is refused before installation. It is retained
as a failed case, not counted as a pass. The prepared VMware profile uses 128 MiB.

## Verification boundaries

Host checks execute the real C registry, FAT broker and native volume using
disposable regular files, then inspect bytes with a separate FAT implementation.
They cover collision/no-write, dirty/mirror refusal, injected write failure,
deleted-hole preservation, GPT offsets and reconnect generations. Test-only
PyFatFS is not shipped with the operating system. Input checks execute the
production mapper, accumulator and descriptor parser.

Native guest tests separately exercise graphical installation, actual installed
startup, Settings, file transfer, flush/removal/reconnect and independent backing
image inspection. VMware must retain all shipped devices, ordinary NVRAM and
independent optical-detached boots. Actual 4K framebuffer, clipped composition,
retained pages and halted idle CPUs are separate gates. A host pass or screenshot
alone cannot qualify the release. See the final acceptance report once published.
