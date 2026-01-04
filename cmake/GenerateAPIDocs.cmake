# GenerateAPIDocs.cmake - Generate API documentation from C++ binding sources
#
# This module provides functionality to automatically generate API documentation
# (api.json, syntax.json, version.json) from C++ binding source files.
#
# Usage:
#   include(GenerateAPIDocs)
#   generate_api_docs(${PROJECT_NAME})

function(generate_api_docs TARGET_NAME)
    # Find Ruby interpreter
    # Check MinGW/MSYS2 paths first as they're most common for this project
    find_program(RUBY_EXECUTABLE ruby
        PATHS
            "C:/msys64/ucrt64/bin"
            "C:/msys64/mingw64/bin"
            "C:/msys64/usr/bin"
            "C:/Ruby33-x64/bin"
            "C:/Ruby32-x64/bin"
            "C:/Ruby31-x64/bin"
            "/usr/bin"
            "/usr/local/bin"
            "$ENV{HOME}/.rbenv/shims"
            "$ENV{HOME}/.rvm/rubies/default/bin"
    )

    if(NOT RUBY_EXECUTABLE)
        message(WARNING "Ruby not found. API documentation generation disabled.")
        message(WARNING "Install Ruby or set RUBY_EXECUTABLE to enable API doc generation.")
        return()
    endif()

    message(STATUS "Using Ruby: ${RUBY_EXECUTABLE}")

    # Define source files to parse
    set(BINDING_SOURCES
        ${CMAKE_SOURCE_DIR}/src/bindings/graphics.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/audio.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/input.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/window.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/util.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/collision.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/binding_helpers.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/console.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/math.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/camera.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/transform.cpp
        ${CMAKE_SOURCE_DIR}/src/bindings/sprite.cpp
    )

    # Filter to only existing files
    set(EXISTING_SOURCES "")
    foreach(SRC ${BINDING_SOURCES})
        if(EXISTS "${SRC}")
            list(APPEND EXISTING_SOURCES "${SRC}")
        endif()
    endforeach()

    if(NOT EXISTING_SOURCES)
        message(WARNING "No binding source files found. API documentation generation disabled.")
        return()
    endif()

    # Output directory and files
    set(API_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/engine/language)
    set(API_OUTPUTS
        ${API_OUTPUT_DIR}/api.json
        ${API_OUTPUT_DIR}/syntax.json
        ${API_OUTPUT_DIR}/version.json
    )

    # Generator script
    set(GENERATOR_SCRIPT ${CMAKE_SOURCE_DIR}/tools/generate_api_docs.rb)

    if(NOT EXISTS "${GENERATOR_SCRIPT}")
        message(WARNING "Generator script not found: ${GENERATOR_SCRIPT}")
        message(WARNING "API documentation generation disabled.")
        return()
    endif()

    # Build source arguments
    set(SOURCE_ARGS "")
    foreach(SRC ${EXISTING_SOURCES})
        list(APPEND SOURCE_ARGS "-s" "${SRC}")
    endforeach()

    # Custom command to generate API docs
    add_custom_command(
        OUTPUT ${API_OUTPUTS}
        COMMAND ${RUBY_EXECUTABLE} ${GENERATOR_SCRIPT}
            ${SOURCE_ARGS}
            -o ${API_OUTPUT_DIR}
            -v ${PROJECT_VERSION}
        DEPENDS ${EXISTING_SOURCES} ${GENERATOR_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation from bindings..."
        VERBATIM
    )

    # Create custom target for documentation generation
    add_custom_target(generate_api_docs
        DEPENDS ${API_OUTPUTS}
    )

    # Make main target depend on API docs (pre-build step)
    add_dependencies(${TARGET_NAME} generate_api_docs)

    message(STATUS "API documentation generation configured")
    message(STATUS "  Sources: ${EXISTING_SOURCES}")
    message(STATUS "  Output: ${API_OUTPUT_DIR}")

endfunction()
