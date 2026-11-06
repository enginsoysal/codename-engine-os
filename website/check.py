"""Verify deployed static bytes, downloads and frozen M4.5 evidence."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]


def verify(base, directory):
    checks = []
    for file in sorted(directory.rglob("*")):
        if not file.is_file():
            continue
        relative = file.relative_to(directory).as_posix()
        with urlopen(Request(base.rstrip("/") + "/" + relative,
                             headers={"Cache-Control": "no-cache"}), timeout=30) as response:
            data = response.read()
            assert response.status == 200, relative
            assert data == file.read_bytes(), f"Deployed bytes differ: {relative}"
            checks.append({"path": relative, "status": response.status,
                           "sha256": hashlib.sha256(data).hexdigest(),
                           "content_type": response.headers.get("Content-Type")})
    snapshot = json.loads((directory / "snapshot.json").read_text(encoding="utf-8"))
    assert len(snapshot["docs"]) >= 53
    assert "docs/handbook/vmware-storage.md" in snapshot["docs"]
    release = next(r for r in snapshot["releases"] if r["tag_name"] == "m45-v0.4.5")
    assert not release["prerelease"]
    iso = next(a for a in release["assets"] if a["name"] == "EngineOS-M45.iso")
    assert iso["digest"] == "sha256:774ea677c74a7d593fb392160833a8f70c42af69a54a2b44f47b4bbc401ddc03"
    for asset in release["assets"]:
        assert asset["browser_download_url"].startswith(
            "https://github.com/enginsoysal/codename-engine-os/releases/download/m45-v0.4.5/")
    acceptance = json.loads((ROOT / "evidence/M4.5/acceptance-results.json").read_text(encoding="utf-8"))
    assert acceptance["passed"] and acceptance["full_m45_complete"]
    assert acceptance["scope_id"] == "M45-VM-v1"
    for path, digest in acceptance["evidence_sha256"].items():
        actual = hashlib.sha256((ROOT / "evidence/M4.5" / path).read_bytes()).hexdigest()
        assert actual == digest, f"Frozen evidence changed: {path}"
    return {"passed": True, "checked_at": datetime.now(timezone.utc).isoformat(),
            "url": base, "source_commit": subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=ROOT).decode().strip(),
            "snapshot_commit": snapshot["ref"], "documents": len(snapshot["docs"]),
            "releases": len(snapshot["releases"]), "http_checks": checks,
            "m45_evidence_hashes_verified": len(acceptance["evidence_sha256"]),
            "m45_scope": acceptance["scope_id"], "iso_digest": iso["digest"]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="https://codenameenginos.brixcanvas.com")
    parser.add_argument("--directory", type=Path, default=ROOT / "build/site")
    parser.add_argument("--output", type=Path, default=ROOT / "evidence/documentation/website-results.json")
    args = parser.parse_args()
    result = verify(args.url, args.directory)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
    print(f"Website PASS: {result['documents']} documents, {len(result['http_checks'])} exact HTTP assets; M4.5 evidence unchanged")
