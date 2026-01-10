# ReadEngineVersion.cmake
# Reads engine version from engine.json at repository root
#
# This module provides a function to parse the engine version from engine.json
# and set CMake variables. Compatible with CMake 3.16+.
#
# Usage:
#   include(cmake/ReadEngineVersion.cmake)
#   read_engine_version(GMR_VERSION_MAJOR GMR_VERSION_MINOR GMR_VERSION_PATCH)

function(read_engine_version OUTPUT_MAJOR OUTPUT_MINOR OUTPUT_PATCH)
    set(ENGINE_JSON "${CMAKE_SOURCE_DIR}/engine.json")

    if(NOT EXISTS "${ENGINE_JSON}")
        message(FATAL_ERROR "engine.json not found at ${ENGINE_JSON}. This file is required for version information.")
    endif()

    file(READ "${ENGINE_JSON}" JSON_CONTENT)

    # Parse version using regex (compatible with CMake 3.16)
    # Matches: "version": "X.Y.Z" where X, Y, Z are integers
    # The regex looks for the engine version specifically by matching the pattern
    # after "engine" section
    string(REGEX MATCH "\"version\"[ \t\r\n]*:[ \t\r\n]*\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\""
           VERSION_MATCH "${JSON_CONTENT}")

    if(NOT VERSION_MATCH)
        message(FATAL_ERROR "Could not parse version from engine.json. Expected format: \"version\": \"X.Y.Z\"")
    endif()

    set(${OUTPUT_MAJOR} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${OUTPUT_MINOR} ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(${OUTPUT_PATCH} ${CMAKE_MATCH_3} PARENT_SCOPE)

    message(STATUS "Engine version from engine.json: ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
endfunction()
