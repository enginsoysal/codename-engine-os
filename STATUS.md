# Voortgang

Bijgewerkt: 5 november 2026.

Architectuur en plan: versie 0.2. M0 gestart; elf layout-/compileer-/hostreferentiecontroles geslaagd. OS-kernel: niet gebouwd.

Projectnaam: **Codename: Engine OS**. Canonieke werkbron: `enginsoysal/codename-engine-os`, openbaar, MIT-licentie. Afgeronde updates worden gecommit en gepusht volgens AGENTS.md.

| Mijlpaal | Status | Bewijs |
|---|---|---|
| M0 Architectuurcontracten | In uitvoering: contracten/keuzes en eerste layouts vastgelegd; elf controles geslaagd; toolpins en aanvullende validatie open | [Stand](m0/README.md), [bewijs](evidence/M0/README.md) |
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
- Elf lokale layout-/compileer-/Windows-referentiecontroles uitgevoerd; alle geslaagd.

## Eerstvolgende M0-afronding

Zie [M0-werkpakketten](m0/README.md). QEMU/OVMF/ISO-tools zijn nog niet gevalideerd; inhoudsvalidatie van de eerste headers en aanvullende referentieproeven staan open. Dit is een concrete resterende werklijst, geen claim dat M0 klaar is. M1 blijft niet gestart.
