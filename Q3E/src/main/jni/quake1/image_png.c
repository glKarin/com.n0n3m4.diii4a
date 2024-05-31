/*
	Copyright (C) 2006  Serge "(515)" Ziryukin, Ashley Rose Hale (LadyHavoc)

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/

//[515]: png implemented into DP ONLY FOR TESTING 2d stuff with csqc
// so delete this bullshit :D
//
//LadyHavoc: rewrote most of this.

#include "darkplaces.h"
#include "image.h"
#include "image_png.h"


static void				(*qpng_set_sig_bytes)		(void*, int);
static int				(*qpng_sig_cmp)				(const unsigned char*, size_t, size_t);
static void*			(*qpng_create_read_struct)	(const char*, void*, void(*)(void *png, const char *message), void(*)(void *png, const char *message));
static void*			(*qpng_create_write_struct)	(const char*, void*, void(*)(void *png, const char *message), void(*)(void *png, const char *message));
static void*			(*qpng_create_info_struct)	(void*);
static void				(*qpng_read_info)			(void*, void*);
static void				(*qpng_set_compression_level)	(void*, int);
static void				(*qpng_set_filter)			(void*, int, int);
static void				(*qpng_set_expand)			(void*);
static void				(*qpng_set_palette_to_rgb)	(void*);
static void				(*qpng_set_tRNS_to_alpha)	(void*);
static void				(*qpng_set_gray_to_rgb)		(void*);
static void				(*qpng_set_filler)			(void*, unsigned int, int);
static void				(*qpng_set_IHDR)			(void*, void*, unsigned long, unsigned long, int, int, int, int, int);
static void				(*qpng_set_packing)			(void*);
static void				(*qpng_set_bgr)				(void*);
static int				(*qpng_set_interlace_handling)	(void*);
static void				(*qpng_read_update_info)	(void*, void*);
static void				(*qpng_read_image)			(void*, unsigned char**);
static void				(*qpng_read_end)			(void*, void*);
static void				(*qpng_destroy_read_struct)	(void**, void**, void**);
static void				(*qpng_destroy_write_struct)	(void**, void**);
static void				(*qpng_set_read_fn)			(void*, void*, void(*)(void *png, unsigned char *data, size_t length));
static void				(*qpng_set_write_fn)		(void*, void*, void(*)(void *png, unsigned char *data, size_t length), void(*)(void *png));
static unsigned int		(*qpng_get_valid)			(void*, void*, unsigned int);
static unsigned int		(*qpng_get_rowbytes)		(void*, void*);
static unsigned char	(*qpng_get_channels)		(void*, void*);
static unsigned char	(*qpng_get_bit_depth)		(void*, void*);
static unsigned int		(*qpng_get_IHDR)			(void*, void*, unsigned long*, unsigned long*, int *, int *, int *, int *, int *);
static unsigned int			(*qpng_access_version_number)		(void); // FIXME is this return type right? It is a png_uint_32 in libpng
static void				(*qpng_write_info)			(void*, void*);
static void				(*qpng_write_row)			(void*, unsigned char*);
static void				(*qpng_write_end)			(void*, void*);

// libpng 1.4+ longjmp hack
typedef void (*qpng_longjmp_ptr) (jmp_buf, int);
static jmp_buf* (*qpng_set_longjmp_fn) (void *, qpng_longjmp_ptr, size_t);
#define qpng_jmpbuf_14(png_ptr) (*qpng_set_longjmp_fn((png_ptr), longjmp, sizeof (jmp_buf)))

// libpng 1.2 longjmp hack
#define qpng_jmpbuf_12(png_ptr) (*((jmp_buf *) png_ptr))

// all version support
#define qpng_jmpbuf(png_ptr) \
	(qpng_set_longjmp_fn ? qpng_jmpbuf_14(png_ptr) : qpng_jmpbuf_12(png_ptr))

static dllfunction_t pngfuncs[] =
{
	{"png_set_sig_bytes",		(void **) &qpng_set_sig_bytes},
	{"png_sig_cmp",				(void **) &qpng_sig_cmp},
	{"png_create_read_struct",	(void **) &qpng_create_read_struct},
	{"png_create_write_struct",	(void **) &qpng_create_write_struct},
	{"png_create_info_struct",	(void **) &qpng_create_info_struct},
	{"png_read_info",			(void **) &qpng_read_info},
	{"png_set_compression_level",	(void **) &qpng_set_compression_level},
	{"png_set_filter",			(void **) &qpng_set_filter},
	{"png_set_expand",			(void **) &qpng_set_expand},
	{"png_set_palette_to_rgb",	(void **) &qpng_set_palette_to_rgb},
	{"png_set_tRNS_to_alpha",	(void **) &qpng_set_tRNS_to_alpha},
	{"png_set_gray_to_rgb",		(void **) &qpng_set_gray_to_rgb},
	{"png_set_filler",			(void **) &qpng_set_filler},
	{"png_set_IHDR",			(void **) &qpng_set_IHDR},
	{"png_set_packing",			(void **) &qpng_set_packing},
	{"png_set_bgr",				(void **) &qpng_set_bgr},
	{"png_set_interlace_handling",	(void **) &qpng_set_interlace_handling},
	{"png_read_update_info",	(void **) &qpng_read_update_info},
	{"png_read_image",			(void **) &qpng_read_image},
	{"png_read_end",			(void **) &qpng_read_end},
	{"png_destroy_read_struct",	(void **) &qpng_destroy_read_struct},
	{"png_destroy_write_struct",	(void **) &qpng_destroy_write_struct},
	{"png_set_read_fn",			(void **) &qpng_set_read_fn},
	{"png_set_write_fn",		(void **) &qpng_set_write_fn},
	{"png_get_valid",			(void **) &qpng_get_valid},
	{"png_get_rowbytes",		(void **) &qpng_get_rowbytes},
	{"png_get_channels",		(void **) &qpng_get_channels},
	{"png_get_bit_depth",		(void **) &qpng_get_bit_depth},
	{"png_get_IHDR",			(void **) &qpng_get_IHDR},
	{"png_access_version_number",		(void **) &qpng_access_version_number},
	{"png_write_info",			(void **) &qpng_write_info},
	{"png_write_row",			(void **) &qpng_write_row},
	{"png_write_end",			(void **) &qpng_write_end},
	{NULL, NULL}
};
static dllfunction_t png14funcs[] =
{
	{"png_set_longjmp_fn",		(void **) &qpng_set_longjmp_fn},
	{NULL, NULL}
};

// Handle for PNG DLL
dllhandle_t png_dll = NULL;
dllhandle_t png14_dll = NULL;


/*
=================================================================

  DLL load & unload

=================================================================
*/

/*
====================
PNG_OpenLibrary

Try to load the PNG DLL
====================
*/
qbool PNG_OpenLibrary (void)
{
	const char* dllnames [] =
	{
#if WIN32
		"libpng16.dll",
		"libpng16-16.dll",
		"libpng15-15.dll",
		"libpng15.dll",
		"libpng14-14.dll",
		"libpng14.dll",
		"libpng12.dll",
#elif defined(MACOSX)
		"libpng16.16.dylib",
		"libpng15.15.dylib",
		"libpng14.14.dylib",
		"libpng12.0.dylib",
#else
		"libpng16.so.16",
		"libpng15.so.15", // WTF libtool guidelines anyone?
		"libpng14.so.14", // WTF libtool guidelines anyone?
		"libpng12.so.0",
		"libpng.so", // FreeBSD
#endif
		NULL
	};

	// Already loaded?
	if (png_dll)
		return true;

	// Load the DLL
	if(!Sys_LoadDependency (dllnames, &png_dll, pngfuncs))
		return false;
	if(qpng_access_version_number() / 100 >= 104)
		if(!Sys_LoadDependency (dllnames, &png14_dll, png14funcs))
		{
			Sys_FreeLibrary (&png_dll);
			return false;
		}
	return true;
}


/*
====================
PNG_CloseLibrary

Unload the PNG DLL
====================
*/
void PNG_CloseLibrary (void)
{
	Sys_FreeLibrary (&png14_dll);
	Sys_FreeLibrary (&png_dll);
}

/*
=================================================================

	PNG decompression

=================================================================
*/

#define PNG_LIBPNG_VER_STRING_12 "1.2.4"
#define PNG_LIBPNG_VER_STRING_14 "1.4.0"
#define PNG_LIBPNG_VER_STRING_15 "1.5.0"
#define PNG_LIBPNG_VER_STRING_16 "1.6.0"

#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)

#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

#define PNG_INFO_tRNS 0x0010

// this struct is only used for status information during loading
static struct
{
	const unsigned char	*tmpBuf;
	int		tmpBuflength;
	int		tmpi;
	//int		FBgColor;
	//int		FTransparent;
	unsigned int	FRowBytes;
	//double	FGamma;
	//double	FScreenGamma;
	unsigned char	**FRowPtrs;
	unsigned char	*Data;
	//char	*Title;
	//char	*Author;
	//char	*Description;
	int		BitDepth;
	int		BytesPerPixel;
	int		ColorType;
	unsigned long	Height; // retarded libpng 1.2 pngconf.h uses long (64bit/32bit depending on arch)
	unsigned long	Width; // retarded libpng 1.2 pngconf.h uses long (64bit/32bit depending on arch)
	int		Interlace;
	int		Compression;
	int		Filter;
	//double	LastModified;
	//int		Transparent;
	qfile_t *outfile;
} my_png;

//LadyHavoc: removed __cdecl prefix, added overrun protection, and rewrote this to be more efficient
static void PNG_fReadData(void *png, unsigned char *data, size_t length)
{
	size_t l;
	l = my_png.tmpBuflength - my_png.tmpi;
	if (l < length)
	{
		Con_Printf("PNG_fReadData: overrun by %i bytes\n", (int)(length - l));
		// a read going past the end of the file, fill in the remaining bytes
		// with 0 just to be consistent
		memset(data + l, 0, length - l);
		length = l;
	}
	memcpy(data, my_png.tmpBuf + my_png.tmpi, length);
	my_png.tmpi += (int)length;
	//Com_HexDumpToConsole(data, (int)length);
}

static void PNG_fWriteData(void *png, unsigned char *data, size_t length)
{
	FS_Write(my_png.outfile, data, length);
}

static void PNG_fFlushData(void *png)
{
}

static void PNG_error_fn(void *png, const char *message)
{
	Con_Printf(CON_ERROR "PNG_LoadImage: error: %s\n", message);
}

static void PNG_warning_fn(void *png, const char *message)
{
	Con_Printf(CON_WARN "PNG_LoadImage: warning: %s\n", message);
}

unsigned char *PNG_LoadImage_BGRA (const unsigned char *raw, int filesize, int *miplevel)
{
	unsigned int c;
	unsigned int	y;
	void *png, *pnginfo;
	unsigned char *imagedata = NULL;
	unsigned char ioBuffer[8192];

	// FIXME: register an error handler so that abort() won't be called on error

	// No DLL = no PNGs
	if (!png_dll)
		return NULL;

	if(qpng_sig_cmp(raw, 0, filesize))
		return NULL;
	png = (void *)qpng_create_read_struct(
		(qpng_access_version_number() / 100 == 102) ? PNG_LIBPNG_VER_STRING_12 :
		(qpng_access_version_number() / 100 == 104) ? PNG_LIBPNG_VER_STRING_14 :
		(qpng_access_version_number() / 100 == 105) ? PNG_LIBPNG_VER_STRING_15 :
		PNG_LIBPNG_VER_STRING_16, // nasty hack... whatever
		0, PNG_error_fn, PNG_warning_fn
	);
	if(!png)
		return NULL;

	// this must be memset before the setjmp error handler, because it relies
	// on the fields in this struct for cleanup
	memset(&my_png, 0, sizeof(my_png));

	// NOTE: this relies on jmp_buf being the first thing in the png structure
	// created by libpng! (this is correct for libpng 1.2.x)
	if (setjmp(qpng_jmpbuf(png)))
	{
		if (my_png.Data)
			Mem_Free(my_png.Data);
		my_png.Data = NULL;
		if (my_png.FRowPtrs)
			Mem_Free(my_png.FRowPtrs);
		my_png.FRowPtrs = NULL;
		qpng_destroy_read_struct(&png, &pnginfo, 0);
		return NULL;
	}
	//

	pnginfo = qpng_create_info_struct(png);
	if(!pnginfo)
	{
		qpng_destroy_read_struct(&png, &pnginfo, 0);
		return NULL;
	}
	qpng_set_sig_bytes(png, 0);

	my_png.tmpBuf = raw;
	my_png.tmpBuflength = filesize;
	my_png.tmpi = 0;
	//my_png.Data		= NULL;
	//my_png.FRowPtrs	= NULL;
	//my_png.Height		= 0;
	//my_png.Width		= 0;
	my_png.ColorType	= PNG_COLOR_TYPE_RGB;
	//my_png.Interlace	= 0;
	//my_png.Compression	= 0;
	//my_png.Filter		= 0;
	qpng_set_read_fn(png, ioBuffer, PNG_fReadData);
	qpng_read_info(png, pnginfo);
	qpng_get_IHDR(png, pnginfo, &my_png.Width, &my_png.Height,&my_png.BitDepth, &my_png.ColorType, &my_png.Interlace, &my_png.Compression, &my_png.Filter);

	// this check guards against pngconf.h with unsigned int *width/height parameters on big endian systems by detecting the strange values and shifting them down 32bits
	// (if it's little endian the unwritten bytes are the most significant
	//  ones and we don't worry about that)
	//
	// this is only necessary because of retarded 64bit png_uint_32 types in libpng 1.2, which can (conceivably) vary by platform
#if LONG_MAX > 4000000000
	if (my_png.Width > LONG_MAX || my_png.Height > LONG_MAX)
	{
		my_png.Width >>= 32;
		my_png.Height >>= 32;
	}
#endif

	if (my_png.ColorType == PNG_COLOR_TYPE_PALETTE)
		qpng_set_palette_to_rgb(png);
	if (my_png.ColorType == PNG_COLOR_TYPE_GRAY || my_png.ColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
		qpng_set_gray_to_rgb(png);
	if (qpng_get_valid(png, pnginfo, PNG_INFO_tRNS))
		qpng_set_tRNS_to_alpha(png);
	if (my_png.BitDepth == 8 && !(my_png.ColorType  & PNG_COLOR_MASK_ALPHA))
		qpng_set_filler(png, 255, 1);
	if (( my_png.ColorType == PNG_COLOR_TYPE_GRAY) || (my_png.ColorType == PNG_COLOR_TYPE_GRAY_ALPHA ))
		qpng_set_gray_to_rgb(png);
	if (my_png.BitDepth < 8)
		qpng_set_expand(png);

	qpng_read_update_info(png, pnginfo);

	my_png.FRowBytes = qpng_get_rowbytes(png, pnginfo);
	my_png.BytesPerPixel = qpng_get_channels(png, pnginfo);

	my_png.FRowPtrs = (unsigned char **)Mem_Alloc(tempmempool, my_png.Height * sizeof(*my_png.FRowPtrs));
	if (my_png.FRowPtrs)
	{
		imagedata = (unsigned char *)Mem_Alloc(tempmempool, my_png.Height * my_png.FRowBytes);
		if(imagedata)
		{
			my_png.Data = imagedata;
			for(y = 0;y < my_png.Height;y++)
				my_png.FRowPtrs[y] = my_png.Data + y * my_png.FRowBytes;
			qpng_read_image(png, my_png.FRowPtrs);
		}
		else
		{
			Con_Printf("PNG_LoadImage : not enough memory\n");
			qpng_destroy_read_struct(&png, &pnginfo, 0);
			Mem_Free(my_png.FRowPtrs);
			return NULL;
		}
		Mem_Free(my_png.FRowPtrs);
		my_png.FRowPtrs = NULL;
	}
	else
	{
		Con_Printf("PNG_LoadImage : not enough memory\n");
		qpng_destroy_read_struct(&png, &pnginfo, 0);
		return NULL;
	}

	qpng_read_end(png, pnginfo);
	qpng_destroy_read_struct(&png, &pnginfo, 0);

	image_width = (int)my_png.Width;
	image_height = (int)my_png.Height;

	if (my_png.BitDepth != 8)
	{
		Con_Printf ("PNG_LoadImage : bad color depth\n");
		Mem_Free(imagedata);
		return NULL;
	}

	// swizzle RGBA to BGRA
	for (y = 0;y < (unsigned int)(image_width*image_height*4);y += 4)
	{
		c = imagedata[y+0];
		imagedata[y+0] = imagedata[y+2];
		imagedata[y+2] = c;
	}

	return imagedata;
}

/*
=================================================================

	PNG compression

=================================================================
*/

#define Z_BEST_SPEED 1
#define Z_BEST_COMPRESSION 9
#define PNG_INTERLACE_NONE 0
#define PNG_INTERLACE_ADAM7 1
#define PNG_FILTER_TYPE_BASE 0
#define PNG_FILTER_TYPE_DEFAULT PNG_FILTER_TYPE_BASE
#define PNG_COMPRESSION_TYPE_BASE 0
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE
#define PNG_NO_FILTERS     0x00
#define PNG_FILTER_NONE    0x08
#define PNG_FILTER_SUB     0x10
#define PNG_FILTER_UP      0x20
#define PNG_FILTER_AVG     0x40
#define PNG_FILTER_PAETH   0x80
#define PNG_ALL_FILTERS (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | \
                         PNG_FILTER_AVG | PNG_FILTER_PAETH)

/*
====================
PNG_SaveImage_preflipped

Save a preflipped PNG image to a file
====================
*/
qbool PNG_SaveImage_preflipped (const char *filename, int width, int height, qbool has_alpha, unsigned char *data)
{
	unsigned int offset, linesize;
	qfile_t* file = NULL;
	void *png, *pnginfo;
	unsigned char ioBuffer[8192];
	int passes, i, j;

	// No DLL = no JPEGs
	if (!png_dll)
	{
		Con_Print("You need the libpng library to save PNG images\n");
		return false;
	}

	png = (void *)qpng_create_write_struct( 
		(qpng_access_version_number() / 100 == 102) ? PNG_LIBPNG_VER_STRING_12 :
		(qpng_access_version_number() / 100 == 104) ? PNG_LIBPNG_VER_STRING_14 :
		(qpng_access_version_number() / 100 == 105) ? PNG_LIBPNG_VER_STRING_15 :
		PNG_LIBPNG_VER_STRING_16, // nasty hack... whatever
		0, PNG_error_fn, PNG_warning_fn
	);
	if(!png)
		return false;
	pnginfo = (void *)qpng_create_info_struct(png);
	if(!pnginfo)
	{
		 qpng_destroy_write_struct(&png, NULL);
		 return false;
	}

	// this must be memset before the setjmp error handler, because it relies
	// on the fields in this struct for cleanup
	memset(&my_png, 0, sizeof(my_png));

	// NOTE: this relies on jmp_buf being the first thing in the png structure
	// created by libpng! (this is correct for libpng 1.2.x)
#ifdef WIN64
	if (setjmp((_JBTYPE *)png))
#elif defined(MACOSX) || defined(WIN32)
	if (setjmp((int *)png))
#elif defined(__ANDROID__)
	if (setjmp((long *)png))
#else
	if (setjmp((struct __jmp_buf_tag *)png))
#endif
	{
		qpng_destroy_write_struct(&png, &pnginfo);
		return false;
	}

	// Open the file
	file = FS_OpenRealFile(filename, "wb", true);
	if (!file)
		return false;
	my_png.outfile = file;
	qpng_set_write_fn(png, ioBuffer, PNG_fWriteData, PNG_fFlushData);

	//qpng_set_compression_level(png, Z_BEST_COMPRESSION);
	qpng_set_compression_level(png, Z_BEST_SPEED);
	qpng_set_IHDR(png, pnginfo, width, height, 8, has_alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	qpng_set_filter(png, 0, PNG_NO_FILTERS);
	qpng_write_info(png, pnginfo);
	qpng_set_packing(png);
	qpng_set_bgr(png);

	passes = qpng_set_interlace_handling(png);

	linesize = width * (has_alpha ? 4 : 3);
	offset = linesize * (height - 1);
	for(i = 0; i < passes; ++i)
		for(j = 0; j < height; ++j)
			qpng_write_row(png, &data[offset - j * linesize]);

	qpng_write_end(png, NULL);
	qpng_destroy_write_struct(&png, &pnginfo);

	FS_Close (file);

	return true;
}
