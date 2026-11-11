# M11.2 — native Files and genuine file operations

The native stage passes [23 guest tasks, production input checks and 288 storage
fault cases](../../evidence/M11/2/acceptance-results.json), with 543 exact source
inputs and thirteen artifact digests. Follow the ordered
[M11 contract](../milestones/m11.md); M10 remains the downloadable release.
Public website verification is a separate required gate before M11.3.

## Implementation decisions

- Extend the native volume with atomic subtree move/remove. Validate every new
  descendant path, reject moving into itself and refuse unsupported directory
  replacement. Preserve the existing native disk format and older milestone flags.
- Confirm metadata-bank bytes after flush before acknowledging a mutation. A
  failed acknowledgement can have an old-or-new result after restart; report
  uncertainty and read-only recovery instead of claiming nothing changed.
- Native copies use one reusable 4-KiB buffer and the existing stream reservation.
  Seal/flush data, verify it incrementally, compare source size/CRC, then publish
  metadata. Source changes or cancellation abort unpublished data. No source is
  removed before a durable destination. Same-volume moves reuse metadata.
- One bounded cooperative job runs at a time. Admit up to 128 top-level roots and
  1024 tree entries; explain quota refusal. Deduplicate selected descendants under
  a selected parent. Snapshot source identity and detect changes while copying.
  Report partial completion accurately and keep other apps responsive.
- Name conflicts require Keep both, Skip or explicitly reviewed file replacement.
  Directory replacement is unavailable. Never overwrite by default. Keep both
  preserves the extension and the native 63-byte component/255-byte path bounds.
- Recycle real payloads into a protected per-user store, with verified original
  path records in a separate protected metadata store. Write the record before
  atomically moving the payload; incomplete records cannot remove an original.
  Restore validates the user path and conflict policy. Permanent removal and
  emptying the Bin require a separate explicit review. Ignore safe orphan records.
- Keep one supported undo record, guarded by a target-subtree content/path fingerprint and in-use checks.
  Unsupported batches and irreversible removal do not pretend to have undo.
- Use real indexed directory listings with bounded sort caches. One selection
  identity serves detail/icon views; names, types and sizes come from actual
  entries. No modified/deleted timestamps are fabricated. Persist bounded view
  and sort choices, add address/Places navigation and show selection counts.
- Supply context-dependent Open/Open with, Copy/Cut/Paste, Rename, Recycle,
  Restore, Properties and shortcuts. Unsupported/provider-protected operations
  are visibly unavailable. Validate shortcut/association records against actual
  installed handlers; no arbitrary shell commands. Desktop files are real data.
- Drag/drop uses selected real paths, clear copy/move feedback and supported
  folder/Desktop destinations. Escape cancels capture without losing the source.
- Preserve the M10 create-only FAT32 provider and its explicit safety bounds;
  do not invent USB cut, recycle, overwrite, format or OS installation support.

## Closing evidence

Candidate build `3110d3a6953b4a6683b5a4d8869b47bd`, ISO SHA256
`48fd64897945e022ad53de5038f7526ac17d031973e59846cc6eecb021f7a38a`,
passes real installed create/edit/copy/cut/paste/rename/recycle/restore, subtree
operations, conflicts, protected paths, shortcuts, Open with, pointer drag/cancel,
Properties, saved sorting/views and detached startup. Stored payload/metadata
bytes are independently inspected. [Exact evidence](../../evidence/M11/2/acceptance-results.json).
Host fault tests remount old-or-new metadata and reject damaged copy publication.
This does not replace final M11.6 controller/app/USB/display qualification.

## Candidate resource observation

The installed desktop object has 530,768 bytes of fixed BSS, against 120,324
bytes in the accepted stage-1 map: a 410,444-byte increase, below the 512-KiB
stage budget. This includes bounded selection/clipboard/job/cache state and the
extra registered click areas. It is separate from the managed allocator and
framebuffer reservation. Ordinary 1280x800 startup currently retains the same
4,288,512 managed bytes and five pre-existing allocation refusals as
stage 1; neither value is called zero overhead or universal low-RAM support.
The final stage requires repeated idle/motion/display observations.

Directory operations were chosen over recursive UI calls because a subtree
rename must publish one validated metadata transaction. Copies deliberately
remain per-entry cooperative jobs: whole-selection atomic copies would need a
new disk transaction design and more staging space. A partial copy is therefore
reported as partial, with confirmed items retained. The fixed 32-folder cache
preference bound and one session Undo record avoid a growing history allocation.

See the [Files guide](../handbook/files-m11.md). The native and exact-source gates pass; the public publication gate is recorded
separately before the next stage.

A failed replacement can retain a complete private staging file while leaving
the old destination and source intact. Disk I/O uncertainty stops writes. Do not
promise automatic space recovery or whole-batch rollback: those are separate
storage-recovery capabilities. GUI system-path protection is an ordinary product
safety boundary, not a claim of a hardened multi-user permission model.
