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
  pkg_check_modules(PKG_SDL2 REQUIRED zlib)
  pkg_check_modules(PKG_SDL2IMAGE SDL2_image)
  if (PKG_SDL2IMAGE_FOUND)
    set(HAVE_SDLIMAGE2 ON)
  endif(PKG_SDL2IMAGE_FOUND)
endif(CMAKE_HOST_UNIX)

if (MSVC)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

  set(PKG_SDL2_INCLUDE_DIRS "" CACHE PATH "Location of SDL2 include files")
  set(PKG_SDL2_LIBRARY_DIRS "/none" CACHE PATH "Location of SDL2 library files")
  set(PKG_SDL2IMAGE_INCLUDE_DIRS "/none" CACHE PATH "Location of SDL2_image include files")
  set(PKG_SDL2IMAGE_LIBRARY_DIRS "/none" CACHE PATH "Location of SDL2_image library files")
  set(PKG_ZLIB_INCLUDE_DIRS "/none" CACHE PATH "Location of zlib include files")
  set(PKG_ZLIB_LIBRARY_DIRS "/none" CACHE PATH "Location of zlib library files")

  set(PKG_SDL2_LIBRARIES SDL2 SDL2main)
  set(PKG_SDL2IMAGE_LIBRARIES SDL2_image)
  set(PKG_ZLIB_LIBRARIES zdll)
endif(MSVC)

include_directories(${PKG_SDL2_INCLUDE_DIRS} ${PKG_SDL2IMAGE_INCLUDE_DIRS} ${PKG_ZLIB_INCLUDE_DIRS} )
link_directories(${PKG_SDL2_LIBRARY_DIRS} ${PKG_SDL2IMAGE_LIBRARY_DIRS} ${PKG_ZLIB_LIBRARY_DIRS} )

add_library(SDL_pnglite STATIC SDL_pnglite.c pnglite.c)
target_link_libraries(SDL_pnglite PRIVATE ${PKG_SDL2_LIBRARIES} ${PKG_ZLIB_LIBRARIES})

install(TARGETS SDL_pnglite ARCHIVE DESTINATION lib)
install(FILES  SDL_pnglite.h DESTINATION include/SDL2)

option(BUILD_TEST_SUITE "Build test-suite" ${HAVE_SDLIMAGE2})
if (BUILD_TEST_SUITE)
  add_executable(pnglite-test test-suite.c)
  target_link_libraries(pnglite-test PRIVATE SDL_pnglite ${PKG_SDL2_LIBRARIES} ${PKG_SDL2IMAGE_LIBRARIES} ${PKG_ZLIB_LIBRARIES})
  if (MSVC) # hmm. does ming32-w64 fall into this trap too?
    set_target_properties(pnglite-test PROPERTIES LINK_FLAGS "setargv.obj")
  endif(MSVC)
endif(BUILD_TEST_SUITE)
