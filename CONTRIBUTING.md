# Contributing

Codename: Engine OS has completed architecture/toolchain milestone M0. M1 kernel work is next. No bootable OS or installer is available yet.

Read the architecture, decisions and milestone acceptance criteria before proposing changes. Keep changes small enough to review and include their purpose, verification and limitations. Document third-party provenance and licenses; do not submit proprietary OS components, SDKs, installers or credentials.

Original contributions are submitted under the repository's MIT license. Existing third-party notices remain applicable to their components.

## Current verification

```powershell
python m0/probes/run_probes.py
python m0/verify_m0.py
```

The first command runs the original small suite. The second runs all fresh M0 gates, including Windows references, C validators and QEMU/OVMF ISO/USB firmware fixtures. Requires the pinned tools and project-local ISO library; see [setup](tools/README.md). Full M0 qualification currently requires a Windows host. These checks do not boot an OS kernel or establish application compatibility.

Public reports redact host-specific paths. Retain the probe scope and known limitations when publishing results. Generated result files should be reviewed before committing.

Use issues for reproducible defects or scoped proposals. Include tool versions, architecture and actual observed behavior. Avoid marking an entire application supported based on one successful startup.
