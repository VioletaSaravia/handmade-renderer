#pragma once
#include <stdint.h>

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
    printf(COL_INFO "[INFO]" COL_RESET "  [%s] " msg "\n", __func__, ##__VA_ARGS__)
#define WARN(msg, ...) \
    printf(COL_WARN "[WARN]" COL_RESET "  [%s] " msg "\n", __func__, ##__VA_ARGS__)
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

typedef float f32;
typedef f32   rad;
typedef f32   deg;

typedef double f64;

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

q8 q8_mul(q8 a, q8 b) { return (q8)((a * b) >> 8); }
q8 q8_div(q8 a, q8 b) { return (q8)((a << 8) / b); }

q8 q8_mul64(q8 a, q8 b) { return (q8)(((i64)a * (i64)b) >> 8); }
q8 q8_div64(q8 a, q8 b) { return (q8)(((i64)a << 8) / b); }

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

q6 q6_mul(q6 a, q6 b) { return (q6)((a * b) >> 6); }
q6 q6_div(q6 a, q6 b) { return (q6)((a << 6) / b); }

q6 q6_mul64(q6 a, q6 b) { return (q6)(((i64)a * (i64)b) >> 6); }
q6 q6_div64(q6 a, q6 b) { return (q6)(((i64)a << 6) / b); }

#ifndef Q_FRAC
#define Q_FRAC 8
#endif
#if Q_FRAC == 8
typedef q8 q32;
#elif Q_FRAC == 6
typedef q6 q32;
#endif

#define Q32(i32_val) ((q32)((i32_val) << Q_FRAC))
#define Q32_FROM_F32(f32_val) ((q32)((f32_val) * (1 << Q_FRAC)))
#define Q32_TO_F32(q32_val) ((f32)(q32_val) / (1 << Q_FRAC))
#define Q32_FROM_I32(i32_val) ((q32)(i32_val) << Q_FRAC)
#define Q32_TO_I32(q32_val) ((i32)(q32_val) >> Q_FRAC)

#define Q32_FLOOR(q32_val) ((q32_val) & ~((1 << Q_FRAC) - 1))
#define Q32_CEIL(q32_val) (((q32_val) + ((1 << Q_FRAC) - 1)) & ~((1 << Q_FRAC) - 1))
#define Q32_ROUND(q32_val) (((q32_val) + (1 << (Q_FRAC - 1))) & ~((1 << Q_FRAC) - 1))
#define Q32_FRAC(q32_val) ((q32_val) & ((1 << Q_FRAC) - 1))
#define Q32_MUL(a, b) ((q32)(((i64)(a) * (i64)(b)) >> Q_FRAC))
#define Q32_DIV(a, b) ((q32)(((i64)(a) << Q_FRAC) / (b)))
#define Q32_MUL64(a, b) ((q32)(((i64)(a) * (i64)(b)) >> Q_FRAC))
#define Q32_DIV64(a, b) ((q32)(((i64)(a) << Q_FRAC) / (b)))

#define Q32_PI (Q32_FROM_F32(3.14159265358979f))
#define Q32_TAU (Q32_FROM_F32(6.28318530717958f))

typedef union {
    struct {
        q8 x, y;
    };
    struct {
        q8 w, h;
    };
    struct {
        q8 u, v;
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

v2i v2i_from_v2(v2 v) {
    return (v2i){
        .x = q8_to_i32(v.x),
        .y = q8_to_i32(v.y),
    };
}

i32 v2i_cross(v2i a, v2i b) { return a.y * b.x - a.x * b.y; }

typedef struct {
    u32 *data;
    v2i  size;
} Texture;

global col32 default_texture_data[64][64];

global Texture default_texture = {
    .data = (u32 *)default_texture_data,
    .size = {.x = 64, .y = 64},
};

void init_default_texture() {
    for (i32 y = 0; y < 64; y++) {
        for (i32 x = 0; x < 64; x++) {
            u32 checker = ((x / 8) % 2) ^ ((y / 8) % 2);
            default_texture_data[y][x] =
                rgb(checker ? 200 : 50, checker ? 200 : 50, checker ? 200 : 50);
        }
    }
}

typedef union {
    q8 val[3];
    struct {
        q8 x, y, z;
    };
    struct {
        q8 r, g, b;
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
        .x = q8_mul64(a.x, b.x),
        .y = q8_mul64(a.y, b.y),
        .z = q8_mul64(a.z, b.z),
    };
}

inline q8 v3_dot(v3 a, v3 b) {
    return q8_mul64(a.x, b.x) + q8_mul64(a.y, b.y) + q8_mul64(a.z, b.z);
}

inline v3 v3_cross(v3 a, v3 b) {
    return (v3){
        .x = q8_mul64(a.y, b.z) - q8_mul64(a.z, b.y),
        .y = q8_mul64(a.z, b.x) - q8_mul64(a.x, b.z),
        .z = q8_mul64(a.x, b.y) - q8_mul64(a.y, b.x),
    };
}

v2 v3_project(v3 v) {
    // Prevent division by zero: clamp z to a small minimum
    q8 min_z = 1; // raw q8 value of 1/256, smallest positive
    q8 z     = v.z > min_z ? v.z : min_z;
    return (v2){q8_div64(v.x, z), q8_div64(v.y, z)};
}

// 64-entry sine table for one quadrant [0, PI/2], in q8 format.
// sin_table[i] = sin(i * (PI/2) / 64) * 256
static const i16 sin_table[65] = {
    0,   6,   13,  19,  25,  31,  38,  44,  50,  56,  62,  68,  74,  80,  86,  92,  98,
    103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 181, 185,
    189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234, 236, 238, 241,
    243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 256, 256,
};

q8 q8_sin(q8 angle) {
    // Normalize to [0, TAU)
    while (angle < 0)
        angle += Q8_TAU;
    while (angle >= Q8_TAU)
        angle -= Q8_TAU;

    // Quarter: 0=[0,PI/2), 1=[PI/2,PI), 2=[PI,3PI/2), 3=[3PI/2,TAU)
    q8  quarter_period = Q8_TAU / 4; // 402
    i32 quadrant       = angle / quarter_period;
    q8  remainder      = angle % quarter_period;

    // Map remainder to table index [0, 64]
    i32 idx = (i32)remainder * 64 / quarter_period;
    if (idx > 64) idx = 64;

    q8 val;
    switch (quadrant) {
    case 0: val = sin_table[idx]; break;
    case 1: val = sin_table[64 - idx]; break;
    case 2: val = -sin_table[idx]; break;
    case 3: val = -sin_table[64 - idx]; break;
    default: val = 0; break;
    }
    return val;
}

q8 q8_cos(q8 angle) { return q8_sin(angle + Q8_TAU / 4); }

v3 v3_rotate_xz(v3 v, q8 angle) {
    q8 cos_a = q8_cos(angle);
    q8 sin_a = q8_sin(angle);

    return (v3){
        .x = q8_mul(v.x, cos_a) - q8_mul(v.z, sin_a),
        .y = v.y,
        .z = q8_mul(v.x, sin_a) + q8_mul(v.z, cos_a),
    };
}

v2 v2_screen(v2 v, v2i screen) {
    // Correct for aspect ratio: use height for both axes to maintain square pixels,
    // then center horizontally.
    q8 half_w = Q8(screen.w) >> 1;
    q8 half_h = Q8(screen.h) >> 1;
    return (v2){
        .x = q8_mul(v.x, half_h) + half_w,
        .y = q8_mul(v.y, half_h) + half_h,
    };
}

// Data

typedef i32 handle;
u8         *os_alloc(i32 size);

typedef struct {
    void *data;
    i32   used, cap;
} Arena;

// TODO
typedef struct GUICtx GUICtx;
typedef struct Logger Logger;

typedef struct {
    Arena   perm;
    Arena   temp;
    GUICtx *gui;
    Logger *logger;
} Context;

typedef struct EngineData EngineData;

Context    *ctx();
EngineData *EG();

u8 *alloc(i32 size, Arena *a) {
    if (a->used + size > a->cap)
        FATAL("Arena out of memory! Used: %d, Requested: %d, Capacity: %d", a->used, size, a->cap);

    u8 *result = (u8 *)a->data + a->used;
    a->used += size;
    return result;
}

Arena arena_new(i32 cap, Arena *parent) {
    u8 *data = parent ? alloc(cap, parent) : os_alloc(cap);
    if (!data) return (Arena){0};

    return (Arena){
        .data = data,
        .used = 0,
        .cap  = cap,
    };
}

handle arena_mark(Arena *a) { return a->used; }
void   arena_reset(Arena *a, handle mark) { a->used = a->used >= mark ? mark : a->used; }
u8    *alloc_perm(i32 size) { return alloc(size, &ctx()->perm); }
u8    *alloc_temp(i32 size) { return alloc(size, &ctx()->temp); }
#define ALLOC(type) (type *)alloc_perm(sizeof(type))
#define ALLOC_ARRAY(type, count) (type *)alloc_perm(sizeof(type) * (count))
#define ALLOC_TEMP(type) (type *)alloc_temp(sizeof(type))
#define ALLOC_TEMP_ARRAY(type, count) (type *)alloc_temp(sizeof(type) * (count))

typedef struct {
    u8 *text;
    i32 len;
} string;

char *string_format(Arena *a, char *fmt, ...);
#define STR(str) (string){.text = str, .len = sizeof(str) - 1}

// 3D

typedef struct {
    i32 a, b, c;       // vertex indices
    v2  uva, uvb, uvc; // [0, Q8(1)]
} Face;

typedef struct {
    v3   *verts;
    i32   verts_count;
    Face *faces;
    i32   faces_count;
} Mesh;

// Forward declare C stdlib parsing functions
f32 strtof(cstr str, char **endptr) {
    f32 result  = 0.0f;
    f32 divisor = 1.0f;
    i32 sign    = 1;

    while (*str == ' ' || *str == '\t')
        str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10.0f + (*str - '0');
        str++;
    }

    if (*str == '.') {
        str++;
        while (*str >= '0' && *str <= '9') {
            divisor *= 10.0f;
            result += (*str - '0') / divisor;
            str++;
        }
    }

    if (endptr) *endptr = (char *)str;
    return sign * result;
}
i64 strtol(cstr, char **, i32);

Mesh mesh_from_obj(Arena *a, cstr obj) {
    // --- First pass: count vertices, texture coords, and faces ---
    i32  vert_count = 0, uv_count = 0, face_count = 0;
    cstr p = obj;
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

    // --- Allocate mesh data ---
    Mesh result = {
        .verts       = (v3 *)alloc(sizeof(v3) * vert_count, a),
        .verts_count = vert_count,
        .faces       = (Face *)alloc(sizeof(Face) * face_count, a),
        .faces_count = face_count,
    };

    // Temp storage for UVs (freed after parsing)
    handle temp_mark = arena_mark(&ctx()->temp);
    v2    *uvs       = uv_count > 0 ? ALLOC_TEMP_ARRAY(v2, uv_count) : 0;

    // --- Second pass: parse data ---
    i32   vi  = 0;
    i32   uvi = 0;
    i32   fi  = 0;
    char *end = '\0';

    p = obj;
    while (*p) {
        while (*p == ' ' || *p == '\t')
            p++;

        if (p[0] == 'v' && p[1] == 't' && p[2] == ' ') {
            p += 3;
            f32 u      = strtof(p, &end);
            p          = end;
            f32 v      = strtof(p, &end);
            p          = end;
            uvs[uvi++] = (v2){q8_from_f32(u), q8_from_f32(v)};
        } else if (p[0] == 'v' && p[1] == ' ') {
            p += 2;
            f32 x              = strtof(p, &end);
            p                  = end;
            f32 y              = strtof(p, &end);
            p                  = end;
            f32 z              = strtof(p, &end);
            p                  = end;
            result.verts[vi++] = (v3){q8_from_f32(x), q8_from_f32(y), q8_from_f32(z)};
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
                    } // skip normal index
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

    arena_reset(&ctx()->temp, temp_mark);
    return result;
}

typedef union {
    q8 val[3][3];

    struct { // 3d transform
        v3 pos, rot, scale;
    };
} m3;

typedef q8 m4[4][4];

const global m3 m3_id = {.val = {{Q8(1), 0, 0}, {0, Q8(1), 0}, {0, 0, Q8(1)}}};
const global m4 m4_id = {{Q8(1), 0, 0, 0}, {0, Q8(1), 0, 0}, {0, 0, Q8(1), 0}, {0, 0, 0, Q8(1)}};

// Collision

typedef struct {
    q8 x, y, w, h;
} rect;

typedef struct {
    i32 x, y, w, h;
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
void draw_rect(rect r, col32 color);
void draw_rect_outline(rect r, col32 color);

void render_line(v2i from, v2i to, col32 color);

// GUI

bool gui_button(char *name, q8 x, q8 y);
bool gui_toggle(char *name, q8 x, q8 y, bool *val);

// IO

string file_read(char *path, Arena *a);
i32    file_write(char *path, char *data);

void *image_read(char *path);

void log(const char *fmt, ...);

// Input

typedef enum {
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

#define MOD_BYTES_USED (32 - M_COUNT)

// Key | ModKey
typedef int KeyCombo;

typedef enum {
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