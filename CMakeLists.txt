cmake_minimum_required(VERSION 2.8)
project(sdl_pnglite)

include(FindPkgConfig)
set(ENV{PKG_CONFIG_PATH} "/home/lxnt/00DFGL/fgtestbed/prefix/lib/pkgconfig/")
set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -ggdb3")
set(CMAKE_C_DEBUG_FLAGS "-Wall -Wextra -pedantic -ggdb3")
pkg_check_modules(PKG_SDL2 REQUIRED sdl2)
link_directories(${PKG_SDL2_LIBRARY_DIRS})
include_directories( ${PKG_SDL2_INCLUDE_DIRS} )

add_library(SDL_pnglite STATIC SDL_pnglite.c pnglite.c)

add_executable(test-save test-save.c )
target_link_libraries(test-save  ${PKG_SDL2_LIBRARIES} SDL_pnglite z)

add_executable(test-load test-load.c )
target_link_libraries(test-load  ${PKG_SDL2_LIBRARIES} SDL_pnglite z)


