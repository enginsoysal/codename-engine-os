# M7.5 exact-artifact evidence

Native guided installation, actual installed restart and media-detached startup.
See acceptance-results.json for the final build, source, artifact hashes and gates.

Development issues corrected before acceptance: the VMware keyboard driver used
the previous wizard Tab order; a test incorrectly required media chaining even
when firmware selected the installed disk directly; the owned RFB connection
closes on reset and must reconnect. These were harness failures, not substituted
passes. Stale failure logs are retained as development history. The final reports
and referenced install/restart logs establish the accepted paths.

QEMU stopped-progress captures decode actual framebuffer bytes rather than the
timer-driven VGA display cache. VMware RFB assembly retains supplied rectangles
and rejects screenshots containing any pixel never supplied by the guest.

Agent visual review is not a human usability study. The original owner VM failure
was not independently diagnosed. No physical/universal hardware claim is made.
