#define ENGINE_IMPL
#include "base.c"
#include "engine.c"
#include "profiler.c"

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

void main_update(void *thread_ctx) {
    ThreadCtx *th = (ThreadCtx *)thread_ctx;
    if (th->id == MAIN) LOOP_PROFILER();

    while (!G->shutdown) {
        if (th->id == MAIN) LOOP_BEGIN();
        f64 frame_start = now_seconds();

        // Input
        if (th->id == MAIN) {
            hot_reload();

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

            if (GetAction(A_FULLSCREEN) == KS_JUST_PRESSED) FullscreenWindow(G->hwnd);
            if (GetAction(A_QUIT) == KS_JUST_PRESSED) DestroyWindow(G->hwnd);
            if (GetAction(A_RESET) == KS_JUST_PRESSED) G->game.init();
        }

        thread_barrier();
        if (th->id == MAIN) LOOP_BLOCK("Game Update");
        G->game.update((q8)(G->dt * 256.0f), th->id);
        if (th->id == MAIN) LOOP_BLOCK_END();
        thread_barrier();

        // Rendering
        if (th->id == MAIN) {
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

                    v2 ratio = (v2){q8_div64(Q8(in_size.x), Q8(out_size.x)),
                                    q8_div64(Q8(in_size.y), Q8(out_size.y))};

                    for (i32 y = 0; y < out_size.y; y++) {
                        for (i32 x = 0; x < out_size.x; x++) {
                            v2i screen_pos = v2i_add(next.params.pos, (v2i){x, y});

                            if (screen_pos.x < 0 || screen_pos.x >= G->screen_size.w ||
                                screen_pos.y < 0 || screen_pos.y >= G->screen_size.h) {
                                continue;
                            }

                            i32 screen_idx  = screen_pos.y * G->screen_size.w + screen_pos.x;
                            v2i tex_coord   = (v2i){q8_to_i32(q8_mul64(Q8(x), ratio.x)),
                                                    q8_to_i32(q8_mul64(Q8(y), ratio.y))};
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
        if (th->id == MAIN) {
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

        thread_barrier();
        if (th->id == MAIN) LOOP_END();
    }
}

void engine_init(HINSTANCE hInstance) {
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
}

i32 APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nCmdShow) {
    engine_init(hInstance);

    BLOCK_BEGIN("game_init");
    if (G->game.init) G->game.init();
    BLOCK_END();

    BLOCK_BEGIN("thread_init");
    for (u32 i = 1; i < THREAD_COUNT; i++) {
        ThreadCtx *ctx = &EG()->thread_ctx[i];
        ctx->id        = i;
        ctx->thread    = _beginthreadex(NULL, 0, main_update, (void *)ctx, 0, &ctx->os_id);
        ctx->temp      = arena_new(MB(1), &EG()->ctx.perm);
        INFO("Thread %02u started", i);
    }
    EG()->thread_ctx[0] = (ThreadCtx){
        .id   = 0,
        .temp = arena_new(MB(1), &EG()->ctx.perm),
    };
    BLOCK_END();
    profiler_end();

    INFO("Main Thread started");
    main_update(&G->thread_ctx[0]);
    INFO("Main Thread ended");

    for (u32 i = 1; i < THREAD_COUNT; i++) {
        ThreadCtx *ctx = &EG()->thread_ctx[i];
        if (!ctx->thread) continue;

        WaitForSingleObject(ctx->thread, INFINITE);

        u32 exit_code = 0;
        GetExitCodeThread(ctx->thread, (LPDWORD)&exit_code);

        CloseHandle(ctx->thread);
        INFO("Thread %02u ended", i);
    }

    if (G->game.quit) G->game.quit();

    return G->msg.wParam;
}