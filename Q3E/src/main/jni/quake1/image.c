
#include "quakedef.h"
#include "image.h"
#include "jpeg.h"
#include "image_png.h"
#include "r_shadow.h"
#include "wad.h"

int		image_width;
int		image_height;

static unsigned char *Image_GetEmbeddedPicBGRA(const char *name);

static void Image_CopyAlphaFromBlueBGRA(unsigned char *outpixels, const unsigned char *inpixels, int w, int h)
{
	int i, n;
	n = w * h;
	for(i = 0; i < n; ++i)
		outpixels[4*i+3] = inpixels[4*i]; // blue channel
}

#if 1
// written by LadyHavoc in a readable way, optimized by Vic, further optimized by LadyHavoc (the non-special index case), readable version preserved below this
void Image_CopyMux(unsigned char *outpixels, const unsigned char *inpixels, int inputwidth, int inputheight, qbool inputflipx, qbool inputflipy, qbool inputflipdiagonal, int numoutputcomponents, int numinputcomponents, int *outputinputcomponentindices)
{
	int index, c, x, y;
	const unsigned char *in, *line;
	int row_inc = (inputflipy ? -inputwidth : inputwidth) * numinputcomponents, col_inc = (inputflipx ? -1 : 1) * numinputcomponents;
	int row_ofs = (inputflipy ? (inputheight - 1) * inputwidth * numinputcomponents : 0), col_ofs = (inputflipx ? (inputwidth - 1) * numinputcomponents : 0);

	for (c = 0; c < numoutputcomponents; c++)
		if (outputinputcomponentindices[c] & 0x80000000)
			break;
	if (c < numoutputcomponents)
	{
		// special indices used
		if (inputflipdiagonal)
		{
			for (x = 0, line = inpixels + col_ofs; x < inputwidth; x++, line += col_inc)
				for (y = 0, in = line + row_ofs; y < inputheight; y++, in += row_inc, outpixels += numoutputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = ((index = outputinputcomponentindices[c]) & 0x80000000) ? index : in[index];
		}
		else
		{
			for (y = 0, line = inpixels + row_ofs; y < inputheight; y++, line += row_inc)
				for (x = 0, in = line + col_ofs; x < inputwidth; x++, in += col_inc, outpixels += numoutputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = ((index = outputinputcomponentindices[c]) & 0x80000000) ? index : in[index];
		}
	}
	else
	{
		// special indices not used
		if (inputflipdiagonal)
		{
			for (x = 0, line = inpixels + col_ofs; x < inputwidth; x++, line += col_inc)
				for (y = 0, in = line + row_ofs; y < inputheight; y++, in += row_inc, outpixels += numoutputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = in[outputinputcomponentindices[c]];
		}
		else
		{
			for (y = 0, line = inpixels + row_ofs; y < inputheight; y++, line += row_inc)
				for (x = 0, in = line + col_ofs; x < inputwidth; x++, in += col_inc, outpixels += numoutputcomponents)
					for (c = 0; c < numoutputcomponents; c++)
						outpixels[c] = in[outputinputcomponentindices[c]];
		}
	}
}
#else
// intentionally readable version
void Image_CopyMux(unsigned char *outpixels, const unsigned char *inpixels, int inputwidth, int inputheight, qbool inputflipx, qbool inputflipy, qbool inputflipdiagonal, int numoutputcomponents, int numinputcomponents, int *outputinputcomponentindices)
{
	int index, c, x, y;
	const unsigned char *in, *inrow, *incolumn;
	if (inputflipdiagonal)
	{
		for (x = 0;x < inputwidth;x++)
		{
			for (y = 0;y < inputheight;y++)
			{
				in = inpixels + ((inputflipy ? inputheight - 1 - y : y) * inputwidth + (inputflipx ? inputwidth - 1 - x : x)) * numinputcomponents;
				for (c = 0;c < numoutputcomponents;c++)
				{
					index = outputinputcomponentindices[c];
					if (index & 0x80000000)
						*outpixels++ = index;
					else
						*outpixels++ = in[index];
				}
			}
		}
	}
	else
	{
		for (y = 0;y < inputheight;y++)
		{
			for (x = 0;x < inputwidth;x++)
			{
				in = inpixels + ((inputflipy ? inputheight - 1 - y : y) * inputwidth + (inputflipx ? inputwidth - 1 - x : x)) * numinputcomponents;
				for (c = 0;c < numoutputcomponents;c++)
				{
					index = outputinputcomponentindices[c];
					if (index & 0x80000000)
						*outpixels++ = index;
					else
						*outpixels++ = in[index];
				}
			}
		}
	}
}
#endif

void Image_GammaRemapRGB(const unsigned char *in, unsigned char *out, int pixels, const unsigned char *gammar, const unsigned char *gammag, const unsigned char *gammab)
{
	while (pixels--)
	{
		out[0] = gammar[in[0]];
		out[1] = gammag[in[1]];
		out[2] = gammab[in[2]];
		in += 3;
		out += 3;
	}
}

// note: pal must be 32bit color
void Image_Copy8bitBGRA(const unsigned char *in, unsigned char *out, int pixels, const unsigned int *pal)
{
	int *iout = (int *)out;
	while (pixels >= 8)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		iout[2] = pal[in[2]];
		iout[3] = pal[in[3]];
		iout[4] = pal[in[4]];
		iout[5] = pal[in[5]];
		iout[6] = pal[in[6]];
		iout[7] = pal[in[7]];
		in += 8;
		iout += 8;
		pixels -= 8;
	}
	if (pixels & 4)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		iout[2] = pal[in[2]];
		iout[3] = pal[in[3]];
		in += 4;
		iout += 4;
	}
	if (pixels & 2)
	{
		iout[0] = pal[in[0]];
		iout[1] = pal[in[1]];
		in += 2;
		iout += 2;
	}
	if (pixels & 1)
		iout[0] = pal[in[0]];
}

/*
=================================================================

  PCX Loading

=================================================================
*/

typedef struct pcx_s
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
} pcx_t;

/*
============
LoadPCX
============
*/
static unsigned char* LoadPCX_BGRA (const unsigned char *f, int filesize, int *miplevel)
{
	pcx_t pcx;
	unsigned char *a, *b, *image_buffer, *pbuf;
	const unsigned char *palette, *fin, *enddata;
	int x, y, x2, dataByte;

	if (filesize < (int)sizeof(pcx) + 768)
	{
		Con_Print("Bad pcx file\n");
		return NULL;
	}

	fin = f;

	memcpy(&pcx, fin, sizeof(pcx));
	fin += sizeof(pcx);

	// LadyHavoc: big-endian support ported from QF newtree
	pcx.xmax = LittleShort (pcx.xmax);
	pcx.xmin = LittleShort (pcx.xmin);
	pcx.ymax = LittleShort (pcx.ymax);
	pcx.ymin = LittleShort (pcx.ymin);
	pcx.hres = LittleShort (pcx.hres);
	pcx.vres = LittleShort (pcx.vres);
	pcx.bytes_per_line = LittleShort (pcx.bytes_per_line);
	pcx.palette_type = LittleShort (pcx.palette_type);

	image_width = pcx.xmax + 1 - pcx.xmin;
	image_height = pcx.ymax + 1 - pcx.ymin;
	if (pcx.manufacturer != 0x0a || pcx.version != 5 || pcx.encoding != 1 || pcx.bits_per_pixel != 8 || image_width > 32768 || image_height > 32768 || image_width <= 0 || image_height <= 0)
	{
		Con_Print("Bad pcx file\n");
		return NULL;
	}

	palette = f + filesize - 768;

	image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width*image_height*4);
	if (!image_buffer)
	{
		Con_Printf("LoadPCX: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}
	pbuf = image_buffer + image_width*image_height*3;
	enddata = palette;

	for (y = 0;y < image_height && fin < enddata;y++)
	{
		a = pbuf + y * image_width;
		for (x = 0;x < image_width && fin < enddata;)
		{
			dataByte = *fin++;
			if(dataByte >= 0xC0)
			{
				if (fin >= enddata)
					break;
				x2 = x + (dataByte & 0x3F);
				dataByte = *fin++;
				if (x2 > image_width)
					x2 = image_width; // technically an error
				while(x < x2)
					a[x++] = dataByte;
			}
			else
				a[x++] = dataByte;
		}
		while(x < image_width)
			a[x++] = 0;
	}

	a = image_buffer;
	b = pbuf;

	for(x = 0;x < image_width*image_height;x++)
	{
		y = *b++ * 3;
		*a++ = palette[y+2];
		*a++ = palette[y+1];
		*a++ = palette[y];
		*a++ = 255;
	}

	return image_buffer;
}

/*
============
LoadPCX
============
*/
qbool LoadPCX_QWSkin(const unsigned char *f, int filesize, unsigned char *pixels, int outwidth, int outheight)
{
	pcx_t pcx;
	unsigned char *a;
	const unsigned char *fin, *enddata;
	int x, y, x2, dataByte, pcxwidth, pcxheight;

	if (filesize < (int)sizeof(pcx) + 768)
		return false;

	image_width = outwidth;
	image_height = outheight;
	fin = f;

	memcpy(&pcx, fin, sizeof(pcx));
	fin += sizeof(pcx);

	// LadyHavoc: big-endian support ported from QF newtree
	pcx.xmax = LittleShort (pcx.xmax);
	pcx.xmin = LittleShort (pcx.xmin);
	pcx.ymax = LittleShort (pcx.ymax);
	pcx.ymin = LittleShort (pcx.ymin);
	pcx.hres = LittleShort (pcx.hres);
	pcx.vres = LittleShort (pcx.vres);
	pcx.bytes_per_line = LittleShort (pcx.bytes_per_line);
	pcx.palette_type = LittleShort (pcx.palette_type);

	pcxwidth = pcx.xmax + 1 - pcx.xmin;
	pcxheight = pcx.ymax + 1 - pcx.ymin;
	if (pcx.manufacturer != 0x0a || pcx.version != 5 || pcx.encoding != 1 || pcx.bits_per_pixel != 8 || pcxwidth > 4096 || pcxheight > 4096 || pcxwidth <= 0 || pcxheight <= 0)
		return false;

	enddata = f + filesize - 768;

	for (y = 0;y < outheight && fin < enddata;y++)
	{
		a = pixels + y * outwidth;
		// pad the output with blank lines if needed
		if (y >= pcxheight)
		{
			memset(a, 0, outwidth);
			continue;
		}
		for (x = 0;x < pcxwidth;)
		{
			if (fin >= enddata)
				return false;
			dataByte = *fin++;
			if(dataByte >= 0xC0)
			{
				x2 = x + (dataByte & 0x3F);
				if (fin >= enddata)
					return false;
				if (x2 > pcxwidth)
					return false;
				dataByte = *fin++;
				for (;x < x2;x++)
					if (x < outwidth)
						a[x] = dataByte;
			}
			else
			{
				if (x < outwidth) // truncate to destination width
					a[x] = dataByte;
				x++;
			}
		}
		while(x < outwidth)
			a[x++] = 0;
	}

	return true;
}

/*
============
LoadPCX
============
*/
qbool LoadPCX_PaletteOnly(const unsigned char *f, int filesize, unsigned char *palette768b)
{
	if (filesize < 768)
		return false;
	memcpy(palette768b, f + filesize - 768, 768);
	return true;
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
}
TargaHeader;

static void PrintTargaHeader(TargaHeader *t)
{
	Con_Printf("TargaHeader:\nuint8 id_length = %i;\nuint8 colormap_type = %i;\nuint8 image_type = %i;\nuint16 colormap_index = %i;\nuint16 colormap_length = %i;\nuint8 colormap_size = %i;\nuint16 x_origin = %i;\nuint16 y_origin = %i;\nuint16 width = %i;\nuint16 height = %i;\nuint8 pixel_size = %i;\nuint8 attributes = %i;\n", t->id_length, t->colormap_type, t->image_type, t->colormap_index, t->colormap_length, t->colormap_size, t->x_origin, t->y_origin, t->width, t->height, t->pixel_size, t->attributes);
}

/*
=============
LoadTGA
=============
*/
unsigned char *LoadTGA_BGRA (const unsigned char *f, int filesize, int *miplevel)
{
	int x, y, pix_inc, row_inci, runlen, alphabits;
	unsigned char *image_buffer;
	unsigned int *pixbufi;
	const unsigned char *fin, *enddata;
	TargaHeader targa_header;
	unsigned int palettei[256];
	union
	{
		unsigned int i;
		unsigned char b[4];
	}
	bgra;

	if (filesize < 19)
		return NULL;

	enddata = f + filesize;

	targa_header.id_length = f[0];
	targa_header.colormap_type = f[1];
	targa_header.image_type = f[2];

	targa_header.colormap_index = f[3] + f[4] * 256;
	targa_header.colormap_length = f[5] + f[6] * 256;
	targa_header.colormap_size = f[7];
	targa_header.x_origin = f[8] + f[9] * 256;
	targa_header.y_origin = f[10] + f[11] * 256;
	targa_header.width = image_width = f[12] + f[13] * 256;
	targa_header.height = image_height = f[14] + f[15] * 256;
	targa_header.pixel_size = f[16];
	targa_header.attributes = f[17];

	if (image_width > 32768 || image_height > 32768 || image_width <= 0 || image_height <= 0)
	{
		Con_Print("LoadTGA: invalid size\n");
		PrintTargaHeader(&targa_header);
		return NULL;
	}

	memset(palettei, 0, sizeof(palettei));

	// advance to end of header
	fin = f + 18;

	// skip TARGA image comment (usually 0 bytes)
	fin += targa_header.id_length;

	// read/skip the colormap if present (note: according to the TARGA spec it
	// can be present even on truecolor or greyscale images, just not used by
	// the image data)
	if (targa_header.colormap_type)
	{
		if (targa_header.colormap_length > 256)
		{
			Con_Print("LoadTGA: only up to 256 colormap_length supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		if (targa_header.colormap_index)
		{
			Con_Print("LoadTGA: colormap_index not supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		if (targa_header.colormap_size == 24)
		{
			for (x = 0;x < targa_header.colormap_length;x++)
			{
				bgra.b[0] = *fin++;
				bgra.b[1] = *fin++;
				bgra.b[2] = *fin++;
				bgra.b[3] = 255;
				palettei[x] = bgra.i;
			}
		}
		else if (targa_header.colormap_size == 32)
		{
			memcpy(palettei, fin, targa_header.colormap_length*4);
			fin += targa_header.colormap_length * 4;
		}
		else
		{
			Con_Print("LoadTGA: Only 32 and 24 bit colormap_size supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
	}

	// check our pixel_size restrictions according to image_type
	switch (targa_header.image_type & ~8)
	{
	case 2:
		if (targa_header.pixel_size != 24 && targa_header.pixel_size != 32)
		{
			Con_Print("LoadTGA: only 24bit and 32bit pixel sizes supported for type 2 and type 10 images\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		break;
	case 3:
		// set up a palette to make the loader easier
		for (x = 0;x < 256;x++)
		{
			bgra.b[0] = bgra.b[1] = bgra.b[2] = x;
			bgra.b[3] = 255;
			palettei[x] = bgra.i;
		}
		// fall through to colormap case
	case 1:
		if (targa_header.pixel_size != 8)
		{
			Con_Print("LoadTGA: only 8bit pixel size for type 1, 3, 9, and 11 images supported\n");
			PrintTargaHeader(&targa_header);
			return NULL;
		}
		break;
	default:
		Con_Printf("LoadTGA: Only type 1, 2, 3, 9, 10, and 11 targa RGB images supported, image_type = %i\n", targa_header.image_type);
		PrintTargaHeader(&targa_header);
		return NULL;
	}

	if (targa_header.attributes & 0x10)
	{
		Con_Print("LoadTGA: origin must be in top left or bottom left, top right and bottom right are not supported\n");
		return NULL;
	}

	// number of attribute bits per pixel, we only support 0 or 8
	alphabits = targa_header.attributes & 0x0F;
	if (alphabits != 8 && alphabits != 0)
	{
		Con_Print("LoadTGA: only 0 or 8 attribute (alpha) bits supported\n");
		return NULL;
	}

	image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
	if (!image_buffer)
	{
		Con_Printf("LoadTGA: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}

	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbufi = (unsigned int*)image_buffer + (image_height - 1)*image_width;
		row_inci = -image_width*2;
	}
	else
	{
		pixbufi = (unsigned int*)image_buffer;
		row_inci = 0;
	}

	pix_inc = 1;
	if ((targa_header.image_type & ~8) == 2)
		pix_inc = (targa_header.pixel_size + 7) / 8;
	switch (targa_header.image_type)
	{
	case 1: // colormapped, uncompressed
	case 3: // greyscale, uncompressed
		if (fin + image_width * image_height * pix_inc > enddata)
			break;
		for (y = 0;y < image_height;y++, pixbufi += row_inci)
			for (x = 0;x < image_width;x++)
				*pixbufi++ = palettei[*fin++];
		break;
	case 2:
		// BGR or BGRA, uncompressed
		if (fin + image_width * image_height * pix_inc > enddata)
			break;
		if (targa_header.pixel_size == 32 && alphabits)
		{
			for (y = 0;y < image_height;y++)
				memcpy(pixbufi + y * (image_width + row_inci), fin + y * image_width * pix_inc, image_width*4);
		}
		else
		{
			for (y = 0;y < image_height;y++, pixbufi += row_inci)
			{
				for (x = 0;x < image_width;x++, fin += pix_inc)
				{
					bgra.b[0] = fin[0];
					bgra.b[1] = fin[1];
					bgra.b[2] = fin[2];
					bgra.b[3] = 255;
					*pixbufi++ = bgra.i;
				}
			}
		}
		break;
	case 9: // colormapped, RLE
	case 11: // greyscale, RLE
		for (y = 0;y < image_height;y++, pixbufi += row_inci)
		{
			for (x = 0;x < image_width;)
			{
				if (fin >= enddata)
					break; // error - truncated file
				runlen = *fin++;
				if (runlen & 0x80)
				{
					// RLE - all pixels the same color
					runlen += 1 - 0x80;
					if (fin + pix_inc > enddata)
						break; // error - truncated file
					if (x + runlen > image_width)
						break; // error - line exceeds width
					bgra.i = palettei[*fin++];
					for (;runlen--;x++)
						*pixbufi++ = bgra.i;
				}
				else
				{
					// uncompressed - all pixels different color
					runlen++;
					if (fin + pix_inc * runlen > enddata)
						break; // error - truncated file
					if (x + runlen > image_width)
						break; // error - line exceeds width
					for (;runlen--;x++)
						*pixbufi++ = palettei[*fin++];
				}
			}

			if (x != image_width)
			{
				// pixbufi is useless now
				Con_Printf("LoadTGA: corrupt file\n");
				break;
			}
		}
		break;
	case 10:
		// BGR or BGRA, RLE
		if (targa_header.pixel_size == 32 && alphabits)
		{
			for (y = 0;y < image_height;y++, pixbufi += row_inci)
			{
				for (x = 0;x < image_width;)
				{
					if (fin >= enddata)
						break; // error - truncated file
					runlen = *fin++;
					if (runlen & 0x80)
					{
						// RLE - all pixels the same color
						runlen += 1 - 0x80;
						if (fin + pix_inc > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						bgra.b[0] = fin[0];
						bgra.b[1] = fin[1];
						bgra.b[2] = fin[2];
						bgra.b[3] = fin[3];
						fin += pix_inc;
						for (;runlen--;x++)
							*pixbufi++ = bgra.i;
					}
					else
					{
						// uncompressed - all pixels different color
						runlen++;
						if (fin + pix_inc * runlen > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						for (;runlen--;x++)
						{
							bgra.b[0] = fin[0];
							bgra.b[1] = fin[1];
							bgra.b[2] = fin[2];
							bgra.b[3] = fin[3];
							fin += pix_inc;
							*pixbufi++ = bgra.i;
						}
					}
				}

				if (x != image_width)
				{
					// pixbufi is useless now
					Con_Printf("LoadTGA: corrupt file\n");
					break;
				}
			}
		}
		else
		{
			for (y = 0;y < image_height;y++, pixbufi += row_inci)
			{
				for (x = 0;x < image_width;)
				{
					if (fin >= enddata)
						break; // error - truncated file
					runlen = *fin++;
					if (runlen & 0x80)
					{
						// RLE - all pixels the same color
						runlen += 1 - 0x80;
						if (fin + pix_inc > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						bgra.b[0] = fin[0];
						bgra.b[1] = fin[1];
						bgra.b[2] = fin[2];
						bgra.b[3] = 255;
						fin += pix_inc;
						for (;runlen--;x++)
							*pixbufi++ = bgra.i;
					}
					else
					{
						// uncompressed - all pixels different color
						runlen++;
						if (fin + pix_inc * runlen > enddata)
							break; // error - truncated file
						if (x + runlen > image_width)
							break; // error - line exceeds width
						for (;runlen--;x++)
						{
							bgra.b[0] = fin[0];
							bgra.b[1] = fin[1];
							bgra.b[2] = fin[2];
							bgra.b[3] = 255;
							fin += pix_inc;
							*pixbufi++ = bgra.i;
						}
					}
				}

				if (x != image_width)
				{
					// pixbufi is useless now
					Con_Printf("LoadTGA: corrupt file\n");
					break;
				}
			}
		}
		break;
	default:
		// unknown image_type
		break;
	}

	return image_buffer;
}

typedef struct q2wal_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} q2wal_t;

static unsigned char *LoadWAL_BGRA (const unsigned char *f, int filesize, int *miplevel)
{
	unsigned char *image_buffer;
	const q2wal_t *inwal = (const q2wal_t *)f;

	if (filesize < (int) sizeof(q2wal_t))
	{
		Con_Print("LoadWAL: invalid WAL file\n");
		return NULL;
	}

	image_width = LittleLong(inwal->width);
	image_height = LittleLong(inwal->height);
	if (image_width > 32768 || image_height > 32768 || image_width <= 0 || image_height <= 0)
	{
		Con_Printf("LoadWAL: invalid size %ix%i\n", image_width, image_height);
		return NULL;
	}

	if (filesize < (int) LittleLong(inwal->offsets[0]) + image_width * image_height)
	{
		Con_Print("LoadWAL: invalid WAL file\n");
		return NULL;
	}

	image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
	if (!image_buffer)
	{
		Con_Printf("LoadWAL: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}
	Image_Copy8bitBGRA(f + LittleLong(inwal->offsets[0]), image_buffer, image_width * image_height, q2palette_bgra_complete);
	return image_buffer;
}

qbool LoadWAL_GetMetadata(const unsigned char *f, int filesize, int *retwidth, int *retheight, int *retflags, int *retvalue, int *retcontents, char *retanimname32c)
{
	const q2wal_t *inwal = (const q2wal_t *)f;

	if (filesize < (int) sizeof(q2wal_t))
	{
		Con_Print("LoadWAL: invalid WAL file\n");
		if (retwidth)
			*retwidth = 16;
		if (retheight)
			*retheight = 16;
		if (retflags)
			*retflags = 0;
		if (retvalue)
			*retvalue = 0;
		if (retcontents)
			*retcontents = 0;
		if (retanimname32c)
			memset(retanimname32c, 0, 32);
		return false;
	}

	if (retwidth)
		*retwidth = LittleLong(inwal->width);
	if (retheight)
		*retheight = LittleLong(inwal->height);
	if (retflags)
		*retflags = LittleLong(inwal->flags);
	if (retvalue)
		*retvalue = LittleLong(inwal->value);
	if (retcontents)
		*retcontents = LittleLong(inwal->contents);
	if (retanimname32c)
	{
		memcpy(retanimname32c, inwal->animname, 32);
		retanimname32c[31] = 0;
	}
	return true;
}

// gfx/* wad lumps and gfx/*.lmp files are simply width and height and paletted pixels, with color 255 as transparent
static unsigned char* LoadLMP_BGRA(const unsigned char *f, int filesize, int *miplevel)
{
	unsigned char *image_buffer;
	int i;

	if (filesize < 9)
	{
		Con_Print("Bad lmp file\n");
		return NULL;
	}

	image_width = f[0] + f[1] * 0x100 + f[2] * 0x10000 + f[3] * 0x1000000;
	image_height = f[4] + f[5] * 0x100 + f[6] * 0x10000 + f[7] * 0x1000000;

	if (image_width > 32768 || image_height > 32768 || image_width <= 0 || image_height <= 0 || image_width * image_height > filesize - 8)
	{
		Con_Print("Bad lmp file\n");
		return NULL;
	}

	image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width*image_height * 4);
	if (!image_buffer)
	{
		Con_Printf("LoadLMP: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}

	for (i = 0; i < image_width * image_height; i++)
	{
		const unsigned char *p = (const unsigned char *)palette_bgra_transparent + 4 * f[8 + i];
		image_buffer[i * 4 + 0] = p[0];
		image_buffer[i * 4 + 1] = p[1];
		image_buffer[i * 4 + 2] = p[2];
		image_buffer[i * 4 + 3] = p[3];
	}

	return image_buffer;
}

// gfx/conchars is a raw 128x128 image with 0 as transparent color rather than 255
static unsigned char *LoadConChars_BGRA(const unsigned char *f, int filesize, int *miplevel)
{
	unsigned char *image_buffer;
	int i;

	image_width = 128;
	image_height = 128;
	if (image_width * image_height > filesize)
	{
		Con_Print("Bad lmp file\n");
		return NULL;
	}

	image_buffer = (unsigned char *)Mem_Alloc(tempmempool, image_width*image_height * 4);
	if (!image_buffer)
	{
		Con_Printf("LoadConChars: not enough memory for %i by %i image\n", image_width, image_height);
		return NULL;
	}

	for (i = 0; i < image_width * image_height; i++)
	{
		const unsigned char *p = (const unsigned char *)palette_bgra_font + 4 * f[i];
		image_buffer[i * 4 + 0] = p[0];
		image_buffer[i * 4 + 1] = p[1];
		image_buffer[i * 4 + 2] = p[2];
		image_buffer[i * 4 + 3] = p[3];
	}

	return image_buffer;
}

void Image_StripImageExtension (const char *in, char *out, size_t size_out)
{
	const char *ext;

	if (size_out == 0)
		return;

	ext = FS_FileExtension(in);
	if (ext && (!strcmp(ext, "tga") || !strcmp(ext, "pcx") || !strcmp(ext, "lmp") || !strcmp(ext, "png") || !strcmp(ext, "jpg") || !strcmp(ext, "wal")))
		FS_StripExtension(in, out, size_out);
	else
		dp_strlcpy(out, in, size_out);
}

static unsigned char image_linearfromsrgb[256];
static unsigned char image_srgbfromlinear_lightmap[256];

void Image_MakeLinearColorsFromsRGB(unsigned char *pout, const unsigned char *pin, int numpixels)
{
	int i;
	// this math from http://www.opengl.org/registry/specs/EXT/texture_sRGB.txt
	if (!image_linearfromsrgb[255])
		for (i = 0;i < 256;i++)
			image_linearfromsrgb[i] = (unsigned char)floor(Image_LinearFloatFromsRGB(i) * 255.0f + 0.5f);
	for (i = 0;i < numpixels;i++)
	{
		pout[i*4+0] = image_linearfromsrgb[pin[i*4+0]];
		pout[i*4+1] = image_linearfromsrgb[pin[i*4+1]];
		pout[i*4+2] = image_linearfromsrgb[pin[i*4+2]];
		pout[i*4+3] = pin[i*4+3];
	}
}

void Image_MakesRGBColorsFromLinear_Lightmap(unsigned char *pout, const unsigned char *pin, int numpixels)
{
	int i;
	// this math from http://www.opengl.org/registry/specs/EXT/texture_sRGB.txt
	if (!image_srgbfromlinear_lightmap[255])
		for (i = 0;i < 256;i++)
			image_srgbfromlinear_lightmap[i] = (unsigned char)floor(bound(0.0f, Image_sRGBFloatFromLinear_Lightmap(i), 1.0f) * 255.0f + 0.5f);
	for (i = 0;i < numpixels;i++)
	{
		pout[i*4+0] = image_srgbfromlinear_lightmap[pin[i*4+0]];
		pout[i*4+1] = image_srgbfromlinear_lightmap[pin[i*4+1]];
		pout[i*4+2] = image_srgbfromlinear_lightmap[pin[i*4+2]];
		pout[i*4+3] = pin[i*4+3];
	}
}

typedef struct imageformat_s
{
	const char *formatstring;
	unsigned char *(*loadfunc)(const unsigned char *f, int filesize, int *miplevel);
}
imageformat_t;

// GAME_TENEBRAE only
imageformat_t imageformats_tenebrae[] =
{
	{"override/%s.tga", LoadTGA_BGRA},
	{"override/%s.png", PNG_LoadImage_BGRA},
	{"override/%s.jpg", JPEG_LoadImage_BGRA},
	{"override/%s.pcx", LoadPCX_BGRA},
	{"%s.tga", LoadTGA_BGRA},
	{"%s.png", PNG_LoadImage_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.pcx", LoadPCX_BGRA},
	{NULL, NULL}
};

imageformat_t imageformats_nopath[] =
{
	{"override/%s.tga", LoadTGA_BGRA},
	{"override/%s.png", PNG_LoadImage_BGRA},
	{"override/%s.jpg", JPEG_LoadImage_BGRA},
	{"textures/%s.tga", LoadTGA_BGRA},
	{"textures/%s.png", PNG_LoadImage_BGRA},
	{"textures/%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.tga", LoadTGA_BGRA},
	{"%s.png", PNG_LoadImage_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.pcx", LoadPCX_BGRA},
	{NULL, NULL}
};

// GAME_DELUXEQUAKE only
// VorteX: the point why i use such messy texture paths is
// that GtkRadiant can't detect normal/gloss textures
// and exclude them from texture browser
// so i just use additional folder to store this textures
imageformat_t imageformats_dq[] =
{
	{"%s.tga", LoadTGA_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"texturemaps/%s.tga", LoadTGA_BGRA},
	{"texturemaps/%s.jpg", JPEG_LoadImage_BGRA},
	{NULL, NULL}
};

imageformat_t imageformats_textures[] =
{
	{"%s.tga", LoadTGA_BGRA},
	{"%s.png", PNG_LoadImage_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.pcx", LoadPCX_BGRA},
	{"%s.wal", LoadWAL_BGRA},
	{NULL, NULL}
};

imageformat_t imageformats_gfx[] =
{
	{"%s.tga", LoadTGA_BGRA},
	{"%s.png", PNG_LoadImage_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.pcx", LoadPCX_BGRA},
	{"%s.lmp", LoadLMP_BGRA},
	{NULL, NULL}
};

imageformat_t imageformats_other[] =
{
	{"%s.tga", LoadTGA_BGRA},
	{"%s.png", PNG_LoadImage_BGRA},
	{"%s.jpg", JPEG_LoadImage_BGRA},
	{"%s.pcx", LoadPCX_BGRA},
	{"%s.lmp", LoadLMP_BGRA},
	{NULL, NULL}
};

int fixtransparentpixels(unsigned char *data, int w, int h);
unsigned char *loadimagepixelsbgra (const char *filename, qbool complain, qbool allowFixtrans, qbool convertsRGB, int *miplevel)
{
	fs_offset_t filesize;
	imageformat_t *firstformat, *format;
	int mymiplevel;
	unsigned char *f, *data = NULL, *data2 = NULL;
	char basename[MAX_QPATH], name[MAX_QPATH], name2[MAX_QPATH], path[MAX_QPATH], afterpath[MAX_QPATH], *c;
	char vabuf[1024];
	//if (developer_memorydebug.integer)
	//	Mem_CheckSentinelsGlobal();
	if (developer_texturelogging.integer)
		Log_Printf("textures.log", "%s\n", filename);
	Image_StripImageExtension(filename, basename, sizeof(basename)); // strip filename extensions to allow replacement by other types
	// replace *'s with #, so commandline utils don't get confused when dealing with the external files
	for (c = basename;*c;c++)
		if (*c == '*')
			*c = '#';
	path[0] = 0;
	name[0] = 0;
	dp_strlcpy(afterpath, basename, sizeof(afterpath));
	if (strchr(basename, '/'))
	{
		int i;
		for (i = 0;i < (int)sizeof(path)-1 && basename[i] != '/' && basename[i];i++)
			path[i] = basename[i];
		path[i] = 0;
		dp_strlcpy(afterpath, basename + i + 1, sizeof(afterpath));
	}
	if (gamemode == GAME_TENEBRAE)
		firstformat = imageformats_tenebrae;
	else if (gamemode == GAME_DELUXEQUAKE)
		firstformat = imageformats_dq;
	else if (!strcasecmp(path, "textures"))
		firstformat = imageformats_textures;
	else if (!strcasecmp(path, "gfx") || !strcasecmp(path, "locale")) // locale/ is used in GAME_BLOODOMNICIDE
		firstformat = imageformats_gfx;
	else if (!path[0])
		firstformat = imageformats_nopath;
	else
		firstformat = imageformats_other;
	// now try all the formats in the selected list
	for (format = firstformat;format->formatstring;format++)
	{
		dpsnprintf (name, sizeof(name), format->formatstring, basename);

		FS_SanitizePath(name);

		if(FS_FileExists(name) && (f = FS_LoadFile(name, tempmempool, true, &filesize)) != NULL)
		{
			mymiplevel = miplevel ? *miplevel : 0;
			image_width = 0;
			image_height = 0;
			data = format->loadfunc(f, (int)filesize, &mymiplevel);
			Mem_Free(f);
			if (data)
			{
				if(format->loadfunc == JPEG_LoadImage_BGRA) // jpeg can't do alpha, so let's simulate it by loading another jpeg
				{
					dpsnprintf (name2, sizeof(name2), format->formatstring, va(vabuf, sizeof(vabuf), "%s_alpha", basename));
					f = FS_LoadFile(name2, tempmempool, true, &filesize);
					if(f)
					{
						int mymiplevel2 = miplevel ? *miplevel : 0;
						int image_width_save = image_width;
						int image_height_save = image_height;
						data2 = format->loadfunc(f, (int)filesize, &mymiplevel2);
						if(data2 && mymiplevel == mymiplevel2 && image_width == image_width_save && image_height == image_height_save)
							Image_CopyAlphaFromBlueBGRA(data, data2, image_width, image_height);
						else
							Con_Printf("loadimagepixelsrgba: corrupt or invalid alpha image %s_alpha\n", basename);
						image_width = image_width_save;
						image_height = image_height_save;
						if(data2)
							Mem_Free(data2);
						Mem_Free(f);
					}
				}
				if (developer_loading.integer)
					Con_DPrintf("loaded image %s (%dx%d)\n", name, image_width, image_height);
				if(miplevel)
					*miplevel = mymiplevel;
				//if (developer_memorydebug.integer)
				//	Mem_CheckSentinelsGlobal();
				if(allowFixtrans && r_fixtrans_auto.integer)
				{
					int n = fixtransparentpixels(data, image_width, image_height);
					if(n)
					{
						Con_Printf("- had to fix %s (%d pixels changed)\n", name, n);
						if(r_fixtrans_auto.integer >= 2)
						{
							char outfilename[MAX_QPATH], buf[MAX_QPATH];
							Image_StripImageExtension(name, buf, sizeof(buf));
							dpsnprintf(outfilename, sizeof(outfilename), "fixtrans/%s.tga", buf);
							Image_WriteTGABGRA(outfilename, image_width, image_height, data);
							Con_Printf("- %s written.\n", outfilename);
						}
					}
				}
				if (convertsRGB)
					Image_MakeLinearColorsFromsRGB(data, data, image_width * image_height);
				return data;
			}
			else
				Con_DPrintf("Error loading image %s (file loaded but decode failed)\n", name);
		}
	}
	if (!strcasecmp(path, "gfx"))
	{
		unsigned char *lmpdata;
		if ((lmpdata = W_GetLumpName(afterpath, &filesize)))
		{
			if (developer_loading.integer)
				Con_Printf("loading gfx.wad lump \"%s\"\n", afterpath);

			mymiplevel = miplevel ? *miplevel : 0;
			if (!strcmp(afterpath, "conchars"))
			{
				// conchars is a raw image and with color 0 as transparent instead of 255
				data = LoadConChars_BGRA(lmpdata, filesize, &mymiplevel);
			}
			else
				data = LoadLMP_BGRA(lmpdata, filesize, &mymiplevel);
			// no cleanup after looking up a wad lump - the whole gfx.wad is loaded at once
			if (data)
				return data;
			Con_DPrintf("Error loading image %s (file loaded but decode failed)\n", name);
		}
	}

	// check if the image name exists as an embedded pic
	if ((data = Image_GetEmbeddedPicBGRA(basename)))
		return data;

	if (complain)
	{
		Con_Printf(CON_ERROR "Couldn't load %s using ", filename);
		for (format = firstformat;format->formatstring;format++)
		{
			dpsnprintf (name, sizeof(name), format->formatstring, basename);
			Con_Printf(format == firstformat ? "\"%s\"" : (format[1].formatstring ? ", \"%s\"" : " or \"%s\".\n"), format->formatstring);
		}
	}

	// texture loading can take a while, so make sure we're sending keepalives
	CL_KeepaliveMessage(false);

	//if (developer_memorydebug.integer)
	//	Mem_CheckSentinelsGlobal();

	return NULL;
}

qbool Image_GetStockPicSize(const char *filename, int *returnwidth, int *returnheight)
{
	unsigned char *data;
	fs_offset_t filesize;
	char lmppath[MAX_QPATH];
	if (!strcasecmp(filename, "gfx/conchars"))
	{
		*returnwidth = 128;
		*returnheight = 128;
		return true;
	}

	dpsnprintf(lmppath, sizeof(lmppath), "%s.lmp", filename);
	data = FS_LoadFile(lmppath, tempmempool, true, &filesize);
	if (data)
	{
		if (filesize > 8)
		{
			int w = data[0] + data[1] * 0x100 + data[2] * 0x10000 + data[3] * 0x1000000;
			int h = data[4] + data[5] * 0x100 + data[6] * 0x10000 + data[7] * 0x1000000;
			if (w >= 1 && w <= 32768 && h >= 1 && h <= 32768)
			{
				*returnwidth = w;
				*returnheight = h;
				Mem_Free(data);
				return true;
			}
		}
		Mem_Free(data);
	}
	if (!strncasecmp(filename, "gfx/", 4))
	{
		data = W_GetLumpName(filename + 4, &filesize);
		if (data && filesize > 8)
		{
			int w = data[0] + data[1] * 0x100 + data[2] * 0x10000 + data[3] * 0x1000000;
			int h = data[4] + data[5] * 0x100 + data[6] * 0x10000 + data[7] * 0x1000000;
			if (w >= 1 && w <= 32768 && h >= 1 && h <= 32768)
			{
				*returnwidth = w;
				*returnheight = h;
				return true;
			}
		}
	}
	return false;
}

extern cvar_t gl_picmip;
rtexture_t *loadtextureimage (rtexturepool_t *pool, const char *filename, qbool complain, int flags, qbool allowFixtrans, qbool sRGB)
{
	unsigned char *data;
	rtexture_t *rt;
	int miplevel = R_PicmipForFlags(flags);
	if (!(data = loadimagepixelsbgra (filename, complain, allowFixtrans, false, &miplevel)))
		return 0;
	rt = R_LoadTexture2D(pool, filename, image_width, image_height, data, sRGB ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, flags, miplevel, NULL);
	Mem_Free(data);
	return rt;
}

int fixtransparentpixels(unsigned char *data, int w, int h)
{
	int const FIXTRANS_NEEDED = 1;
	int const FIXTRANS_HAS_L = 2;
	int const FIXTRANS_HAS_R = 4;
	int const FIXTRANS_HAS_U = 8;
	int const FIXTRANS_HAS_D = 16;
	int const FIXTRANS_FIXED = 32;
	unsigned char *fixMask = (unsigned char *) Mem_Alloc(tempmempool, w * h);
	int fixPixels = 0;
	int changedPixels = 0;
	int x, y;

#define FIXTRANS_PIXEL (y*w+x)
#define FIXTRANS_PIXEL_U (((y+h-1)%h)*w+x)
#define FIXTRANS_PIXEL_D (((y+1)%h)*w+x)
#define FIXTRANS_PIXEL_L (y*w+((x+w-1)%w))
#define FIXTRANS_PIXEL_R (y*w+((x+1)%w))

	memset(fixMask, 0, w * h);
	for(y = 0; y < h; ++y)
		for(x = 0; x < w; ++x)
		{
			if(data[FIXTRANS_PIXEL * 4 + 3] == 0)
			{
				fixMask[FIXTRANS_PIXEL] |= FIXTRANS_NEEDED;
				++fixPixels;
			}
			else
			{
				fixMask[FIXTRANS_PIXEL_D] |= FIXTRANS_HAS_U;
				fixMask[FIXTRANS_PIXEL_U] |= FIXTRANS_HAS_D;
				fixMask[FIXTRANS_PIXEL_R] |= FIXTRANS_HAS_L;
				fixMask[FIXTRANS_PIXEL_L] |= FIXTRANS_HAS_R;
			}
		}
	if(fixPixels == w * h)
		return 0; // sorry, can't do anything about this
	while(fixPixels)
	{
		for(y = 0; y < h; ++y)
			for(x = 0; x < w; ++x)
				if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_NEEDED)
				{
					unsigned int sumR = 0, sumG = 0, sumB = 0, sumA = 0, sumRA = 0, sumGA = 0, sumBA = 0, cnt = 0;
					unsigned char r, g, b, a, r0, g0, b0;
					if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_HAS_U)
					{
						r = data[FIXTRANS_PIXEL_U * 4 + 2];
						g = data[FIXTRANS_PIXEL_U * 4 + 1];
						b = data[FIXTRANS_PIXEL_U * 4 + 0];
						a = data[FIXTRANS_PIXEL_U * 4 + 3];
						sumR += r; sumG += g; sumB += b; sumA += a; sumRA += r*a; sumGA += g*a; sumBA += b*a; ++cnt;
					}
					if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_HAS_D)
					{
						r = data[FIXTRANS_PIXEL_D * 4 + 2];
						g = data[FIXTRANS_PIXEL_D * 4 + 1];
						b = data[FIXTRANS_PIXEL_D * 4 + 0];
						a = data[FIXTRANS_PIXEL_D * 4 + 3];
						sumR += r; sumG += g; sumB += b; sumA += a; sumRA += r*a; sumGA += g*a; sumBA += b*a; ++cnt;
					}
					if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_HAS_L)
					{
						r = data[FIXTRANS_PIXEL_L * 4 + 2];
						g = data[FIXTRANS_PIXEL_L * 4 + 1];
						b = data[FIXTRANS_PIXEL_L * 4 + 0];
						a = data[FIXTRANS_PIXEL_L * 4 + 3];
						sumR += r; sumG += g; sumB += b; sumA += a; sumRA += r*a; sumGA += g*a; sumBA += b*a; ++cnt;
					}
					if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_HAS_R)
					{
						r = data[FIXTRANS_PIXEL_R * 4 + 2];
						g = data[FIXTRANS_PIXEL_R * 4 + 1];
						b = data[FIXTRANS_PIXEL_R * 4 + 0];
						a = data[FIXTRANS_PIXEL_R * 4 + 3];
						sumR += r; sumG += g; sumB += b; sumA += a; sumRA += r*a; sumGA += g*a; sumBA += b*a; ++cnt;
					}
					if(!cnt)
						continue;
					r0 = data[FIXTRANS_PIXEL * 4 + 2];
					g0 = data[FIXTRANS_PIXEL * 4 + 1];
					b0 = data[FIXTRANS_PIXEL * 4 + 0];
					if(sumA)
					{
						// there is a surrounding non-alpha pixel
						r = (sumRA + sumA / 2) / sumA;
						g = (sumGA + sumA / 2) / sumA;
						b = (sumBA + sumA / 2) / sumA;
					}
					else
					{
						// need to use a "regular" average
						r = (sumR + cnt / 2) / cnt;
						g = (sumG + cnt / 2) / cnt;
						b = (sumB + cnt / 2) / cnt;
					}
					if(r != r0 || g != g0 || b != b0)
						++changedPixels;
					data[FIXTRANS_PIXEL * 4 + 2] = r;
					data[FIXTRANS_PIXEL * 4 + 1] = g;
					data[FIXTRANS_PIXEL * 4 + 0] = b;
					fixMask[FIXTRANS_PIXEL] |= FIXTRANS_FIXED;
				}
		for(y = 0; y < h; ++y)
			for(x = 0; x < w; ++x)
				if(fixMask[FIXTRANS_PIXEL] & FIXTRANS_FIXED)
				{
					fixMask[FIXTRANS_PIXEL] &= ~(FIXTRANS_NEEDED | FIXTRANS_FIXED);
					fixMask[FIXTRANS_PIXEL_D] |= FIXTRANS_HAS_U;
					fixMask[FIXTRANS_PIXEL_U] |= FIXTRANS_HAS_D;
					fixMask[FIXTRANS_PIXEL_R] |= FIXTRANS_HAS_L;
					fixMask[FIXTRANS_PIXEL_L] |= FIXTRANS_HAS_R;
					--fixPixels;
				}
	}
	return changedPixels;
}

void Image_FixTransparentPixels_f(cmd_state_t *cmd)
{
	const char *filename, *filename_pattern;
	fssearch_t *search;
	int i, n;
	char outfilename[MAX_QPATH], buf[MAX_QPATH];
	unsigned char *data;
	if(Cmd_Argc(cmd) != 2)
	{
		Con_Printf("Usage: %s imagefile\n", Cmd_Argv(cmd, 0));
		return;
	}
	filename_pattern = Cmd_Argv(cmd, 1);
	search = FS_Search(filename_pattern, true, true, NULL);
	if(!search)
		return;
	for(i = 0; i < search->numfilenames; ++i)
	{
		filename = search->filenames[i];
		Con_Printf("Processing %s... ", filename);
		Image_StripImageExtension(filename, buf, sizeof(buf));
		dpsnprintf(outfilename, sizeof(outfilename), "fixtrans/%s.tga", buf);
		if(!(data = loadimagepixelsbgra(filename, true, false, false, NULL)))
			return;
		if((n = fixtransparentpixels(data, image_width, image_height)))
		{
			Image_WriteTGABGRA(outfilename, image_width, image_height, data);
			Con_Printf("%s written (%d pixels changed).\n", outfilename, n);
		}
		else
			Con_Printf("unchanged.\n");
		Mem_Free(data);
	}
	FS_FreeSearch(search);
}

qbool Image_WriteTGABGR_preflipped (const char *filename, int width, int height, const unsigned char *data)
{
	qbool ret;
	unsigned char buffer[18];
	const void *buffers[2];
	fs_offset_t sizes[2];

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;
	buffer[16] = 24;	// pixel size

	buffers[0] = buffer;
	sizes[0] = 18;
	buffers[1] = data;
	sizes[1] = width*height*3;
	ret = FS_WriteFileInBlocks(filename, buffers, sizes, 2);

	return ret;
}

qbool Image_WriteTGABGRA (const char *filename, int width, int height, const unsigned char *data)
{
	int y;
	unsigned char *buffer, *out;
	const unsigned char *in, *end;
	qbool ret;

	buffer = (unsigned char *)Mem_Alloc(tempmempool, width*height*4 + 18);

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = (width >> 0) & 0xFF;
	buffer[13] = (width >> 8) & 0xFF;
	buffer[14] = (height >> 0) & 0xFF;
	buffer[15] = (height >> 8) & 0xFF;

	for (y = 3;y < width*height*4;y += 4)
		if (data[y] < 255)
			break;

	if (y < width*height*4)
	{
		// save the alpha channel
		buffer[16] = 32;	// pixel size
		buffer[17] = 8; // 8 bits of alpha

		// flip upside down
		out = buffer + 18;
		for (y = height - 1;y >= 0;y--)
		{
			memcpy(out, data + y * width * 4, width * 4);
			out += width*4;
		}
	}
	else
	{
		// save only the color channels
		buffer[16] = 24;	// pixel size
		buffer[17] = 0; // 8 bits of alpha

		// truncate bgra to bgr and flip upside down
		out = buffer + 18;
		for (y = height - 1;y >= 0;y--)
		{
			in = data + y * width * 4;
			end = in + width * 4;
			for (;in < end;in += 4)
			{
				*out++ = in[0];
				*out++ = in[1];
				*out++ = in[2];
			}
		}
	}
	ret = FS_WriteFile (filename, buffer, out - buffer);

	Mem_Free(buffer);

	return ret;
}

static void Image_Resample32LerpLine (const unsigned char *in, unsigned char *out, int inwidth, int outwidth)
{
	int		j, xi, oldx = 0, f, fstep, endx, lerp;
	fstep = (int) (inwidth*65536.0f/outwidth);
	endx = (inwidth-1);
	for (j = 0,f = 0;j < outwidth;j++, f += fstep)
	{
		xi = f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			lerp = f & 0xFFFF;
			*out++ = (unsigned char) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (unsigned char) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (unsigned char) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (unsigned char) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
		}
		else // last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}

#define LERPBYTE(i) r = resamplerow1[i];out[i] = (unsigned char) ((((resamplerow2[i] - r) * lerp) >> 16) + r)
static void Image_Resample32Lerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, r, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth4 = inwidth*4, outwidth4 = outwidth*4;
	unsigned char *out;
	const unsigned char *inrow;
	unsigned char *resamplerow1;
	unsigned char *resamplerow2;
	out = (unsigned char *)outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	resamplerow1 = (unsigned char *)Mem_Alloc(tempmempool, outwidth*4*2);
	resamplerow2 = resamplerow1 + outwidth*4;

	inrow = (const unsigned char *)indata;
	oldy = 0;
	Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
	Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi < endy)
		{
			lerp = f & 0xFFFF;
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				Image_Resample32LerpLine (inrow + inwidth4, resamplerow2, inwidth, outwidth);
				oldy = yi;
			}
			j = outwidth - 4;
			while(j >= 0)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				LERPBYTE( 8);
				LERPBYTE( 9);
				LERPBYTE(10);
				LERPBYTE(11);
				LERPBYTE(12);
				LERPBYTE(13);
				LERPBYTE(14);
				LERPBYTE(15);
				out += 16;
				resamplerow1 += 16;
				resamplerow2 += 16;
				j -= 4;
			}
			if (j & 2)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				LERPBYTE( 4);
				LERPBYTE( 5);
				LERPBYTE( 6);
				LERPBYTE( 7);
				out += 8;
				resamplerow1 += 8;
				resamplerow2 += 8;
			}
			if (j & 1)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				out += 4;
				resamplerow1 += 4;
				resamplerow2 += 4;
			}
			resamplerow1 -= outwidth4;
			resamplerow2 -= outwidth4;
		}
		else
		{
			if (yi != oldy)
			{
				inrow = (unsigned char *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(resamplerow1, resamplerow2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, resamplerow1, inwidth, outwidth);
				oldy = yi;
			}
			memcpy(out, resamplerow1, outwidth4);
		}
	}

	Mem_Free(resamplerow1);
	resamplerow1 = NULL;
	resamplerow2 = NULL;
}

static void Image_Resample32Nolerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j;
	unsigned frac, fracstep;
	// relies on int being 4 bytes
	int *inrow, *out;
	out = (int *)outdata;

	fracstep = inwidth*0x10000/outwidth;
	for (i = 0;i < outheight;i++)
	{
		inrow = (int *)indata + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		j = outwidth - 4;
		while (j >= 0)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out[1] = inrow[frac >> 16];frac += fracstep;
			out[2] = inrow[frac >> 16];frac += fracstep;
			out[3] = inrow[frac >> 16];frac += fracstep;
			out += 4;
			j -= 4;
		}
		if (j & 2)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out[1] = inrow[frac >> 16];frac += fracstep;
			out += 2;
		}
		if (j & 1)
		{
			out[0] = inrow[frac >> 16];frac += fracstep;
			out += 1;
		}
	}
}

/*
================
Image_Resample
================
*/
void Image_Resample32(const void *indata, int inwidth, int inheight, int indepth, void *outdata, int outwidth, int outheight, int outdepth, int quality)
{
	if (indepth != 1 || outdepth != 1)
	{
		Con_Printf ("Image_Resample: 3D resampling not supported\n");
		return;
	}
	if (quality)
		Image_Resample32Lerp(indata, inwidth, inheight, outdata, outwidth, outheight);
	else
		Image_Resample32Nolerp(indata, inwidth, inheight, outdata, outwidth, outheight);
}

// in can be the same as out
void Image_MipReduce32(const unsigned char *in, unsigned char *out, int *width, int *height, int *depth, int destwidth, int destheight, int destdepth)
{
	const unsigned char *inrow;
	int x, y, nextrow;
	if (*depth != 1 || destdepth != 1)
	{
		Con_Printf ("Image_Resample: 3D resampling not supported\n");
		if (*width > destwidth)
			*width >>= 1;
		if (*height > destheight)
			*height >>= 1;
		if (*depth > destdepth)
			*depth >>= 1;
		return;
	}
	// note: if given odd width/height this discards the last row/column of
	// pixels, rather than doing a proper box-filter scale down
	inrow = in;
	nextrow = *width * 4;
	if (*width > destwidth)
	{
		*width >>= 1;
		if (*height > destheight)
		{
			// reduce both
			*height >>= 1;
			for (y = 0;y < *height;y++, inrow += nextrow * 2)
			{
				for (in = inrow, x = 0;x < *width;x++)
				{
					out[0] = (unsigned char) ((in[0] + in[4] + in[nextrow  ] + in[nextrow+4]) >> 2);
					out[1] = (unsigned char) ((in[1] + in[5] + in[nextrow+1] + in[nextrow+5]) >> 2);
					out[2] = (unsigned char) ((in[2] + in[6] + in[nextrow+2] + in[nextrow+6]) >> 2);
					out[3] = (unsigned char) ((in[3] + in[7] + in[nextrow+3] + in[nextrow+7]) >> 2);
					out += 4;
					in += 8;
				}
			}
		}
		else
		{
			// reduce width
			for (y = 0;y < *height;y++, inrow += nextrow)
			{
				for (in = inrow, x = 0;x < *width;x++)
				{
					out[0] = (unsigned char) ((in[0] + in[4]) >> 1);
					out[1] = (unsigned char) ((in[1] + in[5]) >> 1);
					out[2] = (unsigned char) ((in[2] + in[6]) >> 1);
					out[3] = (unsigned char) ((in[3] + in[7]) >> 1);
					out += 4;
					in += 8;
				}
			}
		}
	}
	else
	{
		if (*height > destheight)
		{
			// reduce height
			*height >>= 1;
			for (y = 0;y < *height;y++, inrow += nextrow * 2)
			{
				for (in = inrow, x = 0;x < *width;x++)
				{
					out[0] = (unsigned char) ((in[0] + in[nextrow  ]) >> 1);
					out[1] = (unsigned char) ((in[1] + in[nextrow+1]) >> 1);
					out[2] = (unsigned char) ((in[2] + in[nextrow+2]) >> 1);
					out[3] = (unsigned char) ((in[3] + in[nextrow+3]) >> 1);
					out += 4;
					in += 4;
				}
			}
		}
		else
			Con_Printf ("Image_MipReduce: desired size already achieved\n");
	}
}

void Image_HeightmapToNormalmap_BGRA(const unsigned char *inpixels, unsigned char *outpixels, int width, int height, int clamp, float bumpscale)
{
	int x, y, x1, x2, y1, y2;
	const unsigned char *b, *row[3];
	int p[5];
	unsigned char *out;
	float ibumpscale, n[3];
	ibumpscale = (255.0f * 6.0f) / bumpscale;
	out = outpixels;
	for (y = 0, y1 = height-1;y < height;y1 = y, y++)
	{
		y2 = y + 1;if (y2 >= height) y2 = 0;
		row[0] = inpixels + (y1 * width) * 4;
		row[1] = inpixels + (y  * width) * 4;
		row[2] = inpixels + (y2 * width) * 4;
		for (x = 0, x1 = width-1;x < width;x1 = x, x++)
		{
			x2 = x + 1;if (x2 >= width) x2 = 0;
			// left, right
			b = row[1] + x1 * 4;p[0] = (b[0] + b[1] + b[2]);
			b = row[1] + x2 * 4;p[1] = (b[0] + b[1] + b[2]);
			// above, below
			b = row[0] + x  * 4;p[2] = (b[0] + b[1] + b[2]);
			b = row[2] + x  * 4;p[3] = (b[0] + b[1] + b[2]);
			// center
			b = row[1] + x  * 4;p[4] = (b[0] + b[1] + b[2]);
			// calculate a normal from the slopes
			n[0] = p[0] - p[1];
			n[1] = p[3] - p[2];
			n[2] = ibumpscale;
			VectorNormalize(n);
			// turn it into a dot3 rgb vector texture
			out[2] = (int)(128.0f + n[0] * 127.0f);
			out[1] = (int)(128.0f + n[1] * 127.0f);
			out[0] = (int)(128.0f + n[2] * 127.0f);
			out[3] = (p[4]) / 3;
			out += 4;
		}
	}
}


#include "lhfont.h"

static unsigned char *Image_GenerateConChars(void)
{
	int i;
	unsigned char *data;
	double random;

	image_width = 256;
	image_height = 256;

	data = LoadTGA_BGRA(concharimage, sizeof(concharimage), NULL);
	// Gold numbers
	for (i = 0; i < 8192; i++)
	{
		random = lhrandom(0.0, 1.0);
		data[i * 4 + 3] = data[i * 4 + 0];
		data[i * 4 + 2] = 83 + (unsigned char)(random * 64);
		data[i * 4 + 1] = 71 + (unsigned char)(random * 32);
		data[i * 4 + 0] = 23 + (unsigned char)(random * 16);
	}
	// White chars
	for (i = 8192; i < 32768; i++)
	{
		random = lhrandom(0.0, 1.0);
		data[i * 4 + 3] = data[i * 4 + 0];
		data[i * 4 + 2] = 95 + (unsigned char)(random * 64);
		data[i * 4 + 1] = 95 + (unsigned char)(random * 64);
		data[i * 4 + 0] = 95 + (unsigned char)(random * 64);
	}
	// Gold numbers
	for (i = 32768; i < 40960; i++)
	{
		random = lhrandom(0.0, 1.0);
		data[i * 4 + 3] = data[i * 4 + 0];
		data[i * 4 + 2] = 83 + (unsigned char)(random * 64);
		data[i * 4 + 1] = 71 + (unsigned char)(random * 32);
		data[i * 4 + 0] = 23 + (unsigned char)(random * 16);
	}
	// Red chars
	for (i = 40960; i < 65536; i++)
	{
		random = lhrandom(0.0, 1.0);
		data[i * 4 + 3] = data[i * 4 + 0];
		data[i * 4 + 2] = 96 + (unsigned char)(random * 64);
		data[i * 4 + 1] = 43 + (unsigned char)(random * 32);
		data[i * 4 + 0] = 27 + (unsigned char)(random * 32);
	}

#if 0
	Image_WriteTGABGRA("gfx/generated_conchars.tga", 256, 256, data);
#endif

	return data;
}

static unsigned char *Image_GenerateDitherPattern(void)
{
	int x, y;
	unsigned char *data = (unsigned char *)Mem_Alloc(tempmempool, 8 * 8 * 4);
	image_width = 8;
	image_height = 8;
	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			data[(y * 8 + x) * 4 + 0] = ((x^y) & 4) ? 255 : 0;
			data[(y * 8 + x) * 4 + 1] = ((x^y) & 4) ? 255 : 0;
			data[(y * 8 + x) * 4 + 2] = ((x^y) & 4) ? 255 : 0;
			data[(y * 8 + x) * 4 + 3] = 255;
		}
	}
	return data;
}

// also used in R_SkinFrame code
unsigned char *Image_GenerateNoTexture(void)
{
	int x, y;
	unsigned char *data = (unsigned char *)Mem_Alloc(tempmempool, 16 * 16 * 4);
	image_width = 16;
	image_height = 16;
	// this makes a light grey/dark grey checkerboard texture
	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			data[(y * 16 + x) * 4 + 0] =
			data[(y * 16 + x) * 4 + 1] =
			data[(y * 16 + x) * 4 + 2] = (y < 8) ^ (x < 8) ? 128 : 64;
			data[(y * 16 + x) * 4 + 3] = 255;
		}
	}
	return data;
}

static unsigned char *Image_GenerateWhite(void)
{
	unsigned char *data = (unsigned char *)Mem_Alloc(tempmempool, 1 * 1 * 4);
	image_width = 1;
	image_height = 1;
	data[0] = data[1] = data[2] = data[3] = 255;
	return data;
}

typedef struct embeddedpic_s
{
const char *name;
int width;
int height;
const char *pixels;
}
embeddedpic_t;

static const embeddedpic_t embeddedpics[] =
{
	{
		"gfx/prydoncursor001", 16, 16,
		"477777774......."
		"77.....6........"
		"7.....6........."
		"7....6.........."
		"7.....6........."
		"7..6...6........"
		"7.6.6...6......."
		"76...6...6......"
		"4.....6.6......."
		".......6........"
		"................"
		"................"
		"................"
		"................"
		"................"
		"................"
	},
	{
		"ui/mousepointer", 16, 16,
		"477777774......."
		"77.....6........"
		"7.....6........."
		"7....6.........."
		"7.....6........."
		"7..6...6........"
		"7.6.6...6......."
		"76...6...6......"
		"4.....6.6......."
		".......6........"
		"................"
		"................"
		"................"
		"................"
		"................"
		"................"
	},
	{
		"gfx/crosshair1", 16, 16,
		"................"
		"................"
		"................"
		"...33......33..."
		"...355....553..."
		"....577..775...."
		".....77..77....."
		"................"
		"................"
		".....77..77....."
		"....577..775...."
		"...355....553..."
		"...33......33..."
		"................"
		"................"
		"................"
	},
	{
		"gfx/crosshair2", 16, 16,
		"................"
		"................"
		"................"
		"...3........3..."
		"....5......5...."
		".....7....7....."
		"......7..7......"
		"................"
		"................"
		"......7..7......"
		".....7....7....."
		"....5......5...."
		"...3........3..."
		"................"
		"................"
		"................"
	},
	{
		"gfx/crosshair3", 16, 16,
		"................"
		".......77......."
		".......77......."
		"................"
		"................"
		".......44......."
		".......44......."
		".77..44..44..77."
		".77..44..44..77."
		".......44......."
		".......44......."
		"................"
		"................"
		".......77......."
		".......77......."
		"................"
	},
	{
		"gfx/crosshair4", 16, 16,
		"................"
		"................"
		"................"
		"................"
		"................"
		"................"
		"................"
		"................"
		"........7777777."
		"........752....."
		"........72......"
		"........7......."
		"........7......."
		"........7......."
		"........7......."
		"................"
	},
	{
		"gfx/crosshair5", 8, 8,
		"........"
		"........"
		"....7..."
		"........"
		"..7.7.7."
		"........"
		"....7..."
		"........"
	},
	{
		"gfx/crosshair6", 2, 2,
		"77"
		"77"
	},
	{
		"gfx/crosshair7", 16, 16,
		"................"
		".3............3."
		"..5...2332...5.."
		"...7.3....3.7..."
		"....7......7...."
		"...3.7....7.3..."
		"..2...7..7...2.."
		"..3..........3.."
		"..3..........3.."
		"..2...7..7...2.."
		"...3.7....7.3..."
		"....7......7...."
		"...7.3....3.7..."
		"..5...2332...5.."
		".3............3."
		"................"
	},
	{ NULL, 0, 0, NULL }
};

unsigned char *Image_GetEmbeddedPicBGRA(const char *name)
{
	const embeddedpic_t *p;
	for (p = embeddedpics; p->name; p++)
	{
		if (!strcmp(name, p->name))
		{
			int i;
			unsigned char *data = (unsigned char *)Mem_Alloc(tempmempool, p->width * p->height * 4);
			image_width = p->width;
			image_height = p->height;
			for (i = 0; i < p->width * p->height; i++)
			{
				const unsigned char *c = (const unsigned char *)palette_bgra_embeddedpic + 4 * p->pixels[i];
				Vector4Copy(c, data + 4 * i);
			}
			return data;
		}
	}
	if (!strcmp(name, "white") || !strcmp(name, "#white") || !strcmp(name, "*white") || !strcmp(name, "$whiteimage"))
		return Image_GenerateWhite();
	if (!strcmp(name, "gfx/conchars"))
		return Image_GenerateConChars();
	if (!strcmp(name, "gfx/colorcontrol/ditherpattern"))
		return Image_GenerateDitherPattern();
	return NULL;
}
