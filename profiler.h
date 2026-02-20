#pragma once

#include "base.h"

typedef struct {
    cstr label, file;
    i32  line;

    u64 iterations;
    u64 from, time_ex, time_inc;

    u64 bytesProcessed;
} Block;

#ifndef MAX_BLOCKS
#define MAX_BLOCKS 64
#endif

typedef struct {
    cstr name;
    bool ended;
    u64  start;

    Block blocks[MAX_BLOCKS];
    i32   blocks_len;
    u64   queue[MAX_BLOCKS];
    i32   queue_len;
} Profiler;

typedef struct {
    u64 time, bytes, pageFaults;
} RepBlock;

typedef struct {
    cstr     name;
    RepBlock first, min, max, avg, current;
    u64      repeats, maxRepeats;

    // static RepProfiler New(cstr name, u64 maxRepeats = 100);
    // void               BeginRep();
    // void               AddBytes(u64 bytes);
    // void               EndRep();
    // ~RepProfiler();
} RepProfiler;