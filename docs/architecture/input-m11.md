# M11.1 native interaction implementation

M11.1 is complete within its native interaction stage scope, with
[fourteen actual guest tasks and matching-source acceptance](../../evidence/M11/1/acceptance-results.json).
The M10 download remains qualified; this does not qualify the final M11 release.

## Shared pointer classifier

`include/engine/desktop/interaction11.h` classifies press, release, target identity,
movement and monotonic elapsed milliseconds. The installer and desktop execute
this same production code. Files/desktop items select on first release and open
on a matching second release; buttons activate on their first matching release.
Releasing outside, changing targets, crossing the drag threshold or cancelling
cannot issue the original button command. Secondary release produces a context
action rather than a primary activation. Window tokens include their generation.

The monotonic HPET clock supplies 100-Hz ticks. The device producer converts
them to milliseconds and queues the arrival timestamp and modifiers with each
button edge. Repaint time must not stretch a physical double-click interval. Default
500-ms double-click timing can be changed through Settings to 200/500/900 ms.
The bounded setting is validated and saved separately from optional applications.
The first click is not delayed to wait for a second click.

## Selection, focus and capture

Native Files uses 1024 selection bits per window, matching the existing native
entry bound. Eight windows retain independent selections and anchors. Ctrl toggles,
Shift extends, arrows navigate and Enter opens. Desktop icons use a three-bit
selection and independent keyboard focus; selecting an icon does not launch it.
Dragging background produces a visible marquee. Title movement begins only after
four logical pixels, retaining the existing clipped/cached compositor path.
Navigation clears stale item selection and click history.

The initial context menu supplies Open, selection information and Close; later
M11.2 replaces it with actual file operations. Right-click preserves an existing
multiple selection. Escape consumes menu/capture cancellation. Shift+F10, arrows,
Tab/Shift+Tab and Enter operate the menu without sending the same input to the
underlying file view. Native input providers publish Ctrl/Shift/Alt state.

USB folders use selection before double-open. Existing FAT32 transfer capability
bounds remain explicit: select one supported file for Copy to Downloads, then
open it there. A multi-selection is not silently reduced to one transfer. Full
provider operations remain part of M11.2 and the M10 USB safety limits remain.

## Bounded device edges

The xHCI/shared pointer producer keeps a fixed 16-slot ring (15 admitted edges),
coalescing motion separately while retaining both button identities. Overflow
cancels capture and suppresses the held button until release, rather than
inventing an activation. Device disconnect clears input state and wakes the
consumer with a cancellation. The queue is drained before the idle halt; there
is no new idle polling loop, per-window image buffer or unbounded allocation.

This preserves press/release meaning during a slow repaint. A single saved
left-button flag was sufficient for the previous press-dispatch UI but could not
represent right clicks or multiple release-driven operations. Fixed selection
bits and streamed input avoid general heap structures in the trusted shell.

## Verification

- `python tests/m11/interaction_host.py` executes the production classifier:
  selection/double-open, changed targets, timing, secondary release, drag,
  release-outside and cancellation. Host checks do not prove native integration.
- `python tools/build/m11.py --revision stage1` creates isolated M11 candidates.
- `python tests/m11/selection.py` installs on an owned disposable disk, restarts,
  injects real pointer/keyboard input, reads the native state and captures actual
  framebuffer bytes. The double-click gesture is timed separately from the
  deliberately slow diagnostic single-click helper.

Stage acceptance is recorded under `evidence/M11/1`. Public website verification
is a separate required gate before M11.2. M11.6 separately freezes and qualifies
the final ISO/ZIP, device/controller matrix, application regression and visuals.
