# Contributing

Codename: Engine OS has completed M0 and the virtual M1 own-kernel boot milestone. The kernel boots through UEFI ISO/USB in QEMU; there is no installer or application support yet.

Read the architecture, decisions and milestone acceptance criteria before proposing changes. Keep changes small enough to review and include their purpose, verification and limitations. Document third-party provenance and licenses; do not submit proprietary OS components, SDKs, installers or credentials.

Original contributions are submitted under the repository's MIT license. Existing third-party notices remain applicable to their components.

## Current verification

```powershell
python m0/probes/run_probes.py
python m0/verify_m0.py
python tests/verify_m1.py
```

The first command runs the original small suite. The second runs all fresh M0 gates, including Windows references, C validators and QEMU/OVMF ISO/USB firmware fixtures. Requires the pinned tools and project-local ISO library; see [setup](tools/README.md). Full M0 qualification currently requires a Windows host. These checks do not boot an OS kernel or establish application compatibility.

The third command builds and boots the real M1 kernel, exercises negative/exception paths and captures the framebuffer. Its visual conversion uses Pillow (tested 12.3.0), a host-only test tool; it is not linked into the kernel. See [M1](M1.md) for artifacts and limits.

Public reports redact host-specific paths. Retain the probe scope and known limitations when publishing results. Generated result files should be reviewed before committing.

Use issues for reproducible defects or scoped proposals. Include tool versions, architecture and actual observed behavior. Avoid marking an entire application supported based on one successful startup.
