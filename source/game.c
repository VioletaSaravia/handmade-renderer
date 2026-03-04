#include "engine.c" // TODO(violeta): Remove as dependency of game layer

#include "base.c"

export Info game = {
    .name    = "Handmade Renderer",
    .version = "0.3.0",
    .keybinds =
        {
            [A_UP]     = {K_UP, K_W},
            [A_DOWN]   = {K_DOWN, K_R},
            [A_LEFT]   = {K_LEFT, K_A},
            [A_RIGHT]  = {K_RIGHT, K_S},
            [A_ACCEPT] = {K_ENTER},
            [A_CANCEL] = {K_ESCAPE},
        },
};

#define ENTITY_MAX 2000

typedef i32 EntityID;

typedef struct {
    v3  pos;
    v3n look_at; // TODO(violeta)
} Camera;

struct Data {
    // Level
    Camera cam; // DO NOT MOVE :P
    handle level_mark;

    // GUI
    col32 fg, bg, text_light, text_dark;

    // Terrain
    col32 solid_tiles[4];
    v2i   tilemap_size;
    u8   *tilemap;
    q8    tile_size;

    // Entities
    EntityID *entities;
    i32       count, cap;
    m3       *e_transform;
    Mesh      e_mesh;
    Texture  *e_tex;
};

export void init() {
    init_default_texture();
    data->level_mark = arena_mark(&ctx()->perm);
    string cube_data = file_read("./assets/cube.obj", &ctx()->temp);
    Mesh   cube      = mesh_from_obj(&ctx()->perm, (char *)cube_data.text);

    *data = (Data){
        .cam        = {.pos = (v3){0, 0, Q8(3)}},
        .entities   = ALLOC_ARRAY(EntityID, ENTITY_MAX),
        .count      = 0,
        .cap        = ENTITY_MAX,
        .e_mesh     = cube,
        .e_tex      = &default_texture,
        .fg         = rgb(110, 124, 205),
        .bg         = rgb(51, 45, 116),
        .text_light = rgb(230, 240, 250),
        .text_dark  = rgb(20, 10, 10),
        .solid_tiles =
            {
                rgb(134, 180, 180),
                rgb(128, 0, 128),
                rgb(128, 128, 0),
                rgb(0, 0, 128),
            },
        .tile_size    = Q8(32),
        .tilemap_size = (v2i){64, 64},
    };

    data->e_transform = ALLOC_ARRAY(m3, ENTITY_MAX);

    for (i32 i = 0; i < ENTITY_MAX; i++) {
        data->e_transform[i]       = m3_id;
        data->e_transform[i].pos   = (v3){Q8((i % 5) - 2), 0, Q8((i / 5) - 2)};
        data->e_transform[i].scale = (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(1) >> 1};
    }

    data->e_transform[0]       = m3_id;
    data->e_transform[0].pos   = (v3){0, 0, Q8(1)};
    data->e_transform[0].scale = (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(1) >> 1};

    data->tilemap = ALLOC_ARRAY(u8, data->tilemap_size.y * data->tilemap_size.x);

    for (i32 y = 0; y < data->tilemap_size.y; y++) {
        for (i32 x = 0; x < data->tilemap_size.x; x++) {
            data->tilemap[y * data->tilemap_size.x + x] = (x + y) % 4;
        }
    }
}

export void update(q8 dt, i32 tid) {
    if (tid == MAIN) {
        if (GetAction(A_UP) >= KS_JUST_PRESSED) data->cam.pos.z -= dt;
        if (GetAction(A_DOWN) >= KS_JUST_PRESSED) data->cam.pos.z += dt;
        if (GetAction(A_LEFT) >= KS_JUST_PRESSED) data->cam.pos.x += dt;
        if (GetAction(A_RIGHT) >= KS_JUST_PRESSED) data->cam.pos.x -= dt;

        for (i32 y = 0; y < G->screen_size.h / q8_to_i32(data->tile_size); y++) {
            for (i32 x = 0; x < G->screen_size.w / q8_to_i32(data->tile_size); x++) {
                i32 map_x = x % data->tilemap_size.x;
                i32 map_y = y % data->tilemap_size.y;

                u8 tile_id = data->tilemap[map_y * data->tilemap_size.x + map_x];
                draw_rect((rect){q8_mul(Q8(x), data->tile_size), q8_mul(Q8(y), data->tile_size),
                                 data->tile_size, data->tile_size},
                          data->solid_tiles[tile_id]);
            }
        }

        for (i32 i = 0; i < ENTITY_MAX; i++) {
            data->e_transform[i].rot.y += q8_mul(Q8_PI, dt);
            while (data->e_transform[i].rot.y > Q8_TAU)
                data->e_transform[i].rot.y -= Q8_TAU;
            while (data->e_transform[i].rot.y < 0)
                data->e_transform[i].rot.y += Q8_TAU;

            draw_model(data->e_mesh, data->e_tex, data->e_transform[i]);
        }

        draw_text(string_format(&ctx()->temp, "Memory used: %d KB", ctx()->perm.used / 1024), 10,
                  10, data->text_light);

        // Edge case: passing default_texture as a literal doesn't work for this macro.
        draw_texture(default_texture, .pos = (v2i){200, 10}, .src = (i32rect){0, 0, 48, 48});
    }
}

export void quit() {}

#include "hot_reload.c"