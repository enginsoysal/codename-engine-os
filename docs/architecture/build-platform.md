# Build and platform choices

Current delivery is [Engacer OS M10](../releases/m10-v0.10.0.md), with the native graphical installer, installed desktop and phase-1/2 hardware tasks. [M10 boot streaming and device design](hardware-m10.md), [current build tools](../../tools/README.md).

Historical foundation: M0–M4.5 within their qualified native VM scopes. M4.5 closes M45-VM-v1; physical/factory trust are separate follow-ups. [Decisions](decisions.md), [tools](../../tools/README.md), [M2 usage](../milestones/m2.md).

## C17, assembly and LLVM

Use C17 for loader/kernel/runtime and small explicit assembly for CPU entry, context restore and privileged instructions. Clang/LLD 21.1.6 is the observed bootstrap toolchain, producing bare-metal ELF and PE32+ EFI without a host CRT. This matches C ABI boundaries and reduces initial build-system/porting complexity. Rust remains a possible future component choice for memory safety; its additional toolchain/FFI setup was not chosen for the first boot. C is not inherently safer: bounds/ownership checks, negative tests, review and later fuzz/sanitizer work remain required.

Kernel/user target: `x86_64-unknown-none-elf`, freestanding, no builtins/stack protector/red zone, compiler-generated general registers only and explicit memory routines. M3 uses deliberate inline x87/SSE probes and explicit FXSAVE64/FXRSTOR64 in the context path; OSXSAVE/AVX state remains disabled. EFI target: `x86_64-pc-windows-msvc`, MS-x64 function ABI, EFI application subsystem. Native kernel functions use System V x64; the handoff performs that transition explicitly. Future Windows libraries must expose their matching guest ABI, not cast incompatible function pointers.

Python scripts in `tools/build/` orchestrate explicit compiler/linker calls. The [tool lock](../../tools/toolchain-lock.json) records executable/DLL/firmware hashes and versions. These are observed local pins, not independent upstream-authenticity or bit-reproduction proof. Debug prefixes are remapped; release scans reject raw Windows user paths. Original third-party installations are not globally replaced.

## Loader and boot wire contract

The original x86-64 UEFI loader handles a restricted ET_EXEC ELF64 profile. Normal public media are unsigned; the separate M4.5 owner-enrolled VM build authenticates its signed loader and digest-pinned kernel/init. An original BIOS loader normalizes to the same boot contract. Limine/GRUB would reduce loader work and remain possible alternatives if scope grows; own-loader work was deliberately bounded.

It checks firmware signatures/header CRC, opens kernel/init files through the boot-volume protocol, rejects oversized/malformed/overlapping segments, allocates the fixed kernel bootstrap area from 2 MiB and records all allocations. It collects GOP/ACPI metadata, builds initial low-4-GiB identity tables and a stack, captures the final memory map and calls ExitBootServices with at most four attempts. After success there are no firmware-service calls. An explicit test injects a stale first mapkey and verifies retry.

The [160-byte boot header](../../include/engine/boot/boot_v0.h) uses fixed little-endian fields, addresses/lengths, magic/version/size and CRC-32/ISO-HDLC with its CRC field zeroed. Header CRC does not authenticate firmware or pointed-to buffers. Mandatory ranges: memory map, kernel, initmanifest and reservation list; framebuffer/ACPI may be absent. Zero optional ranges require both address and length zero. Unknown flags/reserved bits are rejected.

[Validation](../../include/engine/boot/validate_v0.h) checks header, CRC, arithmetic, overlap, descriptor shape and framebuffer geometry. The kernel separately checks referenced memory ownership against descriptors/reservations. Request byte validation does not by itself prove mapped/access-permitted pages. GOP formats: 1=RGBX8, 2=BGRX8; unsupported formats produce no framebuffer view. ACPI RSDP checksums are checked; M3 additionally validates RSDT/XSDT, MADT and HPET table checksums/bounds before CPU/clock initialization. A general ACPI interpreter is not implemented.

Kernel entry uses an aligned private stack, interrupts off, DF clear and a System V first-argument boot pointer. M1 retains a broad supervisor RWX mapping; M2 replaces it before user execution with RX/RO-NX/RW-NX kernel sections, a missing null page, guard pages and private user mappings. The development allocator/identity window limits are explicit; M3 extends the common upper-half stack branch with per-CPU entry/IST guards and adds bounded anonymous VM. A complete higher-half kernel relocation and arbitrary mappings remain later work.

## Storage/media

FAT32 is the EFI partition format, using `EFI/BOOT/BOOTX64.EFI`. The image author is a restricted host tool with fixed short-name directories and bounded boot files, not a kernel filesystem implementation. A GPT USB disk and EFI El Torito optical ISO are tested separately. pycdlib 1.22.0 is the pinned pure-Python ISO author; its wheel hash and LGPL notice are recorded. This avoids requiring WSL/xorriso on the current Windows host.

The historical M0–M4.5 init fixture is a fixed 48-byte manifest: `ENGINIT1`, length, development mode and 32-byte build ID. Normal public images use mode 0. Modes 1–4 are test-only exit/panic/corrupt-header/mapkey fixtures; M45 additionally accepts modes 5/6 for FAT32/extended-ext2 native tests. That historical fixture is not an executable system archive or Secure Boot trust chain. M5 and later use actual installation/system bundles; M10 includes its selected native/Windows payloads and streams the kernel directly into reserved pages. See [storage](storage-m6.md), [Windows runtime](windows-runtime-m8.md) and [M10 startup limits](hardware-m10.md).

M4 implements the selected bounded ext2 profile: 4 KiB blocks, 128-byte inodes, classic direct pointers and an explicit feature allowlist. See the [M4 contract](../milestones/m4-contract.md) for file/name/handle limits, writable namespace, flush ordering and offline recovery. FAT32 is too limited for general application storage; NTFS/ext4 or a new format are larger initial projects. Large files and guest metadata remain gates before Steam/Adobe claims. Without journaling this is not a crash-safe consumer storage solution; immutable update versions do not make metadata transactional.

## VM reference and resource policy

Pinned QEMU pc-q35-11.1, TCG and bundled x64 OVMF with private variable copies and read-only firmware code. Historical offline probes have no network; later network/VMware tasks use explicitly configured networking. No host block device is exposed. COM1 supplies logs and GOP supplies a CPU framebuffer. After firmware handoff, the original kernel xHCI driver owns input. The separate M45 stack adds bounded hub routing and read-only BOT/UAS transport with native fault/lifetime tests; see [USB scope](../milestones/m4.5-usb-topology.md). GDB binds loopback only.

M0/M1 used two configured vCPUs, with only the bootstrap kernel active. The preserved M2 profile uses one vCPU. The historical M3 launcher defaults to two active CPUs/128 MiB; its test matrix covers 1/2/4 CPUs and 64/128/512/2048 MiB. Configured guest RAM is not kernel consumption. Page/accounting reports distinguish managed peak/retained pages, boot reservations, firmware/device memory and host-QEMU CPU time. After the finite demo, HLT must be observed rather than a polling loop.

M2 only uses UEFI conventional pages below 1 GiB and keeps loader/firmware allocations reserved. M3 adds locked allocation, conservative commit/COW and test-budget pressure recovery, HPET time, PIT/APIC timer preemption and ACPI INIT/SIPI SMP. Cache/file-backed reclaim, extended XSAVE, migration and general physical-platform coverage remain future work. Beyond the user-observed M1 boot, [M3 VMware Workstation 26.0.1 serial/runtime qualification](../../evidence/M3/vmware/README.md) passes on 1/2/4 CPUs with 128 MiB. This does not qualify every VMware configuration; physical hardware is untested.

Sources: [UEFI](https://uefi.org/specs/UEFI/2.11/), [Clang cross-compilation](https://clang.llvm.org/docs/CrossCompilation.html), [QEMU invocation](https://www.qemu.org/docs/master/system/invocation.html), [ext2](https://cdn.kernel.org/doc/html/latest/filesystems/ext2.html), [pycdlib](https://clalancette.github.io/pycdlib/pycdlib-api.html).

## M5 installation bundle

M5 keeps the same 48-byte init manifest and adds test-only mode 7 for native overlay quota/reclamation. Installed and live kernels share the source build ID; only live media embeds the verified sparse installation bundle. Zero pages are encoded without their contents and expanded through one four-KiB page. The installed kernel omits that bundle. The BIOS two-MiB kernel bound remains enforced. The public ISO/USB use the original BIOS and UEFI loaders; a confirmed blank SATA target receives native GPT/ESP/data setup. See [M5 layout, activation record and recovery boundary](../milestones/m5.md).
