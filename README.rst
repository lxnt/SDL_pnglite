libpnglite with indexed color types support
*******************************************

http://www.w3.org/TR/PNG/


**Caution:** do not use this to read unstrusted data - PNG files you did not author, recompress, optimize or test yourself.


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
    - png_t::palette contains png_t::palette_size RGB entries, unused entries are set to #FFF
    - png_t::transparency_present is 0

- PNG_INDEXED, with transparency:
    - png_get_data() returns I bytestream; required buffer size is width*height
    - png_t::palette contains png_t::palette_size RGB entries, unused entries are set to #FFF.
    - png_t::palette[768...1024] contains png_t::palette_size alpha values, unused entries are set to FF.
    - png_t::transparency_present is 1

- PNG_GREYSCALE, no transparency:
    - png_get_data() returns RGB bytestream; required buffer size is width*height*3
    - png_t::transparency_present is 0

- PNG_GREYSCALE, with transparency:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::colorkey[0..1] contains the transparent sample value. Since 16 bit depth
      is not supported, only png_t::colorkey[1] value should be used.
    - png_t::transparency_present is 1

- PNG_TRUECOLOR, no transparency:
    - png_get_data() returns RGB bytestream; required buffer size is width*height*3
    - png_t::transparency_present is 0

- PNG_TRUECOLOR, with transparency:
    - png_get_data() returns RGBA bytestream; required buffer size is width*height*4
    - png_t::colorkey[0..5] contains the transparent RGB sample values. Since 16 bit depth
      is not supported, only png_t::colorkey[1,3,5] values should be used.
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
    - png_t::palette must be initialized to RGBX or RGBA palette and png_t::palette_size
      must be set to number of colors in the palette.

- PNG_TRUECOLOR:
    - supplied buffer must contain widht*height*3 bytes of RGB samples.
    - png_t::transparency_present may be 0 or 1.
    - png_t::colorkey may be initialized to an RGB sample values.

- PNG_TRUECOLOR_ALPHA:
    - supplied buffer must contain widht*height*4 bytes of RGBA samples.

Other color types are not supported.

Filtering is not done - patches are welcome.

Compression is to be assumed suboptimal.


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
- RGB surfaces are saved as 8bpc RGB preserving colorkey.
- All other surfaces are converted to and saved as 8bpc RGBA ones.


Notable differences from IMG_LoadPNG_RW():
==========================================

- SDL_PIXELFORMAT_RGBA8888 is used instead of SDL_PIXELFORMAT_ABGR8888
- Interlaced images are not accepted
- 16 bit per channel are not accepted.


Notable differences from IMG_SavePNG_RW():
==========================================

- Palettes and colorkeys are preserved as much as possible within the format
  (IMG_SavePNG_RW() doesn't attempt this at all)


Test suite (test-suite.c):
==========================

Test strategy for loading:
--------------------------

- For each image in the test suite, load it both with SDL_LoadPNG() and IMG_Load().
  Pixelformats and image data must be mostly identical.

Test strategy for saving:
-------------------------

- For each image in the test suite, load it, then save to a memory buffer,
  then load from the buffer with IMG_LoadPNG_RW(). Compare pixelformats and pixel data.

Test image set:
---------------

- get PngSuite from http://www.schaik.com/pngsuite/
- remove all 16bpp and interlaced images (``rm *16.png ???i*.png``)
- submit the rest to the test suite:  ``./test-suite /path/to/subset/*.png``
- files starting with 'x' are supposed to fail loading.

Known issues:
-------------

- SDL2 can have colorkeyed RGBA surfaces. PNG does not support colorkeys on RGBA data, thus
  the colorkey is lost on save. Alternative would be to lose alpha channel on matching pixels.
- ``IMG_LoadPNG_RW()`` sets number of palette entries directly. This cannot be done
  via SDL API (``SDL_AllocPalette()`` / ``SDL_SetSurfacePalette()``). Right now SDL_pnglite
  creates short palettes, otherwise test-suite will dutifully show palette mismatches.
- ``tbbn0g04.png: pixel format mismatch spl SDL_PIXELFORMAT_INDEX8 si SDL_PIXELFORMAT_RGB565``
  reason is libpng12 returns 2 channels and bit_depth of 8 for this image (no idea why, it's 4bit),
  then num_channels*bit_depth is used as bpp in ``SDL_CreateRGBSurface()``. This is a bug.
- Also, ``IMG_LoadPNG_RW()`` incorrectly sets greyscale/rgb colorkeys. How this doesn't show up in tests I cannot fathom.

TODO:
=====

libpnglite:
-----------

- Convert to stdint (and/or maybe native-zlib) types
- Maybe present palette as RGBx/RGBA on load (as it is submitted for saving).
- Discover and fix endianness-related bugs.
- Fix png_t::pitch vs filter type ambigousness

SDL_pnglite:
------------

- Discover and fix endianness-related bugs

