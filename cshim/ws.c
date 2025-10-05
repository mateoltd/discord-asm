#include "abi.h"
#include "structs.h"
#include "internal.h"
#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WS_RECEIVE_BUFFER_SIZE 65536

// WebSocket callback function
int discord_ws_callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len) {
    struct discord_ws_context* ws_ctx = (struct discord_ws_context*)user;
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            if (ws_ctx && ws_ctx->gateway) {
                ws_ctx->gateway->state = DISCORD_STATE_CONNECTED;
            }
            lws_callback_on_writable(wsi);
            break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (ws_ctx && in && len > 0) {
                // Ensure we have enough buffer space
                size_t required = ws_ctx->receive_buffer_pos + len + 1;
                if (required > ws_ctx->receive_buffer_size) {
                    size_t new_size = required * 2;
                    char* new_buffer = realloc(ws_ctx->receive_buffer, new_size);
                    if (!new_buffer) {
                        ws_ctx->connection_error = DISCORD_ERROR_MEMORY;
                        return -1;
                    }
                    ws_ctx->receive_buffer = new_buffer;
                    ws_ctx->receive_buffer_size = new_size;
                }
                
                // Copy received data
                memcpy(ws_ctx->receive_buffer + ws_ctx->receive_buffer_pos, in, len);
                ws_ctx->receive_buffer_pos += len;
                
                // Check if this is the final fragment
                if (lws_is_final_fragment(wsi)) {
                    ws_ctx->receive_buffer[ws_ctx->receive_buffer_pos] = '\0';
                    // Buffer is ready for consumption
                }
            }
            break;
            
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // Ready to send data (handled by send function)
            break;
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            if (ws_ctx) {
                ws_ctx->connection_error = DISCORD_ERROR_NETWORK;
                if (ws_ctx->gateway) {
                    ws_ctx->gateway->state = DISCORD_STATE_ERROR;
                }
            }
            break;
            
        case LWS_CALLBACK_CLOSED:
            if (ws_ctx && ws_ctx->gateway) {
                ws_ctx->gateway->state = DISCORD_STATE_DISCONNECTED;
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

// Protocol definition
static struct lws_protocols protocols[] = {
    {
        "discord-gateway",
        discord_ws_callback,
        sizeof(struct discord_ws_context),
        WS_RECEIVE_BUFFER_SIZE,
        0, NULL, 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } // terminator
};

discord_result_t discord_ws_connect(const char* url, discord_gateway_t** gateway) {
    if (!url || !gateway) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    // Parse URL
    struct lws_client_connect_info info = {0};
    char* url_copy = strdup(url);
    if (!url_copy) {
        return DISCORD_ERROR_MEMORY;
    }
    
    // Simple URL parsing (assumes wss://gateway.discord.gg/?v=10&encoding=json format)
    char* host_start = strstr(url_copy, "://");
    if (!host_start) {
        free(url_copy);
        return DISCORD_ERROR_INVALID_PARAM;
    }
    host_start += 3;
    
    char* path_start = strchr(host_start, '/');
    char* port_start = strchr(host_start, ':');
    
    if (path_start) {
        *path_start = '\0';
        info.path = path_start + 1;
    } else {
        info.path = "/";
    }
    
    if (port_start && (!path_start || port_start < path_start)) {
        *port_start = '\0';
        info.port = atoi(port_start + 1);
    } else {
        info.port = 443; // Default WSS port
    }
    
    info.address = host_start;
    
    // Create gateway structure
    discord_gateway_t* gw = malloc(sizeof(discord_gateway_t));
    if (!gw) {
        free(url_copy);
        return DISCORD_ERROR_MEMORY;
    }
    
    memset(gw, 0, sizeof(discord_gateway_t));
    gw->state = DISCORD_STATE_CONNECTING;
    
    // Create WebSocket context structure
    struct discord_ws_context* ws_ctx = malloc(sizeof(struct discord_ws_context));
    if (!ws_ctx) {
        free(gw);
        free(url_copy);
        return DISCORD_ERROR_MEMORY;
    }
    
    memset(ws_ctx, 0, sizeof(struct discord_ws_context));
    ws_ctx->gateway = gw;
    ws_ctx->receive_buffer_size = WS_RECEIVE_BUFFER_SIZE;
    ws_ctx->receive_buffer = malloc(ws_ctx->receive_buffer_size);
    
    if (!ws_ctx->receive_buffer) {
        free(ws_ctx);
        free(gw);
        free(url_copy);
        return DISCORD_ERROR_MEMORY;
    }
    
    gw->ws_ctx = ws_ctx;
    
    // Create libwebsockets context
    struct lws_context_creation_info ctx_info = {0};
    ctx_info.port = CONTEXT_PORT_NO_LISTEN;
    ctx_info.protocols = protocols;
    ctx_info.gid = -1;
    ctx_info.uid = -1;
    ctx_info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    
    ws_ctx->context = lws_create_context(&ctx_info);
    if (!ws_ctx->context) {
        free(ws_ctx->receive_buffer);
        free(ws_ctx);
        free(gw);
        free(url_copy);
        return DISCORD_ERROR_NETWORK;
    }
    
    // Set up connection info
    info.context = ws_ctx->context;
    info.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | 
                          LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    info.host = info.address;
    info.origin = info.address;
    info.protocol = protocols[0].name;
    info.userdata = ws_ctx;
    
    // Connect
    ws_ctx->wsi = lws_client_connect_via_info(&info);
    if (!ws_ctx->wsi) {
        lws_context_destroy(ws_ctx->context);
        free(ws_ctx->receive_buffer);
        free(ws_ctx);
        free(gw);
        free(url_copy);
        return DISCORD_ERROR_NETWORK;
    }
    
    free(url_copy);
    *gateway = gw;
    return DISCORD_OK;
}

discord_result_t discord_ws_send(discord_gateway_t* gateway, const char* data, size_t length) {
    if (!gateway || !gateway->ws_ctx || !data || length == 0) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    struct discord_ws_context* ws_ctx = gateway->ws_ctx;
    if (!ws_ctx->wsi) {
        return DISCORD_ERROR_NETWORK;
    }
    
    // Prepare buffer with LWS pre/post padding
    size_t padded_len = length + LWS_PRE;
    unsigned char* buf = malloc(padded_len);
    if (!buf) {
        return DISCORD_ERROR_MEMORY;
    }
    
    memcpy(buf + LWS_PRE, data, length);
    
    int result = lws_write(ws_ctx->wsi, buf + LWS_PRE, length, LWS_WRITE_TEXT);
    free(buf);
    
    if (result < 0) {
        return DISCORD_ERROR_NETWORK;
    }
    
    return DISCORD_OK;
}

discord_result_t discord_ws_receive(discord_gateway_t* gateway, discord_ws_message_t* message, int timeout_ms) {
    if (!gateway || !gateway->ws_ctx || !message) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    struct discord_ws_context* ws_ctx = gateway->ws_ctx;
    if (!ws_ctx->context) {
        return DISCORD_ERROR_NETWORK;
    }
    
    // Clear previous message data
    ws_ctx->receive_buffer_pos = 0;
    
    // Service the websocket with timeout
    int n = 0;
    int timeout_remaining = timeout_ms;
    const int service_timeout = 50; // Service in 50ms chunks
    
    while (timeout_remaining > 0) {
        n = lws_service(ws_ctx->context, service_timeout);
        if (n < 0) {
            return DISCORD_ERROR_NETWORK;
        }
        
        // Check if we received a complete message
        if (ws_ctx->receive_buffer_pos > 0) {
            message->data = malloc(ws_ctx->receive_buffer_pos + 1);
            if (!message->data) {
                return DISCORD_ERROR_MEMORY;
            }
            
            memcpy(message->data, ws_ctx->receive_buffer, ws_ctx->receive_buffer_pos);
            message->data[ws_ctx->receive_buffer_pos] = '\0';
            message->length = ws_ctx->receive_buffer_pos;
            message->is_binary = 0; // Discord Gateway uses text frames
            
            // Reset buffer for next message
            ws_ctx->receive_buffer_pos = 0;
            return DISCORD_OK;
        }
        
        // Check for connection errors
        if (ws_ctx->connection_error != 0) {
            return ws_ctx->connection_error;
        }
        
        timeout_remaining -= service_timeout;
    }
    
    return DISCORD_ERROR_TIMEOUT;
}

discord_result_t discord_ws_close(discord_gateway_t* gateway) {
    if (!gateway) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    if (gateway->ws_ctx) {
        struct discord_ws_context* ws_ctx = gateway->ws_ctx;
        
        if (ws_ctx->wsi) {
            lws_close_reason(ws_ctx->wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
            // Service a few times to complete the close handshake
            for (int i = 0; i < 10; i++) {
                lws_service(ws_ctx->context, 10);
            }
        }
        
        if (ws_ctx->context) {
            lws_context_destroy(ws_ctx->context);
        }
        
        if (ws_ctx->receive_buffer) {
            free(ws_ctx->receive_buffer);
        }
        
        free(ws_ctx);
    }
    
    if (gateway->session_id) {
        free(gateway->session_id);
    }
    
    if (gateway->resume_gateway_url) {
        free(gateway->resume_gateway_url);
    }
    
    free(gateway);
    return DISCORD_OK;
}

void discord_ws_free_message(discord_ws_message_t* message) {
    if (message && message->data) {
        free(message->data);
        message->data = NULL;
        message->length = 0;
    }
}