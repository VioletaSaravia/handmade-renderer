#pragma once

typedef struct Metrics    Metrics;
typedef struct SystemInfo SystemInfo;

#include "base.h"
#include "base_win.h"
#include "windows.h"

internal Metrics    metrics_init();
internal u64        get_page_fault_count(Metrics);
internal u64        read_cpu_timer();
internal u64        estimate_cpu_freq();
internal inline i64 read_acquire(volatile i64 *src);

internal SystemInfo systeminfo_init();
internal void       systeminfo_print(SystemInfo info);