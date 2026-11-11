# M11 staged native tests

Keep all candidates and reports separate from accepted M10 output paths. Stage
order and publication gates are defined in [the milestone](../../docs/milestones/m11.md).

The accepted input-stage commands below describe its historical source revision
`e833630`. Reproduce that stage only in a separate checkout; its evidence and
artifacts are frozen. The current wrapper targets the stage-4 candidate.

```powershell
python tests/m11/interaction_host.py
python tools/build/m11.py --revision stage1
python tests/m11/selection.py
```

The host test executes the production classifier. The guest test installs on a
new owned sparse disk, exercises ordinary QMP mouse/keyboard events and inspects
state without modifying guest memory. Screenshots decode actual stopped-guest
framebuffer bytes. Timed double-click input does not use the diagnostic single
click helper, whose intentional settling delays exceed a human double click.
The persisted preference is independently read from validated GPT/native metadata
and rechecked after detached disk startup.

Use `ENGACER11_IMAGE` for a separately named later candidate. Never rebuild media
or maps while a harness is using them. Final qualification needs the separate
M11.6 controller, application, error and display matrix; this stage is not a claim
of complete hardware or Windows compatibility.

## Accepted M11.2 record

The commands below document source revision `ac94d20`. Reproduce only in a
separate checkout and preserve the accepted STAGE2 media/maps and evidence:

```powershell
python tests/m11/interaction_host.py
python tests/m11/volume_host.py
python tools/build/m11.py --revision stage2
python tests/m11/files.py
python tests/m11/file_preferences.py
python tests/m11/verify_files.py
```

The production storage DLL injects 288 normal/torn failures across all observed
subtree metadata I/O acknowledgement points, then remounts old/new banks. It also
checks readback/source CRC, streaming cancellation and invalid/colliding paths.
This is host evidence, distinct from native GUI operation. The native harness
installs a disposable disk, types and saves real payloads, uses Files operations
and ordinary pointer/keyboard input, independently checks persisted bytes and
boots without the ISO. `resources.json` records the installed desktop BSS delta
from exact linker maps; it is not configured RAM or total kernel memory.

The input classifier also checks identity changes during a click and between
clicks. Native listing order can change while entries are created: a recycled
row number alone must never activate a different file.

## M11.3 candidate

Freeze the current production source and build before native checks:

```powershell
python tests/m11/start_host.py
python tests/m11/interaction_host.py
python tools/build/m11.py --revision stage3
python tests/m11/start.py
python tests/m11/start_controls.py
python tests/m11/verify_start.py
```

The main journey installs an owned disk, edits categories/pins, validates real
catalogue launches/removal and independent preference bytes, observes an actual
installed reset and QEMU S5 process exit, then boots without the ISO. The controls
journey reuses only that matching owned fixture, verifies USB-HID event counters
and an explicit PS/2-only topology, reaches the final catalogue page and checks
window admission and pin order. Neither writes guest memory. Host validation is
separate from actual menu or power evidence. General ACPI/physical qualification
and the final enabled-device VMware/controller/original-app matrix remain M11.6.

## M11.4 windows and appearance delivery

The exact accepted candidate uses build `33e8ddd4cbc611aae509acb0d4325b4b`.
Preserve STAGE4 media/maps and evidence after publication; reproduce only in a
separate checkout. The commands used for this stage were:

```powershell
python tools/build/m11.py --revision stage4
python tests/m11/interaction_host.py
python tests/m11/windows.py
python tests/m11/window_keys.py
python tests/m11/window_motion.py
python tools/release/vmware_starter11.py --revision stage4
python tests/m11/vmware_windows.py --revision stage4
python tests/m11/verify_windows.py
```

The main journey independently checks the installed guide and appearance bytes,
real installation/reset, all window commands, scale fitting, dirty-close review
and actual 4K. The key and motion journeys reuse only that matching disposable
installed fixture, sequentially so no guests write the same disk concurrently.
Motion compares every viewport pixel after cached movement with a full repaint
forced through ordinary Start open/close input. Managed-page, idle and host/QMP
timings are recorded separately from FPS or total configured RAM. USB and PS2
shortcuts have distinct profiles. The exact archived VMware starter retains all
shipped devices, installs, restarts with its ISO and starts twice without it.
No test writes guest memory or controls the host desktop.

Publication additionally requires source-index/committed-blob digest checks,
the visible GitHub release, full English docs/homepage/download readback and the
separate public proof before M11.5 implementation begins.
