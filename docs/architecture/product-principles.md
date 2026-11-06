# Product principles: convenience, clarity and distribution freedom

These are product requirements, not optional visual polish.

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

The owner revision of 6 November 2026 sets M6 real product installation, M7 real desktop experience, then M8 original MS-DOS and Windows 1.x through Windows 11 application compatibility. M6 is in progress with its initial capacity planner; M7 and M8 are not started. Previous milestone evidence remains bounded historical proof. Engine OS remains the product name. Convenience, optional diagnostics, distribution freedom and measured CPU/RAM use apply to every stage. See [the current contracts](../roadmap/milestones.md).
