# Distribution recipes

`engineos.json` builds the standard M5.1 release. `forge.json` builds a working example with different branding, accent and an additional selected data package. Both keep guided setup and mandatory disk safety checks.

```powershell
python tools/build/m51.py --profile distros/forge.json --validate-only
python tools/build/m51.py --profile distros/forge.json
```

Output goes to `dist/profiles/<id>/`. Use unique profile IDs. The original kernel, live/installed system and ISO/USB media are actually built; the profile is not a screenshot/theme mockup. The standard build also copies its official artifacts to `dist/EngineOS-M51*`.

Only the documented branding and data-package fields are supported today. No desktop, executable package manager or install hook is implied. [Complete recipe tutorial and limits](../docs/handbook/distributions.md), [product principles](../docs/architecture/product-principles.md), [MIT license](../LICENSE).
