#ifndef ENGINE_NETWORK_HTTP_H
#define ENGINE_NETWORK_HTTP_H
#include <stdint.h>
#define HTTP7_LIMIT (64u*1024*1024)
enum { HTTP7_STATUS,HTTP7_HEADERS,HTTP7_FIXED,HTTP7_CHUNK_SIZE,HTTP7_CHUNK_BODY,HTTP7_CHUNK_CR,HTTP7_CHUNK_LF,HTTP7_TRAILERS,HTTP7_DONE,HTTP7_FAILED,HTTP7_REDIRECT,HTTP7_UNTIL_CLOSE };
typedef unsigned (*http7_sink)(void *,const void *,unsigned);
typedef struct {
 unsigned state,status,line_size,header_bytes,interim,seen_length,chunked,encoding,cr;
 uint64_t length,received,remaining;
 char line[4096],location[3072];
#ifdef ENGINE_M74
 char content_type[64];
#endif
} http7_response;
void http7_init(http7_response *);
unsigned http7_feed(http7_response *,const void *,unsigned,http7_sink,void *);
unsigned http7_finish(http7_response *);
#endif
