# M0 — architectuurcontracten

Datum: 5 november 2026. Status: in uitvoering; ontwerpbesluiten en eerste technische proeven vastgelegd.

## Resultaat van deze stap

- [Mappen en opslag](FILESYSTEM.md): eigen structuur, rechten, appomgevingen en gastpaden.
- [Kernelcontracten](KERNEL-CONTRACTS.md): objecten, geheugen, procesmodel en ABI-routing.
- [Semantiekmatrix](COMPATIBILITY.md): Windows/Linux/macOS-verschillen en validatiepoorten.
- [Bouw- en platformkeuzes](BUILD-PLATFORM.md): taal, toolchain, boot, filesystem en testmachine.
- [Eerste proefresultaten](../evidence/M0/README.md): wat daadwerkelijk is getest.
- [Proefscript](probes/run_probes.py): herhaalbare compileer- en Windows-referentieproeven.

## Afsluitcriteria en stand

| Criterium | Stand | Betekenis |
|---|---|---|
| Productgrenzen en lagen | Vastgelegd | Eigen core, afzonderlijke omgevingen, Windows eerst |
| Mappen, mounts en gastweergaven | Vastgelegd | Goedgekeurde richting verwerkt in technisch contract |
| Hoofdtaal/toolchain | Gekozen en compileerroute getoetst | C17 + Clang/LLD; ELF64 en EFI-image gevormd |
| Bootcontract | Vastgelegd | Eigen UEFI-loader; uitvoering volgt in M1 |
| Object-/proces-/geheugenmodel | Ontwerp vastgelegd | Runtimecorrectheid moet nog bewezen worden |
| 32-bitstrategie | Vastgelegd | Hardware compatibility mode; gemengde modules via thunks |
| ABI-risico's | Geïnventariseerd; gedeeltelijk getoetst | Windows-events, multiwait, TLS en bestandsdeling gemeten; geen gastsyscalls uitgevoerd |
| Boot-/requestlayouts | Eerste headers compileerbaar | Assertions op x86-64, ARM64 en i686; geen complete runtimevalidatie |
| Virtueel platform | Profiel gekozen; nog niet uitvoerbaar | QEMU/OVMF/image-tools ontbreken in gecontroleerde omgeving |
| Referentiemetingen Linux/macOS | Niet uitgevoerd | Interfacebronnen onderzocht; geen beschikbare referentiesessie gebruikt |
| Fysieke testcomputer | Niet geselecteerd | Host wordt niet automatisch een wisbaar testdoel |

M0 is nog niet technisch volledig afgerond. Eerst moeten de virtuele tools en firmware worden vastgepind en aanvullende exception-/padproeven worden uitgevoerd. De eerste boot-/ABI-structuren hebben compileerbare layoutcontroles; validatie van inhoud, ranges en kernel-entry blijft apart werk. M1 blijft niet gestart. Een fysieke testcomputer is geen blocker voor de virtuele eerste build.

## Werkpakketten voor de resterende M0-afronding

1. Maak een projectlokale toolmanifest met versie, herkomst, checksum en installatie-/startcontrole voor QEMU, OVMF en ISO-verpakking. De kandidaatconfiguratie wordt pas referentie na een start-/apparaatcontrole.
2. Vul de geslaagde TLS-/multiwaitproeven aan met exceptionoverdracht en pad-/renamegedrag. Voer Linux/macOS-specifieke proeven pas op beschikbare doelomgevingen uit; label documentatie versus meting.
3. Breid de [bootheader](contracts/boot_v0.h) en [requestheader](contracts/syscall_v0.h) uit met inhoudsvalidatie en negatieve inputgevallen. Layoutassertions op drie targets slagen al. De kernelimplementatie zelf hoort bij M1/M2.
4. Leg compatibiliteitscomponenten vast op concrete revisions vóór daadwerkelijk porten; momenteel is geen volledige Wine/Darling-port gekozen of bewezen.
5. Sluit M0 pas met een bijgewerkte acceptatiematrix; meld open runtimepoorten aan M1/M2 expliciet.

## Bronnen en bewijs

Primaire documentatie is gelinkt in de afzonderlijke contracten. De toolchain, headerlayouts en vijf Windows-referentiegevallen zijn in elf controles getoetst. De uiteindelijke OS-werking is daarmee niet bewezen.
