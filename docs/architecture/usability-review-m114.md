# M11.4 native visual and interaction review

This is an agent walkthrough of the actual native interface, not a human study
with children or older adults. The exact acceptance and captures identify the
tested artifact. The broader installation rewrite is M11.5.

| Review question | Native walkthrough and correction |
|---|---|
| Is it clear? | Familiar title-bar minimize/maximize/close symbols, a focused-window accent strip, an explicit system menu and a visible Alt+Tab list distinguish actions and focus. Normal bounds survive snap/maximize/restore. |
| Is it convenient? | Every resize edge/corner works; pointer and keyboard move/size can be cancelled. Windows+arrows and Windows+E are ordinary shortcuts. A scaled or smaller display fits open controls back into the viewport, while deliberate partially off-screen movement stays available. |
| Is it user friendly? | Escape cancels the current interaction, Alt release chooses a switcher entry, narrow-display snap refusal explains why, and Alt+F4 preserves the existing unsaved-work review. Appearance previews before explicit Save. |
| Does it look coherent? | Original Inter coverage, 16-pixel body, 14-pixel secondary and 32-pixel Appearance heading use consistent surfaces, controls, spacing and original icons. Light/dark, accent and procedural wallpaper work without blur or a decoration loop. |
| Is it readable? | Actual ordinary, 150% and 3840x2160 physical captures accompany native input. Settings' final actions remain visible after scale changes. Selected labels retain contrast on light wallpaper. The VMware minimum workspace is captured separately. |

Two concrete pre-publication issues were corrected. Keyboard Move initially
lost its drag state on the mouse release that selected the menu command, so
Escape did not restore the bounds. The native cancel/commit journey now checks
that route. Visual review also found Settings partly outside the viewport after
150% scaling. Placement now fits controls to the new logical workspace, and an
explicit native bounds assertion supplements the screenshot review.

The supplied offline Welcome text now describes the actual M11 click, Files,
Start, window, appearance and provider limits. Its exact installed bytes are
checked independently from the disk. Optional guides/apps do not become shell
dependencies. Appearance saves normal sizes of open app types, not processes or
unsaved documents. Named VM results do not imply a physical hardware study,
unlimited windows, vendor GPU 3D or general Windows application compatibility.
