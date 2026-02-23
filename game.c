#include "base_win.c"

export Info game = {
    .name    = "Handmade Renderer",
    .version = "0.1.0",
};

struct Data {
    v3    camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    m3    obj_transform;
    Mesh *obj_mesh;
    v2i   tilemap_size;
    u8  **tilemap;
    q8    tile_size;
};

export void init() {
    *data = (Data){
        .obj_transform =
            {
                .pos   = {0, 0, Q8(2)},
                .rot   = {0},
                .scale = {Q8(1), Q8(1), Q8(1)},
            },
        .obj_mesh   = &cube,
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
            i32 map_x = x % data->tilemap_size.x;
            i32 map_y = y % data->tilemap_size.y;

            u8 tile_id = data->tilemap[map_y][map_x];
            draw_rect((rect){q8_mul(Q8(x), data->tile_size), q8_mul(Q8(y), data->tile_size),
                             data->tile_size, data->tile_size},
                      data->solid_tiles[tile_id]);
        }
    }

    // TODO(violeta): q8_clamp
    data->obj_transform.rot.y += q8_mul(Q8_PI, dt);
    while (data->obj_transform.rot.y > Q8_TAU)
        data->obj_transform.rot.y -= Q8_TAU;
    while (data->obj_transform.rot.y < 0)
        data->obj_transform.rot.y += Q8_TAU;

    v3 *obj_trans = (v3 *)alloc_temp(sizeof(v3) * 8);
    for (i32 i = 0; i < data->obj_mesh->verts_count; i++) {
        obj_trans[i] =
            v3_add(v3_add(v3_rotate_xz(v3_mul(data->obj_mesh->verts[i], data->obj_transform.scale),
                                       data->obj_transform.rot.y),
                          data->obj_transform.pos),
                   data->camera_pos);
    }

    draw_mesh(obj_trans, data->obj_mesh->verts_count, data->obj_mesh->edges,
              data->obj_mesh->edges_count, rgb(255, 255, 255));
}

export void quit() {}

#include "hot_reload.c"