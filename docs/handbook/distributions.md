# Build your own Engine OS distribution

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

## M6 native installer distributions

M6 extends the recipe into its native wizard and installed volume: branding/accent, default guide selection and selected data files affect the real image. User template paths below `/Users/test/` map to the account selected in the wizard. The sample `distros/engineos-minimal.json` keeps the Engine OS identity and excludes guide data by default. Build it with `python tools/build/m6.py --revision minimal --profile distros/engineos-minimal.json`. The separate output is `dist/EngineOS-M6MINIMAL.iso`; this does not replace the main standard media.

The main build command is `python tools/build/m6.py`. Current schema-1 recipes remain data-only: no executable packages, arbitrary shell hooks, desktop themes or ignored runtime options. M6 bounds template target paths to leave room for the selected account name; original data files retain the recipe size bound. The base user volume supports much larger streamed files. The native executable-package/desktop component model belongs to M7 and later application work. Qualification of the actual custom ISO is part of M6; a validated JSON recipe alone is insufficient.
