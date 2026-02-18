
#include "base_win.c"

struct Data {
    u8 _;
};

export void init() { *data = (Data){0}; }

export void update() {}

export void quit() {}

#include "hot_reload.c"