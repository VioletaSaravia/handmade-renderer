#include "windows.h"
#include <libtcc/libtcc.h>

#include "base.h"
#include "profiler.h"

typedef enum { DCT_RECT, DCT_RECT_OUTLINE, DCT_TEXT, DCT_LINE, DCT_MESH, DCT_COUNT } DrawCmdType;

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

        struct { // line
            v2 from, to;
        };

        struct { // mesh
            v3  *vertices;
            i32  count;
            v2i *edges;
            i32  edges_count;
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

typedef struct {
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
} EngineData;

#ifdef ENGINE_IMPL
static EngineData *G;
#else
import extern EngineData *G;
#endif

Context *ctx() { return &G->ctx; }

void draw_mesh(v3 *p, i32 count, v2i *e, i32 edges_count, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t           = DCT_MESH,
                                               .vertices    = p,
                                               .count       = count,
                                               .edges       = e,
                                               .edges_count = edges_count,
                                               .color       = color};
}

void draw_rect(rect r, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t = DCT_RECT, .r = r, .color = color};
}

void draw_rect_outline(rect r, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t = DCT_RECT_OUTLINE, .r = r, .color = color};
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

void render_line(v2i from, v2i to, col32 color) {
    i32 dx = to.x - from.x;
    i32 dy = to.y - from.y;

    i32 steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) return;

    f32 x_inc = (f32)dx / steps;
    f32 y_inc = (f32)dy / steps;

    f32 x = (f32)from.x;
    f32 y = (f32)from.y;

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

char *string_format(Arena *a, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    va_list copy;
    va_copy(copy, args);

    i32 needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *result = alloc((u64)needed + 1, a);

    vsnprintf(result, (u64)needed + 1, fmt, copy);
    va_end(copy);

    return result;
}

bool gui_button(char *name, q8 x, q8 y) {
    rect  r     = {x, y, Q8(60), Q8(20)};
    bool  col   = col_point_rect(G->mouse_pos, r);
    col32 color = col ? rgb(100, 100, 100) : rgb(30, 30, 30);

    draw_rect(r, color);
    draw_text(name, q8_to_i32(x), q8_to_i32(y), rgb(250, 250, 250));

    return G->keys[K_MOUSE_LEFT] == KS_JUST_PRESSED && col;
}

bool gui_toggle(char *name, q8 x, q8 y, bool *val) {
    rect  r     = {x, y, Q8(60), Q8(20)};
    col32 color = *val ? rgb(100, 100, 100) : rgb(30, 30, 30);

    draw_rect(r, color);
    draw_text(name, q8_to_i32(x), q8_to_i32(y), rgb(250, 250, 250));

    bool pressed = G->keys[K_MOUSE_LEFT] == KS_JUST_PRESSED && col_point_rect(G->mouse_pos, r);
    if (pressed) *val ^= true;
    return pressed;
}

// Manually declare what we need instead of Psapi.h
typedef struct {
    u32  cb;
    u32  PageFaultCount;
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

static f64 now_seconds() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (f64)t.QuadPart / (f64)G->freq.QuadPart;
}

// TCC needs these declared as regular C functions with __stdcall
import BOOL __stdcall K32GetProcessMemoryInfo(HANDLE, MY_PROCESS_MEMORY_COUNTERS_EX *, u32);
import BOOL __stdcall GlobalMemoryStatusEx(MEMORYSTATUSEX *);

#undef EXPORT
#define EXPORT extern "C" __declspec(dllexport)

Metrics metrics_init() {
    Metrics result = {
        .initialized = true,
        .processHandle =
            OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId()),
    };

    return result;
}

u64 ReadPageFaultCount(Metrics m) {
    MY_PROCESS_MEMORY_COUNTERS_EX counters = {0};
    counters.cb                            = sizeof(counters);
    K32GetProcessMemoryInfo(m.processHandle, &counters, sizeof(counters));

    u64 result = counters.PageFaultCount;
    return result;
}

u64 EstimateCPUTimerFreq() {
    u64 MillisecondsToWait = 100;

    LARGE_INTEGER OSFreq = {0};
    QueryPerformanceFrequency(&OSFreq);

    u64           CPUStart = ReadCPUTimer();
    LARGE_INTEGER OSStart  = {0};
    QueryPerformanceCounter(&OSStart);
    LARGE_INTEGER OSEnd      = {0};
    u64           OSElapsed  = 0;
    u64           OSWaitTime = OSFreq.QuadPart * MillisecondsToWait / 1000;
    while (OSElapsed < OSWaitTime) {
        QueryPerformanceCounter(&OSEnd);
        OSElapsed = OSEnd.QuadPart - OSStart.QuadPart;
    }

    u64 CPUEnd     = ReadCPUTimer();
    u64 CPUElapsed = CPUEnd - CPUStart;

    u64 CPUFreq = 0;
    if (OSElapsed) {
        CPUFreq = OSFreq.QuadPart * CPUElapsed / OSElapsed;
    }

    return CPUFreq;
};

u64 ReadCPUTimer(void) {
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64) || \
    defined(__i386__) || defined(_M_IX86)
    // TCC doesn't support __rdtsc intrinsic; use inline asm instead
    u32 lo = 0;
    u32 hi = 0;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;

#elif defined(__aarch64__)
    // ARMv8 (AArch64): use CNTVCT_EL0
    u64 cnt = 0;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));
    return cnt;

#elif defined(__arm__)
    // ARMv7-A: use PMCCNTR (if enabled)
    u32 cc = 0;
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
    return (u64)cc;

#else
    static_assert(false, "Unsupported architecture");
    return 0;
#endif
}

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

SystemInfo systeminfo_init() {
    SystemInfo      result = {0};
    SYSTEM_INFO     sysInfo;
    MEMORYSTATUSEX  memInfo;
    OSVERSIONINFOEX osInfo;

    // System
    GetSystemInfo(&sysInfo);
    // processorArchitecture = sysInfo.wProcessorArchitecture;
    result.numberOfProcessors    = sysInfo.dwNumberOfProcessors;
    result.pageSize              = sysInfo.dwPageSize;
    result.allocationGranularity = sysInfo.dwAllocationGranularity;

    result.processorArchitecture = "Unknown";
    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: result.processorArchitecture = "x64 (AMD/Intel)"; break;
    case PROCESSOR_ARCHITECTURE_INTEL: result.processorArchitecture = "x86"; break;
    case PROCESSOR_ARCHITECTURE_ARM: result.processorArchitecture = "ARM"; break;
    case PROCESSOR_ARCHITECTURE_ARM64: result.processorArchitecture = "ARM64"; break;
    }

    result.cpuFreq = (f64)(EstimateCPUTimerFreq()) / 1000.0 / 1000.0 / 1000.0;

    // Memory
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        result.totalPhys    = memInfo.ullTotalPhys;
        result.availPhys    = memInfo.ullAvailPhys;
        result.totalVirtual = memInfo.ullTotalVirtual;
        result.availVirtual = memInfo.ullAvailVirtual;
    } else {
        result.totalPhys = result.availPhys = result.totalVirtual = result.availVirtual = 0;
    }

    // OS
    ZeroMemory(&osInfo, sizeof(osInfo));
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    if (GetVersionEx((OSVERSIONINFO *)&osInfo)) {
        result.majorVersion = osInfo.dwMajorVersion;
        result.minorVersion = osInfo.dwMinorVersion;
        result.buildNumber  = osInfo.dwBuildNumber;
        result.platformId   = osInfo.dwPlatformId;
    } else {
        result.majorVersion = result.minorVersion = result.buildNumber = result.platformId = 0;
    }

    systeminfo_print(result);

    return result;
}

static bool GetLastWriteTime(const char *filename, FILETIME *outTime) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) return false;

    *outTime = data.ftLastWriteTime;
    return true;
}

static void FullscreenWindow(HWND hWnd) {
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);

    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        GetWindowPlacement(hWnd, &G->prev_placement);

        SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &G->prev_placement);
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

static void CenterWindow(HWND hWnd) {
    HWND hwnd_parent;
    RECT rw_self, rc_parent, rw_parent;
    i32  xpos, ypos;

    hwnd_parent = GetParent(hWnd);
    if (NULL == hwnd_parent) hwnd_parent = GetDesktopWindow();

    GetWindowRect(hwnd_parent, &rw_parent);
    GetClientRect(hwnd_parent, &rc_parent);
    GetWindowRect(hWnd, &rw_self);

    xpos = rw_parent.left + (rc_parent.right + rw_self.left - rw_self.right) / 2;
    ypos = rw_parent.top + (rc_parent.bottom + rw_self.top - rw_self.bottom) / 2;

    SetWindowPos(hWnd, NULL, xpos, ypos, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#define THREAD_COUNT 8

static inline i64 read_acquire(volatile i64 *src) {
    i64 val = *src;
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("" ::: "memory"); // compiler barrier (x86 has strong ordering)
#elif defined(__aarch64__)
    __asm__ volatile("dmb ishld" ::: "memory"); // data memory barrier (acquire)
#elif defined(__arm__)
    __asm__ volatile("dmb ish" ::: "memory");
#else
    __asm__ volatile("" ::: "memory"); // fallback: compiler barrier only
#endif
    return val;
}

static inline void _mm_pause(void) {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("pause");
#elif defined(__aarch64__)
    __asm__ volatile("yield");
#elif defined(__arm__)
    __asm__ volatile("yield");
#else
    // no-op
#endif
}

void thread_barrier(void) {
    static volatile i64 barrier_count      = 0;
    static volatile i64 barrier_generation = 0;
    static i32          barrier_total      = THREAD_COUNT;

    i64 gen       = read_acquire(&barrier_generation);
    i64 new_count = InterlockedIncrement(&barrier_count);

    if (new_count == barrier_total) {
        InterlockedExchange(&barrier_count, 0);
        InterlockedIncrement(&barrier_generation);
    } else {
        while (read_acquire(&barrier_generation) == gen) {
            YieldProcessor();
        }
    }
}

u8 *os_alloc(i32 size) {
    return (u8 *)VirtualAlloc(NULL, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void render_filled_triangle(v2i p0, v2i p1, v2i p2, col32 color) {
    // Sort vertices by y coordinate (p0.y <= p1.y <= p2.y)
    if (p0.y > p1.y) {
        v2i tmp = p0;
        p0      = p1;
        p1      = tmp;
    }
    if (p0.y > p2.y) {
        v2i tmp = p0;
        p0      = p2;
        p2      = tmp;
    }
    if (p1.y > p2.y) {
        v2i tmp = p1;
        p1      = p2;
        p2      = tmp;
    }

    i32 total_height = p2.y - p0.y;
    if (total_height == 0) return;

    for (i32 y = p0.y; y <= p2.y; y++) {
        if (y < 0 || y >= G->screen_size.h) continue;

        bool second_half    = (y > p1.y) || (p1.y == p0.y);
        i32  segment_height = second_half ? (p2.y - p1.y) : (p1.y - p0.y);
        if (segment_height == 0) continue;

        f32 alpha = (f32)(y - p0.y) / (f32)total_height;
        f32 beta  = second_half ? (f32)(y - p1.y) / (f32)segment_height
                                : (f32)(y - p0.y) / (f32)segment_height;

        i32 xa = (i32)(p0.x + (p2.x - p0.x) * alpha);
        i32 xb =
            second_half ? (i32)(p1.x + (p2.x - p1.x) * beta) : (i32)(p0.x + (p1.x - p0.x) * beta);

        if (xa > xb) {
            i32 tmp = xa;
            xa      = xb;
            xb      = tmp;
        }

        // Clamp to screen
        if (xa < 0) xa = 0;
        if (xb >= G->screen_size.w) xb = G->screen_size.w - 1;

        for (i32 x = xa; x <= xb; x++) {
            G->screen_buf[y * G->screen_size.w + x] = color;
        }
    }
}