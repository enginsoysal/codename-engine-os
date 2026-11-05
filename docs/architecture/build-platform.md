# Build and platform choices

Current baseline: M0 contracts, M1 firmware handoff and M2 native isolation. [Decisions](decisions.md), [tools](../../tools/README.md), [M2 usage](../milestones/m2.md).

## C17, assembly and LLVM

Use C17 for loader/kernel/runtime and small explicit assembly for CPU entry, context restore and privileged instructions. Clang/LLD 21.1.6 is the observed bootstrap toolchain, producing bare-metal ELF and PE32+ EFI without a host CRT. This matches C ABI boundaries and reduces initial build-system/porting complexity. Rust remains a possible future component choice for memory safety; its additional toolchain/FFI setup was not chosen for the first boot. C is not inherently safer: bounds/ownership checks, negative tests, review and later fuzz/sanitizer work remain required.

Kernel/user target: `x86_64-unknown-none-elf`, freestanding, no builtins/stack protector/red zone, general registers only and explicit memory routines. EFI target: `x86_64-pc-windows-msvc`, MS-x64 function ABI, EFI application subsystem. Native kernel functions use System V x64; the handoff performs that transition explicitly. Future Windows libraries must expose their matching guest ABI, not cast incompatible function pointers.

Python scripts in `tools/build/` orchestrate explicit compiler/linker calls. The [tool lock](../../tools/toolchain-lock.json) records executable/DLL/firmware hashes and versions. These are observed local pins, not independent upstream-authenticity or bit-reproduction proof. Debug prefixes are remapped; release scans reject raw Windows user paths. Original third-party installations are not globally replaced.

## Loader and boot wire contract

A limited original UEFI loader handles only x86-64, unsigned UEFI and a restricted ET_EXEC ELF64 profile. Limine/GRUB would reduce loader work and remain possible alternatives if scope grows; own-loader work was deliberately bounded.

It checks firmware signatures/header CRC, opens kernel/init files through the boot-volume protocol, rejects oversized/malformed/overlapping segments, allocates the fixed kernel bootstrap area from 2 MiB and records all allocations. It collects GOP/ACPI metadata, builds initial low-4-GiB identity tables and a stack, captures the final memory map and calls ExitBootServices with at most four attempts. After success there are no firmware-service calls. An explicit test injects a stale first mapkey and verifies retry.

The [160-byte boot header](../../include/engine/boot/boot_v0.h) uses fixed little-endian fields, addresses/lengths, magic/version/size and CRC-32/ISO-HDLC with its CRC field zeroed. Header CRC does not authenticate firmware or pointed-to buffers. Mandatory ranges: memory map, kernel, initmanifest and reservation list; framebuffer/ACPI may be absent. Zero optional ranges require both address and length zero. Unknown flags/reserved bits are rejected.

[Validation](../../include/engine/boot/validate_v0.h) checks header, CRC, arithmetic, overlap, descriptor shape and framebuffer geometry. The kernel separately checks referenced memory ownership against descriptors/reservations. Request byte validation does not by itself prove mapped/access-permitted pages. GOP formats: 1=RGBX8, 2=BGRX8; unsupported formats produce no framebuffer view. ACPI RSDP checksums are checked; full ACPI-table consumption is future platform work.

Kernel entry uses an aligned private stack, interrupts off, DF clear and a System V first-argument boot pointer. M1 retains a broad supervisor RWX mapping; M2 replaces it before user execution with RX/RO-NX/RW-NX kernel sections, a missing null page, guard pages and private user mappings. The development allocator/identity window limits are explicit; a higher-half/SMP/general-VM design remains later work.

## Storage/media

FAT32 is the EFI partition format, using `EFI/BOOT/BOOTX64.EFI`. The image author is a restricted host tool with fixed short-name directories and bounded boot files, not a kernel filesystem implementation. A GPT USB disk and EFI El Torito optical ISO are tested separately. pycdlib 1.22.0 is the pinned pure-Python ISO author; its wheel hash and LGPL notice are recorded. This avoids requiring WSL/xorriso on the current Windows host.

The current init file is a fixed 48-byte manifest: `ENGINIT1`, length, development mode and 32-byte build ID. Normal public images use mode 0. Modes 1–4 are test-only exit/panic/corrupt-header/mapkey fixtures. This is not an executable system archive or Secure Boot trust chain.

The selected first writable *future* development filesystem is a bounded ext2 profile: 4 KiB blocks, 128-byte inodes, classic block pointers and an explicit feature allowlist. FAT32 is too limited for general application storage; NTFS/ext4 or a new format are larger initial projects. Large-file support and guest metadata must be proven before Steam/Adobe claims. Without journaling ext2 is not a crash-safe consumer storage solution. M4/M5 must test flush/recovery; immutable update versions do not make underlying metadata transactional.

## VM reference and resource policy

Pinned QEMU pc-q35-11.1, TCG and bundled x64 OVMF; private variable copies, read-only firmware code, no network/host disk sharing. COM1 supplies logs and GOP supplies a CPU framebuffer. xHCI exposes USB storage/input in firmware; no kernel USB driver exists yet. GDB binds loopback only.

M0/M1 used two configured vCPUs, with only the bootstrap kernel active. M2's normal launcher uses one vCPU and 128 MiB guest RAM; tests also use 512 and 64 MiB. Configured guest RAM is not kernel consumption. Page/accounting reports distinguish managed peak/retained pages, boot reservations, firmware/device memory and host-QEMU CPU time. After the finite demo, HLT must be observed rather than a polling loop.

M2 only uses UEFI conventional pages below 1 GiB and keeps loader/firmware allocations reserved. Final page allocation/commit/reclaim, timer preemption, SMP and pressure behavior remain M3 work. VMware's successful user-observed M1 boot is useful but does not qualify every VMware configuration; physical hardware is untested.

Sources: [UEFI](https://uefi.org/specs/UEFI/2.11/), [Clang cross-compilation](https://clang.llvm.org/docs/CrossCompilation.html), [QEMU invocation](https://www.qemu.org/docs/master/system/invocation.html), [ext2](https://cdn.kernel.org/doc/html/latest/filesystems/ext2.html), [pycdlib](https://clalancette.github.io/pycdlib/pycdlib-api.html).
