cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(nbody)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Adding Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(GRAPHICS GRAPHICS_API_OPENGL_43 CACHE STRING "" FORCE)

FetchContent_Declare(
  raylib
  URL "${CMAKE_CURRENT_LIST_DIR}/packages/raylib-master.zip"
  URL_HASH MD5=f9765e188ee29357c3f26d781432bc23
)

FetchContent_MakeAvailable(raylib)

# Our Project
add_executable(${PROJECT_NAME} nbody.c)
target_link_libraries(${PROJECT_NAME} raylib)

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
