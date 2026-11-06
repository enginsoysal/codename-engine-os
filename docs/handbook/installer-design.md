# Engine OS installation wizard design

Status: historical browser design reference. The native M6 implementation follows this direction; see the [real installation guide](install-m6.md) and [qualification status](../milestones/m6.md). This HTML preview is never bootable installation media.

Open [the standalone preview](../../tools/design/install-wizard.html) locally in a browser. It contains no external assets, network calls or disk API. Choices remain in page memory only. All disks and application choices are demonstrations; progress is explicitly animated simulation. Reloading resets choices. The appearance selector describes a future desktop preference; the setup accent is the implemented visual preview control.

## Native wizard delivery requirements

1. Welcome: clear start, language/keyboard/accessibility choices as each is implemented, recommended defaults and optional advanced details.
2. Your disk: detected capacity, availability and exact supported layout choices; explain affected data, preserve Back/Cancel until writes start and revalidate the selected device at commit.
3. Components: removable apps and supported optional services with clear dependencies and required-base labels. Avoid artificial dependence on optional apps.
4. Personalize: user setup, supported theme/accent and distribution defaults. Retain edits when navigating back; validate fields inline. Persist actual selections into the installed system. Hide or label future options rather than silently ignore them.
5. Review: show every effective choice, the exact disk consequences and explicit final confirmation. The native Install action is distinct from Next; no accidental write on entering the page.
6. Install: actual verified stage/progress reporting, bounded logs, readable failures and a clear recovery route. Cancellation after writes start must explain the actual recoverable state; it cannot promise an untouched disk.
7. Finish: appear only after verified completion; explain removal of installation media and the next boot. Failure must never display installation success.

Every ordinary step needs a coherent graphical screen, Back/Next or a clearly named action, mouse and keyboard navigation, visible focus and useful validation. No terminal syntax is needed. Advanced/verbose remains optional. The native framebuffer implementation must be part of the OS; this HTML is a design reference, not a browser dependency to bundle into the kernel. Native rendering/input/backend integration is now implemented; exact release acceptance is tracked separately.

Customization is broad but truthful: supported disk plans, component selection, appearance, user settings and distro defaults. Provide sensible defaults so Next/Next is possible. Critical dependencies remain protected or need a working replacement. Optional application removal must leave unrelated functions operational; [component policy and removal acceptance](../architecture/components.md).

## Preview verification

The browser walkthrough exercised all seven stages, disabled Back on Welcome, removal of the Calculator selection, retained choices across Back/Next, display-name/accent editing, the review summary and simulated progress/Finish. It did not install or remove any OS component. Native runtime acceptance and human usability testing remain pending.
