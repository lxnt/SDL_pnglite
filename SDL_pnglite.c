/*
  SDL_pnglite: An example PNG loader/writer for adding to SDL

  Copyright (C) 2012-2016 Alexander Sabourenkov <llxxnntt@gmail.com>

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
#include "SDL_stdinc.h"

#include "pnglite.h"

static unsigned
rwops_read_wrapper(void* buf, size_t size, size_t num, void* baton)
{
    SDL_RWops *rwops = (SDL_RWops *) baton;
    if (!buf) {
        if (SDL_RWseek(rwops, size * num, RW_SEEK_CUR) < 0) {
            SDL_Error(SDL_EFSEEK);
            return 0;
        }
        return num;
    }

    return SDL_RWread(rwops, buf, size, num);
}

static unsigned
rwops_write_wrapper(void* buf, size_t size, size_t num, void* baton)
{
    SDL_RWops *rwops = (SDL_RWops *) baton;

    if (!buf)
        return 0;

    return SDL_RWwrite(rwops, buf, size, num);
}

static unsigned char
bit_replicate(unsigned char val, unsigned bits)
{
    switch (bits) {
        case 1:
            return val*255;
        case 2:
            return val|(val<<2)|(val<<4)|(val<<6);
        case 4:
            return val|(val<<4);
        default:
            return val;
    }
}

/* determine if this paletted image can be loaded as paletted colorkeyed surface.
    returns -1 if not, index of the colorkey palette entry otherwise. */
static int
find_colorkey(png_t *p) {
    int alpha_zero_index = -1;
    unsigned alpha_one_count = 0;
    unsigned i, alpha;

    if ((p->color_type == PNG_INDEXED) && (p->transparency_present)) {
        for (i = 0; i < p->palette_size; i++) {
            alpha = p->palette[i + 768];
            switch (alpha) {
                case 255:
                    alpha_one_count += 1;
                    break;
                case 0:
                    alpha_zero_index = i;
                    break;
                default:
                    break;
            }
        }
        if (alpha_one_count == (p->palette_size - 1))
            return alpha_zero_index; /* the only transparent */
    }
    return -1;
}

SDL_Surface *
SDL_LoadPNG_RW(SDL_RWops * src, int freesrc)
{
    Sint64 fp_offset = 0;
    SDL_Surface *surface = NULL;
    SDL_Color colorset[256];
    SDL_Palette *palette = NULL;
    int rv;
    png_t png;
    Uint8  *data = NULL;
    int bpp = 32;
    Uint32 Rmask = 0;
    Uint32 Gmask = 0;
    Uint32 Bmask = 0;
    Uint32 Amask = 0;
    Uint64 row, col;
    Uint8 index;
    Uint8 gray_level;
    Uint8 alpha;
    Uint64 pixel; /* what if we get 3-gigapixel PNG ? */
    Uint8 *pitched_row;
    Uint8 *packed_row;
    Uint8 *pixel_start;
    Uint32 color;
    int colorkey; /* -1 = none */

    if (src == NULL) {
        SDL_SetError("Passed a NULL RWops");
        goto error;
    }

    /* this is perfectly safe to call multiple times
       as long as parameters don't change */
    png_init(SDL_malloc, SDL_free);

    fp_offset = SDL_RWtell(src);
    if (fp_offset == -1)
        goto error;

    rv = png_open_read(&png, rwops_read_wrapper, src);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_open_read(): %s", png_error_string(rv));
        goto error;
    }

    if (png.depth > 8) {
        /* support loading 16bpc by losing lsbs ? */
        SDL_SetError("depth %d is not supported", png.depth);
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
            surface = SDL_CreateRGBSurface(0, png.width, png.height, 32,
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

        case PNG_TRUECOLOR:
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
            surface = SDL_CreateRGBSurface(0, png.width, png.height, 24,
                                           Rmask, Gmask, Bmask, Amask);
            if (!surface) {
                goto error;
            }
            rv = png_get_data(&png, surface->pixels);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            for (row = png.height; row > 0; row --) {
                pitched_row = (Uint8 *) surface->pixels + (row-1) * surface->pitch;
                packed_row  = (Uint8 *) surface->pixels + (row-1) * png.pitch;
                SDL_memmove(pitched_row, packed_row, png.pitch);
            }
            if (png.transparency_present) {
                color = SDL_MapRGB(surface->format, png.colorkey[1],
                                    png.colorkey[3], png.colorkey[5]);
                SDL_SetColorKey(surface, SDL_TRUE, color);
            }
            goto done;

        case PNG_GREYSCALE:
            surface = SDL_CreateRGBSurface(0, png.width, png.height, 8,
                                            0, 0, 0, 0);
            if (!surface) {
                goto error;
            }
            /*  grayscale can be of any depth, and anything below 8
                gets expanded to 8, so use stride. */
            data = SDL_malloc(png.width * png.height * png.stride);
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

                pixel_start = (Uint8*)(surface->pixels) +
                                        row*surface->pitch + col;

                *pixel_start = bit_replicate(*(data + pixel), png.depth);
            }

            if (surface->pitch != surface->w) {
                for (row = png.height; row > 0; row --) {
                    pitched_row = (Uint8 *) surface->pixels + (row-1) * surface->pitch;
                    packed_row  = (Uint8 *) surface->pixels + (row-1) * png.width;
                    SDL_memmove(pitched_row, packed_row, png.width);
                }
            }

			gray_level = 0;
            do {
                colorset[gray_level].r = gray_level;
                colorset[gray_level].g = gray_level;
                colorset[gray_level].b = gray_level;
                colorset[gray_level].a = 255;
                gray_level += 1;
            } while (gray_level > 0);

            if (NULL == (palette = SDL_AllocPalette(256)))
                goto error;

            if (SDL_SetPaletteColors(palette, colorset, 0, 256))
                goto error;

            if (SDL_SetSurfacePalette(surface, palette))
                goto error;

            if (png.transparency_present) {
                gray_level = bit_replicate(png.colorkey[1], png.depth);
                SDL_SetColorKey(surface, SDL_TRUE, gray_level);
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
            /*  grayscale+alpha is always 2 bytes per pixel,
                so png.pitch is the right value */
            data = SDL_malloc(png.width * png.pitch);
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
                pixel_start = (Uint8 *) surface->pixels +
                                        row*surface->pitch + col*4;

                *pixel_start++ = alpha;
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
                *pixel_start++ = gray_level;
            }
            goto done;

        case PNG_INDEXED:
            /*  indexed always ends up as 8 bits per pixel. */
            data = SDL_malloc(png.width * png.height);
            if (!data) {
                SDL_OutOfMemory();
                goto error;
            }
            rv = png_get_data(&png, data);
            if (rv != PNG_NO_ERROR) {
                SDL_SetError("png_get_data(): %s", png_error_string(rv));
                goto error;
            }
            colorkey = find_colorkey(&png);
            if ((png.transparency_present) && (colorkey == -1)) {
                SDL_PixelFormatEnumToMasks(SDL_PIXELFORMAT_RGBA8888,
                                &bpp, &Rmask, &Gmask, &Bmask, &Amask);
                surface = SDL_CreateRGBSurface(0, png.width, png.height, bpp,
                                               Rmask, Gmask, Bmask, Amask);
                if (!surface) {
                    goto error;
                }
                for(pixel = 0 ; pixel < png.width * png.height ; pixel++) {
                    row = pixel / png.width;
                    col = pixel % png.width;
                    index = *(data + pixel);
                    pixel_start = (Uint8*)(surface->pixels) +
                                            row*surface->pitch + col*4;
                    *pixel_start++ = png.palette[768 + index]; /* A */
                    *pixel_start++ = png.palette[3*index + 2]; /* B */
                    *pixel_start++ = png.palette[3*index + 1]; /* G */
                    *pixel_start++ = png.palette[3*index + 0]; /* R */
                }
            } else {
                surface = SDL_CreateRGBSurface(0, png.width, png.height, 8,
                                                0, 0, 0, 0);
                if (!surface) {
                    goto error;
                }

                if (surface->pitch != surface->w) {
                    for (row = png.height; row > 0; row --) {
                        pitched_row = (Uint8 *) surface->pixels + (row-1) * surface->pitch;
                        packed_row  = data + (row-1) * png.width;
                        SDL_memcpy(pitched_row, packed_row, png.width);
                    }
                } else {
                    SDL_memcpy(surface->pixels, data, surface->w * surface->h);
                }

                for (col = 0; col < 256; col++) {
                    colorset[col].r = png.palette[3*col + 0];
                    colorset[col].g = png.palette[3*col + 1];
                    colorset[col].b = png.palette[3*col + 2];
                    colorset[col].a = 255;
                }
#if !defined(USE_PALETTE_API)
                SDL_memcpy(surface->format->palette->colors, colorset,
                                png.palette_size * sizeof(SDL_Color));
                surface->format->palette->ncolors = png.palette_size;
#else
                if (NULL == (palette = SDL_AllocPalette(256)))
                    goto error;

                if (SDL_SetPaletteColors(palette, colorset, 0, 256))
                    goto error;

                if (SDL_SetSurfacePalette(surface, palette))
                    goto error;
#endif

                if (colorkey != -1)
                    if (SDL_SetColorKey(surface, SDL_TRUE, colorkey))
                        goto error;
            }
            goto done;

        default:
            SDL_SetError("bogus color type %d", png.color_type);
            goto error;
    }

    goto done;

  error:
    if (src)
        SDL_RWseek(src, fp_offset, RW_SEEK_SET);

    if (surface)
        SDL_FreeSurface(surface);

    surface = NULL;

  done:
    if (palette)
        SDL_FreePalette(palette);

    if (data)
        SDL_free(data);

    if (freesrc && src)
        SDL_RWclose(src);

    return (surface);
}

static int
SDL_SavePNG32_RW(SDL_Surface * src, SDL_RWops * dst, int freedst)
{
    SDL_Surface *tmp = NULL;
    SDL_PixelFormat *format = NULL;
    png_t png;
    Uint8 *data = NULL;
    Uint8 png_color_type;
    int i;
    int rv;
    Uint32 unpitched_row_bytes;
    Uint32 pitched_row_bytes;
    Uint32 colorkey;
    int transparency_present = 0;

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
    if (0 == SDL_GetColorKey(tmp, &colorkey)) {
    /* SDL can have a colorkey on RGBA surfaces.
       Saving to PNG can not not lose this information. */
        if (png_color_type == PNG_TRUECOLOR) {
            SDL_GetRGB(colorkey, tmp->format,
                        &(png.colorkey[1]),
                        &(png.colorkey[3]),
                        &(png.colorkey[5]));
            png.colorkey[0] = 0;
            png.colorkey[2] = 0;
            png.colorkey[4] = 0;
            transparency_present = 1;
        }
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
        SDL_memcpy(data + unpitched_row_bytes * i,
                    (Uint8 *) tmp->pixels + pitched_row_bytes *i,
                    pitched_row_bytes);
    }

    /* write out and be done */
    rv = png_open_write(&png, rwops_write_wrapper, dst);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_open_write(): %s", png_error_string(rv));
        goto error;
    }

    rv = png_set_data(&png, tmp->w, tmp->h, 8, png_color_type, transparency_present, data);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_set_data(): %s", png_error_string(rv));
        goto error;
    }
    rv = 0;
    goto done;
  error:
    rv = -1;
  done:
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

    return rv;
}

int
SDL_SavePNG_RW(SDL_Surface * src, SDL_RWops * dst, int freedst)
{
    Uint8 *data, *ptr, *pixels;
    int i, j, rv;
    png_t png;
    int transparency_present = 0;
    Uint32 colorkey;

    if (!src || !dst) {
        SDL_SetError("Passed a NULL Surface or RWops");
        goto error;
    }

    /* this is perfectly safe to call multiple times
       as long as parameters don't change */
    png_init(SDL_malloc, SDL_free);

    switch (src->format->format) {
        case SDL_PIXELFORMAT_INDEX1LSB:
        case SDL_PIXELFORMAT_INDEX1MSB:
        case SDL_PIXELFORMAT_INDEX4LSB:
        case SDL_PIXELFORMAT_INDEX4MSB:
        case SDL_PIXELFORMAT_INDEX8:
            break;
        default:
            return SDL_SavePNG32_RW(src, dst, freedst);
    }

    data = SDL_malloc(src->w * src->h);
    if (!data) {
        SDL_OutOfMemory();
        goto error;
    }
    ptr = data;
    pixels = src->pixels;
    for (j = 0; j < src->h ; j++)
        for(i = 0; i < src->w; i++) {
            switch (src->format->format) {
                case SDL_PIXELFORMAT_INDEX1LSB:
                    switch (i%8) {
                        case 0:
                            *ptr = *(pixels + src->pitch*j + i/8) & 0x01;
                            break;
                        case 1:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x02 ) >> 1;
                            break;
                        case 2:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x04 ) >> 2;
                            break;
                        case 3:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x08 ) >> 3;
                            break;
                        case 4:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x10 ) >> 4;
                            break;
                        case 5:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x20 ) >> 5;
                            break;
                        case 6:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x40 ) >> 6;
                            break;
                        case 7:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x80 ) >> 7;
                            ptr += 1;
                            break;
                    }
                case SDL_PIXELFORMAT_INDEX1MSB:
                    switch (i%8) {
                        case 0:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x80 ) >> 7;
                            break;
                        case 1:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x40 ) >> 6;
                            break;
                        case 2:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x20 ) >> 5;
                            break;
                        case 3:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x10 ) >> 4;
                            break;
                        case 4:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x08 ) >> 3;
                            break;
                        case 5:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x04 ) >> 2;
                            break;
                        case 6:
                            *ptr = (*(pixels + src->pitch*j + i/8) & 0x02 ) >> 1;
                            break;
                        case 7:
                            *ptr = *(pixels + src->pitch*j + i/8) & 0x01;
                            ptr += 1;
                            break;
                    }
                case SDL_PIXELFORMAT_INDEX4LSB:
                    switch (i%2) {
                        case 0:
                           *ptr = *(pixels + src->pitch*j + i/8) & 0x0f;
                            break;
                        case 1:
                           *ptr = (*(pixels + src->pitch*j + i/8) & 0xf0) >> 4;
                            ptr += 1;
                            break;
                    }
                case SDL_PIXELFORMAT_INDEX4MSB:
                    switch (i%2) {
                        case 0:
                           *ptr = (*(pixels + src->pitch*j + i/8) & 0xf0) >> 4;
                            break;
                        case 1:
                           *ptr = *(pixels + src->pitch*j + i/8) & 0x0f;
                            ptr += 1;
                            break;
                    }
                case SDL_PIXELFORMAT_INDEX8:
                    *ptr = pixels[i + j*src->pitch];
                    ptr += 1;
                    break;
                default: /* should not ever happen here */
                    break;
            }
        }

    SDL_memset(png.palette, 255, 1024);
    png.palette_size = src->format->palette->ncolors;
    if (0 == SDL_GetColorKey(src, &colorkey)) {
        png.palette[4*colorkey + 3] = 0;
        transparency_present = 1;
    }

    for (i = 0; i < src->format->palette->ncolors ; i++) {
        png.palette[4*i + 0] = src->format->palette->colors[i].r;
        png.palette[4*i + 1] = src->format->palette->colors[i].g;
        png.palette[4*i + 2] = src->format->palette->colors[i].b;
    }

    /* write out and be done */
    rv = png_open_write(&png, rwops_write_wrapper, dst);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_open_write(): %s", png_error_string(rv));
        goto error;
    }

    rv = png_set_data(&png, src->w, src->h, 8, PNG_INDEXED, transparency_present, data);
    if (rv != PNG_NO_ERROR) {
        SDL_SetError("png_set_data(): %s", png_error_string(rv));
        goto error;
    }

    rv = 0;
    goto done;
  error:
    rv = -1;
  done:
    if (data)
        SDL_free(data);

    if (freedst && dst)
        SDL_RWclose(dst);

    return rv;
}
