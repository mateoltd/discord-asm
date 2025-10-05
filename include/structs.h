#ifndef DISCORD_ASM_STRUCTS_H
#define DISCORD_ASM_STRUCTS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Gateway connection state
typedef enum {
    DISCORD_STATE_DISCONNECTED = 0,
    DISCORD_STATE_CONNECTING,
    DISCORD_STATE_CONNECTED,
    DISCORD_STATE_IDENTIFYING,
    DISCORD_STATE_READY,
    DISCORD_STATE_HEARTBEATING,
    DISCORD_STATE_RECONNECTING,
    DISCORD_STATE_CLOSING,
    DISCORD_STATE_ERROR
} discord_gateway_state_t;

// Gateway context structure (opaque to Assembly)
struct discord_gateway {
    void* ws_context;               // WebSocket context (libwebsockets)
    discord_gateway_state_t state;  // Current connection state
    int heartbeat_interval;         // Heartbeat interval in ms
    uint64_t last_heartbeat;        // Last heartbeat timestamp
    uint64_t last_heartbeat_ack;    // Last heartbeat ACK timestamp
    int sequence;                   // Last sequence number
    char* session_id;               // Session ID for resume
    char* resume_gateway_url;       // Gateway URL for resume
    int should_reconnect;           // Flag to indicate reconnection needed
};

// Assembly-facing event structure
typedef struct {
    int opcode;                     // Gateway opcode
    char* data;                     // JSON payload data
    size_t data_length;             // Length of payload
    int sequence;                   // Sequence number (if applicable)
    char* event_type;               // Event type for DISPATCH (op 0)
} discord_event_t;

// Heartbeat timer structure (for Assembly heartbeat loop)
typedef struct {
    uint64_t interval_ms;           // Heartbeat interval
    uint64_t last_sent;             // Last heartbeat sent timestamp
    uint64_t next_due;              // Next heartbeat due timestamp
    int sequence;                   // Current sequence for heartbeat
} discord_heartbeat_timer_t;

// Bot configuration
typedef struct {
    char* token;                    // Bot token
    uint32_t intents;               // Intent bitfield
    int shard_id;                   // Shard ID (for sharding)
    int shard_count;                // Total shard count
    char* gateway_url;              // Gateway URL override
} discord_bot_config_t;

// Function pointer types for Assembly callbacks
typedef void (*discord_event_handler_t)(const discord_event_t* event);
typedef void (*discord_error_handler_t)(int error_code, const char* message);

#ifdef __cplusplus
}
#endif

#endif // DISCORD_ASM_STRUCTS_H