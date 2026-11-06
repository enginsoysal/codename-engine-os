# Documentation website

Public destination: https://codenameenginos.brixcanvas.com . This website belongs to Engine OS; its source is independent of the unrelated BrixCanvas application.

## Source and automatic updates

`website/public/` contains a small static documentation reader. On each page load or **Refresh from GitHub**, it requests the public repository tree and published releases. It reads Markdown at the returned immutable commit SHA, so one browsing session uses a consistent documentation version. Reload or refresh to see new commits and releases; no scheduled job, deployment secret or manual documentation copy is required. The version selector reads documentation at a release tag. Search reads the full text with four concurrent requests at most.

Every tracked Markdown page appears in the navigation automatically, grouped into handbook, roadmap, architecture, milestones, release guides, developer guides, test evidence, website maintenance and root project files. JSON contracts, CSV inventories, evidence, images and source links resolve to GitHub at the selected commit. Downloads link directly to GitHub release assets, including checksums and API-provided SHA256 digests. Preview releases are labeled separately from stable releases.

The browser uses unauthenticated GitHub APIs, which have rate limits and require network access. A timestamped bundled snapshot provides documentation and download metadata during an outage. The page explicitly labels this fallback; it never presents that copy as freshly synchronized. The release list independently reports fallback if only that request fails. Visitors can always follow source and release links to GitHub. Offline browsing requires a prior successful load of the Site assets; this is not an installed offline app.

## Build and deployment

Run `python website/build.py --output build/site` from the repository. The script copies the static frontend and creates a public snapshot from committed Git documentation and GitHub release metadata. It requires Python, Git and authenticated `gh` for the build only. Visitors need no account or credentials.

The static reader avoids a permanently running application server, database, synchronization worker and deployment token for documentation updates. A manually copied documentation tree would drift; a full server-rendered portal would add operational work without improving these public read-only guides. GitHub Pages was an alternative, but the existing DNS/TLS routing can serve a dedicated Azure static origin directly. None of these website services runs inside the Engine OS guest or changes its CPU/RAM requirements.

Production uses a dedicated Azure Storage static website in `rg-engineos-docs`, account `stengineosdocs46147764` (Standard LRS, West Europe). Public files are served through the static website endpoint; ordinary blob public access and shared-key authorization are disabled. The authorized publisher uses Microsoft Entra authentication with Storage Blob Data Contributor scoped to this account. The resources and content are separate from the BrixCanvas application.

The `Publish documentation website` GitHub Actions workflow automatically rebuilds and publishes on relevant documentation/frontend pushes to `main`, published/edited releases, or manual dispatch. It always checks out current `main`, pins its third-party actions to verified commit SHAs, serializes publication, checks JavaScript syntax, builds the fallback snapshot and compares uploaded bytes against that build. It verifies the dedicated origin immediately; custom-domain propagation is checked separately. Visitors also fetch live GitHub data without waiting for the workflow.

Azure authentication uses GitHub OIDC, not a saved client secret. `website/github-federation.json` records the non-secret issuer/subject/audience contract. The GitHub environment `engineos-docs` permits only `main` and milestone tags `m*`; the publishing service principal has Storage Blob Data Contributor on this dedicated account only. Three `ENGINEOS_DOCS_AZURE_*` repository variables hold non-secret client, tenant and subscription identifiers. Fork pull requests cannot run the publisher. No Front Door or unrelated application permissions are granted to this identity.

For an authorized manual recovery after building, publish with Azure CLI: `az storage blob upload-batch --account-name stengineosdocs46147764 --auth-mode login --destination '$web' --source build/site --overwrite true --content-cache-control 'public,max-age=0,must-revalidate'`. Use equivalent literal quoting in the host shell. The hosting endpoint is `stengineosdocs46147764.z6.web.core.windows.net`; index and error document are `index.html`.

The Sites skill was used to prepare and save the initial source version in an independent hosting checkout. Two deployment attempts of that same version failed with the provider error `no healthy upstream`. That unpublished Site is retained for diagnosis; it is not the production origin. Azure static hosting is the verified fallback. No private Site credentials or owner metadata are included here.

The hostname's existing DNS and wildcard TLS terminate at Azure Front Door. The `engineosdocs` rule set matches only `codenameenginos.brixcanvas.com` and overrides the origin group with the same name, forwarding over HTTPS with certificate-name validation. It stops rule processing for this hostname before unrelated application relay headers are added. The original `securityRelay` rule set remains after it for other hosts. Preserve unrelated origin groups, route settings and rule sets. No OS code, website code or files belong in the BrixCanvas application checkout. Document actual routing and verification evidence separately from kernel acceptance. [Azure routing override reference](https://learn.microsoft.com/en-us/azure/frontdoor/front-door-rules-engine-actions).

Run `python website/check.py` after publication. It compares every deployed static file byte-for-byte with `build/site`, checks the final M4.5 release metadata and ISO digest, and verifies all frozen M4.5 evidence hashes. The report is `evidence/documentation/website-results.json`; this is website verification, not a new OS runtime qualification. Browser checks cover search, relative navigation, release-tag selection, download listings, narrow-screen layout and explicit GitHub-outage fallback.

## Dependencies and safety

The vendored Marked 18.1.3 renderer and DOMPurify 3.4.17 sanitizer retain upstream license files in `website/public/vendor/`. They originate from their versioned npm packages; see `THIRD_PARTY.json` for package tarball digests. Markdown is sanitized before insertion; relative links resolve against the selected document and commit. External schemes other than HTTPS/HTTP/mailto are rejected. No GitHub tokens are shipped to visitors. Fonts use Google Fonts with local system fallbacks.

Website checks and deployment reports belong under `evidence/documentation/`. A website change does not alter the M45-VM-v1 runtime claim or rewrite the frozen M4.5 acceptance artifacts. M4.5 remains complete within that scope. Installer/partition creation remain M5 work.
