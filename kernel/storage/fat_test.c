#include <engine/storage.h>
unsigned fat_selftest(void){
    char bytes[4096];unsigned size=0;if(!storage_count())return B_ABSENT;
    if(fat_list(0,0)||fat_read(0,0,"/hello.txt",bytes,sizeof(bytes),&size)||size!=26||memcmp(bytes,"Engine OS FAT32 read test\n",26))return B_IO;
    if(fat_read(0,0,"/R\xc3\xa9sum\xc3\xa9.txt",bytes,sizeof(bytes),&size)||size!=13||memcmp(bytes,"Unicode name\n",13))return B_IO;
    if(fat_read(0,0,"/../HELLO.TXT",bytes,sizeof(bytes),&size)!=B_DENIED||fat_read(0,0,"/MISSING.TXT",bytes,sizeof(bytes),&size)!=B_ABSENT)return B_IO;
    return B_OK;
}
