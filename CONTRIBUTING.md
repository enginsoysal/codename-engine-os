# Contributing

Engacer OS is an independent own-kernel project with a native graphical installer,
installed folder desktop and a bounded original 7-Zip Windows console/MSI profile.
M10 develops hardware phases 1 and 2; the [current status](docs/roadmap/status.md)
identifies the accepted release and remaining limits. Read the
[architecture](docs/architecture/overview.md), [decisions](docs/architecture/decisions.md)
and [agent instructions](AGENTS.md) first. Historical Engine OS artifacts remain
verifiable under their original identities.

## Language, structure and documentation

All current documents, code comments, UI strings and release notes are English. Keep code under its source subsystem, docs under architecture/roadmap/milestones/releases, and tests/evidence grouped by milestone. Do not add root-level planning files or put OS files in BrixCanvas. Never publish ignored local archives.

Every behavior/scope change updates contracts/status, user commands, tests/evidence and known limitations in the same coherent change. New findings supersede plans explicitly. Historical tags/reports retain their original identity; do not rewrite their hashes to match new source paths.

## Current UI quality gate

Use the five questions in [product principles](docs/architecture/product-principles.md) and inspect actual native setup/desktop captures at minimum and HiDPI sizes. Do not use browser mockups or enlarged screenshots as 4K proof. The Inter source/rasterizer/output digest is pinned; preserve the full OFL notices. Reproduce M7.3 only in a separate release checkout using [its commands and limits](evidence/M7.3/README.md). New changes require new milestone/revision outputs; never overwrite accepted historical evidence or binaries.

## Verification and resources

```powershell
python tools/build/m10.py
python tools/release/vmware_starter10.py
# Follow the complete tests/m10/README.md sequence on frozen artifacts.
python tests/m10/verify.py
```

Use the [pinned host tools](tools/README.md). M10 qualification requires the pinned compiler, QEMU/OVMF, VMware Workstation, Pillow and the test-only FAT dependencies in tests/m10/requirements.txt. Tests run on disposable images without host disk/directory exposure. M0 can be run separately through `python tests/m0/verify.py`; M1's historical path is `python tests/m1/verify.py`.

CPU/RAM efficiency is a primary acceptance criterion. Bound allocations, queues and logging; share immutable code; measure peak/retained/boot categories separately and verify idle halting. Never equate configured guest memory with kernel consumption. Do not trade away correctness/isolation for a small reported number.

Include rationale, alternatives, verification and limits in changes. Test original app binaries only through lawful access; do not submit proprietary installers/SDKs or credentials. Record third-party revisions, provenance, licenses and host dependencies before ports. Original contributions use [MIT](LICENSE); third-party notices remain applicable.

The project owner requests GitHub publication of every completed coherent update. Run appropriate checks, review, commit and push; never force-push others' work or claim synchronization after failure. Report actual failures and version/task-specific app support rather than invented global guarantees.
