# M7 accepted desktop evidence

The [acceptance report](acceptance-results.json) passes **26 gates** within **M7-VM-v1** and verifies all **146 earlier frozen proofs** unchanged. [Release](../../docs/releases/m7-v0.7.0.md), [VMware tutorial](../../docs/handbook/install-m7.md), [desktop controls](../../docs/handbook/desktop-m7.md).

Standard build: `d2c55268cc978efc04f371aab52bb3ae`.
ISO SHA-256: `58aa32971b47c820d53b0e2b6525bb86cd04939311cc6a9ad922b10fa701d98b`.
Native source commit: `32367f9f3e5eec8d5464aea6347a38834b34da8a`.
Night build: `b7573bd4c2c21284a8a759f69029691f`.

## What counts

Only the explicitly enumerated gates in acceptance count toward completion: nine host gates, thirteen native runtime gates, two custom/update gates and two serialized VMware visual gates. Group reports bind exact standard media, source snapshots, captured harness hashes and individual report hashes. Individual reports deliberately keep `m7_complete: false`: one slice never establishes the whole milestone. Aggregate acceptance establishes the named scope.

The native tasks cover everyday named/nested files, safe deletion, actual terminal commands, document persistence, private CPL3 app faults/timeouts, optional executable removal/reinstall, real Ethernet/DNS, checked HTTPS and failures, larger public binary bytes, BIOS/UEFI optical/raw USB, pressure recovery and idle halt. The Night and maintenance tasks qualify a separately installed profile plus kernel update/rollback/BIOS repair with an unchanged complete data partition. Real VMware BIOS/UEFI runs capture only owned authenticated loopback framebuffers and type into the actual Editor; the existing VM inventory is unchanged.

Managed baseline 159,744–258,048 bytes, 471,040 whole-boot peak and 40,960 private bytes per additional Editor are allocator counters. The installed static kernel span is 1,743,488 bytes; QEMU visible framebuffer is 4,096,000 bytes. Firmware/boot/device reservations are separate and may overlap named regions. Do not sum overlapping reservations or equate configured VM RAM with consumption. Larger download managed baseline/max are both 249,856 bytes. One input repaints a bounded region; idle paints stop and all configured CPUs halt. Host-observed timing is not a human study or hardware benchmark.

## Historical development material

`development-build-manifest.json`, the early BearSSL compile report and nonvisual VMware diagnostic directories describe earlier slices. They are not substitute acceptance gates. Retained failed attempts and explanations live under `history/`; stale failure transcripts were copied there before removal from successful current directories. Long serial traces use public UTF-8/LF formatting and trimmed prompt whitespace. Earlier accepted milestone evidence is never reformatted.

Two unused files changed after the host/native snapshots: the VMware-only serial wait routine and the acceptance collector itself. The collector verifies their original captured bytes against commit `d5ee13d`, records the exception explicitly and checks all actually used harness files remain unchanged. Final VMware/custom groups capture the corrected files. No native source/media changed in that correction. Night's profile reline-ending rebuild reran both custom gates; standard media remained unchanged.

Physical hardware, factory Secure Boot, universal device support, a human usability study and original DOS/Windows execution remain unqualified. Editor/Calculator are original native apps, not Windows compatibility. Downloading the original 7-Zip installer proves bytes and persistence only; it is never bundled or executed. M8 owns original DOS/Windows application support.
