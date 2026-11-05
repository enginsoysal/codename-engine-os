# Mappen, opslag en compatibiliteitsweergaven

Status: gekozen richting, gebaseerd op de door gebruiker goedgekeurde indeling. Geen directories van het toekomstige OS op de host aangemaakt.

## Eigen namespace

```text
/
├── System/
│   ├── Versions/<build-id>/     Onveranderlijke systeemversies
│   ├── Current/                Door bootprofiel geselecteerde versie
│   └── Compatibility/          Logische toegang tot gedeelde omgevingsruntimes
├── Applications/<app-id>/
│   └── Versions/<version>/     Beheerde programmabestanden
├── Users/<account>/
│   ├── Desktop/
│   ├── Documents/
│   ├── Downloads/
│   ├── Pictures/
│   └── AppData/<app-id>/       Instellingen en duurzame persoonlijke appdata
├── Data/
│   ├── Services/<service-id>/  Permanente systeemdienstgegevens
│   ├── Environments/<env-id>/  Registry, gedeelde gastconfiguratie en mappings
│   └── Packages/               Installatie- en updateadministratie
├── Cache/<principal>/<app-id>/ Opnieuw opbouwbaar; niet voor unieke appdata
├── Logs/<principal>/          Begrensd; geen wachtwoorden of tokens
├── Temporary/<principal>/     Tijdelijk, privé en met bewaartermijn
└── Volumes/<mount-name>/      Geautoriseerde extra lokale/netwerkopslag
```

`Current` is een door de systeemdienst beheerde selectie, niet een door apps wijzigbare link. `Compatibility` is een logisch runtime-aanzicht; de inhoud komt uit geversioneerde systeempakketten. De interne uitvoeringsmethode kan een VFS-mount zijn en is niet afhankelijk van symlinkgedrag.

## Hoe en waarom

- Eén root voorkomt dat apppaden afhangen van de plaats van een fysieke schijf. Schijven en partities blijven bestaan onder de opslaglaag.
- Mountidentiteit is een volume-ID; de leesbare naam kan wijzigen. Verwijzingen van de installer gebruiken nooit uitsluitend een verwisselbare labelnaam.
- VFS ondersteunt namespaces per proces/appomgeving. Twee processen kunnen verschillende `C:`-weergaven hebben zonder dat documenten worden gekopieerd.
- Programmaversies blijven normaal alleen-lezen. De installatiedienst maakt versies aan en activeert ze. Dit helpt herstel en voorkomt willekeurige wijziging van gedeelde code.
- Cache en tijdelijk geheugen op schijf worden apart beheerd. Een app kan geen duurzame gegevens verliezen omdat het OS haar opslag ten onrechte als cache classificeert.
- De native namespace is hoofdlettergevoelig en bewaart UTF-8-namen zonder stilzwijgende normalisatie. Gastadapters hebben hun eigen vergelijkingsregels. Namen die botsen in een gastweergave geven een expliciete fout of een vooraf vastgelegd geïsoleerd aanzicht; geen willekeurige bestandselectie.
- Paden zijn labels; autorisatie gebruikt objectidentiteit, account en rechten. Een padalias kan beveiliging niet omzeilen.

## Windows-weergave

| Windows-pad | Eigen opslag/weergave | Gedrag |
|---|---|---|
| `C:\Windows` | Geselecteerde Windows-runtime plus omgevingsconfiguratie | Programmacode alleen-lezen; beheerde configuratie apart |
| `C:\Program Files\<app>` | Geregistreerde appversie | Originele installer ziet verwachte structuur |
| `C:\Program Files (x86)\<app>` | Geregistreerde 32-bit appversie | Gescheiden gastpad; geen andere fysieke schijf nodig |
| `C:\ProgramData` | `Data/Environments/<env-id>/ProgramData` | Gedeelde data binnen de expliciete appomgeving |
| `C:\Users\<name>\Documents` | `Users/<account>/Documents` | Dezelfde bestanden met verleende rechten |
| `C:\Users\<name>\AppData` | Per-app of gedeelde omgevingsdata | Roaming/Local/LocalLow expliciet gemapt |
| `C:\...\Temp` | Privégebied onder `Temporary` | Gastvariabelen wijzen hiernaar |
| `D:` en volgende | Toegewezen volume-ID | Alleen zichtbaar na beleid/toestemming |

Niet iedere installer past netjes onder Program Files. De omgevingsroot kan ook beheerde bestanden op andere gastpaden bevatten. Installaties worden binnen die omgeving gevolgd; absolute paden blijven stabiel zolang de omgeving bestaat. Oude apps die naar hun programmadirectory schrijven krijgen waar nodig een expliciete schrijflaag. Dat geldt niet ongemerkt voor gedeelde systeembibliotheken.

Appomgevingen zijn standaard apart, maar niet verplicht één per executable. Steam met helpers/games of Adobe met componenten kan een gedeelde omgeving krijgen. Onderlinge COM-, service- en registryrelaties moeten dan zichtbaar zijn. Isolatie die applicaties breekt is geen geslaagde compatibiliteit.

## Linux- en macOS-weergave

Linux: `/usr` en benodigde runtimebibliotheken uit een geversioneerd omgevingspakket; `/etc` uit omgevingsconfiguratie; `/var` uit duurzame dienst-/appdata; `/home/<name>` uit het gebruikersaanzicht; `/tmp` privé en tijdelijk. `/proc`, `/sys` en apparaten zijn beperkte virtuele interfaces, geen ongefilterde hostinventaris.

macOS: `/System` uit de doelruntime, `/Library` uit gedeelde omgevingsdata, `/Applications` uit appregistraties, `/Users` uit gebruikersdata en `/Volumes` uit toegestane mounts. `~/Library` mapt naar persoonlijke macOS-appgegevens. Framework- en SDK-bestanden zijn afzonderlijk geversioneerd en alleen beschikbaar wanneer rechtmatig verkregen.

Waarom: paden en gedrag van apps worden ondersteund zonder de eigen root vol te zetten met overlappende ecosystemen. Een gastpad wordt semantisch opgelost; het is geen simpele zoek-en-vervangactie.

## VFS-contract en bescherming

- Mounts hebben expliciete flags voor lezen/schrijven en uitvoeren.
- Resolutie controleert rechten bij elk relevant object; directoryhandles ondersteunen relatieve en racebestendige operaties.
- Canonicalisatie en symlinkresolutie mogen namespaces of toegangsgrenzen niet verlaten zonder bevoegdheid.
- Metadata bewaart waar nodig oorspronkelijke namen, casebeleid, ACL's en gastattributen. De eerste ext2-driver ondersteunt niet automatisch alle gastmetadata.
- Niet-ondersteunde metadata wordt in een gecontroleerde objectgebonden store beheerd of de operatie faalt. Voor M8 moet de gekozen subset aantoonbaar correct zijn; een onbetrouwbare losse sidecar is geen algemene oplossing.
- Verwijderen, open handles, rename en sharing worden centraal bemiddeld zodat verschillende appomgevingen elkaars locks niet onbedoeld omzeilen.
- De GUI mag namen vertalen; technische paden en app-ID's blijven stabiel.

## Toetsen

M4 test casebotsingen, Unicode, locks, rename, links, verwijdering met open handle en volle schijf. M8 test een echte installer en uninstall. Namespace-tests controleren dat `..`, links en drivealiases geen toegang buiten verleende roots geven. Herstarttests controleren stabiele paden en volume-ID's.

Alternatieven: één globale Windows-root maakt het eerste ecosysteem eenvoudiger maar koppelt alle apps aan gedeelde staat; een globale Unix-indeling verplicht Windows/macOS tot extra aanpassing. De gekozen VFS-weergaven kosten complexiteit, maar houden verschillen controleerbaar.
