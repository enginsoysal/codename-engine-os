/* Stable catalogue IDs and bounded preferences; no executable-path launcher. */
#include <engine/desktop/start11.h>
enum {S11_PINNED=42000,S11_ALL,S11_CATEGORIES_VIEW,S11_VIEW,S11_SEARCH,S11_PREV,S11_NEXT,S11_CUSTOMIZE,S11_BACK,S11_DEFAULTS,S11_CAT_NEW,S11_CAT_RENAME,S11_CAT_HIDE,S11_CAT_UP,S11_CAT_DOWN,S11_PIN_REMOVE,S11_PIN_UP,S11_PIN_DOWN,S11_ALIGN,S11_POWER,S11_RESTART,S11_SHUTDOWN,S11_CONFIRM_CANCEL,S11_CONFIRM_OK,S11_NAME_FIELD,S11_NAME_OK,S11_NAME_CANCEL,S11_CATEGORY_MENU,S11_MANAGE_CATS,S11_MANAGE_PINS,S11_MANAGE_TASKS,S11_TASK_REMOVE,S11_PIN_RENAME,S11_FOLDER_OPEN,S11_TASK_UP,S11_TASK_DOWN,
 S11_APP=42100,S11_RESULT=42200,S11_CATEGORY=42300,S11_PIN_SELECT=42400,S11_ASSIGN=42500,S11_CONTEXT_PIN=42600,S11_CONTEXT_TASK=42700,S11_CONTEXT_ASSIGN=42800,S11_TASK_PIN=42900,S11_PIN_FOLDER=43000,S11_MORE_PINS,S11_WINDOWS,S11_WINDOW=43020,S11_TASK_OPEN=43100,S11_TASK_SELECT=43300,S11_END=43400};
typedef struct {unsigned kind,id;} start_result11;
static s11_preferences start_preferences11;
static start_result11 start_results11[S11_APPS+S11_PINS];static unsigned start_result_count11,start_cursor11,start_page11,start_category11;
static unsigned start_manage11,start_manage_page11,start_pin_selected11,start_task_selected11,start_assign_app11;
static unsigned start_search_focus11=1,start_query_cursor11,start_query_anchor11,start_name_mode11,start_name_cursor11,start_name_anchor11,start_name_focus11,start_confirm11;
static char start_query11[256],start_name11[256],start_notice11[160],start_folder11[256];
static desktop_rect start_bounds11;static uint64_t start_window_menu11[DESKTOP_WINDOWS];
static unsigned start_power_shutdown11,start_context_from_pin11;static s11_pin start_context_pin11,start_edit_pin11;
static void start_load11(void);
static void start_draw11(void);
static void start_taskbar11(int);
static unsigned start_action11(uint64_t,unsigned);
static unsigned start_key11(int);
static unsigned start_context11(i11_target,unsigned,unsigned);
static unsigned start_dispatch11(i11_target,unsigned,unsigned,unsigned);
static uint64_t start_identity11(i11_target);
static void start_removed11(unsigned);
static void start_follow_move11(const char*,const char*);
static void power_init11(const os_boot_info_v0*);
static unsigned power_request11(unsigned);

static void start_offer_folder11(i11_target);

static unsigned start_page_size11(void);
static unsigned start_manage_rows11(void);
