# M8 first original Windows MSI profile evidence

The final `acceptance-results.json` is authoritative only when `passed` is true. Scope **M8-MSI1-VM-v1** qualifies the original 7-Zip 26.06 x64 console workflow, not general Windows Installer, Windows GUI or DOS compatibility. See [release limits](../../docs/releases/m8-v0.8.0.md) and [architecture](../../docs/architecture/windows-runtime-m8.md).

The native code source commit is `1bf41f0`; the final binder compares every compiled input with that Git tree and current files. Native framebuffer images come from guest video memory at its actual resolution. VMware screenshots come from the private RFB endpoint of disposable, headless test VMs. Neither route controls the owner's host desktop. Host MSI queries are read-only references and never execute or install the package on Windows.

## Reproduce in a separate checkout

Do not overwrite previously accepted outputs. Use pinned tools, disposable owned images and the matching artifacts:

```powershell
python tools/build/m8.py
python tools/release/vmware_starter8.py
python tests/m8/database.py
python tests/m8/rename.py
python tests/m8/journey.py --firmware uefi --ram 128 --resolution 3840x2160
python tests/m8/journey.py --firmware bios --ram 64
python tests/m8/vmware.py --firmware efi --starter
python tests/m8/vmware.py --firmware bios
```

Run VMware tests sequentially so inventory checks cannot mistake a second test VM for unrelated activity. Keep the newly created UEFI journey's owned disk for the following sequential commands; replace `OWNED_DISK_IMAGE` with that disposable image's absolute path:

```powershell
python tests/m8/safety.py --disk OWNED_DISK_IMAGE
python tests/m8/recovery.py --disk OWNED_DISK_IMAGE
python tests/m8/lifecycle.py --disk OWNED_DISK_IMAGE
python tests/check_repository.py --milestone M8
python tests/m8/verify.py --source-commit 1bf41f0
```

The binder copies successful matching development reports into this evidence directory, validates original payload identity and resources, checks all current artifact hashes, and rechecks frozen prior acceptance proofs/binaries. It fails on a missing/stale/failed gate. Existing-archive update remains a documented compatibility limitation; the native example frontend chooses fresh numbered outputs instead. AHCI can halt during I/O, so cancellation waits for namespace cleanup rather than treating a transient HLT state as completed cleanup.

Guest memory observations identify controls/state and capture video; actions use ordinary keyboard/mouse input. The recovery case deliberately stops its own emulator during cleanup, then uses the normal recovery UI after unforced media-detached boot. This is distinct from a successful whole-operation atomic transaction or a physical power-loss test.
