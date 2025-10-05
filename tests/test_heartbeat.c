#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "abi.h"

void test_time_functions() {
    printf("Testing timing functions...\n");
    
    // Test time measurement
    uint64_t start_time = discord_time_now_ms();
    printf("  Current time: %llu ms\n", (unsigned long long)start_time);
    
    // Test sleep function
    printf("  Sleeping for 100ms...\n");
    discord_sleep_ms(100);
    
    uint64_t end_time = discord_time_now_ms();
    uint64_t elapsed = end_time - start_time;
    
    printf("  Elapsed time: %llu ms\n", (unsigned long long)elapsed);
    
    // Verify sleep worked (allow some tolerance for timing precision)
    assert(elapsed >= 90);  // At least 90ms should have passed
    assert(elapsed <= 200); // But not more than 200ms (generous tolerance)
    
    printf("  ✓ Sleep timing is within acceptable range\n");
}

void test_heartbeat_timing() {
    printf("Testing heartbeat timing simulation...\n");
    
    // Simulate heartbeat interval of 41250ms (typical Discord value)
    const uint32_t heartbeat_interval = 41250;
    const uint32_t test_sleep = 50; // Short sleep for testing
    
    uint64_t last_heartbeat = discord_time_now_ms();
    printf("  Initial heartbeat time: %llu ms\n", (unsigned long long)last_heartbeat);
    
    // Sleep briefly
    discord_sleep_ms(test_sleep);
    
    uint64_t current_time = discord_time_now_ms();
    uint64_t time_since_last = current_time - last_heartbeat;
    
    printf("  Time since last heartbeat: %llu ms\n", (unsigned long long)time_since_last);
    
    // Check if heartbeat would be due (it shouldn't be after just 50ms)
    uint64_t next_heartbeat_due = last_heartbeat + heartbeat_interval;
    int heartbeat_needed = (current_time >= next_heartbeat_due);
    
    printf("  Next heartbeat due at: %llu ms\n", (unsigned long long)next_heartbeat_due);
    printf("  Heartbeat needed now: %s\n", heartbeat_needed ? "YES" : "NO");
    
    assert(!heartbeat_needed); // Should not need heartbeat yet
    printf("  ✓ Heartbeat timing logic works correctly\n");
}

void test_time_monotonic() {
    printf("Testing time monotonicity...\n");
    
    uint64_t times[10];
    
    // Take several time measurements
    for (int i = 0; i < 10; i++) {
        times[i] = discord_time_now_ms();
        discord_sleep_ms(1); // Small sleep between measurements
    }
    
    // Verify time is monotonically increasing
    for (int i = 1; i < 10; i++) {
        assert(times[i] >= times[i-1]);
        printf("  Time %d: %llu ms (diff: +%llu)\n", 
               i, (unsigned long long)times[i], 
               (unsigned long long)(times[i] - times[i-1]));
    }
    
    printf("  ✓ Time measurements are monotonic\n");
}

int main() {
    printf("Discord ASM Heartbeat Timing Tests\n");
    printf("==================================\n\n");
    
    test_time_functions();
    printf("\n");
    
    test_heartbeat_timing();
    printf("\n");
    
    test_time_monotonic();
    printf("\n");
    
    printf("All timing tests passed! ✓\n");
    return 0;
}