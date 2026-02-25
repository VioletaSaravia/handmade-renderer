#pragma once

#include "base.h"
#include "base_win.h"
#include "windows.h"

typedef struct {
    bool  initialized;
    void *processHandle;
} Metrics;

internal Metrics metrics_init();
internal u64     get_page_fault_count(Metrics);
internal u64     read_cpu_timer();
internal u64     estimate_cpu_freq();

typedef struct {
    // System
    cstr processorArchitecture;
    u32  numberOfProcessors;
    u32  pageSize;
    u32  allocationGranularity;
    f64  cpuFreq;

    // Memory
    u64 totalPhys;
    u64 availPhys;
    u64 totalVirtual;
    u64 availVirtual;

    // OS
    u32 majorVersion;
    u32 minorVersion;
    u32 buildNumber;
    u32 platformId;

    // GPU
    cstr gpuName;
    cstr gpuVendor;
    cstr glVersion;
} SystemInfo;

internal SystemInfo systeminfo_init();
internal void       systeminfo_print(SystemInfo info) {
    INFO("System Information");
    printf("\t> Platform: \t\t\tWindows %s\n", info.processorArchitecture);
    printf("\t> Version: \t\t\t%u.%u.%u\n", info.majorVersion, info.minorVersion, info.buildNumber);
    printf("\t> Processor Count: \t\t%u\n", info.numberOfProcessors);
    printf("\t> CPU Frequency: \t\t%.2f GHz\n", info.cpuFreq);
    printf("\t> Page Size: \t\t\t%u bytes\n", info.pageSize);

    INFO("Memory Information");
    printf("\t> Total Physical Memory: \t%llu MB\n", info.totalPhys / (1024 * 1024));
    printf("\t> Available Physical Memory: \t%llu MB\n", info.availPhys / (1024 * 1024));
    printf("\t> Total Virtual Memory: \t%llu MB\n", info.totalVirtual / (1024 * 1024));
    printf("\t> Available Virtual Memory: \t%llu MB\n", info.availVirtual / (1024 * 1024));
}

typedef struct {
    cstr name;
    cstr version;
} Info;