/*
Copyright (C) 2002-2013 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "dpvsimpledecode.h"

#define HZREADERROR_OK 0
#define HZREADERROR_EOF 1
#define HZREADERROR_MALLOCFAILED 2

//#define HZREADBLOCKSIZE 16000
#define HZREADBLOCKSIZE 1048576

typedef struct hz_bitstream_read_s
{
	qfile_t *file;
	int endoffile;
}
hz_bitstream_read_t;

typedef struct hz_bitstream_readblock_s
{
	struct hz_bitstream_readblock_s *next;
	unsigned int size;
	unsigned char data[HZREADBLOCKSIZE];
}
hz_bitstream_readblock_t;

typedef struct hz_bitstream_readblocks_s
{
	hz_bitstream_readblock_t *blocks;
	hz_bitstream_readblock_t *current;
	unsigned int position;
	unsigned int store;
	int count;
}
hz_bitstream_readblocks_t;

static hz_bitstream_read_t *hz_bitstream_read_open(char *filename)
{
	qfile_t *file;
	hz_bitstream_read_t *stream;
	if ((file = FS_OpenVirtualFile(filename, false)))
	{
		stream = (hz_bitstream_read_t *)Z_Malloc(sizeof(hz_bitstream_read_t));
		memset(stream, 0, sizeof(*stream));
		stream->file = file;
		return stream;
	}
	else
		return NULL;
}

static void hz_bitstream_read_close(hz_bitstream_read_t *stream)
{
	if (stream)
	{
		FS_Close(stream->file);
		Z_Free(stream);
	}
}

static hz_bitstream_readblocks_t *hz_bitstream_read_blocks_new(void)
{
	hz_bitstream_readblocks_t *blocks;
	blocks = (hz_bitstream_readblocks_t *)Z_Malloc(sizeof(hz_bitstream_readblocks_t));
	if (blocks == NULL)
		return NULL;
	memset(blocks, 0, sizeof(hz_bitstream_readblocks_t));
	return blocks;
}

static void hz_bitstream_read_blocks_free(hz_bitstream_readblocks_t *blocks)
{
	hz_bitstream_readblock_t *b, *n;
	if (blocks == NULL)
		return;
	for (b = blocks->blocks;b;b = n)
	{
		n = b->next;
		Z_Free(b);
	}
	Z_Free(blocks);
}

static void hz_bitstream_read_flushbits(hz_bitstream_readblocks_t *blocks)
{
	blocks->store = 0;
	blocks->count = 0;
}

static int hz_bitstream_read_blocks_read(hz_bitstream_readblocks_t *blocks, hz_bitstream_read_t *stream, unsigned int size)
{
	int s;
	hz_bitstream_readblock_t *b, *p;
	s = size;
	p = NULL;
	b = blocks->blocks;
	while (s > 0)
	{
		if (b == NULL)
		{
			b = (hz_bitstream_readblock_t *)Z_Malloc(sizeof(hz_bitstream_readblock_t));
			if (b == NULL)
				return HZREADERROR_MALLOCFAILED;
			b->next = NULL;
			b->size = 0;
			if (p != NULL)
				p->next = b;
			else
				blocks->blocks = b;
		}
		if (s > HZREADBLOCKSIZE)
			b->size = HZREADBLOCKSIZE;
		else
			b->size = s;
		s -= b->size;
		if (FS_Read(stream->file, b->data, b->size) != (fs_offset_t)b->size)
		{
			stream->endoffile = 1;
			break;
		}
		p = b;
		b = b->next;
	}
	while (b)
	{
		b->size = 0;
		b = b->next;
	}
	blocks->current = blocks->blocks;
	blocks->position = 0;
	hz_bitstream_read_flushbits(blocks);
	if (stream->endoffile)
		return HZREADERROR_EOF;
	return HZREADERROR_OK;
}

static unsigned int hz_bitstream_read_blocks_getbyte(hz_bitstream_readblocks_t *blocks)
{
	while (blocks->current != NULL && blocks->position >= blocks->current->size)
	{
		blocks->position = 0;
		blocks->current = blocks->current->next;
	}
	if (blocks->current == NULL)
		return 0;
	return blocks->current->data[blocks->position++];
}

static int hz_bitstream_read_bit(hz_bitstream_readblocks_t *blocks)
{
	if (!blocks->count)
	{
		blocks->count += 8;
		blocks->store <<= 8;
		blocks->store |= hz_bitstream_read_blocks_getbyte(blocks) & 0xFF;
	}
	blocks->count--;
	return (blocks->store >> blocks->count) & 1;
}

static unsigned int hz_bitstream_read_bits(hz_bitstream_readblocks_t *blocks, int size)
{
	unsigned int num = 0;
	// we can only handle about 24 bits at a time safely
	// (there might be up to 7 bits more than we need in the bit store)
	if (size > 24)
	{
		size -= 8;
		num |= hz_bitstream_read_bits(blocks, 8) << size;
	}
	while (blocks->count < size)
	{
		blocks->count += 8;
		blocks->store <<= 8;
		blocks->store |= hz_bitstream_read_blocks_getbyte(blocks) & 0xFF;
	}
	blocks->count -= size;
	num |= (blocks->store >> blocks->count) & ((1 << size) - 1);
	return num;
}

static unsigned int hz_bitstream_read_byte(hz_bitstream_readblocks_t *blocks)
{
	return hz_bitstream_read_blocks_getbyte(blocks);
}

static unsigned int hz_bitstream_read_short(hz_bitstream_readblocks_t *blocks)
{
	return (hz_bitstream_read_byte(blocks) << 8)
	     | (hz_bitstream_read_byte(blocks));
}

static unsigned int hz_bitstream_read_int(hz_bitstream_readblocks_t *blocks)
{
	return (hz_bitstream_read_byte(blocks) << 24)
	     | (hz_bitstream_read_byte(blocks) << 16)
	     | (hz_bitstream_read_byte(blocks) << 8)
	     | (hz_bitstream_read_byte(blocks));
}

static void hz_bitstream_read_bytes(hz_bitstream_readblocks_t *blocks, void *outdata, unsigned int size)
{
	unsigned char *out;
	out = (unsigned char *)outdata;
	while (size--)
		*out++ = hz_bitstream_read_byte(blocks);
}

#define BLOCKSIZE 8

typedef struct dpvsimpledecodestream_s
{
	hz_bitstream_read_t *bitstream;
	hz_bitstream_readblocks_t *framedatablocks;

	int error;

	double info_framerate;
	unsigned int info_frames;

	unsigned int info_imagewidth;
	unsigned int info_imageheight;
	unsigned int info_imagebpp;
	unsigned int info_imageRloss;
	unsigned int info_imageRmask;
	unsigned int info_imageRshift;
	unsigned int info_imageGloss;
	unsigned int info_imageGmask;
	unsigned int info_imageGshift;
	unsigned int info_imageBloss;
	unsigned int info_imageBmask;
	unsigned int info_imageBshift;
	unsigned int info_imagesize;
	double info_aspectratio;

	// current video frame (needed because of delta compression)
	int videoframenum;
	// current video frame data (needed because of delta compression)
	unsigned int *videopixels;

	// channel the sound file is being played on
	int sndchan;
}
dpvsimpledecodestream_t;

static int dpvsimpledecode_setpixelformat(dpvsimpledecodestream_t *s, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel)
{
	int Rshift, Rbits, Gshift, Gbits, Bshift, Bbits;
	if (!Rmask)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDRMASK;
		return s->error;
	}
	if (!Gmask)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDGMASK;
		return s->error;
	}
	if (!Bmask)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDBMASK;
		return s->error;
	}
	if (Rmask & Gmask || Rmask & Bmask || Gmask & Bmask)
	{
		s->error = DPVSIMPLEDECODEERROR_COLORMASKSOVERLAP;
		return s->error;
	}
	switch (bytesperpixel)
	{
	case 2:
		if ((Rmask | Gmask | Bmask) > 65536)
		{
			s->error = DPVSIMPLEDECODEERROR_COLORMASKSEXCEEDBPP;
			return s->error;
		}
		break;
	case 4:
		break;
	default:
		s->error = DPVSIMPLEDECODEERROR_UNSUPPORTEDBPP;
		return s->error;
	}
	for (Rshift = 0;!(Rmask & 1);Rshift++, Rmask >>= 1);
	for (Gshift = 0;!(Gmask & 1);Gshift++, Gmask >>= 1);
	for (Bshift = 0;!(Bmask & 1);Bshift++, Bmask >>= 1);
	if (((Rmask + 1) & Rmask) != 0)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDRMASK;
		return s->error;
	}
	if (((Gmask + 1) & Gmask) != 0)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDGMASK;
		return s->error;
	}
	if (((Bmask + 1) & Bmask) != 0)
	{
		s->error = DPVSIMPLEDECODEERROR_INVALIDBMASK;
		return s->error;
	}
	for (Rbits = 0;Rmask & 1;Rbits++, Rmask >>= 1);
	for (Gbits = 0;Gmask & 1;Gbits++, Gmask >>= 1);
	for (Bbits = 0;Bmask & 1;Bbits++, Bmask >>= 1);
	if (Rbits > 8)
	{
		Rshift += (Rbits - 8);
		Rbits = 8;
	}
	if (Gbits > 8)
	{
		Gshift += (Gbits - 8);
		Gbits = 8;
	}
	if (Bbits > 8)
	{
		Bshift += (Bbits - 8);
		Bbits = 8;
	}
	s->info_imagebpp = bytesperpixel;
	s->info_imageRloss = 16 + (8 - Rbits);
	s->info_imageGloss =  8 + (8 - Gbits);
	s->info_imageBloss =  0 + (8 - Bbits);
	s->info_imageRmask = (1 << Rbits) - 1;
	s->info_imageGmask = (1 << Gbits) - 1;
	s->info_imageBmask = (1 << Bbits) - 1;
	s->info_imageRshift = Rshift;
	s->info_imageGshift = Gshift;
	s->info_imageBshift = Bshift;
	s->info_imagesize = s->info_imagewidth * s->info_imageheight * s->info_imagebpp;
	return s->error;
}

// opening and closing streams

// opens a stream
void *dpvsimpledecode_open(clvideo_t *video, char *filename, const char **errorstring)
{
	dpvsimpledecodestream_t *s;
	char t[8], *wavename;
	if (errorstring != NULL)
		*errorstring = NULL;
	s = (dpvsimpledecodestream_t *)Z_Malloc(sizeof(dpvsimpledecodestream_t));
	if (s != NULL)
	{
		s->bitstream = hz_bitstream_read_open(filename);
		if (s->bitstream != NULL)
		{
			// check file identification
			s->framedatablocks = hz_bitstream_read_blocks_new();
			if (s->framedatablocks != NULL)
			{
				hz_bitstream_read_blocks_read(s->framedatablocks, s->bitstream, 8);
				hz_bitstream_read_bytes(s->framedatablocks, t, 8);
				if (!memcmp(t, "DPVideo", 8))
				{
					// check version number
					hz_bitstream_read_blocks_read(s->framedatablocks, s->bitstream, 2);
					if (hz_bitstream_read_short(s->framedatablocks) == 1)
					{
						hz_bitstream_read_blocks_read(s->framedatablocks, s->bitstream, 12);
						s->info_imagewidth = hz_bitstream_read_short(s->framedatablocks);
						s->info_imageheight = hz_bitstream_read_short(s->framedatablocks);
						s->info_framerate = (double) hz_bitstream_read_int(s->framedatablocks) * (1.0 / 65536.0);
						s->info_aspectratio = (double)s->info_imagewidth / (double)s->info_imageheight;

						if (s->info_framerate > 0.0)
						{
							s->videopixels = (unsigned int *)Z_Malloc(s->info_imagewidth * s->info_imageheight * sizeof(*s->videopixels));
							if (s->videopixels != NULL)
							{
								size_t namelen;

								namelen = strlen(filename) + 10;
								wavename = (char *)Z_Malloc(namelen);
								if (wavename)
								{
									sfx_t* sfx;

									FS_StripExtension(filename, wavename, namelen);
									dp_strlcat(wavename, ".wav", namelen);
									sfx = S_PrecacheSound (wavename, false, false);
									if (sfx != NULL)
										s->sndchan = S_StartSound (-1, 0, sfx, vec3_origin, 1.0f, 0);
									else
										s->sndchan = -1;
									Z_Free(wavename);
								}
								// all is well...
								// set the module functions
								s->videoframenum = -10000;
								video->close = dpvsimpledecode_close;
								video->getwidth = dpvsimpledecode_getwidth;
								video->getheight = dpvsimpledecode_getheight;
								video->getframerate = dpvsimpledecode_getframerate;
								video->decodeframe = dpvsimpledecode_video;
								video->getaspectratio = dpvsimpledecode_getaspectratio;

								return s;
							}
							else if (errorstring != NULL)
								*errorstring = "unable to allocate video image buffer";
						}
						else if (errorstring != NULL)
							*errorstring = "error in video info chunk";
					}
					else if (errorstring != NULL)
						*errorstring = "read error";
				}
				else if (errorstring != NULL)
					*errorstring = "not a dpvideo file";
 				hz_bitstream_read_blocks_free(s->framedatablocks);
			}
			else if (errorstring != NULL)
				*errorstring = "unable to allocate memory for reading buffer";
			hz_bitstream_read_close(s->bitstream);
		}
		else if (errorstring != NULL)
			*errorstring = "unable to open file";
		Z_Free(s);
	}
	else if (errorstring != NULL)
		*errorstring = "unable to allocate memory for stream info structure";
	return NULL;
}

// closes a stream
void dpvsimpledecode_close(void *stream)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	if (s == NULL)
		return;
	if (s->videopixels)
		Z_Free(s->videopixels);
	if (s->sndchan != -1)
		S_StopChannel (s->sndchan, true, true);
	if (s->framedatablocks)
		hz_bitstream_read_blocks_free(s->framedatablocks);
	if (s->bitstream)
		hz_bitstream_read_close(s->bitstream);
	Z_Free(s);
}

// utilitarian functions

// returns the current error number for the stream, and resets the error
// number to DPVSIMPLEDECODEERROR_NONE
// if the supplied string pointer variable is not NULL, it will be set to the
// error message
int dpvsimpledecode_error(void *stream, const char **errorstring)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	int e;
	e = s->error;
	s->error = 0;
	if (errorstring)
	{
		switch (e)
		{
			case DPVSIMPLEDECODEERROR_NONE:
				*errorstring = "no error";
				break;
			case DPVSIMPLEDECODEERROR_EOF:
				*errorstring = "end of file reached (this is not an error)";
				break;
			case DPVSIMPLEDECODEERROR_READERROR:
				*errorstring = "read error (corrupt or incomplete file)";
				break;
			case DPVSIMPLEDECODEERROR_SOUNDBUFFERTOOSMALL:
				*errorstring = "sound buffer is too small for decoding frame (please allocate it as large as dpvsimpledecode_getneededsoundbufferlength suggests)";
				break;
			case DPVSIMPLEDECODEERROR_INVALIDRMASK:
				*errorstring = "invalid red bits mask";
				break;
			case DPVSIMPLEDECODEERROR_INVALIDGMASK:
				*errorstring = "invalid green bits mask";
				break;
			case DPVSIMPLEDECODEERROR_INVALIDBMASK:
				*errorstring = "invalid blue bits mask";
				break;
			case DPVSIMPLEDECODEERROR_COLORMASKSOVERLAP:
				*errorstring = "color bit masks overlap";
				break;
			case DPVSIMPLEDECODEERROR_COLORMASKSEXCEEDBPP:
				*errorstring = "color masks too big for specified bytes per pixel";
				break;
			case DPVSIMPLEDECODEERROR_UNSUPPORTEDBPP:
				*errorstring = "unsupported bytes per pixel (must be 2 for 16bit, or 4 for 32bit)";
				break;
			default:
				*errorstring = "unknown error";
				break;
		}
	}
	return e;
}

// returns the width of the image data
unsigned int dpvsimpledecode_getwidth(void *stream)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	return s->info_imagewidth;
}

// returns the height of the image data
unsigned int dpvsimpledecode_getheight(void *stream)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	return s->info_imageheight;
}

// returns the framerate of the stream
double dpvsimpledecode_getframerate(void *stream)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	return s->info_framerate;
}

// return aspect ratio of the stream
double dpvsimpledecode_getaspectratio(void *stream)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	return s->info_aspectratio;
}

static int dpvsimpledecode_convertpixels(dpvsimpledecodestream_t *s, void *imagedata, int imagebytesperrow)
{
	unsigned int a, x, y, width, height;
	unsigned int Rloss, Rmask, Rshift, Gloss, Gmask, Gshift, Bloss, Bmask, Bshift;
	unsigned int *in;

	width = s->info_imagewidth;
	height = s->info_imageheight;

	Rloss = s->info_imageRloss;
	Rmask = s->info_imageRmask;
	Rshift = s->info_imageRshift;
	Gloss = s->info_imageGloss;
	Gmask = s->info_imageGmask;
	Gshift = s->info_imageGshift;
	Bloss = s->info_imageBloss;
	Bmask = s->info_imageBmask;
	Bshift = s->info_imageBshift;

	in = s->videopixels;
	if (s->info_imagebpp == 4)
	{
		unsigned int *outrow;
		for (y = 0;y < height;y++)
		{
			outrow = (unsigned int *)((unsigned char *)imagedata + y * imagebytesperrow);
			for (x = 0;x < width;x++)
			{
				a = *in++;
				outrow[x] = (((a >> Rloss) & Rmask) << Rshift) | (((a >> Gloss) & Gmask) << Gshift) | (((a >> Bloss) & Bmask) << Bshift);
			}
		}
	}
	else
	{
		unsigned short *outrow;
		for (y = 0;y < height;y++)
		{
			outrow = (unsigned short *)((unsigned char *)imagedata + y * imagebytesperrow);
			if (Rloss == 19 && Gloss == 10 && Bloss == 3 && Rshift == 11 && Gshift == 5 && Bshift == 0)
			{
				// optimized
				for (x = 0;x < width;x++)
				{
					a = *in++;
					outrow[x] = ((a >> 8) & 0xF800) | ((a >> 5) & 0x07E0) | ((a >> 3) & 0x001F);
				}
			}
			else
			{
				for (x = 0;x < width;x++)
				{
					a = *in++;
					outrow[x] = (((a >> Rloss) & Rmask) << Rshift) | (((a >> Gloss) & Gmask) << Gshift) | (((a >> Bloss) & Bmask) << Bshift);
				}
			}
		}
	}
	return s->error;
}

static int dpvsimpledecode_decompressimage(dpvsimpledecodestream_t *s)
{
	int i, a, b, colors, g, x1, y1, bw, bh, width, height, palettebits;
	unsigned int palette[256], *outrow, *out;
	g = BLOCKSIZE;
	width = s->info_imagewidth;
	height = s->info_imageheight;
	for (y1 = 0;y1 < height;y1 += g)
	{
		outrow = s->videopixels + y1 * width;
		bh = g;
		if (y1 + bh > height)
			bh = height - y1;
		for (x1 = 0;x1 < width;x1 += g)
		{
			out = outrow + x1;
			bw = g;
			if (x1 + bw > width)
				bw = width - x1;
			if (hz_bitstream_read_bit(s->framedatablocks))
			{
				// updated block
				palettebits = hz_bitstream_read_bits(s->framedatablocks, 3);
				colors = 1 << palettebits;
				for (i = 0;i < colors;i++)
					palette[i] = hz_bitstream_read_bits(s->framedatablocks, 24);
				if (palettebits)
				{
					for (b = 0;b < bh;b++, out += width)
						for (a = 0;a < bw;a++)
							out[a] = palette[hz_bitstream_read_bits(s->framedatablocks, palettebits)];
				}
				else
				{
					for (b = 0;b < bh;b++, out += width)
						for (a = 0;a < bw;a++)
							out[a] = palette[0];
				}
			}
		}
	}
	return s->error;
}

// decodes a video frame to the supplied output pixels
int dpvsimpledecode_video(void *stream, void *imagedata, unsigned int Rmask, unsigned int Gmask, unsigned int Bmask, unsigned int bytesperpixel, int imagebytesperrow)
{
	dpvsimpledecodestream_t *s = (dpvsimpledecodestream_t *)stream;
	unsigned int framedatasize;
	char t[4];
	s->error = DPVSIMPLEDECODEERROR_NONE;
	if (dpvsimpledecode_setpixelformat(s, Rmask, Gmask, Bmask, bytesperpixel))
		return s->error;

	hz_bitstream_read_blocks_read(s->framedatablocks, s->bitstream, 8);
	hz_bitstream_read_bytes(s->framedatablocks, t, 4);
	if (memcmp(t, "VID0", 4))
	{
		if (t[0] == 0)
			return (s->error = DPVSIMPLEDECODEERROR_EOF);
		else
			return (s->error = DPVSIMPLEDECODEERROR_READERROR);
	}
	framedatasize = hz_bitstream_read_int(s->framedatablocks);
	hz_bitstream_read_blocks_read(s->framedatablocks, s->bitstream, framedatasize);
	if (dpvsimpledecode_decompressimage(s))
		return s->error;

	dpvsimpledecode_convertpixels(s, imagedata, imagebytesperrow);
	return s->error;
}
