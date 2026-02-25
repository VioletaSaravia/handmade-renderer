#pragma once

#include <libtcc/libtcc.h>
#include "windows.h"

#include "base.h"
#include "profiler.h"
#include "engine.h"

typedef enum {
    DCT_RECT,
    DCT_RECT_OUTLINE,
    DCT_TEXT,
    DCT_LINE,
    DCT_MESH_WIREFRAME,
    DCT_MESH_SOLID,
    DCT_MODEL,
    DCT_COUNT
} DrawCmdType;

typedef struct {
    DrawCmdType t;

    union {
        struct { // text
            col32 color;
            char *text;
            i32   x, y;
        };

        struct { // rect
            col32 color;
            rect  r;
        };

        struct { // line
            col32 color;
            v2    from, to;
        };

        struct { // model
            v3    *vertices;
            i32    count;
            Face  *faces;
            i32    faces_count;
            col32 *tex;
            v2i    tex_size;
            m3     transform;
        };

        struct { // mesh
            col32 color;
            v3   *vertices;
            i32   count;
            v2i  *edges;
            i32   edges_count;
            m3    transform;
        };
    };
} DrawCmd;

typedef struct {
    TCCState *tcc;

    void (*init)();
    Info *info;
    void (*update)(q8 dt);
    void (*quit)();
    i32 (*gamedata_size)();
    FILETIME last_write;

} GameDLL;

struct EngineData {
    Context ctx;
    GameDLL game;
    u8     *game_memory;

    bool            shutdown;
    MSG             msg;
    LARGE_INTEGER   freq;
    WINDOWPLACEMENT prev_placement;
    v2              mouse_pos;
    KeyState        keys[K_COUNT];
    v2i             screen_size;
    u32            *screen_buf;
    DrawCmd        *draw_queue;
    u32             draw_size, draw_count;
    HWND            hwnd;
    Metrics         metrics;
    SystemInfo      system_info;
    Profiler        profiler;
};