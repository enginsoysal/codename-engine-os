# Bouwtechniek, boot en referentieplatform

## Hoofdtaal: C17 met beperkte assembly

**Keuze:** C17 voor de eerste kernel, loader en kernbibliotheken. Architecture-entry, contextwissels en speciale CPU-instructies krijgen kleine afgebakende assemblydelen. Externe C/C++-componenten kunnen later achter C-grenzen worden geport; geen C++-exceptions of RTTI als kernelvereiste.

**Waarom:** de beschikbare Clang/LLD-toolchain bouwt zowel bare-metal ELF als EFI-PE. C sluit direct aan bij de verwachte ABI's en veel bestaande low-levelcomponenten. We beperken het aantal talen en bouwsystemen tijdens de eerste bootroute.

**Alternatief:** Rust vermindert bepaalde geheugenveiligheidsrisico's en blijft aantrekkelijk voor toekomstige diensten. Het ontbreekt nu lokaal en brengt in deze fase een extra toolchain en FFI-grenzen mee. De keuze voor C is geen bewijs dat C veiliger of sneller is. Boundschecks, eigendomsregels, strenge waarschuwingen, host-sanitizers waar uitvoerbaar, parserfuzzing en negatieve tests zijn vereist compensatiewerk. Rust blijft heroverweegbaar vóór grote implementatie-investering wanneer de object-/driverproeven daar reden toe geven.

## Toolchain en bouwcontract

Gecontroleerd: Clang/LLD 21.1.6 uit de bestaande Swift-toolchain. Python 3.10 beschikbaar. ELF64 x86-64, ELF64 ARM64 en PE32+ EFI compileren/linken in de M0-proef. Dit is een compatibele lokale bootstrap, nog geen onafhankelijk gereproduceerde distributietoolchain.

- Kernel: expliciete target `x86_64-unknown-none-elf`, freestanding C17, geen host-CRT, geen red zone en aanvankelijk geen impliciet FPU/SIMD-gebruik.
- EFI-loader: target `x86_64-pc-windows-msvc`, PE32+, EFI application subsystem, Microsoft x64-aanroepconventie.
- Eigen kernel/user-runtime: System V x86-64 functie-ABI; de syscall-ABI is apart gedefinieerd.
- Windows-runtime gebruikt Microsoft x64 aan de appzijde. Overgangen naar onze diensten zijn expliciete wrappers, geen cast van incompatibele functiepointers.
- Compilergegenereerde helpers zoals geheugenfuncties worden zelf geleverd en op onbedoelde imports gecontroleerd.
- Een klein Python-buildgereedschap orkestreert aanvankelijk expliciete compiler/linkercommando's; vermijd een vereiste op CMake/Ninja/WSL voordat die nodig is.
- Toolmanifest legt versies en hashes vast vóór M1. De gedeelde Swift-installatie wordt niet vervangen of aangepast.

Waarom: het hosttarget standaard gebruiken zou ongemerkt Windows-afhankelijkheden opleveren. [Clang cross-compilation](https://clang.llvm.org/docs/CrossCompilation.html), [freestanding documentatie](https://clang.llvm.org/docs/UsersManual.html).

## Bootloader: kleine eigen UEFI-loader

**Keuze:** een eigen, beperkt UEFI-programma dat een ELF64-kernel en initimage laadt. Geen BIOS-, PXE- of universele firmwareondersteuning in M1.

**Hoe:** laad bestanden via firmwareprotocollen van het bootvolume; valideer alle ELF-ranges en segmentoverlap; reserveer segmenten en overdrachtsbuffers; verzamel GOP- en ACPI-informatie; maak kernelmapping en stack; haal de finale memory map op; beëindig Boot Services met de juiste key en een begrensd herhaalpad; ga via een ABI-wrapper naar kernel-entry.

**Waarom:** de scope is beheersbaar door UEFI-only en geeft een volledig beheerst bootcontract. Limine/GRUB zouden bestaande ondersteuning en minder bootwerk bieden; dat blijft een terugvaloptie als eigen loaderwerk M1 disproportioneel vertraagt. Een eigen loader is geen noodzakelijke voorwaarde voor een eigen OS, maar hier een bewuste beperkte implementatiekeuze.

**Bootprotocol v0:** vaste little-endian velden met magic, versie, totale lengte, flags en checksum van relevante descriptors; fysieke adressen plus lengtes voor memory map, kernel/initimage, framebuffer en ACPI-root; expliciete gereserveerde ranges en bootprofiel. Geen hostpointers, `size_t` of ongedefinieerde structpadding op de wire. Onbekende verplichte velden/versies worden geweigerd. Headerlayout en entryregisters moeten vóór M1 als assertions vastliggen.

De eerste [bootheader](contracts/boot_v0.h) is 160 bytes en heeft geslaagde layoutassertions op x86-64, ARM64 en i686. Flags en reserved velden zijn in v0 nul. `crc32` wordt berekend over `structure_size` bytes met het CRC-veld op nul, volgens CRC-32/ISO-HDLC; descriptorbuffers hebben afzonderlijke rangevalidatie en worden niet door deze header-CRC gedekt. CRC detecteert fouten maar bewijst geen authenticiteit. Het reservations-bereik bevat een array van `os_boot_range`; elk adres plus lengte wordt op overflow en overlap gecontroleerd. ACPI-range verwijst naar de gevalideerde RSDP, waarna tabellen apart worden gevalideerd. Framebufferformaten zijn initieel 1=RGBX8 en 2=BGRX8; overige formaten worden geweigerd of de framebuffer blijft afwezig. Een afwezige optionele range heeft adres en lengte nul. Details van inhoudsvalidatie en negatieve gevallen volgen vóór loadergebruik.

Kernel-entry x86-64: eigen gemapte kernel, interrupts uit, eigen uitgelijnde stack, direction flag vrij, pointer naar bootinfo via System V eerste argument. De loader reserveert zijn overgangspagina's tot de kernel ze expliciet vrijgeeft. Kerneladressen zijn niet de gastadresruimte; ELF kan voor vroege boot een beperkte vaste layout gebruiken. Definitieve hogere-halfmapping en guardgebieden worden vóór usermode vastgelegd.

UEFI is de normatieve basis, inclusief mapkey en bootmedia. [Boot Services](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html?highlight=exitbootservice), [UEFI 2.11 bootmanager](https://uefi.org/specs/UEFI/2.11/03_Boot_Manager.html).

## Opslagformaten

**EFI:** FAT32 op een GPT EFI-systeempartitie, met `EFI/BOOT/BOOTX64.EFI` als removable-media-entry. De bootbestanden worden door bevoegde installatie/updatecode beheerd.

**Initimage:** read-only, deterministisch verpakt bestandspakket met manifest en hashes; geen willekeurige archiefpaden of links buiten de root. De kernel hoeft voor M1 nog geen volledig schijffilesystem te bevatten. Exact pakketformaat wordt als M1-buildcontract vastgelegd.

**Eerste schrijfbare ontwikkelvolume:** beperkt ext2-profiel met expliciete featureallowlist. Begin met 4 KiB-blokken, 128-byte inodes en klassieke block pointers; geen extents, journal, encryptie of onbekende incompat-features. Ondersteunde bestandsgrootte wordt door geteste blockmapping begrensd, met een expliciete fout boven de grens. Voordat grote Steam-/Adobe-data wordt geclaimd moeten grootbestandondersteuning en featureflags zijn uitgebreid.

Waarom ext2: bestaande documentatie, onafhankelijke controletools en een overzichtelijk datamodel. FAT32 is geschikt voor bootuitwisseling maar beperkt als algemene appopslag; NTFS/ext4 zijn grotere eerste implementaties; een nieuw formaat zou een extra onderzoeksproject zijn. [Ext2-documentatie](https://cdn.kernel.org/doc/html/latest/filesystems/ext2.html).

**Grenzen:** ext2 zonder journal garandeert geen transactieherstel bij stroomuitval. Updates naar een nieuwe immutable versie plus activatie verminderen risico, maar maken de onderliggende metadata niet atomair. M5 moet dit testen en mag alleen ontwikkelkwaliteit claimen. Vóór dagelijkse productie wordt journaling/COW of een ander crashbestendig formaat apart gekozen.

## Virtuele referentieconfiguratie

Kandidaat, nog niet uitgevoerd:

- QEMU x86-64 `q35`, TCG als portable basis; versiespecifieke machinevariant vastpinnen zodra de binary is gekozen.
- 512 MiB RAM en 2 vCPU voor de eerste gewone test; 128/256 MiB en 1/4 vCPU als aanvullende druk-/concurrencyprofielen. Dit zijn testinstellingen, geen productminimum.
- OVMF x64, Secure Boot uit, read-only firmwarecode en een aparte beschrijfbare variabelenkopie per test.
- COM1 naar log, framebuffer via standaard VGA/GOP; geen 3D-claim.
- Later expliciet virtio-blk-pci en virtio-net-pci; geen onbedoelde afhankelijkheid van hypervisorversnelling.
- xHCI met USB-toetsenbord/muis. Boot via firmware kan eerder werken dan kernel-USB, maar bewijst die driver niet.
- USB-profiel met echte USB-mass-storage-emulatie; ISO-profiel via optisch medium en EFI El Torito-bootimage. Een diskimage als SATA booten is niet hetzelfde als USB testen.
- Debuggerpoort uitsluitend loopback; geen host-directorysharing als standaard.

Waarom: reproduceerbaar zonder specifieke CPU/GPU; virtio beperkt initieel driverwerk. Echte NVMe, GPU en USB-controllers blijven aanvullende hardwaretrajecten. [QEMU invocation](https://www.qemu.org/docs/master/system/invocation.html).

QEMU, OVMF en ISO-tools zijn nog niet als lokale toolset gevalideerd. WSL is niet geïnstalleerd. M0 installeert geen systeemwijzigingen impliciet en gebruikt geen bestaande hostschijf als testdoel. Het project heeft vanaf 5 november een zelfstandige repository `enginsoysal/codename-engine-os`; de eerdere planningskopie in BrixCanvas is historisch. Broncode, contracten en toekomstige buildbestanden worden in deze eigen repository ontwikkeld.
