# M11 staged native tests

Keep all candidates and reports separate from accepted M10 output paths. Stage
order and publication gates are defined in [the milestone](../../docs/milestones/m11.md).

For the input stage, freeze source and build before running the native harness:

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
