#pragma once

#include "windows.h"

#define rgb RGB

#define export __declspec(dllexport)
#define import __declspec(dllimport)

typedef char          i8;
typedef unsigned char u8;

typedef short          i16;
typedef unsigned short u16;

typedef int          i32;
typedef unsigned int u32;

typedef long          i64;
typedef unsigned long u64;

typedef char bool;
#define false 0
#define true 1

typedef float f32;
typedef f32   rad;
typedef f32   deg;

typedef struct {
    char *text;
    i32   len;
} string;

typedef union {
    struct {
        f32 x, y;
    };
    struct {
        f32 w, h;
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

typedef union {
    struct {
        u32 x, y;
    };
    struct {
        u32 w, h;
    };
} v2u;

typedef u32 col32;

typedef struct {
    void *data;
    i32   used, cap;
} Arena;

typedef struct {
    Arena perm;
    Arena temp;
} Context;

Context *ctx();

// Col

typedef struct {
    i32 x, y, w, h;
} rect;

bool col_point_rect(v2i p, rect r) {
    return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

bool col_rect_rect(rect a, rect b) {
    return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
}

// Graphics

void draw_text(char *text, i32 x, i32 y, col32 color);
void draw_rect(rect r, col32 color);
void draw_circle(i32 x, i32 y, i32 r, col32 color);
void draw_arc(i32 x, i32 y, i32 r, rad from, rad to, col32 color);

// GUI

bool gui_button(i32 x, i32 y);
bool gui_toggle(i32 x, i32 y, bool *val);

// IO

char *file_read(char *path);
i32   file_write(char *path, char *data);