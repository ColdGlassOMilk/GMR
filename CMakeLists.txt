cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include script compilation utilities
include(cmake/CompileScripts.cmake)

# Detect Emscripten
if(EMSCRIPTEN OR PLATFORM STREQUAL "Web")
    set(PLATFORM_WEB TRUE)
    message(STATUS "Building for Web/Emscripten")
else()
    set(PLATFORM_WEB FALSE)
endif()

# Output binary to project root (or build-web for web builds)
if(PLATFORM_WEB)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build-web)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

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
    
    # Calculate shell file path
    set(SHELL_FILE "${CMAKE_SOURCE_DIR}/web/shell.html")
    if(NOT EXISTS "${SHELL_FILE}")
        set(SHELL_FILE "${CMAKE_SOURCE_DIR}/web/minshell.html")
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
    )

    # Note: Scripts are compiled into bytecode and embedded in WASM
    # No need to preload script files for web builds

    # Preload assets folder for web builds (textures, sounds, etc.)
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
        target_link_options(${PROJECT_NAME} PRIVATE
            "--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets"
        )
    endif()

    # Add custom shell if it exists
    # if(EXISTS ${SHELL_FILE})
    #     list(APPEND WEB_LINK_FLAGS "--shell-file=${SHELL_FILE}")
    # endif()
    if(EXISTS "${SHELL_FILE}")
        target_link_options(${PROJECT_NAME} PRIVATE
            "--shell-file=${SHELL_FILE}"
        )
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
    set(MINGW_ROOT "C:/msys64/mingw64")

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/include
        ${MINGW_ROOT}/include
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${RAYLIB_ROOT}/lib/libraylib.a
        ${MINGW_ROOT}/lib/libmruby.a

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

# Compile Ruby scripts to bytecode for Release builds and Web builds
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR PLATFORM_WEB)
    message(STATUS "Enabling compiled script bytecode")
    set(COMPILED_SCRIPTS_HEADER "${CMAKE_BINARY_DIR}/generated/gmr/scripting/compiled_scripts.hpp")

    # Compile scripts and generate header
    compile_ruby_scripts(
        ${PROJECT_NAME}
        "${CMAKE_SOURCE_DIR}/scripts"
        "${COMPILED_SCRIPTS_HEADER}"
    )

    # Enable compiled scripts flag
    target_compile_definitions(${PROJECT_NAME} PRIVATE GMR_USE_COMPILED_SCRIPTS)
else()
    message(STATUS "Using runtime script loading (debug mode)")
endif()

# Copy scripts folder to output (for native builds only - web uses preload)
if(NOT PLATFORM_WEB)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts
        COMMENT "Copying scripts folder..."
    )
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform Web: ${PLATFORM_WEB}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")