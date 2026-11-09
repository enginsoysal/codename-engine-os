#ifndef ENGINE_CAB8_H
#define ENGINE_CAB8_H
#include "msi_db.h"
typedef struct {char name[64];uint32_t offset,size;uint16_t date,time,attributes;} cab8_file;
typedef struct {msi8_blob expanded;unsigned count;cab8_file files[256];} cab8_archive;
cab8_archive *cab8_decode(msi8_blob);
#endif
