# Host tools and entry points

## Layout and commands

- `tools/build/m2.py`: preserved M2 isolation regression build.
- `tools/build/m1.py`: explicit historical M1 build path.
- `tools/build/media.py`: restricted FAT32/GPT/EFI El Torito authoring, not a kernel filesystem.
- `tools/vm/config.py`: shared pinned QEMU setup.
- `tools/build/m3.py`: preserved M3 native scheduling/VM/SMP regression media.
- `tools/build/m4.py`: M4 AHCI/ext2/input media and version-2 filesystem template.
- `tools/vm/run.py`: M4 user launcher; select an earlier milestone explicitly.
- `tests/m0`, `tests/m1`, `tests/m2`, `tests/m3`, `tests/m4`: per-milestone verification.

```powershell
python tools/build/m4.py
python tools/vm/run.py --create-data-disk
python tests/m4/verify.py
```

Run from the repository root. The latest launcher defaults to two vCPUs and 128 MiB; `--cpus 1`, `2` or `4`, `--ram 64`, `--usb`, `--headless`, `--debug` and `--qemu-dir` are available. See [M2](../docs/milestones/m2.md) and the [M3](../docs/milestones/m3.md). No host disk or directory is exposed. Debugger ports bind loopback only.

## M4 storage and recovery

The marked working disk persists as `build/m4/manual/fs-data.img`. Copy both filesystem-template IMG/VMDK files together before attaching a writable VMware SATA disk. Version-1 transport-alpha images are rejected. `python tests/m4/verify.py` covers storage, filesystem faults/recovery, PS/2/USB console and owned VMware tests; `python tests/m3/verify.py` separately reruns the earlier complete native suite. `python tools/build/fsck.py` checks a volume; `--repair-copy` writes only a new exclusive recovery file. See [M4 instructions and limits](../docs/milestones/m4-contract.md).

## Pins and setup

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
