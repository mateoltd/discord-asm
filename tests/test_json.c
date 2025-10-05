#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "abi.h"
#include "opcodes.h"

// Test fixture data
static const char* hello_json = 
    "{"
    "\"t\":null,"
    "\"s\":null,"
    "\"op\":10,"
    "\"d\":{"
        "\"heartbeat_interval\":41250,"
        "\"_trace\":[\"[\\\"\",null]\"]"
    "}"
    "}";

static const char* heartbeat_json = 
    "{"
    "\"op\":1,"
    "\"d\":null"
    "}";

static const char* heartbeat_ack_json =
    "{"
    "\"t\":null,"
    "\"s\":null,"
    "\"op\":11,"
    "\"d\":null"
    "}";

void test_parse_opcode() {
    printf("Testing opcode parsing...\n");
    
    int opcode;
    discord_result_t result;
    
    // Test HELLO opcode
    result = discord_json_parse_opcode(hello_json, &opcode);
    assert(result == DISCORD_OK);
    assert(opcode == DISCORD_OP_HELLO);
    printf("  ✓ HELLO opcode parsed correctly: %d\n", opcode);
    
    // Test HEARTBEAT opcode
    result = discord_json_parse_opcode(heartbeat_json, &opcode);
    assert(result == DISCORD_OK);
    assert(opcode == DISCORD_OP_HEARTBEAT);
    printf("  ✓ HEARTBEAT opcode parsed correctly: %d\n", opcode);
    
    // Test HEARTBEAT_ACK opcode
    result = discord_json_parse_opcode(heartbeat_ack_json, &opcode);
    assert(result == DISCORD_OK);
    assert(opcode == DISCORD_OP_HEARTBEAT_ACK);
    printf("  ✓ HEARTBEAT_ACK opcode parsed correctly: %d\n", opcode);
    
    // Test invalid JSON
    result = discord_json_parse_opcode("{invalid}", &opcode);
    assert(result != DISCORD_OK);
    printf("  ✓ Invalid JSON rejected correctly\n");
    
    // Test NULL parameters
    result = discord_json_parse_opcode(NULL, &opcode);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    
    result = discord_json_parse_opcode(hello_json, NULL);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    printf("  ✓ NULL parameters rejected correctly\n");
}

void test_parse_hello() {
    printf("Testing HELLO message parsing...\n");
    
    int heartbeat_interval;
    discord_result_t result;
    
    // Test valid HELLO message
    result = discord_json_parse_hello(hello_json, &heartbeat_interval);
    assert(result == DISCORD_OK);
    assert(heartbeat_interval == 41250);
    printf("  ✓ Heartbeat interval parsed correctly: %d ms\n", heartbeat_interval);
    
    // Test invalid JSON
    result = discord_json_parse_hello("{\"op\":10}", &heartbeat_interval);
    assert(result != DISCORD_OK);
    printf("  ✓ Invalid HELLO message rejected correctly\n");
    
    // Test NULL parameters
    result = discord_json_parse_hello(NULL, &heartbeat_interval);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    
    result = discord_json_parse_hello(hello_json, NULL);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    printf("  ✓ NULL parameters rejected correctly\n");
}

void test_create_identify() {
    printf("Testing IDENTIFY message creation...\n");
    
    const char* test_token = "Bot.MTk4NjIyNDgzNDcxOTI1MjQ4.Cl2FMQ.ZnCjm1XVW7vRze4b7Cq4se7kKWs";
    char* json = NULL;
    
    discord_result_t result = discord_json_create_identify(test_token, &json);
    assert(result == DISCORD_OK);
    assert(json != NULL);
    
    // Basic validation - should contain token and op code
    assert(strstr(json, "\"op\":2") != NULL);
    assert(strstr(json, test_token) != NULL);
    assert(strstr(json, "\"intents\"") != NULL);
    
    printf("  ✓ IDENTIFY message created: %.100s...\n", json);
    
    discord_json_free(json);
    
    // Test NULL token
    result = discord_json_create_identify(NULL, &json);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    
    // Test NULL output
    result = discord_json_create_identify(test_token, NULL);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    
    printf("  ✓ NULL parameters rejected correctly\n");
}

void test_create_heartbeat() {
    printf("Testing HEARTBEAT message creation...\n");
    
    char* json = NULL;
    
    // Test with sequence number
    discord_result_t result = discord_json_create_heartbeat(42, &json);
    assert(result == DISCORD_OK);
    assert(json != NULL);
    assert(strstr(json, "\"op\":1") != NULL);
    assert(strstr(json, "42") != NULL);
    
    printf("  ✓ HEARTBEAT with sequence created: %s\n", json);
    discord_json_free(json);
    
    // Test with null sequence
    result = discord_json_create_heartbeat(-1, &json);
    assert(result == DISCORD_OK);
    assert(json != NULL);
    assert(strstr(json, "\"op\":1") != NULL);
    assert(strstr(json, "null") != NULL);
    
    printf("  ✓ HEARTBEAT with null sequence created: %s\n", json);
    discord_json_free(json);
    
    // Test NULL output
    result = discord_json_create_heartbeat(0, NULL);
    assert(result == DISCORD_ERROR_INVALID_PARAM);
    
    printf("  ✓ NULL parameters rejected correctly\n");
}

int main() {
    printf("Discord ASM JSON Parsing Tests\n");
    printf("==============================\n\n");
    
    test_parse_opcode();
    printf("\n");
    
    test_parse_hello();
    printf("\n");
    
    test_create_identify();
    printf("\n");
    
    test_create_heartbeat();
    printf("\n");
    
    printf("All JSON tests passed! ✓\n");
    return 0;
}