
#include "base_win.c"

struct Data {
    v2    camera_pos;
    col32 fg, bg, text_light, text_dark;
    col32 solid_tiles[4];
    u8    tilemap[64][64];
    q8    tile_size;
    rect  btn_rect;
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
        .tile_size = Q8(32),
        .btn_rect  = (rect){Q8(8), Q8(8), Q8(59), Q8(19)},
    };

    for (i32 y = 0; y < 64; y++) {
        for (i32 x = 0; x < 64; x++) {
            data->tilemap[y][x] = (x + y) % 4;
        }
    }
}

export void update(q8 dt) {
    data->btn_rect.x += 10 * dt;

    for (q8 y = data->camera_pos.y;
         y <= data->camera_pos.y + q8_from_i32(G->screen_size.h) / data->tile_size; y++) {
        for (q8 x = data->camera_pos.x;
             x <= data->camera_pos.x + q8_from_i32(G->screen_size.w) / data->tile_size; x++) {

            u8 tile_id = data->tilemap[q8_to_i32(y) % 64][q8_to_i32(x) % 64];
            draw_rect((rect){(x - data->camera_pos.x) * data->tile_size,
                             (y - data->camera_pos.y) * data->tile_size, data->tile_size,
                             data->tile_size},
                      data->solid_tiles[tile_id]);
        }
    }

    col32 btn_color = col_point_rect(G->mouse_pos, data->btn_rect) ? data->fg : data->bg;
    draw_rect(data->btn_rect, btn_color);
    draw_text("Blabers!", 10, 10, data->text_light);
}

export void quit() {}

#include "hot_reload.c"