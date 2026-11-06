# BrixCanvas documentation website

The Engine OS website belongs inside BrixCanvas. The requested public address is **https://engineos.brixcanvas.com**. The shorter address was selected from the owner's options on 2026-11-06. OS sources remain in this independent repository; website content uses ordinary editable BrixCanvas Brix and articles.

## Native website

The BrixCanvas project is `Codename: Engine OS`, site ID `b66d0e83-7987-4856-b702-e8d93234b6cd`, initial preview slug `codename-engine-os-ltvck`. The homepage uses native hero, button, text, heading and alert Brix. Documentation belongs to the native Blog content model, grouped by handbook, roadmap, architecture, milestones, releases, developer guides and evidence. No external website is the production origin.

`website/brix/build.mjs` reads committed Markdown and GitHub release metadata and creates `build/brix/site-content.json`. Every manual becomes an ordinary article with native heading, rich-text, code, image and divider Brix. Generated articles provide the documentation index and release downloads/checksums. Relative guide links resolve to native articles; source files and exact heading anchors retain canonical GitHub links. Each article identifies its source path and commit.

Run `node website/brix/build.mjs`. The converter uses the retained MIT-licensed Marked module in `website/public/vendor/`; upstream notices remain alongside it. Import through **Tools > Import content**. The native importer validates the post schema and merges new posts/categories by slug without overwriting existing posts. Subsequent automatic updates therefore need the content API rather than repeated imports.

## Automatic updates

The intended publisher uses the existing BrixCanvas content API, restricted to this site's `read_content` and `write_content` scopes. It must update only generated articles identified by Engine OS source metadata, preserve unrelated owner content, serialize requests and verify resulting articles. Keep its key in an encrypted GitHub Actions secret, never source, browser code, logs or public artifacts. Initial import and review can proceed before the credential is configured. Do not claim automatic synchronization is active until a real GitHub-triggered update succeeds.

Documentation and binaries remain canonical on GitHub. Native articles are generated copies and identify their source commit. Downloads link directly to official release assets. Original ISO files, immutable tags and frozen M45-VM-v1 acceptance evidence are unchanged. M4.5 remains complete within that VM scope; installation and partition creation remain M5 work.

## Correction of the earlier hosting approach

The initial implementation incorrectly used separate Sites/Azure hosting. The owner explicitly corrected this: build and publish in BrixCanvas. The separate GitHub deployment workflow is disabled and removed; both affected Front Door routes are restored to their original `securityRelay` rule set. Existing BrixCanvas routing/TLS is the production path. Do not reinstate separate hosting or change unrelated application routes.

`website/public/`, `website/build.py` and `website/check.py` are retained prototype artifacts, not the production BrixCanvas site. Earlier reports under `evidence/documentation/` describe that historical approach only. Keep new native site evidence separate from those reports and frozen OS acceptance evidence.

Use an agent-owned browser or the Edge extension for browser work. The owner explicitly prohibits taking over the desktop, mouse or keyboard without separate permission.
