cmake_minimum_required(VERSION 3.24...3.30)
project(raylib-game-template)

include(FetchContent)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use raylib's Web platform automatically when CMake is configured with emcc/em++.
if (EMSCRIPTEN OR CMAKE_C_COMPILER MATCHES "emcc$" OR CMAKE_CXX_COMPILER MATCHES "em\\+\\+$")
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
endif()

# Dependencies
set(RAYLIB_VERSION 6.0)
set(FLECS_VERSION 4.1.6)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_Declare(
    flecs
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/SanderMertens/flecs/archive/refs/tags/v${FLECS_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

set(FLECS_SHARED OFF CACHE BOOL "Build shared flecs lib" FORCE)
set(FLECS_STATIC ON CACHE BOOL "Build static flecs lib" FORCE)
set(FLECS_TESTS OFF CACHE BOOL "Build flecs tests" FORCE)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(flecs)

if (TARGET flecs::flecs_static)
    set(FLECS_TARGET flecs::flecs_static)
elseif (TARGET flecs_static)
    set(FLECS_TARGET flecs_static)
elseif (TARGET flecs::flecs)
    set(FLECS_TARGET flecs::flecs)
elseif (TARGET flecs)
    set(FLECS_TARGET flecs)
else()
    message(FATAL_ERROR "Flecs was found, but no CMake target was exported")
endif()

# Our Project
add_executable(${PROJECT_NAME})
add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(include/glm)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if ("${PLATFORM}" STREQUAL "Web")
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/../resources
    )
    #DEPENDS ${PROJECT_NAME}
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    #DEPENDS ${PROJECT_NAME}
endif()

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib ${FLECS_TARGET})

# Web Configurations
if ("${PLATFORM}" STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PUBLIC -sUSE_GLFW=3 PUBLIC --preload-file resources)
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
