#include <libtcc/libtcc.h>

#include "base.h"

typedef enum { DCT_RECT, DCT_TEXT, DCT_COUNT } DrawCmdType;

typedef struct {
    DrawCmdType t;
    col32       color;

    union {
        struct { // text
            char *text;
            i32   x, y;
        };

        struct { // rect
            rect r;
        };
    };
} DrawCmd;

typedef struct {
    u8       *game_memory;
    Context   ctx;
    TCCState *tcc;
    char      text_buf[256];

    void (*init)();
    void (*update)(q8 dt);
    void (*quit)();
    i32 (*gamedata_size)();
    i32 last_gamedata_size;

    LARGE_INTEGER   freq;
    FILETIME        last_write; // Never set/reset?
    WINDOWPLACEMENT prev_placement;
    v2              mouse_pos;
    KeyState        keys[K_COUNT];
    v2i             screen_size;
    u32            *screen_buf;
    DrawCmd        *draw_queue;
    u32             draw_size, draw_count;
} EngineData;

#ifdef ENGINE_IMPL
static EngineData *G;
#else
import extern EngineData *G;
#endif

void draw_rect(rect r, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t = DCT_RECT, .r = r, .color = color};
}

void draw_rect_outline(rect r, col32 color) {
    for (i32 x_coord = r.x; x_coord < r.x + r.w; x_coord++) {
        G->screen_buf[r.y * G->screen_size.w + x_coord]             = color;
        G->screen_buf[(r.y + r.h - 1) * G->screen_size.w + x_coord] = color;
    }
    for (i32 y_coord = r.y; y_coord < r.y + r.h; y_coord++) {
        G->screen_buf[y_coord * G->screen_size.w + r.x]             = color;
        G->screen_buf[y_coord * G->screen_size.w + (r.x + r.w - 1)] = color;
    }
}

void draw_circle(i32 x, i32 y, i32 r, col32 color) {
    for (i32 y_coord = y - r; y_coord <= y + r; y_coord++) {
        for (i32 x_coord = x - r; x_coord <= x + r; x_coord++) {
            i32 dx = x_coord - x;
            i32 dy = y_coord - y;
            if (dx * dx + dy * dy <= r * r) {
                if (x_coord >= 0 && x_coord < G->screen_size.w && y_coord >= 0 &&
                    y_coord < G->screen_size.h) {
                    G->screen_buf[y_coord * G->screen_size.w + x_coord] = color;
                }
            }
        }
    }
}

void draw_circle_outline(i32 x, i32 y, i32 r, col32 color) {
    for (i32 y_coord = y - r; y_coord <= y + r; y_coord++) {
        for (i32 x_coord = x - r; x_coord <= x + r; x_coord++) {
            i32 dx      = x_coord - x;
            i32 dy      = y_coord - y;
            i32 dist_sq = dx * dx + dy * dy;
            if (dist_sq >= (r - 1) * (r - 1) && dist_sq <= r * r) {
                if (x_coord >= 0 && x_coord < G->screen_size.w && y_coord >= 0 &&
                    y_coord < G->screen_size.h) {
                    G->screen_buf[y_coord * G->screen_size.w + x_coord] = color;
                }
            }
        }
    }
}

f32 atan2f(f32 y, f32 x) {
    const f32 PI   = 3.14159265358979f;
    const f32 PI_2 = 1.57079632679489f;

    if (x == 0.0f) {
        if (y > 0.0f) return PI_2;
        if (y < 0.0f) return -PI_2;
        return 0.0f;
    }

    f32 z     = y / x;
    f32 abs_z = z < 0 ? -z : z;

    // Polynomial approximation for atan in range [-1, 1]
    f32 angle = z / (1.0f + 0.28662f * abs_z * abs_z);

    if (x < 0.0f) {
        if (y >= 0.0f)
            angle += PI;
        else
            angle -= PI;
    }

    return angle;
}

void draw_arc(i32 x, i32 y, i32 r, rad from, rad to, col32 color) {
    for (i32 y_coord = y - r; y_coord <= y + r; y_coord++) {
        for (i32 x_coord = x - r; x_coord <= x + r; x_coord++) {
            i32 dx = x_coord - x;
            i32 dy = y_coord - y;
            if (dx * dx + dy * dy <= r * r) {
                f32 angle = atan2f((f32)dy, (f32)dx);
                if (angle >= from && angle <= to) {
                    if (x_coord >= 0 && x_coord < G->screen_size.w && y_coord >= 0 &&
                        y_coord < G->screen_size.h) {
                        G->screen_buf[y_coord * G->screen_size.w + x_coord] = color;
                    }
                }
            }
        }
    }
}

i32 abs(i32 x) { return x < 0 ? -x : x; }

void draw_line(i32 x1, i32 y1, i32 x2, i32 y2, col32 color) {
    i32 dx = x2 - x1;
    i32 dy = y2 - y1;

    i32 steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) return;

    f32 x_inc = (f32)dx / steps;
    f32 y_inc = (f32)dy / steps;

    f32 x = (f32)x1;
    f32 y = (f32)y1;

    for (i32 i = 0; i <= steps; i++) {
        if ((i16)x >= 0 && (i16)x < G->screen_size.w && (i16)y >= 0 && (i16)y < G->screen_size.h) {
            G->screen_buf[(i16)y * G->screen_size.w + (i16)x] = color;
        }
        x += x_inc;
        y += y_inc;
    }
}

void draw_text(char *text, i32 x, i32 y, col32 color) {
    if (G->draw_count == G->draw_size) return;

    G->draw_queue[G->draw_count++] =
        (DrawCmd){.t = DCT_TEXT, .color = color, .text = text, .x = x, .y = y};
}

void *image_read(char *path) { return LoadImage(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); }