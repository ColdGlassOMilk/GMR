cmake_minimum_required(VERSION 3.16)
project(gmr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

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
        set(RAYLIB_WEB_PATH "${CMAKE_SOURCE_DIR}/libs/raylib-web")
    endif()
    
    if(DEFINED ENV{MRUBY_WEB_PATH})
        set(MRUBY_WEB_PATH $ENV{MRUBY_WEB_PATH})
    else()
        set(MRUBY_WEB_PATH "${CMAKE_SOURCE_DIR}/libs/mruby-web")
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
    target_link_options(${PROJECT_NAME} PRIVATE
        -Os
        -sUSE_GLFW=3
        -sGL_ENABLE_GET_PROC_ADDRESS
        -sASSERTIONS=0
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1048576
        -sTOTAL_MEMORY=67108864
        -sASYNCIFY
        -sASYNCIFY_STACK_SIZE=65536
        -sFORCE_FILESYSTEM=1
        "--preload-file=${CMAKE_SOURCE_DIR}/scripts@/scripts"
    )

    
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
    # ===========================================
    # Windows with MinGW
    # ===========================================
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    
    find_package(raylib CONFIG REQUIRED)
    
    target_include_directories(${PROJECT_NAME} PRIVATE C:/msys64/mingw64/include)
    target_link_directories(${PROJECT_NAME} PRIVATE C:/msys64/mingw64/lib)
    
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
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif(NOT PLATFORM_WEB)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Debug/Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
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