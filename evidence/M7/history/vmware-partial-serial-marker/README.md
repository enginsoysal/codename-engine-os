# Partial serial marker race

The BIOS guest reached `M7_DESKTOP_READY`, but the pipe reader had received only a prefix of the build ID when the test asserted the complete line. The retained log ends with that partial ID. The corrected harness waits for the complete build-bound marker instead of treating the first prefix as a complete line. This changes only the VMware test, not Engine OS or its media.
