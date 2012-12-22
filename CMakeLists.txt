cmake_minimum_required(VERSION 2.8)
project(sdl_pnglite)

set(M32 "-m32 -march=i686")
set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(ARCH64 TRUE)
else()
  set(ARCH64 FALSE)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS_DEBUG "-ggdb3")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ggdb3")

if (ARCH64)
    option(BUILD_32BIT "Compile 32-bit version" ON)
    if(BUILD_32BIT)
        set(CMAKE_LIBRARY_PATH /usr/lib32 /usr/lib/i386-linux-gnu)
        set(CMAKE_C_FLAGS "${M32} ${CMAKE_C_FLAGS}")
        set(CMAKE_C_FLAGS_DEBUG "${M32} ${CMAKE_C_FLAGS}")
    endif()
endif()

# mingw32 hack for the zlib
include_directories("${CMAKE_INSTALL_PREFIX}/include")


include(FindPkgConfig)
pkg_check_modules(PKG_SDL2 REQUIRED sdl2)
link_directories(${PKG_SDL2_LIBRARY_DIRS})
include_directories( ${PKG_SDL2_INCLUDE_DIRS} )

add_library(SDL_pnglite STATIC SDL_pnglite.c pnglite.c)
install(TARGETS SDL_pnglite ARCHIVE DESTINATION lib)
install(FILES  SDL_pnglite.h DESTINATION include/SDL2)

add_executable(test-save test-save.c )
target_link_libraries(test-save  SDL_pnglite ${PKG_SDL2_LIBRARIES}  z)

add_executable(test-save-paletted-bmp test-save-paletted-bmp.c )
target_link_libraries(test-save-paletted-bmp ${PKG_SDL2_LIBRARIES})

add_executable(test-load test-load.c )
target_link_libraries(test-load  SDL_pnglite ${PKG_SDL2_LIBRARIES} z)

add_executable(test-lib-load test-lib-load.c pnglite.c )
target_link_libraries(test-lib-load z)

