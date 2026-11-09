#ifndef ENGINE_MSI8_DB_H
#define ENGINE_MSI8_DB_H
#include "runtime.h"
typedef struct {uint8_t *data;unsigned size;} msi8_blob;
typedef struct {char name[64];unsigned type;} msi8_column;
typedef struct {char name[64];unsigned columns,rows;msi8_column column[32];msi8_blob data;} msi8_table;
void msi8_database(const void*,unsigned);
msi8_blob msi8_stream(const char*,unsigned);
msi8_table msi8_table_open(const char*);
const char *msi8_string(unsigned);
const char *msi8_text(const msi8_table*,unsigned,unsigned);
int32_t msi8_integer(const msi8_table*,unsigned,unsigned);
unsigned msi8_raw(const msi8_table*,unsigned,unsigned);
#endif
