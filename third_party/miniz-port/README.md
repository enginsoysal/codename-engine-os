# miniz freestanding boundary

The unmodified upstream sources in `../miniz` are pinned to the commit and
SHA-256 values in their provenance file. The retained MIT notice applies to
those sources. This directory supplies only the missing freestanding C headers
and a static export annotation. Heap, standard I/O, time, compression, archive
and zlib convenience APIs are disabled for the native inflate worker.

The intended execution boundary is a separately loaded, watchdog-bounded CPL3
worker, not kernel execution of an untrusted compressed stream. Dictionary and
decoder pages must be allocated only for an actual extraction and reclaimed on
completion, cancellation, fault or last-window close. Importing these files
alone does not qualify Archive Manager or ZIP support.
