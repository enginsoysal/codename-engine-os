# Voortgang

Bijgewerkt: 5 november 2026.

Architectuur en plan: versie 0.3. M0 afgerond als architectuur-/toolchainbaseline; 51 technische controles plus document-/inventariscontrole geslaagd. OS-kernel en installer: niet gebouwd.

Projectnaam: **Codename: Engine OS**. Canonieke werkbron: `enginsoysal/codename-engine-os`, openbaar, MIT-licentie. Afgeronde updates worden gecommit en gepusht volgens AGENTS.md.

Scope-update: Windows 95–11-apps zijn het brede productdoel. De audit en aanvullende [WC1–WC7-trajecten](WINDOWS-COMPATIBILITY.md) zijn vastgelegd, niet geïmplementeerd. Boot/installatie blijven M1/M5. De elf eerdere M0-controles bewijzen geen Windows-appcompatibiliteit.

| Mijlpaal | Status | Bewijs |
|---|---|---|
| M0 Architectuurcontracten | Voltooid binnen architectuur/toolchainscope; runtimepoorten overgedragen | [Acceptatie](evidence/M0/acceptance-results.json), [bewijs](evidence/M0/README.md) |
| M1 Bootbare kernel | Niet gestart | — |
| M2 Geïsoleerde processen | Niet gestart | — |
| M3 Geheugen en concurrency | Niet gestart | — |
| M4 Opslag en invoer | Niet gestart | — |
| M5 Live en installatie | Niet gestart | — |
| M6 Desktop en netwerk | Niet gestart | — |
| M7 Windows-binarybasis | Niet gestart | — |
| M8 7-Zip | Niet gestart | — |
| M9 DOS | Niet gestart | — |
| M10 GPU-versnelling | Niet gestart | — |
| M11 Steam-client | Niet gestart | — |
| M12 Steam-games | Niet gestart | — |
| M13 Adobe | Niet gestart | — |
| M14 Linux | Niet gestart | — |
| M15 macOS | Niet gestart | — |
| M16 ARM64/Pi | Niet gestart | — |

## Statusregels

Toegestane statussen: niet gestart, in uitvoering, geblokkeerd met concrete oorzaak, verificatie, voltooid met bewijs. Een plandocument is geen werkende kernel. Een mock, stub of testprogramma bewijst alleen het expliciet geteste gedrag.

Voor elke mijlpaal worden tijdens uitvoering bijgehouden: werkpakketten, resterende besluiten, uitgevoerde controles, beperkingen en bewijslink. Mislukkingen en overschrijdingen blijven zichtbaar.

## Uitgevoerd in M0

- Goedgekeurde eigen root en Windows/Linux/macOS-weergaven verwerkt.
- C17/LLVM, eigen UEFI-loader, FAT32/ext2-ontwikkelroute en QEMU-profiel gekozen met redenen/alternatieven.
- Kernelobjecten, rechten, IPC, syscallrouting, geheugencommit en 32-bitstrategie beschreven.
- 51 technische controles: 11 oorspronkelijke proeven, 30 contractchecks, 8 extra Windowsreferenties en 2 firmwareloads.
- QEMU/firmware/compilerhashes en ISO-library gepind; ISO en GPT-USB fixture starten onder UEFI.
- Windows 95–11-generatie-/kwalificatie-inventaris en M1-overdracht toegevoegd.

## Eerstvolgende mijlpaal

M1: echte eigen UEFI-loader, kernel-entry, Boot Services-overdracht, diagnostiek en OS-bootmedia. Zie [werkvolgorde](m0/M1-HANDOFF.md). De M0-fixture is geen OS-/installatiemedium. Een VM voor kerneltests volgt bij M1; de installer volgt bij M5. M1 blijft niet gestart in deze update.
