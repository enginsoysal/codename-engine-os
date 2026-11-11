# M11.4 windows and appearance evidence

Native **M11-STAGE4-VM-v1** acceptance passed on 11 November 2026. This is a
stage-specific delivery, separate from the pending M11.5 setup and M11.6 full
controller/application regression.

- Build: `33e8ddd4cbc611aae509acb0d4325b4b`; 556 exact source inputs and fourteen
  checked artifacts including the separately qualified VMware ZIP.
- ISO: `ae392db4c4b4491ebe1cc959d107b8d97abcb44cd534fb321e1a349c28e34fa5`.
- VMware ZIP: `ab62dfca8e8e2003a8f5e5fb14c9150ebde2b751735da6aa59983b0742587ae5`.
- [Acceptance](acceptance-results.json), [source/artifact manifest](candidate-manifest.json),
  [starter manifest](starter-manifest.json), [resource accounting](resources.json),
  [unchanged accepted deliveries](preserved-releases.json).
- [Native tasks](native/results.json), [USB/PS2 parity](keys/results.json),
  [ordinary/150%/4K cached/full comparisons](motion/results.json),
  [exact VMware installation/restart/two detached boots](vmware/results.json),
  [production classifier](pointer-host-results.json).

All GUI tasks use ordinary guest mouse/keyboard input; no guest memory is written.
Screenshots decode actual physical guest pixels; capture metadata records the
framebuffer size and raw digest. Twelve comparisons check every viewport pixel.
Host/QMP movement timings include software emulation and observation overhead;
they do not measure monitor FPS. Retained managed pages and idle paints stay
unchanged, and all idle CPUs halt. The extra desktop BSS/data is 768 bytes above
M11.3; accumulated fixed state from M11.1 is 423,468 bytes, below 512 KiB.

The prepared VMware ZIP contains a new empty 20-GiB SATA disk, not a preinstalled
desktop. Network/USB/audio stay enabled. Its actual retained-ISO reset and two
disk-only boots preserve the same VMX/NVRAM without forcing boot order. Public
GitHub release/site/download proof is recorded separately before M11.5 starts.
Accepted M10 and M11.1–M11.3 artifacts remain unchanged. No physical hardware,
vendor GPU 3D, unlimited windows or general Windows compatibility is inferred.
