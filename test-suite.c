#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "SDL.h"
#include "SDL_pnglite.h"
#include "SDL_image.h"

#define DUMP
void dump_rwo(const char *fname, const void *rwo_buf, SDL_RWops *rwo, const size_t sz) {
#if defined(DUMP)
    FILE *fp;
    char *fncopy, *bn;
    printf("rwo: size=%d len=%d\n", (int)SDL_RWsize(rwo), (int)sz);
    fncopy = strdup(fname);
    bn = basename(fncopy);
    if (NULL != (fp = fopen(bn, "w"))) {
        fwrite(rwo_buf, sz, 1, fp);
        fclose(fp);
        printf("wrote %s\n", bn);
    } else {
        printf("can't write %s\n", bn);
    }
    /* exit(1); */
#else
    rwo += sz;
#endif
}

int compare_surfaces(const char *fname, SDL_Surface *si_surf, SDL_Surface *spl_surf) {
    SDL_Surface *stmp = NULL;
    int rv = 0, do_free_si_surf = 0, pixel_format_mismatch = 0;

    if (spl_surf->format->format != si_surf->format->format) {
        if ((spl_surf->format->format == SDL_PIXELFORMAT_RGBA8888) &&
            (si_surf->format->format == SDL_PIXELFORMAT_ABGR8888)) {
            stmp = si_surf;
            si_surf = SDL_ConvertSurface(stmp, spl_surf->format, 0);
            if (NULL == si_surf) {
                printf("surf convert failed.\n");
                return 1;
            }
            do_free_si_surf = 1;
        } else {
            printf("%s: pixel format mismatch spl %s si %s\n", fname,
                SDL_GetPixelFormatName(spl_surf->format->format),
                SDL_GetPixelFormatName(si_surf->format->format));
            rv = 1;
            pixel_format_mismatch = 1;
        }
    }

    if (spl_surf->w != si_surf->w) {
        printf("%s: w mismatch spl %d si %d\n", fname, spl_surf->w, si_surf->w);
        rv += 1;
    }
    if (spl_surf->h != si_surf->h) {
        printf("%s: h mismatch spl %d si %d\n", fname, spl_surf->h, si_surf->h);
        rv += 1;
    }
    if (spl_surf->pitch != si_surf->pitch) {
        printf("%s: pitch mismatch spl %d si %d\n", fname, spl_surf->pitch, si_surf->pitch);
        rv += 1;
    }
    if (pixel_format_mismatch)
        goto done;

    if (spl_surf->format->palette) {
        if (spl_surf->format->palette->ncolors != si_surf->format->palette->ncolors) {
            printf("%s: palette ncolors mismatch spl %d si %d\n", fname,
                    spl_surf->format->palette->ncolors, si_surf->format->palette->ncolors);
            rv += 1;
        }
        {
            int ci, cm = 0;
            for (ci = 0; ci < si_surf->format->palette->ncolors; ci++) {
                if (ci == spl_surf->format->palette->ncolors)
                    break;

                if ((spl_surf->format->palette->colors[ci].r != si_surf->format->palette->colors[ci].r) ||
                    (spl_surf->format->palette->colors[ci].g != si_surf->format->palette->colors[ci].g) ||
                    (spl_surf->format->palette->colors[ci].b != si_surf->format->palette->colors[ci].b))
                        cm += 1;
            }
            if (cm > 0) {
                printf("%s: palette mismatch %d colors\n", fname, cm);
                rv += cm;
            }
        }
    }
    {
        Uint32 spl_key = 0, si_key = 42;
        int si_rv, spl_rv;
        spl_rv = SDL_GetColorKey(spl_surf, &spl_key);
        si_rv = SDL_GetColorKey(si_surf, &si_key);
        if (si_rv != spl_rv) {
            printf("%s: SDL_GetColorKey() rv mismatch spl %d si %d \n", fname, spl_rv, si_rv);
            rv += 1;
        } else {
            if ((si_rv == 0) && (spl_key != si_key)) {
                printf("%s: colorkey mismatch spl %x si %x\n", fname, spl_key, si_key);
                rv += 1;
            }
        }
    }

    {
        int i, fails = 0, j;
        Uint8 *spl_row = spl_surf->pixels;
        Uint8 *si_row = si_surf->pixels;
        for (i = 0; i < spl_surf->h ; i++) {
            if ( 0 != memcmp(spl_row + i*spl_surf->pitch, si_row +  i*si_surf->pitch, si_surf->pitch)) {
                fails += 1;
                printf("spl: ");
                for(j=0; j<spl_surf->pitch; j++)
                    printf("%hhx ", *(spl_row + i*spl_surf->pitch + j));
                printf("\n");
                printf("si: ");
                for(j=0; j<si_surf->pitch; j++)
                    printf("%hhx ", *(si_row + i*si_surf->pitch + j));
                printf("\n");
            }
        }
        if (fails > 0) {
            printf("%s: pixel data mismatch (%d rows)\n", fname, fails);
            rv += fails;
        }
    }

  done:
    if (do_free_si_surf)
        SDL_FreeSurface(si_surf);
    return rv;
}

int test_load(const char *fname) {
    SDL_Surface *si_surf, *spl_surf;
    int rv = 0;

    spl_surf = SDL_LoadPNG(fname);
    if (NULL == spl_surf) {
        printf("SDL_LoadPNG(%s): %s\n", fname , SDL_GetError());
        return 1;
    }
#if defined(LOUD)
printf("w %d h %d pitch %d pf %s\n",
    spl_surf->w, spl_surf->h, spl_surf->pitch, SDL_GetPixelFormatName(spl_surf->format->format));
#endif
    si_surf = IMG_Load(fname);
    if (NULL == si_surf) {
        printf("IMG_Load(%s): %s\n", fname , SDL_GetError());
        SDL_FreeSurface(spl_surf);
        return 1;
    }

    rv += compare_surfaces(fname, si_surf, spl_surf);

    SDL_FreeSurface(si_surf);
    SDL_FreeSurface(spl_surf);

    return rv;
}

int test_save(const char *fname) {
    SDL_Surface *si_surf = NULL, *spl_surf = NULL;
    SDL_RWops *rwo = NULL;
    void *rwo_buf = NULL;
    const int rwo_buf_sz = 1048576;
    int rv = 0;
    size_t sz;

    spl_surf = SDL_LoadPNG(fname);
    if (NULL == spl_surf) {
        printf("SDL_LoadPNG(%s): %s\n", fname , SDL_GetError());
        rv = 1;
        goto exit;
    }
#if defined(LOUD)
printf("w %d h %d pitch %d pf %s\n",
    spl_surf->w, spl_surf->h, spl_surf->pitch, SDL_GetPixelFormatName(spl_surf->format->format));
#endif
    rwo_buf = SDL_malloc(rwo_buf_sz);
    if (NULL == rwo_buf) {
        printf("SDL_malloc(): %s\n", SDL_GetError());
        return 1;
    }
    rwo = SDL_RWFromMem(rwo_buf, rwo_buf_sz);
    if (NULL == rwo) {
        printf("SDL_RWFromMem(): %s\n", SDL_GetError());
        rv = 1;
        goto exit;
    }

    if (-1 == SDL_SavePNG_RW(spl_surf, rwo, 0)) {
        printf("%s: SDL_SavePNG_RW(): %s\n", fname, SDL_GetError());
        rv = 1;
        goto exit;
    }
    sz = SDL_RWtell(rwo);
    SDL_RWseek(rwo, 0, RW_SEEK_SET);

    si_surf = IMG_LoadPNG_RW(rwo);
    if (NULL == si_surf) {
        printf("IMG_Load(%s): %s\n", fname , SDL_GetError());
        dump_rwo(fname, rwo_buf, rwo, sz);
        rv = 1;
        goto exit;
    }

    rv += compare_surfaces(fname, si_surf, spl_surf);
    if (rv)
        dump_rwo(fname, rwo_buf, rwo, sz);

  exit:
    if (rwo)
        SDL_FreeRW(rwo);
    if (rwo_buf)
        SDL_free(rwo_buf);
    if (spl_surf)
        SDL_FreeSurface(spl_surf);
    if (si_surf)
        SDL_FreeSurface(si_surf);
    return rv;
}

int gotta_fail(const char *fn) {
    char *fncopy, *bn;
    int rv;

    fncopy = strdup(fn);
    bn = basename(fncopy);

    rv = (bn[0] == 'x');
    free(fncopy);
    return rv;
}

int main(int argc, char *argv[]) {
    int fails, i;
    char *fname;

    for (i = 1; i < argc; i++) {
        fname = argv[i];
        fails = test_load(fname);
        if (fails == 0) {
            if (gotta_fail(fname)) {
                printf("%s: loaded corrupted file.\n", fname);
            } else {
                printf("%s: OK\n", fname);
            }
        }
    }
    printf("========================================\n");
    for (i = 1; i < argc; i++) {
        fname = argv[i];
        fails = test_save(fname);
        if (fails == 0) {
            if (gotta_fail(fname)) {
                printf("%s: loaded corrupted file.\n", fname);
            } else {
                printf("%s: OK\n", fname);
            }
        }
    }
    IMG_Quit();
    SDL_Quit();
    return 0;
}

