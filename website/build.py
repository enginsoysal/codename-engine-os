"""Build the documentation Site without changing kernel qualification evidence."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT).decode("utf-8")


def build(destination):
    destination = Path(destination).resolve()
    if destination == ROOT or ROOT in destination.parents and destination.parts[-1] != "site":
        raise ValueError("Use an external Site dist directory or build/site")
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / "website/public", destination, dirs_exist_ok=True)
    ref = git("rev-parse", "HEAD").strip()
    paths = git("ls-tree", "-r", "--name-only", ref).splitlines()
    docs = {p: git("show", f"{ref}:{p}") for p in paths
            if p.endswith(".md") and (p.startswith("docs/") or p in
                {"README.md", "CONTRIBUTING.md", "SECURITY.md", "AGENTS.md"})}
    titles = {p: next((line[2:] for line in text.splitlines() if line.startswith("# ")), p)
              for p, text in docs.items()}
    releases = json.loads(subprocess.check_output([
        "gh", "api", "repos/enginsoysal/codename-engine-os/releases?per_page=100"
    ], cwd=ROOT).decode("utf-8"))
    # Store public release metadata only; never copy API authorization or local paths.
    fields = ("id", "tag_name", "name", "html_url", "published_at", "prerelease", "draft")
    asset_fields = ("name", "size", "content_type", "digest", "browser_download_url")
    releases = [{**{k: r.get(k) for k in fields}, "assets": [
        {k: a.get(k) for k in asset_fields} for a in r["assets"]
    ]} for r in releases if not r["draft"]]
    data = {"ref": ref, "generated": datetime.now(timezone.utc).isoformat(),
            "titles": titles, "docs": docs, "releases": releases}
    (destination / "snapshot.json").write_bytes(json.dumps(data, ensure_ascii=False).encode("utf-8"))
    print(json.dumps({"documents": len(docs), "releases": len(releases), "source_commit": ref}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default=str(ROOT / "build/site"))
    build(parser.parse_args().output)
