#include <engine/network/http.h>
#include <engine/base.h>
static unsigned lower(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static unsigned equal(const char *a,const char *b){while(*a&&lower((uint8_t)*a)==lower((uint8_t)*b)){++a;++b;}return !*a&&!*b;}
static unsigned token(unsigned c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='!'||c=='#'||c=='$'||c=='%'||c=='&'||c=='\''||c=='*'||c=='+'||c=='-'||c=='.'||c=='^'||c=='_'||c=='`'||c=='|'||c=='~';}
void http7_init(http7_response *r){memset(r,0,sizeof(*r));}
static unsigned line(http7_response *r){
 char *p=r->line;
 if(r->state==HTTP7_STATUS){
  if(r->line_size<12||memcmp(p,"HTTP/1.",7)||(p[7]!='0'&&p[7]!='1')||p[8]!=' '||p[9]<'1'||p[9]>'5'||p[10]<'0'||p[10]>'9'||p[11]<'0'||p[11]>'9'||(p[12]&&p[12]!=' '))return 1;
  r->status=(p[9]-'0')*100+(p[10]-'0')*10+p[11]-'0';r->state=HTTP7_HEADERS;return 0;
 }
 if(r->state==HTTP7_CHUNK_SIZE){
  uint64_t size=0;unsigned n=0;
  while(p[n]&&p[n]!=';'){
   unsigned c=lower((uint8_t)p[n++]),digit=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:16;
   if(digit==16||size>(HTTP7_LIMIT-digit)/16)return 1;size=size*16+digit;
  }
  if(!n||size>HTTP7_LIMIT-r->received)return 1;
  r->remaining=size;r->state=size?HTTP7_CHUNK_BODY:HTTP7_TRAILERS;return 0;
 }
 if(!r->line_size){
  if(r->state==HTTP7_TRAILERS){r->state=HTTP7_DONE;return 0;}
  if(r->status<200){
   if(r->status==101||++r->interim>4||r->seen_length||r->chunked)return 1;
   r->state=HTTP7_STATUS;r->encoding=0;r->location[0]=0;return 0;
  }
  if(r->chunked&&r->seen_length)return 1;
  if(r->status==301||r->status==302||r->status==303||r->status==307||r->status==308){if(!r->location[0])return 1;r->state=HTTP7_REDIRECT;return 0;}
  if(r->status!=200)return 1;
  if(r->chunked)r->state=HTTP7_CHUNK_SIZE;
  else if(r->seen_length){r->remaining=r->length;r->state=r->length?HTTP7_FIXED:HTTP7_DONE;}
  else r->state=HTTP7_UNTIL_CLOSE;
  return 0;
 }
 unsigned colon=0;while(p[colon]&&p[colon]!=':'){if(!token((uint8_t)p[colon]))return 1;++colon;}
 if(!colon||!p[colon])return 1;p[colon++]=0;
 char *value=p+colon;while(*value==' '||*value=='\t')++value;
 unsigned end=0;while(value[end])++end;while(end&&(value[end-1]==' '||value[end-1]=='\t'))value[--end]=0;
 if(r->state==HTTP7_TRAILERS){if(equal(p,"content-length")||equal(p,"transfer-encoding")||equal(p,"content-encoding"))return 1;return 0;}
 if(equal(p,"content-length")){
  if(r->seen_length++||!*value)return 1;uint64_t length=0;
  for(unsigned n=0;value[n];++n){unsigned digit=(uint8_t)value[n]-'0';if(digit>9||length>(HTTP7_LIMIT-digit)/10)return 1;length=length*10+digit;}r->length=length;
 }else if(equal(p,"transfer-encoding")){if(r->chunked||!equal(value,"chunked"))return 1;r->chunked=1;}
 else if(equal(p,"content-encoding")){if(r->encoding++||!equal(value,"identity"))return 1;}
 else if(equal(p,"location")){
  if(r->location[0]||end>=sizeof(r->location))return 1;memcpy(r->location,value,end+1);
 }
 return 0;
}
unsigned http7_feed(http7_response *r,const void *input,unsigned bytes,http7_sink sink,void *context){
 if(!r||(!input&&bytes))return 1;const uint8_t *data=input;
 while(bytes){
  if(r->state==HTTP7_FAILED)return 1;
  if(r->state==HTTP7_REDIRECT)return 0;
  if(r->state==HTTP7_DONE)goto fail;
  if(r->state==HTTP7_FIXED||r->state==HTTP7_CHUNK_BODY||r->state==HTTP7_UNTIL_CLOSE){
   unsigned count=bytes;if(r->state!=HTTP7_UNTIL_CLOSE&&count>r->remaining)count=(unsigned)r->remaining;
   if(count>HTTP7_LIMIT-r->received||!sink||sink(context,data,count))goto fail;
   r->received+=count;data+=count;bytes-=count;
   if(r->state!=HTTP7_UNTIL_CLOSE){r->remaining-=count;if(!r->remaining)r->state=r->state==HTTP7_FIXED?HTTP7_DONE:HTTP7_CHUNK_CR;}
   continue;
  }
  unsigned c=*data++;--bytes;
  if(r->state==HTTP7_CHUNK_CR){if(c!='\r')goto fail;r->state=HTTP7_CHUNK_LF;continue;}
  if(r->state==HTTP7_CHUNK_LF){if(c!='\n')goto fail;r->state=HTTP7_CHUNK_SIZE;continue;}
  if(r->state!=HTTP7_CHUNK_SIZE&&++r->header_bytes>16384)goto fail;
  if(r->cr){
   if(c!='\n')goto fail;r->cr=0;r->line[r->line_size]=0;
   if(line(r))goto fail;r->line_size=0;continue;
  }
  if(c=='\r'){r->cr=1;continue;}
  if(c=='\n'||(!c)||(c<32&&c!='\t')||c==127||r->line_size>=sizeof(r->line)-1)goto fail;
  r->line[r->line_size++]=(char)c;
 }
 return 0;
fail:r->state=HTTP7_FAILED;return 1;
}
unsigned http7_finish(http7_response *r){
 if(r->state==HTTP7_UNTIL_CLOSE)r->state=HTTP7_DONE;
 if(r->state!=HTTP7_DONE){r->state=HTTP7_FAILED;return 1;}return 0;
}
