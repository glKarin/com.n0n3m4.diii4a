/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include <thread>
#include "renderer/tr_local.h"

#include <jpeglib.h>
#include <png.h>


/*
=========================================================

TARGA FORMAT

=========================================================
*/

/* more recent tga2 code, dont need no stinking hacks for flipped images, and also supports more formats */
#define TGA_MAXCOLORS 16384

/* Definitions for image types. */
#define TGA_Null		0	/* no image data */
#define TGA_Map			1	/* Uncompressed, color-mapped images. */
#define TGA_RGB			2	/* Uncompressed, RGB images. */
#define TGA_Mono		3	/* Uncompressed, black and white images. */
#define TGA_RLEMap		9	/* Runlength encoded color-mapped images. */
#define TGA_RLERGB		10	/* Runlength encoded RGB images. */
#define TGA_RLEMono		11	/* Compressed, black and white images. */
#define TGA_CompMap		32	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding. */
#define TGA_CompMap4	33	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process. */

/* Definitions for interleave flag. */
#define TGA_IL_None		0	/* non-interleaved. */
#define TGA_IL_Two		1	/* two-way (even/odd) interleaving */
#define TGA_IL_Four		2	/* four way interleaving */
#define TGA_IL_Reserved	3	/* reserved */

/* Definitions for origin flag */
#define TGA_O_UPPER		0	/* Origin in lower left-hand corner. */
#define TGA_O_LOWER		1	/* Origin in upper left-hand corner. */

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, colormap_size, image_type;
	unsigned char	pixel_size, attributes;
	unsigned short	colormap_index, colormap_length;
	unsigned short	x_origin, y_origin, width, height;
} TargaHeader;

/*
=============
idImageReader::LoadTGA
=============
*/
void idImageReader::LoadTGA() {
	if (!Preamble()) {
		Postamble();
		return;
	}

	int			w, h, x, y, realrow, truerow, baserow, i, temp1, temp2, pixel_size, map_idx;
	int			RLE_count, RLE_flag, size, interleave, origin;
	bool		mapped, rlencoded;
	byte		*data, *dst, r, g, b, a, j, k, l, *ColorMap;
	byte		*buf_p;
	TargaHeader	header;

	//dynamic memory: should be deleted even on error
	mapped = false;
	ColorMap = nullptr;
	data = nullptr;

	buf_p = srcBuffer;
	header.id_length = *buf_p++;
	header.colormap_type = *buf_p++;
	header.image_type = *buf_p++;
	header.colormap_index = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.colormap_length = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.colormap_size = *buf_p++;
	header.x_origin = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.y_origin = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.width = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.height = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.pixel_size = *buf_p++;
	header.attributes = *buf_p++;

	if ( header.id_length != 0 ) {
		buf_p += header.id_length;
	}

	/* validate TGA type */
	switch ( header.image_type ) {
	case TGA_Map:
	case TGA_RGB:
	case TGA_Mono:
	case TGA_RLEMap:
	case TGA_RLERGB:
	case TGA_RLEMono:
		break;
	default:
		common->Warning( "Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported" );
		goto onerror;
	}

	/* validate color depth */
	switch ( header.pixel_size ) {
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
		break;
	default:
		common->Warning( "Only 8, 15, 16, 24 or 32 bit images (with colormaps) supported" );
		goto onerror;
	}
	r = g = b = a = l = 0;

	/* if required, read the color map information. */
	mapped = ( header.image_type == TGA_Map || header.image_type == TGA_RLEMap ) && header.colormap_type == 1;

	if ( mapped ) {
		/* validate colormap size */
		switch ( header.colormap_size ) {
		case 8:
		case 15:
		case 16:
		case 32:
		case 24:
			break;
		default:
			common->Warning( "Only 8, 15, 16, 24 or 32 bit colormaps supported" );
			goto onerror;
		}
		temp1 = header.colormap_index;
		temp2 = header.colormap_length;

		if ( ( temp1 + temp2 + 1 ) >= TGA_MAXCOLORS ) {
			common->Warning( "Too many colors in colormap" );
			goto onerror;
		}
		ColorMap = ( byte * )R_StaticAlloc( TGA_MAXCOLORS * 4 );
		map_idx = 0;

		for ( i = temp1; i < temp1 + temp2; ++i, map_idx += 4 ) {
			/* read appropriate number of bytes, break into rgb & put in map. */
			switch ( header.colormap_size ) {
			case 8:	/* grey scale, read and triplicate. */
				r = g = b = *buf_p++;
				a = 255;
				break;
			case 15:	/* 5 bits each of red green and blue. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int )k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = 255;
				break;
			case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int )k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = ( k & 0x80 ) ? 255 : 0;
				break;
			case 24:	/* 8 bits each of blue, green and red. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = 255;
				l = 0;
				break;
			case 32:	/* 8 bits each of blue, green, red and alpha. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = *buf_p++;
				l = 0;
				break;
			}
			ColorMap[map_idx + 0] = r;
			ColorMap[map_idx + 1] = g;
			ColorMap[map_idx + 2] = b;
			ColorMap[map_idx + 3] = a;
		}
	}

	/* check run-length encoding. */
	rlencoded = ( header.image_type == TGA_RLEMap || header.image_type == TGA_RLERGB || header.image_type == TGA_RLEMono );
	RLE_count = RLE_flag = 0;
	w = header.width;
	h = header.height;
	size = w * h * 4;

	data = ( byte * )R_StaticAlloc( size );
	*dstWidth = w;
	*dstHeight = h;

	/* read the Targa file body and convert to portable format. */
	pixel_size = header.pixel_size;
	origin = ( header.attributes & 0x20 ) >> 5;
	interleave = ( header.attributes & 0xC0 ) >> 6;
	truerow = 0;
	baserow = 0;

	for ( y = 0; y < h; y++ ) {
		realrow = truerow;
		if ( origin == TGA_O_UPPER )	{
			realrow = h - realrow - 1;
		}
		dst = data + realrow * w * 4;

		//stgatilov: fast path for plain 8/24/32-bit images
		if ( !mapped && !rlencoded ) {
			if ( SIMDProcessor->ConvertRowToRGBA8(buf_p, w, pixel_size, true, dst) ) {
				int pixBytes = ((pixel_size + 1) >> 3);
				buf_p += pixBytes * w;
				dst += 4 * w;
				goto RowFinished;
			}
		}

		for ( x = 0; x < w; x++ ) {
			/* check if run length encoded. */
			if ( rlencoded )	{
				if ( !RLE_count ) {
					/* have to restart run. */
					i = *buf_p++;
					RLE_flag = ( i & 0x80 );
					if ( !RLE_flag )	{
						// stream of unencoded pixels
						RLE_count = i + 1;
					} else {
						// single pixel replicated
						RLE_count = i - 127;
					}
					/* decrement count & get pixel. */
					--RLE_count;
				} else {
					/* have already read count & (at least) first pixel. */
					--RLE_count;
					if ( RLE_flag ) {
						/* replicated pixels. */
						goto PixEncode;
					}
				}
			}

			/* read appropriate number of bytes, break into RGB. */
			switch ( pixel_size ) {
			case 8:	/* grey scale, read and triplicate. */
				r = g = b = l = *buf_p++;
				a = 255;
				break;
			case 15:	/* 5 bits each of red green and blue. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int )k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = 255;
				break;
			case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int )k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = ( k & 0x80 ) ? 255 : 0;
				break;
			case 24:	/* 8 bits each of blue, green and red. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = 255;
				l = 0;
				break;
			case 32:	/* 8 bits each of blue, green, red and alpha. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = *buf_p++;
				l = 0;
				break;
			default:
				common->Warning( "Illegal pixel_size '%d'", pixel_size );
				goto onerror;
			}
PixEncode:
			if ( mapped ) {
				map_idx = l * 4;
				*dst++ = ColorMap[map_idx + 0];
				*dst++ = ColorMap[map_idx + 1];
				*dst++ = ColorMap[map_idx + 2];
				*dst++ = ColorMap[map_idx + 3];
			} else {
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				*dst++ = a;
			}
		}
RowFinished:

		if ( interleave == TGA_IL_Four ) {
			truerow += 4;
		} else if ( interleave == TGA_IL_Two ) {
			truerow += 2;
		} else {
			truerow++;
		}

		if ( truerow >= h ) {
			truerow = ++baserow;
		}
	}

	//pass ownership over result to caller
	*dstData = data;
	data = nullptr;

onerror:
	//cleanup
	R_StaticFree(data);
	if ( mapped ) {
		R_StaticFree( ColorMap );
	}

	Postamble();
}

/*
================
R_WriteTGA
================
*/
void R_WriteTGA( const char *filename, const byte *data, int width, int height, bool flipVertical ) {
	idImageWriter wr;
	wr.Source(data, width, height);
	wr.Flip(flipVertical);
	idFile *f = fileSystem->OpenFileWrite(filename);
	wr.Dest(f);
	wr.WriteTGA();
}

/*
================
idImageWriter::WriteTGA
================
*/
bool idImageWriter::WriteTGA() {
	if (!Preamble()) {
		Postamble();
		return false;
	}

	// save alpha channel iff it is given to us
	int dstBpp = srcBpp;

	static const int TGA_HEADER_SIZE = 18;
	int dataSize = srcWidth * srcHeight * dstBpp;

	byte *buffer = ( byte * )Mem_Alloc( dataSize + TGA_HEADER_SIZE );
	memset( buffer, 0, TGA_HEADER_SIZE );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = srcWidth & 255;
	buffer[13] = srcWidth >> 8;
	buffer[14] = srcHeight & 255;
	buffer[15] = srcHeight >> 8;
	buffer[16] = 8 * dstBpp;	// pixel size

	if ( !flip ) {
		buffer[17] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}

	// swap rgba to bgra
	for ( int i = 0, j = 0 ; i < dataSize ; i += srcBpp, j += dstBpp ) {
		buffer[TGA_HEADER_SIZE + j + 0] = srcData[i + 2];		// blue
		buffer[TGA_HEADER_SIZE + j + 1] = srcData[i + 1];		// green
		buffer[TGA_HEADER_SIZE + j + 2] = srcData[i + 0];		// red
		if (dstBpp >= 4) {
			buffer[TGA_HEADER_SIZE + j + 3] = (srcBpp >= 4 ? srcData[i + 3] : 255);		// alpha
		}
	}

	dstFile->Write( buffer, dataSize + TGA_HEADER_SIZE );
	Mem_Free( buffer );

	Postamble();
	return true;
}


/*
=========================================================

JPG FORMAT

Interfaces with the huge libjpeg
=========================================================
*/

struct jpeg_error_mgr_tdm {
	/* "public" fields */
	struct jpeg_error_mgr base;
	/* for return to caller */
	jmp_buf setjmp_buffer;
	/* how many warnings happened */
	int warning_count;
};

static void jpg_output_message(j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);
	// print message to TDM console
	common->Warning("LibJPEG: %s", buffer);
}

static void jpg_emit_message(j_common_ptr cinfo, int msg_level) {
	/* cinfo->err actually points to a jpeg_error_mgr_tdm struct */
	jpeg_error_mgr_tdm* myerr = (jpeg_error_mgr_tdm*) cinfo->err;
	/* It's a trace message. */
	if (msg_level >= 0)
		return;
	/* Always print error message. */
	if (msg_level == INT_MIN) {
		(*myerr->base.output_message) (cinfo);
	} else {
		/* It's a warning message.  Since corrupt files may generate many warnings,
		* the policy implemented here is to show only the first warning. */
		if (myerr->warning_count == 0)
			(*myerr->base.output_message) (cinfo);
		/* Always count warnings. */
		myerr->warning_count++;
	}
}

static void jpg_error_exit(j_common_ptr cinfo) {
	/* cinfo->err actually points to a jpeg_error_mgr_tdm struct */
	jpeg_error_mgr_tdm* myerr = (jpeg_error_mgr_tdm*) cinfo->err;
	/* Print error message */
	jpg_emit_message(cinfo, INT_MIN);
	/* Jump to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

/*
=============
idImageReader::LoadJPG
=============
*/
void idImageReader::LoadJPG() {
	if (!Preamble()) {
		Postamble();
		return;
	}

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr_tdm jerr;

	//dynamic memory: should be freed even on error
	unsigned char *out = nullptr;
	byte *scanline = nullptr;

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error(&jerr.base);
	jerr.base.error_exit = jpg_error_exit;
	jerr.base.emit_message = jpg_emit_message;
	jerr.base.output_message = jpg_output_message;
	jerr.warning_count = 0;

	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has exited due to error. */
		goto onerror;
	}

	/* Now we can initialize the JPEG decompression object. */
	ExtLibs::jpeg_create_decompress( &cinfo );

	/* Step 2: specify data source (eg, a file) */

	ExtLibs::jpeg_mem_src( &cinfo, srcBuffer, srcLength );

	/* Step 3: read file parameters with jpeg_read_header() */

	( void ) ExtLibs::jpeg_read_header( &cinfo, boolean(true) );
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	( void ) ExtLibs::jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */

	*dstWidth = cinfo.output_width;
	*dstHeight = cinfo.output_height;
	if ( cinfo.output_components != 4 && cinfo.output_components != 3 ) {
		common->Warning( "JPG has unsupported color depth (%d)", cinfo.output_components );
		goto onerror;
	}

	/* JSAMPLEs per row in output buffer */
	int row_stride;
	row_stride = cinfo.output_width * 4;
	/* temporary buffer receiving one scanline per read */
	scanline = (byte*)Mem_Alloc(row_stride);
	/* buffer to hold uncompressed data */
	out = ( byte * )R_StaticAlloc( cinfo.output_width * cinfo.output_height * 4 );

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while ( cinfo.output_scanline < cinfo.output_height ) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		byte *dstScanline = out + row_stride * cinfo.output_scanline;
		( void )ExtLibs::jpeg_read_scanlines( &cinfo, &scanline, 1 );
		//copy the read scanline into output array
		for (uint i = 0; i < cinfo.output_width; i++) {
			dstScanline[4*i+0] = scanline[cinfo.output_components*i+0];
			dstScanline[4*i+1] = scanline[cinfo.output_components*i+1];
			dstScanline[4*i+2] = scanline[cinfo.output_components*i+2];
			// clear all the alphas to 255
			dstScanline[4*i+3] = 255;
		}
	}

	/* Step 7: Finish decompression */

	( void )ExtLibs::jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	//pass ownership to caller
	*dstData = out;
	out = nullptr;

	/* Step 8: Release JPEG decompression object */

onerror:
	/* This is an important step since it will release a good deal of memory. */
	ExtLibs::jpeg_destroy_decompress( &cinfo );

	//free memory
	R_StaticFree( out);
	Mem_Free( scanline );

	/* And we're done! */
	Postamble();
}

/*
================
idImageWriter::WriteJPG
================
*/
bool idImageWriter::WriteJPG(int quality) {
	if (!Preamble()) {
		Postamble();
		return false;
	}

	//dynamic memory: should be freed even on error
	byte *rgbScanline = NULL;
	byte *outputBuffer = NULL;
	bool success = false;

	/* This struct contains the JPEG compression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 * It is possible to have several such structures, representing multiple
	 * compression/decompression processes, in existence at once.  We refer
	 * to any one struct (and its associated working data) as a "JPEG object".
	 */
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	 * because applications often want to supply a specialized error handler
	 * (see the second half of this file for an example).  But here we just
	 * take the easy way out and use the standard error handler, which will
	 * print a message on stderr and call exit() if compression fails.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr_tdm jerr;

	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error(&jerr.base);
	jerr.base.error_exit = jpg_error_exit;
	jerr.base.emit_message = jpg_emit_message;
	jerr.base.output_message = jpg_output_message;
	jerr.warning_count = 0;

	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has exited due to error. */
		goto onerror;
	}

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	 * stdio stream.  You can also write your own code to do something else.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to write binary files.
	 */
	size_t outputSize;
	jpeg_mem_dest(&cinfo, &outputBuffer, &outputSize);

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	 * Four fields of the cinfo struct must be filled in:
	 */
	cinfo.image_width = srcWidth; 	/* image width and height, in pixels */
	cinfo.image_height = srcHeight;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	 * (You must set at least cinfo.in_color_space before calling this,
	 * since the defaults depend on the source color space.)
	 */
	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	 * Here we just illustrate the use of quality (quantization table) scaling:
	 */
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	 * Pass TRUE unless you are very sure of what you're doing.
	 */
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	int row_stride;		/* physical row width in image buffer */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */

	/* Here we use the library's state variable cinfo.next_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 * To keep things simple, we pass one scanline per call; you can pass
	 * more if you wish, though.
	 */
	row_stride = srcWidth * srcBpp;	/* bytes per row in input buffer */
	if (srcBpp != 3)
		rgbScanline = (byte*)Mem_Alloc(row_stride);

	while (cinfo.next_scanline < cinfo.image_height) {
		//copy row to output, converting it from RGBA to RGB
		int srcRow = cinfo.next_scanline;
		if (flip)
			srcRow = srcHeight - 1 - srcRow;
		const byte *source = &srcData[srcRow * row_stride];
		if (srcBpp != 3) {
			for (int i = 0; i < srcWidth; i++) {
				rgbScanline[3 * i + 0] = source[srcBpp * i + 0];
				rgbScanline[3 * i + 1] = source[srcBpp * i + 1];
				rgbScanline[3 * i + 2] = source[srcBpp * i + 2];
			}
			row_pointer[0] = rgbScanline;
		}
		else
			row_pointer[0] = (byte*) source;
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	/* After finish_compress, write buffer to file. */
	dstFile->Write( outputBuffer, outputSize );

	success = true;

onerror:
	/* Step 7: release JPEG compression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);

	//cleanup
	free(outputBuffer);
	Mem_Free(rgbScanline);

	/* And we're done! */
	Postamble();
	return success;
}


/*
=========================================================

PNG FORMAT

Interfaces with the huge libpng
=========================================================
*/

static void png_error_handler(png_structp png_ptr, png_const_charp message) {
	common->Warning("LibPNG error: %s", message);
	//note: libpng will longjmp back to our code after that
}

static void png_warning_handler(png_structp png_ptr, png_const_charp message) {
	common->Warning("LibPNG warning: %s", message);
}

static png_voidp png_malloc_fn(png_structp png_ptr, png_size_t size) {
	return Mem_Alloc(size);
}

static void png_free_fn(png_structp png_ptr, png_voidp ptr) {
	return Mem_Free(ptr);
}

static void png_read_data_fn(png_structp png_ptr, png_bytep buffer, png_size_t size) {
	idFile *f = (idFile*)png_get_io_ptr(png_ptr);
	f->Read(buffer, size);
}	

static void png_write_data_fn(png_structp png_ptr, png_bytep buffer, png_size_t size) {
	idFile *f = (idFile*)png_get_io_ptr(png_ptr);
	f->Write(buffer, size);
}	

static void png_output_flush_fn(png_structp png_ptr) {
	idFile *f = (idFile*)png_get_io_ptr(png_ptr);
	f->Flush();
}

/*
=============
idImageReader::LoadPNG
=============
*/
void idImageReader::LoadPNG() {
	if (!Preamble()) {
		Postamble();
		return;
	}

	// wrap data into memory file
	idFile_Memory memfile("LoadPNG_memfile", (char*)srcBuffer, srcLength, false);

	//dynamic memory: should be freed even on error
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	byte *image_buffer = nullptr;
	png_bytep *row_pointers = nullptr;

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.  REQUIRED.
	 */
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, png_error_handler, png_warning_handler
	);

	if (png_ptr == NULL) {
		common->Warning("Failed to initialize LibPNG structure");
		goto onerror;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		common->Warning("Failed to initialize LibPNG structure");
		goto onerror;
	} 

	/* Set error handling if you are using the setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in the png_create_read_struct() earlier.
	 */
	if (setjmp(png_jmpbuf(png_ptr))) {
		goto onerror;
	}

	/* If you are using replacement read functions, instead of calling
	 * png_init_io(), you would call:
	 */
	png_set_read_fn(png_ptr, &memfile, png_read_data_fn);

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED.
	 */
	png_read_info(png_ptr, info_ptr);

	png_uint_32 w, h;
	int bit_depth, color_type, interlace_method;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
		&interlace_method, NULL, NULL);

	*dstWidth = w;
	*dstHeight = h;

	/* Set up the data transformations you want.  Note that these are all
	 * optional.  Only call them if you want/need them.  Many of the
	 * transformations only work on specific types of images, and many
	 * are mutually exclusive.
	 */

	/* Expand grayscale images to the full 8 bits from 1, 2 or 4 bits/pixel. */
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	/* Expand paletted colors into true RGB triplets. */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	/* Expand the grayscale to 24-bit RGB if necessary. */
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	/* Tell libpng to strip 16 bits/color files down to 8 bits/color.
	 * Use accurate scaling. */
	if (bit_depth == 16)
		png_set_scale_16(png_ptr);
	/* Expand paletted or RGB images with transparency to full alpha channels
	 * so the data will be available as RGBA quartets. */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0)
		png_set_tRNS_to_alpha(png_ptr);
	else {
		/* Add filler (or alpha) byte (before/after each RGB triplet). */
		if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	}
	/* Turn on interlace handling.  REQUIRED if you are not using
	 * png_read_image().  To see how to handle interlacing passes,
	 * see the png_read_row() method below:
	 * stgatilov: it is also necessary to avoid warnings on interlaced images
	 */
	png_set_interlace_handling(png_ptr);

	/* Optional call to gamma correct and add the background to the palette
	 * and update info structure.  REQUIRED if you are expecting libpng to
	 * update the palette for you (i.e. you selected such a transform above).
	 * stgatilov: actually, it is necessary to update png_get_rowbytes for transformations
	 */
	png_read_update_info(png_ptr, info_ptr);
	// check that all transformations are set properly
	int need_bytes;
	need_bytes = png_get_rowbytes(png_ptr, info_ptr);
	if (need_bytes != w * 4) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		common->Error("Wrong row size while reading png (%d bytes for %d pixels)", need_bytes, w);
	}

	/* Allocate the memory to hold the image using the fields of info_ptr. */
	image_buffer = (byte*) R_StaticAlloc(w * h * 4);
	row_pointers = (png_bytep *) Mem_Alloc(h * sizeof(png_bytep));
	for (uint row = 0; row < h; row++) {
		row_pointers[row] = image_buffer + row * w * 4;
	}

	/* Read the entire image in one go */
	png_read_image(png_ptr, row_pointers);

	/* Read rest of file, and get additional chunks in info_ptr.  REQUIRED. */
	png_read_end(png_ptr, info_ptr);

	//pass ownership to caller
	*dstData = image_buffer;
	image_buffer = nullptr;

onerror:
	/* Clean up after the read, and free any memory allocated.  REQUIRED. */
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	//cleanup
	R_StaticFree(image_buffer);
	Mem_Free(row_pointers);

	/* That's it! */
	Postamble();
}

/*
================
idImageWriter::WritePNG
================
*/
bool idImageWriter::WritePNG(int level) {
	if (!Preamble()) {
		Postamble();
		return false;
	}

	//dynamic memory: should be freed even on error
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	png_bytep *row_pointers = nullptr;
	bool success = false;

	//for now, let's store alpha iff we are given alpha
	int dstBpp = srcBpp;

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also check that
	 * the library version is compatible with the one used at compile time,
	 * in case we are using dynamically linked libraries.  REQUIRED.
	 */
	png_ptr = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, png_error_handler, png_warning_handler
	);
	if (png_ptr == NULL) {
		common->Warning("Failed to initialize LibPNG structure");
		goto onerror;
	}

	/* Allocate/initialize the image information data.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		common->Warning("Failed to initialize LibPNG structure");
		goto onerror;
	} 

	/* Set up error handling.  REQUIRED if you aren't supplying your own
	 * error handling functions in the png_create_write_struct() call.
	 */
	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem writing the file. */
		goto onerror;
	}

	/* I/O initialization method 2 */
	/* If you are using replacement write functions, instead of calling
	 * png_init_io(), you would call:
	 */
	png_set_write_fn(png_ptr, dstFile, png_write_data_fn, png_output_flush_fn);
	/* where user_io_ptr is a structure you want available to the callbacks. */

	//use our memory allocation routines
	png_set_mem_fn(png_ptr, NULL, png_malloc_fn, png_free_fn);

	//set compression level for DEFLATE
	png_set_compression_level(png_ptr, level);

	/* Set the image information here.  Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8 or 16, but valid values also depend on
	 * the color_type selected.  color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	 * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	 * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	 * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE.
	 * REQUIRED.
	 */
	png_set_IHDR(
		png_ptr, info_ptr, srcWidth, srcHeight, 8,
		dstBpp == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);

	/* Write the file header information.  REQUIRED. */
	png_write_info(png_ptr, info_ptr);

	row_pointers = (png_bytep*)Mem_Alloc(srcHeight * sizeof(png_bytep));
	/* Set up pointers into your "image" byte array. */
	for (int k = 0; k < srcHeight; k++) {
		int srcRow = k;
		if (flip)
			srcRow = srcHeight - 1 - srcRow;
		row_pointers[k] = (png_bytep)(srcData + srcRow * srcWidth * srcBpp);
	}

	/* Write out the entire image data in one call */
	png_write_image(png_ptr, row_pointers);

	/* It is REQUIRED to call this to finish writing the rest of the file. */
	png_write_end(png_ptr, info_ptr);

	success = true;

onerror:
	/* Clean up after the write, and free any allocated memory. */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	//cleanup
	Mem_Free(row_pointers);

	Postamble();
	return success;
}

/*
=========================================================

BMP FORMAT

=========================================================
*/

typedef struct {
	char id[2];
	unsigned int fileSize;
	unsigned int reserved0;
	unsigned int bitmapDataOffset;
	unsigned int bitmapHeaderSize;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned int compression;
	unsigned int bitmapDataSize;
	unsigned int hRes;
	unsigned int vRes;
	unsigned int colors;
	unsigned int importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

/*
==============
idImageReader::LoadBMP
==============
*/
void idImageReader::LoadBMP() {
	if (!Preamble()) {
		Postamble();
		return;
	}

	int			columns, rows, numPixels;
	byte		*pixbuf;
	int			row, column;
	byte		*buf_p;
	BMPHeader_t bmpHeader;
	byte		*bmpRGBA;

	//dynamic memory: should be freed even on error
	bmpRGBA = nullptr;

	buf_p = srcBuffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleInt( * ( int * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleInt( * ( int * ) buf_p );
	buf_p += 4;

	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );

	if ( bmpHeader.bitsPerPixel == 8 ) {
		buf_p += 1024;
	}

	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) {
		common->Warning( "LoadBMP: only Windows-style BMP files supported" );
		goto onerror;
	}

	if ( bmpHeader.fileSize != srcLength ) {
		common->Warning( "LoadBMP: header size does not match file size (%u vs. %d)", bmpHeader.fileSize, srcLength );
		goto onerror;
	}

	if ( bmpHeader.compression != 0 ) {
		common->Warning( "LoadBMP: only uncompressed BMP files supported" );
		goto onerror;
	}

	if ( bmpHeader.bitsPerPixel < 8 ) {
		common->Warning( "LoadBMP: monochrome and 4-bit BMP files not supported" );
		goto onerror;
	}
	columns = bmpHeader.width;
	rows = bmpHeader.height;

	if ( rows < 0 ) {
		rows = -rows;
	}
	numPixels = columns * rows;

	*dstWidth = columns;
	*dstHeight = rows;
	bmpRGBA = ( byte * )R_StaticAlloc( numPixels * 4 );

	for ( row = rows - 1; row >= 0; row-- ) {
		pixbuf = bmpRGBA + row * columns * 4;

		for ( column = 0; column < columns; column++ ) {
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;

			switch ( bmpHeader.bitsPerPixel ) {
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = * ( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;

			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				common->Warning( "LoadBMP: illegal pixel_size '%d'", bmpHeader.bitsPerPixel );
				goto onerror;
			}
		}
	}

	//pass ownership to caller
	*dstData = bmpRGBA;
	bmpRGBA = nullptr;

onerror:
	//cleanup
	R_StaticFree(bmpRGBA);

	Postamble();
}


/*
===================================================================

AUTOMATIC FORMAT

===================================================================
*/


/*
=================
R_LoadImage

Loads any of the supported image types into a canonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timestamp.
=================
*/
void R_LoadImage( const char *originalFilename, byte **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	//clear output variables
	if ( pic )
		*pic = nullptr;
	if ( timestamp )
		*timestamp = -1;
	if ( width )
		*width = 0;
	if ( height )
		*height = 0;

	//see what extension the name has originally
	idStr name = originalFilename;
	idStr ext, originalExtension;
	name.ExtractFileExtension( originalExtension );

	//we try all these extensions in this order, and load the first file which exists
	//note: if specified filename already has extension, then we try this extension first
	static const char *ExtensionsOrder[] = { "tga", "jpg", "png", "bmp", 0 };

	idFile *file = nullptr;
	for ( int i = -1; ; i++ ) {
		const char* tryExtension = ( i < 0 ? originalExtension.c_str() : ExtensionsOrder[i] );
		if ( !tryExtension )
			break;		//no more extensions in list
		if ( !tryExtension[0] )
			continue;	//original filename has no extension

		name.SetFileExtension( tryExtension );
		name.ToLower();

		file = fileSystem->OpenFileRead( name.c_str() );
		if ( file ) {
			ext = tryExtension;
			break;	//opened file successfully
		}
	}

	//handle special cases and read timestamp
	if ( !file )
		return;
	if ( timestamp )
		*timestamp = file->Timestamp();
	if ( file->Length() <= 0 ) {
		fileSystem->CloseFile(file);
		return;
	}
	if ( !pic ) {
		fileSystem->CloseFile(file);
		return;			//just getting timestamp
	}

	//actually load image
	int w, h;
	byte *data;
	idImageReader rd;
	rd.Source(file);
	rd.Dest(data, w, h);
	rd.LoadExtension(ext.c_str());

	//drop image with no pixels
	if ( (w <= 0 || h <= 0) && data ) {
		R_StaticFree( data );
		data = nullptr;
	}

	//store output info
	if ( pic )
		*pic = data;
	if ( width )
		*width = w;
	if ( height )
		*height = h;
}

/*
================
idImageReader::LoadExtension
================
*/
void idImageReader::LoadExtension(const char *extension) {
	//determine extension automatically if NULL is passed
	idStr autoExt;
	if (!extension) {
		idStr filename = srcFile->GetName();
		filename.ExtractFileExtension(autoExt);
		extension = autoExt.c_str();
	}
	//load image from file
	if ( idStr::Icmp(extension, "tga") == 0 ) {
		LoadTGA();
	} else if ( idStr::Icmp(extension, "bmp") == 0 ) {
		LoadBMP();
	} else if ( idStr::Icmp(extension, "jpg") == 0 ) {
		LoadJPG();
	} else if ( idStr::Icmp(extension, "png") == 0 ) {
		LoadPNG();
	} else {
		common->Warning( "Cannot load image with extension %s", extension );
		if (dstData) *dstData = nullptr;
		if (dstWidth) *dstWidth = 0;
		if (dstHeight) *dstHeight = 0;
	}
}

/*
================
idImageReader::Preamble
================
*/
bool idImageReader::Preamble() {
	if (!dstData) {
		common->Warning("idImageReader: no output buffer");
		return false;
	}
	assert(dstWidth && dstHeight);
	*dstData = nullptr;
	*dstWidth = *dstHeight = 0;

	if (!srcFile) {
		common->Warning("idImageReader: no source file");
		return false;
	}
	if ( srcFile->Length() <= 0 ) {
		common->Warning("idImageReader: file is empty");
		return false;
	}
	if ( srcFile->Tell() != 0 ) {
		//LoadXXX use srcFile->Length() to decide how much to read
		common->Warning("idImageReader: filepos must be at start");
		return false;
	}

	//read whole file
	srcLength = srcFile->Length();
	srcBuffer = (byte*)Mem_Alloc(srcLength);
	srcFile->Read(srcBuffer, srcLength);

	return true;	//continue loading
}

/*
================
idImageReader::Postamble
================
*/
void idImageReader::Postamble() {
	if (srcClose) {
		fileSystem->CloseFile(srcFile);
		srcFile = nullptr;
	}

	Mem_Free(srcBuffer);
	srcBuffer = nullptr;
}

/*
================
idImageWriter::WriteExtension
================
*/
bool idImageWriter::WriteExtension(const char *extension) {
	if (idStr::Icmp(extension, "tga") == 0) {
		return WriteTGA();
	} else if (idStr::Icmp(extension, "jpg") == 0) {
		return WriteJPG();
	} else if (idStr::Icmp(extension, "png") == 0) {
		return WritePNG();
	} else {
		common->Warning("Unknown image extension [%s], using TGA format", extension);
		return WriteTGA();
	}
}

/*
================
idImageWriter::Preamble
================
*/
bool idImageWriter::Preamble() {
	if (!dstFile) {
		common->Warning("ImageWriter: no output file");
		return false;
	}
	if (!srcData) {
		common->Warning("ImageWriter: no source data");
		return false;
	}
	if (dstFile->Tell() != 0) {
		common->Warning("ImageWriter: filepos must be at start");
		return false;
	}
	return true;	//continue writing
}

/*
================
idImageWriter::Postamble
================
*/
void idImageWriter::Postamble() {
	if (dstClose) {
		fileSystem->CloseFile( dstFile );
		dstFile = nullptr;
	}
}

/*
===================================================================

DDS AND COMPRESSED TEXTURES

===================================================================
*/

/*
================
R_LoadCompressedImage
================
*/
void R_LoadCompressedImage( const char *filename, imageCompressedData_t **pic, ID_TIME_T *timestamp ) {
	if ( pic )
		*pic = nullptr;
	if ( timestamp )
		*timestamp = -1;

	// get the file timestamp
	ID_TIME_T fileTimestamp;
	fileSystem->ReadFile( filename, NULL, &fileTimestamp );

	if ( timestamp )
		*timestamp = fileTimestamp;
	if ( fileTimestamp == FILE_NOT_FOUND_TIMESTAMP )
		return;
	if ( !pic )
		return;	//no need to load, just checking timestamp

	// open it and read the header
	idFile *f = fileSystem->OpenFileRead( filename );
	if ( !f )
		return;
	int	len = f->Length();

	if ( len < 4 + sizeof( ddsFileHeader_t ) ) {
		fileSystem->CloseFile( f );
		return;
	}

	imageCompressedData_t *compData = (imageCompressedData_t*) R_StaticAlloc(
		imageCompressedData_t::TotalSizeFromFileSize( len )
	);

	compData->fileSize = len;
	f->Read( compData->GetFileData(), len );

	fileSystem->CloseFile( f );

	if ( compData->magic != DDS_MAKEFOURCC( 'D', 'D', 'S', ' ' ) ) {
		common->Printf( "R_LoadCompressedImage( %s ): magic != 'DDS '\n", filename );
		R_StaticFree( compData );
		return;
	}

	ddsFileHeader_t	*header = &compData->header;
	// ( not byte swapping dwReserved1 dwReserved2 )
	header->dwSize = LittleInt( header->dwSize );
	header->dwFlags = LittleInt( header->dwFlags );
	header->dwHeight = LittleInt( header->dwHeight );
	header->dwWidth = LittleInt( header->dwWidth );
	header->dwPitchOrLinearSize = LittleInt( header->dwPitchOrLinearSize );
	header->dwDepth = LittleInt( header->dwDepth );
	header->dwMipMapCount = LittleInt( header->dwMipMapCount );
	header->dwCaps1 = LittleInt( header->dwCaps1 );
	header->dwCaps2 = LittleInt( header->dwCaps2 );
	header->ddspf.dwSize = LittleInt( header->ddspf.dwSize );
	header->ddspf.dwFlags = LittleInt( header->ddspf.dwFlags );
	header->ddspf.dwFourCC = LittleInt( header->ddspf.dwFourCC );
	header->ddspf.dwRGBBitCount = LittleInt( header->ddspf.dwRGBBitCount );
	header->ddspf.dwRBitMask = LittleInt( header->ddspf.dwRBitMask );
	header->ddspf.dwGBitMask = LittleInt( header->ddspf.dwGBitMask );
	header->ddspf.dwBBitMask = LittleInt( header->ddspf.dwBBitMask );
	header->ddspf.dwABitMask = LittleInt( header->ddspf.dwABitMask );

	*pic = compData;
}

/*
================
imageCompressedData_s::ComputeUncompressedData
================
*/
byte *imageCompressedData_s::ComputeUncompressedData() const {
	int w = header.dwWidth;
	int h = header.dwHeight;

	TRACE_CPU_SCOPE( "Decompress:DDS" )

	byte *resData = (byte*) R_StaticAlloc(w * h * 4);

	if ( header.ddspf.dwFlags & DDSF_FOURCC ) {
		switch ( header.ddspf.dwFourCC ) {
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '1' ):
			if ( header.ddspf.dwFlags & DDSF_ALPHAPIXELS ) {
				SIMDProcessor->DecompressRGBA8FromDXT1( contents, w, h, resData, 4 * w, true );
			} else {
				SIMDProcessor->DecompressRGBA8FromDXT1( contents, w, h, resData, 4 * w, false );
			}
			return resData;
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '3' ):
			SIMDProcessor->DecompressRGBA8FromDXT3( contents, w, h, resData, 4 * w );
			return resData;
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '5' ):
			SIMDProcessor->DecompressRGBA8FromDXT5( contents, w, h, resData, 4 * w );
			return resData;
		case DDS_MAKEFOURCC( 'R', 'X', 'G', 'B' ):
			SIMDProcessor->DecompressRGBA8FromDXT5( contents, w, h, resData, 4 * w );
			return resData;
		case DDS_MAKEFOURCC( 'A', 'T', 'I', '2' ):
			SIMDProcessor->DecompressRGBA8FromRGTC( contents, w, h, resData, 4 * w );
			return resData;
		}
	} else if ( ( header.ddspf.dwFlags & DDSF_RGBA ) && header.ddspf.dwRGBBitCount == 32 ) {
		SIMDProcessor->ConvertRowToRGBA8( contents, w * h, 32, true, resData );
		return resData;
	} else if ( ( header.ddspf.dwFlags & DDSF_RGB ) && header.ddspf.dwRGBBitCount == 32 ) {
		SIMDProcessor->ConvertRowToRGBA8( contents, w * h, 32, true, resData );
		return resData;
	} else if ( ( header.ddspf.dwFlags & DDSF_RGB ) && header.ddspf.dwRGBBitCount == 24 ) {
		SIMDProcessor->ConvertRowToRGBA8( contents, w * h, 24, true, resData );
		return resData;
	} else if ( header.ddspf.dwRGBBitCount == 8 ) {
		SIMDProcessor->ConvertRowToRGBA8( contents, w * h, 8, true, resData );
		return resData;
	}

	//unsupported format
	R_StaticFree( resData );
	return nullptr;
}

/*
===================================================================

CUBE MAPS

===================================================================
*/

/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
const char *cubemapFaceNamesCamera[6] = { "_forward.tga", "_back.tga", "_left.tga", "_right.tga", "_up.tga", "_down.tga" };
const char *cubemapFaceNamesNative[6] = { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga", "_pz.tga", "_nz.tga" };

bool R_LoadCubeImages( const char *imgName, cubeFiles_t extensions, byte *pics[6], int *outSize, ID_TIME_T *timestamp ) {
	int			i, j;
	const char	**sides;
	char		fullName[MAX_IMAGE_NAME];
	int			width, height, size = 0;

	if ( extensions == CF_CAMERA ) {
		sides = cubemapFaceNamesCamera;
	} else {
		sides = cubemapFaceNamesNative;
	}

	if ( pics ) {
		memset( pics, 0, 6 * sizeof( pics[0] ) );
	}

	if ( timestamp ) {
		*timestamp = 0;
	}

	for ( i = 0 ; i < 6 ; i++ ) {
		idStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );
		ID_TIME_T thisTime;
		if ( !pics ) {
			// just checking timestamps
			R_LoadImageProgram( fullName, nullptr, &width, &height, &thisTime );
		} else {
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}

		if ( thisTime == FILE_NOT_FOUND_TIMESTAMP ) {
			break;
		}

		if ( i == 0 ) {
			size = width;
		}

		if ( width != size || height != size ) {
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}

		if ( timestamp ) {
			if ( thisTime > *timestamp ) {
				*timestamp = thisTime;
			}
		}

		if ( pics && extensions == CF_CAMERA ) {
			// convert from "camera" images to native cube map images
			switch ( i ) {
			case 0:	// forward
				R_RotatePic( pics[i], width );
				break;
			case 1:	// back
				R_RotatePic( pics[i], width );
				R_HorizontalFlip( pics[i], width, height );
				R_VerticalFlip( pics[i], width, height );
				break;
			case 2:	// left
				R_VerticalFlip( pics[i], width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( pics[i], width, height );
				break;
			case 4:	// up
				R_RotatePic( pics[i], width );
				break;
			case 5: // down
				R_RotatePic( pics[i], width );
				break;
			}
		}
	}

	if ( i != 6 ) {
		// we had an error, so free everything
		if ( pics ) {
			for ( j = 0 ; j < i ; j++ ) {
				R_StaticFree( pics[j] );
			}
		}

		if ( timestamp ) {
			*timestamp = 0;
		}
		return false;
	}

	if ( outSize ) {
		*outSize = size;
	}
	return true;
}
