# M7.4 application boundaries and resource decisions

Status: development, not full milestone acceptance. The owner requested the
standard application set, attractive convenient UI, low resource consumption
and independent removal. [Contract](../milestones/m7.4.md),
[current guide](../handbook/apps-m74.md).

## Two explicit execution classes

`include/engine/desktop/catalog74.h` is the single fixed catalogue of stable
identities, execution class and readiness. Deferred entries remain hidden and
cannot be selected by an application profile. The build reads the same catalogue
instead of maintaining a different Python list.

Editor, Calculator, Calendar, Help and Archive Manager's actual Deflate worker
have separate original CPL3 ELF programs. Code loads on demand, same-app windows
share immutable code pages, and last close releases code/private pages. Complex
untrusted Deflate decoding runs behind the existing watchdog and supervisor
boundary. The decoder's extra 48 KiB of state exists only during extraction;
the broker's three scratch pages are similarly transient.

Clock, Downloads, Terminal, System Monitor, Software Manager, Disk Utility,
Update & Recovery, BMP viewing and Screenshot currently use fixed broker
frontends. Their optional package file is a small declarative manifest, not an
empty ELF loaded into a process. The broker accepts the exact built-in descriptor
bytes; it does not interpret arbitrary manifests as privileged executable code.
Opening one of these frontends therefore needs no otherwise unused process
mailbox/page tables. Its UI, file/device operations and service implementation
remain trusted static kernel code and must be counted in the static kernel span.
This extends the explicit M7 trusted UI/broker model; it is not a general native
third-party GUI ABI or a claim that all UI code is independently unloadable.

The rejected alternative was launching an event worker with no actual event
logic for every system frontend. That allocates around ten private pages plus
code per window without adding an isolation boundary to the actual broker work.
A future third-party GUI runtime should replace fixed frontend descriptors with
a real public IPC/rendering contract, not run arbitrary app modules in ring zero.

## Ownership and removal

CPL3 programs own `/Applications/NAME/program.elf`; fixed frontends own
`/Applications/NAME/manifest.json`. Corresponding verified offline sources live
under `/System/Packages`. Removal changes one app-owned file through the native
atomic metadata commit. It does not remove private AppData, shared documents,
offline sources, services, font/decoder notices or another package. Both the
broker and package operation reject active frontend/worker windows. Removing
Clock cancels its optional session state; the underlying time service remains.
Files and Settings are protected base providers. Settings retains management if
Software Manager is removed. Dependencies currently point only at protected
base services; there is no arbitrary package dependency solver or shell hook.

Schema-3 distro profiles select implemented applications. The standard profile
includes the currently implemented set. Omitted apps retain their offline source
and can be installed later. Unsupported identities/options fail before compiling.
Data-only tutorial files may target Documents, Downloads, Pictures or Music; system
paths and executable hooks remain prohibited.

## Storage and live-image bounds

BMP captures stream the existing physical framebuffer into a new file, avoiding
a second screen-sized allocation. The viewer reads bounded tiles. ZIP extraction
validates the directory/path set before creating a fresh output folder, checks
expanded size and CRC before each file commit, and does not overwrite existing
files. It can leave earlier completed members after a later failure; that limit
is explicit in the guide.

Desktop disk operations reuse the protected disk-service scratch buffers, never
the mounted volume's I/O context. The current system disk is inspection-only.
Desktop recovery restores verified boot bytes while retaining the protective
MBR/GPT portion of the initial 4 KiB; it does not reconstruct a default table.
The native regression test compares GPT bytes before and after repair.

The live installer contains an offline cache and installed-system payload. Its
old 4 MiB kernel-span ceiling is insufficient. M7.4 live BIOS media permits a
bounded 8 MiB span and uses a larger reserved USB boot area. The installed BIOS
partition/layout keeps its separate existing bound. Page tables protect the
actual number of kernel chunks rather than making extra code/data executable.
Raising a bound is not a claim that all of that memory is allocated or consumed;
measure exact live/installed static spans, allocator current/peak, decoder scratch,
framebuffer and firmware separately.

## Qualification boundaries

The calculator, inflate and browser host tests check algorithms and bounded
worker events separately from native acceptance. The first delivery passes
its complete-task, removal/omission/restart, resource, actual 4K and VMware
gates in the [exact report](../../evidence/M7.4/acceptance-results.json).
Richer app capabilities and desktop integrations remain explicit follow-ups.

## Audio ownership and interoperability

The original driver discovers PCI class 04/03/00 rather than a product-name
allowlist. It traverses analog pin connections to a stereo PCM DAC and checks
rate/depth capabilities. CORB/RIRB responses are acknowledged with response
interrupt status enabled locally, while the controller's global response IRQ
remains masked: disabling RIRB status generation can stop command progress
when its response threshold is reached. Discovery does not allocate audio DMA
until Play. Playback uses a fixed 64-KiB PCM ring plus small mapping, command,
descriptor and scratch pages. IOC interrupts request bounded refills; the
idle desktop does not poll the device. Pause retains that ring; Stop must
verify DMA quiescence before freeing it. An unsafe stop quarantines the pages
until restart, with an explicit error instead of freeing live DMA memory.
The compositor and storage streamer yield to the bounded audio pump during
long operations. This is one supported analog HDA/MSI output path, not a
promise of USB, HDMI, Bluetooth or all physical audio hardware.

The implementation follows the [Intel High Definition Audio specification](https://www.intel.com/content/dam/www/public/us/en/documents/product-specifications/high-definition-audio-specification.pdf).
QEMU's public controller/codec sources were inspected for interoperability
while diagnosing command progress. No QEMU implementation is copied or linked
into the MIT kernel. Native captured output remains a separate acceptance gate.

## Native text-input damage and bounded batching

A real 4K Calendar test exposed slow per-character full-window redraw. Calendar
now damages its note/caret region; its footer is damaged only when dirty/error
state changes. Queued printable/backspace edits in Calendar and Editor coalesce
in batches of at most 16 before painting. The first non-text event is carried
to the next ordinary event-loop iteration, so navigation and focus actions keep
their ordering and rebuild controls normally. There is no extra input buffer,
allocation or polling timer. The native 4K reopen/type/save probe checks this
behavior rather than hiding input lag with long test sleeps.

## Partial decoder output and the volume stream contract

The multi-file ZIP/audio task exposed a valid Deflate stream whose input boundary
produced a partial output page before the end of the file. The volume append API
requires each call to fit the remaining current 4-KiB page. The ZIP broker now
splits decoder output at that existing page boundary instead of assuming every
non-final output chunk is page aligned. This adds no buffer and does not change
the shared volume API. The original failed attempt safely retained its earlier
completed files and did not commit the failing member. Native extraction now
checks all three sample members, including the 192,044-byte WAV, against the
independent ZIP reader's bytes and CRCs.

## Audio service during compositor work

The multi-second WAV test exposed audio starvation while the CPU compositor
painted a window. Rectangle and rounded-surface loops now service pending HDA
interrupt work every sixteen physical rows. They do not start a polling timer
or allocate a second framebuffer. The audio reader uses its dedicated scratch
page, so it cannot corrupt a concurrent volume operation. Audio revision
changes also count as desktop work before the idle halt check: an end/error
inside a paint produces a prompt status repaint instead of waiting for the
next unrelated interrupt. Native PCM capture, uninterrupted reference waveform,
Pause/Resume and last-buffer page release are required together.
