#include "base_win.c"

export Info game = {
    .name    = "Software Renderer",
    .version = "0.1.0",
};

struct Data {
    v2i   camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    m3    transform;
    v2i   tilemap_size;
    u8  **tilemap;
    q8    tile_size;
};

export void init() {
    *data = (Data){
        .transform =
            {
                .pos   = {0, 0, Q8(2)},
                .rot   = {0},
                .scale = {Q8(1), Q8(1), Q8(1)},
            },
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

    data->tilemap = ALLOC_ARRAY(u8 *, data->tilemap_size.y);
    for (i32 i = 0; i < data->tilemap_size.y; i++) {
        data->tilemap[i] = ALLOC_ARRAY(u8, data->tilemap_size.x);
    }

    for (i32 y = 0; y < data->tilemap_size.y; y++) {
        for (i32 x = 0; x < data->tilemap_size.x; x++) {
            data->tilemap[y][x] = (x + y) % 4;
        }
    }
}

export void update(q8 dt) {
    for (i32 y = 0; y < G->screen_size.h / q8_to_i32(data->tile_size); y++) {
        for (i32 x = 0; x < G->screen_size.w / q8_to_i32(data->tile_size); x++) {
            i32 map_x = (data->camera_pos.x + x) % data->tilemap_size.x;
            i32 map_y = (data->camera_pos.y + y) % data->tilemap_size.y;

            u8 tile_id = data->tilemap[map_y][map_x];
            draw_rect((rect){q8_mul(Q8(x), data->tile_size), q8_mul(Q8(y), data->tile_size),
                             data->tile_size, data->tile_size},
                      data->solid_tiles[tile_id]);
        }
    }

    // TODO(violeta): q8_clamp
    data->transform.rot.y += q8_mul(Q8_PI, dt);
    while (data->transform.rot.y > Q8_TAU)
        data->transform.rot.y -= Q8_TAU;
    while (data->transform.rot.y < 0)
        data->transform.rot.y += Q8_TAU;

    v3 *obj_transformed = (v3 *)alloc_temp(sizeof(v3) * 8);
    for (i32 i = 0; i < cube.verts_count; i++) {
        obj_transformed[i] = v3_add(
            v3_rotate_xz(v3_mul(cube.verts[i], data->transform.scale), data->transform.rot.y),
            data->transform.pos);
    }

    draw_mesh(obj_transformed, cube.verts_count, cube.edges, cube.edges_count, rgb(255, 255, 255));
}

export void quit() {}

#include "hot_reload.c"