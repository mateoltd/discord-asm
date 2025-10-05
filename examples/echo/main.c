#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abi.h"
#include "opcodes.h"

// Assembly functions from gateway.asm
extern discord_result_t discord_gateway_connect(const char* token);
extern discord_result_t discord_gateway_run(const char* token);
extern discord_result_t discord_gateway_disconnect(void);

static void print_usage(const char* program_name) {
    printf("Usage: %s\n", program_name);
    printf("Environment variables:\n");
    printf("  DISCORD_BOT_TOKEN - Your Discord bot token (required)\n");
    printf("  DISCORD_INTENTS   - Intent bitfield (optional, defaults to basic intents)\n");
}

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    
    printf("Discord ASM Echo Bot - MVP Gateway Client\n");
    printf("==========================================\n\n");
    
    // Get bot token from environment
    const char* token = getenv("DISCORD_BOT_TOKEN");
    if (!token) {
        fprintf(stderr, "Error: DISCORD_BOT_TOKEN environment variable not set\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Validate token format (basic check)
    if (strlen(token) < 50) {
        fprintf(stderr, "Error: Bot token appears to be invalid (too short)\n");
        return 1;
    }
    
    printf("Connecting to Discord Gateway...\n");
    
    // Connect to gateway
    discord_result_t result = discord_gateway_connect(token);
    if (result != DISCORD_OK) {
        fprintf(stderr, "Failed to connect to Discord Gateway: %d\n", result);
        return 1;
    }
    
    printf("Connected successfully!\n");
    printf("Starting gateway event loop...\n");
    printf("(This will connect, identify, start heartbeat, then run indefinitely)\n");
    printf("Press Ctrl+C to stop.\n\n");
    
    // Run main gateway loop
    result = discord_gateway_run(token);
    if (result != DISCORD_OK) {
        fprintf(stderr, "Gateway event loop exited with error: %d\n", result);
    }
    
    printf("\nDisconnecting...\n");
    
    // Disconnect
    discord_gateway_disconnect();
    
    printf("Disconnected. Goodbye!\n");
    return (result == DISCORD_OK) ? 0 : 1;
}