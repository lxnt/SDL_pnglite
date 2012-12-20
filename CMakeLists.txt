cmake_minimum_required(VERSION 2.8)
project(sdl_pnglite)

set(LIB32_LINK_DIR "/home/lxnt/00DFGL/lib32" CACHE PATH "what it says")

set(M32 "-m32 -march=i686")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH64 TRUE)
else()
  set(ARCH64 FALSE)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -ggdb3")
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -pedantic -ggdb3")


if (ARCH64)
    if(NOT CONFIGURING_RENDUMPER)
        option(DF_32BIT_COMPAT "Compile for linking with Dwarf Fortress" ON)
    else()
        set(DF_32BIT_COMPAT TRUE)
    endif()
    if(DF_32BIT_COMPAT)
        set(ENV{PKG_CONFIG_PATH} "/home/lxnt/00DFGL/prefix32/lib/pkgconfig/")
        set(CMAKE_INSTALL_PREFIX "/home/lxnt/00DFGL/prefix32" CACHE PATH "install prefix" FORCE)
        set(CMAKE_C_COMPILER gcc-4.5)
        set(CMAKE_LIBRARY_PATH /usr/lib32 /usr/lib/i386-linux-gnu)
        set(CMAKE_C_FLAGS "${M32} ${CMAKE_C_FLAGS}")
        set(CMAKE_C_FLAGS_DEBUG "${M32} ${CMAKE_C_FLAGS}")
    else()
        set(CMAKE_INSTALL_PREFIX "/home/lxnt/00DFGL/prefix64" CACHE PATH "install prefix" FORCE)
        set(ENV{PKG_CONFIG_PATH} "/home/lxnt/00DFGL/prefix64/lib/pkgconfig/")
    endif()
endif()

include(FindPkgConfig)
pkg_check_modules(PKG_SDL2 REQUIRED sdl2)
link_directories(${PKG_SDL2_LIBRARY_DIRS})
include_directories( ${PKG_SDL2_INCLUDE_DIRS} )

add_library(SDL_pnglite STATIC SDL_pnglite.c pnglite.c)
install(TARGETS SDL_pnglite ARCHIVE DESTINATION lib)
install(FILES  SDL_pnglite.h DESTINATION include/SDL2)

add_executable(test-save test-save.c )
target_link_libraries(test-save  ${PKG_SDL2_LIBRARIES} SDL_pnglite z)

add_executable(test-save-paletted-bmp test-save-paletted-bmp.c )
target_link_libraries(test-save-paletted-bmp  ${PKG_SDL2_LIBRARIES})

add_executable(test-load test-load.c )
target_link_libraries(test-load  ${PKG_SDL2_LIBRARIES} SDL_pnglite z)

add_executable(test-lib-load test-lib-load.c pnglite.c )
target_link_libraries(test-lib-load z)


