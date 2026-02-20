#include "base_win.c"

export Info game = {
    .name    = "Handmade Renderer",
    .version = "0.1.0",
};

struct Data {
    v2i   camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    v3    obj[8];
    i32   obj_count;
    q8    angle;
    v2i   tilemap_size;
    u8  **tilemap;
    q8    tile_size;
};

export void init() {
    *data = (Data){
        .obj =
            {
                (v3){Q8(1) >> 1, Q8(-1) >> 1, Q8(1)},
                (v3){Q8(-1) >> 1, Q8(-1) >> 1, Q8(1)},
                (v3){Q8(-1) >> 1, Q8(1) >> 1, Q8(1)},
                (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(1)},
                (v3){Q8(1) >> 1, Q8(-1) >> 1, Q8(2)},
                (v3){Q8(-1) >> 1, Q8(-1) >> 1, Q8(2)},
                (v3){Q8(-1) >> 1, Q8(1) >> 1, Q8(2)},
                (v3){Q8(1) >> 1, Q8(1) >> 1, Q8(2)},
            },
        .obj_count  = 8,
        .fg         = rgb(110, 124, 205),
        .bg         = rgb(51, 45, 116),
        .text_light = rgb(230, 240, 250),
        .text_dark  = rgb(20, 10, 10),
        .solid_tiles =
            {
                rgb(0, 128, 128),
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

    data->angle += q8_mul(Q8_PI, dt);
    while (data->angle > Q8_TAU)
        data->angle -= Q8_TAU;
    while (data->angle < 0)
        data->angle += Q8_TAU;

    // Compute the center of the cube
    v3 center = {0, 0, 0};
    for (i32 i = 0; i < data->obj_count; i++) {
        center.x += data->obj[i].x;
        center.y += data->obj[i].y;
        center.z += data->obj[i].z;
    }
    center.x /= data->obj_count;
    center.y /= data->obj_count;
    center.z /= data->obj_count;

    static v3 obj_rotated[8];
    for (i32 i = 0; i < data->obj_count; i++) {
        // Move vertex to local space (relative to cube center)
        v3 local = {
            data->obj[i].x - center.x,
            data->obj[i].y - center.y,
            data->obj[i].z - center.z,
        };
        // Rotate around origin (which is now the cube's center)
        v3 rotated = v3_rotate_xz(local, data->angle);
        // Move back to world space
        obj_rotated[i] = (v3){
            rotated.x + center.x,
            rotated.y + center.y,
            rotated.z + center.z,
        };
    }

    draw_mesh(obj_rotated, data->obj_count, rgb(255, 255, 255));
}

export void quit() {}

#include "hot_reload.c"