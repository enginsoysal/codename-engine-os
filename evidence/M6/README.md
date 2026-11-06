# M6 qualification evidence

[Exact acceptance](acceptance-results.json) binds the final standard, update and Minimal artifacts to their source inputs and the declared native code commit. Earlier accepted M4.5/M5/M5.1 evidence is hash-checked and unchanged. Browser preview evidence is historical design evidence only.

## Reproduce

Build with `python tools/build/m6.py`, plus `--revision u1` and `--revision minimal --profile distros/engineos-minimal.json` for the two separate variants. Do not change sources while building or qualifying artifacts.

Run the actual native suites under `tests/m6/`: `layout.py`, `volume.py`, `guided.py` for all four BIOS/UEFI ISO/USB combinations, `maintenance.py`, `failures.py`, `resources.py`, `mouse.py` with USB and `--device ps2`, `terminal.py`, `distribution.py`, and `vmware.py` with EFI and BIOS. Run `python tests/check_repository.py --milestone M6`, then `python tests/m6/verify.py --source-commit 98516b4434a16dd0c914903b6b80cf8817e0783f`.

All destructive tests use disposable owned image files. The suites never target host disks or existing user VMs. QEMU fixtures use a local test directory outside OneDrive; VMware requires its files in this host's accessible project directory. New disk fixtures are genuinely sparse, checked with Win32 allocation accounting before guest startup. Workspaces have unique identifiers. Serial transcripts redact local paths and are normalized before evidence freezing.

## Reading the results

Reports identify source/media/build IDs and actual checks. Native screenshots show the real framebuffer wizard. VMware uses the actual headless product and COM1 keyboard navigation; this is not a QEMU substitution. Its installed boots detach the ISO. Resource counters distinguish managed allocation from static/firmware memory and actual CPU HLT from configured guest RAM.

`history/` preserves failed development or host/harness checks and their corrections. Host storage exhaustion prevented some guests from starting; genuinely sparse Win32 fixtures corrected this. VMware could not open AppData paths on this host; dedicated project fixtures corrected that test setup. Neither is recorded as a native success. No physical hardware, human usability study, desktop or Windows/DOS runtime claim follows from these reports.
