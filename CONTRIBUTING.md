# Contributing

Codename: Engine OS is in architecture milestone M0. No bootable OS is available yet.

Read the architecture, decisions and milestone acceptance criteria before proposing changes. Keep changes small enough to review and include their purpose, verification and limitations. Document third-party provenance and licenses; do not submit proprietary OS components, SDKs, installers or credentials.

Original contributions are submitted under the repository's MIT license. Existing third-party notices remain applicable to their components.

## Current verification

```powershell
python m0/probes/run_probes.py
```

Requires Python and Clang/LLD on PATH. Windows host-reference checks run only on Windows. The script creates temporary compile/link fixtures and writes evidence/M0/probe-results.json. These checks do not boot a kernel or establish application compatibility.

Public reports redact host-specific paths. Retain the probe scope and known limitations when publishing results. Generated result files should be reviewed before committing.

Use issues for reproducible defects or scoped proposals. Include tool versions, architecture and actual observed behavior. Avoid marking an entire application supported based on one successful startup.
