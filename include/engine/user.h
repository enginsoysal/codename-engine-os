#ifndef ENGINE_USER_ABI_H
#define ENGINE_USER_ABI_H
#include <stdint.h>
/* Native development ABI v0. Guest Windows/Linux/Darwin ABIs remain separate. */
enum { S_WRITE=1,S_YIELD,S_EXIT,S_PID,S_EVENT_CREATE,S_EVENT_SIGNAL,S_EVENT_QUERY,
       S_DUP,S_CLOSE,S_SEND,S_RECEIVE,S_MEMORY_CREATE,S_MEMORY_MAP,S_TRANSFER };
enum { U_OK=0,U_INVALID=1,U_DENIED=2,U_UNSUPPORTED=3,U_NOMEM=4,U_BAD_HANDLE=5,U_EMPTY=6,U_LIMIT=7,U_BAD_TYPE=8 };
enum { R_READ=1,R_WRITE=2,R_TRANSFER=4,R_ALL=7 };
typedef struct {uint64_t sender,length;uint8_t data[32];} user_message;
typedef struct {uint64_t handle,marker;} user_payload;
#endif
