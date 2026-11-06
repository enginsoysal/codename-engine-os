# Documentation website

Public destination: https://codenameenginos.brixcanvas.com . This website belongs to Engine OS; its source is independent of the unrelated BrixCanvas application.

## Source and automatic updates

`website/public/` contains a small static documentation reader. On each page load or **Refresh from GitHub**, it requests the public repository tree and published releases. It reads Markdown at the returned immutable commit SHA, so one browsing session uses a consistent documentation version. Reload or refresh to see new commits and releases; no scheduled job, deployment secret or manual documentation copy is required. The version selector reads documentation at a release tag. Search reads the full text with four concurrent requests at most.

Every tracked Markdown page under `docs/`, plus the root project guides, appears in the navigation automatically. JSON contracts, CSV inventories, evidence, images and source links resolve to GitHub at the selected commit. Downloads link directly to GitHub release assets, including checksums and API-provided SHA256 digests. Preview releases are labeled separately from stable releases.

The browser uses unauthenticated GitHub APIs, which have rate limits and require network access. A timestamped bundled snapshot provides documentation and download metadata during an outage. The page explicitly labels this fallback; it never presents that copy as freshly synchronized. The release list independently reports fallback if only that request fails. Visitors can always follow source and release links to GitHub. Offline browsing requires a prior successful load of the Site assets; this is not an installed offline app.

## Build and deployment

Run `python website/build.py --output build/site` from the repository. The script copies the static frontend and creates a public snapshot from committed Git documentation and GitHub release metadata. It requires Python, Git and authenticated `gh` for the build only. Visitors need no account or credentials.

Hosting uses a separate Sites source checkout with `.openai/hosting.json`; it is not a nested Git repository here. Copy `website/public/` and the generated snapshot into that checkout's `dist/`, then use the Sites source helper to commit, push, package and deploy the exact source state. The frontend is maintained in this public repository; the hosting checkout is a deployment copy. Redeploy after frontend changes. Documentation and release changes alone are picked up automatically on reload without redeploying.

The hostname's existing DNS and wildcard TLS terminate at Azure Front Door. An exact-host condition routes only this documentation hostname to its own public Sites origin. Preserve unrelated origin groups, route settings and rule sets. No OS code, website code or files belong in the BrixCanvas application checkout. Document actual routing and verification evidence separately from kernel acceptance.

## Dependencies and safety

The vendored Marked 18.1.3 renderer and DOMPurify 3.4.17 sanitizer retain upstream license files in `website/public/vendor/`. They originate from their versioned npm packages; see `THIRD_PARTY.json` for package tarball digests. Markdown is sanitized before insertion; relative links resolve against the selected document and commit. External schemes other than HTTPS/HTTP/mailto are rejected. No GitHub tokens are shipped to visitors. Fonts use Google Fonts with local system fallbacks.

Website checks and deployment reports belong under `evidence/documentation/`. A website change does not alter the M45-VM-v1 runtime claim or rewrite the frozen M4.5 acceptance artifacts. M4.5 remains complete within that scope. Installer/partition creation remain M5 work.
