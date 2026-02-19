#pragma once

#include "windows.h"

#define KB(n) ((n) * 1024)
#define MB(n) (KB(n) * 1024)
#define GB(n) (MB(n) * 1024)

#define export __declspec(dllexport)
#define import __declspec(dllimport)

typedef struct Data Data;
#ifndef ENGINE_IMPL
import extern Data *data;
#endif

#define rgb(r, g, b) ((col32)(((r) << 16) | ((g) << 8) | (b)))
#define rgba(r, g, b, a) ((col32)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

typedef char          i8;
typedef unsigned char u8;
typedef u8 bool;
#define false 0
#define true 1

typedef short          i16;
typedef unsigned short u16;

typedef int          i32;
typedef unsigned int u32;
typedef u32          col32;

typedef long          i64;
typedef unsigned long u64;

// 1.23.8 fixed point
typedef i32 q8;
#define Q8(i32_val) ((q8)((i32_val) << 8))

// q8  q8_from_f32(f32 val) { return val * 256.0f; }
// f32 q8_to_f32(q8 val) { return (f32)val / 256.0f; }
q8  q8_from_i32(i32 val) { return val << 8; }
i32 q8_to_i32(q8 val) { return val >> 8; }

q8 q8_floor(q8 val) { return val & ~0xFF; }
q8 q8_ceil(q8 val) { return (val + 0xFF) & ~0xFF; }
q8 q8_round(q8 val) { return (val + 0x80) & ~0xFF; }
q8 q8_frac(q8 val) { return val & 0xFF; }

q8 q8_mul(q8 a, q8 b) { return (q8)((a * b) >> 8); }
q8 q8_div(q8 a, q8 b) { return (q8)((a << 8) / b); }

// q8 q8_add_f32(q8 a, f32 b) { return a + q8_from_f32(b); }
// q8 q8_sub_f32(q8 a, f32 b) { return a - q8_from_f32(b); }

typedef float f32;
typedef f32   rad;
typedef f32   deg;

typedef double f64;

typedef struct {
    u8 *text;
    i32 len;
} string;

typedef union {
    struct {
        q8 x, y;
    };
    struct {
        q8 w, h;
    };
} v2;

typedef union {
    struct {
        i32 x, y;
    };
    struct {
        i32 w, h;
    };
} v2i;

typedef struct {
    void *data;
    i32   used, cap;
} Arena;

typedef struct {
    Arena perm;
    Arena temp;
} Context;

// Data

u8 *alloc(i32 size, Arena *a);
u8 *alloc_perm(i32 size);
u8 *alloc_temp(i32 size);
#define ALLOC(type) (type *)alloc_perm(sizeof(type))
#define ALLOC_ARRAY(type, count) (type *)alloc_perm(sizeof(type) * (count))

#define STR(str) (string){.text = str, .len = sizeof(str) - 1}

// Collision

typedef struct {
    q8 x, y, w, h;
} rect;

bool col_point_rect(v2 p, rect r) {
    return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

bool col_rect_rect(rect a, rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

rect col_rect_rect_area(rect a, rect b) {
    rect result = {0};

    if (!col_rect_rect(a, b)) return result;

    result.x = a.x > b.x ? a.x : b.x;
    result.y = a.y > b.y ? a.y : b.y;
    result.w = (a.x + a.w < b.x + b.w ? a.x + a.w : b.x + b.w) - result.x;
    result.h = (a.y + a.h < b.y + b.h ? a.y + a.h : b.y + b.h) - result.y;

    return result;
}

// Debug Drawing

void draw_text(char *text, i32 x, i32 y, col32 color);
void draw_rect(rect r, col32 color);
void draw_rect_outline(rect r, col32 color);
void draw_circle(i32 x, i32 y, i32 r, col32 color);
void draw_circle_outline(i32 x, i32 y, i32 r, col32 color);
void draw_line(i32 x1, i32 y1, i32 x2, i32 y2, col32 color);
void draw_arc(i32 x, i32 y, i32 r, rad from, rad to, col32 color);

// GUI

bool gui_button(i32 x, i32 y);
bool gui_toggle(i32 x, i32 y, bool *val);

// IO

string file_read(char *path);
i32    file_write(char *path, char *data);

void *image_read(char *path);

void log(const char *fmt, ...);

// Input

typedef enum {
    K_NONE = 0,
    K_UP,
    K_DOWN,
    K_LEFT,
    K_RIGHT,
    K_ENTER,
    K_ESCAPE,
    K_W,
    K_A,
    K_R,
    K_S,
    K_COUNT,
} Key;

typedef enum {
    KS_JUST_PRESSED = 0,
    KS_PRESSED,
    KS_RELEASED,
    KS_JUST_RELEASED,
} KeyState;