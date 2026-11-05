# M0 → M1: kortste route naar een echte VM-test

## Nu bewezen

De bestaande Windows-hosttoolchain produceert bare-metal ELF en EFI-images. QEMU/EDK2 is vastgepind en laadt onze kleine EFI-omgevingsfixture vanaf een optische ISO en een GPT/FAT32 USB-device. Headerlayouts en hun eerste inhoudsvalidators zijn getest. Dit is de M0-basis; het is nog geen OS dat je kunt installeren.

## M1-werkpakketten in uitvoeringsvolgorde

1. **Bronlayout/build:** `boot/uefi`, `kernel/arch/x86_64`, `kernel/core`, `include`, `runtime`, `tools` en `tests`. Verplaats stabiele contracten pas met verwijzingen en behoud de M0-regressies. Pin elke nieuwe dependency.
2. **Eigen loader:** valideer UEFI-tabellen en protocollen, laad een ELF64-kernel en initimage, controleer alle segmentranges en reserveer geheugen. De M0-fixture gebruikte deze overdracht bewust nog niet.
3. **Eigen kernel-entry:** maak stack en mappings, bouw bootinfo volgens het M0-contract, beëindig Boot Services met een begrensde mapkey-retry, pas de MS-x64→System-V-grens correct toe en ga naar kernel-entry.
4. **Kernelbewijs:** serial met build-id, eigen exceptiontabellen en een gecontroleerde panic. Geef loadergeheugen pas vrij na expliciete eigendomsoverdracht. Hostvalidators vervangen controle van echte gemapte fysieke ranges niet.
5. **Media:** hergebruik of harden de M0-FAT/GPT-fixtureverpakking voor de echte bootbestanden. De huidige writer is beperkt tot één EFI-testbestand; breid hem niet stilzwijgend als algemene filesystemdriver uit. Voeg initimage/kernelbestanden en afzonderlijke ISO-/USB-tests toe.
6. **VM-commando:** lever een startscript en buildartefacten die de echte kernel tonen. QEMU is de eerste referentie; VirtualBox/VMware/Hyper-V moeten apart worden getest voordat ze ondersteund heten.

**M1 klaar:** de echte kernel boot vanaf beide media en rapporteert werkende crashdiagnostiek. Alleen `ENGINE_OS_M0_ENVIRONMENT_OK` zien telt niet.

## Daarna naar installatie

M2: eigen geïsoleerde userprocessen. M3: geheugen, scheduling en synchronisatie. M4: persistente opslag, blockdriver en invoer. M5: liveomgeving en installer naar een lege virtuele schijf, daarna zelfstandig booten zonder installatiemedium.

Een vroege M1-VM biedt boot-/crashtests; een M5-VM biedt installatietests. Geen schijninstaller toevoegen om die twee resultaten gelijk te laten lijken. Het installeren van bestaande Windows-apps begint pas na de M7/M8-infrastructuur.

## Windows 95–11-invarianten

- Reserveer loader-/procesprofielen voor Win16/NE, PE32 en PE32+; de eerste kernel hoeft deze nog niet te implementeren.
- Houd gastpaden en versiegedrag uit de native kernel-ABI.
- Behoud ruimte voor legacy-emulatie, 32-bit contexten, thunks, COM/services, runtimes en verschillende grafische API-generaties.
- Eigen hardwaredrivers eerst; VxD/NT-driverafhankelijkheden blijven een afzonderlijke WC6-poort en geen impliciete compatibiliteit.
- Definieer elke compatibiliteitsclaim met originele installer, architectuur, versie en taken uit het WC1-register.

## Aanvaarde latere poorten

Werkelijke syscall-entry, gast-TLS-switches, racevrijheid, Linux/Mach-runtimecorrectheid en fysieke hardware zijn geen M0-uitvoeringsresultaten. Ze hebben expliciete M2/M3/M7/M14/M15/M16-poorten. De fysieke testcomputer is nog niet gekozen; virtueel verdergaan vereist geen toegang tot de hostschijf.
