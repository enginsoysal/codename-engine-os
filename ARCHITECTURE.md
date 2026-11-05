# Architectuur

Status: ontwerpbaseline 0.2; M0 in uitvoering. Besluiten hebben expliciete validatiepoorten. Zie [M0-contracten en resultaten](m0/README.md).

## Systeemlagen

```text
Eigen apps       Windows PE       Linux ELF       macOS Mach-O       DOS
    |                 |                |                |             |
Eigen runtime    Windows-omgeving  Linux-omgeving  macOS-omgeving  DOS-runtime
    +-----------------+----------------+----------------+-------------+
                                  |
             Systeemdiensten: bestanden, netwerk, audio, desktop,
             identiteit, installatie, configuratie en appintegratie
                                  |
                       Eigen geversioneerde kernel-ABI
                                  |
              Kernelobjecten, VM, threads, scheduler, IPC,
              synchronisatie, toegangscontrole, interrupts
                                  |
                  Platformcode en modulaire drivers
                                  |
                               Hardware
```

Een uitvoeringsomgeving is een verzameling loaders, bibliotheken, eventueel servers en beperkte kerneladapters. Het is geen volledige VM met een ander OS. DOS en toekomstige instructievertaling vormen expliciete uitzonderingen waar CPU- of apparaatemulatie nodig is.

## Kernel en objectmodel

Een kleine modulaire kernel bevat fysiek/virtueel geheugenbeheer, threads, scheduling, timers, IPC en beveiligde objecttoegang. Procesgebonden handles verwijzen naar objecten met rechten en gecontroleerde levensduur. Referentietelling, overdracht, sluiten en procesbeëindiging hebben vastgelegde regels; voor cycli en complexe grafen is een afzonderlijk eigendomsmodel nodig.

Interne handles zijn onforgeerbaar via de proces-handlelijst. Gastinterfaces mogen eigen zichtbare handlewaarden en foutcodes gebruiken. Drivers, kernelcode en systeemservers krijgen zo weinig mogelijk rechten. Een gesandboxte app krijgt expliciete toegang tot documenten of apparaten; vertrouwde systeeminstallatie gebruikt een bemiddelde verhoging van rechten.

Kritische objectsemantiek: atomair wachten en wekken, meerdere wachtobjecten, time-outs op monotone tijd, annulering, gedeelde mappings en procesopruiming. Geen universele primitieve wordt aangenomen voordat de semantiek van de doelomgevingen is vergeleken.

## Proces- en ABI-model

Het Windows-doel omvat het Windows 95–11-tijdperk. Daarom komen naast PE32/PE32+ ook Win16/NE, gemengde installerketens en generatiegebonden Windows 9x-/NT-semantiek expliciet in de scope. Win16 wordt als geïsoleerde legacyomgeving onderzocht; DOS alleen is onvoldoende. Moderne runtimes/pakketten en appgebonden kerneldrivers vragen afzonderlijke contracten. Zie [Windows-scope en aanvullende trajecten](WINDOWS-COMPATIBILITY.md). Geen van deze aanvullingen is reeds geïmplementeerd.

De loader stelt formaat, architectuur, ABI, runtime en afhankelijkheden vast. PE/ELF/Mach-O herkennen is slechts het begin. Per proces wordt een uitvoeringsomgeving geregistreerd. Syscall-entry behandelt dat proces volgens zijn ABI; nummers van verschillende omgevingen mogen niet botsen.

Kernelafhandeling verzorgt alleen noodzakelijke mechanismen. Complex beleid wordt naar de juiste server of bibliotheek gestuurd. Directe syscalls, TLS, exceptions/signalen, stacklayout, geheugenadressen, threadstart en herintreding worden apart gevalideerd. Geen methode die alleen DLL-imports onderschept wordt als volledige binaire compatibiliteit beschouwd.

32-bit Windows- en oude DOS-software worden expliciet gepland; x86-64 alleen dekt deze niet automatisch. Het 32-bit-adresmodel en ondersteuningsstrategie worden in M0 vastgelegd, met uitvoeringstests vóór Steam.

## Geheugen

Geheugenobjecten vormen de basis voor anonieme mappings, bestanden, gedeelde code en copy-on-write. Allocatie, virtuele reservering, commit en resident fysiek geheugen zijn verschillende begrippen. Overcommitbeleid en out-of-memorygedrag worden expliciet ontworpen.

De kernel kan caches terugwinnen zonder appdata stilzwijgend te verliezen. Geheugendruk wordt gesignaleerd aan diensten. Paginafaults mogen niet tot deadlocks leiden wanneer hun afhandeling zelf geheugen of opslag nodig heeft; noodreserves en lockvolgorde zijn onderdeel van het ontwerp. Compressie en swap volgen pas na correct basisgedrag.

VRAM blijft een eigen resourceklasse met driverafhankelijke residency, fences en DMA-regels. Een overkoepelend budget is mogelijk, maar RAM, VRAM en opslag zijn niet zonder voorwaarden uitwisselbaar.

## Bestanden en identiteit

De door de gebruiker goedgekeurde eigen hoofdstructuur is `/System`, `/Applications`, `/Users`, `/Data`, `/Cache`, `/Logs`, `/Temporary` en `/Volumes`. Er is geen verplichte native `C:` of `/var`. Fysieke schijven en partities blijven bestaan achter de VFS; extra opslag wordt op volume-ID gekoppeld.

Windows-apps krijgen een omgevingsgebonden `C:\Windows`, `C:\Program Files`, `C:\ProgramData` en `C:\Users` te zien. Linux-apps krijgen waar nodig `/usr`, `/etc`, `/var`, `/home` en virtuele systeeminterfaces; macOS-apps krijgen hun eigen systeem- en bibliotheekpaden. Gebruikersdocumenten worden gedeeld via objecten en rechten, niet gekopieerd per ecosysteem. Native namen zijn hoofdlettergevoelig; gastlookup volgt de expliciet ondersteunde gastsemantiek.

Systeem- en appcode zijn normaal alleen-lezen en geversioneerd. Configuratie, permanente appdata, cache en tijdelijke bestanden zijn gescheiden. Gedeelde appomgevingen zijn mogelijk voor onderling afhankelijke apps. Zie [volledige indeling, mappings en redenen](m0/FILESYSTEM.md).

Een eigen VFS ondersteunt meerdere fysieke bestandssystemen. Omgevingsadapters implementeren Windows-paden, Linux-mounts of macOS-metadata zonder alle verschillen weg te poetsen. Rechten en identiteit worden gecontroleerd in een gemeenschappelijke autorisatielaag; gast-ACL's krijgen een expliciete mapping en geen automatische extra rechten.

Gevalgevoeligheid, Unicode, delen/vergrendelen, open-bestandverwijdering, atomair hernoemen, links en metadata worden als contracten beschreven. Apps delen gebruikersdocumenten via bemiddelde toegang, met afzonderlijke configuratie en systeemmappen waar nodig.

## Drivers en diensten

Busdetectie en protocolondersteuning worden gescheiden. Driverselectie kan klasse, protocolversie en apparaatidentiteit vereisen. De app ziet mogelijkheden, niet fabrikantdetails. DMA vraagt pinning en adresvalidatie; hardware-isolatie wordt gebruikt waar beschikbaar.

Systeembeleid, desktop en compatibiliteitsservers draaien in usermode. Kleine, noodzakelijke drivers mogen aanvankelijk in kernelmode draaien. Usermode-drivers zijn mogelijk waar interrupts, latency en DMA dat veilig toelaten; volledige isolatie wordt niet zonder implementatiebewijs geclaimd.

## Desktop en graphics

Eén compositor beheert vensters en invoer. Omgevingen vertalen hun vensters naar dit model, inclusief focus, schaling, klembord en toegankelijkheid. Framebufferweergave is de eerste route. GPU-versnelling, shadercompilatie, grafische API's en video zijn afzonderlijke uitbreidingen. Een API-vertaler vervangt geen hardwaredriver.

## Boot, installatie en herstel

Eerste platform: x86-64 UEFI, aanvankelijk zonder Secure Boot. Bootloader laadt kernel en initimage, verzamelt geheugenkaart/framebuffer/platforminformatie en beëindigt UEFI Boot Services met correcte overdracht. De kernel neemt geheugen, timers en drivers over; GOP levert geen blijvende algemene GPU-driver.

USB-diskimage en UEFI-ISO zijn afzonderlijke buildartefacten met eigen tests. Bootprotocol is geversioneerd en bevat structuurgroottes, adressen, eigendom en geldigheidsduur. Dezelfde kernel draait live, geïnstalleerd en in herstelmodus.

Installatie gebruikt expliciete doelkeuze, preflight, verifieerbaar kopiëren en bootregistratie. Updates bewaren een terugvalmogelijkheid en een compatibele herstelroute. Secure Boot vereist later een ontworpen vertrouwensketen; alleen een EFI-bestand maken is onvoldoende.

## Ontwikkeling en diagnostiek

M0 kiest C17 met beperkte assembly en Clang/LLD, een eigen beperkte UEFI-loader, FAT32 voor EFI en een beperkt ext2-profiel als eerste ontwikkelvolume. Ext2 zonder journal is geen definitieve crashbestendige productoplossing. QEMU q35/TCG met OVMF is het gekozen virtuele profiel; tools/firmware zijn nog niet als uitvoerbare referentie vastgepind. Zie [bouw- en platformkeuzes](m0/BUILD-PLATFORM.md).

Een reproduceerbare cross-build voorkomt afhankelijkheid van de host-ABI. De toolchain, compilerflags en gebruikte componentversies worden vastgelegd. Seriële logging, symbolische crashrapporten en een debuggerverbinding bestaan vóór de desktop. Hosttests controleren datastructuren; QEMU-tests controleren kernelgedrag; echte hardware controleert de grenzen van het virtuele platform.

## Technische bronnen

- [UEFI 2.11](https://uefi.org/sites/default/files/resources/UEFI_Spec_Final_2.11.pdf): boot en firmwarecontracten.
- [Wine API Guide](https://source.winehq.org/WineAPI/): referentie voor Windows-interfaceoppervlak.
- [Linux ABI](https://docs.kernel.org/admin-guide/abi.html): kernel/userspace-contracten.
- [Darling](https://github.com/darlinghq/darling): voorbeeld van Mach-O, Mach/Darwin en frameworkimplementaties.

Deze bronnen onderbouwen interfacecategorieën; zij valideren niet automatisch de voorgestelde eigen implementatie of doelapps.
