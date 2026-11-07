"""Check anonymous HTTPS access to native BrixCanvas guides and release assets."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
ORIGIN = "https://engineos.brixcanvas.com"
ISO_SHA = "774ea677c74a7d593fb392160833a8f70c42af69a54a2b44f47b4bbc401ddc03"


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.skip = 0
        self.parts = []
        self.links = []
        self.lang = None
        self.iframes = 0

    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        if tag in {"script", "style"}:
            self.skip += 1
        if tag == "html":
            self.lang = attrs.get("lang")
        if tag == "a":
            self.links.append(attrs.get("href", ""))
        if tag == "iframe":
            self.iframes += 1

    def handle_endtag(self, tag):
        if tag in {"script", "style"}:
            self.skip -= 1

    def handle_data(self, data):
        if not self.skip:
            self.parts.append(data)

    @property
    def text(self):
        return " ".join(" ".join(self.parts).split())


def get(route):
    with urlopen(Request(ORIGIN + route, headers={"User-Agent": "EngineOS-documentation-check/1.0"}), timeout=30) as response:
        assert response.status == 200, route
        page = Page()
        page.feed(response.read().decode("utf-8"))
        assert page.lang and page.lang.startswith("en"), f"Non-English document language: {route}"
        assert not page.iframes, f"An external website was embedded: {route}"
        return page


def verify(payload, homepage_release="M7.1"):
    ref = payload["source_commit"]
    home = get("/")
    for text in ["From first boot.", "To what comes next.", "PROJECT SNAPSHOT / X86-64", "Run it in VMware", "Build your own distro.", "Repair and update", "Before you install "+homepage_release]:
        assert text in home.text, f"Missing preferred native design: {text}"
    for fiction in ["Driftlab", "Northwind", "Book now", "Tidemark"]:
        assert fiction not in home.text, f"Template fiction remains: {fiction}"
    home_guide="m71" if homepage_release=="M7.1" else homepage_release.lower()
    for slug in ["docs-handbook-install-"+home_guide, "docs-handbook-distributions", "docs-handbook-recovery-"+home_guide]:
        assert f"{ORIGIN}/blog/{slug}" in home.links, f"Stale homepage guide: {slug}"
    m71 = json.loads((ROOT / "evidence/M7.1/acceptance-results.json").read_text(encoding="utf-8"))
    assert m71["passed"] and m71["scope_id"] == "M71-VM-v1"
    for relative, digest in m71["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M7.1" / relative).read_bytes()).hexdigest() == digest, relative
    m7 = json.loads((ROOT / "evidence/M7/acceptance-results.json").read_text(encoding="utf-8"))
    assert m7["passed"] and m7["scope_id"] == "M7-VM-v1"
    for relative, digest in m7["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M7" / relative).read_bytes()).hexdigest() == digest, relative
    m6 = json.loads((ROOT / "evidence/M6/acceptance-results.json").read_text(encoding="utf-8"))
    assert m6["passed"] and m6["scope_id"] == "M6-VM-v1"
    for relative, digest in m6["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M6" / relative).read_bytes()).hexdigest() == digest, relative
    m51 = json.loads((ROOT / "evidence/M5.1/acceptance-results.json").read_text(encoding="utf-8"))
    assert m51["passed"] and m51["full_m51_complete"]
    for relative, digest in m51["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M5.1" / relative).read_bytes()).hexdigest() == digest, relative
    m5 = json.loads((ROOT / "evidence/M5/acceptance-results.json").read_text(encoding="utf-8"))
    assert m5["passed"] and m5["full_m5_complete"] and m5["scope_id"] == "M5-VM-v1"
    for relative, digest in m5["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M5" / relative).read_bytes()).hexdigest() == digest, relative

    def check_article(post):
        route = "/blog/" + post["slug"]
        page = get(route)
        assert post["title"] in page.text, f"Missing article title: {route}"
        marker = next(m["value"] for m in post["meta"] if m["key"] == "engine_source_path")
        if marker.endswith(".md"):
            assert "Canonical source:" in page.text, f"Native article body missing: {route}"
            expected = f"https://github.com/enginsoysal/codename-engine-os/blob/{ref}/{marker}"
            assert expected in page.links, f"Wrong source commit: {route}"
        if post["slug"] == "downloads":
            assert m71["iso_sha256"] in page.text, "Current M7.1 ISO checksum is not visible"
            assert m7["iso_sha256"] in page.text, "Current M7 ISO checksum is not visible"
            assert m6["iso_sha256"] in page.text, "Current M6 ISO checksum is not visible"
            assert ISO_SHA in page.text, "Final ISO checksum is not visible"
            assert m51["iso_sha256"] in page.text, "Current M5.1 ISO checksum is not visible"
            assert m5["iso_sha256"] in page.text, "Current M5 ISO checksum is not visible"
            expected_links = Page()
            for block in post["blocks"]:
                expected_links.feed(block.get("props", {}).get("html", ""))
            assets = {u for u in expected_links.links if "/releases/download/" in u}
            assert assets and assets <= set(page.links), "A release download is missing"
        if post["slug"] == "documentation":
            for other in payload["page"]["blog"]["posts"]:
                if other["slug"] != "documentation":
                    assert f"{ORIGIN}/blog/{other['slug']}" in page.links, f"Guide absent from index: {other['slug']}"
        return {"route": route, "passed": True, "visible_characters": len(page.text), "links": len(page.links)}

    with ThreadPoolExecutor(max_workers=4) as workers:
        articles = list(workers.map(check_article, payload["page"]["blog"]["posts"]))
    acceptance = json.loads((ROOT / "evidence/M4.5/acceptance-results.json").read_text(encoding="utf-8"))
    assert acceptance["passed"] and acceptance["full_m45_complete"] and acceptance["scope_id"] == "M45-VM-v1"
    for relative, digest in acceptance["evidence_sha256"].items():
        assert hashlib.sha256((ROOT / "evidence/M4.5" / relative).read_bytes()).hexdigest() == digest, relative
    return {"passed": True, "checked_at": datetime.now(timezone.utc).isoformat(), "website": ORIGIN,
            "native_articles": len(articles), "homepage_release": homepage_release, "homepage_update_pending": homepage_release != "M7.1", "m71_scope": m71["scope_id"], "m71_iso_sha256": m71["iso_sha256"], "m7_scope": m7["scope_id"], "m7_iso_sha256": m7["iso_sha256"], "article_source_commit": ref, "english_html_language": True,
            "external_website_iframes": 0, "anonymous_https": True, "preferred_homepage_design": True,
            "m45_scope": "M45-VM-v1", "frozen_evidence_hashes_verified": len(acceptance["evidence_sha256"]),
            "m6_scope": m6["scope_id"], "m6_iso_sha256": m6["iso_sha256"], "m6_evidence_hashes_verified": len(m6["evidence_sha256"]),
            "m45_iso_sha256": ISO_SHA, "m5_iso_sha256": m5["iso_sha256"], "m5_scope": "M5-VM-v1",
            "m51_iso_sha256": m51["iso_sha256"], "m51_evidence_hashes_verified": len(m51["evidence_sha256"]),
            "m5_evidence_hashes_verified": len(m5["evidence_sha256"]),
            "automatic_sync_evidence": "evidence/documentation/github-sync-results.json",
            "articles": articles}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload", type=Path, default=ROOT / "build/brix/site-content.json")
    parser.add_argument("--output", type=Path, default=ROOT / "evidence/documentation/native-brix-results.json")
    parser.add_argument("--homepage-release", choices=["M6", "M7", "M7.1"], default="M7.1", help="Explicitly record a historical homepage while verifying current articles")
    args = parser.parse_args()
    result = verify(json.loads(args.payload.read_text(encoding="utf-8")), args.homepage_release)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
    print(f"Native BrixCanvas PASS: {result['native_articles']} public English articles; preferred design; all M4.5 evidence unchanged")
