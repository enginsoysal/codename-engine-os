# Pinned public CA data

`cacert.pem` is the unchanged 5 November 2026 Mozilla root bundle distributed by curl under MPL-2.0. Its downloaded SHA-256 matches the separately fetched upstream digest; exact URLs/hash are in `provenance.json`. `LICENSE.txt` retains the Mozilla license. This data is separately licensed from original MIT Engine OS code.

`tools/build/trust7.py` uses the project-pinned cryptography package to extract CA subject names and RSA/EC public keys into generated immutable BearSSL trust anchors. No private/test root is added to the production bundle. Builds verify the pinned source digest and refuse unsupported key types rather than silently omitting roots.

The PEM conversion does not carry Firefox-specific external name constraints or browser policy. The [upstream explanation](https://curl.se/docs/caextract.html) describes that limitation. Engine OS does not claim full browser trust-policy equivalence, revocation/CT enforcement, or automatic trust-store updates. A later root update requires a reviewed pinned-data change and requalification. TLS runtime is still under development; these files alone do not establish secure downloading.
