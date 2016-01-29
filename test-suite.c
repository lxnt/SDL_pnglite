#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "SDL_pnglite.h"
#include "SDL_image.h"


int test_load(const char *fname) {
    SDL_Surface *si_surf, *spl_surf, *stmp;
    int rv = 0;
    
    spl_surf = SDL_LoadPNG(fname);
    if (NULL == spl_surf) {
        printf("SDL_LoadPNG(%s): %s\n", fname , SDL_GetError());
        return 1;
    }
    si_surf = IMG_Load(fname);
    if (NULL == si_surf) {
        printf("IMG_Load(%s): %s\n", fname , SDL_GetError());
        SDL_FreeSurface(spl_surf);
        return 1;
    }
    if (spl_surf->format->format != si_surf->format->format) {
        if ((spl_surf->format->format == SDL_PIXELFORMAT_RGBA8888) &&
            (si_surf->format->format == SDL_PIXELFORMAT_ABGR8888)) {
            stmp = si_surf;
            si_surf = SDL_ConvertSurface(stmp, spl_surf->format, 0);
            SDL_FreeSurface(stmp);
            if (NULL == si_surf) {
                printf("surf convert failed.\n");
                SDL_FreeSurface(spl_surf);
                return 1;
            }
        } else {
            printf("%s: format doesnt match spl %s si %s\n", fname,
                SDL_GetPixelFormatName(spl_surf->format->format),
                SDL_GetPixelFormatName(si_surf->format->format));
            rv += 1;
            goto ret;
        }
    }

    if (spl_surf->w != si_surf->w) {
        printf("%s: w doesnt match spl %d si %d\n", fname, spl_surf->w, si_surf->w);
        rv += 1;
    }
    if (spl_surf->h != si_surf->h) {
        printf("%s: h doesnt match spl %d si %d\n", fname, spl_surf->h, si_surf->h);
        rv += 1;
    }
    if (spl_surf->pitch != si_surf->pitch) {
        printf("%s: pitch doesnt match spl %d si %d\n", fname, spl_surf->pitch, si_surf->pitch);
        rv += 1;
    }
    if (rv == 0) {
        int i, fails = 0, j, k;
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
            printf("%s: pixel data doesnt match (%d rows)\n", fname, fails);
            rv += fails;
        }
    }

 ret:
    SDL_FreeSurface(si_surf);
    SDL_FreeSurface(spl_surf);

    return rv;
}

int test_save(const char *fname) {
    return 0;
}

int main(int argc, char *argv[]) {
    int fails;    

    for (; argc > 1; argc--) {
        
        if ((fails = test_load(argv[argc-1])) > 0) {
            if (argv[argc-1][0] != 'x') {
                printf("%s: load: %d fails\n", argv[argc-1], fails);
            }
        } else {
            printf("%s: OK\n", argv[argc-1]);
        }
        continue;
        fflush(stdout);
        if ((fails = test_save(argv[argc-1])) > 0) {
            printf("%s: save: %d fails\n", argv[argc-1], fails);
        }

    }
    return 0;
}

