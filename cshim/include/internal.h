#ifndef DISCORD_ASM_CSHIM_INTERNAL_H
#define DISCORD_ASM_CSHIM_INTERNAL_H

#include "abi.h"
#include "structs.h"
#include <libwebsockets.h>

// Internal WebSocket context
struct discord_ws_context {
    struct lws_context* context;
    struct lws* wsi;
    discord_gateway_t* gateway;
    char* receive_buffer;
    size_t receive_buffer_size;
    size_t receive_buffer_pos;
    int connection_error;
    int close_reason;
};

// Internal gateway structure
struct discord_gateway {
    struct discord_ws_context* ws_ctx;
    discord_gateway_state_t state;
    int heartbeat_interval;
    uint64_t last_heartbeat;
    uint64_t last_heartbeat_ack;
    int sequence;
    char* session_id;
    char* resume_gateway_url;
    int should_reconnect;
};

// Internal function declarations
int discord_ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len);

#endif // DISCORD_ASM_CSHIM_INTERNAL_H