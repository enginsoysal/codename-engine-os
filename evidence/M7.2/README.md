# M7.2 native installer and folder desktop evidence

Run from the canonical repository using the pinned local toolchain:

```powershell
python tools/build/m72.py
python tools/release/vmware_starter.py
python tests/m72/qualify.py
python tests/check_repository.py --milestone M7.2
python tests/m72/verify.py --commit HEAD
```

Build and package once, freeze native sources and archive bytes, then qualify. Rebuilding the growable VMware disk produces a different ZIP identity; do not replace the tested archive before publishing. Tests create disposable owned guests/images only. VMware remote display is authenticated and bound to loopback inside test configuration, never the user's desktop. The published VMX has no such observation settings.

`runtime-results.json` records ten fresh runtime gates and one actual-C boot-variable host gate. Each directory contains task assertions, actual serial output and captured native frames. The final aggregate records exact input Git bytes and artifact hashes, runtime reports/logs/harness inputs, separately measured resources, public starter archive identity and all retained historical proof hashes. Publication/readback evidence is separate from OS runtime qualification.

The `baseline` directory shows a retained M7.1 disk starting with ordinary UEFI discovery. It disproves neither the user's report nor an unknown configuration problem. The M7.2 persistent-VM tasks explicitly address the former forced-order/fresh-VM coverage gap. Accepted M4.5–M7.1 proof files and binary assets are not overwritten.

Scope and limitations: [milestone](../../docs/milestones/m7.2.md), [architecture](../../docs/architecture/desktop-m72.md), [user tutorial](../../docs/handbook/install-m72.md).
