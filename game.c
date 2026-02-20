#include "base_win.c"

export Info game = {
    .name    = "Handmade Renderer",
    .version = "0.1.0",
};

struct Data {
    v2i   camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    v2i   tilemap_size;
    u8  **tilemap;
    q8    tile_size;
};

export void init() {
    *data = (Data){
        .fg         = rgb(110, 124, 205),
        .bg         = rgb(51, 45, 116),
        .text_light = rgb(230, 240, 250),
        .text_dark  = rgb(20, 10, 10),
        .solid_tiles =
            {
                rgb(0, 255, 255),
                rgb(255, 0, 255),
                rgb(255, 255, 0),
                rgb(0, 0, 255),
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

    gui_button("test", Q8(10), Q8(10));
}

export void quit() {}

#include "hot_reload.c"