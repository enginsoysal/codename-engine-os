# M7.3 native graphics and usability

Status: candidate under qualification. A build is not an accepted release.

## Decision and implementation

The owner's five questions—clarity, convenience, friendliness, graphical quality
and readability—are product acceptance requirements. Ordinary setup, recovery,
disk confirmation and folder tasks must remain graphical. Human usability
studies are separate from the engineering review and have not been conducted.

The original 5×7 pixel font was appropriate for initial bring-up but unsuitable
for the product UI. M7.3 uses unchanged, openly licensed Inter outlines as its
font source. A pinned host rasterizer produces grayscale coverage at actual
physical pixel sizes. The native compositor blends those masks directly into
the framebuffer. It never enlarges a low-resolution font bitmap.

The reviewed sizes are 16/24/32/36/48/64 physical pixels; regular body text and
semibold headings form a small semantic type scale. Coverage is immutable,
run-length encoded and bounded to one MiB. Source font, rasterizer versions,
generated-header digest and full OFL license are pinned in `third_party/inter`.
The full license is included in the installer, installed System/Licenses,
release notices and VMware package.

A runtime TrueType interpreter and a general vector compositor were deferred:
they would add parsing, allocations and caching before the current UI needs
them. This implementation supports ASCII, not international shaping, arbitrary
font replacement or arbitrary fractional scale factors. Distribution profiles
still control supported content and appearance; theme changes cannot weaken
disk consent or system file protections.

## Coordinates, modes and scale

The physical boot contract remains unchanged. A UI-only view supplies logical
coordinates. Rectangles, glyph placement, cursor and input hit testing use the
same transform at 100%, 150% or 200%. Auto chooses 200% for a framebuffer at least
3840×2000 and 150% for at least 2560×1400; otherwise it uses 100%.

Settings offers Auto and the three explicit sizes. A choice leaving less than
1024×768 logical space is refused. Save preferences stores the explicit choice
in a separate checked native file; an unsuitable stored choice falls back to
Auto on a smaller display. Files, application state and unsaved text are kept
when the UI scale changes. Window bounds are constrained to the new workspace.

Before ExitBootServices the UEFI loader can select the validated active EDID
preferred mode when the matching RGB/BGR GOP mode exists within 3840×2160.
Without that protocol it retains firmware's selected framebuffer. This does
not implement a post-boot display driver, hotplug, multi-monitor or universal
GPU support. BIOS continues using its existing 1024×768 mode. A 4K screenshot
must come from a real 3840×2160 guest framebuffer, never image enlargement.

## Rendering and resource choices

The renderer clips writes, uses measured text fitting/wrapping and antialiased
rounded surfaces. The UI writes into the existing framebuffer, with no
per-window full-screen backing buffers. Cursor save storage is bounded to
32×48 pixels. Rounded shadow work skips covered interiors. Damage controls
desktop repainting; the event loop continues to halt when idle. Animation is
not added merely to make installation look busy: progress describes verified
native disk work.

Managed allocator usage must be reported separately from immutable kernel font
data, boot/firmware reservations and framebuffer storage. At 3840×2160, tightly
packed 32-bit visible pixels alone occupy 33,177,600 bytes. This is video
storage, not a claim that the entire OS uses that much managed RAM.

## Review and evidence

The candidate harness captures native setup pages, missing-selection guidance,
disk management, confirmation, recovery, Files and Settings. It performs actual
installation, normal guest input, folder creation, independent metadata/CRC
verification and detached disk startup. Qualification results belong under
`evidence/M7.3`; all previously accepted milestone proof remains immutable.

Before release, inspect every captured page at minimum and HiDPI sizes for the
five questions. Essential text must neither overlap nor disappear behind
controls. A passing scripted task does not establish that a first-time person
finds the flow convenient, nor qualify every physical screen or Raspberry Pi.
