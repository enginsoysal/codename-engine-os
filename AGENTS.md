# Codename: Engine OS — project instructions

## Scope and architecture

- This standalone repository is the canonical project. Never place Engine OS sources, plans, build outputs or evidence in the unrelated BrixCanvas repository.
- The old planning copy has been moved out of BrixCanvas into this repository's ignored `tools/local/legacy-planning` archive. It is historical and must not be published or treated as current source. Use this repository root explicitly as the working directory even when the chat opens elsewhere.
- Read README.md, docs/roadmap/status.md and relevant architecture contracts before implementing changes.
- Build an independent OS and kernel. The owner priority order is M6 real product installation, M7 real desktop, then M8 original MS-DOS and Windows 1.x through Windows 11 applications. Windows is the first foreign application ecosystem; Linux, macOS and ARM remain deferred extensions. Keep the product name Engine OS.
- Keep milestone implementation and evidence separate from architecture claims. Never mark runtime behavior complete based on a stub, header or host reference test.
- Record each substantial decision with how, why, alternatives, limitations and verification. Keep status and milestone documents consistent.
- All committed documentation, code comments, UI strings and new release notes must be in English. Update architecture, status, tests/evidence and user instructions together whenever behavior or scope changes.
- CPU/RAM efficiency is a primary requirement. Bound allocations/queues, share immutable code, measure peak and retained memory separately, and halt when idle instead of polling. Never equate configured guest RAM with kernel consumption or native probes with Windows compatibility.

## Product requirements

- Owner requirement, 8 November 2026: review every installation step for inexperienced children and older adults. Disk choices must be an obvious bounded list with whole-row hit targets, visible selection and useful disabled actions. Next must not clear/reload the entire UI. Completion requires actual installed startup, including the common retained-ISO case; a live-session desktop is insufficient. Keep novice scenario review distinct from a real human study.

- Owner priority, 9 November 2026: M9 automatic hardware support now precedes further M8 application expansion. First address stuttering VMware folder-window movement with measured graphics/input improvements and automatic supported-device binding. Do not infer complete hardware support, GPU acceleration or smoothness from an enumerated adapter or a screenshot.

- Owner requirement, 7 November 2026: appearance and convenience are first-class acceptance criteria. Review every ordinary step, error, confirmation and desktop surface for clarity, convenience, user friendliness, graphical quality and readability. Use consistent semantic typography, measured text, spacing and visual hierarchy; reject overlapping or clipped essential text, arbitrary adjacent font sizes and pixel-enlarged bitmap UI fonts. Verify the real native UI at ordinary and HiDPI resolutions, including an actual 3840x2160 framebuffer. Keep user control over suitable scaling. Do not call a browser mockup or an upscaled screenshot proof of native 4K support. Human usability studies remain distinct from the agent's design review.

- Convenience and user friendliness are acceptance requirements from the first boot. Ordinary install/repair tasks must use guided choices, clear review/cancel, readable errors and optional details; terminal syntax and placeholders are not the normal user flow.
- M6 requires a native graphical Back/Next/Finish wizard with retained customizable choices and meaningful progress. A browser mockup is design evidence only. Optional apps must be independently removable: the desktop/base cannot depend on Calculator. Resolve shared/required dependencies explicitly; never silently cascade app removal into system removal. Follow docs/architecture/components.md.
- Preserve distribution freedom through validated, versioned profiles and working custom ISO examples. Only advertise configuration choices that affect implemented behavior. Core destructive-operation guards cannot be bypassed by a theme/profile.
- Follow docs/architecture/product-principles.md; automate complete user tasks and distinguish those checks from an actual human usability study.

## GitHub publication — explicit user instruction

The owner requested an open-source project and that every update goes to GitHub.

- After each coherent completed update, run the checks appropriate to that change, inspect the diff, update status/evidence where needed, commit and push to the canonical GitHub repository before reporting completion.
- This instruction authorizes routine publication of project changes; do not repeatedly ask for permission to push.
- Never publish credentials, private accounts, proprietary installers/SDKs, unrelated project files or raw local user paths. Use public evidence with sensitive paths redacted.
- Do not use force-push or discard others' changes. Fetch and reconcile concurrent work safely.
- If a check fails, retain the honest result and fix the problem. If publication fails, report the exact failure and do not claim synchronization.
- Use codex/ prefixes for new task branches. Follow repository branch rules; the initial baseline lives on main.

## Licensing and safety

- Original project contributions use the root MIT license. Third-party components retain their own notices and require provenance/license review before inclusion.
- Run OS/storage tests against disposable images or explicitly designated test devices. Never treat the development host disk as a test target.
- Avoid global tool replacements; prefer pinned project-local tool manifests.

## Accepted release immutability

M4.5, M5, M5.1, M6, M7, M7.1, M7.2, M7.3, M7.4, M7.5, M8 and the M9-GFX-VM-v1 delivery accepted evidence, tags and release binaries are historical proof. Do not rerun a development build/test into their frozen output paths or rewrite accepted hashes. Use a new follow-up prefix/evidence directory; reproduce a historical release only in a separate checkout.
