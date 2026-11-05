# Compatibiliteitsmatrix en risicoproeven

## Kernmatrix

| Onderwerp | Windows | Linux | macOS/Darwin | Eigen mechanisme en gevolg |
|---|---|---|---|---|
| Laden | PE, imports, relocations | ELF, interpreter, auxv | Mach-O, dyld, load commands | Aparte gevalideerde loaders; formaat bepaalt niet alle ABI-details |
| Functie-ABI | MS x64 / x86 | Doelarchitectuur-ABI | Doelarchitectuur-ABI | Wrappers; geen onveilige aanname dat struct-/stacklayout gelijk is |
| Syscalls | NT-stubs en soms directe calls | Directe kernel-entry | Mach traps en Darwin syscalls | Decoder per procesprofiel; veilige eigen entry/return |
| Threads/TLS | TEB/PEB, TLS, exceptions | Threadpointer, libc, signal state | Threadstate, pthread/runtime | Context opslaan plus per-profiel runtimegegevens |
| Wachten | Events, mutexen, multiwait, APC | Futex/poll/signalen | Mach ports en andere primitives | Waitsets en conditionele waits; betekenis blijft profielspecifiek |
| Fouten | NTSTATUS, Win32 last-error | errno en syscallfouten | Mach-returnwaarden / errno | Vertaling op de grens; geen stil succes voor stubs |
| Geheugen | reserve/commit, allocation granularity | mmap/mprotect, mappingflags | Mach VM en mappings | Memory-objecten, ranges, backing en expliciete restricties |
| Bestandsnamen | Vaak case-insensitive; Unicode-regels | Bytepaden; doorgaans case-sensitive | Volumeafhankelijk, Unicodegedrag | Native UTF-8 plus naam-/lookupadapter; botsingen expliciet |
| Open/verwijderen | Sharingflags, deletevoorwaarden | Unlink bij open fd gebruikelijk | POSIX plus platformdetails | Objectlevensduur los van naam; centrale delete/sharebemiddeling |
| Identiteit | Tokens en ACL's | UID/GID en rechten | POSIX plus platformbeveiliging | Eigen credentials en gecontroleerde mapping |
| GUI | Win32/GDI en andere frameworks | Meerdere display-/toolkitroutes | AppKit/CoreGraphics en meer | Eén compositor met adapters; slechts één API is onvoldoende |
| Drivers | .sys en NT-drivercontracten | Kernelmodules | Platformextensies | Eigen drivers; gastdrivercompatibiliteit apart |

## Proefplan en huidige dekking

| ID | Proef | Waarom | Stand |
|---|---|---|---|
| P01 | Freestanding x86-64 ELF compile/link | Geen verborgen hostkernel/CRT nodig voor minimale binary | Geslaagd; header en architectuur gecontroleerd |
| P02 | Freestanding ARM64 ELF compile/link | Toolchain blokkeert tweede architectuur niet vooraf | Geslaagd; geen ARM-runtimebewijs |
| P03 | EFI PE32+ compile/link | UEFI-uitvoerformaat is lokaal te produceren | Geslaagd; firmware-uitvoering niet getest |
| P04 | Windows auto-reset event tweemaal wachten | Auto-reset consumeert signaled state | Geslaagd op Windows-host |
| P05 | Windows manual-reset event tweemaal wachten | Staat blijft gezet zonder reset | Geslaagd op Windows-host |
| P06 | Windows bestand zonder sharing opnieuw openen | API moet ook negatieve gevallen juist uitvoeren | Geslaagd: sharing violation 32 |
| P07 | Multiwait, timeout en sluiten | Race- en lifecyclecontracten | Wait-any/all en zero-time timeout gemeten; concurrent sluiten/races nog open |
| P08 | TLS per thread en exceptionoverdracht | Essentieel voor binary-start en foutafhandeling | TLS-isolatie gemeten; exceptionoverdracht nog open |
| P09 | Directe syscall-entry op eigen kernel | DLL-vertaling alleen is onvoldoende | M2/M7 runtimepoort; M0 entry-/decodecontract beschreven |
| P10 | Linux signal/futex/mmapreferenties | Verschillende fout-/wachtregels | Documentatie; geen eigen referentiemeting |
| P11 | Mach-port-/Darwinreferenties | Mach IPC is geen hernoemde Windows-event | Documentatie; geen eigen referentiemeting |
| P12 | Rename/unlink, case en Unicode | Eén simpele padmapping is onvoldoende | Contract beschreven; referentietests nog te doen |
| P13 | Boot-/requestlayouts op x64, ARM64 en i686 | Wire layout mag niet van hostpointers afhangen | Drie compileercontroles geslaagd |

## Hergebruikbeleid

Wine is kandidaat voor Windows-bibliotheken en gedragstests, niet een complete drop-in op onze kernel. Afhankelijkheden van POSIX, threads, signalen, geheugen en drivers moeten worden geport of vervangen. Een POSIX-intern portingvlak mag bestaan zonder dat het OS een Linux-kernel krijgt. Linux-compatibiliteit voor gebruikersapps is een afzonderlijke belofte.

Darling is kandidaatbron voor Mach/Darwin-kennis en componenten, maar leunt op Linux. Zijn server- en frameworkroutes vragen een echte port. Geen claim dat macOS-GUI of Xcode al werkt door de repository toe te voegen.

Voor elk overgenomen component: concrete revision, herkomst, licentie, distributieverplichtingen, security-updates, hostafhankelijkheden en testplan. Componenten worden pas in de build opgenomen na die registratie.

## Primaire bronnen

- [Wine API Guide](https://source.winehq.org/WineAPI/): interfacecatalogus en implementatieverwijzingen.
- [Microsoft eventvoorbeelden](https://learn.microsoft.com/en-us/windows/win32/sync/using-event-objects): event-/waitgedrag.
- [Linux ABI](https://docs.kernel.org/admin-guide/abi.html): kernel/userspace-contracten.
- [Darling syscall-emulatie](https://docs.darlinghq.org/internals/basics/system-call-emulation.html): aparte Mach-/Darwin-aanroepen.
- [Darling broncode](https://github.com/darlinghq/darling): loader, server en frameworks.

De matrix is een ontwerpanalyse. Alleen expliciet als geslaagd gemarkeerde eigen proeven tellen als meting; andere rijen zijn geen compatibiliteitsbewijs.
