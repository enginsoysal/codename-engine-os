# Build your own Engine OS distribution

## M7.1 installer and desktop profiles

Build the current disk-manager media with `python tools/build/m71.py`. Standard uses `distros/engineos-disk.json`; the separately installable Night example uses `python tools/build/m71.py --revision night --profile distros/engineos-disk-night.json`. Both identify version 0.7.1 and retain the Engine OS name. The same implemented schema-2 appearance/optional-app fields below apply; disk consent, protected partitions and core checks cannot be removed by a profile. The final M7.1 custom gate qualifies real Night installation/defaults, Calculator omission/reinstall and restart persistence. Historical M7 profiles/artifacts remain unchanged.

## M7 native desktop profiles

M7 adds validated schema-2 desktop defaults. The standard profile is `distros/engineos-desktop.json`. The separate `distros/engineos-night.json` example retains the Engine OS name, uses dark windows, a lavender accent and a plum background, and leaves Calculator uninstalled by default. The offline package cache remains available for later installation. Build it with:

```powershell
python tools/build/m7.py --revision night --profile distros/engineos-night.json
```

This produces `dist/EngineOS-M7NIGHT.iso` and its own manifest. Fresh installation applies the actual desktop defaults; updating an existing installation does not silently remove an already installed optional app or overwrite saved personal preferences. The wizard permits changing Calculator and guide selection before disk writes.

Schema 2 retains the schema-1 identity/data fields and requires exactly one `desktop` object with boolean `dark` and `calculator`, a six-digit `wallpaper` colour, and a printable `tagline` of at most 48 characters. Accent, installer/distribution name, welcome text and selected data packages remain implemented fields. Unknown options, non-boolean toggles, arbitrary hooks and core-removal fields are rejected. Historical data-only builds refuse schema 2. Core boot/storage checks and TLS policy cannot be bypassed by a profile.

Host schema checks and native Night installation/defaults, absent Calculator, real Editor save, offline reinstall and detached restart are exercised in the [custom-profile report](../../evidence/M7/distribution/results.json). Final release identity remains governed by aggregate acceptance. [M7 status](../milestones/m7.md).

## M6 native installer distributions

M6 extends the recipe into its native wizard and installed volume: branding/accent, default guide selection and selected data files affect the real image. User template paths below `/Users/test/` map to the account selected in the wizard. The sample `distros/engineos-minimal.json` keeps the Engine OS identity and excludes guide data by default. Build it with `python tools/build/m6.py --revision minimal --profile distros/engineos-minimal.json`. The separate output is `dist/EngineOS-M6MINIMAL.iso`; this does not replace the main standard media.

The main build command is `python tools/build/m6.py`. Current schema-1 recipes remain data-only: no executable packages, arbitrary shell hooks, desktop themes or ignored runtime options. M6 bounds template target paths to leave room for the selected account name; original data files retain the recipe size bound. The base user volume supports much larger streamed files. The native executable-package/desktop component model belongs to M7 and later application work. Qualification of the actual custom ISO is part of M6; a validated JSON recipe alone is insufficient.

## Historical M5.1 recipes

The Forge example below belongs to the immutable M5.1 demonstration. The current product remains Engine OS; use Engine OS Minimal above for M6.

M5.1 provides working recipe-based ISO customization. It does not require changes to kernel code for branding, accent, welcome text or included data files. Both the example **Forge Community OS** and the standard edition are built and booted during qualification.

## Build the example

Use the repository's [pinned Windows build environment](../../tools/README.md), then run from the repository root:

```powershell
python tools/build/m51.py --profile distros/forge.json --validate-only
python tools/build/m51.py --profile distros/forge.json
```

Outputs: `dist/profiles/forge/EngineOS-forge-M51.iso`, USB image, kernel artifacts, `build-manifest.json` and `SHA256SUMS.txt`. Boot its ISO using the [same VMware setup](install-m51.md). It has its own name, orange accent and an additional `Developer.txt`. Those selected files are also placed in an actual installed system.

## Make your recipe

Copy `distros/forge.json` to a new file under `distros/`. Choose a unique lowercase ID; reserve `engineos` for the standard release. The strict version-1 fields are:

```json
{
  "schema": 1,
  "id": "my-edition",
  "name": "My Community OS",
  "version": "0.5.1",
  "welcome": "Welcome. Choose an option below.",
  "accent": "#44bbaa",
  "packages": ["welcome-guide"],
  "files": []
}
```

Name/version/welcome are printable ASCII with maximum lengths 32/16/76. Accent is a six-digit RGB colour. Package IDs name existing manifests under `distros/packages/<id>/package.json`. These are **data packages**, not installable Windows/Linux applications or a dependency-resolving package manager.

Add a file through `files`, for example:

```json
{"source":"packages/developer-notes/content.txt","target":"/Users/test/Documents/MyNotes.txt"}
```

Sources are relative to `distros/` and must resolve inside it. Additional targets must be inside `/Users/test/Documents/` or `/Data/Packages/`; no traversal, duplicate targets or host absolute paths. Maximum eight packages and 32 files, each at most 48 KiB. Add a data package by following the two included package manifests. No executable hooks or arbitrary shell commands are supported.

Build with `python tools/build/m51.py --profile distros/my-edition.json`. The output directory follows the profile ID, so keep IDs unique. The manifest records source/input/file SHA256 and build identity; the live and installed filesystem records the recipe at `/System/Distro/profile.json`. Changing selected profile content changes the build identity. Boot repair requires that same distribution/build ISO.

## What can be customized later

MIT source permits forks and component changes. Future desktop, compositor, shell, language, native executable packages and runtime customization need implemented contracts before becoming profile choices. Unknown fields/packages are rejected today, not silently accepted. Disk review, confirmation and recovery guards stay mandatory in every profile. [Product principles](../architecture/product-principles.md), [M5.1 contract](../milestones/m5.1.md).
