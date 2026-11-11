/* Per-slot geometry is generation-scoped; no per-window pixel buffers. */
typedef struct {uint64_t token;desktop_rect normal;unsigned mode;} window_state11;
static window_state11 window_geometry11[DESKTOP_WINDOWS];
static uint64_t switch_tokens11[DESKTOP_WINDOWS];
static unsigned switch_count11,switch_choice11,switch_open11,window_snap_preview11;
static desktop_rect window_before11,window_normal_before11;
static unsigned window_mode_before11,window_drag_edges11,window_keyboard_move11;
enum {W11_MAX=44000,W11_RESTORE,W11_LEFT,W11_RIGHT,W11_MOVE,W11_SIZE,W11_SWITCH=44100,W11_END=44200};
static void window_admit11(uint64_t);
static unsigned window_action11(uint64_t,unsigned);
static unsigned window_key11(int);
static unsigned window_menu11(i11_target,unsigned,unsigned);
static void window_caption11(desktop_window*);
static void window_overlay11(void);
static unsigned window_edges11(desktop_window*,int,int);
static void window_drag_begin11(i11_target);
static void window_drag_update11(unsigned,unsigned);
static void window_drag_end11(unsigned);

static unsigned move_cached9(desktop_rect,desktop_rect);

static void window_reflow11(void);
