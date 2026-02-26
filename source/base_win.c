#include "base_win.h"

typedef struct Data Data;
#ifndef ENGINE_IMPL
import extern Data *data;
#endif

#ifdef ENGINE_IMPL
global EngineData *G;
#else
import extern EngineData *G;
#endif

EngineData *EG() { return G; }
Context    *ctx() { return &G->ctx; }

void draw_wireframe(v3 *verts, i32 verts_count, v2i *edges, i32 edges_count, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t           = DCT_MESH_WIREFRAME,
                                               .vertices    = verts,
                                               .count       = verts_count,
                                               .edges       = edges,
                                               .edges_count = edges_count,
                                               .color       = color};
}

void draw_mesh(v3 *verts, i32 verts_count, v2i *edges, i32 edges_count, col32 color) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t           = DCT_MESH_SOLID,
                                               .vertices    = verts,
                                               .count       = verts_count,
                                               .edges       = edges,
                                               .edges_count = edges_count,
                                               .color       = color};
}

void draw_model(Mesh mesh, Texture *tex, m3 transform) {
    if (G->draw_count == G->draw_size) return;
    G->draw_queue[G->draw_count++] = (DrawCmd){.t           = DCT_MODEL,
                                               .vertices    = mesh.verts,
                                               .count       = mesh.verts_count,
                                               .faces       = mesh.faces,
                                               .faces_count = mesh.faces_count,
                                               .tex         = tex->data,
                                               .tex_size    = tex->size,
                                               .transform   = transform};
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

internal inline void _mm_pause() {
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

#define THREAD_COUNT 8
void thread_barrier() {
    persist volatile i64 barrier_count      = 0;
    persist volatile i64 barrier_generation = 0;
    persist const i32    barrier_total      = THREAD_COUNT;

    i64 gen       = read_acquire(&barrier_generation);
    i64 new_count = InterlockedIncrement(&barrier_count);

    if (new_count == barrier_total) {
        InterlockedExchange(&barrier_count, 0);
        InterlockedIncrement(&barrier_generation);
    } else {
        while (read_acquire(&barrier_generation) == gen) {
            _mm_pause();
        }
    }
}

KeyState GetAction(Action a) {
    for (i32 i = 0; i < 2; i++) {
        KeyState state = EG()->keys[Keybinds[a][i]];
        if (state != KS_RELEASED) return state;
    }
    return KS_RELEASED;
};

string file_read(char *path, Arena *a) {
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        ERR("Failed to open file %s. Error code: %lu", path, GetLastError());
        return (string){0};
    }

    DWORD file_size = GetFileSize(file, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        ERR("Failed to get file size for %s. Error code: %lu", path, GetLastError());
        CloseHandle(file);
        return (string){0};
    }

    char *buffer     = alloc((u64)file_size, a);
    DWORD bytes_read = 0;
    if (!ReadFile(file, buffer, file_size, &bytes_read, NULL) || bytes_read != file_size) {
        ERR("Failed to read file %s. Error code: %lu", path, GetLastError());
        CloseHandle(file);
        return (string){0};
    }

    CloseHandle(file);
    return (string){.text = buffer, .len = file_size};
}