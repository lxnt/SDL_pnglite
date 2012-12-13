/*
  SDL_pnglite: An example PNG loader/writer for adding to SDL

  Copyright (C) 2012 Alexander Sabourenkov <screwdriver@lxnt.info>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_pnglite.h
 *  
 *  Header file for pnglite library wrapper.
 */
#ifndef _SDL_pnglite_h
#define _SDL_pnglite_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  Load a surface from a seekable SDL data stream (memory or file).
 *  
 *  If \c freesrc is non-zero, the stream will be closed after being read.
 *  
 *  The new surface should be freed with SDL_FreeSurface().
 *  
 *  \return the new surface, or NULL if there was an error.
 */
extern DECLSPEC SDL_Surface *SDLCALL SDL_LoadPNG_RW(SDL_RWops * src,
                                                    int freesrc);

/**
 *  Load a surface from a file.
 *  
 *  Convenience macro.
 */
#define SDL_LoadPNG(surface, file) \
                SDL_LoadPNG_RW(SDL_RWFromFile(file, "rb"), 1)

/**
 *  Save a surface to a seekable SDL data stream (memory or file).
 *  
 *  If \c freedst is non-zero, the stream will be closed after being written.
 *  
 *  \return 0 if successful or -1 if there was an error.
 */
extern DECLSPEC int SDLCALL SDL_SavePNG_RW
    (SDL_Surface * surface, SDL_RWops * dst, int freedst);

/** 
 *  Save a surface to a file.
 *  
 *  Convenience macro.
 */
#define SDL_SavePNG(surface, file) \
                SDL_SavePNG_RW(surface, SDL_RWFromFile(file, "wb"), 1)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"


#endif