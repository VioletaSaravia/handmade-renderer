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

#ifndef DISABLE_LOOP_PROFILER

#define LOOP_PROFILER() EG()->loop_profiler = loopprofiler_new(__func__)
#define LOOP_BLOCK(id, label, bytes) \
    loop_block_begin(&EG()->loop_profiler, id, label, __FILE__, __LINE__, bytes)
#define LOOP_BLOCK_END() loop_block_end(&EG()->loop_profiler)
#define LOOP_BLOCK_BYTES(bytes) loop_block_add_bytes(&EG()->loop_profiler, bytes)
#define LOOP_BEGIN() loop_begin(&EG()->loop_profiler)
#define LOOP_END() loop_end(&EG()->loop_profiler)

#else

#define LOOP_BLOCK(...)
#define LOOP_BLOCK_END(...)
#define LOOP_BLOCK_BYTES(...)
#define LOOP_PROFILER()
#define LOOP_BEGIN(...)
#define LOOP_END(...)

#endif