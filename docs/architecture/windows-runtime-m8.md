# Original Windows console execution — first M8 profile

This describes the implemented first profile. Release qualification still requires the matching acceptance report. It is not a specification for complete Windows compatibility.

## Executable boundary

Engine OS loads a separate freestanding ELF runtime into a private CPL3 address space. That runtime validates and maps the original AMD64 PE executable and DLL, applies checked DIR64 relocations, resolves supported imports to original Engine OS implementations, protects code as executable/read-only and data as writable/non-executable, and calls the original entry point using the Microsoft x64 calling convention. The unmodified 7-Zip machine code performs compression and extraction. No Microsoft DLL, host Windows installer, Linux kernel or Wine runtime supplies those tasks.

The kernel gives each Windows job a separate page table, checked syscall copies, a bounded demand-grown user stack, private heap pages, TEB/GS base and saved x87/SSE state. Timer quanta return control to the native desktop. The worker is lazy and its owned pages are released on completion, cancellation or a contained user fault. The shared transition stack retains four pages per boot after first use. Immutable kernel/runtime metadata and boot reservations are reported separately from managed worker pages.

The first profile assigns one processor and one Windows thread. Archive examples explicitly use `-mmt=off`. Selected file, console, time, memory, string/CRT, OLE string/variant and private registry operations are implemented. Static PE TLS, Windows exception unwinding, general Windows GUI/COM, services, guest drivers, multithreaded Windows execution, DirectX, AVX state and arbitrary MSI custom actions are not provided. Required unsupported operations fail; a resolver entry is not evidence that every behavior of an API is compatible.

## Original MSI and product lifecycle

The first installer adapter accepts only the digest-pinned original 7-Zip 26.06 x64 MSI. It reads CFB v3, the MSI string pool, column catalog and column-major tables. Original file identifiers, component directories, sizes and long filenames determine the installed layout. An unmodified, digest-pinned libmspack LZX decoder expands the embedded CAB through original bounded callbacks. Independent tests compare database cells with read-only Windows Installer database queries and compare all 106 expanded files with original SHA-256 references. Neither reference mechanism performs guest installation.

This is a **bounded console-profile adapter**, not a replacement implementation of every Windows Installer standard action. All original package files are retained, including currently unsupported GUI binaries. Original feature/component/registry/shortcut records are saved as product metadata; unsupported GUI shortcuts and shell activation stay explicitly disabled. The console's private path registry is implemented. Original payload presence must never be presented as working Explorer extensions or a working Windows GUI.

The native frontend provides review, explicit install/remove consent, progress, original program execution, archive examples, paged output and complete local licensing notices. A durable `install.plan` is written before payload files. Each file is committed through the native copy-on-write volume, then verified against the planned content. `product.bin` is published last. A failed/cancelled fresh install attempts cleanup only beneath the newly created product root. A surviving valid plan enables recovery; an empty orphan directory can also be recovered. Invalid ownership metadata is refused rather than used for arbitrary deletion.

Removal checks the manifest's owned paths and current file contents. Unchanged owned files are removed; changed files are retained. User documents are outside the product root. The product marker is removed after processing its files, so an interrupted removal can be retried. The operation is recoverable, not a single atomic transaction across every product file.

## Storage grants and aliases

Installed products use `/Applications/Windows/{product-code}`. The selected profile exposes `C:\Program Files\7-Zip` and `C:\Users\<account>` as compatibility aliases. They do not rename the native Engine OS filesystem or create a second OS underneath it. Runtime writes are confined to the current user's tree. Installer/removal jobs have a separate fixed product-root grant; the kernel retains the original grant independently of the user-writable mailbox. One random-access writer buffers at most 8 MiB and commits on close, which permits real ZIP header seeks without exposing partially rewritten native files. File handles, metadata records, directory enumeration, mappings, decoded archives and paths have explicit bounds.

The native volume gains atomic regular-file rename/replacement through one metadata commit. Host execution of the actual C implementation tests every metadata-write/flush interruption, including torn writes, and requires a complete old or new namespace after remount. This is storage evidence, separate from application acceptance.

## Separate installation payload at boot

The original MSI is a boot data module, rather than a 2-MiB constant inside the fixed low-address kernel. Embedding it enlarged the fixed kernel reservation and caused a reproduced UEFI fixed-allocation failure. Keeping large installer data separate preserves the small kernel placement and is the extensible route for future packages.

The normal 48-byte `ENGINIT1` header remains intact. M8 live media appends an `ENGMSI81` record containing the payload length and SHA-256, followed by the original MSI. The kernel checks the actual payload against the independently pinned package digest before distribution. UEFI loads this file into separately reserved pages. BIOS appends it after the flattened core load, includes it in the loader CRC/reservation, and reports the core and init ranges separately. Installed boot uses the ordinary 48-byte init file and retains no live MSI buffer. Installation copies the original package into the account's Downloads folder.

## Qualification boundaries

Acceptance must bind the source tree, runtime/kernel/ISO hashes, original package and executable hashes, complete install/start/use/reboot/remove/reinstall tasks, real native framebuffer captures, measured resources and owned VMware BIOS/UEFI lifecycles. Host compilation, a parsed MSI, copied payloads, a version banner or a live-session desktop alone cannot qualify this profile. The wider DOS and thirteen Windows-generation tracks remain separate work.
