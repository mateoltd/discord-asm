#include "abi.h"
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
    #include <sysinfoapi.h>
#elif defined(__APPLE__)
    #include <mach/mach_time.h>
    #include <unistd.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
#endif

uint64_t discord_time_now_ms(void) {
#ifdef _WIN32
    // Windows: Use GetTickCount64 for millisecond precision
    return GetTickCount64();
#elif defined(__APPLE__)
    // macOS: Use mach_absolute_time with timebase conversion
    static mach_timebase_info_data_t timebase = {0, 0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    
    uint64_t abs_time = mach_absolute_time();
    // Convert to nanoseconds, then to milliseconds
    return (abs_time * timebase.numer / timebase.denom) / 1000000;
#else
    // Linux/Unix: Use clock_gettime with CLOCK_MONOTONIC
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
    }
    
    // Fallback to gettimeofday
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
    }
    
    return 0;
#endif
}

void discord_sleep_ms(uint32_t milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep takes microseconds
#endif
}