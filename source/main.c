#define ENGINE_IMPL
#include "engine.c"
#include "base.c"
#include "profiler.c"

void tcc_err(void *opaque, const char *msg) { printf(msg); }

static int draw_cmd_cmp(const void *a, const void *b) {
    const DrawCmd *da = (const DrawCmd *)a;
    const DrawCmd *db = (const DrawCmd *)b;

    v3 cam = *(v3 *)G->game_memory;
    q8 za  = v3_add(da->transform.pos, cam).z;
    q8 zb  = v3_add(db->transform.pos, cam).z;

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

    GetLastWriteTime(GAME_ENTRYPOINT, &result.last_write);

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

static void hot_reload() {
    FILETIME new_write = {0};
    if (!GetLastWriteTime(GAME_ENTRYPOINT, &new_write)) return;
    if (CompareFileTime(&new_write, &G->game.last_write) == 0) return;

    GameDLL new_dll = load_dll();
    if (!new_dll.tcc) return;

    tcc_delete(G->game.tcc);
    G->game = new_dll;
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
            .draw_size      = 1024,
            .metrics        = metrics_init(),
            .system_info    = systeminfo_init(),
            .profiler       = profiler_new("Handmade Renderer"),
        };
        ctx()->temp = arena_new(MB(8), &ctx()->perm);
    }

    BLOCK_BEGIN("init");
    G->draw_queue = ALLOC_ARRAY(DrawCmd, G->draw_size);

    QueryPerformanceFrequency(&G->freq);
    const f32 target_dt  = 1.0f / 60.0f;
    f32       dt         = target_dt;
    f64       next_frame = now_seconds();

    G->game = load_dll();
    if (!G->game.tcc) return 1;

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

    if (!RegisterClass(&wc)) return 0;

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT  wr    = {0, 0, G->screen_size.w, G->screen_size.h};
    AdjustWindowRect(&wr, style, false);

    cstr window_name =
        string_format(&G->ctx.perm, "%s %s", G->game.info->name, G->game.info->version);
    G->hwnd = CreateWindow(G->game.info->name, window_name, style, CW_USEDEFAULT, CW_USEDEFAULT,
                           wr.right - wr.left, wr.bottom - wr.top, 0, 0, hInstance, 0);
    if (!G->hwnd) return 0;

    if (G->game.init) G->game.init();
    BLOCK_END();

    RepProfiler rep = repprofiler_new("game loop", 1000);
    while (!G->shutdown) {
        hot_reload();
        rep_begin(&rep);
        f64 frame_start = now_seconds();

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
                case VK_ESCAPE: DestroyWindow(G->hwnd); break;
                case VK_F5: G->game.init(); break;
                case VK_F11: FullscreenWindow(G->hwnd); break;

                case VK_UP: G->keys[K_UP] = KS_JUST_PRESSED; break;
                case VK_DOWN: G->keys[K_DOWN] = KS_JUST_PRESSED; break;
                case VK_LEFT: G->keys[K_LEFT] = KS_JUST_PRESSED; break;
                case VK_RIGHT: G->keys[K_RIGHT] = KS_JUST_PRESSED; break;
                case 'W': G->keys[K_W] = KS_JUST_PRESSED; break;
                case 'A': G->keys[K_A] = KS_JUST_PRESSED; break;
                case 'R': G->keys[K_R] = KS_JUST_PRESSED; break;
                case 'S': G->keys[K_S] = KS_JUST_PRESSED; break;

                default: break;
                }
                break;

            case WM_KEYUP:
                switch (G->msg.wParam) {
                case VK_UP: G->keys[K_UP] = KS_JUST_RELEASED; break;
                case VK_DOWN: G->keys[K_DOWN] = KS_JUST_RELEASED; break;
                case VK_LEFT: G->keys[K_LEFT] = KS_JUST_RELEASED; break;
                case VK_RIGHT: G->keys[K_RIGHT] = KS_JUST_RELEASED; break;
                case 'W': G->keys[K_W] = KS_JUST_RELEASED; break;
                case 'A': G->keys[K_A] = KS_JUST_RELEASED; break;
                case 'R': G->keys[K_R] = KS_JUST_RELEASED; break;
                case 'S': G->keys[K_S] = KS_JUST_RELEASED; break;

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

        G->game.update((q8)(dt * 256.0f));

        {
            HDC  hdc = GetDC(G->hwnd);
            RECT rc  = {0};
            GetClientRect(G->hwnd, &rc);

            i32 client_w = rc.right - rc.left;
            i32 client_h = rc.bottom - rc.top;

            HDC     mem_dc  = CreateCompatibleDC(hdc);
            HBITMAP mem_bmp = CreateCompatibleBitmap(hdc, client_w, client_h);
            HBITMAP old_bmp = SelectObject(mem_dc, mem_bmp);

            qsort(G->draw_queue, G->draw_count, sizeof(DrawCmd), draw_cmd_cmp);
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
                        n    = v3_add(n, *(v3 *)G->game_memory);

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
                    v2i *screen_verts  = (v2i *)alloc_temp(sizeof(v2i) * next.count);
                    q8  *transformed_z = (q8 *)alloc_temp(sizeof(q8) * next.count);
                    for (i32 v = 0; v < next.count; v++) {
                        v3 n = v3_mul(next.vertices[v], next.transform.scale);
                        n    = v3_rotate_xz(n, next.transform.rot.y);
                        n    = v3_add(n, next.transform.pos);
                        n    = v3_add(n, *(v3 *)G->game_memory);

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
                            if (x_coord >= 0 && x_coord < G->screen_size.w && y_coord >= 0 &&
                                y_coord < G->screen_size.h) {
                                i32 coord            = y_coord * G->screen_size.w + x_coord;
                                G->screen_buf[coord] = next.color;
                            }
                        }
                    }
                    break;
                }
                default: break;
                }
            }

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
        }

        {
            next_frame += target_dt;

            f64 remaining = next_frame - now_seconds();
            if (remaining > 0.0) {
                DWORD sleep_ms = (DWORD)(remaining * 1000.0);
                if (sleep_ms > 0) Sleep(sleep_ms);
            }

            arena_reset(&ctx()->temp, 0);
            dt = now_seconds() - frame_start;
        }
        rep_end(&rep);
    }

    repprofiler_print(&rep);
    if (G->game.quit) G->game.quit();
    profiler_end();
    return G->msg.wParam;
}