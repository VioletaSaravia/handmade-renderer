#include "base_win.c"

export Info game = {
    .name    = "Template",
    .version = "0.1.0",
};

struct Data {
    u8 _;
};

export void init() {
    *data = (Data){
        ._ = 0,
    };
}

export void update(q8 dt) {}

export void quit() {}

#include "hot_reload.c"