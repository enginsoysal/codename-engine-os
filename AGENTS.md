# Codename: Engine OS — project instructions

## Scope and architecture

- This standalone repository is the canonical project. Earlier planning copies under BrixCanvas are historical, not the working source.
- Read README.md, STATUS.md and relevant M0 contracts before implementing changes.
- Build an independent OS and kernel. Windows application compatibility is the first product path; Linux, macOS and ARM remain planned extensions.
- Keep milestone implementation and evidence separate from architecture claims. Never mark runtime behavior complete based on a stub, header or host reference test.
- Record each substantial decision with how, why, alternatives, limitations and verification. Keep status and milestone documents consistent.

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
