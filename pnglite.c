/*  pnglite.c - pnglite library
    For conditions of distribution and use, see copyright notice in pnglite.h
*/
#define DO_CRC_CHECKS 1
#define USE_ZLIB 1

#if USE_ZLIB
#include <zlib.h>
#else
#include "zlite.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pnglite.h"



static png_alloc_t png_alloc;
static png_free_t png_free;

static size_t file_read(png_t* png, void* out, size_t size, size_t numel)
{
	size_t result;
	if(png->read_fun)
	{
		result = png->read_fun(out, size, numel, png->user_pointer);
	}
	else
	{
		if(!out)
		{
			result = fseek(png->user_pointer, (long)(size*numel), SEEK_CUR);
		}
		else
		{
			result = fread(out, size, numel, png->user_pointer);
		}
	}

	return result;
}

static size_t file_write(png_t* png, void* p, size_t size, size_t numel)
{
	size_t result;

	if(png->write_fun)
	{
		result = png->write_fun(p, size, numel, png->user_pointer);
	}
	else
	{
		result = fwrite(p, size, numel, png->user_pointer);
	}

	return result;
}

static int file_read_ul(png_t* png, unsigned *out)
{
	unsigned char buf[4];

	if(file_read(png, buf, 1, 4) != 4)
		return PNG_FILE_ERROR;

	*out = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];

	return PNG_NO_ERROR;
}

static int file_write_ul(png_t* png, unsigned in)
{
	unsigned char buf[4];

	buf[0] = (in>>24) & 0xff;
	buf[1] = (in>>16) & 0xff;
	buf[2] = (in>>8) & 0xff;
	buf[3] = (in) & 0xff;

	if(file_write(png, buf, 1, 4) != 4)
		return PNG_FILE_ERROR;

	return PNG_NO_ERROR;
}
	

static unsigned get_ul(unsigned char* buf)
{
	unsigned result;
	unsigned char foo[4];

	memcpy(foo, buf, 4);

	result = (foo[0]<<24) | (foo[1]<<16) | (foo[2]<<8) | foo[3];

	return result;
}

static unsigned set_ul(unsigned char* buf, unsigned in)
{
	buf[0] = (in>>24) & 0xff;
	buf[1] = (in>>16) & 0xff;
	buf[2] = (in>>8) & 0xff;
	buf[3] = (in) & 0xff;

	return PNG_NO_ERROR;
}

int png_init(png_alloc_t pngalloc, png_free_t pngfree)
{
	if(pngalloc)
		png_alloc = pngalloc;
	else
		png_alloc = &malloc;

	if(pngfree)
		png_free = pngfree;
	else
		png_free = &free;

	return PNG_NO_ERROR;
}

static int pot_align(int value, int pot)
{
    return (value + (1<<pot) - 1) & ~((1<<pot) -1);
}

static int png_check_png(png_t* png)
{
    int channels[] = { 1, 0, 3, 1, 2, 0, 4 };

    switch(png->depth)
    {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        case 16:
            return PNG_NOT_SUPPORTED;
        default:
            return PNG_CORRUPTED_ERROR;
    }

    switch(png->color_type)
    {
        case PNG_GREYSCALE:
            break;
        case PNG_TRUECOLOR:
        case PNG_GREYSCALE_ALPHA:
        case PNG_TRUECOLOR_ALPHA:
            if (png->depth < 8)
                return PNG_CORRUPTED_ERROR;
            break;
        case PNG_INDEXED:
            if (png->depth > 8)
                return PNG_CORRUPTED_ERROR;
            break;
        default:
            return PNG_CORRUPTED_ERROR;
    }

    /* bytes per pixel or one */
    png->stride = pot_align(png->depth * channels[png->color_type], 3) >> 3;

    /* bytes per scanline */
    png->pitch = pot_align(png->width * png->depth * channels[png->color_type], 3) >> 3;

    if(png->interlace_method)
        return PNG_NOT_SUPPORTED;

    return PNG_NO_ERROR;
}

static unsigned png_calc_crc(char *name, unsigned char *chunk, unsigned length)
{
    unsigned crc;

    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (unsigned char *)name, 4);
    crc = crc32(crc, chunk, length);

    return crc;
}

static int png_read_check_crc(png_t *png, char *name, unsigned char *chunk, unsigned length)
{
    unsigned crc;

    if (file_read_ul(png, &crc) != PNG_NO_ERROR)
        return PNG_EOF_ERROR;

    if(crc != png_calc_crc(name, chunk, length))
        return PNG_CRC_ERROR;

    return PNG_NO_ERROR;
}

static int png_calc_write_crc(png_t *png, char *name, unsigned char *chunk, unsigned length)
{
    unsigned crc;

    crc = png_calc_crc(name, chunk, length);

    if (file_write_ul(png, crc) != PNG_NO_ERROR)
        return PNG_IO_ERROR;

    return PNG_NO_ERROR;
}

static int png_read_ihdr(png_t* png)
{
	unsigned length;
	unsigned char ihdr[13+4];		 /* length should be 13, make room for type (IHDR) */

	file_read_ul(png, &length);

	if(length != 13)
	{
		return PNG_CRC_ERROR;
	}

	if(file_read(png, ihdr, 1, 13+4) != 13+4)
		return PNG_EOF_ERROR;

        if (png_read_check_crc(png, "IHDR", ihdr+4, 13) != PNG_NO_ERROR)
            return PNG_CRC_ERROR;

	png->width = get_ul(ihdr+4);
	png->height = get_ul(ihdr+8);
	png->depth = ihdr[12];
	png->color_type = ihdr[13];
	png->compression_method = ihdr[14];
	png->filter_method = ihdr[15];
	png->interlace_method = ihdr[16];

	return png_check_png(png);
}

static int png_write_ihdr(png_t* png)
{
	unsigned char ihdr[13+4];
	unsigned char *p = ihdr;
	
	file_write(png, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 1, 8);

	file_write_ul(png, 13);
    
	*p = 'I';			p++;
	*p = 'H';			p++;
	*p = 'D';			p++;
	*p = 'R';			p++;
	set_ul(p, png->width);	p+=4;
	set_ul(p, png->height);	p+=4;
	*p = png->depth;			p++;
	*p = png->color_type;	p++;
	*p = 0;				p++;
	*p = 0;				p++;
	*p = 0;				p++;

	file_write(png, ihdr, 1, 13+4);

    return png_calc_write_crc(png, "IHDR", ihdr + 4, 13);
}

void png_print_info(png_t* png)
{
	printf("PNG INFO:\n");
	printf("\twidth:\t\t%d\n", png->width);
	printf("\theight:\t\t%d\n", png->height);
	printf("\tdepth:\t\t%d\n", png->depth);
	printf("\tcolor:\t\t");

	switch(png->color_type)
	{
	case PNG_GREYSCALE:			printf("greyscale\n"); break;
	case PNG_TRUECOLOR:			printf("truecolor\n"); break;
	case PNG_INDEXED:			printf("palette\n"); break;
	case PNG_GREYSCALE_ALPHA:	printf("greyscale with alpha\n"); break;
	case PNG_TRUECOLOR_ALPHA:	printf("truecolor with alpha\n"); break;
	default:					printf("unknown, this is not good\n"); break;
	}

	printf("\tcompression:\t%s\n",	png->compression_method?"unknown, this is not good":"inflate/deflate");
	printf("\tfilter:\t\t%s\n",		png->filter_method?"unknown, this is not good":"adaptive");
	printf("\tinterlace:\t%s\n",	png->interlace_method?"interlace":"no interlace");
}

int png_open_read(png_t* png, png_read_callback_t read_fun, void* user_pointer)
{
	char header[8];
	int result;

	png->read_fun = read_fun;
	png->write_fun = 0;
	png->user_pointer = user_pointer;

	if(!read_fun && !user_pointer)
		return PNG_WRONG_ARGUMENTS;

	if(file_read(png, header, 1, 8) != 8)
		return PNG_EOF_ERROR;

	if(memcmp(header, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) != 0)
		return PNG_HEADER_ERROR;

	result = png_read_ihdr(png);

	return result;
}

int png_open_write(png_t* png, png_write_callback_t write_fun, void* user_pointer)
{
	png->write_fun = write_fun;
	png->read_fun = 0;
	png->user_pointer = user_pointer;

	if(!write_fun && !user_pointer)
		return PNG_WRONG_ARGUMENTS;

	return PNG_NO_ERROR;
}

int png_open(png_t* png, png_read_callback_t read_fun, void* user_pointer)
{
	return png_open_read(png, read_fun, user_pointer);
}

int png_open_file_read(png_t *png, const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if(!fp)
		return PNG_FILE_ERROR;

	return png_open_read(png, 0, fp);
}

int png_open_file_write(png_t *png, const char* filename)
{
	FILE* fp = fopen(filename, "wb");

	if(!fp)
		return PNG_FILE_ERROR;

	return png_open_write(png, 0, fp);
}

int png_open_file(png_t *png, const char* filename)
{
	return png_open_file_read(png, filename);
}

int png_close_file(png_t* png)
{
	fclose(png->user_pointer);

	return PNG_NO_ERROR;
}

static int png_init_deflate(png_t* png, unsigned char* data, int datalen)
{
	z_stream *stream;
	png->zs = png_alloc(sizeof(z_stream));

	stream = png->zs;

	if(!stream)
		return PNG_MEMORY_ERROR;

	memset(stream, 0, sizeof(z_stream));

	if(deflateInit(stream, Z_DEFAULT_COMPRESSION) != Z_OK)
		return PNG_ZLIB_ERROR;

	stream->next_in = data;
	stream->avail_in = datalen;

	return PNG_NO_ERROR;
}

static int png_init_inflate(png_t* png)
{
#if USE_ZLIB
	z_stream *stream;
	png->zs = png_alloc(sizeof(z_stream));
#else
	zl_stream *stream;
	png->zs = png_alloc(sizeof(zl_stream));
#endif

	stream = png->zs;

	if(!stream)
		return PNG_MEMORY_ERROR;

	

#if USE_ZLIB
	memset(stream, 0, sizeof(z_stream));
	if(inflateInit(stream) != Z_OK)
		return PNG_ZLIB_ERROR;
#else
	memset(stream, 0, sizeof(zl_stream));
	if(z_inflateInit(stream) != Z_OK)
		return PNG_ZLIB_ERROR;
#endif

	stream->next_out = png->png_data;
	stream->avail_out = png->png_datalen;

	return PNG_NO_ERROR;
}

static int png_end_deflate(png_t* png)
{
	z_stream *stream = png->zs;

	if(!stream)
		return PNG_MEMORY_ERROR;

	deflateEnd(stream);

	png_free(png->zs);

	return PNG_NO_ERROR;
}

static int png_end_inflate(png_t* png)
{
#if USE_ZLIB
	z_stream *stream = png->zs;
#else
	zl_stream *stream = png->zs;
#endif

	if(!stream)
		return PNG_MEMORY_ERROR;

#if USE_ZLIB
	if(inflateEnd(stream) != Z_OK)
#else
	if(z_inflateEnd(stream) != Z_OK)
#endif
	{
		printf("ZLIB says: %s\n", stream->msg);
		return PNG_ZLIB_ERROR;
	}

	png_free(png->zs);

	return PNG_NO_ERROR;
}

static int png_inflate(png_t* png, unsigned char* data, int len)
{
	int result;
#if USE_ZLIB
	z_stream *stream = png->zs;
#else
	zl_stream *stream = png->zs;
#endif

	if(!stream)
		return PNG_MEMORY_ERROR;

	stream->next_in = (unsigned char*)data;
	stream->avail_in = len;
	
#if USE_ZLIB
	result = inflate(stream, Z_SYNC_FLUSH);
#else
	result = z_inflate(stream);
#endif

	if(result != Z_STREAM_END && result != Z_OK)
	{
		printf("%s\n", stream->msg);
		return PNG_ZLIB_ERROR;
	}

	if(stream->avail_in != 0)
		return PNG_ZLIB_ERROR;

	return PNG_NO_ERROR;
}

static int png_deflate(png_t* png, unsigned char* outdata, int outlen, int *outwritten)
{
	int result;

	z_stream *stream = png->zs;


	if(!stream)
		return PNG_MEMORY_ERROR;

	stream->next_out = (unsigned char*)outdata;
	stream->avail_out = outlen;

	result = deflate(stream, Z_SYNC_FLUSH);

	*outwritten = outlen - stream->avail_out;

	if(result != Z_STREAM_END && result != Z_OK)
	{
		printf("%s\n", stream->msg);
		return PNG_ZLIB_ERROR;
	}

	return result;
}

static int png_write_idats(png_t* png, unsigned char* data)
{
	unsigned char *chunk;
	unsigned long written;
	unsigned long crc;
	unsigned size = png->height * (png->pitch + 1);
	
	(void)png_init_deflate;
	(void)png_end_deflate;
	(void)png_deflate;

	chunk = png_alloc(size);
	memcpy(chunk, "IDAT", 4);
	
	written = size;
	compress(chunk+4, &written, data, size);
	
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, chunk, written+4);
	set_ul(chunk+written+4, crc);
	file_write_ul(png, written);
	file_write(png, chunk, 1, written+8);
	png_free(chunk);

	file_write_ul(png, 0);
	file_write(png, "IEND", 1, 4);
	crc = crc32(0L, (const unsigned char *)"IEND", 4);
	file_write_ul(png, crc);
	
	return PNG_NO_ERROR;
}

static int png_read_idat(png_t* png, unsigned firstlen) 
{
	unsigned type = 0;
	unsigned char *chunk;
	int result;
	unsigned length = firstlen;
	unsigned old_len = length;
	unsigned orig_crc;

	chunk = png_alloc(firstlen); 

	result = png_init_inflate(png);

	if(result != PNG_NO_ERROR)
	{
		png_end_inflate(png);
		png_free(chunk); 
		return result;
	}

	do
	{
		if(file_read(png, chunk, 1, length) != length)
		{
			png_end_inflate(png);
			png_free(chunk); 
			return PNG_FILE_ERROR;
		}

		file_read_ul(png, &orig_crc);

		if(orig_crc != png_calc_crc("IDAT", chunk, length))
		{
			result = PNG_CRC_ERROR;
			break;
		}

		result = png_inflate(png, chunk, length);

		if(result != PNG_NO_ERROR) break;
		
		file_read_ul(png, &length);

		if(length > old_len)
		{
			png_free(chunk); 
			chunk = png_alloc(length); 
			old_len = length;
		}

		if(file_read(png, &type, 1, 4) != 4)
		{
			result = PNG_FILE_ERROR;
			break;
		}

	}while(type == *(unsigned int*)"IDAT");

	if(type == *(unsigned int*)"IEND")
		result = PNG_DONE;

	png_free(chunk);
	png_end_inflate(png);

	return result;
}

static int png_process_chunk(png_t* png)
{
    int result = PNG_NO_ERROR;
    unsigned type;
    unsigned length;

    if (file_read_ul(png, &length) != PNG_NO_ERROR)
        return PNG_EOF_ERROR;

    if (file_read(png, &type, 4, 1) != 1)
        return PNG_EOF_ERROR;

    if (type == *(unsigned int *) "PLTE")
    {
        if (length % 3)
            return PNG_CORRUPTED_ERROR;

        memset(png->palette, 255, 1024);

        if (file_read(png, png->palette, length, 1) != 1)
            return PNG_EOF_ERROR;

        if (png_read_check_crc(png, "PLTE", png->palette, length) != PNG_NO_ERROR)
            return PNG_CRC_ERROR;

    }
    else if (type == *(unsigned int*)"tRNS")
    {
        png->transparency_present = 1;
        switch(png->color_type) {
            case PNG_INDEXED:
                if (length > 256)
                    return PNG_CORRUPTED_ERROR;

                if (file_read(png, png->palette + 768, length, 1) != 1)
                    return PNG_EOF_ERROR;

                return png_read_check_crc(png, "tRNS", png->palette + 768, length);

            case PNG_TRUECOLOR:
                if (length != 6)
                    return PNG_CORRUPTED_ERROR;

                if (file_read(png, png->colorkey, length, 1) != 1)
                    return PNG_EOF_ERROR;

                if (png_read_check_crc(png, "tRNS", png->colorkey, length) != PNG_NO_ERROR)
                    return PNG_CRC_ERROR;

                png->colorkey[0] = png->colorkey[1];
                png->colorkey[1] = png->colorkey[3];
                png->colorkey[2] = png->colorkey[5];

                return PNG_NO_ERROR;

            case PNG_GREYSCALE:
                if (length != 2)
                    return PNG_CORRUPTED_ERROR;

                file_read(png, png->colorkey, length, 1);

                if (png_read_check_crc(png, "tRNS", png->colorkey, length) != PNG_NO_ERROR)
                    return PNG_CRC_ERROR;

                png->colorkey[0] = png->colorkey[1];

                return PNG_NO_ERROR;

            default:
                return PNG_CORRUPTED_ERROR;
        }
    }
    else if(type == *(unsigned int*)"IDAT")	/* if we found an idat, all other idats should be followed with no other chunks in between */
    {
        png->png_datalen = png->height * (png->pitch + 1);
        png->png_data = png_alloc(png->png_datalen);

        if(!png->png_data)
            return PNG_MEMORY_ERROR;

        return png_read_idat(png, length);
    }
    else if(type == *(unsigned int*)"IEND")
    {
        return PNG_DONE;
    }
    else
    {
        if (file_read(png, 0, length + 4, 1) != 1) /* unknown chunk */
            return PNG_EOF_ERROR;
    }

    return result;
}

static int png_write_plte(png_t *png)
{
    unsigned char plte[4 + 4 + 768 + 4];
    int i;
    const int length = 768;

    memcpy(plte + 4, "PLTE", 4);
    for(i = 0 ; i < 256; i++) {
        plte[8 + 3*i + 0] = png->palette[4*i + 0];
        plte[8 + 3*i + 1] = png->palette[4*i + 1];
        plte[8 + 3*i + 2] = png->palette[4*i + 2];
    }

    if (file_write(png, plte, length + 8, 1) != 1)
        return PNG_IO_ERROR;

    return png_calc_write_crc(png, "PLTE", plte + 4, length);
}

static int png_write_trns(png_t *png)
{
    unsigned char trns[4 + 4 + 256 + 4];
    int i;
    int length;

    switch (png->color_type)
    {
        case PNG_INDEXED:
            length = 256;
            for(i = 0 ; i < 256; i++)
                trns[8 + i] = png->palette[4*i + 3];
            break;

        case PNG_GREYSCALE:
            length = 2;
            trns[8 + 0] = 0;
            trns[8 + 1] = png->colorkey[0];
            break;

        case PNG_TRUECOLOR:
            length = 6;
            trns[8 + 0] = 0;
            trns[8 + 1] = png->colorkey[0];
            trns[8 + 2] = 0;
            trns[8 + 3] = png->colorkey[1];
            trns[8 + 4] = 0;
            trns[8 + 5] = png->colorkey[2];
            break;

        default:
            return PNG_NOT_SUPPORTED;
    }
    memmove(trns + 4, "tRNS", 4);
    set_ul(trns, length);

    if (file_write(png, trns, length + 8, 1) != 1)
        return PNG_IO_ERROR;

    return png_calc_write_crc(png, "tRNS", trns + 4, length);
}

static unsigned char png_paeth_predictor(unsigned char a, unsigned char b, unsigned char c)
{
	int p = (int)a + b - c;
	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);

	int pr;

	if(pa <= pb && pa <= pc)
		pr = a;
	else if(pb <= pc)
		pr = b;
	else
		pr = c;

	return (char)pr;
}

static int png_filter(png_t* png, unsigned char* data)
{
	return PNG_NO_ERROR;
}


static int png_unfilter(png_t* png, unsigned char* data)
{
    unsigned i, p, t;
    unsigned char a, b, c;
    unsigned char *filtered;
    unsigned char *reconstructed;
    unsigned char *up_reconstructed;
    unsigned char filter_type;
    for(i = 0; i < png->height ; i++)
    {
        filtered = png->png_data + (png->pitch + 1) * i;
        reconstructed = data + png->pitch * i;
        up_reconstructed = i > 0 ? data + png->pitch * (i - 1) : 0;
        filter_type = *filtered++;

        switch(filter_type)
        {
            case PNG_FILTER_NONE:
                memcpy(reconstructed, filtered, png->pitch);
                break;

            case PNG_FILTER_SUB:
                memcpy(reconstructed, filtered, png->stride);
                for (p = png->stride; p < png->pitch ; p++)
                    reconstructed[p] = filtered[p] + reconstructed[p - png->stride];
                break;

            case PNG_FILTER_UP:
                if (up_reconstructed)
                    for (p = 0; p < png->pitch ; p++)
                        reconstructed[p] = filtered[p] + up_reconstructed[p];
                else
                    memcpy(reconstructed, filtered, png->pitch);
                break;

            case PNG_FILTER_AVERAGE:
                for (p = 0; p < png->pitch ; p++) {
                    b = up_reconstructed ? up_reconstructed[p] : 0;
                    a = p >= png->stride ? reconstructed[p - png->stride] : 0;
                    t = a + b;
                    reconstructed[p] = filtered[p] + t/2;
                }
                break;

            case PNG_FILTER_PAETH:
                for (p = 0; p < png->pitch ; p++) {
                    c = (up_reconstructed && (p >= png->stride)) ? up_reconstructed[p - png->stride] : 0;
                    b = up_reconstructed ? up_reconstructed[p] : 0;
                    a = p >= png->stride ? reconstructed[p - png->stride] : 0;
                    reconstructed[p] = filtered[p] + png_paeth_predictor(a, b, c);
                }
                break;

            default:
                return PNG_UNKNOWN_FILTER;
        }
    }
    return PNG_NO_ERROR;
}

int png_get_data(png_t* png, unsigned char* data)
{
    int result = PNG_NO_ERROR;
    unsigned char *unpacked_pixel;
    unsigned char *packed_pixel;
    unsigned char *packed_data;

    png->transparency_present = 0;
    png->png_data = NULL;

    while(result == PNG_NO_ERROR)
        result = png_process_chunk(png);

    if(result != PNG_DONE) {
        if (png->png_data)
            png_free(png->png_data);
        return result;
    }

    if (png->png_data == NULL)
        /* no IDAT chunk in file */
        return PNG_CORRUPTED_ERROR;

    if (png->depth < 8) {
        packed_data = png_alloc(png->width * png->pitch);

        if (!packed_data)
            return PNG_MEMORY_ERROR;

        result = png_unfilter(png, packed_data);

        if (result != PNG_NO_ERROR) {
            png_free(packed_data);
            png_free(png->png_data);
            return result;
        }

        unpacked_pixel = data;
        packed_pixel = packed_data;

        while (unpacked_pixel - data < png->width * png->height) {
            switch (png->depth) {
                case 1:
                    *unpacked_pixel++ = (*packed_pixel & 0x80) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x40) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x20) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x10) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x08) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x04) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x02) ? 1 : 0;
                    *unpacked_pixel++ = (*packed_pixel & 0x01) ? 1 : 0;
                    break;
                case 2:
                    *unpacked_pixel++ = (*packed_pixel & 0xC0) >> 6;
                    *unpacked_pixel++ = (*packed_pixel & 0x30) >> 4;
                    *unpacked_pixel++ = (*packed_pixel & 0x0C) >> 2;
                    *unpacked_pixel++ = (*packed_pixel & 0x03);
                    break;
                case 4:
                    *unpacked_pixel++ = (*packed_pixel & 0xF0) >> 4;
                    *unpacked_pixel++ = (*packed_pixel & 0x0F);
                    break;
                default:
                    /* can't be, we caught this in read_ihdr */
                    return PNG_CORRUPTED_ERROR;
            }
            packed_pixel++;
        }
        png_free(packed_data);
    } else {
        result = png_unfilter(png, data);
    }
    png_free(png->png_data);
    return result;
}

int png_set_data(png_t* png, unsigned width, unsigned height, char depth, int color, int transparency, unsigned char* data)
{
    unsigned i;
    int err;
    unsigned char *filtered;

    png->width = width;
    png->height = height;
    png->depth = depth;
    png->color_type = color;
    png->filter_method = 0;
    png->interlace_method = 0;

    if (png_check_png(png) || (png->depth != 8))
        return PNG_NOT_SUPPORTED;

    filtered = png_alloc((png->pitch + 1) * height);
    if (!filtered)
        return PNG_MEMORY_ERROR;

    for(i = 0; i < png->height; i++)
    {
        filtered[i * (png->pitch + 1)] = 0;
        memcpy(filtered + i*(png->pitch + 1) + 1, data + i*png->pitch, png->pitch);
    }
    png_filter(png, filtered);
    png_write_ihdr(png);

    if (png->color_type == PNG_INDEXED)
        if ((err = png_write_plte(png)))
            return err;

    if (transparency)
        if ((err = png_write_trns(png)))
            return err;

    png_write_idats(png, filtered);
    png_free(filtered);

    return PNG_NO_ERROR;
}


char* png_error_string(int error)
{
	switch(error)
	{
	case PNG_NO_ERROR:
		return "No error";
	case PNG_FILE_ERROR:
		return "Unknown file error.";
	case PNG_HEADER_ERROR:
		return "No PNG header found. Are you sure this is a PNG?";
	case PNG_IO_ERROR:
		return "Failure while reading file.";
	case PNG_EOF_ERROR:
		return "Reached end of file.";
	case PNG_CRC_ERROR:
		return "CRC or chunk length error.";
	case PNG_MEMORY_ERROR:
		return "Could not allocate memory.";
	case PNG_ZLIB_ERROR:
		return "zlib reported an error.";
	case PNG_UNKNOWN_FILTER:
		return "Unknown filter method used in scanline.";
	case PNG_DONE:
		return "PNG done";
	case PNG_NOT_SUPPORTED:
		return "The PNG is unsupported by pnglite, too bad for you!";
	case PNG_WRONG_ARGUMENTS:
		return "Wrong combination of arguments passed to png_open. You must use either a read_function or supply a file pointer to use.";
        case PNG_CORRUPTED_ERROR:
            return "PNG data does not follow the specification or is corrupted.";
	default:
		return "Unknown error.";
	};
}
