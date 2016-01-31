cmake_minimum_required(VERSION 2.8)
project(sdl_pnglite)

set(HAVE_SDLIMAGE2 OFF)

if (CMAKE_HOST_UNIX)
  set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/")

  set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -fvisibility=hidden")
  set(CMAKE_C_FLAGS_DEBUG "-ggdb3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -ggdb3")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -ggdb3")

  include(FindPkgConfig)
  pkg_check_modules(PKG_SDL2 REQUIRED sdl2)
  link_directories(${PKG_SDL2_LIBRARY_DIRS})
  include_directories( ${PKG_SDL2_INCLUDE_DIRS} )

  pkg_check_modules(PKG_SDL2IMAGE SDL2_image)
  if (PKG_SDL2IMAGE_FOUND)
    set(HAVE_SDLIMAGE2 ON)
  endif(PKG_SDL2IMAGE_FOUND)
endif(CMAKE_HOST_UNIX)

add_library(SDL_pnglite STATIC SDL_pnglite.c pnglite.c)
install(TARGETS SDL_pnglite ARCHIVE DESTINATION lib)
install(FILES  SDL_pnglite.h DESTINATION include/SDL2)

option(BUILD_TEST_SUITE "Build test-suite" ${HAVE_SDLIMAGE2})
if (BUILD_TEST_SUITE)
    link_directories(${PKG_SDL2IMAGE_LIBRARY_DIRS})
    include_directories( ${PKG_SDL2IMAGE_INCLUDE_DIRS} )
    add_executable(test-suite test-suite.c)
    target_link_libraries(test-suite  SDL_pnglite ${PKG_SDL2_LIBRARIES} ${PKG_SDL2IMAGE_LIBRARIES} z)
endif()
