# M10 — hardware phases 1 and 2 qualification

Engacer OS 0.10.0 is qualified within **M10-DEVICES-VM-v1**, on 10 November 2026.
[Acceptance](acceptance-results.json) binds 27 gates to exact published source
bytes, build ID, ISO and VMware starter hashes. [Release and limits](../../docs/releases/m10-v0.10.0.md),
[reproduction commands](../../tests/m10/README.md), [architecture](../../docs/architecture/hardware-m10.md).

## Evidence groups

- `filesystem-host`, `input-host`: actual production C, independent FAT inspection,
  bounded input/layout/HID behavior; these are host checks, not guest device proof.
- `journey`: five native graphical install/USB/input tasks with backing-byte checks
  and two disk-only boots, including real 4K, BIOS, 64 MiB and larger-cluster media.
- `usb-negative`, `usb-quota`: actual read-only/I/O/malformed media refusal,
  generation/reconnect recovery and eight-device admission/resource bounds.
- `native-motion`: ordinary/150%/4K clipped versus complete repaint, reachable
  captions, stable retained pages and idle halt without repainting.
- `vmware-results`: thirteen fixed-controller/firmware combinations plus the exact
  downloadable starter. Shipped devices stay enabled, existing user VM inventory
  stays unchanged, folders/profile bytes persist and the same disk boots twice
  without ISO. The starter includes real Devices/layout interaction and original
  7-Zip MSI/program/archive/extract/remove/reinstall with 106 payload hash checks.
- `motion-results`: actual visible native Files movement in an owned VMware guest,
  active SVGA FIFO and timing observations, not a monitor refresh-rate guarantee.
- `known-unqualified`: 64-MiB OVMF with boot-attached USB storage remains a failed
  init-allocation topology. Its `passed: false` is deliberate and never counted
  among successful gates. The ordinary downloadable starter uses 128 MiB.

All fixtures are owned disposable images/VMs, with no host disk exposed. Public
logs redact local user paths; raw VRAM and owned-work pointers are not published.
PyFatFS is test-only and is not shipped. Native 4K captures are actual framebuffer
bytes, not upscaled browser mockups. Agent design review is not a human study.
Physical/ARM, vendor GPU 3D, arbitrary HID, USB formatting/install, exFAT/NTFS writes,
general Windows apps and later network/audio/device phases remain outside this
acceptance. Historical release evidence and binaries remain unchanged.

Website/editor/English-article/download readback is a separate external check at
`evidence/documentation/homepage-m10`, not inferred from OS runtime acceptance.
