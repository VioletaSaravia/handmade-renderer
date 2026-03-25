#define ENGINE_IMPL
#include "base.c"
#include "profiler.c"

internal inline void _mm_pause() { __asm__ volatile("pause"); }

internal f64 now_seconds() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (f64)t.QuadPart / (f64)EG()->freq.QuadPart;
}

internal Metrics metrics_init() {
    Metrics result = {
        .initialized = true,
        .processHandle =
            OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId()),
    };

    return result;
}

internal u64 get_page_fault_count(Metrics m) {
    MY_PROCESS_MEMORY_COUNTERS_EX counters = {0};
    counters.cb                            = sizeof(counters);
    K32GetProcessMemoryInfo(m.processHandle, &counters, sizeof(counters));

    u64 result = counters.PageFaultCount;
    return result;
}

internal u64 estimate_cpu_freq() {
    u64 MillisecondsToWait = 100;

    LARGE_INTEGER OSFreq = {0};
    QueryPerformanceFrequency(&OSFreq);

    u64           CPUStart = read_cpu_timer();
    LARGE_INTEGER OSStart  = {0};
    QueryPerformanceCounter(&OSStart);
    LARGE_INTEGER OSEnd      = {0};
    u64           OSElapsed  = 0;
    u64           OSWaitTime = OSFreq.QuadPart * MillisecondsToWait / 1000;
    while (OSElapsed < OSWaitTime) {
        QueryPerformanceCounter(&OSEnd);
        OSElapsed = OSEnd.QuadPart - OSStart.QuadPart;
    }

    u64 CPUEnd     = read_cpu_timer();
    u64 CPUElapsed = CPUEnd - CPUStart;

    u64 CPUFreq = 0;
    if (OSElapsed) {
        CPUFreq = OSFreq.QuadPart * CPUElapsed / OSElapsed;
    }

    return CPUFreq;
};

internal u64 read_cpu_timer() {
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64) || \
    defined(__i386__) || defined(_M_IX86)
    u32 lo = 0;
    u32 hi = 0;
    // NOTE(violeta): TCC doesn't support __rdtsc intrinsic
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | lo;

#elif defined(__aarch64__)
    u64 cnt = 0;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));
    return cnt;

#elif defined(__arm__)
    u32 cc = 0;
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
    return (u64)cc;

#else
    static_assert(false, "Unsupported architecture");
    return 0;
#endif
}

// TODO(violeta): What's this??
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

internal SystemInfo systeminfo_init() {
    SystemInfo      result  = {0};
    SYSTEM_INFO     sysInfo = {0};
    MEMORYSTATUSEX  memInfo = {0};
    OSVERSIONINFOEX osInfo  = {0};

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

    result.cpuFreq = (f64)(estimate_cpu_freq()) / 1000.0 / 1000.0 / 1000.0;

    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        result.totalPhys    = memInfo.ullTotalPhys;
        result.availPhys    = memInfo.ullAvailPhys;
        result.totalVirtual = memInfo.ullTotalVirtual;
        result.availVirtual = memInfo.ullAvailVirtual;
    } else {
        result.totalPhys = result.availPhys = result.totalVirtual = result.availVirtual = 0;
    }

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

internal void systeminfo_print(SystemInfo info) {
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

internal bool get_last_write_time(const char *filename, FILETIME *outTime) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) return false;

    *outTime = data.ftLastWriteTime;
    return true;
}

internal void fullscreen_window(HWND hWnd) {
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);

    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        GetWindowPlacement(hWnd, &EG()->prev_placement);

        SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &EG()->prev_placement);
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

internal void center_window(HWND hWnd) {
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

internal inline i64 read_acquire(volatile i64 *src) {
    i64 val = *src;
    __asm__ volatile("" ::: "memory"); // x86 only
    return val;
}

internal void render_filled_triangle(v2i p0, v2i p1, v2i p2, col32 color) {
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
        if (y < 0 || y >= EG()->screen_size.h) continue;

        bool second_half    = (y > p1.y) || (p1.y == p0.y);
        i32  segment_height = second_half ? (p2.y - p1.y) : (p1.y - p0.y);
        if (segment_height == 0) continue;

        q8 alpha = Q8(y - p0.y) / total_height;
        q8 beta  = second_half ? Q8(y - p1.y) / segment_height : Q8(y - p0.y) / segment_height;

        i32 xa = p0.x + q8_to_i32((p2.x - p0.x) * alpha);
        i32 xb = second_half ? p1.x + q8_to_i32((p2.x - p1.x) * beta)
                             : p0.x + q8_to_i32((p1.x - p0.x) * beta);

        if (xa > xb) {
            i32 tmp = xa;
            xa      = xb;
            xb      = tmp;
        }

        // Clamp to screen
        if (xa < 0) xa = 0;
        if (xb >= EG()->screen_size.w) xb = EG()->screen_size.w - 1;

        for (i32 x = xa; x <= xb; x++) {
            EG()->screen_buf[y * EG()->screen_size.w + x] = color;
        }
    }
}

internal void render_textured_triangle(v2i p0, v2i p1, v2i p2, uv t0, uv t1, uv t2, v3 z,
                                       col32 *tex, v2i tex_size) {
    // Sort by y, keeping UVs and Z in sync
    if (p0.y > p1.y) {
        v2i tp   = p0;
        p0       = p1;
        p1       = tp;
        uv tt    = t0;
        t0       = t1;
        t1       = tt;
        q8 tz    = z.val[0];
        z.val[0] = z.val[1];
        z.val[1] = tz;
    }
    if (p0.y > p2.y) {
        v2i tp   = p0;
        p0       = p2;
        p2       = tp;
        uv tt    = t0;
        t0       = t2;
        t2       = tt;
        q8 tz    = z.val[0];
        z.val[0] = z.val[2];
        z.val[2] = tz;
    }
    if (p1.y > p2.y) {
        v2i tp   = p1;
        p1       = p2;
        p2       = tp;
        uv tt    = t1;
        t1       = t2;
        t2       = tt;
        q8 tz    = z.val[1];
        z.val[1] = z.val[2];
        z.val[2] = tz;
    }

    i32 total_height = p2.y - p0.y;
    if (total_height == 0) return;

    // Pre-compute 1/z in Q8: Q8(1) / z = 256 / z
    q8 inv_z0 = z.val[0] != 0 ? Q8(1) * Q8(1) / z.val[0] : 0;
    q8 inv_z1 = z.val[1] != 0 ? Q8(1) * Q8(1) / z.val[1] : 0;
    q8 inv_z2 = z.val[2] != 0 ? Q8(1) * Q8(1) / z.val[2] : 0;

    // u/z, v/z in Q8
    q8 u0z = q8_mul(t0.u, inv_z0), v0z = q8_mul(t0.v, inv_z0);
    q8 u1z = q8_mul(t1.u, inv_z1), v1z = q8_mul(t1.v, inv_z1);
    q8 u2z = q8_mul(t2.u, inv_z2), v2z = q8_mul(t2.v, inv_z2);

    for (i32 y = p0.y; y <= p2.y; y++) {
        if (y < 0 || y >= EG()->screen_size.h) continue;

        bool second_half    = (y > p1.y) || (p1.y == p0.y);
        i32  segment_height = second_half ? (p2.y - p1.y) : (p1.y - p0.y);
        if (segment_height == 0) continue;

        // alpha = (y - p0.y) / total_height, beta similarly, as Q8
        q8 alpha = Q8(y - p0.y) / total_height;
        q8 beta  = second_half ? Q8(y - p1.y) / segment_height : Q8(y - p0.y) / segment_height;

        // Interpolate x
        i32 xa = p0.x + q8_to_i32((p2.x - p0.x) * alpha);
        i32 xb = second_half ? p1.x + q8_to_i32((p2.x - p1.x) * beta)
                             : p0.x + q8_to_i32((p1.x - p0.x) * beta);

        // Interpolate u/z, v/z, 1/z along edges
        q8 uza = u0z + q8_mul(u2z - u0z, alpha);
        q8 vza = v0z + q8_mul(v2z - v0z, alpha);
        q8 iza = inv_z0 + q8_mul(inv_z2 - inv_z0, alpha);

        q8 uzb, vzb, izb;
        if (second_half) {
            uzb = u1z + q8_mul(u2z - u1z, beta);
            vzb = v1z + q8_mul(v2z - v1z, beta);
            izb = inv_z1 + q8_mul(inv_z2 - inv_z1, beta);
        } else {
            uzb = u0z + q8_mul(u1z - u0z, beta);
            vzb = v0z + q8_mul(v1z - v0z, beta);
            izb = inv_z0 + q8_mul(inv_z1 - inv_z0, beta);
        }

        if (xa > xb) {
            i32 tmp = xa;
            xa      = xb;
            xb      = tmp;
            q8 tq   = uza;
            uza     = uzb;
            uzb     = tq;
            tq      = vza;
            vza     = vzb;
            vzb     = tq;
            tq      = iza;
            iza     = izb;
            izb     = tq;
        }

        i32 span = xb - xa;

        i32 x_start = xa < 0 ? 0 : xa;
        i32 x_end   = xb >= EG()->screen_size.w ? EG()->screen_size.w - 1 : xb;

        for (i32 x = x_start; x <= x_end; x++) {
            // t = (x - xa) / span as Q8
            q8 t = span == 0 ? 0 : Q8(x - xa) / span;

            q8 inv_zp = iza + q8_mul(izb - iza, t);
            q8 uzp    = uza + q8_mul(uzb - uza, t);
            q8 vzp    = vza + q8_mul(vzb - vza, t);

            // Recover perspective-correct u, v: u = (u/z) / (1/z)
            q8 tex_u = inv_zp != 0 ? q8_div(uzp, inv_zp) : 0;
            q8 tex_v = inv_zp != 0 ? q8_div(vzp, inv_zp) : 0;

            i32 tx = q8_to_i32(q8_mul(tex_u, Q8(tex_size.w - 1))) & (tex_size.w - 1);
            i32 ty = q8_to_i32(q8_mul(tex_v, Q8(tex_size.h - 1))) & (tex_size.h - 1);

            EG()->screen_buf[y * EG()->screen_size.w + x] = tex[ty * tex_size.w + tx];
        }
    }
}

void tcc_err(void *opaque, cstr msg) { printf(string_format(&ctx()->temp, (char *)msg, "\n")); }

static int sort_draw_cmd_by_distance(const void *a, const void *b) {
    const DrawCmd *da = (const DrawCmd *)a;
    const DrawCmd *db = (const DrawCmd *)b;

    q8 za = v3_add(da->transform.pos, cam()->pos).z;
    q8 zb = v3_add(db->transform.pos, cam()->pos).z;

    if (zb > za) return 1;
    if (zb < za) return -1;
    return 0;
}

#define GAME_ENTRYPOINT "source\\game.c"
static GameDLL load_dll() {
    BLOCK_BEGIN("load_dll");
    GameDLL result = {
        .tcc = tcc_new(),
    };
    if (!result.tcc) goto cleanup;

    get_last_write_time(GAME_ENTRYPOINT, &result.last_write);

    tcc_set_error_func(result.tcc, NULL, tcc_err);

    tcc_add_include_path(result.tcc, "include/winapi");
    tcc_add_library_path(result.tcc, "lib");
    tcc_add_library(result.tcc, "msvcrt");
    tcc_add_library(result.tcc, "kernel32");
    tcc_add_library(result.tcc, "user32");
    tcc_add_library(result.tcc, "gdi32");

    if (tcc_set_output_type(result.tcc, TCC_OUTPUT_MEMORY) == -1) goto cleanup;
    if (tcc_add_file(result.tcc, GAME_ENTRYPOINT) == -1) goto cleanup;
    if (tcc_add_symbol(result.tcc, "G", &G) == -1) goto cleanup;
    if (tcc_add_symbol(result.tcc, "data", &G->game_memory) == -1) goto cleanup;
    if (tcc_relocate(result.tcc, TCC_RELOCATE_AUTO) == -1) goto cleanup;

    INFO("Game compiled");

    result.info = tcc_get_symbol(result.tcc, "game");
    if (!result.info) goto cleanup;

    result.init = tcc_get_symbol(result.tcc, "init");
    if (!result.init) goto cleanup;

    result.update = tcc_get_symbol(result.tcc, "update");
    if (!result.update) goto cleanup;

    result.gamedata_size = tcc_get_symbol(result.tcc, "gamedata_size");
    if (!result.gamedata_size) goto cleanup;

    result.quit = tcc_get_symbol(result.tcc, "quit");
    if (!result.quit) goto cleanup;

    if (G->game.tcc && result.gamedata_size() != G->game.gamedata_size()) result.init();

    BLOCK_END();
    return result;

cleanup:
    if (result.tcc) tcc_delete(result.tcc);
    printf("\n");
    ERR("Couldn't load %s", GAME_ENTRYPOINT);
    BLOCK_END();
    return (GameDLL){0};
}

static bool hot_reload() {
    FILETIME new_write = {0};
    if (!get_last_write_time(GAME_ENTRYPOINT, &new_write)) return false;
    if (CompareFileTime(&new_write, &G->game.last_write) == 0) return false;

    INFO("We're hot reloading!");
    GameDLL new_dll = load_dll();
    if (!new_dll.tcc) {
        return false;
    }

    tcc_delete(G->game.tcc);
    G->game = new_dll;

    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, u32 message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        center_window(hwnd);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    case WM_ERASEBKGND: return 1;

    default: return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

i32 APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nCmdShow) {
    // SetProcessDPIAware();
    {
        Arena perm = arena_new(MB(32), NULL);

        G  = (EngineData *)alloc(sizeof(EngineData), &perm);
        *G = (EngineData){
            .ctx =
                {
                    .perm = perm,
                },
            .prev_placement = {sizeof(WINDOWPLACEMENT)},
            .screen_size    = {.w = 640, .h = 360},
            .draw_size      = 20000,
            .metrics        = metrics_init(),
            .system_info    = systeminfo_init(),
            .profiler       = profiler_new("Handmade Renderer Initialization"),
        };

        col32 *default_texture_data = ALLOC_ARRAY(col32, 64 * 64);
        for (i32 y = 0; y < 64; y++) {
            for (i32 x = 0; x < 64; x++) {
                u32 checker = ((x / 8) % 2) ^ ((y / 8) % 2);
                default_texture_data[y * 64 + x] =
                    rgb(checker ? 200 : 50, checker ? 200 : 50, checker ? 200 : 50);
            }
        }

        G->default_texture = (Texture){
            .data = (col32 *)default_texture_data,
            .size = {.x = 64, .y = 64},
        };
    }

    BLOCK_BEGIN("engine_init");
    ctx()->temp   = arena_new(MB(8), &ctx()->perm);
    G->draw_queue = ALLOC_ARRAY(DrawCmd, G->draw_size);

    QueryPerformanceFrequency(&G->freq);
    G->target_dt  = 1.0f / 60.0f;
    G->dt         = G->target_dt;
    G->next_frame = now_seconds();

    G->game = load_dll();
    if (!G->game.tcc) FATAL("Couldn't initialize TCC");
    G->game.info->keybinds[A_FULLSCREEN][0] = (KeyCombo){K_F11};
    G->game.info->keybinds[A_QUIT][0]       = (KeyCombo){K_F4 | M_SHIFT};
    G->game.info->keybinds[A_RESET][0]      = (KeyCombo){K_F5};

    G->game_memory = alloc_perm(G->game.gamedata_size());
    G->screen_buf  = ALLOC_ARRAY(u32, G->screen_size.w * G->screen_size.h);

    WNDCLASS wc = {
        .hInstance     = hInstance,
        .lpszClassName = G->game.info->name,
        .lpfnWndProc   = (WNDPROC)WndProc,
        .style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW,
        .hbrBackground = NULL, // (HBRUSH)GetStockObject(BLACK_BRUSH),
        .hIcon         = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
    };

    if (!RegisterClass(&wc)) FATAL("Couldn't initialize window");

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT  wr    = {0, 0, G->screen_size.w, G->screen_size.h};
    AdjustWindowRect(&wr, style, false);

    cstr window_name =
        string_format(&G->ctx.perm, "%s %s", G->game.info->name, G->game.info->version);
    G->hwnd = CreateWindow(G->game.info->name, window_name, style, CW_USEDEFAULT, CW_USEDEFAULT,
                           wr.right - wr.left, wr.bottom - wr.top, 0, 0, hInstance, 0);
    if (!G->hwnd) FATAL("Couldn't initialize window");
    BLOCK_END();

    BLOCK_BEGIN("game_init");
    if (G->game.init) G->game.init();
    BLOCK_END();

    profiler_end();

    LOOP_PROFILER();

    while (!G->shutdown) {
        LOOP_BEGIN();
        f64  frame_start = now_seconds();
        bool reloaded    = hot_reload();

        // Input
        {
            for (i32 i = 0; i < K_COUNT; i++) {
                if (G->keys[i] == KS_JUST_RELEASED) G->keys[i] = KS_RELEASED;
                if (G->keys[i] == KS_JUST_PRESSED) G->keys[i] = KS_PRESSED;
            }

            while (PeekMessage(&G->msg, NULL, 0, 0, PM_REMOVE)) {
                switch (G->msg.message) {
                case WM_QUIT: G->shutdown = true; break;

                case WM_LBUTTONDOWN: G->keys[K_MOUSE_LEFT] = KS_JUST_PRESSED; break;
                case WM_LBUTTONUP: G->keys[K_MOUSE_LEFT] = KS_JUST_RELEASED; break;
                case WM_MBUTTONDOWN: G->keys[K_MOUSE_MID] = KS_JUST_PRESSED; break;
                case WM_MBUTTONUP: G->keys[K_MOUSE_MID] = KS_JUST_RELEASED; break;
                case WM_RBUTTONDOWN: G->keys[K_MOUSE_RIGHT] = KS_JUST_PRESSED; break;
                case WM_RBUTTONUP: G->keys[K_MOUSE_RIGHT] = KS_JUST_RELEASED; break;

                case WM_KEYDOWN:
                    switch (G->msg.wParam) {
                    case VK_ESCAPE: break;
                    case VK_CONTROL: G->keys[K_CONTROL] = KS_JUST_PRESSED; break;
                    case VK_SHIFT: G->keys[K_SHIFT] = KS_JUST_PRESSED; break;

                    case VK_F1: G->keys[K_F1] = KS_JUST_PRESSED; break;
                    case VK_F2: G->keys[K_F2] = KS_JUST_PRESSED; break;
                    case VK_F3: G->keys[K_F3] = KS_JUST_PRESSED; break;
                    case VK_F4: G->keys[K_F4] = KS_JUST_PRESSED; break;
                    case VK_F5: G->keys[K_F5] = KS_JUST_PRESSED; break;
                    case VK_F6: G->keys[K_F6] = KS_JUST_PRESSED; break;
                    case VK_F7: G->keys[K_F7] = KS_JUST_PRESSED; break;
                    case VK_F8: G->keys[K_F8] = KS_JUST_PRESSED; break;
                    case VK_F9: G->keys[K_F9] = KS_JUST_PRESSED; break;
                    case VK_F10: G->keys[K_F10] = KS_JUST_PRESSED; break;
                    case VK_F11: G->keys[K_F11] = KS_JUST_PRESSED; break;
                    case VK_F12: G->keys[K_F12] = KS_JUST_PRESSED; break;

                    case VK_UP: G->keys[K_UP] = KS_JUST_PRESSED; break;
                    case VK_DOWN: G->keys[K_DOWN] = KS_JUST_PRESSED; break;
                    case VK_LEFT: G->keys[K_LEFT] = KS_JUST_PRESSED; break;
                    case VK_RIGHT: G->keys[K_RIGHT] = KS_JUST_PRESSED; break;

                    case 'Q': G->keys[K_Q] = KS_JUST_PRESSED; break;
                    case 'W': G->keys[K_W] = KS_JUST_PRESSED; break;
                    case 'F': G->keys[K_F] = KS_JUST_PRESSED; break;
                    case 'P': G->keys[K_P] = KS_JUST_PRESSED; break;
                    case 'G': G->keys[K_G] = KS_JUST_PRESSED; break;
                    case 'J': G->keys[K_J] = KS_JUST_PRESSED; break;
                    case 'L': G->keys[K_L] = KS_JUST_PRESSED; break;
                    case 'U': G->keys[K_U] = KS_JUST_PRESSED; break;
                    case 'Y': G->keys[K_Y] = KS_JUST_PRESSED; break;
                    case 'A': G->keys[K_A] = KS_JUST_PRESSED; break;
                    case 'R': G->keys[K_R] = KS_JUST_PRESSED; break;
                    case 'S': G->keys[K_S] = KS_JUST_PRESSED; break;
                    case 'T': G->keys[K_T] = KS_JUST_PRESSED; break;
                    case 'D': G->keys[K_D] = KS_JUST_PRESSED; break;
                    case 'H': G->keys[K_H] = KS_JUST_PRESSED; break;
                    case 'N': G->keys[K_N] = KS_JUST_PRESSED; break;
                    case 'E': G->keys[K_E] = KS_JUST_PRESSED; break;
                    case 'I': G->keys[K_I] = KS_JUST_PRESSED; break;
                    case 'O': G->keys[K_O] = KS_JUST_PRESSED; break;
                    case 'Z': G->keys[K_Z] = KS_JUST_PRESSED; break;
                    case 'X': G->keys[K_X] = KS_JUST_PRESSED; break;
                    case 'C': G->keys[K_C] = KS_JUST_PRESSED; break;
                    case 'V': G->keys[K_V] = KS_JUST_PRESSED; break;
                    case 'B': G->keys[K_B] = KS_JUST_PRESSED; break;
                    case 'K': G->keys[K_K] = KS_JUST_PRESSED; break;
                    case 'M': G->keys[K_M] = KS_JUST_PRESSED; break;

                    default: break;
                    }
                    break;

                case WM_KEYUP:
                    switch (G->msg.wParam) {
                    case VK_CONTROL: G->keys[K_CONTROL] = KS_JUST_RELEASED; break;
                    case VK_SHIFT: G->keys[K_SHIFT] = KS_JUST_RELEASED; break;

                    case VK_UP: G->keys[K_UP] = KS_JUST_RELEASED; break;
                    case VK_DOWN: G->keys[K_DOWN] = KS_JUST_RELEASED; break;
                    case VK_LEFT: G->keys[K_LEFT] = KS_JUST_RELEASED; break;
                    case VK_RIGHT: G->keys[K_RIGHT] = KS_JUST_RELEASED; break;

                    case VK_F1: G->keys[K_F1] = KS_JUST_RELEASED; break;
                    case VK_F2: G->keys[K_F2] = KS_JUST_RELEASED; break;
                    case VK_F3: G->keys[K_F3] = KS_JUST_RELEASED; break;
                    case VK_F4: G->keys[K_F4] = KS_JUST_RELEASED; break;
                    case VK_F5: G->keys[K_F5] = KS_JUST_RELEASED; break;
                    case VK_F6: G->keys[K_F6] = KS_JUST_RELEASED; break;
                    case VK_F7: G->keys[K_F7] = KS_JUST_RELEASED; break;
                    case VK_F8: G->keys[K_F8] = KS_JUST_RELEASED; break;
                    case VK_F9: G->keys[K_F9] = KS_JUST_RELEASED; break;
                    case VK_F10: G->keys[K_F10] = KS_JUST_RELEASED; break;
                    case VK_F11: G->keys[K_F11] = KS_JUST_RELEASED; break;
                    case VK_F12: G->keys[K_F12] = KS_JUST_RELEASED; break;

                    case 'Q': G->keys[K_Q] = KS_JUST_RELEASED; break;
                    case 'W': G->keys[K_W] = KS_JUST_RELEASED; break;
                    case 'F': G->keys[K_F] = KS_JUST_RELEASED; break;
                    case 'P': G->keys[K_P] = KS_JUST_RELEASED; break;
                    case 'G': G->keys[K_G] = KS_JUST_RELEASED; break;
                    case 'J': G->keys[K_J] = KS_JUST_RELEASED; break;
                    case 'L': G->keys[K_L] = KS_JUST_RELEASED; break;
                    case 'U': G->keys[K_U] = KS_JUST_RELEASED; break;
                    case 'Y': G->keys[K_Y] = KS_JUST_RELEASED; break;
                    case 'A': G->keys[K_A] = KS_JUST_RELEASED; break;
                    case 'R': G->keys[K_R] = KS_JUST_RELEASED; break;
                    case 'S': G->keys[K_S] = KS_JUST_RELEASED; break;
                    case 'T': G->keys[K_T] = KS_JUST_RELEASED; break;
                    case 'D': G->keys[K_D] = KS_JUST_RELEASED; break;
                    case 'H': G->keys[K_H] = KS_JUST_RELEASED; break;
                    case 'N': G->keys[K_N] = KS_JUST_RELEASED; break;
                    case 'E': G->keys[K_E] = KS_JUST_RELEASED; break;
                    case 'I': G->keys[K_I] = KS_JUST_RELEASED; break;
                    case 'O': G->keys[K_O] = KS_JUST_RELEASED; break;
                    case 'Z': G->keys[K_Z] = KS_JUST_RELEASED; break;
                    case 'X': G->keys[K_X] = KS_JUST_RELEASED; break;
                    case 'C': G->keys[K_C] = KS_JUST_RELEASED; break;
                    case 'V': G->keys[K_V] = KS_JUST_RELEASED; break;
                    case 'B': G->keys[K_B] = KS_JUST_RELEASED; break;
                    case 'K': G->keys[K_K] = KS_JUST_RELEASED; break;
                    case 'M': G->keys[K_M] = KS_JUST_RELEASED; break;

                    default: break;
                    }
                    break;

                case WM_MOUSEMOVE:
                    G->mouse_pos = (v2){
                        .x = Q8(G->msg.lParam & 0xFFFF),
                        .y = Q8((G->msg.lParam >> 16) & 0xFFFF),
                    };
                    break;

                default: break;
                }

                TranslateMessage(&G->msg);
                DispatchMessage(&G->msg);
            }

            if (GetAction(A_FULLSCREEN) == KS_JUST_PRESSED) fullscreen_window(G->hwnd);
            if (GetAction(A_QUIT) == KS_JUST_PRESSED) DestroyWindow(G->hwnd);
            if (GetAction(A_RESET) == KS_JUST_PRESSED) G->game.init();
        }

        LOOP_BLOCK("Game Update");
        G->game.update((q8)(G->dt * 256.0f));
        LOOP_BLOCK_END();

        // Rendering
        {
            LOOP_BLOCK("Rendering");
            if (G->draw_count == G->draw_size) WARN("Maximum draw_size reached");

            qsort(G->draw_queue, G->draw_count, sizeof(DrawCmd), sort_draw_cmd_by_distance);

            for (i32 i = 0; i < G->draw_count; i++) {
                DrawCmd next = G->draw_queue[i];

                switch (next.t) {
                case DCT_MESH_WIREFRAME: {
                    v2i *screen_verts = (v2i *)alloc_temp(sizeof(v2i) * next.count);
                    for (i32 v = 0; v < next.count; v++) {
                        v3 n            = next.vertices[v];
                        screen_verts[v] = v2i_from_v2(v2_screen(v3_project(n), G->screen_size));
                    }

                    for (i32 v = 0; v < next.edges_count; v++) {
                        v2i edge = next.edges[v];
                        render_line(screen_verts[edge.from], screen_verts[edge.to], WHITE);
                    }
                    break;
                }
                case DCT_MESH_SOLID: {
                    v2i *screen_verts  = (v2i *)alloc_temp(sizeof(v2i) * next.count);
                    q8  *transformed_z = (q8 *)alloc_temp(sizeof(q8) * next.count);
                    for (i32 v = 0; v < next.count; v++) {
                        v3 n = v3_mul(next.vertices[v], next.transform.scale);
                        n    = v3_rotate_xz(n, next.transform.rot.y);
                        n    = v3_add(n, next.transform.pos);
                        n    = v3_add(n, cam()->pos);

                        transformed_z[v] = n.z;
                        v2 vec           = v3_project(n);
                        vec              = v2_screen(vec, G->screen_size);
                        screen_verts[v]  = v2i_from_v2(vec);
                    }
                    if (!next.faces || next.faces_count == 0) break;

                    for (i32 f = 0; f < next.faces_count; f++) {
                        Face face = next.faces[f];

                        {
                            v2i sa      = screen_verts[face.a];
                            v2i sb      = screen_verts[face.b];
                            v2i sc      = screen_verts[face.c];
                            i32 cross2d = v2i_cross((v2i){.x = sb.x - sa.x, .y = sb.y - sa.y},
                                                    (v2i){.x = sc.x - sa.x, .y = sc.y - sa.y});
                            if (cross2d >= 0) continue;
                        }

                        render_filled_triangle(screen_verts[face.a], screen_verts[face.b],
                                               screen_verts[face.c], next.color);
                    }

                    break;
                }
                case DCT_MODEL: {
                    v2i *screen_verts  = ALLOC_TEMP_ARRAY(v2i, next.count);
                    q8  *transformed_z = ALLOC_TEMP_ARRAY(q8, next.count);
                    for (i32 v = 0; v < next.count; v++) {
                        v3 n = v3_mul(next.vertices[v], next.transform.scale);
                        n    = v3_rotate_xz(n, next.transform.rot.y);
                        n    = v3_add(n, next.transform.pos);
                        n    = v3_add(n, cam()->pos);

                        transformed_z[v] = n.z;
                        v2 vec           = v3_project(n);
                        vec              = v2_screen(vec, G->screen_size);
                        screen_verts[v]  = v2i_from_v2(vec);
                    }
                    if (!next.faces || next.faces_count == 0) break;

                    for (i32 f = 0; f < next.faces_count; f++) {
                        Face face = next.faces[f];

                        {
                            v2i sa      = screen_verts[face.a];
                            v2i sb      = screen_verts[face.b];
                            v2i sc      = screen_verts[face.c];
                            i32 cross2d = v2i_cross((v2i){.x = sb.x - sa.x, .y = sb.y - sa.y},
                                                    (v2i){.x = sc.x - sa.x, .y = sc.y - sa.y});
                            if (cross2d >= 0) continue;
                        }

                        render_textured_triangle(screen_verts[face.a], screen_verts[face.b],
                                                 screen_verts[face.c], face.uva, face.uvb, face.uvc,
                                                 (v3){transformed_z[face.a], transformed_z[face.b],
                                                      transformed_z[face.c]},
                                                 next.tex, next.tex_size);
                    }

                    break;
                }
                case DCT_RECT: {
                    next.r = (rect){
                        .x = q8_to_i32(next.r.x),
                        .y = q8_to_i32(next.r.y),
                        .w = q8_to_i32(next.r.w),
                        .h = q8_to_i32(next.r.h),
                    };
                    for (i32 y_coord = next.r.y; y_coord < next.r.y + next.r.h; y_coord++) {
                        for (i32 x_coord = next.r.x; x_coord < next.r.x + next.r.w; x_coord++) {
                            if (x_coord < 0 || x_coord >= G->screen_size.w || y_coord < 0 ||
                                y_coord >= G->screen_size.h) {
                                continue;
                            }

                            i32 coord            = y_coord * G->screen_size.w + x_coord;
                            G->screen_buf[coord] = next.color;
                        }
                    }
                    break;
                }

                case DCT_TEXTURE_2D: {
                    Texture *tex      = next.texture;
                    v2i      out_size = next.params.dst.size.x == 0 || next.params.dst.size.y == 0
                                            ? tex->size
                                            : next.params.dst.size;
                    v2i      in_size  = next.params.src.size.x == 0 || next.params.src.size.y == 0
                                            ? tex->size
                                            : next.params.src.size;

                    v2 ratio = (v2){q8_div(Q8(in_size.x), Q8(out_size.x)),
                                    q8_div(Q8(in_size.y), Q8(out_size.y))};

                    for (i32 y = 0; y < out_size.y; y++) {
                        for (i32 x = 0; x < out_size.x; x++) {
                            v2i screen_pos = v2i_add(next.params.pos, (v2i){x, y});

                            if (screen_pos.x < 0 || screen_pos.x >= G->screen_size.w ||
                                screen_pos.y < 0 || screen_pos.y >= G->screen_size.h) {
                                continue;
                            }

                            i32 screen_idx  = screen_pos.y * G->screen_size.w + screen_pos.x;
                            v2i tex_coord   = (v2i){q8_to_i32(q8_mul(Q8(x), ratio.x)),
                                                    q8_to_i32(q8_mul(Q8(y), ratio.y))};
                            i32 texture_idx = tex_coord.y * tex->size.x + tex_coord.x;

                            G->screen_buf[screen_idx] = tex->data[texture_idx];
                        }
                    }
                    break;
                }
                default: break;
                }
            }

            HDC  hdc = GetDC(G->hwnd);
            RECT rc  = {0};
            GetClientRect(G->hwnd, &rc);

            i32 client_w = rc.right - rc.left;
            i32 client_h = rc.bottom - rc.top;

            HDC     mem_dc  = CreateCompatibleDC(hdc);
            HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, client_w, client_h);
            HBITMAP old_bmp = SelectObject(mem_dc, mem_bmp);

            HDC              buf_dc  = CreateCompatibleDC(hdc);
            BITMAPINFOHEADER buf_bmi = {
                .biSize        = sizeof(BITMAPINFOHEADER),
                .biWidth       = G->screen_size.w,
                .biHeight      = -G->screen_size.h,
                .biPlanes      = 1,
                .biBitCount    = 32,
                .biCompression = BI_RGB,
            };
            void   *buf_bits = NULL;
            HBITMAP buf_bmp  = CreateDIBSection(buf_dc, (BITMAPINFO *)&buf_bmi, DIB_RGB_COLORS,
                                                &buf_bits, NULL, 0);
            HBITMAP buf_old  = SelectObject(buf_dc, buf_bmp);

            memcpy(buf_bits, G->screen_buf, G->screen_size.w * G->screen_size.h * sizeof(u32));

            for (i32 i = 0; i < G->draw_count; i++) {
                DrawCmd next = G->draw_queue[i];
                if (next.t == DCT_TEXT) {
                    RECT r = {
                        .left   = next.x,
                        .top    = next.y,
                        .right  = G->screen_size.w,
                        .bottom = G->screen_size.h,
                    };
                    SetTextColor(buf_dc, next.color);
                    SetBkMode(buf_dc, TRANSPARENT);
                    DrawText(buf_dc, next.text, -1, &r, DT_LEFT | DT_TOP);
                }
            }

            GdiFlush();
            CopyMemory(G->screen_buf, buf_bits, G->screen_size.w * G->screen_size.h * sizeof(u32));

            SelectObject(buf_dc, buf_old);
            DeleteObject(buf_bmp);
            DeleteDC(buf_dc);

            BITMAPINFOHEADER bmi = {
                .biSize        = sizeof(BITMAPINFOHEADER),
                .biWidth       = G->screen_size.w,
                .biHeight      = -G->screen_size.h,
                .biPlanes      = 1,
                .biBitCount    = 32,
                .biCompression = BI_RGB,
            };

            StretchDIBits(mem_dc, 0, 0, client_w, client_h, 0, 0, G->screen_size.w,
                          G->screen_size.h, G->screen_buf, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
                          SRCCOPY);

            BitBlt(hdc, 0, 0, client_w, client_h, mem_dc, 0, 0, SRCCOPY);

            SelectObject(mem_dc, old_bmp);
            DeleteObject(mem_bmp);
            DeleteDC(mem_dc);

            ReleaseDC(G->hwnd, hdc);
            G->draw_count = 0;
            LOOP_BLOCK_END();
        }

        // Timing
        {
            LOOP_BLOCK("Leftover");
            G->next_frame += G->target_dt;

            f64 remaining = G->next_frame - now_seconds();
            if (remaining > 0.0) {
                DWORD sleep_ms = (DWORD)(remaining * 1000.0);
                if (sleep_ms > 0) Sleep(sleep_ms);
            }

            arena_reset(&ctx()->temp, 0);
            G->dt = now_seconds() - frame_start;
            LOOP_BLOCK_END();
        }

        LOOP_END();
    }

    if (G->game.quit) G->game.quit();

    return G->msg.wParam;
}