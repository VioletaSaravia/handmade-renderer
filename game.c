
#include "base_win.c"

struct Data {
    v2i   camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    u8    tilemap[64][64];
    i32   tile_size;
};

export void init() {
    *data = (Data){.fg         = rgb(110, 124, 205),
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
                   .tile_size = 32};

    for (i32 y = 0; y < 64; y++) {
        for (i32 x = 0; x < 64; x++) {
            data->tilemap[y][x] = (x + y) % 4;
        }
    }
}

export void update() {
    for (i32 y = data->camera_pos.y; y < data->camera_pos.y + G->screen_size.h / data->tile_size;
         y++) {
        for (i32 x = data->camera_pos.x;
             x < data->camera_pos.x + G->screen_size.w / data->tile_size; x++) {
            u8 tile_id = data->tilemap[y % 64][x % 64];
            draw_rect((rect){(x - data->camera_pos.x) * data->tile_size,
                             (y - data->camera_pos.y) * data->tile_size, data->tile_size,
                             data->tile_size},
                      data->solid_tiles[tile_id]);
        }
    }

    col32 btn_color = col_point_rect(G->mouse_pos, (rect){8, 8, 59, 19}) ? data->fg : data->bg;
    draw_rect((rect){8, 8, 59, 19}, btn_color);
    draw_text("Blabers!", 10, 10, data->text_light);
}

export void quit() {}

#include "hot_reload.c"