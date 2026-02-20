#include "profiler.h"

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

void BeginBlock(u64 id, cstr label, cstr file, i32 line, u64 bytesProcessed) {
    Profiler *p = &G->profiler;
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

void AddBytes(u64 bytes) {
    G->profiler.blocks[G->profiler.queue[G->profiler.queue_len - 1]].bytesProcessed += bytes;
}

void EndBlock() {
    LARGE_INTEGER now = {0};
    QueryPerformanceCounter(&now);

    Profiler *p = &G->profiler;
    Block    *m = &p->blocks[p->queue[--p->queue_len]];
    m->time_ex += now.QuadPart - m->from;
    m->time_inc += now.QuadPart - m->from;

    if (p->queue_len > 0) {
        Block *prev = &p->blocks[p->queue[p->queue_len - 1]];
        prev->from  = now.QuadPart;
        prev->time_inc += now.QuadPart - m->from;
    }
}

void profiler_end() {
    Profiler *p = &G->profiler;
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
                   ((f64)(next.bytesProcessed) / nextTimeEx / 1024.0 / 1024.0 / 1024.0));
        }
    }
}

// i32 ByTime(const void *from, const void *to) {
//     return ((RepBlock *)(from))->time - ((RepBlock *)(to))->time;
// }

// i32 ByBytes(const void *from, const void *to) {
//     return ((RepBlock *)(from))->bytes - ((RepBlock *)(to))->bytes;
// }

// i32 ByPageFaults(const void *from, const void *to) {
//     return ((RepBlock *)(from))->pageFaults - ((RepBlock *)(to))->pageFaults;
// }

// static f64 ToGb(f64 bytes) { return bytes / 1024.0 / 1024.0 / 1024.0; }

// RepProfiler New(cstr name, u64 maxRepeats) {
//     return (RepProfiler){
//         .name       = name,
//         .first      = {},
//         .min        = {},
//         .max        = {},
//         .avg        = {},
//         .current    = {},
//         .repeats    = 0,
//         .maxRepeats = maxRepeats,
//     };
// }

// void BeginRep() {
//     LARGE_INTEGER perfCounter = {0};
//     QueryPerformanceCounter(&perfCounter);
//     current = (RepBlock){
//         .time       = (u64)(perfCounter.QuadPart),
//         .bytes      = 0,
//         .pageFaults = ReadPageFaultCount(G->metrics),
//     };
// }

// void AddBytes(u64 bytes) { current.bytes += bytes; }

// void EndRep() {
//     LARGE_INTEGER perfCounter = {0};
//     QueryPerformanceCounter(&perfCounter);
//     current.time       = perfCounter.QuadPart - current.time;
//     current.pageFaults = ReadPageFaultCount(G->metrics) - current.pageFaults;

//     if (current.time < min.time || min.time == 0) {
//         min = current;
//     }

//     if (current.time >= max.time) {
//         max = current;
//     }

//     avg.bytes += current.bytes;
//     avg.time += current.time;
//     avg.pageFaults += current.pageFaults;

//     if (repeats == 0) first = current;

//     repeats++;
// }

// ~RepProfiler() {
//     INFO("Finished %s after %llu repeats.", name, repeats);

//     // FIRST
//     LARGE_INTEGER perfFreq;
//     QueryPerformanceFrequency(&perfFreq);

//     f64 firstTime = f64(first.time) / f64(perfFreq.QuadPart);
//     printf("\t> Initial: \t%.3f ms\t%.3f GB/s\t%llu pf\n", firstTime * 1000.0,
//            ToGb(f64(first.bytes) / firstTime), first.pageFaults);

//     // MIN
//     f64 minTime = f64(min.time) / f64(perfFreq.QuadPart);
//     printf("\t> Fastest: \t%.3f ms\t%.3f GB/s\t%llu pf\n", minTime * 1000.0,
//            ToGb(f64(min.bytes) / minTime), min.pageFaults);

//     // MAX
//     f64 maxTime = f64(max.time) / f64(perfFreq.QuadPart);
//     printf("\t> Slowest: \t%.3f ms\t%.3f GB/s\t%llu pf\n", maxTime * 1000.0,
//            ToGb(f64(max.bytes) / maxTime), max.pageFaults);

//     // AVERAGE
//     f64 avgBytes  = f64(avg.bytes) / f64(repeats);
//     f64 avgFaults = f64(avg.pageFaults) / f64(repeats);
//     f64 avgTime   = f64(avg.time) / f64(repeats);
//     avgTime /= (f64)(perfFreq.QuadPart);

//     printf("\t> Average: \t%.3f ms\t%.3f GB/s\t%.2f pf\n", avgTime * 1000.0,
//            ToGb(f64(avgBytes) / avgTime), avgFaults);
// }

#ifndef DISABLE_PROFILER

#define PROFILER_NEW(name) New(name)
#define PROFILER_END() profiler_end()
#define PROFILE_BLOCK_BEGIN(name) BeginBlock(__COUNTER__ + 1, name, __FILE__, __LINE__)
#define PROFILE_ADD_BANDWIDTH(bytes) AddBytes(bytes)
#define PROFILE_BLOCK_END() EndBlock()
#define PROFILE_SCOPE(name) \
    auto _profilerFlag = BeginScopeBlock(__COUNTER__ + 1, name, __FILE__, __LINE__)
#define PROFILE_FUNCTION() \
    auto _profilerFlag = BeginScopeBlock(__COUNTER__ + 1, __func__, __FILE__, __LINE__)
#define PROFILE(name, code)                                      \
    BeginBlock(__COUNTER__ + 1, name, __FILE__, __LINE__); \
    code;                                                        \
    EndBlock();

#define REPETITION_PROFILE(name, count)                    \
    do {                                                   \
        auto _profiler = New(name, count);                 \
        while (_profiler.repeats < _profiler.maxRepeats) { \
            _profiler.BeginRep();

#define REPETITION_BANDWIDTH(bytes) _profiler.AddBytes(bytes)

#define REPETITION_END() \
    _profiler.EndRep();  \
    }                    \
    }                    \
    while (0)            \
        ;

#else

#define PROFILER_NEW(...)
#define PROFILER_END(...)
#define PROFILE_BLOCK_BEGIN(...)
#define PROFILE_ADD_BANDWIDTH(...)
#define PROFILE_BLOCK_END(...)
#define PROFILE_SCOPE(...)
#define PROFILE_FUNCTION(...)
#define PROFILE(name, code) code

#define REPETITION_PROFILE(...)
#define REPETITION_BANDWIDTH(...)
#define REPETITION_END(...)

#endif
