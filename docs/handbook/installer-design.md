# Engine OS installation wizard design

Current native implementation: [M7.5 guided installation and installed restart](install-m75.md), with [reference review and novice step matrix](../architecture/installer-m75.md). Previous implementation: [M7.2 installation and immediate folder desktop](install-m72.md), with its [architecture and actual native evidence](../architecture/desktop-m72.md).

The current HTML file is an explanatory browser design reference, not bootable installation media or runtime acceptance evidence. Open [the standalone preview](../../tools/design/install-wizard.html) locally. It has no external assets, network calls, disk APIs, package installation or restart capability. Its demo disk is explicitly fictional. Choices remain in page memory; reloading resets them. The app/style and progress screens explain the native design and do not pretend to execute those operations. Use the released ISO to test actual customization, writes and installed startup.

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

The historical browser walkthrough exercised all seven stages, disabled Back on Welcome, removal of the Calculator selection, retained choices across Back/Next, display-name/accent editing, the review summary and simulated progress/Finish. It did not install or remove any OS component. Native runtime acceptance and human usability testing remain pending.
