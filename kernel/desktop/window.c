#include <engine/desktop/window.h>

static int32_t maximum(int32_t a, int32_t b) { return a > b ? a : b; }
static int32_t minimum(int32_t a, int32_t b) { return a < b ? a : b; }

static desktop_rect clipped(const desktop_model *m, desktop_rect r) {
    /* Compute ends in 64 bits: callers can supply hostile signed coordinates. */
    int64_t end_x = (int64_t)r.x + r.width, end_y = (int64_t)r.y + r.height;
    int32_t x = maximum(0, r.x), y = maximum(0, r.y);
    int32_t right = end_x < 0 ? 0 : end_x > m->width ? (int32_t)m->width : (int32_t)end_x;
    int32_t bottom = end_y < 0 ? 0 : end_y > m->height ? (int32_t)m->height : (int32_t)end_y;
    return (desktop_rect){x, y, maximum(0, right - x), maximum(0, bottom - y)};
}

void desktop_damage(desktop_model *m, desktop_rect r) {
    if (r.width <= 0 || r.height <= 0) return;
    r = clipped(m, r);
    if (!r.width || !r.height) return;
    if (!m->damaged) m->damage = r;
    else {
        int32_t right = maximum(m->damage.x + m->damage.width, r.x + r.width);
        int32_t bottom = maximum(m->damage.y + m->damage.height, r.y + r.height);
        m->damage.x = minimum(m->damage.x, r.x);
        m->damage.y = minimum(m->damage.y, r.y);
        m->damage.width = right - m->damage.x;
        m->damage.height = bottom - m->damage.y;
    }
    m->damaged = 1;
}

int desktop_take_damage(desktop_model *m, desktop_rect *out) {
    if (!m->damaged || !out) return 0;
    *out = m->damage;
    m->damaged = 0;
    return 1;
}

void desktop_init(desktop_model *m, uint32_t width, uint32_t height) {
    *m = (desktop_model){0};
    m->width = width < 320 ? 320 : width > 8192 ? 8192 : width;
    m->height = height < 260 ? 260 : height > 8192 ? 8192 : height;
    m->focused = DESKTOP_WINDOWS;
    desktop_damage(m, (desktop_rect){0, 0, (int32_t)m->width, (int32_t)m->height});
}

uint64_t desktop_token(const desktop_model *m, unsigned slot) {
    if (slot >= DESKTOP_WINDOWS || !m->windows[slot].live) return 0;
    return ((uint64_t)m->windows[slot].generation << 32) | (slot + 1u);
}

desktop_window *desktop_lookup(desktop_model *m, uint64_t token) {
    uint32_t low = (uint32_t)token;
    if (!low || low > DESKTOP_WINDOWS) return 0;
    desktop_window *w = &m->windows[low - 1u];
    return w->live && w->generation == token >> 32 ? w : 0;
}

static desktop_rect constrained(desktop_model *m, desktop_rect r) {
    r.width = minimum(maximum(r.width, DESKTOP_MIN_WIDTH), (int32_t)m->width);
    r.height = minimum(maximum(r.height, DESKTOP_MIN_HEIGHT), (int32_t)m->height - 60);
    r.x = minimum(maximum(r.x, 0), (int32_t)m->width - r.width);
    r.y = minimum(maximum(r.y, 0), (int32_t)m->height - 60 - r.height);
    return r;
}

int desktop_focus(desktop_model *m, uint64_t token) {
    desktop_window *w = desktop_lookup(m, token);
    if (!w) return 0;
    unsigned slot = (unsigned)(w - m->windows), position = 0;
    while (position < m->count && m->order[position] != slot) ++position;
    if (position == m->count) return 0;
    if (m->focused < DESKTOP_WINDOWS) desktop_damage(m, m->windows[m->focused].bounds);
    for (unsigned i = position; i + 1 < m->count; ++i) m->order[i] = m->order[i + 1];
    m->order[m->count - 1] = (uint8_t)slot;
    m->focused = slot;
    w->minimized = 0;
    desktop_damage(m, w->bounds);
    ++m->changes;
    return 1;
}

uint64_t desktop_open(desktop_model *m, uint32_t app, desktop_rect r) {
    for (unsigned i = 0; i < DESKTOP_WINDOWS; ++i) {
        desktop_window *w = &m->windows[i];
        if (w->live || w->generation == UINT32_MAX) continue;
        ++w->generation;
        w->app = app;
        w->bounds = constrained(m, r);
        w->live = 1;
        w->minimized = 0;
        m->order[m->count++] = (uint8_t)i;
        uint64_t token = desktop_token(m, i);
        (void)desktop_focus(m, token);
        return token;
    }
    return 0;
}

int desktop_close(desktop_model *m, uint64_t token) {
    desktop_window *w = desktop_lookup(m, token);
    if (!w) return 0;
    unsigned slot = (unsigned)(w - m->windows), position = 0;
    desktop_damage(m, w->bounds);
    while (position < m->count && m->order[position] != slot) ++position;
    for (unsigned i = position; i + 1 < m->count; ++i) m->order[i] = m->order[i + 1];
    --m->count;
    w->live = 0;
    m->focused = DESKTOP_WINDOWS;
    for (unsigned i = m->count; i; --i) {
        unsigned candidate = m->order[i - 1];
        if (!m->windows[candidate].minimized) { m->focused = candidate; break; }
    }
    if (m->focused < DESKTOP_WINDOWS) desktop_damage(m, m->windows[m->focused].bounds);
    ++m->changes;
    return 1;
}

int desktop_place(desktop_model *m, uint64_t token, desktop_rect r) {
    desktop_window *w = desktop_lookup(m, token);
    if (!w) return 0;
    desktop_damage(m, w->bounds);
    w->bounds = constrained(m, r);
    desktop_damage(m, w->bounds);
    ++m->changes;
    return 1;
}

int desktop_minimize(desktop_model *m, uint64_t token, int minimized) {
    desktop_window *w = desktop_lookup(m, token);
    if (!w) return 0;
    if (!minimized) return desktop_focus(m, token);
    w->minimized = 1;
    desktop_damage(m, w->bounds);
    m->focused = DESKTOP_WINDOWS;
    for (unsigned i = m->count; i; --i) {
        unsigned slot = m->order[i - 1];
        if (!m->windows[slot].minimized) { m->focused = slot; break; }
    }
    if (m->focused < DESKTOP_WINDOWS) desktop_damage(m, m->windows[m->focused].bounds);
    ++m->changes;
    return 1;
}

uint64_t desktop_hit(const desktop_model *m, int32_t x, int32_t y) {
    for (unsigned i = m->count; i; --i) {
        unsigned slot = m->order[i - 1];
        const desktop_window *w = &m->windows[slot];
        desktop_rect r = w->bounds;
        if (!w->minimized && x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height)
            return desktop_token(m, slot);
    }
    return 0;
}
