# Kernelcontracten — ontwerp v0

Deze contracten beschrijven gekozen mechanismen, geen reeds werkende implementatie. Gastsemantiek wordt nader getoetst in [de matrix](COMPATIBILITY.md).

## Objecten en rechten

Types: process, thread, address-space, memory-object, channel, event, timer, directory, file en device. De eerste implementatie mag een subset bevatten; onbekende typen worden geweigerd. Handles zijn proceslokale 64-bit tokens met slot en generatie. Alleen een valide lookup plus rechtencontrole geeft toegang; een numeriek token is geen globaal objectadres en geen cryptografisch geheim.

Rechten zijn expliciete bitsets per type; dupliceren kan rechten verminderen, nooit stilzwijgend uitbreiden. Overdracht via IPC vermeldt objecttype en toegekende rechten. Objecten hebben sterke referenties, expliciete sluitregels en gecontroleerde opruiming bij procesexit. Geen algemene referentiecyclus zonder breekregel. VFS-bestandsidentiteit en open-handlelevensduur staan los van padnaam.

Waarom: dezelfde interne mechanismen kunnen Windows-handles, Unix-filedescriptors en Mach-portrechten dragen, terwijl hun zichtbare nummers en lifecycle verschillen. Gastbeveiligingsmodellen worden niet één-op-één gelijkgesteld aan deze bitsets.

## Processen, threads en omgevingen

Een proces heeft adresruimte, credentials, namespace, resourcebudget, handlelijst, omgevings-ID en ABI-profiel. Het profiel wordt alleen door de vertrouwde loader vastgesteld. Een programma kan niet via een doorgegeven nummer van gast-ABI wisselen. Threads hebben registercontext, stack/guard, TLS-context, schedulingstatus en cancellation/notificationstate.

Windows PE64 en PE32 zijn aparte profielen; ELF/Linux en Mach-O/Darwin ook. Een native daemon mag meerdere gastomgevingen bedienen maar blijft zelf native. Communicatie tussen profielen verloopt via expliciete IPC; pointers uit het ene proces zijn geen geldige pointers in een server.

32-bit x86 draait later in hardware compatibility mode met een begrensde 32-bit adresruimte. Gateways zetten 32-bit structuren/pointers om naar het 64-bit interne model. 32- en 64-bit DLL's worden niet willekeurig in één app gemengd. Bij benodigde 32→64-overgangen zijn expliciete thunks en stack-/registerconversies vereist. DOS real mode wordt niet op deze manier rechtstreeks aangeboden, maar via M9-emulatie.

## Native ABI en routering

Eigen x86-64 syscall: nummer in RAX; argumenten RDI, RSI, RDX, R10, R8, R9; RCX/R11 clobbered. RAX geeft een eigen statuscode terug; grotere resultaten komen via gecontroleerde outputstructuren. Deze registerkeuze lijkt op Linux maar de nummering, resultaten en objectcontracten zijn eigen. Structuren beginnen met grootte en versie; velden hebben vaste breedte. Voor ARM64 wordt dezelfde semantiek later naar de bijbehorende registers gemapt.

Eén per-CPU entrypad haalt eerst de actieve thread en het procesprofiel op. Native, Linux en NT/Darwin krijgen afzonderlijke decoders. Registertoestand wordt vastgelegd vóór het inschakelen van herintreding; useradressen worden op geldigheid en toegangsrechten gecontroleerd. Terugkeer valideert user-PC, stack en flags. Snelle return is een optimalisatie na een veilige algemene route.

Windows-syscallnummers hangen van het ondersteunde profiel af; geen claim dat één NT-tabel iedere versie dekt. Onze ntdll gebruikt gecontroleerde stubs; apps met eigen directe syscalls vragen aanvullende profielen en tests. 32-bit interrupt-/sysenter-routes worden afzonderlijk geïmplementeerd. Mach traps en Unix/Darwin-aanroepen krijgen hun eigen decodepad. De omgevings-ID alleen lost de semantische verschillen niet op.

Usermode forwarding gebruikt per-thread requestslots in gecontroleerd gedeeld geheugen en notificatie/antwoordkanalen. De kernel bewaart de oorspronkelijke gastcontext. Complexe verzoeken mogen een server gebruiken, maar één gedeeld requestslot per proces is onvoldoende bij meerdere threads of reentrancy. Een server kan gastkernelrechten niet laten gelden voor willekeurige userpointers.

## Synchronisatie en exceptions

Events hebben expliciete manual-/auto-resetsemantiek. Waitsets ondersteunen wait-any en wait-all waar nodig, met atomair registreren/testen, monotone deadline en consistente sluit-/annuleringsregels. Geen intervalpolling als algemene correcte implementatie. Conditionele waits op useradressen gebruiken gecontroleerde compare-and-enqueue; ze mogen geen permanente mapping van onbetrouwbare appdata vereisen.

Threadcontext en exceptionrecords worden per profiel vertaald. Linux-signalen, Windows-exceptions/APC's en Mach-exceptions zijn verschillende mechanismen; een generieke notificationqueue is alleen transport. Een slechte handler of corrupte userstack mag de kernel niet meenemen. Niet ondersteunde uitzonderingssemantiek is een compatibiliteitsbeperking.

## Geheugen en commit

Basispagina x86-64: 4 KiB. Interfaces gebruiken bytebereiken en publiceren de geldige granulariteit, zodat ARM64 niet van verborgen 4 KiB-constanten afhankelijk is. Grote pagina's zijn later optioneel. W^X is de standaard; JIT-gevallen vragen expliciete gecontroleerde overgang zonder algemene permanente RWX-toestemming.

Virtuele reservering verbruikt adresruimte, commit legt backingverplichtingen vast, residency telt aanwezige pagina's. Voor de eerste kernel geldt conservatieve commit: geen belofte op anonieme geheugenruimte zonder aantoonbare backing/reserve. Bestandsmappings kunnen uit backing worden herladen. Discardbare cache is apart herkenbaar.

Budgetoverschrijding leidt tot drukmeldingen, reclaim, gecontroleerde allocatiefout en als laatste beleidsmatig appbeëindigen. Systeemkritieke reserves zijn begrensd en niet vrij beschikbaar voor apps. Locks, I/O en paginafaults krijgen een vaste lockvolgorde; faultafhandeling mag niet wachten op een gebruikersserver die zelf dezelfde schaarse resource nodig heeft.

## IPC, drivers en DMA

Kleine berichten worden gekopieerd; grote buffers gebruiken expliciete gedeelde memory-objecten. Lengtes, offsets, overflow en eigendom worden gecontroleerd. Eerste versie heeft begrensde queues, backpressure en annulering. Prioriteitsinversie in serverafhankelijkheden wordt gemeten en waar nodig met overerving/budgetbeleid aangepakt.

Driver-API publiceert busidentiteit, middelen, interruptregistratie, DMA-buffers en lifecycle. MMIO is niet vrij toegankelijk vanuit apps. DMA-pages worden gepind tot completion/foutcleanup; een timeout is geen bewijs dat hardware gestopt is. Zonder IOMMU is driverisolatie beperkter en wordt dat expliciet gerapporteerd.

## Nog te bewijzen

Layoutassertions voor [requestheaders](contracts/syscall_v0.h) en bootinfo slagen op drie targets. Windows-hostreferenties voor TLS en eenvoudige multiwait zijn gemeten. Syscall-entry/return, daadwerkelijke kernel-TLS-wissels, reentrancy, multiwaitraces en guestexceptions vragen nog runtimewerk. M0 heeft alleen de in het bewijsverslag genoemde host- en compileerproeven uitgevoerd. M2/M3 en M7 moeten de kernelimplementatie daarvan aantonen.
