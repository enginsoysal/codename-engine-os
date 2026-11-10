# Product principles: convenience, clarity and distribution freedom

These are product requirements, not optional visual polish.

## Visual quality and usability gate — 7 November 2026

The owner prioritizes the experience of using Engacer OS, not the length of its feature list. Every installer step, normal desktop task, confirmation and error must be reviewed against five questions: Is it clear? Is it convenient? Is it user friendly? Is it graphically polished? Is everything readable?

Use an intentional type scale, smooth native text, consistent controls, contrast, whitespace and measured wrapping. Essential instructions cannot overlap, disappear beyond a control or depend on tiny print. Scaling must keep text and input hit targets aligned on ordinary and 4K displays. Increasing framebuffer resolution while enlarging the old pixel font is insufficient. Resource efficiency must coexist with this quality: retain bounded memory, direct clipped rendering and halt at idle. Test actual native framebuffers and complete tasks; distinguish the agent's structured design review from a novice/human usability study.

## Convenient defaults

Normal tasks must have a discoverable guided path. Boot must explain available actions and their consequences. A new user must not need to translate `N`, raw status codes or sector counts into an installation procedure. Reuse detected capabilities, but never guess a destructive target or hide a storage limit. Keep the safe back/cancel path available until writes begin.

## Clear interaction, optional diagnostics

Menus use ordinary language, real choices, visible progress and useful next steps after failure. State whether data is temporary or persistent. A message must distinguish success, refusal, failure and an uncertain durable outcome. Keep detailed logs and a terminal available through Advanced/Details; do not make them prerequisites for ordinary use.

The acceptance question is task-based: can installation, cancellation and repair be completed using only what the screen says? Automated menu tests enforce the mechanical route; actual novice testing is still needed to assess comprehension. Do not claim universal ease of use, accessibility or a human study from automation alone. M5.1 delivers a keyboard-driven text wizard; M6 must preserve these principles in product installation and M7 in its desktop.

## Open distribution architecture

Separate mechanism from identity, presentation and selected content. Provide a versioned, validated recipe and reproducible source/input identity for each image. Publish working examples and an actual build command. Keep safety/core contracts stable across distributions and document format evolution. Reject unsupported options rather than storing configuration that has no effect.

M5.1 concretely supports branding, accent, welcome text and data packages/files through [distribution profiles](../handbook/distributions.md). Future desktops, shells and package runtimes need their own explicit contracts and tests before becoming profile options. The kernel and tooling remain MIT source; authors can fork components as well as use recipes.

## Resource and safety requirements

Guidance must not require a heavyweight GUI stack or a polling input loop. Bound menu state, buffers and optional content; retain CPU halt at input idle. Convenience never bypasses explicit destructive review, target revalidation, integrity checks or recovery limits. Record implementation, why it was chosen, measured behavior and limitations together.

## Current product delivery order

The owner revision of 6 November 2026 sets M6 real product installation, M7 real desktop experience, then M8 original MS-DOS and Windows 1.x through Windows 11 application compatibility. M6 is complete within M6-VM-v1, with a native graphical installer, scalable volume and tested recovery; M7 is complete within M7-VM-v1 with a native installed desktop and checked HTTPS downloads; M8 qualifies a first original Windows console/MSI profile; the wider DOS/Windows program remains in progress. M10 hardware phases 1 and 2 now take priority before further application profiles. Previous milestone evidence remains bounded historical proof. The owner selected Engacer OS for new M10 UI/media; historical Engine OS identities stay intact. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply to every stage. See [the current contracts](../roadmap/milestones.md).

## Graphical setup and removable optional apps

M6 requires a native graphical wizard with Back/Next/Finish, retained editable choices, simple defaults and optional advanced details. Extensive customization must have implemented effects. Optional apps must not be dependencies of base services or the desktop; removing Calculator must leave unrelated functions usable. Required components need explicit dependency handling or a verified replacement. See [component boundaries](components.md) and [wizard design](../handbook/installer-design.md). The browser preview is not native implementation evidence.

The owner revision of 7 November 2026 prioritizes M7.2 installation and a visible real folder desktop before more applications. The final installer action opens persistent Files immediately; independent disk boot, same-VM firmware discovery and truthful recovery limits remain separate acceptance requirements. [Current native design](desktop-m72.md), [user flow](../handbook/install-m72.md).
