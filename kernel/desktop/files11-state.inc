/* Bounded file-shell state; no per-window framebuffer or general heap queue. */
enum {F11_COPY=40000,F11_CUT,F11_PASTE,F11_RENAME,F11_RECYCLE,F11_RESTORE,F11_PERMANENT,F11_EMPTY,
 F11_PROPERTIES,F11_OPEN_WITH,F11_SHORTCUT,F11_UNDO,F11_VIEW,F11_SORT,F11_DIRECTION,F11_ADDRESS,
 F11_PLACES,F11_BIN,F11_NEW_TEXT,F11_CANCEL_JOB,F11_DIALOG_OK,F11_DIALOG_CANCEL,F11_KEEP_BOTH,
 F11_SKIP,F11_REPLACE,F11_ALWAYS,F11_ADDRESS_FIELD,F11_DESKTOP_PREV,F11_DESKTOP_NEXT,F11_OPEN,
 F11_SORT_NAME,F11_SORT_SIZE,F11_SORT_TYPE,F11_NAME_FIELD,F11_JOB_DETAILS,
 F11_ASSOC_APP=40100,F11_PLACE=40200,F11_CRUMB=41000,F11_DESKTOP_ITEM=33000};
enum {F11_JOB_COPY=1,F11_JOB_MOVE,F11_JOB_RECYCLE,F11_JOB_RESTORE,F11_JOB_REMOVE,F11_JOB_LINK};
enum {F11_DLG_RENAME=1,F11_DLG_CONFLICT,F11_DLG_PROPERTIES,F11_DLG_OPEN_WITH,F11_DLG_REMOVE,F11_DLG_EMPTY,F11_DLG_NEW_FOLDER,F11_DLG_NEW_TEXT,F11_DLG_RESULT};
enum {F11_NATIVE,F11_USB,F11_BUILTIN};
#define F11_ROOTS 128u
typedef struct {char path[256];unsigned view,sort,descending;} file_view_record11;
typedef struct {uint64_t magic;unsigned version,count,next,reserved;file_view_record11 records[32];} file_views11;
typedef struct {unsigned view,sort,descending,address_active;char address[256];unsigned cursor,anchor;} file_window11;
typedef struct {uint64_t generation;char path[256];unsigned count,sort,descending;uint16_t order[V6_ENTRIES];} file_listing11;
typedef struct {uint64_t magic;unsigned version,kind;char original[256];uint64_t size;uint32_t crc,reserved;} recycle_record11;
typedef struct {uint64_t magic;unsigned version,kind,app,reserved;char target[256];} shortcut_record11;
typedef struct {char extension[12];unsigned app;} association11;
typedef struct {uint64_t magic;unsigned version,count,next,reserved;association11 records[32];} associations11;
typedef struct {char source[256],target[256];unsigned provider,disk,context,part,kind,skip,replace;uint64_t size;uint32_t crc;} file_root11;
typedef struct {uint64_t first,size,path_hash;uint32_t crc;uint16_t index,root,depth,kind;} file_node11;
typedef struct {
 unsigned active,waiting,mode,count,node_count,node,root,phase,cancel,completed,skipped,files,folders,conflicts,replace_allowed;
 uint64_t owner,total_bytes,done_bytes,offset,verified,last_paint;
 file_root11 roots[F11_ROOTS];file_node11 nodes[V6_ENTRIES];
 char source[256],target[256],staged[256];uint64_t source_first,source_size;uint32_t source_crc;
} file_job11;
typedef struct {unsigned kind,count;file_root11 roots[F11_ROOTS];} file_clipboard11;
typedef struct {unsigned mode,focus,always,cursor,anchor,provider,item_kind;uint64_t owner,size;char path[256],input[256],error[128];} file_dialog11;
typedef struct {unsigned kind;uint64_t generation;char source[256],target[256];} file_undo11;
typedef struct {const char*label;unsigned command,enabled;} file_menu11;
static file_window11 file_windows11[DESKTOP_WINDOWS];
static file_listing11 file_lists11[DESKTOP_WINDOWS+1];
static file_views11 file_views_state11;
static associations11 file_associations11;
static char trash_names11[V6_ENTRIES][64];static uint64_t trash_names_generation11;
static file_clipboard11 file_clip11;static file_job11 file_job_state11;static file_dialog11 file_dialog_state11;static file_undo11 file_undo_state11;
static uint8_t file_buffer11[4096],desktop_files_selected11[V6_ENTRIES/8],marquee_desktop_files_saved11[V6_ENTRIES/8];
static unsigned desktop_page11,desktop_sort11,desktop_descending11,clipboard_kind11;
static desktop_rect file_field_rect11;
static unsigned file_field_scroll11;
static file_menu11 file_menu_items11[12];static unsigned file_menu_count11,file_menu_scroll11;static char file_menu_reason11[128],file_context_path11[256];
static uint64_t files_observed_generation11;
static unsigned file_drag11,file_drag_count11,file_drag_copy11,file_drag_x11,file_drag_y11;
static file_root11 file_drag_roots11[F11_ROOTS];
static unsigned file_job_error11;static char file_job_result11[256];
static const v6_entry *ordered_child11(application*,unsigned);
static const v6_entry *desktop_child11(unsigned);
static unsigned desktop_children11(void);
static unsigned selection_index11(application*,unsigned);
static unsigned file_selected_count11(application*);
static void file_visit11(application*);
static void file_settings_load11(void);
static void file_refresh_trash_names11(void);
static const char *file_label11(const v6_entry*);
static void draw_files11(desktop_window*,application*,uint64_t);
static void files_draw_overlay11(void);
static void files_draw_context11(void);
static void files_context_open11(i11_target,unsigned,unsigned);
static void files_context_invoke11(unsigned);
static unsigned files_context_next11(unsigned,int);
static unsigned file_action11(uint64_t,unsigned);
static unsigned files_key11(int);
static void files_pump11(void);
static unsigned files_pending11(void);
static void files_poll11(void);
static void file_open11(const char*,unsigned);
static unsigned files_begin_drag11(i11_target,unsigned);
static void files_drag_motion11(unsigned,unsigned,unsigned);
static void files_drop11(i11_target,unsigned);
static void files_cancel_drag11(void);
static unsigned file_modal_control11(uint64_t,unsigned);
static unsigned file_block_control11(uint64_t,unsigned);
static unsigned file_block_owner11(uint64_t);
static void desktop_select11(unsigned,unsigned,unsigned);
static void draw_desktop_files11(void);
static void file_created_folder11(const char*);
static unsigned file_default_app11(const char*);
static void file_copy_selection11(uint64_t,unsigned);
static void files_sort_menu11(uint64_t);
static void files_places_menu11(uint64_t);
static void action(uint64_t,unsigned);
static void close_window(uint64_t,int);
static unsigned file_columns11(const desktop_window*);
static unsigned file_rows11(const desktop_window*);
static unsigned file_page_size11(const desktop_window*);

static void select_row11(application*,unsigned,unsigned,unsigned);
