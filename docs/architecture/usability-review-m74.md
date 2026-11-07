# Standard-app design review — M7.4

This is an agent's structured native-interface review, not a human usability
study. Exact artifact acceptance remains in the matching evidence report.
Review ordinary tasks, errors and confirmations at 1280×800 and native
3840×2160, retaining the existing Inter typography and display scaling.

| Owner question | Review and implementation |
|---|---|
| Is everything clear? | Task headings, named folders/files, explicit supported formats and a separate offline app manager. Disk operations identify the disk and effect before consent. |
| Is everything convenient? | Start launches apps; Files opens BMP/ZIP/WAV/HTML with their matching app. ZIP extraction creates a fresh folder; Screenshot offers Open picture; Settings always reinstalls removed packages. |
| Is everything user friendly? | Unsaved notes ask before closing. Removal preserves documents/private data and refuses active windows. Missing audio and unsupported formats leave the desktop usable. No terminal syntax is needed for these ordinary tasks. |
| Is it graphically attractive? | Consistent rounded surfaces, accent buttons, measured native Inter text, themed backgrounds and shared window controls. Actual 4K screenshots exercise the native compositor. |
| Is everything readable? | Consistent semantic text roles, measured wrapping/fitting, minimum useful window sizes and no enlarged pixel UI font. Review headings, confirmations, footer messages and picker paths separately. |

The native 4K typing test exposed excessive Calendar repaint damage. The
implementation now damages only the note/caret/footer as needed and batches
at most sixteen queued text edits. The audio task exposed compositor
starvation and partial ZIP output boundaries; those are functional issues,
not merely visual polish, and require native repeat evidence after fixing.

The first suite has explicit capability limits. Browser is static HTML/text;
Media Player accepts PCM WAV, Image Viewer accepts BMP and Archive Manager
extracts Stored/Deflate ZIP. Richer codecs, modern web rendering, Calendar
reminders, advanced editor features and additional desktop integrations remain
future work. Do not present the initial apps as universal format compatibility.

[Application guide](../handbook/apps-m74.md) ·
[Architecture and resource ownership](applications-m74.md) ·
[Milestone contract](../milestones/m7.4.md).
