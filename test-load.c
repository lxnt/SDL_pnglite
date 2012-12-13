#include <stdio.h>
#include "SDL.h"
#include "SDL_pnglite.h"


int main(int argc, char *argv[]) {
    SDL_Surface *s;
    char fname[4096];
    
    for (; argc > 1; argc--) {
        printf("doing %s\n", argv[argc-1]);
        fflush(stdout);
        s = SDL_LoadPNG(argv[argc-1]);
        if (!s) {
            printf("SDL_LoadPNG(%s): %s\n", argv[argc-1], SDL_GetError());
            continue;
        }
        strcpy(fname, "out/");
        strcat(fname, argv[argc-1]);
        strcat(fname, ".bmp");
        if (SDL_SaveBMP(s, fname))
            printf("SDL_SaveBMP(...,%s): %s\n", fname, SDL_GetError());
        else
            printf("converted %s -> %s\n", argv[argc-1], fname);
        SDL_FreeSurface(s);
    }
    return 0;
}

