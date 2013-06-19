#include <stdio.h>
#include "SDL.h"
#include "SDL_pnglite.h"


int main(int argc, char *argv[]) {
    SDL_Surface *s;
    char fname[4096];
    int col;
    SDL_Color palette[256];

    for (; argc > 1; argc--) {
        printf("doing %s\n", argv[argc-1]);
        fflush(stdout);
        s = SDL_CreateRGBSurface(0, 33, 33, 8, 0, 0, 0, 0);
        if (!s) {
            printf("SDL_CreateRGBSurface(): %s\n", SDL_GetError());
            continue;
        }

        for (col = 0; col < 256; col++) {
            palette[col].r = col;
            palette[col].g = col;
            palette[col].b = col;
            palette[col].a = col;
            printf("%d ", col);
        }
        puts("\n");
        if (SDL_SetPaletteColors(s->format->palette, palette, 0, 256))
            printf("SDL_CreateRGBSurface(): %s\n", SDL_GetError());
        
        strcpy(fname, "out/");
        strcat(fname, argv[argc-1]);
        strcat(fname, ".bmp");
        if (SDL_SaveBMP(s, fname))
            printf("SDL_SaveBMP(...,%s): %s\n", fname, SDL_GetError());
        else
            printf("saved %s\n", fname);
        SDL_FreeSurface(s);
    }
    return 0;
}

