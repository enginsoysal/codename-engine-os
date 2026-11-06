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


def verify(payload):
    ref = payload["source_commit"]
    home = get("/")
    for text in ["From first boot.", "To what comes next.", "PROJECT SNAPSHOT / X86-64", "Run it in VMware", "Where is my disk?", "Meet the console"]:
        assert text in home.text, f"Missing preferred native design: {text}"
    for fiction in ["Driftlab", "Northwind", "Book now", "Tidemark"]:
        assert fiction not in home.text, f"Template fiction remains: {fiction}"

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
            assert ISO_SHA in page.text, "Final ISO checksum is not visible"
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
            "native_articles": len(articles), "article_source_commit": ref, "english_html_language": True,
            "external_website_iframes": 0, "anonymous_https": True, "preferred_homepage_design": True,
            "m45_scope": "M45-VM-v1", "frozen_evidence_hashes_verified": len(acceptance["evidence_sha256"]),
            "iso_sha256": ISO_SHA, "automatic_sync_verified": False, "articles": articles}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload", type=Path, default=ROOT / "build/brix/site-content.json")
    parser.add_argument("--output", type=Path, default=ROOT / "evidence/documentation/native-brix-results.json")
    args = parser.parse_args()
    result = verify(json.loads(args.payload.read_text(encoding="utf-8")))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
    print(f"Native BrixCanvas PASS: {result['native_articles']} public English articles; preferred design; all M4.5 evidence unchanged")
