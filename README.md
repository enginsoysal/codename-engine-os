# Codename: Engine OS

An independent, open-source operating system built around its own kernel and system services. The first application goal is installing and using unmodified Windows software; Linux, macOS, DOS and ARM support are staged roadmap goals.

**Current state: M1 own-kernel boot implemented.** The UEFI loader exits firmware boot services and starts our ELF64 kernel with its own page tables, GDT/TSS/IDT and framebuffer output. ISO/USB VM boot and exception diagnostics are tested in QEMU. There is no installer or application support yet. [Download the M1 prerelease](https://github.com/enginsoysal/codename-engine-os/releases/tag/m1-v0.1.0), [run/build instructions](M1.md), [evidence](evidence/M1/README.md). Existing documentation is primarily in Dutch. Original project code and documentation use the [MIT license](LICENSE); third-party tools retain their respective licenses.

Canonical repository: [enginsoysal/codename-engine-os](https://github.com/enginsoysal/codename-engine-os).

## Architectuur en uitvoeringsplan

De aangescherpte productrichting is Windows-appcompatibiliteit over het Windows 95–11-tijdperk. Boot en installatie waren al gepland; volledige generatiedekking vraagt aanvullende trajecten. Zie [scope, ontbrekende onderdelen en kwaliteitscriteria](WINDOWS-COMPATIBILITY.md). Universele foutloosheid is geen bewezen of afgegeven garantie.

Vastgelegd: 5 november 2026. Documentversie: 0.4.

Status: M0 afgerond; M1 eigen-kernelboot geïmplementeerd en getoetst. [M1 starten](M1.md); installatie volgt in M5. Appcompatibiliteit blijft latere scope.

## Doel

Een zelfstandig besturingssysteem met een eigen kernel, eigen systeemdiensten en eigen desktop. De eerste gebruikersroute is een bestaande Windows-installer downloaden, installeren en de applicatie gebruiken. Daarna volgen bredere Windows-compatibiliteit, DOS, Linux en macOS. De codebasis moet later ook ARM64 en een geselecteerd Raspberry Pi-model ondersteunen.

De gebruiker bepaalt productrichting en acceptatie. De coding agent voert implementatie, diagnose, documentatie en beschikbare verificatie uit. Werken op echte hardware, firmware-instellingen en externe accounts kunnen menselijke handelingen vereisen; die worden concreet benoemd wanneer ze nodig zijn. M1-opdracht: eigen kernel via ISO booten, Windows 95–11-eisen behouden en updates naar GitHub publiceren.

## Documenten

- [Architectuur](ARCHITECTURE.md): systeemgrenzen en technische bouwstenen.
- [Ontwerpbesluiten](DECISIONS.md): gekozen aanpak, redenen, alternatieven en heroverwegingscriteria.
- [Mijlpalen](MILESTONES.md): per mijlpaal wat, hoe, waarom, afhankelijkheden en bewijs.
- [Voortgang](STATUS.md): actuele status en regels om voltooiing te registreren.
- [M0-contracten en proeven](m0/README.md): concrete technische keuzes en resterende afsluitcriteria.
- [M1 kernelboot](M1.md): ISO, startcommando, implementatiekeuzes en huidige grenzen.
- [Windows 95–11-doel](WINDOWS-COMPATIBILITY.md): scopeaudit en aanvullende WC1–WC7-trajecten.
- [Bijdragen](CONTRIBUTING.md) en [projectinstructies](AGENTS.md): verificatie en publicatieafspraken.

## GitHub-publicatie

Deze repository is vanaf 5 november 2026 de werkbron. Afgeronde samenhangende updates worden gecontroleerd, gecommit en naar GitHub gepusht. De eerdere planningskopie in BrixCanvas is historisch. Ruwe lokale paden en privégegevens horen niet in openbare bewijsbestanden.

## Uitgangspunten

1. Eigen kernel en systeemdiensten; geen Windows-, Linux- of macOS-kernel als onderlaag.
2. Bestaande apps blijven waar mogelijk ongewijzigd. Hun verwachte interfaces worden geïmplementeerd.
3. Directe CPU-uitvoering geldt voor passende processorarchitecturen. Andere instructiesets vereisen vertaling of emulatie.
4. Compatibiliteit is aantoonbaar per app, versie, architectuur en taak. 'Alle apps' is richting, geen releasegarantie.
5. Hardware wordt automatisch ontdekt; gebruik vereist een ondersteund protocol en driver.
6. Lage permanente geheugenlast en bruikbaarheid onder druk zijn producteisen.
7. USB-image, bootable ISO, installatie en herstel horen bij het product.
8. Een eigen OS staat hergebruik van geschikte open-sourcecomponenten toe. Herkomst, licentie en portingkosten worden vooraf beoordeeld.

## Volgorde en afhankelijkheden

M0 → M1 → M2 → M3 → M4 → M5 → M6 → M7 → M8 is het eerste hoofdtraject.

Daarna: M9 DOS; M10 graphics → M11 Steam → M12 games → M13 Adobe als productprioriteit. De feitelijke technische afhankelijkheid van Adobe van Steam is er niet. Linux M14 en macOS M15 bouwen op de core en eigen compatibiliteitsinfrastructuur, niet verplicht op Adobe. ARM64 M16 wordt vanaf M0 architectonisch voorbereid en kan na een stabiele core worden opgepakt.

M0 omvat kleine uitvoerbaarheidsonderzoeken voor lastige ABI- en compatibiliteitsvragen. Daardoor ontdekken we fundamentele blokkades vóór jaren werk aan de desktop.

## Wanneer telt een mijlpaal als klaar?

- De bijbehorende acceptatiecriteria zijn uitgevoerd op benoemde configuraties.
- Resultaten zijn reproduceerbaar met build-id, commando's en logs.
- Relevante oudere acceptatietests blijven slagen.
- Bekende beperkingen, resourcegebruik en herstelgedrag staan in het bewijsverslag.
- De status is aangepast op basis van bewijs, niet op basis van geschreven code of een screenshot.

Er zijn nog geen betrouwbare doorlooptijden of definitieve RAM-budgetten. Na M3 worden de eerste onderbouwde resourcebudgetten vastgesteld. Grote productmijlpalen krijgen vóór implementatie een versiegebonden testmatrix en kleinere werkpakketten.

## Bewijslocatie tijdens implementatie

Per mijlpaal bevat `evidence/Mxx/` een README, buildmanifest, testresultaten, logs, metingen en relevante beelden zodra beschikbaar. [M0 bevat de eerste proefresultaten](evidence/M0/README.md).
