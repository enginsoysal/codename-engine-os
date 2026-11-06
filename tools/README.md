# Host tools and entry points

## Layout and commands

- `tools/build/m2.py`: preserved M2 isolation regression build.
- `tools/build/m1.py`: explicit historical M1 build path.
- `tools/build/media.py`: restricted FAT32/GPT/EFI El Torito authoring, not a kernel filesystem.
- `tools/vm/config.py`: shared pinned QEMU setup.
- `tools/build/m3.py`: preserved M3 native scheduling/VM/SMP regression media.
- `tools/build/m4.py`: M4 AHCI/ext2/input media and version-2 filesystem template.
- `tools/vm/run.py`: M5 user launcher; select earlier milestones explicitly.
- `tests/m0` through `tests/m5`: per-milestone verification.

```powershell
python tools/build/m5.py
python tools/vm/run.py --create-install-disk
python tests/m5/verify.py
```

Run from the repository root. The latest launcher defaults to two vCPUs and 128 MiB; `--cpus 1`, `2` or `4`, `--ram 64`, `--usb`, `--headless`, `--debug` and `--qemu-dir` are available. See [M2](../docs/milestones/m2.md) and the [M3](../docs/milestones/m3.md). No host disk or directory is exposed. Debugger ports bind loopback only.

## M4 storage and recovery

The marked working disk persists as `build/m4/manual/fs-data.img`. Copy both filesystem-template IMG/VMDK files together before attaching a writable VMware SATA disk. Version-1 transport-alpha images are rejected. `python tests/m4/verify.py` covers storage, filesystem faults/recovery, PS/2/USB console and owned VMware tests; `python tests/m3/verify.py` separately reruns the earlier complete native suite. `python tools/build/fsck.py` checks a volume; `--repair-copy` writes only a new exclusive recovery file. See [M4 instructions and limits](../docs/milestones/m4-contract.md).

## Pins and setup

M4.5 VM-scope completion: `python tools/build/m45.py`, `python tools/vm/run.py --milestone M45 --data-disk build/m45/example.img`, `python tests/m45/verify.py`. The M45 launcher uses a temporary overlay for ordinary disk images. [Storage/trust scope](../docs/milestones/m4.5-storage.md), [security test tool pins](security-toolchain.json). The normal public ISO is unsigned; owner-key Secure Boot tests generate separate local-only media and never modify host firmware/certificate stores.

[toolchain-lock.json](toolchain-lock.json) records the observed compiler/linker hashes, QEMU executable/adjacent DLLs, EDK2 firmware/template/notices, Python version and original reference profile. These local pins are not independently reproduced upstream-authenticity proof. Do not silently replace a different installed version; review pins and rerun gates. The existing Swift/LLVM and QEMU installations are not globally modified.

```powershell
python -m pip install --require-hashes --no-deps --no-compile --target tools/local/python -r tools/requirements.txt
```

The ignored local directory contains pycdlib 1.22.0 with a pinned wheel hash. It is a host-only LGPL-2.1-only ISO dependency, not linked into the OS; notices remain in its dist-info directory. [Upstream](https://github.com/clalancette/pycdlib), [package](https://pypi.org/project/pycdlib/1.22.0/). This removes a WSL/xorriso prerequisite on the current Windows host.

Clang/LLD must be on PATH with the recorded hashes. The QEMU default directory is `C:/Program Files/qemu`; its code/template/notices come from `share/`. QEMU/OVMF binaries are not redistributed by this repository. Screenshot conversion uses the host's Pillow 12.3.0 (reported test tool, not an OS dependency).

## Qualification

```powershell
python tests/m0/verify.py
python tests/m1/verify.py
python tests/m3/verify.py
```

Full host-reference qualification currently requires Windows. M0 loads only an EFI environment fixture; M1 loads the own kernel; M2 exercises real native ring-3 isolation. M3 qualifies threads/VM/SMP in its bounded native profile, including 1/2/4 genuinely active CPUs. Historical M1 reports are bound to the release tag. Rebuilding/testing that path may refresh local reports with a new current-source build identity.

Optional VMware qualification: `python tests/m3/vmware.py --cpus 2` creates and stops only a separate diskless/networkless EFI test VM in the ignored project build directory; existing VMs are inventoried and left untouched. See [profiles/evidence](../evidence/M3/vmware/README.md). VMware is not a prerequisite for the portable QEMU acceptance suite.

Generated files stay under ignored `build/` and `dist/`. Public evidence redacts host user/project/temp paths. Debug paths are remapped and release artifacts scanned before publication. ISO timestamps may change image hashes; each release's manifest identifies its exact files. Never infer kernel consumption from configured guest RAM.

The old planning copy is privately archived under ignored `tools/local/legacy-planning`; it is not current documentation and must not be published or returned to BrixCanvas.

## M4.5 trust and hardware qualification

`python tools/build/m45.py` builds the normal unsigned BIOS/UEFI milestone media. `python tests/m45/verify.py` runs the complete implemented-profile matrix, including 23 owner-trust checks and current M3/M0 regressions. `python tools/build/secure.py` defaults to a disposable VM identity; persistent owner PFX/password/pin use and its private-staging boundary are documented in the [owner workflow](../docs/milestones/m4.5-owner-trust.md). Signing/enrollment/negative-test media stay under ignored output and never enter public release assets. Factory trust, production key operations and physical qualification remain separate unqualified follow-ups, outside the user-approved M45-VM-v1 completion contract.

## M5 live/install/recovery

`python tools/build/m5.py` builds the live ISO/USB and embedded installed system. `python tools/vm/run.py --create-install-disk` attaches a new/reused disposable 256-MiB image under ignored build output. The guest still requires `plan N` and exact confirmation. After success, `python tools/vm/run.py --create-install-disk --installed` boots that disk with no installation media. An explicitly supplied `--data-disk` is writable in M5; use disposable image files only. M45 retains its snapshot behavior.

`python tests/m5/verify.py` builds and runs the exact native VM acceptance matrix. `--no-build` runs gates against the current build; `--collect` refuses missing/stale/failed reports and checks every source/artifact hash. It includes native failure/recovery and low-RAM tests, headless owned VMware EFI/BIOS installation through COM1, and repository documentation checks. Existing VMs are inventoried and left untouched. These tests use regular generated image files, never physical disks. [M5 contract](../docs/milestones/m5.md), [installation](../docs/handbook/install.md), [recovery](../docs/handbook/recovery.md).

## M5.1 guided distribution builds

`python tools/build/m51.py` builds the standard profile and public `EngineOS-M51.iso`. The launcher now defaults to M51. `python tools/build/m51.py --profile distros/forge.json` creates the independent Forge outputs under `dist/profiles/forge/`; `--validate-only` checks a recipe before compilation.

Full qualification sequence: build Forge, run `python tests/m51/guided.py --custom`, build the standard profile again, then run `python tests/m51/verify.py`. `--collect` checks existing successful reports and current source/artifact identity; it does not rerun runtime tests. M5/M4.5 evidence is not overwritten. See [distribution tutorial](../docs/handbook/distributions.md) and [guided installation](../docs/handbook/install-m51.md).
