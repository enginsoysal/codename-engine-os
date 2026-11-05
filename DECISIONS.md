# Ontwerpbesluiten

Status: baseline; 'gekozen' betekent richting voor implementatie, geen gemeten resultaat.

| ID | Keuze | Waarom | Alternatief en prijs | Validatie/heroverweging |
|---|---|---|---|---|
| ADR-001 | Eigen kernel en eigen systeemdiensten | Zelfstandig platform en controle over resources | Bestaande kernel versnelt hardwareondersteuning, maar voldoet niet aan de eigen-kerneldoelstelling | Producteis; wijzigen alleen als scopebesluit |
| ADR-002 | Modulaire kernel; zwaar beleid in usermode | Kleine vertrouwde basis en afzonderlijk uitbreidbare omgevingen | Zuivere microkernel kan isolatie vergroten, maar vraagt meer IPC; brede monolithische kernel vergroot foutimpact | M3/M10 meten IPC, latency en kopieën; geen prestatieclaims vooraf |
| ADR-003 | Omgevingsgebonden ABI-adapters | Windows/Linux/macOS verschillen in gedrag, niet alleen namen | Eén oppervlakkige universele API verliest noodzakelijke semantiek | M0-semantiekmatrix en M7 syscalltests |
| ADR-004 | Windows eerst; complete 7-Zip-route | Tastbaar bewijs van installer tot dagelijks gebruik | Vier ecosystemen tegelijk verspreidt inspanning en vergroot integratierisico | M8 vereist echte bestanden en correcte resultaten |
| ADR-005 | x86-64 UEFI als eerste platform | Past bij eerste Windows-doel en houdt bootmatrix beperkt | BIOS en ARM vanaf dag één vergroten driver- en testlast | ARM-grenzen bij M0; tweede architectuur bij M16 |
| ADR-006 | Native ABI eigen geversioneerd contract | Interne evolutie los van gastcompatibiliteit | Windows-ABI overal maakt latere omgevingen moeilijker onafhankelijk te modelleren | M0/M7 controleren voldoende expressiviteit |
| ADR-007 | Centrale objectrechten en appomgevingen | Compatibele API's mogen geen onbeperkte hostrechten betekenen | Volledige gedeelde systeemstaat is eenvoudiger, maar minder veilig en reproduceerbaar | M2/M8 negatieve toegangstests |
| ADR-008 | Gedeelde VFS met semantische adapters | Gedeelde opslag en behoud van verschil in bestandsgedrag | Alles als POSIX of alles als Windows behandelen breekt verwachtingen | M4 semantiekmatrix; geen stil verlies van metadata |
| ADR-009 | Bestaand gedocumenteerd opslagformaat eerst | Betrouwbare persistente opslag vóór nieuw filesystemonderzoek | Eigen formaat geeft vrijheid, maar voegt een zelfstandig integriteitsproject toe | M0 vergelijking; read-only eenvoud mag geen definitieve writable oplossing worden |
| ADR-010 | RAM-budgetten op echte baseline | Voorkomt willekeurige cijfers en misleidende idleclaims | Vooraf harde ongefundeerde getallen kunnen verkeerd gedrag belonen | M3 budgetten; elke latere milestone metingen |
| ADR-011 | Compressie/swap na correct VM-beheer | Extra lagen maskeren geen fouten in eigendom of paging | Vroeg toevoegen verhoogt deadlock- en dataverliesrisico | Druktests met beperkte geheugens en vertraagde I/O |
| ADR-012 | Componenthergebruik toegestaan | Eigen OS vereist niet elk protocol of font opnieuw schrijven | Alles zelf schrijven verhoogt onderhoud en tijd sterk | Voor iedere component licentie, afhankelijkheden, port en alternatief documenteren |
| ADR-013 | Eén systeem met live/install/herstelprofielen | Minder verschil tussen test- en gebruikersomgeving | Aparte installersystemen krijgen eigen drivers en drift | M5 installeert dezelfde geteste build |
| ADR-014 | Versiegebonden appcompatibiliteit | Succes is reproduceerbaar en regressies zijn lokaliseerbaar | 'App X werkt' zonder versie of taken is niet controleerbaar | Iedere appmijlpaal bevat versie, hash waar mogelijk, taken en uitsluitingen |
| ADR-015 | Geen volledige Windows-drivercompatibiliteit in eerste release | Gebruikersapps en hardwaredrivers zijn verschillende omvangrijke interfaces | Direct bestaande .sys-drivers draaien vraagt extra NT-kernelcontracten en veiligheidswerk | Apart besluit zodra een doelapp of apparaat dit vereist |
| ADR-016 | C17 + beperkte assembly, Clang/LLD | Eén aanwezige cross-toolchain en directe C-ABI-/portingroute | Rust vermindert bepaalde geheugenfouten maar vraagt extra toolchain/FFI; C vereist streng veiligheidswerk | ELF x64/ARM64 en EFI-formaat getoetst; runtime nog niet |
| ADR-017 | Eigen root met gastweergaven | Zelfstandige indeling plus bestaande apppaden | Eén globale Windows-/Unix-root koppelt alle omgevingen aan dezelfde semantiek | [Mappencontract](m0/FILESYSTEM.md); VFS-tests M4/M8 |
| ADR-018 | Geversioneerde code; data/cache/temp gescheiden | Beheer, herstel en duidelijke bewaartermijnen | Apps vrij laten schrijven in code vereenvoudigt sommige legacygevallen maar vergroot risico | Beheerde schrijflaag alleen waar nodig; uninstall-/rollbacktests |
| ADR-019 | Eigen beperkte UEFI-loader | Beheerst bootcontract binnen UEFI-onlyscope | Limine/GRUB verkleinen implementatiewerk; terugvaloptie bij disproportionele vertraging | EFI-formaat gebouwd; echte overdracht in M1 |
| ADR-020 | FAT32 EFI + ext2-ontwikkelvolume | Standaardboot en overzichtelijke eerste persistente opslag | NTFS/ext4/nieuw formaat groter; FAT32 onvoldoende algemeen | Featureallowlist, fsck en crashtests; production storage later apart |
| ADR-021 | QEMU q35/TCG + OVMF als kandidaat | Reproduceerbaar zonder specifieke hostversnelling | Host-passthrough moeilijker reproduceerbaar | Versiepin en bootcontrole nog open |
| ADR-022 | Procesgebonden ABI-decoders | Zelfde syscallnummer kan per ecosysteem iets anders betekenen | Alleen imports onderscheppen mist directe syscalls | Entry-/returnproeven in M2/M7 |
| ADR-023 | x86 32-bit via compatibility mode | Originele 32-bit Windows-apps op x86-64 | CPU-emulatie duurder; alleen 64-bit sluit relevante apps uit | Thunks, TLS en stacktests vóór Steam |
| ADR-024 | Conservatieve commit bij start | Geen geheugenbelofte zonder backing/reserve | Agressieve overcommit kan meer apps starten maar later onverwacht falen | M3 druk-/OOM-tests en latere beleidsherziening |

## Nog te besluiten vóór relevante implementatie

- M0: hoofdtaal, toolchainroute, bootloaderstrategie en ontwikkelfilesystem zijn gekozen; concrete tool-/firmwarepins, boot-/ABI-headerlayouts en aanvullende referentieproeven staan nog open.
- M0: eerste x86-64-testcomputer en beschikbare fysieke testmiddelen; onbekend blijft expliciet onbekend.
- M0: 32-bitstrategie en ABI-routing zijn beschreven; uitvoering blijft te bewijzen. Zie [kernelcontracten](m0/KERNEL-CONTRACTS.md).
- M3: eerste RAM-budgetten, commitbeleid en definitie van out-of-memoryherstel.
- M5: update-/rollbackformaat en installertransacties op het gekozen filesystem.
- M10: GPU-familie, driverbron en grafische API-route; portingonderzoek vóór productbelofte.
- M11–M15: doelversies, testaccounts, rechtmatig beschikbare installers/SDK's en externe dienstafhankelijkheden.
- M16: exact Pi-model en firmware-/bootmethode.

Een keuze wijzigen vereist een korte aanvulling: bewijs, effect op afhankelijkheden, migratie en regressietests. De roadmap mag bij nieuwe bevindingen worden verbeterd; een mislukte proef wordt niet als voltooiing geregistreerd.

## Hoe en waarom per nieuw besluit

De uitvoerige redenering, afgewezen alternatieven, implementatiestappen en toetscriteria staan in [bouw/platform](m0/BUILD-PLATFORM.md), [mappen/opslag](m0/FILESYSTEM.md), [kernelcontracten](m0/KERNEL-CONTRACTS.md) en [compatibiliteitsmatrix](m0/COMPATIBILITY.md). Deze tabel is de besluitindex; een 'gekozen' regel is geen claim van werkende implementatie.
