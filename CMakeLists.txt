cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include script compilation utilities
include(cmake/CompileScripts.cmake)

# API documentation generation option (disabled by default - use 'gmrcli setup' instead)
option(GMR_GENERATE_API_DOCS "Generate API documentation during build (normally handled by gmrcli setup)" OFF)
if(GMR_GENERATE_API_DOCS)
    include(cmake/GenerateAPIDocs.cmake)
endif()

# Detect Emscripten
if(EMSCRIPTEN OR PLATFORM STREQUAL "Web")
    set(PLATFORM_WEB TRUE)
    message(STATUS "Building for Web/Emscripten")
else()
    set(PLATFORM_WEB FALSE)
endif()

# Project directory - where game scripts live and where output goes
# If GMR_PROJECT_DIR is set, use it; otherwise fall back to engine directory
if(DEFINED GMR_PROJECT_DIR)
    # Convert relative path to absolute (required for file() commands)
    # BASE_DIR must be CMAKE_BINARY_DIR since relative paths are from the build directory
    get_filename_component(PROJECT_OUTPUT_DIR "${GMR_PROJECT_DIR}" ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")
else()
    set(PROJECT_OUTPUT_DIR ${CMAKE_SOURCE_DIR})
endif()

# Output binary to release folder in project directory
if(PLATFORM_WEB)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/release/web)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/release)
endif()

# Sources
set(SOURCES
    src/main.cpp
    src/state.cpp
    src/camera.cpp
    src/resources/texture_manager.cpp
    src/resources/sound_manager.cpp
    src/resources/tilemap_manager.cpp
    src/bindings/binding_helpers.cpp
    src/bindings/graphics.cpp
    src/bindings/input.cpp
    src/bindings/audio.cpp
    src/bindings/window.cpp
    src/bindings/util.cpp
    src/bindings/console.cpp
    src/bindings/collision.cpp
    src/bindings/math.cpp
    src/bindings/camera.cpp
    src/bindings/transform.cpp
    src/bindings/sprite.cpp
    src/bindings/node.cpp
    src/bindings/scene.cpp
    src/bindings/ease.cpp
    src/bindings/tween.cpp
    src/bindings/sprite_animation.cpp
    src/bindings/animator.cpp
    src/bindings/state_machine.cpp
    src/bindings/filesystem.cpp
    src/bindings/storage.cpp
    src/filesystem/paths.cpp
    src/filesystem/operations.cpp
    src/animation/easing.cpp
    src/animation/animation_manager.cpp
    src/state_machine/state_machine_manager.cpp
    src/input/input_manager.cpp
    src/input/input_context.cpp
    src/event/event_queue.cpp
    src/transform.cpp
    src/node.cpp
    src/scene.cpp
    src/sprite.cpp
    src/draw_queue.cpp
    src/scripting/loader.cpp
    src/scripting/helpers.cpp
    src/scripting/script_error.cpp
    src/output/ndjson.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Generate API documentation (pre-build step) - only if enabled
if(GMR_GENERATE_API_DOCS)
    generate_api_docs(${PROJECT_NAME})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

# Platform-specific setup
if(PLATFORM_WEB)
    # ===========================================
    # Emscripten / WebAssembly Build
    # ===========================================
    
    # Web-specific compile definitions
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WEB)
    
    # Emscripten compiler flags
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Os                         # Optimize for size
        -DPLATFORM_WEB
    )
    
    # Find raylib and mruby (must be built for Emscripten)
    # Users should set these paths or install to Emscripten sysroot
    if(DEFINED ENV{RAYLIB_WEB_PATH})
        set(RAYLIB_WEB_PATH $ENV{RAYLIB_WEB_PATH})
    else()
        set(RAYLIB_WEB_PATH "${CMAKE_SOURCE_DIR}/deps/raylib/web")
    endif()

    if(DEFINED ENV{MRUBY_WEB_PATH})
        set(MRUBY_WEB_PATH $ENV{MRUBY_WEB_PATH})
    else()
        set(MRUBY_WEB_PATH "${CMAKE_SOURCE_DIR}/deps/mruby/web")
    endif()
    
    # Include paths for web libraries
    if(EXISTS "${RAYLIB_WEB_PATH}/include")
        target_include_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_WEB_PATH}/include)
    endif()
    if(EXISTS "${MRUBY_WEB_PATH}/include")
        target_include_directories(${PROJECT_NAME} PRIVATE ${MRUBY_WEB_PATH}/include)
    endif()
    
    # Library paths
    if(EXISTS "${RAYLIB_WEB_PATH}/lib")
        target_link_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_WEB_PATH}/lib)
    endif()
    if(EXISTS "${MRUBY_WEB_PATH}/lib")
        target_link_directories(${PROJECT_NAME} PRIVATE ${MRUBY_WEB_PATH}/lib)
    endif()
    
    # Calculate shell file path - check project dir first, fall back to engine
    # Shell templates are in game/web/ folder
    if(EXISTS "${PROJECT_OUTPUT_DIR}/game/web/shell.html")
        set(SHELL_FILE "${PROJECT_OUTPUT_DIR}/game/web/shell.html")
    elseif(EXISTS "${PROJECT_OUTPUT_DIR}/game/web/minshell.html")
        set(SHELL_FILE "${PROJECT_OUTPUT_DIR}/game/web/minshell.html")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/game/web/shell.html")
        set(SHELL_FILE "${CMAKE_SOURCE_DIR}/game/web/shell.html")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/game/web/minshell.html")
        set(SHELL_FILE "${CMAKE_SOURCE_DIR}/game/web/minshell.html")
    else()
        set(SHELL_FILE "")
    endif()
    
    # Emscripten linker flags
    # set(WEB_LINK_FLAGS
    #     "-Os"
    #     "-sUSE_GLFW=3"
    #     "-sGL_ENABLE_GET_PROC_ADDRESS"
    #     "-sASSERTIONS=0"
    #     "-sWASM=1"
    #     "-sALLOW_MEMORY_GROWTH=1"
    #     "-sSTACK_SIZE=1048576"
    #     "-sTOTAL_MEMORY=67108864"
    #     "-sASYNCIFY"
    #     "-sASYNCIFY_STACK_SIZE=65536"
    #     "-sFORCE_FILESYSTEM=1"
    #     "--preload-file=${CMAKE_SOURCE_DIR}/scripts@/scripts"
    # )
    # Emscripten linker flags
    target_link_options(${PROJECT_NAME} PRIVATE
        -Os
        -sUSE_GLFW=3
        -sGL_ENABLE_GET_PROC_ADDRESS
        -sASSERTIONS=0
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1048576
        -sTOTAL_MEMORY=67108864
        # ASYNCIFY is needed for emscripten_sleep/main loop but is VERY slow to link
        -sASYNCIFY
        -sASYNCIFY_STACK_SIZE=65536
        # Note: ASYNCIFY_ONLY optimization removed - causes quoting issues with Ninja on Windows
        # The full ASYNCIFY pass is slower but more reliable across platforms
        # IDBFS for IndexedDB persistence
        -lidbfs.js
    )

    # Note: Scripts are compiled into bytecode and embedded in WASM
    # No need to preload script files for web builds

    # Preload assets folder for web builds (textures, sounds, etc.)
    # Use relative path from build directory to avoid spaces in paths breaking file_packager
    if(EXISTS "${PROJECT_OUTPUT_DIR}/game/assets")
        file(RELATIVE_PATH ASSETS_REL_PATH "${CMAKE_BINARY_DIR}" "${PROJECT_OUTPUT_DIR}/game/assets")
        target_link_options(${PROJECT_NAME} PRIVATE
            "--preload-file=${ASSETS_REL_PATH}@/assets"
        )
        message(STATUS "Preloading assets from: ${PROJECT_OUTPUT_DIR}/game/assets (${ASSETS_REL_PATH})")
    endif()

    # Add custom shell if it exists
    # Use relative path from build directory to avoid spaces in paths
    if(NOT "${SHELL_FILE}" STREQUAL "")
        file(RELATIVE_PATH SHELL_REL_PATH "${CMAKE_BINARY_DIR}" "${SHELL_FILE}")
        target_link_options(${PROJECT_NAME} PRIVATE
            "--shell-file=${SHELL_REL_PATH}"
        )
        message(STATUS "Using shell file: ${SHELL_REL_PATH}")
    endif()

    
    # Join flags with spaces
    # string(REPLACE ";" " " WEB_LINK_FLAGS_STR "${WEB_LINK_FLAGS}")
    
    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     LINK_FLAGS "${WEB_LINK_FLAGS_STR}"
    # )
    
    # Link libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        mruby
    )
    
    message(STATUS "Web build configuration:")
    message(STATUS "  RAYLIB_WEB_PATH: ${RAYLIB_WEB_PATH}")
    message(STATUS "  MRUBY_WEB_PATH: ${MRUBY_WEB_PATH}")
    message(STATUS "  Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gmr.html")
    
elseif(WIN32)
    set(RAYLIB_ROOT "${CMAKE_SOURCE_DIR}/deps/raylib/native")
    set(MRUBY_ROOT "${CMAKE_SOURCE_DIR}/deps/mruby/native")

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/include
        ${MRUBY_ROOT}/include
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/lib/libraylib.a
        ${MRUBY_ROOT}/lib/libmruby.a

        winmm
        gdi32
        user32
        shell32
        advapi32
        ole32
        uuid
        opengl32
        ws2_32
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        $<$<CONFIG:Release>:-mwindows>
    )

elseif(APPLE)
    # ===========================================
    # macOS
    # ===========================================
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
    # ===========================================
    # Linux
    # ===========================================
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
# if(MSVC)
#     target_compile_options(${PROJECT_NAME} PRIVATE /W4)
# elseif(NOT PLATFORM_WEB)
#     target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
# endif()
if(MSVC)
    # Disable unused local variable warning (C4101)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4101)
else()
    # Disable unused variable warning for GCC/Clang
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unused-variable)
endif()


# Debug/Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
endif()

# REPL configuration
# GMR_REPL_ENABLED: Enable command-only REPL in Release builds (off by default)
option(GMR_REPL_ENABLED "Enable REPL in Release builds (command-only, no arbitrary Ruby eval)" OFF)

# REPL sources - always compiled, runtime behavior differs by build type
set(REPL_SOURCES
    src/repl/repl_session.cpp
    src/repl/output_capture.cpp
)

# Multi-line detector only compiled for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND REPL_SOURCES src/repl/multiline_detector.cpp)
endif()

# Add REPL sources
target_sources(${PROJECT_NAME} PRIVATE ${REPL_SOURCES})

# Add REPL include directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include/gmr/repl
)

# Console module - built-in dev console (always compiled)
set(CONSOLE_SOURCES
    src/console/console_module.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${CONSOLE_SOURCES})

# Add console include directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include/gmr/console
)

# Enable REPL availability flag
if(GMR_REPL_ENABLED OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GMR_REPL_AVAILABLE)
    message(STATUS "REPL enabled")
endif()

# Ruby debugger - only for native Debug builds (never for Web or Release)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT PLATFORM_WEB)
    message(STATUS "Ruby debugger enabled")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GMR_DEBUG_ENABLED MRB_USE_DEBUG_HOOK)

    # Debug server sources
    set(DEBUG_SOURCES
        src/debug/debug_state.cpp
        src/debug/debug_hooks.cpp
        src/debug/debug_server.cpp
        src/debug/breakpoint_manager.cpp
        src/debug/variable_inspector.cpp
        src/debug/json_protocol.cpp
    )
    target_sources(${PROJECT_NAME} PRIVATE ${DEBUG_SOURCES})

    # Debug headers
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include/gmr/debug
    )
endif()

# Compile Ruby scripts to bytecode for Release builds and Web builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR PLATFORM_WEB)
    message(STATUS "Enabling compiled script bytecode")
    set(COMPILED_SCRIPTS_HEADER "${CMAKE_BINARY_DIR}/generated/gmr/scripting/compiled_scripts.hpp")

    # Compile scripts from project directory (where the user's game scripts live)
    compile_ruby_scripts(
        ${PROJECT_NAME}
        "${PROJECT_OUTPUT_DIR}/game/scripts"
        "${COMPILED_SCRIPTS_HEADER}"
    )

    # Enable compiled scripts flag
    target_compile_definitions(${PROJECT_NAME} PRIVATE GMR_USE_COMPILED_SCRIPTS)
else()
    message(STATUS "Using runtime script loading (debug mode)")
endif()

# Copy game assets to output directory
# - Debug builds: Copy entire game folder (scripts loaded from disk for hot-reload)
# - Release builds: Only copy assets (scripts are compiled into bytecode)
if(NOT PLATFORM_WEB)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release: Only copy assets folder (scripts are embedded as bytecode)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_OUTPUT_DIR}/game/assets
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/game/assets
            COMMENT "Copying game assets..."
        )
    else()
        # Debug: Copy entire game folder for hot-reload
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_OUTPUT_DIR}/game
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/game
            COMMENT "Copying game folder..."
        )
    endif()
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform Web: ${PLATFORM_WEB}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")