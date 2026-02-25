#pragma once

#include "base.h"
#include "base_win.h"

typedef struct {
    cstr label, file;
    i32  line;

    u64 iterations;
    u64 from, time_ex, time_inc;

    u64 bytesProcessed;
} Block;

#ifndef MAX_BLOCKS
#define MAX_BLOCKS 128
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
} RepProfiler;

RepProfiler repprofiler_new(cstr name, u64 maxRepeats);
void        rep_begin(RepProfiler *p);
void        rep_add_bytes(RepProfiler *p, u64 bytes);
void        rep_end(RepProfiler *p);
void        repprofiler_print(RepProfiler *p);

typedef struct {
    cstr name;
    u64  start;

    Block blocks[MAX_BLOCKS];
    i32   blocks_len;
    u64   queue[MAX_BLOCKS];
    i32   queue_len;
} LoopProfiler;

LoopProfiler loopprofiler_new(cstr name);
void loop_block_begin(LoopProfiler *p, u64 id, cstr label, cstr file, i32 line, u64 bytesProcessed);
void loop_block_add_bytes(LoopProfiler *p, u64 bytes);
void loop_block_end(LoopProfiler *p);
void loop_begin(LoopProfiler *p);
void loop_end(LoopProfiler *p);