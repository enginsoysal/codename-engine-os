# Windows Apps — M8 development guide

**Development only. The complete guest installation, execution and removal journey is not yet qualified. Do not treat this guide as a released compatibility promise.** M7.5 remains the latest accepted release until M8 acceptance and downloads are published.

The first target is the original **7-Zip 26.06 x64 MSI**, published by Igor Pavlov. Engine OS targets its original console executable and compression library. The Windows File Manager GUI, Explorer extensions, other MSI packages and other Windows generations are outside this first profile.

## Intended ordinary workflow

1. Install Engine OS from the matching M8 ISO onto a disposable VMware virtual disk. Restart and confirm that the installed desktop opens.
2. Open **Start → Windows Apps**, or open `7z2606-x64.msi` in your Downloads folder.
3. Choose **Check MSI**. Engine OS reads the original package and shows the bounded compatibility profile.
4. Choose **Install**, read the confirmation, then choose **Install 7-Zip**. No terminal syntax is required.
5. Choose **Run 7-Zip** to execute the original Windows console application and inspect its output. **Previous** and **More output** navigate longer output.
6. **Create sample ZIP** prepares a separate sample document and asks the original executable to archive it. **Extract sample ZIP** asks it to extract that archive into a separate Documents subfolder. Existing personal documents remain separate from application installation files.
7. To remove the product, choose **Remove**, then confirm **Remove 7-Zip**. The intended behavior is to remove unchanged owned program files and registration while preserving user documents and modified product files.

These are development flows to be tested, not evidence that the behaviors have passed. The [release gates](../milestones/m8-msi.md) require original payload verification, real program execution, archive data verification, restart persistence, removal and reinstall.

## Errors and recovery

- Only the exact reviewed upstream MSI is accepted initially. A different package or changed file must be rejected before installation changes.
- An incomplete product with a valid recovery plan exposes **Recover / remove**. The intended recovery removes unchanged files owned by that plan before a new installation.
- **Cancel task** stops the current protected worker. Failed cleanup must be reported explicitly rather than presented as success.
- Unsupported APIs, insufficient resources and application faults must show an error. A successfully parsed MSI alone does not mean its application is supported.

Windows application files use `/Applications/Windows/<product-code>`. Windows-style paths are aliases inside the compatibility profile. Engine OS continues to use its own native filesystem structure and kernel.
