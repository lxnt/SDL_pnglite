#include <stdlib.h>
#include <stdio.h>

#include "pnglite.h"

int main(int argc, char *argv[]) {
    int i, rv;
    void *data;
    png_t png;
    
    
    
    for (i = 1; i < argc; i++) {
        
        rv = png_open_file_read(&png, argv[i]);
        printf("png_open_file_read(%s): %s \n", argv[i], png_error_string(rv));
        if (!rv) {
            data = malloc(png.width*png.height);
            rv = png_get_data(&png, data);
            printf("png_get_data(%s): %s \n", argv[i], png_error_string(rv));
            free(data);
        }
        
        rv = png_close_file(&png);
        printf("png_close_file(%s): %s \n", argv[i], png_error_string(rv));
    
        

    }
    return 0;
}

