# BrixCanvas documentation website

The Engacer OS website belongs inside BrixCanvas. The native dashboard saved the owner-selected name and **https://engaceros.brixcanvas.com** on 2026-11-10. The internal site ID/slug and scoped publisher key stay unchanged; historical release identities stay intact. M11.4 windows/appearance is the current separately scoped stage delivery. Native homepage Save, responsive review, current article readback and exact download-hash verification are required separately for every publication. OS sources remain in this independent repository; website content uses ordinary editable BrixCanvas Brix and articles.

## Native website

### Historical M10 publication — 10 November 2026

The Engacer OS M10 homepage uses the four editable native sections from
`website/brix/design.mjs`, the canonical engaceros.brixcanvas.com route and the
renamed enginsoysal/engacer-os repository. Current setup, Devices/USB tasks and
later hardware limits are linked directly. English Markdown articles and official
GitHub downloads are synchronized automatically, with per-article source commits.
The public check is `python tests/m10/website.py`; publication proof belongs under
`evidence/documentation/homepage-m10`, separate from runtime qualification.

### Historical M9.2 homepage follow-up

The historical M9.2 native homepage was **published and verified**, retaining the
preferred layout. The historical composition recorded the matching English copy:
current VMware setup, supported controller scope, display/resource evidence,
explicit hardware/application limits and correct installation/restart guidance.
Fresh public readback contains M9.2 and no M7.3 homepage reference. The M9.2
manual and official ISO/VMware ZIP links and complete hashes were checked.
All 145 native articles remain present (143 source articles plus the two
generated index/download articles). Desktop and narrow mobile layouts were
reviewed; measured document width equals viewport width without horizontal
overflow, and the mobile menu exposes the main navigation.

The previous Save failure is resolved. The editor sends the native blog data
with its canvas, exceeding the framework's default 1 MiB action-body cap. The
owner-approved bounded BrixCanvas correction retains the live application and
raises only that cap to 4 MiB. A fresh native Save sent 1,278,039 characters
with all 145 posts, received HTTP 200 and displayed **Saved ✓**. No article was
removed and no credential or separate hosting workaround was used. See
[follow-up evidence](../evidence/documentation/homepage-m92-followup.json).

The BrixCanvas project is `Engacer OS` (historically `Codename: Engine OS`), site ID `b66d0e83-7987-4856-b702-e8d93234b6cd`, initial preview slug `codename-engine-os-ltvck`. The homepage uses native Menu, Layout, Button, Text, Heading, Code and Alert Brix. Documentation belongs to the native Blog content model, grouped by handbook, roadmap, architecture, milestones, releases, developer guides and evidence. Native site-wide search reads article body text, titles and tags. No external website is the production origin.

The owner preferred the initial local prototype's appearance over the generated BrixCanvas template. `website/brix/design.mjs` records the replacement native composition: navy Menu Brix, light status strip, two-column introduction/terminal, three guide cards, scope notice and minimal footer. The layout uses ordinary nested Layout, Heading, Text, Code, Button and Alert Brix, not an HTML embed. Import individual sections through BrixCanvas's native **Paste Brix** operation using its `brixcanvas: 1` clipboard format; all children remain editable. The site uses the native minimal/Inter font palette and English public locale.

`python website/brix/check.py` checks anonymous HTTPS access, visible canonical source links, the complete article index, official release URLs and the final ISO checksum. It also verifies every frozen M4.5 evidence hash. Use the import payload matching the publication being checked; `--payload` selects another saved payload. Reports belong under `evidence/documentation/`, separate from OS runtime qualification.

`website/brix/build.mjs` reads committed Markdown and GitHub release metadata and creates `build/brix/site-content.json`. Every manual becomes an ordinary article with native heading, rich-text, code, image and divider Brix. Generated articles provide the documentation index and release downloads/checksums. Relative guide links resolve to native articles; source files and exact heading anchors retain canonical GitHub links. Each article identifies its source path and commit.

Run `node website/brix/build.mjs`. The converter uses the retained MIT-licensed Marked module in `website/public/vendor/`; upstream notices remain alongside it. Import through **Tools > Import content**. The native importer validates the post schema and merges new posts/categories by slug without overwriting existing posts. Subsequent automatic updates therefore need the content API rather than repeated imports.

## Brand and link audit

Current website navigation, article titles, authors, tags and source links use
**Engacer OS**, `engaceros.brixcanvas.com` and `enginsoysal/engacer-os`.
The publisher normalizes legacy website/repository URLs while preserving the
exact path, release tag, asset filename, source commit and fragment. Older guide
titles use the new brand with a historical label and a visible explanation;
original instructions and immutable evidence retain their original screen labels
and binary names. This avoids presenting an old installer as the current release.

Run `node --test website/brix/branding.test.mjs website/brix/sync.test.mjs` before
publication. `python -X utf8 website/brix/audit.py` crawls every generated article
and discovered public navigation page, records metadata and old-name contexts,
and checks canonical links. Its default reports go to ignored build outputs;
accepted OS proof is never overwritten by a website audit.
`python -X utf8 website/brix/audit_links.py --input build/brix/audit.json --output build/brix/link-audit.json` validates the observed repository objects, live release assets and external HTTP references. HTTP 308 redirects are followed on the pinned Python 3.10 host; redirect loops and missing pages remain failures. Vendor access refusals/timeouts are recorded as unverified, not mislabeled as working or broken. Share/message actions are never submitted. Public audit reports
belong in a new folder under `evidence/documentation/`.

## Automatic updates

`website/brix/sync.mjs` uses the existing BrixCanvas content API, restricted to this site's `read_content` and `write_content` scopes. It checks the expected site slug/name before writing, validates every slug ownership marker before the first mutation, preserves unrelated articles and sends at most one request per second. New content is created as a clearly marked empty draft and then filled with native blocks. A failed draft is recoverable only if that exact marker and empty body still match. Removed source documents become drafts; the publisher never deletes them. Every generated article is read back and verified.

The active `.github/workflows/brixcanvas-docs.yml` workflow runs on relevant `main` pushes, release publication/edits and manual dispatch. Repository variable `BRIXCANVAS_ENGINEOS_SYNC_ENABLED` is `true`; the site-specific credential is stored only as encrypted repository secret `BRIXCANVAS_ENGINEOS_KEY`. The owner approved creating this content-only key on 2026-11-06. The first real [GitHub publication](https://github.com/enginsoysal/engacer-os/actions/runs/42209026210) updated and read back all 55 articles successfully. See [workflow evidence](../evidence/documentation/github-sync-results.json). The key must never enter source, logs or public artifacts. Disable future synchronization by setting the variable to `false`; revoke access through this site's Integrations panel. Run `node --test website/brix/sync.test.mjs` and `node website/brix/sync.mjs --dry-run` without a credential to validate preparation.

Edit generated documentation in the canonical GitHub files. Local edits to generated article bodies will be replaced on synchronization; manually authored articles without the stable publisher ownership marker remain untouched. Page design, menus, site access and unrelated app settings are outside the publisher's scope. Public rendering can retain the previous cached article for up to 60 seconds after an API write; verification reads the native API directly.

Documentation and binaries remain canonical on GitHub. Native articles identify
their source commit and downloads link directly to official release assets.
M10 is the earlier qualified phase-1/2 virtual-device release; M11.4 is the new
window/appearance stage delivery, with its own test scope. Broader hardware and
general DOS/Windows compatibility remain open. Accepted releases, tags, hashes
and OS evidence are unchanged. Article/download updates are automatic; homepage
editing is a separate native Brix operation. Browser work uses only the
owner-authorized Edge extension or agent-owned browser. Historical guides,
downloads and publication evidence remain available.

### Website release acceptance

1. Push canonical documentation and verify the content-sync job succeeds.
2. Update the native homepage snapshot, current guide links and scope notice
   from the latest site version. Article synchronization does not perform this step.
3. Confirm native Save succeeds, then inspect a fresh public page, current
   guide/download destinations, exact hashes, article retention and responsive layout.
4. Record website evidence separately from OS qualification. A failed Save or
   an article-only sync must never be reported as a completed homepage update.

## Correction of the earlier hosting approach

The initial implementation incorrectly used separate Sites/Azure hosting. The owner explicitly corrected this: build and publish in BrixCanvas. The separate GitHub deployment workflow is disabled and removed; both affected Front Door routes are restored to their original `securityRelay` rule set. Existing BrixCanvas routing/TLS is the production path. Do not reinstate separate hosting or change unrelated application routes.

`website/public/`, `website/build.py` and `website/check.py` are retained prototype artifacts, not the production BrixCanvas site. Earlier reports under `evidence/documentation/` describe that historical approach only. Keep new native site evidence separate from those reports and frozen OS acceptance evidence.

Use an agent-owned browser or the Edge extension for browser work. The owner explicitly prohibits taking over the desktop, mouse or keyboard without separate permission.

## Historical M7.2 delivery

Current guides focus on the redesigned installer, immediate real persistent Files desktop and the prepared VMware ZIP. Native downloads prefer m72-v0.7.2 and keep historical files/checksums visible. Home retains the preferred editable native Brix design, with M7.2 setup/recovery links and explicit CD-versus-SATA restart guidance. The content publisher handles committed English Markdown and release assets automatically; homepage design still requires an authorized native Brix edit. New verification uses M72-VM-v1 acceptance and preserves prior proof. [Current tutorial](../docs/handbook/install-m72.md), [folder guide](../docs/handbook/desktop-m72.md), [recovery](../docs/handbook/recovery-m72.md).

## Historical M7.3 delivery

Native downloads prefer m73-v0.7.3, including the matching VMware ZIP, full notices and SHA256 values. All committed English Markdown, including the new installation/desktop/recovery and graphics/design review documents, synchronizes into native Brix articles. The homepage keeps the approved native composition and points to M7.3 setup/recovery. Its manual native Brix edit follows the content-sync jobs so optimistic site versions do not conflict. New public readback verifies every canonical source commit and frozen M73-VM-v1 proof; earlier release assets/proofs remain unchanged. [Current guide](../docs/handbook/install-m73.md), [desktop](../docs/handbook/desktop-m73.md), [recovery](../docs/handbook/recovery-m73.md).

## Historical M7.4 application delivery

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
At that delivery the public homepage remained at M7.3: the native editor
rejected Save twice. Edits were retained for recovery and no successful
homepage publication was claimed. The 10 November follow-up above resolves
this historical homepage-only blocker separately from article synchronization.

## M7.5 installation follow-up

Native downloads prefer m75-v0.7.5 when the release is published. The updated
English installer manual, seven-step reference review, exact acceptance and
restart guidance synchronize from GitHub. The VMware ZIP instruction now ends
with Restart to Engine OS; matching media may stay connected. Homepage editing
is separate from article publication; no new successful homepage edit is claimed.
[Current installation guide](../docs/handbook/install-m75.md),
[release](../docs/releases/m75-v0.7.5.md).

## M8 first original Windows application

Native downloads prefer `m8-v0.8.0` after publication, with its tested VMware ZIP,
ISO, hashes, notices and original 7-Zip corresponding source. Committed English
installation, app, architecture and recovery instructions synchronize through
the existing content-only GitHub Actions publisher. The downloads article states
the exact 7-Zip console profile and its limitations. Existing-ZIP update is not
qualified; sample tasks choose fresh output names. Homepage composition remains
a separate native Brix edit; article synchronization does not prove a homepage
redesign. [Installation](../docs/handbook/install-m8.md),
[Windows app guide](../docs/handbook/windows-apps-m8.md),
[release](../docs/releases/m8-v0.8.0.md).


## M9 graphics-first delivery

After publication, downloads prefer `m9-v0.9.0` and its matching ISO/VMware ZIP.
The current English installation guide explains automatic supported VMware
graphics/pointer binding, RAM-buffer accounting, firmware-resolution limits and
the retained original Windows console profile. Broader M9 hardware support
remains in progress. Article/download synchronization does not edit the native
BrixCanvas homepage. [Guide](../docs/handbook/install-m9.md),
[scope](../docs/milestones/m9.md), [release](../docs/releases/m9-v0.9.0.md).

## M11 reader navigation

The public homepage leads with the latest completed M11 stage and a clearly
labeled native VM capture. The owner's M11.4 clarification requires a visible
GitHub stage release and matching test media after each remaining stage, with
its actual qualification. The downloads selector requires the matching ISO,
VMware ZIP and build manifest before promoting a stage. Full final regression
remains M11.6. Update the homepage after each ordered stage; a GitHub
article sync alone does not change the native homepage sections.

The Documentation index is ordered by reader task: Start here, current M11
work, daily use/customization, architecture/development, roadmap/evidence,
website maintenance, then the historical archive. Curated current installation
and Files guides come before technical and historical material. Numeric titles
sort naturally and every generated article appears exactly once. The publisher's
navigation tests preserve this order and keep uncategorized references visible.

M11.4 changes the full homepage image/feature card to native windows/Appearance,
links its current installation guide and release, labels M10 as historical and
keeps Files/Start/windows user guides before technical and archived articles.
The publisher tests cover release-asset completeness and source ownership.
Record the successful Actions sync, native Save, fresh anonymous home/guide/index/
downloads readback, image readiness and official asset SHA256 readback under
`evidence/M11/4` before proceeding to M11.5. Never save a stale editor blog
snapshot after CI: reload the editor after synchronization first.
