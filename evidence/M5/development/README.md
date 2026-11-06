# M5 development diagnostics

These are retained failures, not final qualification.

- The first live payload exceeded the BIOS loader's two-MiB kernel bound because zero-filled FAT metadata was embedded literally. The payload now encodes zero pages without allocating/storing their content, validates its encoded bounds and expands only one four-KiB I/O page at a time. The BIOS bound was retained.
- The first QEMU harness assigned the blank destination an explicit boot priority ahead of the ISO. Firmware correctly entered its shell. The harness now prioritizes the installed disk only for media-detached boots and cleans up owned guests on failed startup. See the preserved boot-order report.

Final reports outside this folder must match the final build ID and artifact hashes.

The first full runtime matrix passed, but its final repository gate detected Windows-default decoding of UTF-8 architecture documents. The affected text was repaired, all source files were normalized to the repository LF policy, and the final release matrix was rerun against the resulting exact image. No failed matrix was published as completion.

A qualification invocation was launched before the asynchronous build command had fully exited and read its intermediate manifest. The independent kernel-hash check refused the missing final field. The test launcher now requires final M5 manifest fields and current source hashes before any guest starts; qualification was restarted only after the build process exited successfully. See early-manifest-read.json.
