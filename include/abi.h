#ifndef DISCORD_ASM_ABI_H
#define DISCORD_ASM_ABI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Cross-platform calling convention macros
#ifdef _WIN32
    #define DISCORD_CALL __cdecl
    #define DISCORD_EXPORT __declspec(dllexport)
#else
    #define DISCORD_CALL
    #define DISCORD_EXPORT __attribute__((visibility("default")))
#endif

// Forward declarations
typedef struct discord_gateway discord_gateway_t;
typedef struct discord_ws_message discord_ws_message_t;

// Result codes
typedef enum {
    DISCORD_OK = 0,
    DISCORD_ERROR_INVALID_PARAM = -1,
    DISCORD_ERROR_NETWORK = -2,
    DISCORD_ERROR_AUTH = -3,
    DISCORD_ERROR_JSON = -4,
    DISCORD_ERROR_MEMORY = -5,
    DISCORD_ERROR_TIMEOUT = -6
} discord_result_t;

// WebSocket message structure
struct discord_ws_message {
    char* data;
    size_t length;
    int is_binary;
};

// C Shim API - WebSocket Operations
DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_ws_connect(const char* url, discord_gateway_t** gateway);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_ws_send(discord_gateway_t* gateway, const char* data, size_t length);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_ws_receive(discord_gateway_t* gateway, discord_ws_message_t* message, int timeout_ms);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_ws_close(discord_gateway_t* gateway);

DISCORD_EXPORT void DISCORD_CALL 
discord_ws_free_message(discord_ws_message_t* message);

// C Shim API - JSON Operations
DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_json_parse_opcode(const char* json, int* opcode);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_json_parse_hello(const char* json, int* heartbeat_interval);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_json_create_identify(const char* token, char** json_out);

DISCORD_EXPORT discord_result_t DISCORD_CALL 
discord_json_create_heartbeat(int sequence, char** json_out);

DISCORD_EXPORT void DISCORD_CALL 
discord_json_free(char* json);

// C Shim API - Timing Operations
DISCORD_EXPORT uint64_t DISCORD_CALL 
discord_time_now_ms(void);

DISCORD_EXPORT void DISCORD_CALL 
discord_sleep_ms(uint32_t milliseconds);

#ifdef __cplusplus
}
#endif

#endif // DISCORD_ASM_ABI_H