# M7.5: understandable installation and independent startup

Owner requirement, 8 November 2026: every ordinary step should be approachable by an inexperienced 11-year-old or 60-year-old. The acceptance review uses novice scenarios; automated execution and agent review are not a human usability study.

## Reference review and choices

We reviewed representative official installation guides, not every operating system in existence:

- [Windows installation](https://support.microsoft.com/en-US/Windows/deployment/install-upgrade/reinstall-windows-with-the-installation-media): adopt an explicit selectable disk list and deliberate deletion decision. Use whole-row hit targets, selection marks, capacities and unavailable Next before selection. No Windows assets or partition-preserving OS installation are claimed.
- [Ubuntu Desktop](https://ubuntu.com/tutorials/install-ubuntu-desktop): adopt a guided interactive route, separate advanced configuration and final summary.
- [macOS installation](https://support.apple.com/en-asia/102655): retain graphical Disk tools, guarded changes and a clear return to setup.
- [Zorin replacement](https://help.zorin.com/docs/getting-started/replace-your-zorin-os-installation/): include starting the installed operating system in the normal journey. Restart is the primary final action; opening the live-session desktop is a separate shortcut.

Avoid text that secretly acts as selection, unexplained erase toggles, numeric status codes as ordinary instructions, redundant full-screen redraws and success based only on opening a live desktop.

## Step-by-step novice review

| Step | Ordinary action | Clarity and convenience | Required verification |
|---|---|---|---|
| Welcome | Read short explanation; Next | No command syntax, network or online account | Mouse and keyboard can proceed |
| Disk | Select a row in a bordered list | Radio marks, whole-row hit targets, capacities, disabled Next before selection | Capacity-column click; missing/small/multiple disks |
| Apps | Review essentials; optional guide | Customization has implemented effects; packages removable later | Toggle and revisit |
| Style | Enter account name; choose colour | Input box, useful defaults and plain rules | Editing, empty-name rejection and persistence |
| Review | Check named disk and choices; Install | Existing files require separate erase consent | No early writes; Back; other disks unchanged |
| Installing | Wait through real stages/progress | Fixed frame; no simulated timer or success | Actual progress capture and injected failure |
| Ready | Restart to Engine OS | Actual restart is the normal path | ISO retained/removed; same VM; installed kernel and saved data |

Controls use large hit regions, semantic Inter typography, measured fitting/wrapping and consistent hierarchy. Keyboard focus has a visible border. Actual 4K coverage uses native physical font sizes and existing scaling policy.

## Rendering and resources

Retain the outer frame. Next updates content and sidebar; Tab changes only focus rings; repeated progress updates only the stage/progress region. No second full-resolution framebuffer, browser runtime or polling animation loop is added. Idle CPUs halt. Native frame counters and screenshots distinguish implementation from a browser preview.

## Installed startup

Verify writes, GPT mirrors, activation metadata and saved settings before completion. Restart requests platform reset. Matching installation media can redirect to exactly one installed system when left connected. UEFI performs a bounded filesystem scan for an Engine-owned matching build marker and hard-disk path, then LoadImage/StartImage runs the actual disk loader. BIOS scans bounded EDD hard disks for committed matching activation identity and transfers through the installed bootstrap. Neither substitutes the live desktop. Zero/ambiguous matches remain in setup. Escape at early media startup bypasses redirection for recovery.

The marker is not cryptographic trust or universal hardware qualification. Existing firmware-registration guards remain. Media-detached boot is an independent gate.

## Limits

Named x86-64 VMware/QEMU profiles, unsigned media and supported SATA bounds remain. OS installation consumes the selected whole disk after consent; no dual boot or resize. Cross-release package migration and physical hardware are separate. The exact original user failure needs its own screen/log/configuration; reproducing a retained-ISO return to setup cannot establish every previous failure's cause.
