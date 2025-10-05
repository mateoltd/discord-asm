#include "abi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Simple JSON parser for Discord Gateway messages
// Note: This is a minimal implementation focused on Discord Gateway needs
// For production, consider using a proper JSON library like cJSON

// Helper function to find a JSON value by key
static const char* find_json_value(const char* json, const char* key, size_t* value_len) {
    if (!json || !key) return NULL;
    
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    const char* key_pos = strstr(json, search_pattern);
    if (!key_pos) return NULL;
    
    const char* value_start = key_pos + strlen(search_pattern);
    
    // Skip whitespace
    while (*value_start && isspace(*value_start)) {
        value_start++;
    }
    
    if (!*value_start) return NULL;
    
    const char* value_end = value_start;
    
    if (*value_start == '"') {
        // String value
        value_start++; // Skip opening quote
        value_end = value_start;
        while (*value_end && *value_end != '"') {
            if (*value_end == '\\') value_end++; // Skip escaped character
            value_end++;
        }
    } else if (*value_start == '{') {
        // Object value
        int brace_count = 1;
        value_end = value_start + 1;
        while (*value_end && brace_count > 0) {
            if (*value_end == '{') brace_count++;
            else if (*value_end == '}') brace_count--;
            value_end++;
        }
    } else if (*value_start == '[') {
        // Array value
        int bracket_count = 1;
        value_end = value_start + 1;
        while (*value_end && bracket_count > 0) {
            if (*value_end == '[') bracket_count++;
            else if (*value_end == ']') bracket_count--;
            value_end++;
        }
    } else {
        // Number, boolean, or null
        while (*value_end && !isspace(*value_end) && 
               *value_end != ',' && *value_end != '}' && *value_end != ']') {
            value_end++;
        }
    }
    
    if (value_len) {
        *value_len = value_end - value_start;
    }
    
    return value_start;
}

// Helper function to extract integer from JSON value
static int extract_int(const char* value, size_t len) {
    if (!value || len == 0) return 0;
    
    char temp[32];
    size_t copy_len = len < sizeof(temp) - 1 ? len : sizeof(temp) - 1;
    memcpy(temp, value, copy_len);
    temp[copy_len] = '\0';
    
    return atoi(temp);
}

discord_result_t discord_json_parse_opcode(const char* json, int* opcode) {
    if (!json || !opcode) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    size_t value_len;
    const char* op_value = find_json_value(json, "op", &value_len);
    if (!op_value) {
        return DISCORD_ERROR_JSON;
    }
    
    *opcode = extract_int(op_value, value_len);
    return DISCORD_OK;
}

discord_result_t discord_json_parse_hello(const char* json, int* heartbeat_interval) {
    if (!json || !heartbeat_interval) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    // Find the "d" (data) object first
    size_t d_len;
    const char* d_value = find_json_value(json, "d", &d_len);
    if (!d_value) {
        return DISCORD_ERROR_JSON;
    }
    
    // Create a null-terminated copy of the data object to parse
    char* d_copy = malloc(d_len + 1);
    if (!d_copy) {
        return DISCORD_ERROR_MEMORY;
    }
    
    memcpy(d_copy, d_value, d_len);
    d_copy[d_len] = '\0';
    
    // Find heartbeat_interval in the data object
    size_t interval_len;
    const char* interval_value = find_json_value(d_copy, "heartbeat_interval", &interval_len);
    
    if (!interval_value) {
        free(d_copy);
        return DISCORD_ERROR_JSON;
    }
    
    *heartbeat_interval = extract_int(interval_value, interval_len);
    free(d_copy);
    
    return DISCORD_OK;
}

discord_result_t discord_json_create_identify(const char* token, char** json_out) {
    if (!token || !json_out) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    // Calculate required buffer size
    size_t token_len = strlen(token);
    size_t base_len = 512; // Base JSON structure
    size_t total_len = base_len + token_len;
    
    char* json = malloc(total_len);
    if (!json) {
        return DISCORD_ERROR_MEMORY;
    }
    
    int result = snprintf(json, total_len,
        "{"
        "\"op\":2,"
        "\"d\":{"
            "\"token\":\"%s\","
            "\"intents\":%d,"
            "\"properties\":{"
                "\"os\":\"discord-asm\","
                "\"browser\":\"discord-asm\","
                "\"device\":\"discord-asm\""
            "}"
        "}"
        "}",
        token,
        513 // GUILD_MESSAGES + DIRECT_MESSAGES (basic intents)
    );
    
    if (result < 0 || (size_t)result >= total_len) {
        free(json);
        return DISCORD_ERROR_JSON;
    }
    
    *json_out = json;
    return DISCORD_OK;
}

discord_result_t discord_json_create_heartbeat(int sequence, char** json_out) {
    if (!json_out) {
        return DISCORD_ERROR_INVALID_PARAM;
    }
    
    char* json = malloc(64);
    if (!json) {
        return DISCORD_ERROR_MEMORY;
    }
    
    int result;
    if (sequence >= 0) {
        result = snprintf(json, 64, "{\"op\":1,\"d\":%d}", sequence);
    } else {
        result = snprintf(json, 64, "{\"op\":1,\"d\":null}");
    }
    
    if (result < 0 || result >= 64) {
        free(json);
        return DISCORD_ERROR_JSON;
    }
    
    *json_out = json;
    return DISCORD_OK;
}

void discord_json_free(char* json) {
    if (json) {
        free(json);
    }
}