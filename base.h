#pragma once

// Forward-declare C standard library functions to suppress implicit declaration warnings
int  printf(const char *fmt, ...);
int  vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args);
void abort(void);

#define assert(expr)                                                             \
    do {                                                                         \
        if (!(expr)) {                                                           \
            printf("Assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
            abort();                                                             \
        }                                                                        \
    } while (0)

#define COL_RESET "\033[0m"
#define COL_INFO "\033[32m"          // Green
#define COL_WARN "\033[33m"          // Yellow
#define COL_ERROR "\033[31m"         // Red
#define COL_FATAL "\033[41m\033[97m" // White on Red background

typedef const char *cstr;

#define LIST_VAR "\n\t> "

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

#define global static
#define persist static
#define internal static

typedef struct {
    char *name;
    char *version;
} Info;

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

#define WHITE rgb(255, 255, 255)
#define BLACK rgb(0, 0, 0)

typedef long          i64;
typedef unsigned long u64;

// 1.23.8 fixed point
typedef i32 q8;
#define Q8(i32_val) ((q8)((i32_val) << 8))

#define Q8_PI (q8)(804)
#define Q8_TAU (q8)(1608)

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

q8 q8_mul64(q8 a, q8 b) { return (q8)(((i64)a * (i64)b) >> 8); }
q8 q8_div64(q8 a, q8 b) { return (q8)(((i64)a << 8) / b); }

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
    struct {
        q8 u, v;
    };
} v2;

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

typedef union {
    struct {
        q8 x, y, z;
    };
    struct {
        q8 r, g, b;
    };
} v3;

inline v3 v3_add(v3 a, v3 b) {
    return (v3){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
}

inline v3 v3_mul(v3 a, v3 b) {
    return (v3){
        .x = q8_mul64(a.x, b.x),
        .y = q8_mul64(a.y, b.y),
        .z = q8_mul64(a.z, b.z),
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

typedef struct {
    void *data;
    i32   used, cap;
} Arena;

typedef struct {
    Arena perm;
    Arena temp;
} Context;

Context *ctx();

// Data

u8 *alloc(i32 size, Arena *a);
u8 *alloc_perm(i32 size);
u8 *alloc_temp(i32 size);
#define ALLOC(type) (type *)alloc_perm(sizeof(type))
#define ALLOC_ARRAY(type, count) (type *)alloc_perm(sizeof(type) * (count))

char *string_format(Arena *a, char *fmt, ...);

#define STR(str) (string){.text = str, .len = sizeof(str) - 1}

// 3D

typedef struct {
    v3  *verts;
    i32  verts_count;
    v2i *edges;
    i32  edges_count;
} Mesh;

typedef struct {
    v3 pos, rot, scale;
} Transform3D;

static v3 cube_mesh[8] = {
    {Q8(1) >> 1, Q8(-1) >> 1, Q8(1) >> 1},  {Q8(-1) >> 1, Q8(-1) >> 1, Q8(1) >> 1},
    {Q8(-1) >> 1, Q8(1) >> 1, Q8(1) >> 1},  {Q8(1) >> 1, Q8(1) >> 1, Q8(1) >> 1},
    {Q8(1) >> 1, Q8(-1) >> 1, Q8(-1) >> 1}, {Q8(-1) >> 1, Q8(-1) >> 1, Q8(-1) >> 1},
    {Q8(-1) >> 1, Q8(1) >> 1, Q8(-1) >> 1}, {Q8(1) >> 1, Q8(1) >> 1, Q8(-1) >> 1},
};

static v2i cube_edges[12] = {
    // Bottom face (z = -0.5)
    {4, 5},
    {5, 6},
    {6, 7},
    {7, 4},
    // Top face (z = 0.5)
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 0},
    // Vertical edges connecting top to bottom
    {0, 4},
    {1, 5},
    {2, 6},
    {3, 7},
};

static Mesh cube = {
    .verts       = cube_mesh,
    .verts_count = 8,
    .edges       = cube_edges,
    .edges_count = 12,
};

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

// Debug Drawing

void draw_text(char *text, i32 x, i32 y, col32 color);
void draw_rect(rect r, col32 color);
void draw_rect_outline(rect r, col32 color);

void render_line(v2i from, v2i to, col32 color);

// GUI

bool gui_button(char *name, q8 x, q8 y);
bool gui_toggle(char *name, q8 x, q8 y, bool *val);

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
    K_MOUSE_LEFT,
    K_MOUSE_RIGHT,
    K_MOUSE_MID,
    K_COUNT,
} Key;

typedef enum {
    KS_RELEASED = 0,
    KS_JUST_RELEASED,
    KS_JUST_PRESSED,
    KS_PRESSED,
} KeyState;

typedef struct {
    u64 permMemorySize, tempMemorySize;
} AppInfo;

typedef struct {
    bool  initialized;
    void *processHandle;
} Metrics;

u64 ReadPageFaultCount(Metrics);

Metrics metrics_init();

u64 ReadCPUTimer();

u64 EstimateCPUTimerFreq();

typedef struct {
    // System
    cstr processorArchitecture;
    u32  numberOfProcessors, pageSize, allocationGranularity;
    f64  cpuFreq;

    // Memory
    u64 totalPhys, availPhys, totalVirtual, availVirtual;

    // OS
    u32 majorVersion, minorVersion, buildNumber, platformId;

    // GPU
    cstr gpuName, gpuVendor, glVersion;
} SystemInfo;

SystemInfo systeminfo_init();

void systeminfo_print(SystemInfo info) {
    INFO("System Information");
    printf("\t> Platform: \t\t\tWindows %s\n", info.processorArchitecture);
    printf("\t> Version: \t\t\t%u.%u.%u\n", info.majorVersion, info.minorVersion, info.buildNumber);
    printf("\t> Processor Count: \t\t%u\n", info.numberOfProcessors);
    printf("\t> CPU Frequency: \t\t%.2f GHz\n", info.cpuFreq);
    printf("\t> Page Size: \t\t\t%u bytes\n", info.pageSize);

    INFO("Memory Information");
    printf("\t> Total Physical Memory: \t%llu MB\n", info.totalPhys / (1024 * 1024));
    printf("\t> Available Physical Memory: \t%llu MB\n", info.availPhys / (1024 * 1024));
    printf("\t> Total Virtual Memory: \t%llu MB\n", info.totalVirtual / (1024 * 1024));
    printf("\t> Available Virtual Memory: \t%llu MB\n", info.availVirtual / (1024 * 1024));
}
