# BrixCanvas documentation website

The Engine OS website belongs inside BrixCanvas. The requested public address is **https://engineos.brixcanvas.com**. The shorter address was selected from the owner's options on 2026-11-06. OS sources remain in this independent repository; website content uses ordinary editable BrixCanvas Brix and articles.

## Native website

The BrixCanvas project is `Codename: Engine OS`, site ID `b66d0e83-7987-4856-b702-e8d93234b6cd`, initial preview slug `codename-engine-os-ltvck`. The homepage uses native Menu, Layout, Button, Text, Heading, Code and Alert Brix. Documentation belongs to the native Blog content model, grouped by handbook, roadmap, architecture, milestones, releases, developer guides and evidence. Native site-wide search reads article body text, titles and tags. No external website is the production origin.

The owner preferred the initial local prototype's appearance over the generated BrixCanvas template. `website/brix/design.mjs` records the replacement native composition: navy Menu Brix, light status strip, two-column introduction/terminal, three guide cards, scope notice and minimal footer. The layout uses ordinary nested Layout, Heading, Text, Code, Button and Alert Brix, not an HTML embed. Import individual sections through BrixCanvas's native **Paste Brix** operation using its `brixcanvas: 1` clipboard format; all children remain editable. The site uses the native minimal/Inter font palette and English public locale.

`python website/brix/check.py` checks anonymous HTTPS access, visible canonical source links, the complete article index, official release URLs and the final ISO checksum. It also verifies every frozen M4.5 evidence hash. Use the import payload matching the publication being checked; `--payload` selects another saved payload. Reports belong under `evidence/documentation/`, separate from OS runtime qualification.

`website/brix/build.mjs` reads committed Markdown and GitHub release metadata and creates `build/brix/site-content.json`. Every manual becomes an ordinary article with native heading, rich-text, code, image and divider Brix. Generated articles provide the documentation index and release downloads/checksums. Relative guide links resolve to native articles; source files and exact heading anchors retain canonical GitHub links. Each article identifies its source path and commit.

Run `node website/brix/build.mjs`. The converter uses the retained MIT-licensed Marked module in `website/public/vendor/`; upstream notices remain alongside it. Import through **Tools > Import content**. The native importer validates the post schema and merges new posts/categories by slug without overwriting existing posts. Subsequent automatic updates therefore need the content API rather than repeated imports.

## Automatic updates

`website/brix/sync.mjs` uses the existing BrixCanvas content API, restricted to this site's `read_content` and `write_content` scopes. It checks the expected site slug/name before writing, validates every slug ownership marker before the first mutation, preserves unrelated articles and sends at most one request per second. New content is created as a clearly marked empty draft and then filled with native blocks. A failed draft is recoverable only if that exact marker and empty body still match. Removed source documents become drafts; the publisher never deletes them. Every generated article is read back and verified.

The active `.github/workflows/brixcanvas-docs.yml` workflow runs on relevant `main` pushes, release publication/edits and manual dispatch. Repository variable `BRIXCANVAS_ENGINEOS_SYNC_ENABLED` is `true`; the site-specific credential is stored only as encrypted repository secret `BRIXCANVAS_ENGINEOS_KEY`. The owner approved creating this content-only key on 2026-11-06. The first real [GitHub publication](https://github.com/enginsoysal/codename-engine-os/actions/runs/42209026210) updated and read back all 55 articles successfully. See [workflow evidence](../evidence/documentation/github-sync-results.json). The key must never enter source, logs or public artifacts. Disable future synchronization by setting the variable to `false`; revoke access through this site's Integrations panel. Run `node --test website/brix/sync.test.mjs` and `node website/brix/sync.mjs --dry-run` without a credential to validate preparation.

Edit generated documentation in the canonical GitHub files. Local edits to generated article bodies will be replaced on synchronization; manually authored articles without the Engine OS ownership marker remain untouched. Page design, menus, site access and unrelated app settings are outside the publisher's scope. Public rendering can retain the previous cached article for up to 60 seconds after an API write; verification reads the native API directly.

Documentation and binaries remain canonical on GitHub. Native articles are generated copies and identify their source commit. Downloads link directly to official release assets. Original ISO files, immutable tags and frozen M45-VM-v1 acceptance evidence are unchanged. M4.5 and the earlier installation releases remain historical. M7.1 was accepted within M71-VM-v1; M7.3 is the current visual/HiDPI follow-up; original DOS/Windows execution remains M8. Native articles and the download index synchronize automatically, including the M7.1 installation, partition-management and recovery tutorials with exact checksums. The native homepage composition is updated alongside releases while retaining the preferred design; editable snapshot and guide links identify the current installer. [M7.1 homepage publication evidence](../evidence/documentation/m71-homepage-results.json) records its observed status. Browser edits use only the owner-authorized Edge extension or agent-owned browser; no host desktop takeover or new publishing permission is used. Historical articles/downloads remain available.

## Correction of the earlier hosting approach

The initial implementation incorrectly used separate Sites/Azure hosting. The owner explicitly corrected this: build and publish in BrixCanvas. The separate GitHub deployment workflow is disabled and removed; both affected Front Door routes are restored to their original `securityRelay` rule set. Existing BrixCanvas routing/TLS is the production path. Do not reinstate separate hosting or change unrelated application routes.

`website/public/`, `website/build.py` and `website/check.py` are retained prototype artifacts, not the production BrixCanvas site. Earlier reports under `evidence/documentation/` describe that historical approach only. Keep new native site evidence separate from those reports and frozen OS acceptance evidence.

Use an agent-owned browser or the Edge extension for browser work. The owner explicitly prohibits taking over the desktop, mouse or keyboard without separate permission.

## Historical M7.2 delivery

Current guides focus on the redesigned installer, immediate real persistent Files desktop and the prepared VMware ZIP. Native downloads prefer m72-v0.7.2 and keep historical files/checksums visible. Home retains the preferred editable native Brix design, with M7.2 setup/recovery links and explicit CD-versus-SATA restart guidance. The content publisher handles committed English Markdown and release assets automatically; homepage design still requires an authorized native Brix edit. New verification uses M72-VM-v1 acceptance and preserves prior proof. [Current tutorial](../docs/handbook/install-m72.md), [folder guide](../docs/handbook/desktop-m72.md), [recovery](../docs/handbook/recovery-m72.md).

## Historical M7.3 delivery

Native downloads prefer m73-v0.7.3, including the matching VMware ZIP, full notices and SHA256 values. All committed English Markdown, including the new installation/desktop/recovery and graphics/design review documents, synchronizes into native Brix articles. The homepage keeps the approved native composition and points to M7.3 setup/recovery. Its manual native Brix edit follows the content-sync jobs so optimistic site versions do not conflict. New public readback verifies every canonical source commit and frozen M73-VM-v1 proof; earlier release assets/proofs remain unchanged. [Current guide](../docs/handbook/install-m73.md), [desktop](../docs/handbook/desktop-m73.md), [recovery](../docs/handbook/recovery-m73.md).

## M7.4 application delivery

Native downloads prefer m74-v0.7.4 and its prepared VMware ZIP. English
installation/app instructions, format limits, package ownership, resource
figures and exact acceptance synchronize automatically from GitHub. An M7.4
homepage snapshot and app-guide card retain the approved native composition
in `website/brix/design.mjs`; native publication/readback is recorded separately
from article synchronization. [Install](../docs/handbook/install-m74.md),
[apps](../docs/handbook/apps-m74.md), [release](../docs/releases/m74-v0.7.4.md).

Publication readback: both M7.4 GitHub content-sync jobs succeeded. All 33
public release asset digests and sizes match the local qualified artifacts; see
[publication proof](../evidence/documentation/m74-release-publication.json).
The public homepage still shows M7.3: the native editor rejected Save twice
with an explicit save-error message. M7.4 edits remain in the authorized Edge
editor for recovery. No successful homepage publication is claimed. Current
articles and downloads are independent of this homepage-only blocker.
