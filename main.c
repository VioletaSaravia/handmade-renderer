#include <windows.h>

#include "base.h"
#include "libtcc.h"

typedef struct {
    char *text;
    i32   x, y;
    col32 color;
} DrawCmd;

#define DRAW_QUEUE_MAX 64

static struct {
    void     *memory;
    Context   ctx;
    TCCState *tcc;
    char      text_buf[256];

    void (*init)(void);
    void (*update)(void);

    FILETIME        lastWriteTime; // Never set/reset?
    WINDOWPLACEMENT wpPrev;
    v2i             mousePos;
    v2i             screen_size;
    u32            *screen_buf;
    DrawCmd         draw_queue[DRAW_QUEUE_MAX];
    u32             draw_count;
} G = {
    .wpPrev      = {sizeof(WINDOWPLACEMENT)},
    .screen_size = {.w = 360, .h = 240},
};

Context *ctx() { return &G.ctx; }

void draw_rect(rect r, col32 color) {
    for (i32 y_coord = r.y; y_coord < r.y + r.h; y_coord++) {
        for (i32 x_coord = r.x; x_coord < r.x + r.w; x_coord++) {
            G.screen_buf[y_coord * G.screen_size.w + x_coord] = color;
        }
    }
}

void draw_circle(i32 x, i32 y, i32 r, col32 color) {
    for (i32 y_coord = y - r; y_coord <= y + r; y_coord++) {
        for (i32 x_coord = x - r; x_coord <= x + r; x_coord++) {
            i32 dx = x_coord - x;
            i32 dy = y_coord - y;
            if (dx * dx + dy * dy <= r * r) {
                if (x_coord >= 0 && x_coord < G.screen_size.w && y_coord >= 0 &&
                    y_coord < G.screen_size.h) {
                    G.screen_buf[y_coord * G.screen_size.w + x_coord] = color;
                }
            }
        }
    }
}

f32 atan2f(f32 y, f32 x) {
    const f32 PI   = 3.14159265358979f;
    const f32 PI_2 = 1.57079632679489f;

    if (x == 0.0f) {
        if (y > 0.0f)
            return PI_2;
        if (y < 0.0f)
            return -PI_2;
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
                    if (x_coord >= 0 && x_coord < G.screen_size.w && y_coord >= 0 &&
                        y_coord < G.screen_size.h) {
                        G.screen_buf[y_coord * G.screen_size.w + x_coord] = color;
                    }
                }
            }
        }
    }
}

void draw_text(char *text, i32 x, i32 y, col32 color) {
    if (G.draw_count == DRAW_QUEUE_MAX)
        return;

    G.draw_queue[G.draw_count++] = (DrawCmd){.text = text, .x = x, .y = y, .color = color};
}

bool GetLastWriteTime(const char *filename, FILETIME *outTime) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
        return false;

    *outTime = data.ftLastWriteTime;
    return true;
}

void tcc_err(void *opaque, const char *msg) { OutputDebugString(msg); }

bool ReloadUpdate() {
    if (G.tcc) {
        tcc_delete(G.tcc);
        G.tcc    = NULL;
        G.update = NULL;
    }

    G.tcc = tcc_new();
    if (!G.tcc)
        return false;

    tcc_set_error_func(G.tcc, NULL, tcc_err);

    tcc_add_include_path(G.tcc, "include/winapi");
    tcc_add_library_path(G.tcc, "lib");
    tcc_add_library(G.tcc, "msvcrt");
    tcc_add_library(G.tcc, "kernel32");
    tcc_add_library(G.tcc, "user32");
    tcc_add_library(G.tcc, "gdi32");

    if (tcc_set_output_type(G.tcc, TCC_OUTPUT_MEMORY) == -1)
        return false;
    if (tcc_add_file(G.tcc, "game.c") == -1)
        return false;
    if (tcc_add_symbol(G.tcc, "data", G.memory) == -1)
        return false;
    if (tcc_relocate(G.tcc, TCC_RELOCATE_AUTO) == -1)
        return false;

    G.init = tcc_get_symbol(G.tcc, "init");
    if (!G.init)
        return false;
    G.update = tcc_get_symbol(G.tcc, "update");
    if (!G.update)
        return false;

    return true;
}

void CheckHotReload() {
    FILETIME newTime;

    if (!GetLastWriteTime("update.c", &newTime))
        return;

    if (CompareFileTime(&newTime, &G.lastWriteTime) != 0)
        ReloadUpdate();
}

void CenterWindow(HWND hWnd) {
    HWND hwnd_parent;
    RECT rw_self, rc_parent, rw_parent;
    i32  xpos, ypos;

    hwnd_parent = GetParent(hWnd);
    if (NULL == hwnd_parent)
        hwnd_parent = GetDesktopWindow();

    GetWindowRect(hwnd_parent, &rw_parent);
    GetClientRect(hwnd_parent, &rc_parent);
    GetWindowRect(hWnd, &rw_self);

    xpos = rw_parent.left + (rc_parent.right + rw_self.left - rw_self.right) / 2;
    ypos = rw_parent.top + (rc_parent.bottom + rw_self.top - rw_self.bottom) / 2;

    SetWindowPos(hWnd, NULL, xpos, ypos, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void FullscreenWindow(HWND hWnd) {
    DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);

    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        // Store current window placement
        GetWindowPlacement(hWnd, &G.wpPrev);

        // Remove window decorations and maximize
        SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        // Restore previous window placement
        SetWindowLong(hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &G.wpPrev);
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

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

#define APPNAME "HELLO_WIN"

i32 APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nCmdShow) {
    G.memory     = VirtualAlloc(NULL, 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    G.screen_buf = VirtualAlloc(NULL, G.screen_size.w * G.screen_size.h * sizeof(u32),
                                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!ReloadUpdate())
        return 1;

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

    if (FALSE == RegisterClass(&wc))
        return 0;

    // create the browser
    HWND hwnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             G.screen_size.w, // CW_USEDEFAULT,
                             G.screen_size.h, // CW_USEDEFAULT,
                             0, 0, hInstance, 0);

    if (!hwnd)
        return 0;

    G.init();

    bool shutdown = false;
    MSG  msg;
    draw_rect((rect){0, 0, G.screen_size.w, G.screen_size.h}, rgb(255, 255, 255));
    while (!shutdown) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
            case WM_QUIT:
                shutdown = true;
                break;

            case WM_KEYDOWN:
                switch (msg.wParam) {
                case VK_ESCAPE:
                    DestroyWindow(hwnd);
                    break;

                case VK_F11:
                    FullscreenWindow(hwnd);
                    break;

                default:
                    break;
                }
                break;

            case WM_MOUSEMOVE:
                G.mousePos = (v2i){
                    .x = (i16)(msg.lParam & 0xFFFF),
                    .y = (i16)((msg.lParam >> 16) & 0xFFFF),
                };
                break;

            default:
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (G.update)
            G.update();

        col32 btn_color = col_point_rect(G.mousePos, (rect){8, 8, 59, 19}) ? rgb(155, 201, 142)
                                                                           : rgb(218, 166, 153);
        draw_rect((rect){8, 8, 59, 19}, btn_color);
        draw_text("Blabers!", 10, 10, rgb(20, 10, 10));
        draw_circle(100, 100, 25, rgb(122, 63, 63));
        CheckHotReload();

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
                .biWidth       = G.screen_size.w,
                .biHeight      = -G.screen_size.h, // Negative = top-down
                .biPlanes      = 1,
                .biBitCount    = 32,
                .biCompression = BI_RGB,
            };

            SetDIBitsToDevice(hdc, 0, 0, G.screen_size.w, G.screen_size.h, 0, 0, 0, G.screen_size.h,
                              G.screen_buf, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

            for (i32 i = 0; i < G.draw_count; i++) {
                DrawCmd next = G.draw_queue[i];

                RECT r = {
                    .left   = next.x,
                    .top    = next.y,
                    .right  = next.x + G.screen_size.w,
                    .bottom = next.y + G.screen_size.h,
                };

                SetTextColor(hdc, next.color);
                SetBkMode(hdc, TRANSPARENT);
                DrawText(hdc, next.text, -1, &r, DT_LEFT | DT_TOP);
                // DrawText(hdc, next.text, -1, &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            }

            ReleaseDC(hwnd, hdc);
        }

        G.draw_count = 0;
        Sleep(16);
    }

    return msg.wParam;
}