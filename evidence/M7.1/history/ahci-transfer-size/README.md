# AHCI transfer adapter correction

The first native FAT32 format refused a final metadata chunk because the established AHCI driver accepts 512-byte or 4096-byte transfers, while the generic disk editor can request any sector multiple up to 4096. The operation correctly reported failure; it was not counted as success. The native adapter now splits the bounded tail into supported sector transfers. Host backend checks alone did not expose this transport integration error.
