# Native windows and appearance — M11.4

Status: M11.4 native qualification passed on 11 November 2026, within
M11-STAGE4-VM-v1. [Exact acceptance](../../evidence/M11/4/acceptance-results.json),
[release](../releases/m114-v0.11.4.md), [visual review](usability-review-m114.md).
GitHub release and fresh public website readback are separate closing gates
before M11.5 starts.

## Decisions

The existing eight-window model owns focus, stacking and clipping. A generation-
scoped normal rectangle and mode extend each slot; maximize and left/right snap
do not allocate window-sized pixel buffers. Restore uses the saved normal bounds.
Drag previews are thin compositor borders. App-specific minimum sizes prevent
offering a half-screen placement that would hide required controls.

All edges and corners use the shared release/drag classifier. Escape or input
disconnect restores the pre-drag rectangle and mode. A caption dragged out of
maximized/snap state restores beneath the pointer; ordinary signed clipping and
the reachable-caption rules remain. The system menu exposes keyboard Move/Size,
Minimize, Maximize, Restore, Snap and Close. Normal Close preserves the existing
dirty-document review. Unsupported actions are disabled rather than simulated.

Alt+Tab snapshots at most eight generation-tagged MRU tokens. The visible list
needs no thumbnails or extra surfaces. Alt release commits, Shift reverses and
Escape cancels. USB HID and PS/2 encode shortcuts at arrival rather than guessing
modifier state after a potentially slow paint. Windows-key release opens Start
only if it did not participate in a chord. Win+arrows place windows and Win+E
opens Files. This is native shell behavior, not Windows API compatibility.

Appearance uses original procedural wallpaper, existing OFL Inter coverage masks,
light/dark native surfaces, accent and physical-resolution scaling. The per-user
`/System/Settings/appearance11-<user>.bin` record is versioned and bounded to
352 bytes, including twenty normal app rectangles. No photo decoder, per-window
framebuffer, runtime font interpreter, blur or idle animation is introduced.
Save preferences is explicit; failure leaves documents and preview state intact.
Automatic scaling chooses 200% at real 4K. A saved scale that cannot fit the
minimum logical workspace falls back to Auto for that session.

Changing scale fits open windows and their restore rectangles to the new usable
viewport, keeping Settings' actions visible. Loading saved normal geometry on a
smaller display also fits it. This differs deliberately from a user dragging a
window partly outside the screen, where the existing reachable-caption rule
continues to apply. Explicit Save captures normal sizes of currently open apps;
for multiple windows of an app the most recently focused instance wins. It does
not save processes, unsaved documents or a whole desktop session.

Secondary text uses genuine Inter coverage at 14, 21 and 28 physical pixels for
100%, 150% and 200%. The existing twelve faces remain before the three appended
regular faces. The original font/provenance is unchanged; the separate reviewed
M11 raster lock binds the generated header and tool versions. Selected desktop
labels retain a light foreground on their dark selection surface even when the
wallpaper is light. An accent strip identifies the focused window.

## Alternatives and resource reasons

| Decision | How and why | Alternative rejected |
|---|---|---|
| Share existing screen composition | Reuse clipped cached client movement, repaint exposed areas and captions; ordinary release does not repaint the whole screen again | Eight 4K window surfaces would add roughly 253 MiB before other memory |
| Use a bounded MRU switcher | Snapshot at most eight generation-tagged window tokens and draw a list | Live thumbnail surfaces, continuously refreshed animations and stale slot-only identities |
| Keep original normal bounds | Store mode separately; restoring and cancelling have explicit geometry | Overwriting the only normal rectangle during snapping or maximizing |
| Show a thin snap outline | Render compositor borders without another framebuffer | Blur, translucent copied screenshots or a decorative animation loop |
| Apply app minimum sizes | Refuse impossible half-screen layouts with an explanation | Clipping required buttons or pretending a snap succeeded |
| Rasterize original Inter at physical sizes | Bounded compressed masks, measured widths, no runtime font interpreter/cache | Enlarging a small bitmap, arbitrary adjacent font sizes or depending on host fonts |
| Store a 352-byte per-user record | Validate version, fields and geometry; preserve files on save failure | Unbounded theme files, silently persisting every pointer move or coupling appearance to an optional app |

Keyboard movement/sizing uses the same constrained placement as pointer input.
Enter commits; Escape and disconnect restore pre-drag mode and geometry. Mixed
primary/secondary chords cancel capture. Alt+Tab cancels a pending drag before
switching focus. Owner dialogs remain in their app's surface; the global Start
and switcher surfaces sit above file dialogs without routing clicks through them.

## Qualification

The exact compiled desktop object has 543,784 bytes of BSS and 28 bytes of data,
768 bytes above M11.3. The accumulated fixed shell-state increase from M11.1 is
423,468 bytes, below the 512-KiB budget. These are object/map measurements, not
configured guest RAM or total kernel consumption. The separate immutable font
atlas has 1,045 glyph records and 270,910 compressed coverage bytes. Native
managed-page, idle and motion observations are separate acceptance checks below.

Sixteen native tasks, twelve cached/full viewport comparisons and seven host
classifier groups pass on build `33e8ddd4cbc611aae509acb0d4325b4b`. The exact
VMware SATA/UEFI starter installs with every shipped device enabled, restarts
with its ISO attached and boots independently twice without it. USB/PS2
shortcuts, dirty-close review, saved-byte inspection, separate installed boots,
ordinary/150%/actual-4K captures, unchanged retained managed pages and halted idle
CPUs have separate records. Observed QEMU event-to-drain medians include slow
software emulation and QMP observation (about 1.19/2.41/3.34 seconds); they are
not display FPS or a claim of GPU acceleration. Full controller/application
regressions remain M11.6. Physical GPU acceleration, arbitrary fonts/photos and
unlimited windows are not implied.
