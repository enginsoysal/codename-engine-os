# M7.3 exact-artifact qualification

The accepted native build is `8bf85110f7ac5a7ee48a824c3cc609dd` within M73-VM-v1.
[Acceptance](acceptance-results.json), [runtime task aggregate](runtime-results.json),
[native design review](visual-review.json), [release guide](../../docs/releases/m73-v0.7.3.md).

## Reproduction

Use a separate checkout of the release. Do not overwrite these frozen accepted
files or historical binary artifacts while developing a later follow-up.

```powershell
python tools/build/m73.py
python tools/release/vmware_starter73.py
python tests/m73/qualify.py
python tests/check_repository.py --milestone M7.3
python tests/m73/verify.py --commit 798d577428057c86e215583c1f9bf8d875ce6fef
```

The source font digest, Pillow 12.3.0, FreeType 2.14.3 and exact generated header
are locked. The design review is a recorded manual assessment of native captures,
not a test that silently regenerates a passing opinion. Qualification uses owned
sparse disks and headless VMs only, with no host desktop input or existing-disk
access. VMware tests use authenticated loopback guest RFB and retain the exact
same VMX/NVRAM for detached restarts. The starter test extracts the exact ZIP;
its only additions are owned test observation channels, absent from the public VM.

The native resolution tasks configure the owned OVMF preferred-resolution
variable, then verify actual framebuffer dimensions and pointer-driven tasks.
They never enlarge screenshots or change kernel framebuffer state from the host.
The progress fixture pauses only an owned VM at a completed native paint for
capture and resumes it to verified installation success. The host mode-selector
probe executes original C against bounded protocol fixtures; it is not GPU proof.

## Categories

- Real setup steps, missing-selection guidance, Back retention and verified
  installation to the immediate persistent desktop.
- Folder creation/navigation, independent metadata/CRC checks, readable guide
  bytes and font license, saved display sizes and smaller-display fallback.
- Actual 4K/200%, 2560×1440/150%, ordinary 100% and low-memory tasks; light/dark
  appearance, inappropriate-size refusal and detached disk startup.
- Real graphical partition operations, unchecked consent refusal with byte-for-byte
  preservation and confirmed FAT32 formatting that keeps unrelated partitions.
- Recovery/update/rollback of an accepted M7.2 workspace, original file bytes and
  the exact older desktop retained.
- SATA EIO recovery screens and working Details/Return/Back, concrete small-disk
  refusal and missing-SATA instructions.
- UEFI boot-registration and EDID/GOP host contracts, including negative cases.
- Exact VMware BIOS, UEFI and starter lifecycles; existing VM inventory unchanged.

Resource values distinguish allocator pages from immutable kernel/font bytes,
boot/firmware reservations and video storage. Navigation retains no additional
managed pages or allocation failures; input-idle repainting is zero and the two
CPUs halt. Full kernel and 4K framebuffer categories are in acceptance. These
checks do not establish physical hardware, Windows compatibility or novice
usability. [Design decisions and practical limits](../../docs/architecture/graphics-m73.md).
