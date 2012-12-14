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

#include "SDL_video.h"
#include "SDL_endian.h"
#include "SDL_pixels.h"
/*#include "SDL_error.h"*/

#include <pnglite.h>

static unsigned
rwops_read_wrapper(void* buf, size_t size, size_t num, void* baton)
{
    SDL_RWops *rwops = (SDL_RWops *) baton;
    if (!buf) {
        if (SDL_RWseek(rwops, size * num, RW_SEEK_CUR) < 0) {
            SDL_Error(SDL_EFSEEK);
            return 0;
        }
        return size * num;
    }

    return SDL_RWread(rwops, buf, size, num);
}

static unsigned
rwops_write_wrapper(void* buf, size_t size, size_t num, void* baton)
{
    SDL_RWops *rwops = (SDL_RWops *) baton;
    size_t rv;
    if (!buf)
        return 0;

    rv = SDL_RWwrite(rwops, buf, size, num);
    if (rv != size*num)
        SDL_Error(SDL_EFWRITE);
    return 0;
}

SDL_Surface *
SDL_LoadPNG_RW(SDL_RWops * src, int freesrc)
{
    Sint64 fp_offset = 0;
    SDL_Surface *surface = NULL;
    int rv;
    png_t png;
    Uint8  *data = NULL;
    int bpp = 32;
    Uint32 Rmask = 0;
    Uint32 Gmask = 0;
    Uint32 Bmask = 0;
    Uint32 Amask = 0;
    int row;
    int col;
    Uint8 gray_level;
    Uint8 alpha;
    Uint64 pixel; /* what if we get 3-gigapixel PNG ? */
    Uint8 *pitched_row;
    Uint8 *packed_row;
    Uint8 *pixel_start;
    Uint32 row_bytes;

    if (src == NULL) {
        goto error;
    }

    /* this is perfectly safe to call multiple times
       as long as parameters don't change */
    png_init(SDL_malloc, SDL_free);

    fp_offset = SDL_RWtell(src);
    SDL_ClearError();

    rv = png_open_read(&png, rwops_read_wrapper, src);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_open_read(): %s", png_error_string(rv));
        goto error;
    }

    if (png.bpp > 4) {
        /* support loading 16bpc by losing lsbs ? */
        SDL_SetError("pnglite: %d bytes per pixel not supported", png.bpp);
        goto error;
    }

    switch (png.color_type) {
        case PNG_TRUECOLOR_ALPHA:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            Rmask = 0xFF000000;
            Gmask = 0x00FF0000;
            Bmask = 0x0000FF00;
            Amask = 0x000000FF;
#else
            Rmask = 0x000000FF;
            Gmask = 0x0000FF00;
            Bmask = 0x00FF0000;
            Amask = 0xFF000000;
#endif
            /* should be no problems with pitch */
            surface = SDL_CreateRGBSurface(0, png.width, png.height, bpp,
                                           Rmask, Gmask, Bmask, Amask);
            if (!surface) {
                goto error;
            }
            rv = png_get_data(&png, surface->pixels);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            goto done;

       case  PNG_TRUECOLOR:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            Amask = 0x00000000;
#else
            Rmask = 0x000000FF;
            Gmask = 0x0000FF00;
            Bmask = 0x00FF0000;
            Amask = 0x00000000;
#endif
            bpp = 24;
        /* have to memmove rows to account for the pitch */
            surface = SDL_CreateRGBSurface(0, png.width, png.height, bpp,
                                           Rmask, Gmask, Bmask, Amask);
            if (!surface) {
                goto error;
            }
            rv = png_get_data(&png, surface->pixels);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            row_bytes = surface->w * png.bpp;
            for (row = png.height - 1; row >= 0; row --) {
                pitched_row = surface->pixels + row * surface->pitch;
                packed_row  = surface->pixels + row * row_bytes;
                SDL_memmove(pitched_row, packed_row, row_bytes);
            }
            goto done;

        case PNG_GREYSCALE:
            SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGB24,
                            &bpp, &Rmask, &Gmask, &Bmask, &Amask);
            surface = SDL_CreateRGBSurface(0, png.width, png.height, bpp,
                                           Rmask, Gmask, Bmask, Amask);
            if (!surface) {
                goto error;
            }
            data = SDL_malloc(png.width * png.height * png.bpp);
            if (!data) {
                SDL_OutOfMemory();
                goto error;
            }
            rv = png_get_data(&png, data);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            for(pixel = 0 ; pixel < png.width * png.height ; pixel++) {
                gray_level = *(data + pixel);
                row = pixel / png.width;
                col = pixel % png.width;
                pixel_start = (Uint8*)(surface->pixels) + 
                                        row*surface->pitch + col*3;
                
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
            }
            goto done;

        case PNG_GREYSCALE_ALPHA:
            SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888,
                            &bpp, &Rmask, &Gmask, &Bmask, &Amask);
            surface = SDL_CreateRGBSurface(0, png.width, png.height, bpp,
                                           Rmask, Gmask, Bmask, Amask);
            if (!surface) {
                goto error;
            }
            data = SDL_malloc(png.width * png.height * png.bpp);
            if (!data) {
                SDL_OutOfMemory();
                goto error;
            }
            rv = png_get_data(&png, data);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            for(pixel = 0 ; pixel < png.width * png.height ; pixel++) {
                row = pixel / png.width;
                col = pixel % png.width;
                gray_level = *(data + 2*pixel);
                alpha = *(data + 2*pixel + 1);
                pixel_start = (Uint8*)(surface->pixels) + 
                                        row*surface->pitch + col*4;
                
                *pixel_start++ = alpha;
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
            }
            goto done;

        default:
        /* the four indexed formats are not supported by pnglite
           at the moment, we won't get here, but error out just in case */
            SDL_SetError("pnglite: color type %d not supported",
                            png.color_type);
            goto error;
    }

    goto done;

  error:
    if (src) {
        SDL_RWseek(src, fp_offset, RW_SEEK_SET);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
    surface = NULL;

  done:
    if (data) {
        SDL_free(data);
    }
    if (freesrc && src) {
        SDL_RWclose(src);
    }
    return (surface);
}

int
SDL_SavePNG_RW(SDL_Surface * src, SDL_RWops * dst, int freedst)
{
    SDL_Surface *tmp = NULL;
    SDL_PixelFormat *format;
    png_t png;
    Uint8 *data = NULL;
    Uint8 png_color_type;
    int i;
    int rv;
    Uint32 unpitched_row_bytes;
    Uint32 pitched_row_bytes;

    /* this is perfectly safe to call multiple times
       as long as parameters don't change */
    png_init(SDL_malloc, SDL_free);

    if (!src || !dst) {
        SDL_SetError("EINVAL");
        goto error;
    }

    if (src->format->Amask > 0) {
        format = SDL_AllocFormat(
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                        SDL_PIXELFORMAT_ABGR8888
#else
                        SDL_PIXELFORMAT_RGBA8888
#endif
                        );
        png_color_type = PNG_TRUECOLOR_ALPHA;
    } else {
        format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB24);
        png_color_type = PNG_TRUECOLOR;
    }
    if (!format) {
        goto error;
    }
    tmp = SDL_ConvertSurface(src, format, 0);
    if (!tmp) {
        SDL_SetError("Couldn't convert image to %s",
                     SDL_GetPixelFormatName(format->format));
        goto error;
    }

    unpitched_row_bytes = tmp->w * tmp->format->BytesPerPixel;
    pitched_row_bytes = tmp->pitch;
    data = SDL_malloc(unpitched_row_bytes * tmp->h);
    if (!data) {
        SDL_OutOfMemory();
        goto error;
    }
    /* now get rid of pitch */
    for(i = 0; i < tmp->h ; i++) {
        SDL_memmove(data + unpitched_row_bytes * i,
                    tmp->pixels + pitched_row_bytes *i,
                    pitched_row_bytes);
    }
    /* write out and be done */
    rv = png_open_write(&png, rwops_write_wrapper, dst);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_open_write(): %s", png_error_string(rv));
        goto error;
    }

    rv = png_set_data(&png, tmp->w, tmp->h, 8, png_color_type, data);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_set_data(): %s", png_error_string(rv));
        goto error;
    }

  error:
    if (format) {
        SDL_FreeFormat(format);
    }
    if (tmp) {
        SDL_FreeSurface(tmp);
    }
    if (data) {
        SDL_free(data);
    }
    if (freedst && dst) {
        SDL_RWclose(dst);
    }

    return SDL_strlen(SDL_GetError()) ? -1 : 0;
}
