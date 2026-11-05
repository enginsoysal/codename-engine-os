# Build-host tools

The checked-in [toolchain lock](toolchain-lock.json) pins the observed compiler/linker executable hashes, QEMU executable and adjacent DLL hashes, EDK2 firmware/templates/notices, Python version and the selected machine profile. It contains no host usernames or absolute installation paths.

These are **observed local pins**, not an independently authenticated upstream build or proof of bit-identical reproduction on every host. The pre-existing QEMU install identifies itself as 11.1.0 with the full source revision in the lock. We do not replace it with a different stable binary silently. A tool change requires reviewing the lock and rerunning the M0 gates. Firmware notices are provided by `share/edk2-licenses.txt` in that installation; binaries are not redistributed in this repository.

## Python ISO tool

```powershell
python -m pip install --require-hashes --no-deps --no-compile --target tools/local/python -r tools/requirements.txt
```

This project-local directory is ignored by Git. The pinned pycdlib wheel is a build-host dependency under LGPL-2.1-only; it is not linked into the OS. Package and source: [PyPI](https://pypi.org/project/pycdlib/1.22.0/), [upstream repository](https://github.com/clalancette/pycdlib). Original wheel notices remain in its dist-info directory.

Why this instead of xorriso for the first Windows-host route: pure Python, small dependency and explicit EFI El Torito support, with no WSL requirement. This replaces the earlier open xorriso prerequisite, not the ISO boot requirement.

## M0 verification

Run from repository root, with Clang/LLD on PATH:

```powershell
python m0/probes/run_probes.py
python m0/probes/contracts.py
python m0/probes/windows_reference.py
python m0/probes/environment.py --qemu-dir "C:/Program Files/qemu"
```

The last command checks the lock before starting QEMU, uses `pc-q35-11.1`, TCG, 512 MiB and two vCPUs, read-only EDK2 code plus private variable copies, no network and hidden/no-window process creation. It tests an optical ISO and a GPT/FAT32 USB-storage image. Each run must see a serial marker and the expected emulator exit code; a successful QEMU process launch alone is insufficient.

**The generated images are M0 environment fixtures, not Engine OS installation media.** They load a tiny EFI test and immediately exit. M1 will implement the kernel handoff/ExitBootServices, and M5 the installation workflow.

Only files under `build/` and generated evidence are touched. There is no host block-device passthrough or shared host directory. For another toolchain installation, first review/adjust paths and pins, then rerun; never disable lock comparison merely to obtain a passing result.
