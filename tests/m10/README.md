# M10 qualification commands

Run from the canonical repository on the Windows build/test host. Tests create
owned disposable image files and headless guest VMs, never host block devices
or the user's existing VM. Native input goes to the guest, not the host desktop.
Use the pinned compiler/QEMU setup and installed VMware Workstation from the
existing build instructions. Test-only FAT inspection dependencies:

```powershell
python -m pip install --target tools/local/m10-python -r tests/m10/requirements.txt
python tools/build/m10.py
python tools/release/vmware_starter10.py
```

Freeze all build inputs, ISO, maps and VMware ZIP before qualification. A source
or artifact change invalidates results from the previous build. Never rebuild
into accepted M4.5–M9.2 outputs. Building alone does not qualify M10.

```powershell
python tests/m10/filesystem_host.py
python tests/m10/input_host.py
python tests/m10/journey.py
python tests/m10/journey.py --ram 64
python tests/m10/journey.py --firmware bios
python tests/m10/journey.py --resolution 3840x2160 --hub
python tests/m10/journey.py --large-usb
python tests/m10/usb_negative.py
python tests/m10/usb_quota.py
python tests/m10/known_boot.py
python tests/m10/motion_native.py --resolution 1280x800
python tests/m10/motion_native.py --resolution 2560x1440
python tests/m10/motion_native.py --resolution 3840x2160
python tests/m10/matrix.py --resume
python tests/m10/motion_vmware.py --image M10
```

Run VMware tests sequentially. Their unchanged-inventory guard must not observe
another test starting/stopping concurrently. QEMU tests can use separate owned
fixtures. For motion, `--disk` can reuse a completed journey's owned `owned.img`;
the helper validates its location. The local `owned-work.json` pointer must not
be published. A read-only inspection of actual native controls/VRAM informs
ordinary guest input; no guest application state is written by the harness.

The 64-MiB OVMF task connects USB storage after installed startup. `known_boot.py`
separately preserves the boot-attached USB/init-allocation failure as unqualified;
its `passed` value deliberately stays false. A changed result requires diagnosis
and qualification, not silently changing the expectation to a pass. The ordinary
VMware starter uses 128 MiB. Native 4K captures are actual framebuffer bytes.

Publish source and check all matching reports before acceptance:

```powershell
python tests/m10/verify.py
node --test website/brix/sync.test.mjs
```

The verifier rejects stale build IDs, changed artifact hashes, mismatched
published source bytes, failed native gates and an untested VMware ZIP. It
copies public evidence without local fixture pointers or raw VRAM. Release
publication, native BrixCanvas homepage Save/readback and automatic article
sync/readback remain separate external checks.
