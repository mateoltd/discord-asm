# Echo example bot
add_executable(discord-asm-echo main.c)
target_link_libraries(discord-asm-echo discord-asm-core)

# Set the output directory
set_target_properties(discord-asm-echo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/echo"
)