# M0 — eerste technische resultaten

Uitgevoerd op 5 november 2026 op de Windows-ontwikkelhost.

Herhalen vanaf de huidige projectmap:

```powershell
python m0/probes/run_probes.py
```

Het script gebruikt tijdelijke bestanden, wist alleen zijn eigen tijdelijke directory en schrijft [probe-results.json](probe-results.json). Het installeert geen tools en maakt geen bootbaar OS. Compiler-/linkercommando's, foutuitvoer en resulterende hashes staan in JSON. Project-, gebruikers- en tijdelijke paden worden voor openbare publicatie vervangen door placeholders.

## Gemeten

Elf controles geslaagd:

1. C17 → freestanding x86-64 ELF64 → gelinkt bestand met juiste machine-ID.
2. C17 → freestanding ARM64 ELF64 → gelinkt bestand met juiste machine-ID.
3. C17 → PE32+ x64 met EFI application subsystem en zonder standaardlibraries.
4. Windows auto-reset event geeft na eerste succesvolle wait een timeout bij volgende zero-time wait.
5. Windows manual-reset event blijft bij tweede wait signaled.
6. Opnieuw openen van een zonder sharing geopend bestand faalt met Windows-fout 32.
7. Windows wait-any selecteert het tweede gezette event; wait-all wacht tot beide events gezet zijn.
8. Windows TLS houdt hoofdthread- en workerwaarden gescheiden.
9. Boot-/requestheader-layouts slagen op x86-64.
10. Dezelfde layoutassertions slagen op ARM64.
11. Dezelfde layoutassertions slagen op i686, zonder hostpointervelden.

Beschikbaar: Clang/LLD 21.1.6, Python, LLVM-inspectietools en LLDB. Niet gevonden op PATH: QEMU, xorriso en Rust. WSL-opvraag meldt dat WSL niet geïnstalleerd is. 'Niet op PATH' bewijst niet dat een programma nergens op de computer staat.

## Niet bewezen

Geen firmwareboot, ISO/USB-image, werkende kernel, gastsyscall, kernel-TLS-switch, Linux-/macOS-runtime, driver of appcompatibiliteit. ELF/PE-formaten controleren toont de uitvoerbare toolchainroute, niet dat deze proefcode een echte OS-entry implementeert. De vijf Windows-proeven beschrijven referentiegedrag op de host; onze kernel implementeert dat nog niet.

Daarom blijft M0 in uitvoering en M1 niet gestart. Zie [M0-afsluitcriteria](../../m0/README.md).
