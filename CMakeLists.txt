cmake_minimum_required(VERSION 3.20)
project(discord-asm VERSION 0.1.0 LANGUAGES C ASM_NASM)

# Platform detection and toolchain setup
if(WIN32)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT win64)
    set(CMAKE_C_STANDARD 99)
    if(MSVC)
        add_compile_options(/W4)
    endif()
elseif(APPLE)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT macho64)
    set(CMAKE_C_STANDARD 99)
    add_compile_options(-Wall -Wextra)
elseif(UNIX)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
    set(CMAKE_C_STANDARD 99)
    add_compile_options(-Wall -Wextra)
endif()

# Find required dependencies
find_package(OpenSSL REQUIRED)
find_package(PkgConfig)

# Try to find libwebsockets via pkg-config first
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LWS libwebsockets)
endif()

# Fallback to find_package if pkg-config doesn't work
if(NOT LWS_FOUND)
    find_path(LWS_INCLUDE_DIR libwebsockets.h
        HINTS ${LWS_ROOT}/include
        PATHS /usr/include /usr/local/include)
    find_library(LWS_LIBRARY websockets
        HINTS ${LWS_ROOT}/lib
        PATHS /usr/lib /usr/local/lib)
    
    if(LWS_INCLUDE_DIR AND LWS_LIBRARY)
        set(LWS_FOUND TRUE)
        set(LWS_INCLUDE_DIRS ${LWS_INCLUDE_DIR})
        set(LWS_LIBRARIES ${LWS_LIBRARY})
    endif()
endif()

if(NOT LWS_FOUND)
    message(WARNING "libwebsockets not found. Will try to use vendored version.")
    # TODO: Add vendored libwebsockets support
endif()

# Include directories
include_directories(include cshim/include)

# C Shim Library
file(GLOB CSHIM_SOURCES cshim/*.c)
add_library(discord-asm-cshim STATIC ${CSHIM_SOURCES})
target_include_directories(discord-asm-cshim PUBLIC 
    include 
    cshim/include
    ${OpenSSL_INCLUDE_DIR}
)

if(LWS_FOUND)
    target_include_directories(discord-asm-cshim PUBLIC ${LWS_INCLUDE_DIRS})
    target_link_libraries(discord-asm-cshim PUBLIC ${LWS_LIBRARIES})
endif()

target_link_libraries(discord-asm-cshim PUBLIC OpenSSL::SSL OpenSSL::Crypto)

# Assembly core library
file(GLOB ASM_SOURCES asm/x64/*.asm)
add_library(discord-asm-core STATIC ${ASM_SOURCES})
target_link_libraries(discord-asm-core PUBLIC discord-asm-cshim)

# Examples
add_subdirectory(examples)

# Tests
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS discord-asm-core discord-asm-cshim
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include)