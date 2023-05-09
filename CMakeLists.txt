cmake_minimum_required(VERSION 3.25)

project(TestGame)

###
# Base directory relative includes for everyone
###
include_directories(src include)
###

###
# Global Flags
###
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated -Wno-error=unused-command-line-argument)
###

###
# Flags
###
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g3 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
###

###
# Dependencies
###
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML
    GIT_TAG 2.6.x
)
FetchContent_MakeAvailable(SFML)
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs
    GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(flecs)
###

###
# Targets
###
file(GLOB TestGame_src CONFIGURE_DEPENDS "src/*.cpp" "src/*/*.cpp" "src/*.hpp" "src/*/*.hpp")
add_executable(TestGame ${TestGame_src})
target_link_libraries(TestGame fmt::fmt flecs::flecs_static sfml-graphics sfml-window sfml-system sfml-audio sfml-network)
if (WIN32)
    file(GLOB SFML_dlls "${SFML_BINARY_DIR}/lib/*.dll")
    add_custom_command(TARGET TestGame POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SFML_dlls} ${CMAKE_BINARY_DIR}/)
endif ()
###
