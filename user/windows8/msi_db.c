/* Bounded CFB v3 and MSI relational database reader; no host installer calls. */
#include "msi_db.h"
#define END8 0xfffffffeu
#define FREE8 0xffffffffu
static const uint8_t *compound8;
static unsigned compound_size8,sectors8,fat8[16384],mini_fat8[16384],mini_count8;
static uint8_t *directory8,*mini_stream8;
static unsigned directory_size8,mini_size8;
static char *strings8[8192];
static unsigned string_count8;
static uint16_t u16db8(const void*p){const uint8_t*b=p;return b[0]|(uint16_t)b[1]<<8;}
static uint32_t u32db8(const void*p){const uint8_t*b=p;return b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static void require8(int condition,const char*message){if(!condition)fail8(message);}
static const uint8_t *sector8(unsigned sector){require8(sector<sectors8,"MSI sector is outside the package.");return compound8+512+(uint64_t)sector*512;}
static msi8_blob chain8(unsigned start,unsigned bytes,unsigned mini){
 require8(bytes<=8*1024*1024,"MSI stream exceeds the supported size.");
 msi8_blob result={alloc8(bytes?bytes:1),bytes};require8(result.data!=0,"Not enough memory for the MSI stream.");
 unsigned size=mini?64:512,at=start,done=0,hops=0;
 while(done<bytes){require8(++hops<=16384,"MSI sector chain has a cycle.");const uint8_t*source;
  if(mini){require8(at<mini_count8&&(uint64_t)at*64+64<=mini_size8,"MSI mini stream chain is invalid.");source=mini_stream8+at*64;}
  else source=sector8(at);
  unsigned count=bytes-done;if(count>size)count=size;memcpy(result.data+done,source,count);done+=count;
  require8(at<(mini?mini_count8:sectors8),"MSI allocation index is invalid.");at=mini?mini_fat8[at]:fat8[at];
 }
 require8(!bytes||at==END8,"MSI stream chain has unexpected trailing sectors.");return result;
}
static const char alphabet8[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";
static void stream_name8(const uint8_t*entry,char*out,unsigned cap,unsigned*table){
 unsigned bytes=u16db8(entry+64),used=0;require8(bytes>=2&&bytes<=64&&!(bytes&1),"MSI stream name is invalid.");*table=0;
 for(unsigned i=0;i+2<bytes;i+=2){unsigned c=u16db8(entry+i);if(!i&&c==0x4840){*table=1;continue;}
  if(c>=0x3800&&c<0x4800){c-=0x3800;require8(used+2<cap,"MSI stream name is too long.");out[used++]=alphabet8[c&63];out[used++]=alphabet8[c>>6];}
  else {if(c>=0x4800&&c<0x4840)c=(uint8_t)alphabet8[c-0x4800];require8((c>=32||(!i&&c==5))&&c<127&&used+1<cap,"MSI stream name encoding is unsupported.");out[used++]=(char)c;}
 }out[used]=0;
}
msi8_blob msi8_stream(const char*name,unsigned table){
 for(unsigned at=0;at+128<=directory_size8;at+=128){const uint8_t*e=directory8+at;if(e[66]!=2)continue;char actual[128];unsigned kind;stream_name8(e,actual,sizeof(actual),&kind);
  if(kind==table&&equal8(name,actual)){require8(!u32db8(e+124),"MSI stream is larger than supported.");unsigned bytes=u32db8(e+120);return chain8(u32db8(e+116),bytes,bytes<4096);}}
 return (msi8_blob){0,0};
}
static unsigned column_width8(unsigned type){if(type&0x800)return 2;unsigned width=type&255;require8(width==2||width==4,"MSI column type is unsupported.");return width;}
static unsigned value8(const msi8_table*t,unsigned row,unsigned column){
 require8(row<t->rows&&column<t->columns,"MSI table access is outside its bounds.");unsigned offset=0;
 for(unsigned i=0;i<column;++i)offset+=column_width8(t->column[i].type)*t->rows;
 unsigned width=column_width8(t->column[column].type);offset+=row*width;require8(offset+width<=t->data.size,"MSI table data is truncated.");return width==2?u16db8(t->data.data+offset):u32db8(t->data.data+offset);
}
const char *msi8_string(unsigned id){require8(id<string_count8,"MSI string reference is invalid.");return strings8[id]?strings8[id]:"";}
unsigned msi8_raw(const msi8_table*t,unsigned row,unsigned column){return value8(t,row,column);}
const char *msi8_text(const msi8_table*t,unsigned row,unsigned column){require8(t->column[column].type&0x800,"MSI column is not a string.");return msi8_string(value8(t,row,column));}
int32_t msi8_integer(const msi8_table*t,unsigned row,unsigned column){require8(!(t->column[column].type&0x800),"MSI column is not numeric.");unsigned v=value8(t,row,column);return !v?INT32_MIN:(column_width8(t->column[column].type)==2?(int32_t)v-0x8000:(int32_t)(v^0x80000000u));}
static msi8_table columns_table8;
msi8_table msi8_table_open(const char*name){
 msi8_table t={0};copy8(t.name,name,sizeof(t.name));unsigned width=0;
 for(unsigned row=0;row<columns_table8.rows;++row)if(equal8(msi8_text(&columns_table8,row,0),name)){
  int number=msi8_integer(&columns_table8,row,1);require8(number>=1&&number<=32,"MSI column number is unsupported.");unsigned i=(unsigned)number-1;require8(!t.column[i].type,"MSI has duplicate columns.");copy8(t.column[i].name,msi8_text(&columns_table8,row,2),64);t.column[i].type=(unsigned)msi8_integer(&columns_table8,row,3);if((unsigned)number>t.columns)t.columns=(unsigned)number;
 }
 for(unsigned i=0;i<t.columns;++i){require8(t.column[i].type!=0,"MSI column sequence has a gap.");width+=column_width8(t.column[i].type);}
 t.data=msi8_stream(name,1);if(!t.data.data){require8(!t.columns,"MSI table stream is missing.");return t;}
 require8(width&&!(t.data.size%width)&&t.data.size/width<=4096,"MSI table row layout is unsupported.");t.rows=t.data.size/width;return t;
}
void msi8_database(const void*data,unsigned bytes){
 static const uint8_t magic[]={0xd0,0xcf,0x11,0xe0,0xa1,0xb1,0x1a,0xe1};compound8=data;compound_size8=bytes;
 require8(bytes>=512&&bytes<=8*1024*1024&&!(bytes%512)&&!memcmp(data,magic,8),"This is not a supported MSI compound file.");
 require8(u16db8(compound8+26)==3&&u16db8(compound8+28)==0xfffe&&u16db8(compound8+30)==9&&u16db8(compound8+32)==6&&u32db8(compound8+56)==4096,"MSI compound-file geometry is unsupported.");
 sectors8=bytes/512-1;require8(sectors8<=16384&&u32db8(compound8+44)<=109&&u32db8(compound8+72)==0,"MSI extended FAT is unsupported.");
 unsigned fat_sectors=u32db8(compound8+44),count=0;require8(fat_sectors*128>=sectors8,"MSI FAT is truncated.");
 for(unsigned i=0;i<fat_sectors;++i){const uint8_t*s=sector8(u32db8(compound8+76+i*4));for(unsigned j=0;j<128&&count<16384;++j)fat8[count++]=u32db8(s+j*4);}
 unsigned at=u32db8(compound8+48),dir_sectors=0;
 for(;at!=END8;at=fat8[at]){require8(at<sectors8&&++dir_sectors<=1024,"MSI directory chain is invalid.");}
 msi8_blob dir=chain8(u32db8(compound8+48),dir_sectors*512,0);directory8=dir.data;directory_size8=dir.size;
 const uint8_t*root=directory8;require8(directory_size8>=128&&root[66]==5&&!u32db8(root+124),"MSI root storage is invalid.");
 msi8_blob mini=chain8(u32db8(root+116),u32db8(root+120),0);mini_stream8=mini.data;mini_size8=mini.size;
 unsigned mini_sectors=u32db8(compound8+64);require8(mini_sectors<=128,"MSI mini FAT exceeds its bound.");
 msi8_blob mini_fat=chain8(u32db8(compound8+60),mini_sectors*512,0);mini_count8=mini_fat.size/4;for(unsigned i=0;i<mini_count8;++i)mini_fat8[i]=u32db8(mini_fat.data+i*4);free8(mini_fat.data);
 msi8_blob pool=msi8_stream("_StringPool",1),text=msi8_stream("_StringData",1);require8(pool.data&&text.data&&pool.size>=4&&!(pool.size%4),"MSI string pool is missing.");
 unsigned codepage=u32db8(pool.data);require8(codepage==1252||codepage==65001||codepage==0,"MSI code page is unsupported.");
 string_count8=pool.size/4;require8(string_count8<=8192,"MSI has too many strings.");strings8[0]="";unsigned offset=0;
 for(unsigned i=1;i<string_count8;++i){unsigned length=u16db8(pool.data+i*4);require8(length||!u16db8(pool.data+i*4+2),"MSI long string encoding is unsupported.");require8(length<=4096&&offset+length<=text.size,"MSI string data is truncated.");char*out=alloc8(length*3+1);require8(out!=0,"Not enough memory for MSI strings.");unsigned used=0;
  for(unsigned j=0;j<length;++j){unsigned c=text.data[offset+j];require8(c!=0,"MSI string contains an embedded terminator.");if(codepage!=65001&&c>=128){require8(c>=160,"MSI CP1252 control-range characters are unsupported.");out[used++]=(char)(0xc0|(c>>6));out[used++]=(char)(0x80|(c&63));}else out[used++]=(char)c;}out[used]=0;strings8[i]=out;offset+=length;
 }require8(offset==text.size,"MSI string data contains trailing bytes.");free8(pool.data);free8(text.data);
 columns_table8=(msi8_table){.columns=4,.column={{"Table",0x800},{"Number",2},{"Name",0x800},{"Type",2}},.data=msi8_stream("_Columns",1)};
 require8(columns_table8.data.data&&!(columns_table8.data.size%8)&&columns_table8.data.size/8<=4096,"MSI column catalog is invalid.");columns_table8.rows=columns_table8.data.size/8;(void)compound_size8;
}
