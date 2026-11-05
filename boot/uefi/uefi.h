#ifndef ENGINE_UEFI_H
#define ENGINE_UEFI_H
/* Original minimal declarations of public UEFI wire interfaces, x64 only.
   Normative source: UEFI 2.11 sections 4, 7, 9, 12, 13. No firmware implementation included. */
#include <engine/base.h>
typedef uint64_t efi_status;
typedef void *efi_handle;
typedef struct {uint32_t a;uint16_t b,c;uint8_t d[8];} efi_guid;
typedef struct {uint64_t signature;uint32_t revision,size,crc,reserved;} efi_header;
#define EFI_ERROR(s) (((s)>>63)!=0)
#define EFI_BUFFER_TOO_SMALL (UINT64_C(0x8000000000000005))
#define EFI_INVALID_PARAMETER (UINT64_C(0x8000000000000002))
typedef struct efi_file efi_file;
struct efi_file {
    uint64_t revision;
    efi_status (*open)(efi_file*,efi_file**,uint16_t*,uint64_t,uint64_t);
    efi_status (*close)(efi_file*);
    void *delete_file;
    efi_status (*read)(efi_file*,uint64_t*,void*);
    void *write,*get_position,*set_position;
    efi_status (*get_info)(efi_file*,efi_guid*,uint64_t*,void*);
};
typedef struct {uint64_t revision;efi_status (*open_volume)(void*,efi_file**);} efi_filesystem;
typedef struct {uint32_t revision;efi_handle parent;void *system;efi_handle device;
    void *path,*reserved;uint32_t options_size;void *options,*base;uint64_t size;
    uint32_t code_type,data_type;void *unload;} efi_loaded_image;
typedef struct {
    efi_header header;
    void *raise_tpl,*restore_tpl;
    efi_status (*allocate_pages)(uint32_t,uint32_t,uint64_t,uint64_t*);
    efi_status (*free_pages)(uint64_t,uint64_t);
    efi_status (*memory_map)(uint64_t*,void*,uint64_t*,uint64_t*,uint32_t*);
    void *allocate_pool,*free_pool,*create_event,*set_timer,*wait_event,*signal_event,*close_event,*check_event;
    void *install_protocol,*reinstall_protocol,*uninstall_protocol;
    efi_status (*handle_protocol)(efi_handle,efi_guid*,void**);
    void *reserved,*register_notify,*locate_handle,*locate_device_path,*install_config;
    void *load_image,*start_image,*exit_image,*unload_image;
    efi_status (*exit_boot_services)(efi_handle,uint64_t);
    void *next_count,*stall;
    efi_status (*watchdog)(uint64_t,uint64_t,uint64_t,uint16_t*);
    void *connect,*disconnect,*open_protocol,*close_protocol,*open_info,*protocols,*locate_handle_buffer;
    efi_status (*locate_protocol)(efi_guid*,void*,void**);
} efi_boot_services;
typedef struct {efi_guid guid;void *table;} efi_config;
typedef struct {
    efi_header header;uint16_t *vendor;uint32_t revision;
    efi_handle input_handle;void *input;efi_handle output_handle;void *output;
    efi_handle error_handle;void *error_output,*runtime;
    efi_boot_services *boot;uint64_t table_count;efi_config *tables;
} efi_system_table;
typedef struct {uint32_t version,width,height,format;uint32_t masks[4];uint32_t pixels_per_scanline;} efi_gop_info;
typedef struct {uint32_t max_mode,mode;efi_gop_info *info;uint64_t info_size,framebuffer,framebuffer_size;} efi_gop_mode;
typedef struct {void *query_mode,*set_mode,*blt;efi_gop_mode *mode;} efi_gop;
typedef struct {uint32_t type,pad;uint64_t physical,virtual_address,pages,attributes;} efi_memory_descriptor;
typedef struct {
    efi_header header;void *get_time,*set_time,*get_wakeup,*set_wakeup,*set_virtual_map,*convert_pointer;
    efi_status (*get_variable)(uint16_t*,efi_guid*,uint32_t*,uint64_t*,void*);
    void *next_variable;
    efi_status (*set_variable)(uint16_t*,efi_guid*,uint32_t,uint64_t,void*);
    void *next_high_count,*reset_system;
} efi_runtime_services;
_Static_assert(offsetof(efi_runtime_services,get_variable)==72,"UEFI variable getter layout");
_Static_assert(offsetof(efi_runtime_services,set_variable)==88,"UEFI variable setter layout");
_Static_assert(offsetof(efi_system_table,boot)==96,"UEFI system table layout");
_Static_assert(offsetof(efi_boot_services,memory_map)==56,"UEFI memory map layout");
_Static_assert(offsetof(efi_boot_services,exit_boot_services)==232,"UEFI exit layout");
_Static_assert(offsetof(efi_boot_services,locate_protocol)==320,"UEFI locate layout");
_Static_assert(offsetof(efi_loaded_image,device)==24,"UEFI loaded image layout");
_Static_assert(offsetof(efi_file,get_info)==64,"UEFI file layout");
#endif
