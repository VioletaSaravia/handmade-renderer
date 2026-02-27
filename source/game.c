#include "engine.c" // TODO(violeta): Remove as dependency of game layer

#include "base.c"

export Info game = {
    .name    = "Handmade Renderer",
    .version = "0.2.1",
    .keybinds =
        {
            [A_FULLSCREEN] = {K_F11},
            [A_QUIT]       = {K_F4 | M_SHIFT},
            [A_RESET]      = {K_F5},
            [A_UP]         = {K_UP, K_W},
            [A_DOWN]       = {K_DOWN, K_R},
            [A_LEFT]       = {K_LEFT, K_A},
            [A_RIGHT]      = {K_RIGHT, K_S},
            [A_ACCEPT]     = {K_ENTER},
            [A_CANCEL]     = {K_ESCAPE},
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
    m3       *obj_transform;
    Mesh      obj_mesh;
    Texture  *obj_tex;
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
        .obj_mesh   = cube,
        .obj_tex    = &default_texture,
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

    data->obj_transform = ALLOC_ARRAY(m3, ENTITY_MAX);

    for (i32 i = 0; i < ENTITY_MAX; i++) {
        data->obj_transform[i]       = m3_id;
        data->obj_transform[i].pos   = (v3){Q8((i % 5) - 2), 0, Q8((i / 5) - 2)};
        data->obj_transform[i].scale = (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(1) >> 1};
    }

    data->obj_transform[0]       = m3_id;
    data->obj_transform[0].pos   = (v3){0, 0, Q8(1)};
    data->obj_transform[0].scale = (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(1) >> 1};

    data->tilemap = ALLOC_ARRAY(u8, data->tilemap_size.y * data->tilemap_size.x);

    for (i32 y = 0; y < data->tilemap_size.y; y++) {
        for (i32 x = 0; x < data->tilemap_size.x; x++) {
            data->tilemap[y * data->tilemap_size.x + x] = (x + y) % 4;
        }
    }
}

export void update(q8 dt) {
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
        data->obj_transform[i].rot.y += q8_mul(Q8_PI, dt);
        while (data->obj_transform[i].rot.y > Q8_TAU)
            data->obj_transform[i].rot.y -= Q8_TAU;
        while (data->obj_transform[i].rot.y < 0)
            data->obj_transform[i].rot.y += Q8_TAU;

        draw_model(data->obj_mesh, data->obj_tex, data->obj_transform[i]);
    }

    draw_text(string_format(&ctx()->temp, "Total memory used: %d KB", ctx()->perm.used / 1024), 10,
              10, data->text_light);
    draw_text(string_format(&ctx()->temp, "Game memory used: %d KB",
                            (ctx()->perm.used - data->level_mark + sizeof(Data)) / 1024),
              10, 30, data->text_light);
}

export void quit() {}

#include "hot_reload.c"