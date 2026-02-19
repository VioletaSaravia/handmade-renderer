#include <libtcc/libtcc.h>
#include <windows.h>

#define ENGINE_IMPL
#include "base_win.c"

static bool GetLastWriteTime(const char *filename, FILETIME *outTime) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) return false;

    *outTime = data.ftLastWriteTime;
    return true;
}

void tcc_err(void *opaque, const char *msg) { OutputDebugString(msg); }

static bool LoadGameDLL() {
    GetLastWriteTime("game.c", &G->last_write);

    if (G->tcc) {
        tcc_delete(G->tcc);
        G->tcc    = NULL;
        G->init   = NULL;
        G->update = NULL;
    }

    G->tcc = tcc_new();
    if (!G->tcc) return false;

    tcc_set_error_func(G->tcc, NULL, tcc_err);

    tcc_add_include_path(G->tcc, "include/winapi");
    tcc_add_library_path(G->tcc, "lib");
    tcc_add_library(G->tcc, "msvcrt");
    tcc_add_library(G->tcc, "kernel32");
    tcc_add_library(G->tcc, "user32");
    tcc_add_library(G->tcc, "gdi32");

    if (tcc_set_output_type(G->tcc, TCC_OUTPUT_MEMORY) == -1) return false;
    if (tcc_add_file(G->tcc, "game.c") == -1) return false;
    if (tcc_add_symbol(G->tcc, "G", &G) == -1) return false;
    if (tcc_add_symbol(G->tcc, "data", &G->game_memory) == -1) return false;
    if (tcc_relocate(G->tcc, TCC_RELOCATE_AUTO) == -1) return false;

    void (*new_init)() = tcc_get_symbol(G->tcc, "init");
    if (new_init) G->init = new_init;

    void (*new_update)() = tcc_get_symbol(G->tcc, "update");
    if (new_update)
        G->update = new_update;
    else
        return false;

    i32 (*new_gamedata_size)() = tcc_get_symbol(G->tcc, "gamedata_size");
    if (new_gamedata_size)
        G->gamedata_size = new_gamedata_size;
    else
        return false;

    if (G->init && G->gamedata_size() != G->last_gamedata_size && G->last_gamedata_size != 0)
        G->init();
    G->last_gamedata_size = G->gamedata_size();

    void (*new_quit)() = tcc_get_symbol(G->tcc, "quit");
    if (new_quit) G->quit = new_quit;

    return true;
}

static void CheckHotReload() {
    FILETIME newTime = {0};

    if (!GetLastWriteTime("game.c", &newTime)) {
        return;
    }

    if (CompareFileTime(&newTime, &G->last_write) != 0) {
        LoadGameDLL();
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

LRESULT CALLBACK WndProc(HWND hwnd, u32 message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CenterWindow(hwnd);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    case WM_ERASEBKGND: return 1;

    default: return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

#define APPNAME "HELLO_WIN"

static f64 now_seconds() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (f64)t.QuadPart / (f64)G->freq.QuadPart;
}

i32 APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nCmdShow) {
    G  = VirtualAlloc(NULL, sizeof(EngineData), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    *G = (EngineData){
        .prev_placement = {sizeof(WINDOWPLACEMENT)},
        .screen_size    = {.w = 360, .h = 240},
        .draw_queue =
            VirtualAlloc(NULL, sizeof(DrawCmd) * 128, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE),
        .draw_size = 128,
    };

    QueryPerformanceFrequency(&G->freq);
    const f32 target_dt  = 1.0f / 60.0f; // 16.666 ms
    f32       dt         = target_dt;
    f64       next_frame = now_seconds();

    if (!LoadGameDLL()) return 1;

    G->game_memory =
        (u8 *)VirtualAlloc(NULL, G->gamedata_size() * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    G->screen_buf = VirtualAlloc(NULL, G->screen_size.w * G->screen_size.h * sizeof(u32),
                                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    char szAppName[] = APPNAME; // The name of this application
    char szTitle[]   = APPNAME; // The title bar text

    WNDCLASS wc = {
        .hInstance     = hInstance,
        .lpszClassName = szAppName,
        .lpfnWndProc   = (WNDPROC)WndProc,
        .style         = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW,
        .hbrBackground = NULL, //(HBRUSH)GetStockObject(BLACK_BRUSH),
        .hIcon         = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
    };

    if (FALSE == RegisterClass(&wc)) return 0;

    // create the browser
    HWND hwnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             G->screen_size.w, // CW_USEDEFAULT,
                             G->screen_size.h, // CW_USEDEFAULT,
                             0, 0, hInstance, 0);

    if (!hwnd) return 0;

    if (G->init) G->init();

    bool shutdown = false;
    MSG  msg;
    draw_rect((rect){0, 0, G->screen_size.w, G->screen_size.h}, rgb(255, 255, 255));
    while (!shutdown) {
        f64 frame_start = now_seconds();

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
            case WM_QUIT: shutdown = true; break;

            case WM_KEYDOWN:
                switch (msg.wParam) {
                case VK_ESCAPE: DestroyWindow(hwnd); break;

                case VK_F5: LoadGameDLL(); break;
                case VK_F6:
                    LoadGameDLL();
                    if (G->init) G->init();
                    break;

                case VK_F11: FullscreenWindow(hwnd); break;

                default: break;
                }
                break;

            case WM_MOUSEMOVE:
                G->mouse_pos = (v2i){
                    .x = (i16)(msg.lParam & 0xFFFF),
                    .y = (i16)((msg.lParam >> 16) & 0xFFFF),
                };
                break;

            default: break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        G->update(dt);

        {
            // Rendering
            PAINTSTRUCT ps  = {0};
            HDC         hdc = GetDC(hwnd);
            RECT        rc  = {0};

            GetClientRect(hwnd, &rc);

            // i32 width  = rc.right;
            // i32 height = rc.bottom;

            // Draw to screen
            BITMAPINFOHEADER bmi = {
                .biSize        = sizeof(BITMAPINFOHEADER),
                .biWidth       = G->screen_size.w,
                .biHeight      = -G->screen_size.h, // Negative = top-down
                .biPlanes      = 1,
                .biBitCount    = 32,
                .biCompression = BI_RGB,
            };

            SetDIBitsToDevice(hdc, 0, 0, G->screen_size.w, G->screen_size.h, 0, 0, 0,
                              G->screen_size.h, G->screen_buf, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

            for (i32 i = 0; i < G->draw_count; i++) {
                DrawCmd next = G->draw_queue[i];

                switch (next.t) {
                case DCT_TEXT: {
                    RECT r = {
                        .left   = next.x,
                        .top    = next.y,
                        .right  = next.x + G->screen_size.w,
                        .bottom = next.y + G->screen_size.h,
                    };
                    SetTextColor(hdc, next.color);
                    SetBkMode(hdc, TRANSPARENT);

                    DrawText(hdc, next.text, -1, &r, DT_LEFT | DT_TOP);
                    break;
                }

                case DCT_RECT: {
                    for (i32 y_coord = next.r.y; y_coord < next.r.y + next.r.h; y_coord++) {
                        for (i32 x_coord = next.r.x; x_coord < next.r.x + next.r.w; x_coord++) {
                            i32 coord = (y_coord % G->screen_size.h) * G->screen_size.w +
                                        (x_coord % G->screen_size.w);
                            G->screen_buf[coord] = next.color;
                        }
                    }
                    break;
                }
                }
            }

            ReleaseDC(hwnd, hdc);
            G->draw_count = 0;
        }

        next_frame += target_dt;

        f64 remaining = next_frame - now_seconds();
        if (remaining > 0.0) {
            DWORD sleep_ms = (DWORD)(remaining * 1000.0);
            if (sleep_ms > 0) Sleep(sleep_ms);
        }

        dt = now_seconds() - frame_start;
    }

    if (G->quit) G->quit();
    return msg.wParam;
}