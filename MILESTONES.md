# Mijlpalen — werk, aanpak en bewijs

Versie 0.2 — 5 november 2026. Alle implementatiemijlpalen zijn nog te realiseren.

## Aangescherpte Windows-eis

De bestaande M0–M16 dekken boot en installatie, maar geen volledige ondersteuning van alle Windows 95–11-apps. De gebruiker heeft die brede appcompatibiliteit als productdoel vastgesteld. [WC1–WC7](WINDOWS-COMPATIBILITY.md) vullen de roadmap aan met generatie-/installerregister, Win16, Windows 9x/NT-profielen, runtimes/pakketten, multimedia, driver-/dienstafhankelijkheden en kwalificatie. Deze trajecten zijn niet gestart en geen universele foutloosheidsgarantie.

WC1 begint binnen M0; WC2–WC6 breiden vooral M7–M13 uit; WC7 geldt voor elke appmijlpaal. DOS in M9 dekt Win16 niet automatisch. M1/M5 blijven respectievelijk de boot- en installatiepoorten.

Dit is het ontwikkelcontract per mijlpaal. Voor uitvoering worden versiegebonden taken en fixtures toegevoegd; geen enkele doelapp wordt zonder integratietest als ondersteund beschouwd. Volgorde en scopewijzigingen worden gekoppeld aan [ontwerpbesluiten](DECISIONS.md).

## M0 — Architectuurcontracten en technische uitvoerbaarheid

**Doel en afhankelijkheden:** de eerste implementatie kan beginnen zonder tegenstrijdige systeemcontracten. Geen voorafgaande OS-code nodig.

**Wat moet gebeuren**

1. Leg kernelobjecten, rechten, eigendom, syscallconventies en ABI-versies vast.
2. Beschrijf bootprotocol, adreslayout, geheugenreserves, interruptmodel en platformgrenzen.
3. Vergelijk Windows-, Linux- en Mach-semantiek voor wachten, threads, exceptions/signalen, mappings en bestandsdeling.
4. Kies hoofdtaal, cross-toolchain, bootloaderaanpak en filesystem voor het eerste schrijfbare systeem.
5. Leg QEMU/UEFI-referentie vast en selecteer of noteer de nog benodigde fysieke testcomputer.
6. Bepaal 32-bitroute, compatibiliteitscomponenten en licentie-/portingvoorwaarden.

**Hoe:** schrijf kleine ABI-proefprogramma's en specificaties voor directe syscalls, TLS, threadstart en geladen bibliotheken. Tijdens implementatie worden die op referentieomgevingen onderzocht; voor deze documentatiefase wordt nog niets gebouwd. Maak per kandidaatcomponent een afhankelijkhedenkaart. Beschrijf eerst de interne contracten, daarna de vertaling per omgeving.

**Waarom:** een desktop bouwen op ongeschikte proces- of geheugensemantiek veroorzaakt later een dure herbouw. Een kleine uitvoerbaarheidsproef is goedkoper dan een ongevalideerde universele abstractie.

**Acceptatie/bewijs:** contractdocumenten zonder open fundamentele conflicten; beslismatrix taal/toolchain/filesystem; geteste buildomgeving zodra uitvoering start; proefresultaten of expliciete blockers voor de ABI-routes. Secundaire productkeuzes mogen open blijven met eigenaar en beslismoment.

**Risico/uitweg:** als een primitieve onvoldoende blijkt, eerst het objectmodel corrigeren. Niet met app-specifieke hacks verbergen.

**Afsluiting op 5 november 2026:** M0 afgerond met [51 technische controles en document-/inventariscontrole](evidence/M0/acceptance-results.json). Boot via firmware is getoetst met een duidelijk gelabelde EFI-omgevingsfixture, niet de toekomstige kernel. [Overdracht naar M1](m0/M1-HANDOFF.md) benoemt de nog te implementeren runtimepoorten. Concrete Wine/Darling-revisions worden gekozen vóór een daadwerkelijke port; Linux/Mach-runtimeproeven blijven M14/M15. De Windows 95–11-scope is uitgewerkt in WC1–WC7 en de generatie-inventaris.

## M1 — Bootbare kernel en diagnostiek

**Doel/afhankelijkheden:** M0; een minimale eigen kernel start reproduceerbaar via USB-image en ISO.

**Wat:** maak cross-build, linkerlayout, bootloader, initimage en imageverpakking. Initialiseer stack, geheugenoverdracht, exceptiontabellen en seriële uitvoer. Voeg panicpad, symbolen, debuggeraansluiting en build-id toe. Definieer eigendom van alle bootbuffers.

**Hoe:** eerst QEMU met UEFI en seriële logging. Laat de loader de kernel laden en Boot Services correct beëindigen. Valideer de ontvangen geheugenkaart voordat allocatie begint. Maak aparte taken voor USB-diskimage en UEFI-ISO met dezelfde kernelinhoud.

**Waarom:** alle volgende functies hebben betrouwbare opstart- en foutdiagnostiek nodig. Een vroege USB/ISO-route voorkomt dat installatie later een los project wordt.

**Acceptatie:** beide artefacten starten via hun bedoelde medium in QEMU; kernel toont dezelfde build-id; een bewust veroorzaakte exception levert adres en diagnose; ongeldige bootinformatie wordt afgewezen. Eén fysieke UEFI-boot wordt apart geregistreerd zodra testhardware beschikbaar is. Tot die tijd wordt uitsluitend virtuele boot als bewezen gemeld.

**Bewijs/risico:** buildmanifest, imagehashes, startcommando's en seriële logs. Geen afhankelijkheid van firmwaregeheugen dat na overdracht kan worden hergebruikt. Secure Boot en Legacy BIOS zijn hier geen voltooiingsvereisten.

## M2 — Geïsoleerde processen en kernelobjecten

**Doel/afhankelijkheden:** M1; eigen programma's draaien buiten kernelmode.

**Wat:** bouw minimale fysieke allocator, pagina-tabellen, aparte adresruimtes, usermode-entry, proces-/threadobjecten, handlelijsten en een kleine eigen syscall-ABI. Voeg basis-IPC en gecontroleerd starten/stoppen toe.

**Hoe:** gebruik aanvankelijk een eenvoudig eigen programmaformaat of beperkt ELF-profiel als interne testloader; dit is nog geen Linux-compatibiliteit. Kopieer of valideer alle userpointers. Laat objectrechten bepalen of een proces toegang krijgt, met gecontroleerde overdracht tussen processen.

**Waarom:** geheugenbescherming en levensduurbeheer zijn voorwaarden voor veilige installers en compatibiliteitsservers. Een app mag het OS niet kunnen beschadigen met een foutief adres.

**Acceptatie:** twee processen hebben geïsoleerde privédata; gedeeld geheugen werkt alleen expliciet; toegang tot kernelgeheugen faalt; foutieve pointers en handles veroorzaken een gecontroleerde fout; een gecrashte app wordt opgeruimd terwijl de andere doorwerkt.

**Bewijs/risico:** negatieve beveiligingstests, herhaalde start/stopmetingen en logs. Houd kernelallocaties en achterblijvende objecten bij. Geen volledige scheduler of demand paging vereist; minimale allocatie hoort al hier, geavanceerd geheugen volgt in M3.

## M3 — Geheugen, scheduling en concurrency

**Doel/afhankelijkheden:** M2; stabiele uitvoering met meerdere threads en CPU-kernen, plus controleerbaar resourcegebruik.

**Wat:** implementeer preëmptie, timers, meerdere kernen, prioriteiten, atomair wachten/wekken, time-outs en lockregels. Breid VM uit met reservering/commit, mappings, copy-on-write en gecontroleerde paginafaults. Voeg geheugenaccounting, drukmeldingen, noodreserves en OOM-beleid toe.

**Hoe:** begin met deterministische threadscenario's en ga daarna naar stressbelasting. Gebruik monotone tijd voor deadlines. Test races rond stopzetten, annuleren en objectsluiting. Introduceer file-backed demand paging na M4; in deze fase zijn anonieme faults en gesimuleerde paginabronnen voldoende. Compressie en echte swap volgen als aparte werkpakketten zodra opslag betrouwbaar is.

**Waarom:** Windows-, Linux- en macOS-gedrag kunnen alleen betrouwbaar worden ondersteund op correcte geheugentoegang en synchronisatie.

**Acceptatie:** geen verloren wake-ups in de vastgelegde stresssuite; copy-on-write scheidt wijzigingen correct; geheugenuitputting wordt gecontroleerd afgehandeld; de basis blijft reageren op meerdere RAM-configuraties. Stel daarna eerste budgetten vast voor kernel, console en systeemdiensten.

**Bewijs/risico:** p50/p95-latency, testduur, CPU-configuratie en geheugenbalans na opruimen. Een test kan races vinden, maar bewijst geen afwezigheid ervan; code-review van lockvolgorde en eigendom blijft nodig.

## M4 — Persistente opslag, hardwaredetectie en invoer

**Doel/afhankelijkheden:** M3; data blijft correct bewaard en basishardware is bruikbaar.

**Wat:** implementeer busdetectie, driverregistratie, block-I/O, gekozen filesystem, VFS, buffering en flushgedrag. Voeg USB-controllerondersteuning, toetsenbord en muis toe. Ondersteun eerst virtuele opslag, daarna de gekozen echte opslagcontroller. Werk bestandssemantiek en metadata uit.

**Hoe:** scheid bus, driver en appinterface. Gebruik begrensde I/O-wachtrijen en gecontroleerde DMA. Test bestanden met bekende inhoudshashes. Beschrijf per VFS-operatie de verwachte atomiciteit en herstelbaarheid. Koppel file-backed mappings en demand paging aan de opslagroute.

**Waarom:** de installer, registry, apps en gebruikersdocumenten hebben betrouwbare persistente opslag nodig. Detectie zonder werkende datatransfer telt niet als driverondersteuning.

**Acceptatie:** bestanden maken, lezen, wijzigen en hernoemen blijven na reboot correct; disk-full en I/O-fouten zijn beheerst; geïnjecteerde onderbrekingen leveren het gedocumenteerde herstelgedrag. USB-invoer werkt op de gekozen configuratie. Unsupported hardware meldt helder dat een driver ontbreekt.

**Bewijs/risico:** gegevenshashes, reboot- en foutinjectielogs, hardwarelijst en mounttests. Begin met wegwerpimages; fysieke schijftests alleen op het expliciete testdoel. Geen claim van universele USB- of NVMe-ondersteuning op basis van één apparaat.

## M5 — Live omgeving, installer en herstel

**Doel/afhankelijkheden:** M4; gebruiker kan vanaf USB/ISO installeren en zonder medium verder booten.

**Wat:** maak liveprofiel, installatieprogramma, schijfinspectie, GPT-/partitiehandelingen, EFI-systeempartitie, OS-plaatsing en bootregistratie. Voeg een minimale herstelmodus toe. Leg een updateformaat met terugvalpad vast.

**Hoe:** de eerste installer mag tekstgebaseerd zijn. Toon doelapparaat en geplande wijzigingen vóór schrijven; voer preflight op ruimte en beschikbare drivers uit. Kopieer een gecontroleerde build en verifieer inhoud. Een tijdelijk live-schrijfgebied mag caches of documenten niet stilzwijgend persistent laten lijken. Gebruik voor de eerste installatie een aparte testschijf; behoud van andere OS-installaties is later een eigen testpakket.

**Waarom:** installatie is onderdeel van het platform. Dezelfde kernel en drivers voor live en geïnstalleerd gebruik beperken verschillen.

**Acceptatie:** USB-image en ISO starten live; installatie naar een lege virtuele schijf slaagt; na verwijderen van het medium start die schijf zelfstandig. Verkeerde doelkeuze kan vóór schrijven worden afgebroken. Gesimuleerde kopieerfouten worden gemeld; herstel kan de bootconfiguratie diagnosticeren. Voer later dezelfde route op de referentiecomputer uit.

**Bewijs/risico:** installatielog, partitieoverzicht vóór/na, bootbewijs zonder medium en minimaal één hersteltest. Atomiciteit van OS-updates wordt apart getest, niet afgeleid uit een geslaagde eerste installatie.

## M6 — Eigen desktop, netwerk en downloads

**Doel/afhankelijkheden:** M5; bruikbare basis om de eerste Windows-installer op te halen.

**Wat:** bouw compositor met framebufferroute, vensters, invoerroutering, fonts, focus en klembord. Voeg shell/bestandsbeheer en appstart toe. Implementeer één netwerkdriver, IP, DNS en HTTPS met betrouwbare certificaat- en tijdafhandeling; hergebruik een geschikte TLS-bibliotheek waar passend.

**Hoe:** begin met softwareweergave en expliciete buffers. Maak een downloadvoorziening met voortgang, foutmelding en doelpad; een complete browser is hier geen vereiste. Scheid netwerkprotocolstack van driver en app-API. Beperk geheugencaches en meet responstijd tijdens I/O.

**Waarom:** een minimaal dagelijks bruikbare desktop bewijst de samenhang zonder dat een browserengine een nieuwe hoofdafhankelijkheid wordt.

**Acceptatie:** gebruiker navigeert naar documenten, opent meerdere eigen vensters en downloadt een bekend bestand via HTTPS; inhoud klopt; ongeldige certificaten worden afgewezen; netwerkverlies is herstelbaar. Desktop blijft reageren tijdens downloads en geheugendruk.

**Bewijs/risico:** functionele sessie, netwerkfouttests en idle-/belastingsmeting. Wi-Fi, volledige toegankelijkheid en brede taalondersteuning krijgen vervolgwerkpakketten; basis-Unicode en een uitbreidbare tekstroute worden niet uitgesteld.

## M7 — Windows-binarybasis

**Doel/afhankelijkheden:** M3/M4/M6; echte ongewijzigde Windows-programma's draaien boven onze kernel.

**Wat:** implementeer PE-loader, imports/relocaties, DLL-resolutie, TLS, exceptionpad en Windows-processtart. Bouw de benodigde NT-/Win32-basis voor geheugen, bestanden, threads, synchronisatie en omgevingsdata. Voeg omgevingsgebonden syscallrouting toe. Begin de registry- en service-infrastructuur waar nodig.

**Hoe:** vergelijk kleine referentieprogramma's op Windows met dezelfde binaries op ons OS. Leg per functie parameters, resultaat, foutcodes en randgevallen vast. Bepaal expliciet welke Wine-onderdelen te porten zijn en welke hostafhankelijkheden eigen implementaties vragen. Test 64-bit eerst en voer de M0-strategie voor 32-bit uit vóór Steam.

**Waarom:** direct werkende machinecode heeft een correcte omgeving nodig. Een loader alleen bewijst geen compatibiliteit.

**Acceptatie:** benoemde, ongewijzigde consolebinaries doorlopen bestandstaken en multithreading met correcte resultaten; DLL- en foutpaden werken; directe syscallproeven komen bij de juiste adapter; appcrashes blijven geïsoleerd. Niet-geïmplementeerde functies leveren diagnose in plaats van ongemerkt succes.

**Bewijs/risico:** binaire hashes, referentie-uitvoer en API-dekkingsrapport. Porten van Wine is geen drop-in: afhankelijkheden van andere kernels en runtimes worden per onderdeel behandeld.

## M8 — 7-Zip: downloaden, installeren en gebruiken

**Doel/afhankelijkheden:** M7; eerste complete gebruikersroute met een originele Windows-installer.

**Wat:** pin exacte 7-Zip-versie en architectuur. Voeg benodigde GUI-, registry-, pad-, installer- en shellfuncties toe. Integreer snelkoppeling en starten vanuit bestandsbeheer. Houd wijzigingen per appinstallatie bij.

**Hoe:** traceer installer en GUI; implementeer ontbrekende algemene functies met herbruikbare semantiek. Gebruik originele, rechtmatig verkrijgbare installatiebestanden. Werk met een dataset met kleine/grote bestanden, Unicode-namen, lege bestanden en meerdere archieftypen. Download via M6, niet via een vooraf verborgen geïnstalleerde app.

**Waarom:** deze route bewijst loader, GUI, opslag en configuratie samen. Een aangepaste demo zou belangrijke problemen omzeilen.

**Acceptatie:** downloaden → installeren → GUI starten → archief maken → uitpakken → hashes vergelijken → OS herstarten → opnieuw gebruiken → verwijderen. Beschadigde archieven, onvoldoende ruimte en beperkte rechten leveren bruikbare fouten. Buiten het doelgebied mag verwijderen geen data verliezen.

**Bewijs/risico:** taakverslag, exacte installer, vergelijkingshashes, benodigde API's en RAM/latency. Het resultaat ondersteunt deze versie; bredere 7-Zip-ondersteuning vraagt aanvullende tests. Na M8 wordt het eerste externe-appcompatibiliteitsprofiel gepubliceerd.

## M9 — Geïntegreerde DOS-omgeving

**Doel/afhankelijkheden:** M4/M6; aparte DOS-runtime zonder de hostveiligheid op te geven.

**Wat:** kies DOS- en x86-emulatiecomponent of eigen implementatiescope; ondersteun COM/EXE, virtuele schijf, invoer, timers, beeld en geluid. Inventariseer real-mode, protected-mode/DOS-extender en historische hardwarebehoeften per spel.

**Hoe:** draai de omgeving als geïsoleerd hostproces. Map een gekozen gamemap als virtuele schijf. Emuleer benodigde hardware en begrens toegang tot echte bestanden. Per-gameprofielen bevatten CPU-tempo, geluid en toetsenindeling. Gebruik games die rechtmatig beschikbaar zijn.

**Waarom:** oude DOS-programma's verwachten hardware en een geheugenmodel die een moderne desktopkernel niet rechtstreeks als host mag aanbieden.

**Acceptatie:** geselecteerde games starten vanuit de desktop; invoer, audio, timing, save/load en afsluiten werken. Een runaway game kan worden beëindigd zonder hostcrash; toegang buiten de gemapte map faalt. Kies waar mogelijk zowel een eenvoudig als een DOS-extenderdoel.

**Bewijs/risico:** titels/versies, runtimeprofielen en save-tests. Software-emulatie is hier bewust; dit wordt niet als directe native uitvoering beschreven.

## M10 — GPU-versnelling en grafische API-route

**Doel/afhankelijkheden:** M3/M4/M6; versnelde graphics op één expliciet ondersteunde GPU-route.

**Wat:** selecteer GPU-familie, driverbron, firmwarevoorwaarden en grafische basis-API. Implementeer command submission, GPU-adresruimtes, buffers, fences, residency en foutafhandeling. Port of bouw benodigde userspace-driver en shadercompiler. Kies een route voor de Direct3D-versies van de eerste games.

**Hoe:** voer vóór driverimplementatie een afhankelijkhedenonderzoek uit. Begin met een klein renderprogramma en lees beelden terug voor vergelijking. Koppel compositor daarna aan versnelling. Voeg Direct3D-vertaling pas toe wanneer de onderliggende API voldoende correct is. Gebruik Mesa-/vertalercomponenten alleen na concrete portanalyse.

**Waarom:** CPU-rendering kan een desktop dragen, maar is geen realistische algemene basis voor moderne games. Eén bewezen GPU is waardevoller dan oppervlakkige detectie van veel kaarten.

**Acceptatie:** correcte testbeelden, stabiele synchronisatie, begrensd geheugengebruik en langdurige rendering op de gekozen GPU. Een app met ongeldige opdrachten mag geen onbeperkte systeemtoegang krijgen. GPU-hangs worden gedetecteerd; herstelgrenzen worden vastgelegd. Meet CPU/GPU-tijd en framevariatie.

**Bewijs/risico:** apparaat-/driverversie, API-dekking en render-/druktests. Virtuele GPU-tests vervangen geen bewijs op fysieke hardware. Bij ontbrekende firmware of documentatie wordt de hardwarekeuze herzien.

## M11 — Steam-client

**Doel/afhankelijkheden:** M8, benodigde 32-bitondersteuning en een bruikbare grafische route. M10 is productmatig gewenst; de precieze technische minimumroute wordt door clientonderzoek bepaald.

**Wat:** selecteer clientbuild en ondersteun installer, webinterface, helpers, services, beveiligde verbindingen, fonts, login en updates. Maak account- en downloadstate persistent.

**Hoe:** onderzoek processen en hun afhankelijkheden; voeg ontbrekende algemene Windows-functies toe. Test met een daarvoor beschikbaar account zonder credentials in logs op te nemen. Houd clientondersteuning gescheiden van gameondersteuning. Zelfupdates krijgen een regressietest en expliciete versieadministratie.

**Waarom:** Steam is een appplatform met veel subprocessen en netwerkgedrag. Een zichtbaar loginvenster bewijst nog geen bruikbare client.

**Acceptatie:** officiële installer werkt; gebruiker logt in; bibliotheek en winkelinterface laden voor de afgesproken taken; downloaden, pauzeren, hervatten en updaten werken; na reboot blijft de juiste staat behouden. Afmelden en verwijderen werken zonder achterblijvende actieve helpers.

**Bewijs/risico:** clientversie, takenmatrix, proces-/RAM-meting en updategedrag. Externe dienstwijzigingen kunnen nieuwe implementatie vragen. MFA en accounttoestemming blijven gebruikershandelingen; geen verzonnen of omzeilde toegang.

## M12 — Eerste Steam-games

**Doel/afhankelijkheden:** M10/M11; een kleine benoemde selectie spellen is daadwerkelijk speelbaar.

**Wat:** kies eerst eenvoudige titels zonder noodzakelijke kernel-anticheat, daarna een zwaardere 3D-titel. Ondersteun per titel graphics, audio, controllers, fullscreen, saves en eventuele runtimes. GTA V kan later een expliciet afzonderlijk doel worden; editie en gebruiksmodus moeten dan vastliggen.

**Hoe:** leg hardware, resolutie, instellingen en testscène vast. Maak onderscheid tussen starten, bruikbaar spelen en afgesproken prestaties. Test langere sessies en geheugen-/VRAM-druk. Pak API-correctheid aan vóór app-specifieke workarounds.

**Waarom:** speelbaarheid is een keten van systemen. Gemiddelde FPS verbergt haperingen en correcte rendering alleen zegt niets over saves of input.

**Acceptatie:** afgesproken sessie zonder bekende kritieke fout; save/load na reboot; correcte invoer en audio; vooraf gekozen frame-time- en geheugendoelen gehaald op benoemde hardware. Anticheat-/DRM-blokkades worden per titel vermeld, niet als opgelost aangemerkt door enkel offline te testen.

**Bewijs/risico:** gamebuild, instellingen, frametimes, sessielog en compatibiliteitsprofiel. Gebruikelijke updates kunnen regressies geven; oude bewezen versies blijven als referentie herkenbaar.

## M13 — Eerste Adobe-pakket

**Doel/afhankelijkheden:** M8 plus de werkelijk benodigde netwerk-, GUI- en GPU-functies. Steam zelf is geen technische voorwaarde.

**Wat:** kies één pakket en versie; beschrijf installatie, identiteit/licentie, helperdiensten, fonts, kleurbeheer, GPU en bestanden. Voor Photoshop worden bewerkingstaken en testdocumenten vooraf gekozen. Andere Adobe-pakketten volgen afzonderlijk.

**Hoe:** onderzoek installatie en startketen met legitieme toegang. Bouw ontbrekende generieke functies; test eerst CPU-routes waar toegestaan en daarna GPU-paden. Houd kleurprofielen en uitvoerformaten onderdeel van correctheid. Externe licentie-/dienstbeperkingen zijn een expliciete haalbaarheidspoort.

**Waarom:** een geopende editor bewijst niet dat het product bruikbaar is. Opslaan, opnieuw openen en correcte uitvoer zijn belangrijker dan alleen de splashscreen.

**Acceptatie:** installeren, rechtmatig activeren, document openen, afgesproken bewerkingen uitvoeren, exporteren, opslaan en opnieuw openen. Resultaten worden inhoudelijk vergeleken; niet elk formaat vereist een byte-identieke hash. Metingen bevatten piekgeheugen, tijdelijke opslag en taaklatency.

**Bewijs/risico:** pakketversie, taakmatrix, voorbeeldresultaten en bekende beperkingen. Als noodzakelijke externe toegang niet beschikbaar is, wordt deze mijlpaal geblokkeerd op die concrete afhankelijkheid; geen belofte dat kernelwijzigingen dit altijd oplossen.

## M14 — Linux-binaries

**Doel/afhankelijkheden:** stabiele M3/M4/M6-basis en ABI-infrastructuur; onafhankelijk van M13 uitvoerbaar.

**Wat:** implementeer ELF-start, dynamische linkeromgeving, Linux-syscalls, TLS, signalen, futex-achtig wachtgedrag en benodigde pseudo-filesystems. Voeg sockets, procesrelaties en bestandstoestanden toe voor de gekozen software. Leg libc-/distributieprofiel vast.

**Hoe:** begin met statische CLI-binaries en voeg daarna dynamische binaries toe. Gebruik ongewijzigde programma's uit een vastgelegd profiel; een hercompileerde eigen POSIX-demo telt niet als Linux-binaire compatibiliteit. Breid vervolgens uit naar GUI-software met een expliciete displayprotocolroute.

**Waarom:** POSIX is niet hetzelfde als de Linux-ABI. Afzonderlijke adapters behouden beide verwachtingen zonder de eigen kernel als Linux te presenteren.

**Acceptatie:** benoemde statische en dynamische CLI-programma's werken, inclusief pipes, signalen, threads en bestanden; een later subpakket bewijst één ongewijzigde GUI-app. Linux- en Windows-apps kunnen gelijktijdig draaien zonder syscallverwarring of ongewenste toegang tot elkaars configuratie.

**Bewijs/risico:** binary/libc-versies, syscallmatrix en referentieresultaten. Willekeurige distro-pakketten ondersteunen is geen automatisch gevolg van een werkende ELF-loader.

## M15 — macOS-binaries en Xcode-traject

**Doel/afhankelijkheden:** core, desktop, ABI-routing en herbruikbare POSIX-functies. Linux-compatibiliteit kan helpen, maar hoeft geen runtime-onderlaag te zijn.

**Wat:** implementeer Mach-O, laadgedrag, Mach-communicatie, Darwin-interfaces en de frameworks van geselecteerde apps. Vergelijk hergebruik van Darling-componenten met de afhankelijkheden van onze eigen kernel. Leg doelarchitectuur en macOS-/SDK-generatie vast.

**Hoe:** splits op in M15a CLI, M15b GUI-frameworks en M15c Xcode-onderzoek. Voor Xcode volgen aparte poorten: starten/editor, project laden, build, debugger, en eventueel simulator. Beschikbaarheid en gebruiksvoorwaarden van tools/SDK's worden onderzocht vóór distributieontwerp.

**Waarom:** Xcode is een samenstel van tools en frameworks; één alles-of-niets-taak zou voortgang en blockers verbergen. Externe Apple-diensten blijven buiten onze controle.

**Acceptatie:** M15a/b bewijzen benoemde ongewijzigde apps. Xcode geldt alleen voor de expliciet geteste versie en taken. Een compiler die los werkt telt niet als werkend Xcode; een editor zonder build evenmin. Simulator- en distributieondersteuning krijgen eigen resultaten.

**Bewijs/risico:** afhankelijkhedenkaart, versies, frameworkdekking en taakresultaten. Als noodzakelijke tooling of externe diensten niet beschikbaar zijn, wordt de betreffende submijlpaal concreet begrensd; geen volledige Xcode-belofte vooraf.

## M16 — ARM64 en een gekozen Raspberry Pi

**Doel/afhankelijkheden:** stabiele core; platformgrenzen vanaf M0. Start technisch mogelijk vóór M15, na voldoende stabiliteit van M3/M4.

**Wat:** implementeer ARM64-entry, pagina-tabellen, exceptions, timers, interrupts, atomics en contextswitch. Voeg platformbeschrijving en drivers toe. Kies exact Pi-model, firmwareketen, opslag en beeldroute. Maak een apart bootimage.

**Hoe:** eerst QEMU ARM64 met eigen usermodeprogramma's, dan fysieke Pi. Houd platformcode buiten algemene geheugen- en proceslogica. Controleer verschillen in memory ordering, DMA en cachecoherentie. Meet RAM vanaf boot en kies daarna de bruikbare console-/desktopconfiguratie.

**Waarom:** portability vereist gedeelde mechanismen met echte platformimplementaties. Een x86-64-ISO werkt niet vanzelf op Raspberry Pi.

**Acceptatie:** dezelfde codebasis boot virtueel en op het gekozen Pi-model; opslag en invoer werken; native ARM64-apps draaien; afgesproken lichte omgeving voldoet aan het gemeten budget en blijft stabiel onder druk. Appcompatibiliteit wordt opnieuw per architectuur bewezen.

**Bewijs/risico:** model, firmware, imagehash, bootlogs en RAM-/taakmetingen. x86-Windows-apps vereisen op ARM een afzonderlijke instructievertaler; dit is geen impliciet resultaat van M16. Een Pi zonder voldoende driverondersteuning wordt niet als ondersteund gepubliceerd.

## Controles die door alle relevante mijlpalen heen lopen

| Onderwerp | Vanaf | Vereist bewijs |
|---|---|---|
| Reproduceerbare builds | M1 | Toolchain, commando's, build-id en artefacthashes |
| Crashdiagnostiek en regressies | M1 | Geautomatiseerde bootchecks en symbolische foutinformatie |
| Isolatie en rechten | M2 | Negatieve tests en gecontroleerde procesopruiming |
| Resourcegebruik | M2/M3 | Privé/gedeeld/cache/kernel apart; baseline en belasting |
| Storage-integriteit | M4 | Reboot, disk-full, foutinjectie en herstelgrenzen |
| Installatie en updateherstel | M5 | Zelfstandig booten en aantoonbaar terugvalpad |
| Appcompatibiliteit | M7 | Versie, architectuur, taken, beperkingen en referentie |
| Echte hardware | Zodra beschikbaar | Apparaatlijst, firmware en fysieke testlogs |

RAM-compressie, swap, Secure Boot, brede Wi-Fi, energiebeheer, slaapstand, encryptie, toegankelijkheid en verdere hardwaredekking vragen aanvullende werkpakketten. Zij verdwijnen niet uit het product door deze eerste roadmap; vóór een dagelijkse desktoprelease wordt hun vereiste scope expliciet vastgesteld. Deze planning beschrijft eerst een ontwikkelbaar OS en compatibiliteitsbewijs, geen reeds complete consumentenrelease.
