#include <engine/network/tls.h>
#include <engine/network/tcp.h>
#include <engine/network/network.h>
#include <engine/base.h>
#include <bearssl.h>
#include "trust7.h"

/* One bounded TLS 1.2 client; explicit entropy, time, hostname and public roots. */
static br_ssl_client_context client;
static br_x509_minimal_context verifier;
static uint8_t buffer[BR_SSL_BUFSIZE_BIDI];
static unsigned status,error;
static const uint16_t suites[]={
    BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
    BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
    BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
    BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
};
static void failed(unsigned code){error=code;status=TLS7_FAILED;tcp7_abort();memset(&client,0,sizeof(client));memset(&verifier,0,sizeof(verifier));memset(buffer,0,sizeof(buffer));}
unsigned tls7_status(void){return status;}
unsigned tls7_error(void){return error;}
void tls7_abort(void){tcp7_abort();memset(&client,0,sizeof(client));memset(&verifier,0,sizeof(verifier));memset(buffer,0,sizeof(buffer));status=TLS7_CLOSED;error=0;}
unsigned tls7_start(const char *host,uint32_t address,unsigned port,uint32_t days,uint32_t seconds){
    if(status==TLS7_CONNECTING||status==TLS7_READY)return 1;
    tls7_abort();
    unsigned length=0;if(host)while(length<254&&host[length])++length;
    /* Clock policy covers 2020..2099 UTC, never an absent clock/expiry bypass. */
    if(!length||length>253||days<737790||days>=767010||seconds>=86400){failed(1001);return 1;}
    uint8_t entropy[48];if(!net7_random(entropy,sizeof(entropy))){memset(entropy,0,sizeof(entropy));failed(1002);return 1;}
    br_ssl_client_init_full(&client,&verifier,root7_anchors,ROOT7_COUNT);
    br_ssl_engine_set_versions(&client.eng,BR_TLS12,BR_TLS12);
    br_ssl_engine_set_suites(&client.eng,suites,sizeof(suites)/sizeof(suites[0]));
    br_ssl_engine_add_flags(&client.eng,BR_OPT_NO_RENEGOTIATION);
    br_ssl_engine_set_buffer(&client.eng,buffer,sizeof(buffer),1);
    br_x509_minimal_set_time(&verifier,days,seconds);
    br_ssl_engine_inject_entropy(&client.eng,entropy,sizeof(entropy));memset(entropy,0,sizeof(entropy));
    if(!br_ssl_client_reset(&client,host,0)){failed((unsigned)br_ssl_engine_last_error(&client.eng));return 1;}
    if(tcp7_connect(address,port)){failed(1003);return 1;}
    status=TLS7_CONNECTING;net7_schedule();return 0;
}
void tls7_pump(void){
    if(status!=TLS7_CONNECTING&&status!=TLS7_READY)return;
    if(tcp7_status()==TCP7_FAILED){failed(1004);return;}
    if(tcp7_status()==TCP7_CONNECTING)return;
    for(unsigned budget=0;budget<8;++budget){
        unsigned state=br_ssl_engine_current_state(&client.eng);
        if(state&BR_SSL_CLOSED){
            unsigned code=(unsigned)br_ssl_engine_last_error(&client.eng);
            if(code)failed(code);else {status=TLS7_PEER_CLOSED;tcp7_close();}return;
        }
        if(state&BR_SSL_SENDAPP)status=TLS7_READY;
        unsigned progress=0;size_t length=0;
        uint8_t *data=br_ssl_engine_sendrec_buf(&client.eng,&length);
        if(length){unsigned count=tcp7_write(data,(unsigned)length);if(count){br_ssl_engine_sendrec_ack(&client.eng,count);progress=1;}}
        data=br_ssl_engine_recvrec_buf(&client.eng,&length);
        if(length){unsigned count=tcp7_read(data,(unsigned)length);if(count){br_ssl_engine_recvrec_ack(&client.eng,count);progress=1;}}
        if(!progress){
            if(tcp7_status()==TCP7_PEER_CLOSED&&!(state&BR_SSL_RECVAPP))failed(1005);
            return;
        }
    }
    net7_schedule();
}
unsigned tls7_write(const void *input,unsigned bytes){
    if(status!=TLS7_READY||!input||!bytes)return 0;size_t size=0;
    uint8_t *data=br_ssl_engine_sendapp_buf(&client.eng,&size);
    unsigned count=bytes>size?(unsigned)size:bytes;
    if(count){memcpy(data,input,count);br_ssl_engine_sendapp_ack(&client.eng,count);br_ssl_engine_flush(&client.eng,0);net7_schedule();}return count;
}
unsigned tls7_read(void *output,unsigned bytes){
    if(status!=TLS7_READY||!output||!bytes)return 0;size_t size=0;
    const uint8_t *data=br_ssl_engine_recvapp_buf(&client.eng,&size);
    unsigned count=bytes>size?(unsigned)size:bytes;
    if(count){memcpy(output,data,count);br_ssl_engine_recvapp_ack(&client.eng,count);net7_schedule();}return count;
}
void tls7_close(void){if(status==TLS7_READY){br_ssl_engine_close(&client.eng);net7_schedule();}}
