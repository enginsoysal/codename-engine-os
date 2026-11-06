# M5.1 task and distribution evidence

[Acceptance](acceptance-results.json) binds the actual standard ISO/source to its successful task reports. [Build manifest](build-manifest.json), [scope](../../docs/milestones/m5.1.md), [installation](../../docs/handbook/install-m51.md), [distribution recipes](../../docs/handbook/distributions.md).

The matrix covers BIOS/UEFI ISO/USB menu-only installation, full-disk no-write cancellation, sample creation/read without commands, two media-detached persistent boots, same-build repair with a whole-data-partition hash, independent GPT/FAT/kernel/ext2 verification and halted CPUs. Copy EIO and missing/nonblank/undersized disks exercise readable refusal/error/details paths. Native overlay quota/reclamation and unsafe profile rejection are separate gates. VMware EFI/BIOS use the exact public ISO and new 20-GiB sparse SATA VMDKs; pre-existing running VMs are inventoried and preserved.

The separate Forge recipe is actually compiled, booted, installed and booted without media. Its screenshot verifies the configured accent; native file reads verify the selected package. Its build identity differs from the standard profile. This is data-package/branding customization, not an implemented desktop/app ecosystem.

Screenshots are actual QEMU framebuffers: [welcome](uefi-iso/welcome.png), [disk review](uefi-iso/review.png), [completion](uefi-iso/screen.png), [Forge](forge/screen.png). Runtime logs are sanitized. Earlier M5/M4.5 frozen reports are not rewritten. Automated scripted tasks prove the no-command route, not universal user comprehension or a human study. Limits remain documented in the scope.

Build Forge, run `python tests/m51/guided.py --custom`, build the standard profile, then run `python tests/m51/verify.py`. Use `--collect` only for already successful matching reports; it checks exact source/artifact identity and normalizes report bytes before freezing hashes.
