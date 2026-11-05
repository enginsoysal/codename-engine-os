# Windows 95–11: productdoel, dekking en ontbrekende trajecten

Vastgelegd op 5 november 2026 naar aanleiding van de aangescherpte gebruikerseis.

## Productdoel en betekenis

De gebruiker wil een zelfstandig, bootbaar en installeerbaar OS met een eigen implementatie van Windows-functionaliteit. Applicaties gemaakt voor Windows 95 tot en met Windows 11 moeten met hun originele installatiebestanden geïnstalleerd kunnen worden en vlekkeloos starten. Correct gebruik na het starten blijft onderdeel van het bestaande productdoel.

Dit is de gewenste richting, geen reeds haalbare universele releasegarantie. Een jaartal of Windows-versienaam specificeert niet welke architectuur, API's, drivers, installatievormen en externe diensten een app nodig heeft. Onbekende en ongeteste apps blijven onbekend; blockers worden niet stilzwijgend uit de eis verwijderd.

Eigen code betekent eigen bronimplementatie; de gekozen hoofdtaal is C17 met beperkte assembly. Een nieuwe programmeertaal ontwikkelen is niet gevraagd en maakt geen deel uit van de roadmap. Hergebruik van geschikte open-sourcecomponenten blijft toegestaan. Er wordt geen Microsoft-kernel of volledige Windows-installatie als verplichte onderlaag gebruikt.

Het doel gaat over compatibel gedrag. Een pixel-identieke Windows-desktop of het namaken van alle server-, beheer- en clouddiensten volgt niet automatisch uit appcompatibiliteit. 'Windows compleet namaken' vereist voor die bredere interpretatie een aparte productinventarisatie; de huidige M0–M16 bewijzen dat niet.

## Audit van de bestaande mijlpalen

| Eis | Aanwezige route | Dekking vóór aanscherping |
|---|---|---|
| USB/ISO boot | M1 | Expliciet gepland |
| Naar schijf installeren en zelfstandig booten | M5 | Expliciet gepland |
| Eigen desktop en netwerk | M6 | Basis, geen volledige Windows-shell |
| Win32/Win64 en NT-basis | M7/M8 | Geselecteerde programma's en 7-Zip |
| DOS | M9 | Afzonderlijk gepland; geen bewijs van Win16 |
| Win16/NE en oude 16-bit-installers | Geen volledige route | Ontbrak als expliciet traject |
| Windows 95/98/Me-gedrag | Gedeeltelijk impliciet | Ontbrak als afzonderlijk generatieprofiel |
| NT-generaties tot Windows 11 | M7/M11/M13 | Geen brede generatie-/runtimeacceptatie |
| Windows-kerneldrivers/VxD-afhankelijkheden | ADR-015 | Bewust geen eerste-releasebelofte; conflicteert met een absolute alle-appseis |
| Alle app- en installatievormen | Geen volledige inventaris | Onder meer moderne pakket-/runtimepaden niet voldoende uitgewerkt |
| Vlekkeloos voor alle apps | Geen bewijs | Geen van de bestaande mijlpalen rechtvaardigt deze claim |

## Aanvullende trajecten zonder bestaande IDs te hernummeren

### WC1 — Applicatie- en installerregister (vanaf M0, doorlopend)

**Wat/hoe:** registreer appnaam, exacte versie/hash, oorspronkelijke Windows-doelversies, architectuur, installerarchitectuur, runtimes, API's, drivers, diensten, hardware en taken. Stel per generatie een testset samen: 95/98/Me, NT/2000/XP, Vista/7, 8/8.1 en 10/11. Dit zijn inventarisgroepen, geen aanname dat één profiel een hele groep ondersteunt.

**Waarom:** een 32-bit app kan een 16-bit installer hebben; alleen de hoofdbinary bekijken mist die afhankelijkheid.

**Acceptatie:** elke gepubliceerde compatibiliteitsclaim heeft een registerrecord en reproduceerbare taken. Geen percentage 'alle Windows-apps' zonder gedefinieerde populatie. Originele commerciële installers komen niet automatisch in de openbare repository.

### WC2 — Win16 en historische installatieketens (M7/M9 als fundament)

**Wat/hoe:** NE-loader, Win16-API's, segmented pointers, berichten-/threadgedrag en 16↔32-bit-overgangen onderzoeken en implementeren. Kies een geïsoleerde legacy-runtime met benodigde CPU-/apparaatemulatie. DOS-emulatie mag worden gedeeld waar passend maar vervangt Win16-API's niet. Onderzoek oude installatiedragers en gemengde installerketens.

**Waarom:** modern PE32/PE32+ ondersteunen dekt deze software niet. Microsoft beschrijft dat x64-Windows geen algemene 16-bit-appuitvoering ondersteunt; ons doel is dus op dit punt breder dan die moderne omgeving.

**Acceptatie:** ongewijzigde Win16-app, 32-bit-app met 16-bit-installer, installatie/herstart/start/verwijderen en bestandsuitwisseling aantoonbaar getest. Geen ongecontroleerde uitvoering van legacycode in host-kernelmode.

### WC3 — Windows 9x versus NT-profielen (na M7; onderzoek vanaf M0)

**Wat/hoe:** versiegebonden gedrag vergelijken voor paden, Unicode/ANSI, registry, IPC, timers, gedeelde toestand en oude grafische API's. Implementeer betekenisvolle profielen en hun onderliggende gedrag. Alleen een ander OS-versienummer teruggeven telt niet als ondersteuning.

**Waarom:** Windows 95/98/Me en de NT-lijn zijn geen identieke omgevingen. Conflicterende verwachtingen worden via omgevingsprofielen gescheiden.

**Acceptatie:** registerdoelen uit meerdere generaties installeren en starten met hun toegewezen profiel; verschillen en werkelijke afhankelijkheden zijn vastgelegd. Profielkeuze is zichtbaar/traceerbaar en waar mogelijk automatisch op bewijs gebaseerd.

### WC4 — Runtimes, services en installatievormen (M7/M8/M11/M13)

**Wat/hoe:** inventariseer EXE-/MSI-installers en latere pakketvormen zoals MSIX/AppX; COM/OLE/ActiveX, side-by-side assemblies, Visual C++-/VB-runtimes, .NET Framework/moderne .NET, browser-/webcomponenten, servicebeheer en geplande taken waar doelapps die gebruiken. WinRT/UWP/Windows App SDK krijgen afzonderlijke afhankelijkhedenkaarten en tests. Ook herstel, updates, uninstall en rechtenverhoging horen erbij.

**Waarom:** een werkende Win32-loader levert geen complete applicatieomgeving. Appcomponenten en hun installers kunnen verschillende generaties gebruiken.

**Acceptatie:** elke vereiste runtime-/pakketroute heeft een concrete app en end-to-endtaak; eigendoms-/licentievoorwaarden en externe dienstafhankelijkheden zijn benoemd. Ontbrekende services zijn echte blockers, geen succesvolle no-opstubs.

### WC5 — Oude en moderne multimedia (uitbreiding M10/M12)

**Wat/hoe:** inventariseer de benodigde GDI-, DirectDraw-, Direct3D-, audio-, input- en mediaroutes per doelapp. Bouw adapters boven eigen hardwaredrivers; leg API-versie en apparaatvereisten vast. Test oude fullscreen-/kleurmodi en moderne render-/shaderpaden afzonderlijk.

**Waarom:** één werkende 3D-demo bewijst niet dat oude 2D-games of moderne DirectX-apps correct werken.

**Acceptatie:** correcte uitvoer, input, audio en stabiel starten op benoemde hardware per route. Prestatiecriteria volgen uit concrete workloads; hardwarevereisten verdwijnen niet door API-compatibiliteit.

### WC6 — Drivergebonden software, beveiliging en externe diensten

**Wat/hoe:** inventariseer VxD-, NT-driver-, filter-, anticheat-, DRM- en apparaatgebonden afhankelijkheden. Kies per klasse eigen functioneel alternatief, een aparte drivercompatibiliteitsimplementatie of een geïsoleerde legacy-machineomgeving met eigen code. Niet iedere klasse heeft nu een bewezen oplossing. Onderzoek service-/licentietoegang zonder beveiligingscontroles te omzeilen.

**Waarom:** userspace-API's en kerneldrivers zijn verschillende contracten. Ons OS beheersen geeft geen toegang tot verdwenen activatieservers, ontbrekende fysieke apparaten of leverancierstoestemming.

**Acceptatie:** per registerdoel concrete afhankelijkheden en bewijs of expliciete blocker. Onopgeloste verplichte afhankelijkheid betekent dat de doelapp niet ondersteund is. ADR-015 blijft geldig voor de eerste release, maar is geen dekking van het uiteindelijke brede productdoel.

### WC7 — Compatibiliteitskwalificatie en regressies

**Wat/hoe:** test originele installer op schone omgeving, installatie, eerste start, herstart OS, herhaalde start, kernfuncties, opslaan/openen en uninstall. Controleer childprocessen, services, foutlogs, datacorruptie en isolatie. Automatiseer reproduceerbare onderdelen en documenteer handmatige taken.

**Waarom:** 'vlekkeloos' mag niet betekenen dat alleen een splashscreen zichtbaar is of fouten worden verborgen. Een geslaagde testset maakt nog geen uitspraak over ongeteste software.

**Acceptatie:** voor de gepubliceerde versie/taakmatrix geen bekende kritieke of blokkerende fouten; afwijkingen blijven zichtbaar. Volledige foutloosheid van alle mogelijke apps en invoer wordt niet geclaimd. Launchable, installed, usable en performant blijven afzonderlijke meetresultaten.

## Afhankelijkheden en prioriteit

M0 heeft de [machineleesbare generatie-/appinventaris](m0/windows-targets.json) vastgelegd. Het register bevat bewust nog geen geslaagde apps; de proefprogramma's zijn Windows-hostreferenties en UEFI-toolchecks. [M0 → M1](m0/M1-HANDOFF.md) bewaart de Windows 95–11-eisen bij de eerste echte kernelimplementatie.

M0–M8 blijft het eerste verticale implementatiepad, met WC1 vanaf M0 en legacy/runtimerisico's al in het ontwerp. WC2–WC7 vullen de Windows-roadmap aan; hun concrete werkpakketten worden voor uitvoering verder gesplitst. Linux/macOS blijven toekomstige trajecten en krijgen geen hogere prioriteit dan deze Windows-doelen. UEFI-only boot blijft geldig: oude apps ondersteunen verplicht het OS niet tot booten op een pc uit 1995.

## Primaire bronnen

- [Microsoft: NTVDM en 16-bit-apps](https://learn.microsoft.com/en-us/windows/compatibility/ntvdm-and-16-bit-app-support).
- [Microsoft: x64-Windows en 16-bit-componenten/installers](https://learn.microsoft.com/en-us/troubleshoot/windows-client/application-management/x64-windows-not-support-16-bit-programs).
- [Microsoft: applicatie-installatie op x64](https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/WinProg64/application-installation.md).

Deze bronnen onderbouwen het onderscheid tussen applicatie- en installerarchitectuur; zij bewijzen geen compatibiliteit van Engine OS.
