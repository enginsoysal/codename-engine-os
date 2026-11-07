#include <engine/network/download.h>
#include <engine/network/network.h>
#include <engine/network/tls.h>
#include <engine/network/clock.h>
#include <engine/network/http.h>
#include <engine/m3.h>
static download7_info info;
static net7_url url;
static v6_volume *volume;
static http7_response response;
static unsigned redirects,reserved,request_size,request_sent;
static char request[1024];
static uint64_t started,last_paint;
static void state(unsigned value,const char *message){info.state=value;info.message=message;++info.revision;}
static unsigned active(void){return info.state>=DOWNLOAD7_RESOLVING&&info.state<=DOWNLOAD7_RECEIVING;}
static void fail(const char *message){info.tls_error=tls7_error();tls7_abort();if(volume)v6_stream_abort(volume);reserved=0;state(DOWNLOAD7_FAILED,message);}
download7_info download7_current(void){return info;}
void download7_cancel(void){if(active()){tls7_abort();v6_stream_abort(volume);reserved=0;state(DOWNLOAD7_CANCELLED,"Download cancelled. No partial file was added.");}}
static unsigned copy(char *out,const char *in,unsigned cap){unsigned n=0;while(n+1<cap&&in[n]){out[n]=in[n];++n;}out[n]=0;return !in[n];}
static void request_text(const char *text){while(*text&&request_size<sizeof(request)-1)request[request_size++]=*text++;request[request_size]=0;}
static void make_request(void){
 request_size=request_sent=0;request_text("GET ");request_text(url.path);request_text(" HTTP/1.1\r\nHost: ");request_text(url.host);
 if(url.port!=443){char digits[6];unsigned n=0,value=url.port;do{digits[n++]=(char)('0'+value%10);value/=10;}while(value);request_text(":");while(n){char c[2]={digits[--n],0};request_text(c);}}
 request_text("\r\nUser-Agent: EngineOS/0.7\r\nAccept: */*\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n");
}
unsigned download7_start(v6_volume *v,const char *source,const char *path){
 if(active())return 1;tls7_abort();memset(&info,0,sizeof(info));volume=v;reserved=redirects=0;
 if(!v||!path||!copy(info.path,path,sizeof(info.path))||net7_parse_url(source,&url)){fail("Use a valid https:// address and a new Downloads filename.");return 1;}
 clock7_time now;if(clock7_read(&now)){fail("The UTC hardware clock is invalid. Correct the VM clock and retry.");return 1;}
 if(net7_resolve(url.host)){fail("Cannot resolve this address. Check the network and hardware entropy.");return 1;}
 started=last_paint=m3_clock_ticks();state(DOWNLOAD7_RESOLVING,"Finding the download server...");net7_schedule();return 0;
}
static unsigned reserve(void){
 if(reserved)return 0;
 uint64_t limit=response.seen_length?response.length:HTTP7_LIMIT;if(!limit)limit=1;
 unsigned result=v6_stream_begin(volume,info.path,limit);if(result)return result;reserved=1;return 0;
}
static unsigned body(void *context,const void *input,unsigned bytes){
 (void)context;if(reserve())return 1;const uint8_t *data=input;
 while(bytes){unsigned count=4096-(unsigned)(info.bytes%4096);if(count>bytes)count=bytes;
  if(v6_stream_append(volume,data,count))return 1;info.bytes+=count;data+=count;bytes-=count;
 }return 0;
}
static unsigned redirect(void){
 if(++redirects>4)return 1;net7_url next;
 if(response.location[0]=='/'&&response.location[1]!='/'){
  next=url;if(!copy(next.path,response.location,sizeof(next.path)))return 1;
  /* Revalidate a same-origin absolute path with the same strict URL parser. */
  char full[800]="https://";unsigned n=8,h=0;while(next.host[h])full[n++]=next.host[h++];
  if(next.port!=443){char digits[6];unsigned count=0,value=next.port;full[n++]=':';do{digits[count++]=(char)('0'+value%10);value/=10;}while(value);while(count)full[n++]=digits[--count];}
  h=0;while(next.path[h])full[n++]=next.path[h++];full[n]=0;if(net7_parse_url(full,&next))return 1;
 }else if(net7_parse_url(response.location,&next))return 1;
 tls7_abort();url=next;if(net7_resolve(url.host))return 1;
 state(DOWNLOAD7_RESOLVING,"Following a checked HTTPS redirect...");return 0;
}
void download7_pump(void){
 if(!active())return;
 if(net7_current().state!=NET7_CONNECTED){fail("Connection lost. Retry the download; your other files are unchanged.");return;}
 if(m3_clock_ticks()-started>30000){fail("Download timed out. No partial file was added.");return;}
 if(info.state==DOWNLOAD7_RESOLVING){
  uint32_t address=0;unsigned dns=net7_dns_status(&address);
  if(dns==NET7_DNS_FAILED){fail("The server name could not be resolved. Check the address and retry.");return;}
  if(dns!=NET7_DNS_READY)return;
  clock7_time now;if(clock7_read(&now)){fail("The UTC hardware clock is invalid. Correct it before downloading.");return;}
  if(tls7_start(url.host,address,url.port,now.days,now.seconds)){fail("Secure connection could not start. Check clock, entropy and network.");return;}
  http7_init(&response);make_request();state(DOWNLOAD7_CONNECTING,"Checking the server certificate and secure connection...");
 }
 unsigned tls=tls7_status();
 if(tls==TLS7_FAILED){
  fail("Secure connection or certificate validation failed. Check the address and UTC clock.");return;
 }
 if(tls!=TLS7_READY&&tls!=TLS7_PEER_CLOSED)return;
 if(request_sent<request_size){unsigned count=tls7_write(request+request_sent,request_size-request_sent);request_sent+=count;if(request_sent<request_size)return;state(DOWNLOAD7_RECEIVING,"Downloading securely to your Downloads folder...");}
 uint8_t data[2048];
 for(unsigned budget=0;budget<2;++budget){
  unsigned count=tls7_read(data,sizeof(data));if(!count)break;
  if(http7_feed(&response,data,count,body,0)){fail("Download response or disk write failed. No partial file was added.");return;}
  if(response.state==HTTP7_REDIRECT){if(redirect())fail("This redirect is unsupported or unsafe. Use the final HTTPS address.");return;}
  if(response.state==HTTP7_DONE)break;
 }
 if(tls==TLS7_PEER_CLOSED&&http7_finish(&response)){fail("The server ended an incomplete download. Retry the file.");return;}
 if(response.state==HTTP7_DONE){
  if(reserve()||v6_stream_finish(volume)){fail("The downloaded file could not be committed. Check free space and disk health.");return;}
  reserved=0;tls7_abort();state(DOWNLOAD7_DONE,"Download complete. The file is saved in your Downloads folder.");
  serial_text("M7_HTTPS_DOWNLOAD_SAVED bytes=");serial_hex(info.bytes);serial_text("\n");return;
 }
 if(m3_clock_ticks()-last_paint>=50){last_paint=m3_clock_ticks();++info.revision;}
}
