# M0 — afsluitbewijs

Uitgevoerd op 5 november 2026 op de Windows-ontwikkelhost.

Herhalen vanaf de huidige projectmap:

```powershell
python m0/probes/run_probes.py
python m0/verify_m0.py
```

De eerste opdracht herhaalt de oorspronkelijke elf proeven; de tweede voert alle M0-gates vers uit. Setup staat in [tools](../../tools/README.md). Resultaten: [acceptatie](acceptance-results.json), [basis](probe-results.json), [contracten](contract-results.json), [Windowsreferenties](windows-reference-results.json), [firmware/media](environment-results.json). Project-, gebruikers- en tijdelijke paden in openbare proefrapporten zijn geredigeerd. Gegenereerde binaries en images blijven lokaal onder `build/`.

## Gemeten

Oorspronkelijke elf controles geslaagd:

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

Aanvullend geslaagd:

- 30 checks tegen de daadwerkelijke C-inhoudsvalidators: goede header, CRC, afkapping, ongeldige versie/flags, overflow, overlap, descriptorvorm, framebuffer en userbuffergrenzen. Samengestelde checks omvatten 160 afgeknotte bootlengtes, 24 requestlengtes en 1000 deterministische enkelbitcorrupties.
- 8 Windows-hostreferenties: geblokkeerde rename zonder delete-sharing, Unicode-rename na sluiten, lezen na delete-sharing, uiteindelijke naamverwijdering, case-insensitive lookup in de referentiemap, afzonderlijke Unicode-normalisatievormen, continuable exceptiondelivery en handleropruiming.
- 2 firmwareloads: OVMF voert `efi_environment.c` uit vanaf een optische ISO en een GPT/FAT32-image via USB-storage achter xHCI. Beide geven seriële marker `ENGINE_OS_M0_ENVIRONMENT_OK` en debug-exitcode 33. Zie de [ISO-log](environment-iso-serial.log) en [USB-log](environment-usb-serial.log).

Totaal: **51 technische checks**, plus controle van documentlinks en Windows-generatie-inventaris. QEMU bleek al buiten PATH geïnstalleerd. De waargenomen compiler-, QEMU-/DLL- en firmwarehashes staan in het [toolmanifest](../../tools/toolchain-lock.json). pycdlib 1.22.0 is de gepinde ISO-verpakker; xorriso/WSL zijn daarvoor niet meer vereist.

## Niet bewezen

Geen eigen kernelboot/ExitBootServices-overdracht, installer, gastsyscall, kernel-TLS-switch, Linux-/macOS-runtime, hardwaredriver of appcompatibiliteit. De EFI-fixture blijft in de firmwareomgeving en stopt na zijn marker. De C-validators controleren bytes/arithmetic, niet dat verwezen geheugen gemapt/toegankelijk is. Hostreferenties zijn geen gastimplementatie. Lokale hashes zijn geen onafhankelijk upstream-authenticiteitsbewijs. ISO-timestamps kunnen artefacthashes tussen builds veranderen; de gate eist succesvolle firmware-loads, niet byte-identieke ISO's.

M0 is binnen deze expliciete architectuur-/toolchainscope afgesloten. M1 is niet gestart. Zie [afsluitcriteria](../../m0/README.md) en [overdracht](../../m0/M1-HANDOFF.md).
