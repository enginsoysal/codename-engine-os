# M10 device and USB usability review

This is an agent review of the native interface and completed guest tasks,
not a usability study with children or older adults. Installation retains the
[guided wizard requirements](installer-m75.md); M10 adds everyday device tasks.

| Question | Implemented behavior and review |
|---|---|
| Is the next action clear? | Start → Settings → Devices shows actual support states. Files has a named USB drives location, selectable drive/file rows and explicit copy destinations. |
| Is it convenient? | Supported providers bind automatically. Keyboard choices save automatically. Copies to Downloads retain the name or choose a collision alternative. Reconnection makes the drive usable again. |
| Is it friendly to inexperienced users? | Ordinary tasks use buttons. Write protection and unavailable devices do not silently become successful copies. Existing destination files are refused rather than overwritten. Safe remove provides a completion message before unplugging. |
| Is the appearance consistent? | Devices and USB views use the existing native window, button, spacing and accent system. USB rows adapt to available height. No terminal is required for the normal flow. |
| Is everything readable? | Native ordinary and actual 3840×2160 captures use the shared Inter typography and logical scaling. Labels and actions remain separate, with limited lists and paging rather than overlapping text. |

The native journeys exercise real pointer/keyboard input, a saved Dvorak choice
with actual typing, focused Files wheel scrolling, copying in both directions,
flush/removal/reconnect and independent byte verification. Separate motion tests
compare clipped cached frames with complete repaint and check retained pages
and halted idle CPUs. The VMware starter additionally exercises the visible
Devices layout controls without writing guest application state.

The scope deliberately remains visible: Limited 2D is not 3D acceleration,
Detected is not ready for use, and unsupported write profiles are refused.
Copies currently run synchronously with a keep-connected message and have no
mid-copy cancellation. Broader accessibility, international input methods,
general driver plugins and physical-device/human studies remain follow-ups.

[User tutorial](../handbook/hardware-m10.md) ·
[Architecture and limits](hardware-m10.md) ·
[Exact qualification](../../evidence/M10/acceptance-results.json).
