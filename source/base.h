#pragma once

#include <libtcc/libtcc.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#define export __declspec(dllexport)
#define import __declspec(dllimport)

#define global static
#define persist static
#define internal static

typedef const char *cstr;

typedef int8_t  i8;
typedef uint8_t u8;
typedef u8 bool;
#define false (bool)0
#define true (bool)1

typedef int16_t  i16;
typedef uint16_t u16;

typedef int32_t  i32;
typedef uint32_t u32;
typedef u32      col32;
#define rgb(r, g, b) ((col32)(((r) << 16) | ((g) << 8) | (b)))
#define rgba(r, g, b, a) ((col32)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#define WHITE rgb(255, 255, 255)
#define BLACK rgb(0, 0, 0)

typedef int64_t  i64;
typedef uint64_t u64;

#define KB(n) ((n) * 1024)
#define MB(n) (KB(n) * 1024)
#define GB(n) (MB(n) * 1024)

// Forward declare for tcc >.<
int  printf(cstr fmt, ...);
int  vsnprintf(char *buf, u64 size, cstr fmt, char *args);
void abort();
void qsort(void *ptr, u64 count, u64 size, i32 (*comp)(const void *, const void *));

#define COL_RESET "\033[0m"
#define COL_INFO "\033[32m"          // green
#define COL_WARN "\033[33m"          // yellow
#define COL_ERROR "\033[31m"         // red
#define COL_FATAL "\033[41m\033[97m" // white on red

#define LIST_VAR "\n\t> "
#define LIST_VAR2 "\t> "

// TODO(violeta): hash and log repeats once!
#define INFO(msg, ...) \
    printf(COL_INFO "[INFO]" COL_RESET " [%s] " msg "\n", __func__, ##__VA_ARGS__)
#define WARN(msg, ...) \
    printf(COL_WARN "[WARN]" COL_RESET " [%s] " msg "\n", __func__, ##__VA_ARGS__)
#define ERR(msg, ...) \
    printf(COL_ERROR "[ERROR]" COL_RESET " [%s] " msg "\n", __func__, ##__VA_ARGS__)
#define FATAL(msg, ...)                                                                \
    do {                                                                               \
        printf(COL_FATAL "[FATAL]" COL_RESET " [%s:%d] " msg "\n", __func__, __LINE__, \
               ##__VA_ARGS__);                                                         \
        abort();                                                                       \
    } while (0);

#define assert(expr)                              \
    do {                                          \
        if (!(expr)) {                            \
            FATAL("Assertion failed: %s", #expr); \
        }                                         \
    } while (0);

typedef float  f32;
typedef double f64;
typedef f32    rad;
typedef f32    deg;

// 1.23.8 fixed point
typedef i32 q8;
#define Q8(i32_val) ((q8)((i32_val) << 8))

#define Q8_PI (q8)(804)
#define Q8_TAU (q8)(1608)

q8  q8_from_f32(f32 val) { return val * 256.0f; }
f32 q8_to_f32(q8 val) { return (f32)val / 256.0f; }
q8  q8_from_i32(i32 val) { return val << 8; }
i32 q8_to_i32(q8 val) { return val >> 8; }

q8 q8_floor(q8 val) { return val & ~0xFF; }
q8 q8_ceil(q8 val) { return (val + 0xFF) & ~0xFF; }
q8 q8_round(q8 val) { return (val + 0x80) & ~0xFF; }
q8 q8_frac(q8 val) { return val & 0xFF; }

q8 q8_mul(q8 a, q8 b) { return (q8)(((i64)a * (i64)b) >> 8); }
q8 q8_div(q8 a, q8 b) { return (q8)(((i64)a << 8) / b); }

// 1.25.6 fixed point
typedef i32 q6;
#define Q6(i32_val) ((q6)((i32_val) << 6))

#define Q6_PI (q6)(1608 / 4)
#define Q6_TAU (q6)(1608 / 2)

q6  q6_from_f32(f32 val) { return val * 64.0f; }
f32 q6_to_f32(q6 val) { return (f32)val / 64.0f; }
q6  q6_from_i32(i32 val) { return val << 6; }
i32 q6_to_i32(q6 val) { return val >> 6; }

q6 q6_floor(q6 val) { return val & ~0x3F; }
q6 q6_ceil(q6 val) { return (val + 0x3F) & ~0x3F; }
q6 q6_round(q6 val) { return (val + 0x20) & ~0x3F; }
q6 q6_frac(q6 val) { return val & 0x3F; }

q6 q6_mul(q6 a, q6 b) { return (q6)(((i64)a * (i64)b) >> 6); }
q6 q6_div(q6 a, q6 b) { return (q6)(((i64)a << 6) / b); }

#ifndef Q_FRAC
#define Q_FRAC 8
#endif
#if Q_FRAC == 8
typedef q8 q32;
#elif Q_FRAC == 6
typedef q6 q32;
#endif

#define Q32(i32_val) ((q32)((i32_val) << Q_FRAC))
#define q32_to_i32(q32_val) ((i32)(q32_val) >> Q_FRAC)
#define q32_from_f32(f32_val) ((q32)((f32_val) * (1 << Q_FRAC)))
#define q32_to_f32(q32_val) ((f32)(q32_val) / (1 << Q_FRAC))
#define q32_from_i32(i32_val) ((q32)(i32_val) << Q_FRAC)

#define q32_floor(q32_val) ((q32_val) & ~((1 << Q_FRAC) - 1))
#define q32_ceil(q32_val) (((q32_val) + ((1 << Q_FRAC) - 1)) & ~((1 << Q_FRAC) - 1))
#define q32_round(q32_val) (((q32_val) + (1 << (Q_FRAC - 1))) & ~((1 << Q_FRAC) - 1))
#define q32_frac(q32_val) ((q32_val) & ((1 << Q_FRAC) - 1))

#define q32_mul(a, b) ((q32)(((i64)(a) * (i64)(b)) >> Q_FRAC))
#define q32_div(a, b) ((q32)(((i64)(a) << Q_FRAC) / (b)))

#define Q32_PI (q32_from_f32(3.14159265358979f))
#define Q32_TAU (q32_from_f32(6.28318530717958f))

typedef union {
    struct {
        q32 x, y;
    };
    struct {
        q32 w, h;
    };
    struct {
        q32 u, v;
    };
} v2;

typedef v2 uv;

typedef union {
    struct {
        i32 x, y;
    };
    struct {
        i32 w, h;
    };
    struct {
        i32 from, to;
    };
} v2i;

inline v2i v2i_from_v2(v2 v) {
    return (v2i){
        .x = q32_to_i32(v.x),
        .y = q32_to_i32(v.y),
    };
}

inline v2i v2i_add(v2i a, v2i b) {
    return (v2i){
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

inline i32 v2i_cross(v2i a, v2i b) { return a.y * b.x - a.x * b.y; }

typedef union {
    q32 val[3];
    struct {
        q32 x, y, z;
    };
    struct {
        q32 r, g, b;
    };
} v3;

// Normalized vector
typedef v2 v2n;
// Normalized vector
typedef v3 v3n;

typedef union {
    struct {
        i32 x, y, z;
    };
    struct {
        i32 a, b, c;
    };
} v3i;

inline v3 v3_add(v3 a, v3 b) {
    return (v3){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

inline v3 v3_sub(v3 a, v3 b) {
    return (v3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

inline v3 v3_mul(v3 a, v3 b) {
    return (v3){
        .x = q32_mul(a.x, b.x),
        .y = q32_mul(a.y, b.y),
        .z = q32_mul(a.z, b.z),
    };
}

inline q32 v3_dot(v3 a, v3 b) { return q32_mul(a.x, b.x) + q32_mul(a.y, b.y) + q32_mul(a.z, b.z); }

inline v3 v3_cross(v3 a, v3 b) {
    return (v3){
        .x = q32_mul(a.y, b.z) - q32_mul(a.z, b.y),
        .y = q32_mul(a.z, b.x) - q32_mul(a.x, b.z),
        .z = q32_mul(a.x, b.y) - q32_mul(a.y, b.x),
    };
}

v2 v3_project(v3 v) {
    q32 z = v.z > 1 ? v.z : 1;
    return (v2){q32_div(v.x, z), q32_div(v.y, z)};
}

#if Q_FRAC == 8
// 64-entry sine table for one quadrant [0, PI/2], in q8 format.
// sin_table[i] = sin(i * (PI/2) / 64) * 256
static const i16 sin_table[65] = {
    0,   6,   13,  19,  25,  31,  38,  44,  50,  56,  62,  68,  74,  80,  86,  92,  98,
    103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 181, 185,
    189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234, 236, 238, 241,
    243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 256, 256,
};
#elif Q_FRAC == 6
// 64-entry sine table for one quadrant [0, PI/2], in q6 format.
// sin_table[i] = sin(i * (PI/2) / 64) * 64
static const i16 sin_table[65] = {
    0,   3,   6,   9,   12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  51,
    54,  57,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99,  102, 105,
    108, 111, 114, 117, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 153, 156, 159,
    162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 192, 195, 198, 201, 204,
};
#endif

q32 q32_sin(q32 angle) {
    // Normalize to [0, TAU)
    while (angle < 0)
        angle += Q32_TAU;
    while (angle >= Q32_TAU)
        angle -= Q32_TAU;

    // Quarter: 0=[0,PI/2), 1=[PI/2,PI), 2=[PI,3PI/2), 3=[3PI/2,TAU)
    q32 quarter_period = Q32_TAU / 4; // 402
    i32 quadrant       = angle / quarter_period;
    q32 remainder      = angle % quarter_period;

    // Map remainder to table index [0, 64]
    i32 idx = (i32)remainder * 64 / quarter_period;
    if (idx > 64) idx = 64;

    q32 val;
    switch (quadrant) {
    case 0: val = sin_table[idx]; break;
    case 1: val = sin_table[64 - idx]; break;
    case 2: val = -sin_table[idx]; break;
    case 3: val = -sin_table[64 - idx]; break;
    default: val = 0; break;
    }
    return val;
}

q32 q32_cos(q32 angle) { return q32_sin(angle + Q32_TAU / 4); }

v3 v3_rotate_xz(v3 v, q32 angle) {
    q32 cos_a = q32_cos(angle);
    q32 sin_a = q32_sin(angle);

    return (v3){
        .x = q32_mul(v.x, cos_a) - q32_mul(v.z, sin_a),
        .y = v.y,
        .z = q32_mul(v.x, sin_a) + q32_mul(v.z, cos_a),
    };
}

v2 v2_screen(v2 v, v2i screen) {
    // Correct for aspect ratio: use height for both axes to maintain square pixels,
    // then center horizontally.
    q32 half_w = Q32(screen.w) >> 1;
    q32 half_h = Q32(screen.h) >> 1;
    return (v2){
        .x = q32_mul(v.x, half_h) + half_w,
        .y = q32_mul(v.y, half_h) + half_h,
    };
}

// Data

u8  *os_alloc(i32 size);
void os_free(u8 *ptr);

typedef struct {
    void *data;
    i32   used, cap;
} Arena;

Arena arena_new(i32 cap, Arena *parent);

u8 *alloc(i32 size, Arena *a);

i32  arena_mark(Arena *a) { return a->used; }
void arena_reset(Arena *a, i32 mark) { a->used = a->used >= mark ? mark : a->used; }
u8  *alloc_perm(i32 size);
u8  *alloc_temp(i32 size);
#define ALLOC(type) (type *)alloc_perm(sizeof(type))
#define ALLOC_ARRAY(type, count) (type *)alloc_perm(sizeof(type) * (count))
#define ALLOC_TEMP(type) (type *)alloc_temp(sizeof(type))
#define ALLOC_TEMP_ARRAY(type, count) (type *)alloc_temp(sizeof(type) * (count))

typedef struct EngineData EngineData;
typedef struct Info       Info;

typedef struct {
    Arena perm;
    Arena temp;
} Context;

Context *ctx();
#define PERM (&ctx()->perm)
#define TEMP (&ctx()->temp)
EngineData *EG();

typedef struct {
    u8 *text;
    i32 len;
} string;

char *string_format(Arena *a, char *fmt, ...);
#define STR(str) (string){.text = (u8 *)str, .len = sizeof(str) - 1}

bool gui_button(char *name, q32 x, q32 y);
bool gui_toggle(char *name, q32 x, q32 y, bool *val);

// 3D

typedef struct {
    i32 a, b, c;       // vertex indices
    v2  uva, uvb, uvc; // [0, Q32(1)]
} Face;

typedef struct {
    v3   *verts;
    i32   verts_count;
    Face *faces;
    i32   faces_count;
} Mesh;

Mesh mesh_from_obj(string obj, Arena *a) {
    i32  vert_count = 0, uv_count = 0, face_count = 0;
    cstr p = (cstr)obj.text;
    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (p[0] == 'v' && p[1] == 't' && p[2] == ' ')
            uv_count++;
        else if (p[0] == 'v' && p[1] == ' ')
            vert_count++;
        else if (p[0] == 'f' && p[1] == ' ')
            face_count++;
        while (*p && *p != '\n' && *p != '\r')
            p++;
        if (*p == '\r') p++;
        if (*p == '\n') p++;
    }

    Mesh result = {
        .verts       = (v3 *)alloc(sizeof(v3) * vert_count, a),
        .verts_count = vert_count,
        .faces       = (Face *)alloc(sizeof(Face) * face_count, a),
        .faces_count = face_count,
    };

    v2 *uvs = uv_count > 0 ? (v2 *)os_alloc(sizeof(v2) * uv_count) : 0;

    i32   vi  = 0;
    i32   uvi = 0;
    i32   fi  = 0;
    char *end = '\0';

    p = (cstr)obj.text;
    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;

        if (p[0] == 'v' && p[1] == 't' && p[2] == ' ') {
            p += 3;
            f32 u      = strtod(p, &end);
            p          = end;
            f32 v      = strtod(p, &end);
            p          = end;
            uvs[uvi++] = (v2){q32_from_f32(u), q32_from_f32(v)};
        } else if (p[0] == 'v' && p[1] == ' ') {
            p += 2;
            f32 x              = strtod(p, &end);
            p                  = end;
            f32 y              = strtod(p, &end);
            p                  = end;
            f32 z              = strtod(p, &end);
            p                  = end;
            result.verts[vi++] = (v3){q32_from_f32(x), q32_from_f32(y), q32_from_f32(z)};
        } else if (p[0] == 'f' && p[1] == ' ') {
            p += 2;
            i32 idx[3]    = {0};
            i32 uv_idx[3] = {-1, -1, -1};

            for (i32 i = 0; i < 3; i++) {
                while (*p == ' ')
                    p++;
                idx[i] = (i32)strtol(p, &end, 10) - 1;
                p      = end;
                if (*p == '/') {
                    p++;
                    if (*p != '/' && *p != ' ' && *p != '\n' && *p != '\r' && *p) {
                        uv_idx[i] = (i32)strtol(p, &end, 10) - 1;
                        p         = end;
                    }
                    if (*p == '/') {
                        p++;
                        strtol(p, &end, 10);
                        p = end;
                    }
                }
            }

            result.faces[fi].a   = idx[0];
            result.faces[fi].b   = idx[1];
            result.faces[fi].c   = idx[2];
            result.faces[fi].uva = (uv_idx[0] >= 0 && uvs) ? uvs[uv_idx[0]] : (v2){0, 0};
            result.faces[fi].uvb = (uv_idx[1] >= 0 && uvs) ? uvs[uv_idx[1]] : (v2){0, 0};
            result.faces[fi].uvc = (uv_idx[2] >= 0 && uvs) ? uvs[uv_idx[2]] : (v2){0, 0};
            fi++;
        }

        while (*p && *p != '\n' && *p != '\r')
            p++;
        if (*p == '\r') p++;
        if (*p == '\n') p++;
    }

    os_free((u8 *)uvs);
    return result;
}

typedef union {
    q32 val[3][3];

    struct { // 3d transform
        v3 pos, rot, scale;
    };
} m3;

typedef q32 m4[4][4];

const global m3 m3_id = {.val = {{Q32(1), 0, 0}, {0, Q32(1), 0}, {0, 0, Q32(1)}}};
const global m4 m4_id = {
    {Q32(1), 0, 0, 0}, {0, Q32(1), 0, 0}, {0, 0, Q32(1), 0}, {0, 0, 0, Q32(1)}};

// Collision

typedef union rect {
    struct {
        q32 x, y, w, h;
    };
    struct {
        v2 pos, size;
    };
} rect;

typedef union i32rect {
    struct {
        i32 x, y, w, h;
    };
    struct {
        v2i pos, size;
    };
} i32rect;

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

// Debug

void draw_text(char *text, i32 x, i32 y, col32 color);

typedef struct Texture {
    col32 *data;
    v2i    size;
} Texture;

typedef struct DrawTextureParams {
    v2i     pos;      // screen position
    i32rect src;      // [0, texture size]
    i32rect dst;      // [0, screen size]
    q32     rotation; // [0, TAU)
    col32   tint;
} DrawTextureParams;

void draw_texture_pro(Texture tex, DrawTextureParams params);
#define draw_texture(tex, ...) draw_texture_pro((tex), (DrawTextureParams){0, __VA_ARGS__})
void draw_rect(rect r, col32 color);
void draw_rect_outline(rect r, col32 color);

// IO

string file_read(char *path, Arena *a);
bool   file_write(char *path, string data);

typedef void *Image;
Image         image_read(char *path);

// Input

// Physical key codes.
// Max value must be under `2^MOD_BITS_USED` to fit in KeyCombo.
typedef enum Key {
    K_NONE = 0,
    K_CONTROL,
    K_SHIFT,
    K_ALT,
    K_UP,
    K_DOWN,
    K_LEFT,
    K_RIGHT,
    K_ENTER,
    K_ESCAPE,
    K_Q,
    K_W,
    K_F,
    K_P,
    K_G,
    K_J,
    K_L,
    K_U,
    K_Y,
    K_A,
    K_R,
    K_S,
    K_T,
    K_D,
    K_H,
    K_N,
    K_E,
    K_I,
    K_O,
    K_Z,
    K_X,
    K_C,
    K_V,
    K_B,
    K_K,
    K_M,
    K_F1,
    K_F2,
    K_F3,
    K_F4,
    K_F5,
    K_F6,
    K_F7,
    K_F8,
    K_F9,
    K_F10,
    K_F11,
    K_F12,
    K_SEMICOLON,
    K_COLON,
    K_COMMA,
    K_PERIOD,
    K_BRACKET_OPEN,
    K_BRACKET_CLOSE,
    K_BRACE_OPEN,
    K_BRACE_CLOSE,
    K_SLASH_FORWARD,
    K_QUOTE_SINGLE,
    K_QUOTE_DOUBLE,
    K_MOUSE_LEFT,
    K_MOUSE_RIGHT,
    K_MOUSE_MID,
    K_COUNT,
} Key;

typedef enum { M_CONTROL = 1 << 31, M_SHIFT = 1 << 30, M_ALT = 1 << 29, M_COUNT = 3 } ModKey;

// Number of bits used for modifier keys.
#define MOD_BITS_USED (32 - M_COUNT)

// `Key | ModKey`
// Upper `M_COUNT` bits are modifiers, lower ones are for keys.
typedef i32 KeyCombo;

typedef enum Action {
    A_NONE = 0,
    A_FULLSCREEN,
    A_QUIT,
    A_RESET,
    A_UP,
    A_DOWN,
    A_LEFT,
    A_RIGHT,
    A_ACCEPT,
    A_CANCEL,
    A_COUNT,
} Action;

typedef enum {
    KS_RELEASED = 0,
    KS_JUST_RELEASED,
    KS_JUST_PRESSED,
    KS_PRESSED,
} KeyState;

KeyState GetAction(Action k);

typedef struct {
    v3  pos;
    v3n look_at; // TODO(violeta)
} Camera;

Camera *cam();

typedef struct Metrics    Metrics;
typedef struct SystemInfo SystemInfo;

typedef enum {
    DCT_RECT,
    DCT_RECT_OUTLINE,
    DCT_TEXT,
    DCT_LINE,
    DCT_MESH_WIREFRAME,
    DCT_MESH_SOLID,
    DCT_MODEL,
    DCT_TEXTURE_2D,
    DCT_COUNT
} DrawCmdType;

typedef struct DrawCmd {
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

        struct { // 2d texture
            Texture          *texture;
            DrawTextureParams params;
        };

        struct { // model
            col32 *tex;
            v2i    tex_size;
            v3    *vertices;
            i32    count;
            Face  *faces;
            i32    faces_count;
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

struct Info {
    cstr     name;
    cstr     version;
    KeyCombo keybinds[A_COUNT][2];
};

struct Metrics {
    bool  initialized;
    void *processHandle;
};

typedef struct {
    TCCState *tcc;

    void (*init)();
    Info *info;
    void (*update)(q32 dt);
    void (*quit)();
    i32 (*gamedata_size)();
    FILETIME last_write;

} GameDLL;

struct SystemInfo {
    // System
    cstr processorArchitecture;
    u32  numberOfProcessors;
    u32  pageSize;
    u32  allocationGranularity;
    f64  cpuFreq;

    // Memory
    u64 totalPhys;
    u64 availPhys;
    u64 totalVirtual;
    u64 availVirtual;

    // OS
    u32 majorVersion;
    u32 minorVersion;
    u32 buildNumber;
    u32 platformId;

    // GPU
    cstr gpuName;
    cstr gpuVendor;
    cstr glVersion;
};

#include "profiler.h"

struct EngineData {
    Context ctx;
    GameDLL game;
    u8     *game_memory;

    Camera          cam;
    bool            shutdown;
    f32             dt;
    f32             target_dt;
    f64             next_frame;
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
    LoopProfiler    loop_profiler;
    Texture         default_texture;
};

#undef EXPORT
#define EXPORT extern "C" __declspec(dllexport)

typedef struct {
    u32 cb;
    u32 PageFaultCount;
    u64 PeakWorkingSetSize;
    u64 WorkingSetSize;
    u64 QuotaPeakPagedPoolUsage;
    u64 QuotaPagedPoolUsage;
    u64 QuotaPeakNonPagedPoolUsage;
    u64 QuotaNonPagedPoolUsage;
    u64 PagefileUsage;
    u64 PeakPagefileUsage;
    u64 PrivateUsage;
} MY_PROCESS_MEMORY_COUNTERS_EX;

import BOOL __stdcall K32GetProcessMemoryInfo(HANDLE, MY_PROCESS_MEMORY_COUNTERS_EX *, u32);
import BOOL __stdcall GlobalMemoryStatusEx(MEMORYSTATUSEX *);

internal f64        now_seconds();
internal Metrics    metrics_init();
internal u64        get_page_fault_count(Metrics);
internal u64        estimate_cpu_freq();
internal u64        read_cpu_timer();
internal inline i64 read_acquire(volatile i64 *src);

internal SystemInfo systeminfo_init();
internal void       systeminfo_print(SystemInfo info);

internal bool get_last_write_time(const char *filename, FILETIME *outTime);
internal void fullscreen_window(HWND hWnd);
internal void center_window(HWND hWnd);

internal void render_line(v2i from, v2i to, col32 color);
internal void render_filled_triangle(v2i p0, v2i p1, v2i p2, col32 color);
internal void render_textured_triangle(v2i p0, v2i p1, v2i p2, uv t0, uv t1, uv t2, v3 z,
                                       col32 *tex, v2i tex_size);