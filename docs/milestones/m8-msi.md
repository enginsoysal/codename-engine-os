# M8 first delivery: an original MSI and Windows console application

Status: **implementation in progress; no M8 ISO or application compatibility is qualified.** M7.5 remains the latest accepted release.

The owner requested an original Windows application that can be installed from MSI, started, used and removed inside Engine OS. This first delivery targets the unmodified, publicly redistributable **7-Zip 26.06 x64 MSI** and its original **7z.exe / 7z.dll**. It does not close the wider DOS, Windows 1.x, Win16, Windows 9x, Win32 and Windows 11 compatibility program.

## Architecture and reasons

- A separate, lazy CPL3 runtime executes x86-64 PE instructions directly. Engine OS implements the selected imported APIs in its own code. This preserves process isolation and avoids putting foreign code in the kernel. Neither host Windows nor a host installer performs the guest installation.
- The MSI reader interprets the original CFB streams, string pool and column-major relational tables. A digest-pinned, unmodified libmspack LZX decoder expands the embedded CAB through bounded memory callbacks. The decoder retains LGPL 2.1 licensing. Original package and source archives retain upstream licenses and provenance.
- The first executable is the original console application. A native Engine OS frontend will provide ordinary installation and archive tasks. Windows GUI, Explorer shell extensions, COM activation and a general Windows Installer service are separate implementation work; copying their files does not demonstrate those services.
- The filesystem broker grants runtime writes to the current user's data. Installation/removal jobs receive a separate product-directory grant. PE mappings use executable or writable pages, with a bounded heap and stack. Unsupported required behavior must fail with an explanation.
- Original products live beneath `/Applications/Windows/<product-code>`. Windows paths are compatibility aliases, not a change to the native Engine OS folder contract. Optional native apps remain independent.

Alternative approaches rejected for this slice: invoking the host's installer, translating the application into a native substitute, presenting extracted files as a completed installation, or declaring success from import resolution alone. A full Windows GUI and MSI implementation would broaden the first acceptance target beyond a testable delivery.

## Required acceptance before release

1. Parse the original MSI and compare database values with an independent, read-only format reference; compare every expanded payload against original SHA-256 digests. Reject malformed, unsupported and out-of-bounds inputs.
2. Implement preview, explicit consent, product/feature/component registration, owned-file manifests and interrupted-operation recovery. Required unsupported actions must be identified before modification.
3. Install from the original MSI **inside the guest**. Verify installed original executable/DLL bytes independently, then execute the original application and complete archive create/extract tasks with byte-for-byte data checks.
4. Reboot the installed OS and repeat the application task. Remove the product, verify its registrations and executable are gone, retain user documents, and reinstall successfully.
5. Exercise cancellation, malformed package, insufficient memory/storage, unsupported API behavior and process fault containment. Measure peak and retained pages separately; no Windows worker should consume CPU while idle.
6. Repeat the convenient installer and independent disk startup tests with disposable VMware BIOS/UEFI VMs. Verify the actual native interface at ordinary and 3840x2160 resolutions. Preserve historical M7.5 evidence and binaries.
7. Update user guides, scope, source/license notices and release hashes; publish the tested ISO, VMware starter package and corresponding source to GitHub.

## Development evidence boundary

Host format tests are useful independent checks of parsing and decompression. They do not prove guest installation, PE execution, reboot persistence, removal, usability or VMware behavior. Compilation is not runtime acceptance. Until the complete guest chain passes, no new ISO should be labeled ready for user testing.
