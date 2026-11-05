# M0 evidence

Run `python tests/m0/verify.py` using the [documented host tools](../../tools/README.md). The gate regenerates results rather than accepting stale files. Scope: architecture/toolchain and host reference semantics, not a kernel or app implementation.

- [Aggregate acceptance](acceptance-results.json).
- [11 initial checks](probe-results.json): x64/ARM64 freestanding ELF, EFI output, fixed layouts on three architectures, Windows events/multiwait/TLS/file-sharing references.
- [30 actual C validator checks](contract-results.json): header/CRC/range/overlap/descriptor/framebuffer/request boundaries, including 160 truncated boot lengths, 24 truncated requests and 1000 bit mutations.
- [Eight Windows references](windows-reference-results.json): sharing-aware rename/delete, Unicode/case behavior, continuable vectored exception delivery/cleanup.
- [Two EFI-only firmware loads](environment-results.json), with [optical](environment-iso-serial.log) and [USB](environment-usb-serial.log) serial markers.

Total: 51 technical checks plus document/inventory validation. QEMU was found outside PATH and pinned alongside firmware/compiler/DLL hashes. pycdlib is a host-only pinned ISO author. Local pins do not authenticate an independently reproduced upstream compiler. ISO timestamps can change image hashes; successful firmware loading is the behavioral criterion.

The M0 EFI fixture stays in firmware services, writes `ENGINE_OS_M0_ENVIRONMENT_OK` and exits. It is not an own-kernel or installer image. Later M1/M2 evidence is separate. Host measurements do not establish guest API compatibility. Public results redact project/home/temp paths; generated binaries remain under ignored build directories. [M0 scope](../../docs/milestones/m0.md).
