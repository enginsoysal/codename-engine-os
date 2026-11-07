# Update and recover Engine OS M7.1

Boot the M7.1 ISO and choose Update, Rollback or Repair at Welcome. Select the installation disk, review and Confirm. Maintenance does not use Erase and install or format the data partition. After verified success power off, detach ISO and boot the disk.

| Action | Behavior |
|---|---|
| Update | Stage this ISO's verified bundle in the inactive slot and activate its kernel |
| Rollback | Replay the previous verified slot; a previous valid slot must exist |
| Repair | Restore boot files from the verified active stored slot |

M7.1 qualifies updating an actual M7 v0.7.0 desktop, booting M7.1, rolling back to M7 and repairing a deliberately damaged BIOS payload. Independent complete-data-partition hashes and document bytes remain unchanged. This tests the compatible shipped app/volume ABI, not arbitrary future migrations or upgrades from older M6/M5 installations without desktop apps.

Night defaults apply to fresh installation. Update does not silently remove an existing Calculator or overwrite preferences. For replacement use the [installation guide](install-m71.md). Damaged GPT uses separate advanced [table repair](disk-manager-m71.md#delete-clear-or-repair); boot-slot Repair and GPT-copy repair are different operations.

Interrupted boot-file replay can require another ISO-assisted Repair rather than automatic failover. Failed formatting/partition edits are not automatically rolled back. Keep backups and the ISO; never infer success from an error. [Release limits](../releases/m71-v0.7.1.md).
