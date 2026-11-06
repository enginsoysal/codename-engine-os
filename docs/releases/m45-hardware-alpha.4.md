# M4.5 hardware preview 4 — persistent owner trust

This is an in-progress M4.5 preview, not full milestone completion. The original kernel still boots through dual BIOS/UEFI public test media. There is no installer, desktop or Windows application compatibility yet.

## Changes

- Reuse an encrypted owner PFX with a required independent certificate pin across signed payload builds; reject wrong identity/password, expired/incompatible certificates, stale source/kernel and malformed init.
- Keep passwords out of arguments and child environments; stage SignTool's one-use private PFX in an owner-only NTFS directory and remove it after success/failure. No global certificate import occurs.
- Expand the owner-enrolled VM suite to 23 checks, including an authenticated payload update, signing cleanup failure, db rotation, retired signer refusal, image/KEK role separation, foreign/replayed update refusal and explicit policy-snapshot recovery.
- Report actual xHCI initialization, live route/class bindings, read-only transfer/quarantine state and rejected root/hub ports in the hardware command.
- Update English architecture, workflow, release history, acceptance evidence and physical handoff requirements together.

## Qualification and use

`python tests/m45/verify_increment.py` rebuilds the public media and checks 29 gates, including named QEMU/VMware profiles and current M3/M0 regressions. Source/artifact hashes identify the exact tested build. The signing suite uses disposable TCG firmware variables, not the development host or existing VMs. [Evidence](../../evidence/M4.5/increment-results.json), [owner workflow and exact limits](../milestones/m4.5-owner-trust.md).

Download `EngineOS-M45.iso` for a separate x86-64 VMware test VM or `EngineOS-M45-usb.img` for an explicitly designated disposable removable medium. Verify `M45-SHA256SUMS.txt`. Normal public downloads are unsigned: Secure Boot must be disabled. Locally owner-signed UEFI media remain separate ignored output; private keys and enrollment/negative-test media are never public assets.

Physical boot/storage/input, the designated machine's production key enrollment/rotation/recovery and OEM/Microsoft factory trust remain open external qualification. The test signing host must be trusted; temporary deletion is not secure erasure, and no HSM/remote signing adapter is claimed. VM policy restoration is not physical firmware recovery. Broader unsupported controllers remain explicit. M4.5 stays open until agreed external acceptance exists; M5 has not started.
