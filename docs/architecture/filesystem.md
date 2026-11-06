# Files, storage and compatibility namespaces

Approved target architecture. [M4](../milestones/m4-contract.md) implements a bounded persistent ext2/VFS with the root categories below and a test account. M5 adds a separate 64-MiB boot ESP, a bounded 16-MiB persistent native partition and an on-demand volatile live overlay. System version metadata records the installed build; general versioned mount activation, guest path/ACL adapters, untrusted-user file syscalls and app installation remain later work. See the [M5 contract](../milestones/m5.md). These OS paths are not directories to create on the development host. The kernel diagnostic console is trusted and does not enforce the future multi-user/system-immutability policy.

## M6 native implementation

M6 replaces the internal demonstration volume with a capacity-aware native extent volume, while retaining earlier filesystem readers separately. Its actual system bundles, data layout, update/recovery mechanism and explicit limits are documented in [M6 storage](storage-m6.md). The root tree below remains the target namespace: current M6 creates the base categories, selected user folders and system settings; generalized version mount views, guest path/ACL adapters and untrusted app-file services remain future work.

## Native root

```text
/
├── System/
│   ├── Versions/<build-id>/       Immutable system versions
│   ├── Current/                  Privileged selection of the active version
│   └── Compatibility/            Logical access to versioned guest runtimes
├── Applications/<app-id>/
│   └── Versions/<version>/       Managed application code
├── Users/<account>/
│   ├── Desktop/
│   ├── Documents/
│   ├── Downloads/
│   ├── Pictures/
│   └── AppData/<app-id>/          Durable personal app state
├── Data/
│   ├── Services/<service-id>/     Durable system-service state
│   ├── Environments/<env-id>/     Registry/configuration/guest mappings
│   └── Packages/                 Installation/update records
├── Cache/<principal>/<app-id>/    Rebuildable data only
├── Logs/<principal>/             Bounded logs; no credentials/tokens
├── Temporary/<principal>/        Private temporary data with retention policy
└── Volumes/<mount-name>/         Authorized local/removable/network storage
```

`Current` and `Compatibility` are system-managed views, not app-writable symlinks. Physical disks/partitions remain below the logical namespace. Mount identity is a stable volume ID; display labels can change. Installers must not identify a target disk solely by its label.

## Why this layout

A single native root separates system code, application code, durable data and disposable data without tying app paths to a physical `C:` partition. Immutable versions simplify recovery and reduce code tampering. Cache eviction must never delete unique documents. Per-environment namespaces support incompatible path/metadata rules without mixing all ecosystems into one global layout.

Native names preserve UTF-8 with case-sensitive lookup and no silent normalization. Guest adapters apply their own documented rules. Collisions produce an explicit error or a defined isolated view, never arbitrary file selection. Authorization follows object identity/rights across aliases.

## Windows view

| Guest path | Native backing/view | Rules |
|---|---|---|
| `C:\Windows` | Selected Windows runtime plus controlled configuration | Read-only runtime code |
| `C:\Program Files\<app>` | Registered app version | Original installer sees the expected path |
| `C:\Program Files (x86)\<app>` | Registered 32-bit app version | Logical separation, no extra disk required |
| `C:\ProgramData` | `Data/Environments/<env-id>/ProgramData` | State shared within that environment |
| `C:\Users\<name>\Documents` | `Users/<account>/Documents` | The same authorized documents |
| `C:\Users\<name>\AppData` | Personal app/environment data | Explicit Local/Roaming/LocalLow mappings |
| Guest temporary paths | Private `Temporary` subtree | Guest variables point to that view |
| `D:` and later | Assigned volume IDs | Policy-controlled visibility |

These are semantic views, not text substitutions. Windows sharing/delete rules, case behavior, Unicode, links and ACLs must be implemented correctly. Installers can use paths outside Program Files; track their writes inside the managed environment. Old apps that write into their program directory may need an explicit write layer, never unrestricted replacement of shared system libraries.

Separate environments are the default, but dependent apps may deliberately share one: Steam/helpers/games or Adobe components can need common COM, services and registry state. Isolation that breaks required cooperation is not successful compatibility.

## Linux and macOS views

Linux: versioned `/usr` and runtime libraries, environment `/etc`, durable `/var`, authorized `/home`, private `/tmp`, and restricted virtual `/proc`, `/sys` and device views. POSIX paths alone do not provide the Linux ABI.

macOS: target-runtime `/System`, shared `/Library`, registered `/Applications`, authorized `/Users` and `/Volumes`; personal `~/Library` maps to personal macOS app data. Frameworks/SDKs are separately versioned and available only with proper provenance/access.

## VFS/security contract and verification

Mounts carry read/write/execute flags. Resolve each object with rights checks and support directory-relative race-resistant operations. Canonicalization, `..` and links cannot escape granted roots. Preserve needed original names, case policy, guest attributes and ACLs; unsupported metadata needs a controlled object-bound store or a clear error, not unreliable sidecar assumptions.

Centralize open/share/delete/rename mediation so different environments cannot bypass each other's locks. Translate GUI labels while keeping technical paths stable. M4 tests case/Unicode collisions, locks, rename, links, delete-with-open-handle, full disks and recovery; M8 adds an original installer/uninstaller. Reboot tests verify stable paths/volume IDs. A global Windows root is simpler initially but couples state; a global Unix root imposes foreign semantics. The selected views cost implementation work but preserve both independence and compatibility.
