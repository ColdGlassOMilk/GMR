cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binary to project root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Sources
set(SOURCES
    src/main.cpp
    src/state.cpp
    src/resources/texture_manager.cpp
    src/resources/sound_manager.cpp
    src/bindings/binding_helpers.cpp
    src/bindings/graphics.cpp
    src/bindings/input.cpp
    src/bindings/audio.cpp
    src/bindings/window.cpp
    src/bindings/util.cpp
    src/bindings/console.cpp
    src/scripting/loader.cpp
    src/scripting/helpers.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Platform-specific setup
if(WIN32)
    # Windows with MinGW - use find_package for raylib
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    
    find_package(raylib CONFIG REQUIRED)
    
    target_include_directories(${PROJECT_NAME} PRIVATE C:/msys64/mingw64/include)
    target_link_directories(${PROJECT_NAME} PRIVATE C:/msys64/mingw64/lib)
    
    # Link raylib (this brings in glfw and other deps automatically)
    # Then link mruby and its dependencies
    target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        mruby
        ws2_32
    )
    
    # Static linking for standalone executable
    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
    
    # Hide console window in release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
    endif()
    
elseif(APPLE)
    # macOS
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        mruby
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
    
else()
    # Linux
    target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        mruby
        m
        pthread
        dl
        GL
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Debug/Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
endif()

# Copy scripts folder to output (optional, for out-of-source builds)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scripts
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts
    COMMENT "Copying scripts folder..."
)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")