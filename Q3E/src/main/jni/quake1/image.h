
#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>
#include "qtypes.h"
#include "cvar.h"
#include "r_textures.h"

extern int image_width, image_height;

unsigned char *Image_GenerateNoTexture(void);

// swizzle components (even converting number of components) and flip images
// (warning: input must be different than output due to non-linear read/write)
// (tip: component indices can contain values | 0x80000000 to tell it to
// store them directly into output, so 255 | 0x80000000 would write 255)
void Image_CopyMux(unsigned char *outpixels, const unsigned char *inpixels, int inputwidth, int inputheight, qbool inputflipx, qbool inputflipy, qbool inputflipdiagonal, int numoutputcomponents, int numinputcomponents, int *outputinputcomponentindices);

// applies gamma correction to RGB pixels, in can be the same as out
void Image_GammaRemapRGB(const unsigned char *in, unsigned char *out, int pixels, const unsigned char *gammar, const unsigned char *gammag, const unsigned char *gammab);

// converts 8bit image data to BGRA, in can not be the same as out
void Image_Copy8bitBGRA(const unsigned char *in, unsigned char *out, int pixels, const unsigned int *pal);

void Image_StripImageExtension (const char *in, char *out, size_t size_out);

// called by conchars.tga loader in gl_draw.c, otherwise private
unsigned char *LoadTGA_BGRA (const unsigned char *f, int filesize, int *miplevel);

// loads a texture, as pixel data
unsigned char *loadimagepixelsbgra (const char *filename, qbool complain, qbool allowFixtrans, qbool convertsRGB, int *miplevel);

// searches for lmp and wad pics of the provided name and returns true and their dimensions if found
qbool Image_GetStockPicSize(const char *filename, int *returnwidth, int *returnheight);

// loads an 8bit pcx image into a 296x194x8bit buffer, with cropping as needed
qbool LoadPCX_QWSkin(const unsigned char *f, int filesize, unsigned char *pixels, int outwidth, int outheight);

// loads the palette from an 8bit pcx image into your provided array
qbool LoadPCX_PaletteOnly(const unsigned char *f, int filesize, unsigned char *palette768b);

// get the metadata from a Quake2 wal file
qbool LoadWAL_GetMetadata(const unsigned char *f, int filesize, int *retwidth, int *retheight, int *retflags, int *retvalue, int *retcontents, char *retanimname32c);

// loads a texture, as a texture
rtexture_t *loadtextureimage (rtexturepool_t *pool, const char *filename, qbool complain, int flags, qbool allowFixtrans, qbool sRGB);

// writes an upside down BGR image into a TGA
qbool Image_WriteTGABGR_preflipped (const char *filename, int width, int height, const unsigned char *data);

// writes a BGRA image into a TGA file
qbool Image_WriteTGABGRA (const char *filename, int width, int height, const unsigned char *data);

// resizes the image (in can not be the same as out)
void Image_Resample32(const void *indata, int inwidth, int inheight, int indepth, void *outdata, int outwidth, int outheight, int outdepth, int quality);

// scales the image down by a power of 2 (in can be the same as out)
void Image_MipReduce32(const unsigned char *in, unsigned char *out, int *width, int *height, int *depth, int destwidth, int destheight, int destdepth);

void Image_HeightmapToNormalmap_BGRA(const unsigned char *inpixels, unsigned char *outpixels, int width, int height, int clamp, float bumpscale);

// console command to fix the colors of transparent pixels (to prevent weird borders)
void Image_FixTransparentPixels_f(cmd_state_t *cmd);
extern cvar_t r_fixtrans_auto;

#define Image_LinearFloatFromsRGBFloat(c) (((c) <= 0.04045f) ? (c) * (1.0f / 12.92f) : (float)pow(((c) + 0.055f)*(1.0f/1.055f), 2.4f))
#define Image_sRGBFloatFromLinearFloat(c) (((c) < 0.0031308f) ? (c) * 12.92f : 1.055f * (float)pow((c), 1.0f/2.4f) - 0.055f)
#define Image_LinearFloatFromsRGB(c) Image_LinearFloatFromsRGBFloat((c) * (1.0f / 255.0f))
#define Image_sRGBFloatFromLinear(c) Image_sRGBFloatFromLinearFloat((c) * (1.0f / 255.0f))
#define Image_sRGBFloatFromLinear_Lightmap(c) Image_sRGBFloatFromLinearFloat((c) * (2.0f / 255.0f)) * 0.5f

void Image_MakeLinearColorsFromsRGB(unsigned char *pout, const unsigned char *pin, int numpixels);
void Image_MakesRGBColorsFromLinear_Lightmap(unsigned char *pout, const unsigned char *pin, int numpixels);

#endif

