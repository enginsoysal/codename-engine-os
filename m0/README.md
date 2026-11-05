# M0 — afgesloten architectuur- en toolchainbaseline

Afgerond: 5 november 2026. Scope: gereedheid voor M1; geen werkende OS-kernel of installer.

## Resultaat van deze stap

- [Mappen en opslag](FILESYSTEM.md): eigen structuur, rechten, appomgevingen en gastpaden.
- [Kernelcontracten](KERNEL-CONTRACTS.md): objecten, geheugen, procesmodel en ABI-routing.
- [Semantiekmatrix](COMPATIBILITY.md): Windows/Linux/macOS-verschillen en validatiepoorten.
- [Bouw- en platformkeuzes](BUILD-PLATFORM.md): taal, toolchain, boot, filesystem en testmachine.
- [Windows-doelregister](windows-targets.json): acht generatiegroepen en verplichte appkwalificatievelden.
- [Bewijs](../evidence/M0/README.md) en [acceptatieresultaat](../evidence/M0/acceptance-results.json).
- [Verificatie](verify_m0.py): vier verse suites plus document-/inventariscontrole.
- [M1-overdracht](M1-HANDOFF.md): kortste uitvoeringsroute naar kernelboot en daarna installatie.

## Afsluitcriteria en stand

| Criterium | Stand | Betekenis |
|---|---|---|
| Productgrenzen en lagen | Afgerond | Eigen core; Windows 95–11 via WC1–WC7 |
| Mappen, mounts en gastweergaven | Vastgelegd | Goedgekeurde richting verwerkt in technisch contract |
| Hoofdtaal/toolchain | Gekozen en compileerroute getoetst | C17 + Clang/LLD; ELF64 en EFI-image gevormd |
| Bootcontract | Vastgelegd en header getoetst | Eigen kerneloverdracht volgt in M1 |
| Object-/proces-/geheugenmodel | Ontwerp vastgelegd | Runtimecorrectheid moet nog bewezen worden |
| Win16/32/64-strategie | Vastgelegd | Legacy-runtime, compatibility mode en directe x64-uitvoering; nog geen appuitvoering |
| ABI-risico's | Windows-referenties getoetst | Events, multiwait, TLS, exceptions, sharing, rename en Unicode |
| Boot-/requestlayouts | Compileerbaar en inhoudsvalidator getest | Assertions op x86-64, ARM64 en i686; negatieve hosttests |
| Virtueel platform | Gepind en uitgevoerd | QEMU pc-q35-11.1/TCG/OVMF; ISO en GPT-USB firmwarefixture |
| ISO-verpakking | Gepind en uitgevoerd | pycdlib 1.22.0 met wheelhash; geen WSL/xorriso nodig |
| Referentiemetingen Linux/macOS | Niet uitgevoerd | Interfacebronnen onderzocht; geen beschikbare referentiesessie gebruikt |
| Fysieke testcomputer | Niet geselecteerd | Host wordt niet automatisch een wisbaar testdoel |

M0 is afgerond binnen de architectuur-/toolchainscope. De baseline heeft 51 technische controles: 11 oorspronkelijke bouw/layout/Windowsproeven, 30 contractchecks, 8 aanvullende Windowsreferenties en 2 firmwareloads. Contracttests bevatten bovendien 160 afgeknotte bootlengtes, 24 requestlengtes en 1000 deterministische bitcorrupties. De actuele verse resultaten staan in het acceptatierapport.

## Overdracht en expliciete latere poorten

1. M1: echte loader/kernel-entry, fysieke inhoudscontrole en ExitBootServices. De M0-fixture gebruikt firmware, schrijft een marker en sluit af; dat is geen kerneloverdracht.
2. M2/M3/M7: daadwerkelijke syscall-entry, gast-TLS, racegedrag en exceptionimplementatie. Windows-hostmetingen zijn referenties, geen eigen implementatie.
3. M14/M15: Linux-/Mach-runtimeproeven op passende omgevingen. Documentatie is geen gemeten compatibiliteit.
4. Concrete Wine/Darling-revisions worden vóór een port gekozen. Er is nu geen component overgenomen; willekeurige revisions vastzetten zou geen port valideren.
5. Een fysieke testcomputer wordt vóór fysieke tests gekozen; de hostschijf wordt nooit automatisch een testdoel. Virtueel M1 starten kan al.

## Bronnen en bewijs

Primaire documentatie staat in de contracten. M0 heeft een getoetste bouw-/testbasis; kernelwerking, installatie en appcompatibiliteit zijn niet bewezen. Het Windows-appregister is daarom nog leeg. Nieuwe bevindingen wijzigen deze baseline via een gemotiveerd besluit plus regressietests.
