# Tests
add_executable(test-json test_json.c)
target_link_libraries(test-json discord-asm-cshim)

add_executable(test-heartbeat test_heartbeat.c)  
target_link_libraries(test-heartbeat discord-asm-cshim)

# Register tests with CTest
add_test(NAME JsonParsingTest COMMAND test-json)
add_test(NAME HeartbeatTimingTest COMMAND test-heartbeat)

# Test fixtures directory
file(COPY fixtures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})