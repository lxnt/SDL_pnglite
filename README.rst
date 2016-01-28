libpnglite with indexed color types support
*******************************************

http://www.w3.org/TR/PNG/


Reading:
========


PNG interlacing and ancillary chunks
------------------------------------

Interlacing is not supported. Attempt to read interlaced image file will fail.

Of ancillary chunks only tRNS is parsed. Others are silently ignored.
No access to them is provided.


PNG per-channel depth
----------------------

Everything that's less than 8 bits is expanded to 8 bits.

16 bit per channel images are not supported.


PNG color types and transparency:
---------------------------------

- PNG_INDEXED, no transparency:
    - png_get_data() returns I bytestream; required buffer size is width*height
    - png_t::palette contains 256-entry RGB, unused entries are set to #FFF
    - png_t::transparency_present is 0

- PNG_INDEXED, with transparency:
    - png_get_data() returns I bytestream; required buffer size is width*height
    - png_t::palette contains 256-entry RGB palette followed by 256 bytes of alpha channel,
      unused entries are set to #FFF and FF.
    - png_t::transparency_present is 1

- PNG_GREYSCALE, no transparency:
    - png_get_data() returns RGB bytestream; required buffer size is width*height*3
    - png_t::transparency_present is 0

- PNG_GREYSCALE, with transparency:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::colorkey[0] contains the transparent sample value.
    - png_t::transparency_present is 1

- PNG_TRUECOLOR, no transparency:
    - png_get_data() returns RGB bytestream; required buffer size is width*height*3
    - png_t::transparency_present is 0

- PNG_TRUECOLOR, with transparency:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::colorkey[0..2] contains the transparent RGB sample values.
    - png_t::transparency_present is 1

- PNG_GREYSCALE_ALPHA:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::transparency_present is not defined

- PNG_TRUECOLOR_ALPHA:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::transparency_present is not defined


Writing:
========

When png_t::color_type is set to:

- PNG_INDEXED:
    - supplied buffer must contain widht*height bytes of palette indices.
    - png_t::transparency_present may be 0 or 1.
    - png_t::palette must be initialized to 256-entry RGBX or RGBA palette.

- PNG_TRUECOLOR:
    - supplied buffer must contain widht*height*3 bytes of RGB samples.
    - png_t::transparency_present may be 0 or 1.
    - png_t::colorkey may be initialized to an RGB sample values.

- PNG_TRUECOLOR_ALPHA:
    - supplied buffer must contain widht*height*4 bytes of RGBA samples.

Other color types are not supported.

Filtering is not done - patches are welcome.

Compression is to be assumed suboptimal.
Indexed images are always written out as 8 bits per pixel.

Full palette is always written out.

If png_t::transparency_present == 1, 256 byte tRNS chunk is written out for indexed images.


SDL_Surface wrapper for the above
*********************************

All above caveats apply.

SDL_LoadPNG() / SDL_LoadPNG_RW():
=================================

- Attempts to load a png from given filename / RWops object.
- Indexed-color images without transparency are returned as paletted surfaces.
- Indexed-color images with transparency are returned as paletted surfaces with colorkey 
  if and only if the transparency chunks marks exacly one color as fully transparent, and
  all others as fully opaque. Otherwise they are returned as RGBA32.
- Truecolor+alpha and grayscale+alpha are returned as RGBA32.
- Truecolor (no alpha) are returned as RGB24 (transparency results in colorkey).
- Grayscale images are returned as indexed color (transparency results in colorkey).


SDL_SavePNG() / SDL_SavePNG_RW():
=================================

- Attemps to save given surface as png image to given filename / RWops object.
- Paletted surfaces with or without colorkey are saved as indexed color.
- All other surfaces are converted to and saved as 8bpc RGB/RGBA ones.

Not implemented yet:

- RGB surfaces with or without colorkey and RGBA ones are saved as such.


Test suite:
===========

Test strategy for reading:

- for each image in the test suite, load it both with SDL_LoadPNG and IMG_LoadPNG.
   pixelformats and image data must be mostly identical.

Test strategy for writing:

- for each image in the test suite, load it, then save to a temporary file,
  then load the temporary file. Compare pixelformats and pixel data.

