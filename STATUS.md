# Voortgang

Bijgewerkt: 5 november 2026.

Architectuur en plan: versie 0.4. M0 afgerond; M1 eigen kernelboot en diagnostiek geïmplementeerd/getoetst. Installer en appcompatibiliteit: niet gebouwd.

Projectnaam: **Codename: Engine OS**. Canonieke werkbron: `enginsoysal/codename-engine-os`, openbaar, MIT-licentie. Afgeronde updates worden gecommit en gepusht volgens AGENTS.md.

Projectscheiding: de actieve lokale repository staat zelfstandig buiten BrixCanvas. De oude `outputs/os-architecture`-kopie is daar volledig weggehaald en met bestands-hashcontrole verplaatst naar het genegeerde lokale archief `tools/local/legacy-planning` binnen Engine OS. Geen OS-code of nieuwe output terugplaatsen in BrixCanvas.

Scope-update: Windows 95–11-apps zijn het brede productdoel. De audit en aanvullende [WC1–WC7-trajecten](WINDOWS-COMPATIBILITY.md) zijn vastgelegd, niet geïmplementeerd. Boot/installatie blijven M1/M5. De elf eerdere M0-controles bewijzen geen Windows-appcompatibiliteit.

| Mijlpaal | Status | Bewijs |
|---|---|---|
| M0 Architectuurcontracten | Voltooid binnen architectuur/toolchainscope; runtimepoorten overgedragen | [Acceptatie](evidence/M0/acceptance-results.json), [bewijs](evidence/M0/README.md) |
| M1 Bootbare kernel | Voltooid voor gepinde QEMU/UEFI ISO-/USB-route; fysieke hardware niet gekwalificeerd | [M1-bewijs](evidence/M1/README.md) |
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

## Uitgevoerd in M1

- Eigen UEFI-loader, ELF-controle, initmanifest, memory-map-/reserveringsoverdracht en begrensde ExitBootServices-retry.
- Eigen paginatabellen/stack, GDT/TSS/IDT, seriële diagnostiek, framebufferconsole en symboliseerbare #UD-panic.
- Normale ISO en GPT-USB-image, VM-launcher, checksums en debug-symbolen voor prerelease.
- 29 M1-checks en alle 51 M0-regressiechecks slagen; bron-/artefacthashes en documentlinks gecontroleerd.
- Windows 95–11-eisen blijven in aparte toekomstige applicatieomgevingen; geen Windows-apps als ondersteund gemeld.

## Eerstvolgende mijlpaal

M2: beschermde adresruimtes, eigen usermodeprocessen, handles en minimale syscalls. De M1-kernel kan nu in een VM booten, maar bevat nog geen installer; die volgt bij M5 na opslag/invoer. Zie [huidige startinstructies](M1.md).
