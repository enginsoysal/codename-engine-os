#ifndef OS_M0_SYSCALL_V0_H
#define OS_M0_SYSCALL_V0_H
/* Native wire layouts; never directly reuse for guest NT/Linux/Darwin structs. */
#include <stdint.h>
#include <stddef.h>
typedef uint64_t os_handle;
typedef uint32_t os_status;
enum { OS_OK = 0, OS_INVALID_ARGUMENT = 1, OS_ACCESS_DENIED = 2,
       OS_NOT_SUPPORTED = 3, OS_NO_MEMORY = 4, OS_BAD_HANDLE = 5 };
typedef struct { uint32_t structure_size, version; } os_request_header;
typedef struct { os_request_header header; uint64_t address, byte_length; } os_user_buffer_v0;
typedef struct { os_request_header header; os_handle handle; uint64_t rights; } os_handle_request_v0;
_Static_assert(sizeof(os_request_header) == 8, "request header layout");
_Static_assert(sizeof(os_user_buffer_v0) == 24, "buffer layout");
_Static_assert(sizeof(os_handle_request_v0) == 24, "handle request layout");
_Static_assert(offsetof(os_user_buffer_v0, address) == 8, "address offset");
#endif
