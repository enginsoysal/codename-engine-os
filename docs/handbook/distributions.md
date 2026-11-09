# Build your own Engine OS distribution

## Current M9.2 hardware edition

Start from `distros/engineos-hardware.json`. Use a separate checkout for a custom edition, keep a distinct profile ID and build with `python tools/build/m92.py --profile distros/my-edition.json --revision custom`. The custom suffix produces separate `EngineOS-M92CUSTOM` media; qualify that edition independently. Do not overwrite the accepted standard release or infer that its checks qualify a changed profile.

The schema-3 optional application list, accent, account defaults, welcome text and bounded data packages retain their validated contracts. The hardware guide is a separate data package. Files, Settings, shared services and destructive-operation guards remain required; removing Calculator does not remove them. Controller binding is automatic and follows native capabilities rather than an editable bus-name allowlist. Arbitrary driver plugins, alternate kernels at runtime and a general application package ecosystem remain separate work.

[Current installation/controller guide](install-m92.md), [storage decisions](../architecture/storage-m92.md). The older build commands below describe their named historical profiles.

## M7.4 standard application profiles

Build the current app suite with `python tools/build/m74.py`. The standard
schema-3 profile is `distros/engineos-apps.json`. Its `desktop.applications`
list selects implemented optional package IDs; Files, Settings and shared
services remain required. Keep the legacy `calculator` default consistent with
the list. The simplified wizard exposes the guide, account and accent; change
apps later through Settings or Software Manager.

`distros/engineos-minimal-apps.json` selects no optional apps. Build separate
example media with `python tools/build/m74.py --profile distros/engineos-minimal-apps.json --revision minimal`.
Native acceptance boots that image, verifies all sixteen omissions, installs
each package offline through Settings and opens them after independent disk
startup. Omission disables the frontend/worker installation; trusted broker
code and retained offline sources remain. The standard suite also verifies
removal/reinstall and private-data retention.

Appearance fields and data-only packages keep their existing behavior; schema
3 also permits tutorial content in Downloads, Pictures and Music. There are no
arbitrary executable hooks. A recipe cannot bypass storage consent or remove
protected core services. [Fields, app IDs and instructions](apps-m74.md#build-a-distribution-with-a-different-application-set),
[ownership and memory choices](../architecture/applications-m74.md).

The profiles below describe their historical milestone tools and artifacts.

## M7.3 smooth native UI profile

Build the new visual release with `python tools/build/m73.py`. Its separate
default is `distros/engineos-polished.json`, version 0.7.3, with a readable
welcome guide and the existing schema-2 appearance/content fields. Use a copied
profile and `python tools/build/m73.py --profile distros/engineos-polished.json --revision custom`
for separate output names. The native wizard uses the profile's name and
welcome text; the Engine OS profile retains the Engine OS product name.
Desktop accent, light/dark defaults, wallpaper, tagline and data packages retain
their implemented effects. Existing saved preferences win during an update.

Smooth Inter coverage and supported display sizes are compiled into this UI,
rather than exposed as arbitrary font/theme plugins. Source forks remain
possible; a changed font/rasterizer requires a reviewed license/provenance lock
and new native qualification. [Graphics decisions](../architecture/graphics-m73.md).
Profile validation alone does not qualify a custom ISO, and core storage consent
cannot be disabled by a recipe. Earlier profile examples below retain their
original milestone/build tools.

## M7.2 desktop-first profile

Build with `python tools/build/m72.py`. The current default is `distros/engineos-workspace.json`: Engine OS, light windows, mint accent, a teal wallpaper and no installed Calculator by default. The same validated schema-2 profile fields remain available. Use `python tools/build/m72.py --profile distros/engineos-workspace.json --revision custom` for separately named media; edit a copied profile to choose actual defaults/content. The desktop remains independent of Calculator, and optional app expansion is not this release's focus. The simplified wizard exposes the guide, account and accent; Calculator selection is controlled by the distro profile rather than cluttering setup. Existing saved appearance wins during update. Core disk consent and firmware boot registration cannot be disabled through a theme. Profile validation is not proof that an arbitrary custom image boots; qualify each image. [Desktop choices and limits](../architecture/desktop-m72.md).

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

## Beyond the implemented profile fields

MIT source permits forks and component changes. The native desktop is implemented; arbitrary compositor/shell replacement, language packs, new executable packages and runtime plugin customization need their own contracts before becoming profile choices. Unknown fields/packages are rejected, rather than silently accepted. Disk review, confirmation and recovery guards stay mandatory in every profile. [Product principles](../architecture/product-principles.md), [historical M5.1 contract](../milestones/m5.1.md).


## M8 Windows-console edition

Start from `distros/engineos-windows.json` and build with `python tools/build/m8.py --profile distros/my-edition.json`. The schema-3 `desktop.applications` list accepts `windows` as the optional Windows Apps frontend in an M8 build. Appearance, native optional apps and bounded data files retain their validated profile fields. Removing the frontend does not remove the desktop or other native apps.

The reviewed original MSI is a separate, digest-pinned boot module and is copied to Downloads during installation; it is not an arbitrary executable profile hook. Changing the supported foreign runtime/package requires source and acceptance work. Windows Apps currently accepts only the reviewed 7-Zip package. The supplied Windows guide is a separate data package, preserving historical release inputs. [Runtime design](../architecture/windows-runtime-m8.md), [first-profile limits](../milestones/m8-msi.md).
