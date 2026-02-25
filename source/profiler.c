#include "profiler.h"
#include <winnt.h>

Profiler profiler_new(cstr name) {
    Profiler result = {
        .name  = name,
        .ended = false,
        .start = 0,
    };
    LARGE_INTEGER perfCounter = {0};
    QueryPerformanceCounter(&perfCounter);
    result.start = perfCounter.QuadPart;
    return result;
}

void block_begin(u64 id, cstr label, cstr file, i32 line, u64 bytesProcessed) {
    Profiler *p = &EG()->profiler;
    if (id >= MAX_BLOCKS) {
        return;
    }

    Block        *m    = &p->blocks[id];
    LARGE_INTEGER time = {0};
    QueryPerformanceCounter(&time);

    if (p->queue_len > 0) {
        Block *prev = &p->blocks[p->queue[p->queue_len - 1]];
        prev->time_ex += time.QuadPart - prev->from;
        prev->time_inc += time.QuadPart - prev->from;
    }

    m->from  = time.QuadPart;
    m->label = label;
    m->file  = file;
    m->line  = line;
    m->bytesProcessed += bytesProcessed;

    p->queue[p->queue_len++] = id;

    m->iterations++;

    return;
}

void block_bytes(u64 bytes) {
    EG()->profiler.blocks[EG()->profiler.queue[EG()->profiler.queue_len - 1]].bytesProcessed +=
        bytes;
}

void block_end() {
    LARGE_INTEGER now = {0};
    QueryPerformanceCounter(&now);

    Profiler *p = &EG()->profiler;
    Block    *m = &p->blocks[p->queue[--p->queue_len]];
    m->time_ex += now.QuadPart - m->from;
    m->time_inc += now.QuadPart - m->from;

    if (p->queue_len > 0) {
        Block *prev = &p->blocks[p->queue[p->queue_len - 1]];
        prev->from  = now.QuadPart;
        prev->time_inc += now.QuadPart - m->from;
    }
}

static f64 to_gb(f64 bytes) { return bytes / 1024.0 / 1024.0 / 1024.0; }

void profiler_end() {
    Profiler *p = &EG()->profiler;
    if (p->start == 0) return;
    if (p->ended) return;

    p->ended = true;

    LARGE_INTEGER perfCounter = {0}, perfFreq = {0};
    QueryPerformanceCounter(&perfCounter);
    QueryPerformanceFrequency(&perfFreq);

    f64 totalTime = (f64)(perfCounter.QuadPart - p->start) / (f64)(perfFreq.QuadPart);

    INFO("Finished %s in %.6f seconds", p->name, totalTime);
    printf(" %-24s \t| %-25s \t| %-25s \t| %-12s\n", "Name[n]", "Time (Ex)", "Time (Inc)",
           "Bandwidth");
    printf("-----------------------------------------------------------------------------------"
           "--------------------"
           "--------\n");

    for (u64 i = 1; i < MAX_BLOCKS; i++) {
        Block next = p->blocks[i];
        if (next.iterations == 0) continue;

        f64 nextTimeEx  = ((f64)(next.time_ex) / (f64)(perfFreq.QuadPart));
        f64 nextTimeInc = ((f64)(next.time_inc) / (f64)(perfFreq.QuadPart));
        if (next.bytesProcessed == 0) {
            printf(" %-20s [%llu] \t| %.5f secs\t(%.2f%%) \t| %.5f secs\t(%.2f%%) \t|\n",
                   next.label, next.iterations, nextTimeEx, (nextTimeEx / totalTime) * 100,
                   nextTimeInc, (nextTimeInc / totalTime) * 100);
        } else {
            printf(" %-20s [%llu] \t| %.5f secs\t(%.2f%%) \t| %.5f secs\t(%.2f%%) \t| %.3f GB/s\n",
                   next.label, next.iterations, nextTimeEx, (nextTimeEx / totalTime) * 100,
                   nextTimeInc, (nextTimeInc / totalTime) * 100,
                   to_gb((f64)(next.bytesProcessed) / nextTimeEx));
        }
    }
}

i32 by_time(const void *from, const void *to) {
    return ((RepBlock *)(from))->time - ((RepBlock *)(to))->time;
}

i32 by_bytes(const void *from, const void *to) {
    return ((RepBlock *)(from))->bytes - ((RepBlock *)(to))->bytes;
}

i32 by_page_faults(const void *from, const void *to) {
    return ((RepBlock *)(from))->pageFaults - ((RepBlock *)(to))->pageFaults;
}

RepProfiler repprofiler_new(cstr name, u64 maxRepeats) {
    return (RepProfiler){
        .name       = name,
        .maxRepeats = maxRepeats,
    };
}

void rep_begin(RepProfiler *p) {
    LARGE_INTEGER perfCounter = {0};
    QueryPerformanceCounter(&perfCounter);
    p->current = (RepBlock){
        .time       = (u64)(perfCounter.QuadPart),
        .bytes      = 0,
        .pageFaults = get_page_fault_count(EG()->metrics),
    };
}

void rep_add_bytes(RepProfiler *p, u64 bytes) { p->current.bytes += bytes; }

void rep_end(RepProfiler *p) {
    LARGE_INTEGER perfCounter = {0};
    QueryPerformanceCounter(&perfCounter);
    p->current.time       = perfCounter.QuadPart - p->current.time;
    p->current.pageFaults = get_page_fault_count(EG()->metrics) - p->current.pageFaults;

    if (p->current.time < p->min.time || p->min.time == 0) {
        p->min = p->current;
    }

    if (p->current.time >= p->max.time) {
        p->max = p->current;
    }

    p->avg.bytes += p->current.bytes;
    p->avg.time += p->current.time;
    p->avg.pageFaults += p->current.pageFaults;

    if (p->repeats == 0) p->first = p->current;

    p->repeats++;
}

void repprofiler_print(RepProfiler *p) {
    INFO("Finished %s after %llu repeats.", p->name, p->repeats);

    // FIRST
    LARGE_INTEGER perfFreq = {0};
    QueryPerformanceFrequency(&perfFreq);

    f64 firstTime = (f64)(p->first.time) / (f64)(perfFreq.QuadPart);
    printf("\t> Initial: \t%.3f ms\t%.3f GB/s\t%llu pf\n", firstTime * 1000.0,
           to_gb((f64)(p->first.bytes) / firstTime), p->first.pageFaults);

    // MIN
    f64 minTime = (f64)(p->min.time) / (f64)(perfFreq.QuadPart);
    printf("\t> Fastest: \t%.3f ms\t%.3f GB/s\t%llu pf\n", minTime * 1000.0,
           to_gb((f64)(p->min.bytes) / minTime), p->min.pageFaults);

    // MAX
    f64 maxTime = (f64)(p->max.time) / (f64)(perfFreq.QuadPart);
    printf("\t> Slowest: \t%.3f ms\t%.3f GB/s\t%llu pf\n", maxTime * 1000.0,
           to_gb((f64)(p->max.bytes) / maxTime), p->max.pageFaults);

    // AVERAGE
    f64 avgBytes  = (f64)(p->avg.bytes) / (f64)(p->repeats);
    f64 avgFaults = (f64)(p->avg.pageFaults) / (f64)(p->repeats);
    f64 avgTime   = (f64)(p->avg.time) / (f64)(p->repeats);
    avgTime /= (f64)(perfFreq.QuadPart);

    printf("\t> Average: \t%.3f ms\t%.3f GB/s\t%.2f pf\n", avgTime * 1000.0,
           to_gb((f64)(avgBytes) / avgTime), avgFaults);
}

#ifndef DISABLE_PROFILER

#define BLOCK_BEGIN(name) block_begin(__COUNTER__ + 1, name, __FILE__, __LINE__, 0)
#define BLOCK_END() block_end()
#define PROFILE(name, code)                                    \
    block_begin(__COUNTER__ + 1, name, __FILE__, __LINE__, 0); \
    code;                                                      \
    block_end();

#define REPETITION_PROFILE(name, count)                        \
    do {                                                       \
        RepProfiler _profiler_ = repprofiler_new(name, count); \
        while (_profiler_.repeats < _profiler_.maxRepeats) {   \
            rep_begin(&_profiler_);

#define REPETITION_END()  \
    rep_end(&_profiler_); \
    }                     \
    }                     \
    while (0)             \
        ;

#else

#define BLOCK_BEGIN(...)
#define BLOCK_END(...)
#define PROFILE(name, code) code

#define REPETITION_PROFILE(...)
#define REPETITION_END(...)

#endif

LoopProfiler loopprofiler_new(cstr name) {
    LARGE_INTEGER perfCounter = {0};
    QueryPerformanceCounter(&perfCounter);
    return (LoopProfiler){
        .name  = name,
        .start = perfCounter.QuadPart,
    };
}

void loop_begin(LoopProfiler *p) {
    LARGE_INTEGER perfCounter = {0};
    QueryPerformanceCounter(&perfCounter);
    p->start = perfCounter.QuadPart;

    loop_block_begin(p, 0, "Loop", __FILE__, __LINE__, 0);
}
void loop_end(LoopProfiler *p) {
    loop_block_end(p);

    LARGE_INTEGER perfCounter = {0}, perfFreq = {0};
    QueryPerformanceCounter(&perfCounter);
    QueryPerformanceFrequency(&perfFreq);

    f64 totalTime = (f64)(perfCounter.QuadPart - p->start) / (f64)(perfFreq.QuadPart);

    INFO("Finished %s in %.6f seconds", p->name, totalTime);
    printf(" %-24s \t| %-25s \t| %-25s \t| %-12s\n", "Name[n]", "Time (Ex)", "Time (Inc)",
           "Bandwidth");
    printf("-----------------------------------------------------------------------------------"
           "--------------------"
           "--------\n");
}

void loop_block_begin(LoopProfiler *p, u64 id, cstr label, cstr file, i32 line,
                      u64 bytesProcessed) {
    Profiler *p = &EG()->profiler;
    if (id >= MAX_BLOCKS) {
        return;
    }

    Block        *m    = &p->blocks[id];
    LARGE_INTEGER time = {0};
    QueryPerformanceCounter(&time);

    if (p->queue_len > 0) {
        Block *prev = &p->blocks[p->queue[p->queue_len - 1]];
        prev->time_ex += time.QuadPart - prev->from;
        prev->time_inc += time.QuadPart - prev->from;
    }

    m->from  = time.QuadPart;
    m->label = label;
    m->file  = file;
    m->line  = line;
    m->bytesProcessed += bytesProcessed;

    p->queue[p->queue_len++] = id;

    m->iterations++;

    return;
}

void loop_block_add_bytes(LoopProfiler *p, u64 bytes) {}

void loop_block_end(LoopProfiler *p) {
    LARGE_INTEGER now = {0};
    QueryPerformanceCounter(&now);

    Profiler *p = &EG()->profiler;
    Block    *m = &p->blocks[p->queue[--p->queue_len]];
    m->time_ex += now.QuadPart - m->from;
    m->time_inc += now.QuadPart - m->from;

    if (p->queue_len > 0) {
        Block *prev = &p->blocks[p->queue[p->queue_len - 1]];
        prev->from  = now.QuadPart;
        prev->time_inc += now.QuadPart - m->from;
    }
}