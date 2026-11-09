# M9 graphics and input decisions

The first M9 delivery targets VMware window movement before expanding foreign application compatibility. The kernel remains Engine OS's original kernel. No VMware Tools or imported Linux driver is installed.

## One admitted RAM framebuffer

The CPU compositor previously blended text and rounded surfaces directly in firmware scanout memory. Video-memory reads and repeated font work are costly. M9 composes into one screen-sized RAM allocation when contiguous conventional pages are available. Allocation is capped at 32 MiB and requires more than 24 MiB of free managed pages to remain. Failure keeps the direct framebuffer path. A packed 1024×768 buffer needs 3 MiB; packed 3840×2160 needs about 31.6 MiB. These pages remain allocated for the session and count as retained memory.

This trades a bounded amount of RAM for cheaper drawing. Per-window backing buffers would grow with window count and are deliberately avoided. Contiguous allocation reuses the existing ownership bitmap, admits the whole request atomically, zeroes pages and refuses fragmented runs. Host allocator tests supplement native guest tests; they cannot qualify a GPU.

## Correct, batched movement

Only an unchanged, topmost window with no other pending damage can reuse its client pixels. Direction-aware row copies handle overlapping moves. Title, footer, borders, shadows and newly exposed background are recomposed. Resizing, changed content and unsuitable fractional scaling use the existing exact repaint path.

All subregions of a dragged frame are composed in RAM before one combined presentation. Publishing each region separately exposed partially updated frames during development. The cursor is a separate scanout overlay: restoring it reads the authoritative RAM image, and cursor pixels never become part of the copied client. Full-repaint pixel comparison checks the final result independently of the cached movement path.

## Automatic VMware binding

The SVGA II backend matches PCI `15ad:0405`, validates the existing firmware framebuffer and FIFO geometry, then sends explicit bounded 2D update commands. It retains the firmware display mode. A malformed FIFO or bounded-wait timeout quarantines this path and keeps CPU framebuffer presentation. This is CPU composition plus explicit SVGA presentation, not 3D acceleration. Normal batched dragging does not depend on the optional legacy rectangle-copy command.

The absolute-pointer backend first verifies VMware's CPUID signature and protocol response. It maps absolute coordinates to the actual physical framebuffer, then uses the existing scale and button-edge input path. It falls back to PS/2 after an invalid response; USB input remains available. Wheel integration is not added by this delivery.

## Verification and limits

Use owned headless VM fixtures, real installed restart, guest pointer input, independent raw-framebuffer captures and a complete-repaint comparison. Record actual resolution, build identity, retained buffer bytes, idle halts and measured event-to-composition latency. Loopback remote-display capture adds host overhead and is not a monitor refresh-rate measurement. Automated scenario checks are not a human usability study.

Runtime display-mode changes, SVGA3/MMIO, a 3D command stack, other physical GPU families and universal hardware qualification remain follow-ups. See the [M9 delivery contract](../milestones/m9.md).
