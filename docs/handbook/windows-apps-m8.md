# Windows Apps — original 7-Zip console profile

Use the matching [M8 release and checksums](../releases/m8-v0.8.0.md) and its exact acceptance report. This guide covers one original Windows console profile; it does not promise general Windows application compatibility.

The first target is the original **7-Zip 26.06 x64 MSI**, published by Igor Pavlov. Engine OS targets its original console executable and compression library. The Windows File Manager GUI, Explorer extensions, other MSI packages and other Windows generations are outside this first profile.

## Install, use and remove

1. Install Engine OS from the matching M8 ISO onto a disposable VMware virtual disk. Restart and confirm that the installed desktop opens.
2. Open **Start → Windows Apps**, or open `7z2606-x64.msi` in your Downloads folder.
3. Choose **Review installer**. Engine OS reads the original package and shows the bounded compatibility profile.
4. Choose **Install 7-Zip**, read the confirmation, then choose **Install 7-Zip**. No terminal syntax is required.
5. Choose **Open 7-Zip** to execute the original Windows console application and inspect its output. **Previous** and **More output** navigate longer output.
6. **Create test archive** prepares a separate sample document and asks the original executable to archive it. **Extract test archive** extracts the most recently numbered sample archive into a separate Documents subfolder. Repeated examples automatically choose new output names such as `EngineOS-Windows-2.zip` and `EngineOS-Windows-Extracted-2`; earlier results stay intact. There are at most 100 numbered sample outputs. Existing personal documents remain separate from application installation files.
7. To remove the product, choose **Remove app**, then confirm **Remove 7-Zip**. Removal deletes unchanged owned program files and registration while preserving user documents and modified product files.

The [matching acceptance](../../evidence/M8/acceptance-results.json) binds original payload verification, real program execution, archive bytes, restart persistence, removal and reinstall to the published artifacts. Updating an existing ZIP is unsupported; the example buttons automatically use new outputs to avoid that path. Modified-owned-file removal and arbitrary APIs are not separately qualified.

## Errors and recovery

- Only the exact reviewed upstream MSI is accepted initially. A different package or changed file must be rejected before installation changes.
- An incomplete product with a valid recovery plan exposes **Recover / remove**. Recovery removes unchanged files owned by that plan before a new installation.
- **Cancel task** stops the current protected worker. Failed cleanup must be reported explicitly rather than presented as success.
- Unsupported APIs, insufficient resources and application faults must show an error. A successfully parsed MSI alone does not mean its application is supported.

Windows application files use `/Applications/Windows/<product-code>`. Windows-style paths are aliases inside the compatibility profile. Engine OS continues to use its own native filesystem structure and kernel.

Open **Licenses** for the complete local 7-Zip and libmspack notices, with Previous/Read more and Back to app. Corresponding 7-Zip source is included in the VMware starter; Engine OS and decoder sources are in the canonical repository. [Installation tutorial](install-m8.md).
