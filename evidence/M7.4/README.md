# M7.4 standard-application evidence

The exact acceptance report identifies the current candidate/release by source
commit, build ID and artifact SHA256. Its listed gate reports must pass for
those exact bytes. Ordinary tasks use actual native controls and injected input
in owned disposable VMs; host algorithm tests are labelled separately.

Development directories retain earlier failure logs and candidate captures.
A historical `failure.log` is not a current gate result. The explicit recovery
and archive regression records preserve discovered defects and their fixes.
The accepted reports and matching native logs, not a mockup or a successful
build alone, determine qualification. The first app delivery has explicit
format/feature limits in the handbook; richer capabilities remain follow-ups.

Reproduce the standard build with `python tools/build/m74.py`. The minimal
profile example uses `--profile distros/engineos-minimal-apps.json --revision
minimal`. Run the M7.4 native scripts only against their new disposable images;
never overwrite historical M4.5 through M7.3 evidence. After all gates, bind
committed native inputs with `python tests/m74/verify.py --commit HEAD`.

Text logs normalize CRLF to LF before freezing, so Git checkouts retain the
accepted bytes. Raw device audio capture is preserved as binary.
