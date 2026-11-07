# M7.3 five-question design review

This is an engineering/design review of native VM screens and scripted user
tasks. It is not a novice usability study, accessibility certification or a
promise that everyone will prefer the appearance. The owner judges the final
experience; the five questions remain release requirements for future changes.

## Review method

For each surface ask: Is the purpose clear? Is the next action convenient? Can
a person proceed without terminal syntax? Is the layout visually coherent? Is
the text readable without overlap, clipping or arbitrary scale changes?

Inspect real framebuffer captures at normal/minimum and HiDPI sizes, then
exercise the normal input path. Browser previews and enlarged screenshots do
not satisfy this review. Runtime evidence is stored in `evidence/M7.3`.

| Surface | Design decision and review focus |
| --- | --- |
| Welcome | A single Next default, seven named steps, short explanation of when disk writes begin. Existing desktop and Recovery are secondary actions. Firmware startup-entry creation is disclosed. |
| Disk selection | Explicit row selection, capacity, SATA eligibility and a useful missing-selection message. A nonblank target points to Manage disks or explicit whole-disk replacement. |
| Setup | Desktop and Files are included; the guide is an ordinary optional choice. No terminal or dependency syntax is needed. |
| Style | Named account field, simple supported-character guidance and three implemented accent choices. Choices survive Back and installed boot. |
| Review | A stable summary and separate whole-disk erase consent. Back preserves choices. Data-partition capacity is identified precisely. |
| Installing | Static illustration and actual verified progress. No animation polling or false success. |
| Finish | Open desktop enters the real persistent volume. The later-boot instruction says to disconnect only CD/DVD and retain SATA. Maintenance separately explains restarting. |
| Recovery and failure | Plain-language purpose, optional technical details, a safe return action and concrete recovery instructions. No raw status code is the ordinary next step. |
| Partition overview | Disk identity and capacities, readable partition/free-space rows, clear actions and Back to setup. Controller BDF stays out of the ordinary identity display. |
| Create/format/repair | Readable size/name input and effects explained before review. Quick format is distinguished from secure erasure. Other partitions are preserved where the operation promises that. |
| Destructive review | Target and consequences remain visible. Keep/Cancel is the emphasized safe route. Apply without explicit consent leaves bytes unchanged. |
| Files and folder dialog | Familiar locations, visible path, Back/Forward/Up, named folder creation, delete confirmation and measured fitting of long labels/paths. No Calculator dependency. |
| Appearance | Consistent body text and controls, light/dark choices and Auto/100/150/200 display size. Unsuitable sizing keeps the existing usable size and gives an explanation; saving persists the choice. |
| Personal text and advanced output | Existing text uses smooth glyphs; editor wraps by measured advances and terminal cells reserve sufficient width/height. Diagnostics remain optional. |

## Shared visual rules

Inter is rasterized at its actual physical size. Body text uses one regular
style; semibold headings establish deliberate hierarchy. Controls use measured
fitting; paragraphs wrap inside their content region. Ordinary setup uses a
light content surface with a dark navigation rail. Recovery/disk tools use a
consistent dark surface. Selection and focus are visible. Essential paragraphs
must not emit the native text-overflow diagnostic.

An explicit UI scale transforms drawing and input together. All three scale
choices use native-size masks rather than enlarged pixel characters. An
unsupported scale cannot strand the user with unreachable controls. Scale
changes preserve open application state; a new boot starts the folder desktop
using the saved suitable size or Auto.

## Practical limits retained

The installer still uses a supported whole SATA disk for OS installation;
arbitrary dual boot and in-place partition resizing are not offered. Files
browses the installed native volume, with bounded windows/history and no trash.
International shaping, screen readers, touch input, monitor hotplug and
post-boot GPU mode changes remain future work. Keyboard navigation and manual
scale choices are implemented, but do not establish full accessibility.

Resource review must include immutable font storage, framebuffer video storage
and managed allocator measurements separately. Idle should not repaint or
busy-poll. Better appearance does not waive destructive-operation safeguards,
verified boot slots, file integrity or honest compatibility limits.
