#include "quakedef.h"
#include "shader.h"
#include "glquake.h"	//we need some of the gl format enums

#ifdef __GNUC__
#pragma
#endif

#ifndef HAVE_CLIENT
//#define Con_Printf(f, ...)
float	FloatSwap (float l)
{
	union {qbyte b[4]; float f;} in, out;
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	return out.f;
}

#undef S_COLOR_BLACK
#undef S_COLOR_RED
#undef S_COLOR_GREEN
#undef S_COLOR_YELLOW
#undef S_COLOR_BLUE
#undef S_COLOR_CYAN
#undef S_COLOR_WHITE
#undef S_COLOR_TRANS
#undef S_COLOR_WHITE
#undef S_COLOR_GRAY
#else
#ifdef IMAGEFMT_TGA
cvar_t r_dodgytgafiles = CVARD("r_dodgytgafiles", "0", "Many old glquake engines had a buggy tga loader that ignored bottom-up flags. Naturally people worked around this and the world was plagued with buggy images. Most engines have now fixed the bug, but you can reenable it if you have bugged tga files.");
#endif
#ifdef IMAGEFMT_PCX
cvar_t r_dodgypcxfiles = CVARD("r_dodgypcxfiles", "0", "When enabled, this will ignore the palette stored within pcx files, for compatibility with quake2.");
#endif
cvar_t r_dodgymiptex = CVARD("r_dodgymiptex", "1", "When enabled, this will force regeneration of mipmaps, discarding mips1-4 like glquake did. This may eg solve fullbright issues with some maps, but may reduce distant detail levels.");
cvar_t r_keepimages = CVARD("r_keepimages", "0", "Retain unused images in memory for slightly faster map loading.\n0: Redundant images will be purged after each map change.\n1: Images will be retained until vid_reload (potentially consuming a lot of ram).");
cvar_t r_ignoremapprefixes = CVARD("r_ignoremapprefixes", "0",  "Ignores when textures were loaded from map-specific paths.\n0: textures/foo/tex.tga will not be confused with textures/foo/tex.tga.\n1: The same texture might be loaded multiple times over.");

char *r_defaultimageextensions =
#ifdef IMAGEFMT_DDS
	"dds "	//compressed or something
#endif
#ifdef IMAGEFMT_KTX
	"ktx "	//compressed or something. not to be confused with the qw mod by the same name. GL requires that etc2 compression is supported by modern drivers, but not necessarily the hardware. as such, dds with its s3tc bias should always come first (as the patents mean that drivers are much less likely to advertise it when they don't support it properly).
	//"ktx2 "
#endif
#ifdef IMAGEFMT_TGA
	"tga"	//fairly fast to load
	//" htga"
#endif
#if defined(IMAGEFMT_PNG) || defined(FTE_TARGET_WEB)
	" png"	//pngs, fairly common, but slow
#endif
#ifdef IMAGEFMT_BMP
	//" bmp"	//wtf? at least not lossy
	//" ico"	//noone wants this...
#endif
#if defined(IMAGEFMT_JPG) || defined(FTE_TARGET_WEB)
	" jpg"	//q3 uses some jpegs, for some reason
	//" jpeg"	//thankfuly the quake community stuck to .jpg instead
#endif
#ifdef IMAGEFMT_PBM
	//" pfm" //float version (technically seperate, but similarish)
	//" pbm" //1-bit v grey
	//" pgm" //greyscale
	//" ppm" //rgb values
	//" pam" //'arbitrary' version
#endif
#ifdef IMAGEFMT_PSD
	//" psd" //paintshop images (8bit+16bit, but base layer only)
#endif
#ifdef IMAGEFMT_XCF
	//" xcf" //gimp's own format
#endif
#ifdef IMAGEFMT_HDR
	//" hdr" //some file that uses RGBE formatted data, for hdr images.
#endif
#ifdef IMAGEFMT_PKM
	//" pkm"	//compressed format, but lacks mipmaps which makes it terrible to use.
#endif
#ifdef IMAGEFMT_ASTC
	//" astc"	//compressed format, but lacks mipmaps which makes it terrible to use.
#endif
#ifdef IMAGEFMT_GIF
	//" gif"
#endif
#ifdef IMAGEFMT_PIC
	//" pic"
#endif
#ifdef IMAGEFMT_PCX
	" pcx"	//pcxes are the original gamedata of q2. So we don't want them to override pngs.
#endif
#ifdef IMAGEFMT_LMP
	//" lmp" //lame outdated junk. any code that expects a lmp will use that extension, so don't bother swapping out extensions for it.
#endif
	;

static void QDECL R_ImageExtensions_Callback(struct cvar_s *var, char *oldvalue);
cvar_t r_imageextensions			= CVARCD("r_imageextensions", NULL, R_ImageExtensions_Callback, "The list of image file extensions which might exist on disk (note that this does not list all supported formats, only the extensions that should be searched for).");
cvar_t r_image_downloadsizelimit	= CVARFD("r_image_downloadsizelimit", "131072", CVAR_NOTFROMSERVER, "The maximum allowed file size of images loaded from a web-based url. 0 disables completely, while empty imposes no limit.");
extern cvar_t			scr_sshot_compression;
extern cvar_t gl_lerpimages;
extern cvar_t gl_picmip2d;
extern cvar_t gl_picmip;
extern cvar_t gl_picmip_world;
extern cvar_t gl_picmip_sprites;
extern cvar_t gl_picmip_other;
extern cvar_t r_shadow_bumpscale_basetexture;
extern cvar_t r_shadow_bumpscale_bumpmap;
extern cvar_t r_shadow_heightscale_basetexture;
extern cvar_t r_shadow_heightscale_bumpmap;


static bucket_t *imagetablebuckets[256];
static hashtable_t imagetable;
static image_t *imagelist;
#endif

#ifdef AVAIL_STBI
	#if defined(IMAGEFMT_PNG) && !defined(AVAIL_PNGLIB) && !defined(FTE_TARGET_WEB)
		#define STBI_ONLY_PNG
		#define STBIW_ONLY_PNG
		#undef IMAGEFMT_PNG
	#endif
	#if defined(IMAGEFMT_JPG) && !defined(AVAIL_JPEGLIB) && !defined(FTE_TARGET_WEB)
		#define STBI_ONLY_JPEG
		#define STBIW_ONLY_JPEG
		#undef IMAGEFMT_JPG
	#endif
	#if defined(IMAGEFMT_BMP) && 0 //use our own implementation, giving .ico too
		#define STBI_ONLY_BMP
		#define STBIW_ONLY_BMP
		#undef IMAGEFMT_BMP
	#endif
	#if defined(IMAGEFMT_PSD) && 0 //use our own implementation
		#define STBI_ONLY_PSD
		#undef IMAGEFMT_PSD
	#endif
	#if defined(IMAGEFMT_TGA) && 0 //use our own implementation, giving htga and some twiddles.
		#define STBI_ONLY_TGA
		#define STBIW_ONLY_TGA
		#undef IMAGEFMT_TGA
	#endif
	#if defined(IMAGEFMT_GIF) //&& 0
		#define STBI_ONLY_GIF
		#undef IMAGEFMT_GIF
	#endif
	#if defined(IMAGEFMT_HDR) && 0 //use our own implementation, we're not using the stbi_loadf stuff anyway
		#define STBI_ONLY_HDR
		#define STBIW_ONLY_HDR
		#undef IMAGEFMT_HDR
	#endif
	#if defined(IMAGEFMT_PIC) //&& 0
		#define STBI_ONLY_PIC
		#undef IMAGEFMT_PIC
	#endif
	#if defined(IMAGEFMT_PBM) && 0 //use our own implementation, giving pfm.pbm.pam too
		#define STBI_ONLY_PNM
		#undef IMAGEFMT_PBM
	#endif

	//now we know whether we need stbi or not, pull in the right stuff.
	#if defined(STBI_ONLY_PNG) || defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_BMP) || defined(STBI_ONLY_PSD) || defined(STBI_ONLY_TGA) || defined(STBI_ONLY_GIF) || defined(STBI_ONLY_HDR) || defined(STBI_ONLY_PIC) || defined(STBI_ONLY_PNM)
		#define STBI_NO_STDIO
		#define STBI_MALLOC BZ_Malloc
		#define STBI_REALLOC BZ_Realloc
		#define STBI_FREE BZ_Free
		//#define STBI_NO_FAILURE_STRINGS //not thread safe, so don't bother showing messages.
		#define STB_IMAGE_IMPLEMENTATION
		#include "../libs/stb_image.h"	//from https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
	#endif
	#if defined(STBIW_ONLY_PNG) || defined(STBIW_ONLY_JPEG) || defined(STBIW_ONLY_BMP) || defined(STBIW_ONLY_TGA) || defined(STBIW_ONLY_HDR)
		#define STBI_WRITE_NO_STDIO
		#define STB_IMAGE_WRITE_STATIC
		#define STBIWDEF fte_inlinestatic
		#define STB_IMAGE_WRITE_IMPLEMENTATION
		#include "../libs/stb_image_write.h"	//from https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
	#endif
#endif

#if defined(IMAGEFMT_GIF) && !defined(FTE_TARGET_WEB)
	#pragma message("IMAGEFMT_GIF requires AVAIL_STBI")
	#undef IMAGEFMT_GIF
#endif
#if defined(IMAGEFMT_PNG) && !defined(AVAIL_PNGLIB) && !defined(FTE_TARGET_WEB)
	#pragma message("IMAGEFMT_PNG requires AVAIL_PNGLIB or AVAIL_STBI")
	#undef IMAGEFMT_PNG
#elif !defined(IMAGEFMT_PNG) && defined(AVAIL_PNGLIB)
	#undef AVAIL_PNGLIB
#endif
#if defined(IMAGEFMT_JPG) && !defined(AVAIL_JPEGLIB) && !defined(FTE_TARGET_WEB)
	#pragma message("IMAGEFMT_JPG requires AVAIL_JPEGLIB or AVAIL_STBI")
	#undef IMAGEFMT_JPG
#elif !defined(IMAGEFMT_JPG) && defined(AVAIL_JPEGLIB)
	#undef AVAIL_JPEGLIB
#endif

#if defined(IMAGEFMT_EXR) && defined(FTE_TARGET_WEB)
	#undef IMAGEFMT_EXR
#endif

#ifndef LIBPNG_STATIC
#define DYNAMIC_LIBPNG
#endif
#ifndef LIBJPEG_STATIC
#define DYNAMIC_LIBJPEG
#endif

#ifdef DECOMPRESS_ASTC
#define ASTC_PUBLIC
#ifdef ASTC3D
#define ASTC_WITH_3D
#endif
#include "image_astc.h"
#endif

//for soft-decoding e5bgr9's exponent. multiply by the per-channel mantissa for a 0-1 result.
const float rgb9e5tab[32] = {
	//aka: pow(2, biasedexponent - bias-bits) where bias is 15 and bits is 9
	1.0/(1<<24),	1.0/(1<<23),	1.0/(1<<22),	1.0/(1<<21),	1.0/(1<<20),	1.0/(1<<19),	1.0/(1<<18),	1.0/(1<<17),
	1.0/(1<<16),	1.0/(1<<15),	1.0/(1<<14),	1.0/(1<<13),	1.0/(1<<12),	1.0/(1<<11),	1.0/(1<<10),	1.0/(1<<9),
	1.0/(1<<8),		1.0/(1<<7),		1.0/(1<<6),		1.0/(1<<5),		1.0/(1<<4),		1.0/(1<<3),		1.0/(1<<2),		1.0/(1<<1),
	1.0,			1.0*(1<<1),		1.0*(1<<2),		1.0*(1<<3),		1.0*(1<<4),		1.0*(1<<5),		1.0*(1<<6),		1.0*(1<<7),
};
static float HalfToFloat(unsigned short val);
static unsigned short FloatToHalf(float val);



static struct
{
	void *module;
	plugimageloaderfuncs_t *funcs;
} *imageloader;
static size_t		imageloader_count;
qboolean Image_RegisterLoader(void *module, plugimageloaderfuncs_t *driver)
{
	int i;
	if (!driver)
	{
		for (i = 0; i < imageloader_count; )
		{
			if (imageloader[i].module == module)
			{
				memmove(&imageloader[i], &imageloader[i+1], imageloader_count-(i+1));
				imageloader_count--;
			}
			else
				i++;
		}
		return true;
	}
	else
	{
		void *n = BZ_Malloc(sizeof(*imageloader)*(imageloader_count+1));
		memcpy(n, imageloader, sizeof(*imageloader)*imageloader_count);
		Z_Free(imageloader);
		imageloader = n;
		imageloader[imageloader_count].module = module;
		imageloader[imageloader_count].funcs = driver;
		imageloader_count++;
		return true;
	}
}


#if defined(AVAIL_JPEGLIB) || defined(AVAIL_PNGLIB)
static void GenerateXMPData(char *blob, size_t blobsize, int width, int height, unsigned int metainfo)
{	//XMP is a general thing that applies to multiple formats - or at least png+jpeg.
	//we need this if we want to correctly flag the data as a 360 image.
#ifdef HAVE_CLIENT
	Q_snprintfz(blob, blobsize,
		"<x:xmpmeta xmlns:x='adobe:ns:meta/'>"
			"<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>"
		);

	if (metainfo)
		Q_snprintfz(blob, blobsize,
					"<rdf:Description rdf:about='' xmlns:GPano=\"http://ns.google.com/photos/1.0/panorama/\">"
						"<GPano:ProjectionType>equirectangular</GPano:ProjectionType>"
						"<GPano:PosePitchDegrees>%f</GPano:PosePitchDegrees>"
						"<GPano:PoseHeadingDegrees>%f</GPano:PoseHeadingDegrees>"
						"<GPano:PoseRollDegrees>%f</GPano:PoseRollDegrees>"
						"<GPano:InitialViewHeadingDegrees>%f</GPano:InitialViewHeadingDegrees>"
						"<GPano:InitialViewPitchDegrees>%f</GPano:InitialViewPitchDegrees>"
						"<GPano:InitialViewRollDegrees>%f</GPano:InitialViewRollDegrees>"
						"<GPano:CroppedAreaLeftPixels>0</GPano:CroppedAreaLeftPixels>"
						"<GPano:CroppedAreaTopPixels>0</GPano:CroppedAreaTopPixels>"
						"<GPano:CroppedAreaImageWidthPixels>%i</GPano:CroppedAreaImageWidthPixels>"
						"<GPano:CroppedAreaImageHeightPixels>%i</GPano:CroppedAreaImageHeightPixels>"
						"<GPano:FullPanoWidthPixels>%i</GPano:FullPanoWidthPixels>"
						"<GPano:FullPanoHeightPixels>%i</GPano:FullPanoHeightPixels>"
					"</rdf:Description>",
			r_refdef.viewangles[0], r_refdef.viewangles[1], r_refdef.viewangles[2],
			r_refdef.viewangles[0], r_refdef.viewangles[1], r_refdef.viewangles[2],
			width, height, width, height);

	Q_snprintfz(blob+strlen(blob), blobsize-strlen(blob),
			"</rdf:RDF>"
		"</x:xmpmeta>"
		);
#else
	blob[blobsize] = 0;
#endif
}
#endif

#ifdef IMAGEFMT_TGA
#ifndef _WIN32
#include <unistd.h>
#endif

typedef struct {	//cm = colourmap
	char	id_len;		//0
	char	cm_type;	//1
	qbyte	version;	//2
		char pad1;
	short	cm_idx;		//3
	short	cm_len;		//5
	char	cm_size;	//7
		char pad2;
	short	originx;	//8 (ignored)
	short	originy;	//10 (ignored)
	short	width;		//12-13
	short	height;		//14-15
	qbyte	bpp;		//16
	qbyte	attribs;	//17
} tgaheader_t;

static char *ReadGreyTargaFile (qbyte *data, int flen, tgaheader_t *tgahead, int asgrey)	//preswapped header
{
	int				columns, rows;
	int				row, column;
	qbyte			*pixbuf, *pal;
	qboolean		flipped;

	qbyte *pixels = BZ_Malloc(tgahead->width * tgahead->height * (asgrey?1:4));

	if (tgahead->version!=1
		&& tgahead->version!=3)
	{
		Con_Printf("LoadGrayTGA: Only type 1 and 3 greyscale targa images are understood.\n");
		BZ_Free(pixels);
		return NULL;
	}

	if (tgahead->version==1 && tgahead->bpp != 8 &&
		tgahead->cm_size != 24 && tgahead->cm_len != 256)
	{
		Con_Printf("LoadGrayTGA: Strange palette type\n");
		BZ_Free(pixels);
		return NULL;
	}

	columns = tgahead->width;
	rows = tgahead->height;

	flipped = !((tgahead->attribs & 0x20) >> 5);
#ifdef HAVE_CLIENT
	if (r_dodgytgafiles.value)
		flipped = true;
#endif

	if (tgahead->version == 1)
	{	//paletted data...
		pal = data;
		data += tgahead->cm_len*3;
		if (asgrey)
		{
			for(row=rows-1; row>=0; row--)
			{
				if (flipped)
					pixbuf = pixels + row*columns;
				else
					pixbuf = pixels + ((rows-1)-row)*columns;

				for(column=0; column<columns; column++)
					*pixbuf++= *data++;
			}
		}
		else
		{
			for(row=rows-1; row>=0; row--)
			{
				if (flipped)
					pixbuf = pixels + row*columns*4;
				else
					pixbuf = pixels + ((rows-1)-row)*columns*4;

				for(column=0; column<columns; column++)
				{
					*pixbuf++= pal[*data*3+2];
					*pixbuf++= pal[*data*3+1];
					*pixbuf++= pal[*data*3+0];
					*pixbuf++= 255;
					data++;
				}
			}
		}
		return pixels;
	}
	//version 3 now. pure greyscale

	if (asgrey)
	{
		for(row=rows-1; row>=0; row--)
		{
			if (flipped)
				pixbuf = pixels + row*columns;
			else
				pixbuf = pixels + ((rows-1)-row)*columns;

			pixbuf = pixels + row*columns;
			for(column=0; column<columns; column++)
				*pixbuf++= *data++;
		}
	}
	else
	{
		for(row=rows-1; row>=0; row--)
		{
			if (flipped)
				pixbuf = pixels + row*columns*4;
			else
				pixbuf = pixels + ((rows-1)-row)*columns*4;

			for(column=0; column<columns; column++)
			{
				*pixbuf++= *data;
				*pixbuf++= *data;
				*pixbuf++= *data;
				*pixbuf++= 255;
				data++;
			}
		}
	}

	return pixels;
}

#define MISSHORT(ptr) (*(ptr) | (*(ptr+1) << 8))
//remember to free it
//greyonly causes the function to fail if given anything but greyscale images
//this is for detecting heightmaps instead of normalmaps.
void *ReadTargaFile(qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean greyonly, uploadfmt_t forceformat)
{
	//tga files sadly lack a true magic header thing.
	unsigned char *data;

	qboolean flipped;

	tgaheader_t tgaheader;	//things are misaligned, so no pointer.

	if (length < 18 || buf[1] > 1)
		return NULL;	//probably not a tga...

	tgaheader.id_len = buf[0];
	tgaheader.cm_type = buf[1];
	tgaheader.version = buf[2];
	tgaheader.cm_idx = MISSHORT(buf+3);
	tgaheader.cm_len = MISSHORT(buf+5);
	tgaheader.cm_size = buf[7];
	tgaheader.originx = MISSHORT(buf+8);
	tgaheader.originy = MISSHORT(buf+10);
	tgaheader.width = MISSHORT(buf+12);
	tgaheader.height = MISSHORT(buf+14);
	tgaheader.bpp = buf[16];
	tgaheader.attribs = buf[17];

	switch(tgaheader.version)
	{
	case 0:	//No image data included.
		return NULL;	//not really valid for us. reject it after all
	case 1:	//Uncompressed, color-mapped images.
	case 2:	//Uncompressed, RGB images.
	case 3:	//Uncompressed, black and white images.
	case 9:	//Runlength encoded color-mapped images.
	case 10:	//Runlength encoded RGB images.
	case 11:	//Runlength encoded, black and white images.
	case 32:	//Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
	case 33:	//Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process.
		if (buf[16] != 8 && buf[16] != 16 && buf[16] != 24 && buf[16] != 32)
			return NULL;	//unsupported bitdepths
		break;
	//0x80+ are third-party extensions...
	case 0x82:	//half-float rgb
	case 0x83:	//half-float greyscale
		if (forceformat != PTI_INVALID)
			return NULL;
		if ((buf[16]&15) || buf[16]<16 || buf[16] > 16*4)
			return NULL;	//unsupported bitdepths
		break;
	default:
		return NULL;
	}
	//validate the size to some sanity limit.
	if ((unsigned short)tgaheader.width > 16384 || (unsigned short)tgaheader.height > 16384)
		return NULL;


	flipped = !((tgaheader.attribs & 0x20) >> 5);
#ifdef HAVE_CLIENT
	if (r_dodgytgafiles.value)
		flipped = true;
#endif

	data=buf+18;
	data += tgaheader.id_len;

	*width = tgaheader.width;
	*height = tgaheader.height;

	if (greyonly)	//grey only, load as 8 bit..
	{
		if (!(tgaheader.version == 1) && !(tgaheader.version == 3) && !(tgaheader.version == 11))
			return NULL;
	}
	if (tgaheader.version == 1 || tgaheader.version == 3)
	{
		if (forceformat==PTI_L8 || forceformat==PTI_RGBA8 || forceformat==PTI_RGBX8)
			*format = forceformat;
		else if (tgaheader.version == 3)
			*format = PTI_L8;
		else
			*format = PTI_RGBX8;
		return ReadGreyTargaFile(data, length, &tgaheader, *format==PTI_L8);
	}
	else if (tgaheader.version == 10 || tgaheader.version == 9 || tgaheader.version == 11)
	{
		//9:RLE paletted
		//10:RLE bgr(a)
		//11:RLE greyscale
#undef getc
#define getc(x) *data++
		unsigned int row, rows=tgaheader.height, column, columns=tgaheader.width, packetHeader, packetSize, j;
		qbyte *pixbuf, *targa_rgba;
		unsigned int inraw;

		qbyte blue, red, green, alphabyte;

		byte_vec4_t palette[256];
		enum
		{
			rle_p8,
			rle_a1rgb5,
			rle_bgr8,
			rle_bgra8,
			rle_l8,
			rle_l8a8,
		} rlemode;
		int outbytes;

		*format = PTI_RGBX8;
		if (tgaheader.version == 9)
		{	//RLE palette
			if (tgaheader.bpp == 8)	//FIXME: tgaheader.bpp can be 8, 15, or 16.
				rlemode = rle_p8;
			else
				return NULL;

			for (row = 0; row < 256; row++)
			{
				palette[row][0] = row;
				palette[row][1] = row;
				palette[row][2] = row;
				palette[row][3] = 255;
			}

			if (forceformat == PTI_L8 || forceformat == PTI_INVALID)
				*format = forceformat = PTI_L8;
			else
				*format = forceformat = PTI_LLLX8;


			if (tgaheader.cm_type)
			{
				qboolean grey = true;
				switch(tgaheader.cm_size)
				{
				default:
					return NULL;
				case 24:
					for (row = 0; row < tgaheader.cm_len; row++)
					{
						if (data[0] != data[1] || data[0] != data[2])
							grey = false;
						palette[row][0] = *data++;
						palette[row][1] = *data++;
						palette[row][2] = *data++;
						palette[row][3] = 255;
					}
					if (grey && forceformat == PTI_INVALID)
						*format = PTI_L8;
					else if (forceformat != PTI_L8)
						*format = PTI_RGBA8;
					break;
				case 32:
					for (row = 0; row < tgaheader.cm_len; row++)
					{
						if (data[0] != data[1] || data[0] != data[2] || data[3] != 0xff)
							grey = false;
						palette[row][0] = *data++;
						palette[row][1] = *data++;
						palette[row][2] = *data++;
						palette[row][3] = *data++;
					}
					if (grey && forceformat == PTI_INVALID)
						*format = PTI_L8;
					else if (forceformat != PTI_L8)
						*format = PTI_RGBA8;
					break;
				}
			}
		}
		else if (tgaheader.version == 10)
		{	//RLE truecolour
			if (tgaheader.bpp == 16)
				rlemode = rle_a1rgb5;
			else if (tgaheader.bpp == 24)
				rlemode = rle_bgr8;
			else if (tgaheader.bpp == 32)
				rlemode = rle_bgra8;
			else
				return NULL;

			*format = (tgaheader.bpp==24)?PTI_RGBX8:PTI_RGBA8;
		}
		else if (tgaheader.version == 11)
		{	//RLE greyscale
			if (tgaheader.bpp == 8)
				rlemode = rle_l8;
			else if (tgaheader.bpp == 16)
				rlemode = rle_l8a8;
			else
				return NULL;

			if (forceformat == PTI_L8)
				*format = forceformat;
			else if (rlemode==rle_l8a8)
				*format = PTI_LLLA8; //should probably use PTI_L8A8, but the caller will know to optimise
			else
				*format = PTI_LLLX8;
		}
		else
			return NULL;

		if (*format == PTI_L8)
			outbytes = 1;
		else if (*format == PTI_L8A8)
			outbytes = 2;
		else
			outbytes = 4;
		targa_rgba=BZ_Malloc(rows*columns*outbytes);
		for(row=rows; row-->0; )
		{
			if (flipped)
				pixbuf = targa_rgba + row*columns*outbytes;
			else
				pixbuf = targa_rgba + ((rows-1)-row)*columns*outbytes;
			for(column=0; column<columns; )
			{
				packetHeader=*data++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)
				{	// run-length packet
					switch (rlemode)
					{
					case rle_p8:
						blue = palette[*data][0];
						green = palette[*data][1];
						red = palette[*data][2];
						alphabyte = palette[*data][3];
						data++;
						break;
					case rle_l8a8:
						blue = green = red = *data++;
						alphabyte = *data++;
						break;
					case rle_l8:
						blue = green = red = *data++;
						alphabyte = 255;
						break;

					case rle_a1rgb5:
						inraw = data[0] | (data[1]<<8);
						data+=2;
						alphabyte = (inraw&0x8000)?255:0;
						red = ((inraw>>10)&0x1f)<<3;
						green = ((inraw>>5)&0x1f)<<3;
						blue = ((inraw>>0)&0x1f)<<3;
						break;
					case rle_bgr8:
						blue = *data++;
						green = *data++;
						red = *data++;
						alphabyte = 255;
						break;
					case rle_bgra8:
						blue = *data++;
						green = *data++;
						red = *data++;
						alphabyte = *data++;
						break;
					default:
						blue = green = red = alphabyte = 255;
						break;
					}

					if (*format!=PTI_L8)	//keep colours
					{
						for(j=0;j<packetSize;j++)
						{
							*pixbuf++=red;
							*pixbuf++=green;
							*pixbuf++=blue;
							*pixbuf++=alphabyte;
							column++;
							if (column==columns)
							{ // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*4;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*4;
							}
						}
					}
					else	//convert to greyscale
					{
						for(j=0;j<packetSize;j++)
						{
							*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
							column++;
							if (column==columns)
							{ // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*1;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*1;
							}
						}
					}
				}
				else
				{                            // non run-length packet
					if (*format!=PTI_L8)	//keep colours
					{
						for(j=0;j<packetSize;j++)
						{
							switch (rlemode)
							{
							case rle_p8:
								blue = palette[*data][0];
								green = palette[*data][1];
								red = palette[*data][2];
								alphabyte = palette[*data][3];
								data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_l8a8:
								blue = green = red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_l8:
								blue = green = red = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case rle_a1rgb5:
								inraw = data[0] | (data[1]<<8);
								data+=2;
								alphabyte = (inraw&0x8000)?255:0;
								red = ((inraw>>10)&0x1f)<<3;
								green = ((inraw>>5)&0x1f)<<3;
								blue = ((inraw>>0)&0x1f)<<3;

								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_bgr8:
								blue = *data++;
								green = *data++;
								red = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case rle_bgra8:
								blue = *data++;
								green = *data++;
								red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							}
							column++;
							if (column==columns)
							{ // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*4;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*4;
							}
						}
					}
					else	//convert to grey
					{
						for(j=0;j<packetSize;j++)
						{
							switch (rlemode)
							{
							case rle_p8:
								blue = palette[*data][0];
								green = palette[*data][1];
								red = palette[*data][2];
								*pixbuf++ = (blue + green + red)/3;
								data++;
								break;
							case rle_l8a8:
								blue = green = red = *data++;
								alphabyte = *data++;
								*pixbuf++ = green;
								break;
							case rle_l8:
								blue = green = red = *data++;
								*pixbuf++ = green;
								break;
							case rle_a1rgb5:
								inraw = data[0] | (data[1]<<8);
								data+=2;
								alphabyte = (inraw&0x8000)?255:0;
								red = ((inraw>>10)&0x1f)<<3;
								green = ((inraw>>5)&0x1f)<<3;
								blue = ((inraw>>0)&0x1f)<<3;

								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							case rle_bgr8:
								blue = *data++;
								green = *data++;
								red = *data++;
								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							case rle_bgra8:
								blue = *data++;
								green = *data++;
								red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							}
							column++;
							if (column==columns)
							{ // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*1;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*1;
							}
						}
					}
				}
			}
		}
		breakOut:;

		return targa_rgba;
	}
	else if ((tgaheader.version == 0x82||tgaheader.version == 0x83) && forceformat && forceformat!=PTI_RGBA16F)
		Con_Printf("HTGA: required output format is not half-float\n");
	else if ((tgaheader.version == 0x82||tgaheader.version == 0x83) && !(tgaheader.bpp&15) && tgaheader.bpp>=16 && tgaheader.bpp<=16*4)
	{	//packed r[g[b[a]]]f
		unsigned short *initbuf, *inrow, *outrow;
		int x, y, mul;

		if (tgaheader.version == 0x83 && tgaheader.bpp==16)
			*format = forceformat = PTI_R16F;
		else
			*format = forceformat = PTI_RGBA16F; //gray+alpha needs to be rgbaf

		initbuf = BZ_Malloc(tgaheader.height*tgaheader.width* ((forceformat==PTI_R16F)?2:8));

		mul = tgaheader.bpp/8;
//flip +convert to 32 bit
		outrow = &initbuf[(int)(0)*tgaheader.width*mul];
		for (y = 0; y < tgaheader.height; y+=1)
		{
			if (flipped)
				inrow = (unsigned short*)&data[(int)(tgaheader.height-y-1)*tgaheader.width*mul];
			else
				inrow = (unsigned short*)&data[(int)(y)*tgaheader.width*mul];

			switch(mul)
			{
			default:	//bug!
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = 0;
					*outrow++ = 0;
					*outrow++ = 0;
					*outrow++ = 0xf<<10;
				}
				break;
			case 2:	//Lum
				if (forceformat == PTI_R16F)
				{
					for (x = 0; x < tgaheader.width; x+=1)
						*outrow++ = *inrow++;
				}
				else
				{
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = *inrow;
						*outrow++ = *inrow;
						*outrow++ = *inrow;
						*outrow++ = 0xf<<10; //1.0
						inrow+=1;
					}
				}
				break;
			case 4:
				if (tgaheader.version == 0x83)
				{	//treat as LumAlpha
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[0];
						*outrow++ = inrow[0];
						*outrow++ = inrow[0];
						*outrow++ = inrow[1];
						inrow+=2;
					}
				}
				else
				{	//RG
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[0];
						*outrow++ = inrow[1];
						*outrow++ = 0;
						*outrow++ = 0xf<<10; //1.0
						inrow+=2;
					}
				}
				break;
			case 6: //BGR
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = inrow[2];
					*outrow++ = inrow[1];
					*outrow++ = inrow[0];
					*outrow++ = 0xf<<10; //1.0
					inrow+=3;
				}
				break;
			case 8: //BGRA16F, swizzle to rgba
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = inrow[2];
					*outrow++ = inrow[1];
					*outrow++ = inrow[0];
					*outrow++ = inrow[3];
					inrow+=4;
				}
				break;
			}
		}
		return initbuf;
	}
	else if (tgaheader.version == 2)
	{	//packed format
		qbyte *initbuf, *inrow, *outrow;
		int x, y, mul;
		qbyte blue, red, green;

		if (tgaheader.bpp == 8)
			return NULL;
		initbuf=BZ_Malloc(tgaheader.height*tgaheader.width* ((forceformat==PTI_L8)?1:4));

		mul = tgaheader.bpp/8;
//flip +convert to 32 bit
		if (forceformat==PTI_L8)
		{
			*format = forceformat;
			outrow = &initbuf[(int)(0)*tgaheader.width];
		}
		else
		{
			outrow = &initbuf[(int)(0)*tgaheader.width*mul];
			*format = (mul==4)?PTI_RGBA8:PTI_RGBX8;
		}
		for (y = 0; y < tgaheader.height; y+=1)
		{
			if (flipped)
				inrow = &data[(int)(tgaheader.height-y-1)*tgaheader.width*mul];
			else
				inrow = &data[(int)(y)*tgaheader.width*mul];

			if (forceformat!=PTI_L8)
			{
				switch(mul)
				{
				case 2:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = ((inrow[1] & 0x7c)>>2) *8;					//red
						*outrow++ = (((inrow[1] & 0x03)<<3) + ((inrow[0] & 0xe0)>>5))*8;	//green
						*outrow++ = (inrow[0] & 0x1f)*8;					//blue
						*outrow++ = (int)(inrow[1]&0x80)*2-1;			//alpha?
						inrow+=2;
					}
					break;
				case 3:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[2];
						*outrow++ = inrow[1];
						*outrow++ = inrow[0];
						*outrow++ = 255;
						inrow+=3;
					}
					break;
				case 4:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[2];
						*outrow++ = inrow[1];
						*outrow++ = inrow[0];
						*outrow++ = inrow[3];
						inrow+=4;
					}
					break;
				}
			}
			else
			{
				switch(mul)
				{
				case 2:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = ((inrow[1] & 0x7c)>>2) *8;					//red
						green = (((inrow[1] & 0x03)<<3) + ((inrow[0] & 0xe0)>>5))*8;	//green
						blue = (inrow[0] & 0x1f)*8;					//blue
//						alphabyte = (int)(inrow[1]&0x80)*2-1;			//alpha?

						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=2;
					}
					break;
				case 3:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = inrow[2];
						green = inrow[1];
						blue = inrow[0];
						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=3;
					}
					break;
				case 4:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = inrow[2];
						green = inrow[1];
						blue = inrow[0];
						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=4;
					}
					break;
				}
			}
		}

		if (forceformat!=PTI_L8)
		{
			for (x = 0; x < tgaheader.width*tgaheader.height*4; x+=4)
			{
				if (initbuf[x+0] != initbuf[x+1] || initbuf[x+0] != initbuf[x+2] || initbuf[x+3] != 0xff)
					break;
			}
			if (x == tgaheader.width*tgaheader.height*4)
			{	//no alpha
				if (forceformat==PTI_INVALID)
					*format = PTI_LLLX8;
			}
			else
			{
				for (; x < tgaheader.width*tgaheader.height*4; x+=4)
				{
					if (initbuf[x+0] != initbuf[x+1] || initbuf[x+0] != initbuf[x+2])
						break;
				}
				if (x == tgaheader.width*tgaheader.height*4)
				{	//okay, there's some alpha data in there.
					if (forceformat==PTI_INVALID)
						*format = PTI_LLLA8;
				}
			}
		}
		return initbuf;
	}
	else
		Con_Printf("TGA: Unsupported version\n");
	return NULL;
}

qboolean WriteTGA(const char *filename, enum fs_relative fsroot, const qbyte *fte_restrict rgb_buffer, qintptr_t bytestride, int width, int height, enum uploadfmt fmt)
{
	qboolean success = false;
	size_t c, i;
	vfsfile_t *vfs;
	int ipx,opx;
	qboolean rgb;
	static const unsigned char footer[26] =
	{	//added in v2, just makes it clear that it is actually a tga
		0,0,0,0,//extension area offset
		0,0,0,0,//developer area offset
		'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E', //the truth is out there
		'.', 0
	};
	unsigned char header[18];
	memset (header, 0, 18);

	if (fmt == PTI_RGBA16F)
	{
		header[2] = 0x82;	//uncompressed RGB half-float
		opx = 8;
		ipx = 8;
		rgb = true;
	}
	else if (fmt == PTI_R16F)
	{
		header[2] = 0x83;	//uncompressed greyscale half-float
		opx = 2;
		ipx = 2;
		rgb = false;
	}
	else
	{
		header[2] = 2;			// uncompressed true-colour type
		if (fmt == PTI_ARGB1555)
		{
			rgb = false;
			ipx = 2;
			opx = 2;
			header[17] |= 1&0xf;	//1bit alpha.
		}
		else if (fmt == PTI_RGBA8 || fmt == PTI_BGRA8)
		{
			rgb = fmt==TF_RGBA32;
			ipx = 4;
			opx = 4;
			header[17] |= 8&0xf;	//alpha is 8bit
		}
		else if (fmt == PTI_RGBX8 || fmt == PTI_BGRX8)
		{
			rgb = fmt==PTI_RGBX8;
			ipx = 4;
			opx = 3;
		}
		else if (fmt == PTI_RGB8 || fmt == PTI_BGR8)
		{
			rgb = fmt==PTI_RGB8;
			ipx = 3;
			opx = 3;
		}
		/*else if (fmt == PTI_RGBA16)
		{
			rgb = true;
			ipx = 8;
			opx = 8;
		}*/
		else if (fmt==PTI_LLLA8)
		{
			rgb = false;
			ipx = 4;
			opx = 2;
			header[2] = 3;			// greyscale
			header[17] |= 8&0xf;			// with alpha
		}
		else if (fmt==PTI_LLLX8)
		{
			rgb = false;
			ipx = 4;
			opx = 1;
			header[2] = 3;			// greyscale
		}
		else if (fmt == PTI_L8)
		{
			rgb = false;
			ipx = 1;
			opx = 1;
			header[2] = 3;			// greyscale
		}
		else if (fmt == PTI_L8A8)
		{
			rgb = false;
			ipx = 2;
			opx = 2;
			header[2] = 3;			// greyscale
			header[17] |= 8&0xf;			//with alpha
		}
		else
			return false;
	}

	FS_CreatePath(filename, fsroot);
	vfs = FS_OpenVFS(filename, "wb", fsroot);
	if (vfs)
	{
		header[12] = width&255;
		header[13] = width>>8;
		header[14] = height&255;
		header[15] = height>>8;
		header[16] = opx*8;		// pixel size
		header[17] |= 0x00;		// flags

		if (bytestride < 0)
		{	//if we're upside down, lets just use an upside down tga.
			rgb_buffer += bytestride*(height-1);
			bytestride = -bytestride;
			//now we can just do everything without worrying about rows
		}
		else	//our data is top-down, set up the header to also be top-down.
			header[17] |= 0x20;

		VFS_WRITE(vfs, header, sizeof(header));
		if (ipx == opx && !rgb)
		{	//can just directly write it
			//bgr24, bgra24
			c = (size_t)width*height*opx;

			VFS_WRITE(vfs, rgb_buffer, c);
		}
		else
		{
			qbyte *fte_restrict rgb_out = malloc((size_t)width*opx*height);

			if (opx == 1)
			{	//L8, LLLX8
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
					rgb_out[i] = rgb_buffer[i*ipx+0];
			}
			else if (opx == 2)
			{	//L8A8, LLLA8
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*2+0] = rgb_buffer[i*ipx+0];
					rgb_out[i*2+1] = rgb_buffer[i*ipx+ipx-1];
				}
			}
			//no need to swap alpha, and if we're just swapping alpha will be fine in-place.
			else if (opx == 8)
			{	//rgba16, rgba16f
				//(output is bgra still)
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*opx+0] = rgb_buffer[i*ipx+4];
					rgb_out[i*opx+1] = rgb_buffer[i*ipx+5];
					rgb_out[i*opx+2] = rgb_buffer[i*ipx+2];
					rgb_out[i*opx+3] = rgb_buffer[i*ipx+3];
					rgb_out[i*opx+4] = rgb_buffer[i*ipx+0];
					rgb_out[i*opx+5] = rgb_buffer[i*ipx+1];
					rgb_out[i*opx+6] = rgb_buffer[i*ipx+6];
					rgb_out[i*opx+7] = rgb_buffer[i*ipx+7];
				}
			}
			else if (opx == 4)
			{	//rgba32, bgra32
				int rc = rgb?0:2;
				int bc = rgb?2:0;
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*4+0] = rgb_buffer[i*ipx+bc];
					rgb_out[i*4+1] = rgb_buffer[i*ipx+1];
					rgb_out[i*4+2] = rgb_buffer[i*ipx+rc];
					rgb_out[i*4+3] = rgb_buffer[i*ipx+3];
				}
			}
			else //if (opx == 3)
			{	//rgba32, bgra32
				int rc = rgb?0:2;
				int bc = rgb?2:0;
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*3+0] = rgb_buffer[i*ipx+bc];
					rgb_out[i*3+1] = rgb_buffer[i*ipx+1];
					rgb_out[i*3+2] = rgb_buffer[i*ipx+rc];
				}
			}
			c *= opx;

			VFS_WRITE(vfs, rgb_out, c);
			free(rgb_out);
		}
		VFS_WRITE(vfs, footer, sizeof(footer));

		success = VFS_CLOSE(vfs);
	}
	return success;
}
#endif

#ifdef AVAIL_PNGLIB
	#ifndef AVAIL_ZLIB
		#error PNGLIB requires ZLIB
	#endif

	#undef channels

	#ifndef PNG_SUCKS_WITH_SETJMP
		#include "png.h"
	#endif

	#ifdef DYNAMIC_LIBPNG
		#define PSTATIC(n)
		static dllhandle_t *libpng_handle;
		#define LIBPNG_LOADED() (libpng_handle != NULL)
	#else
		#define LIBPNG_LOADED() 1
		#define PSTATIC(n) = &n
		#ifdef _MSC_VER
			#ifdef _WIN64
				#pragma comment(lib, MSVCLIBSPATH "libpng64.lib")
			#else
				#pragma comment(lib, MSVCLIBSPATH "libpng.lib")
			#endif
		#endif
	#endif

#ifndef PNG_NORETURN
#define PNG_NORETURN
#endif
#ifndef PNG_ALLOCATED
#define PNG_ALLOCATED
#endif

#if PNG_LIBPNG_VER < 10500
	#define png_const_infop png_infop
	#define png_const_structp png_structp
	#define png_const_bytep png_bytep
	#define png_const_unknown_chunkp png_unknown_chunkp
	#define png_const_textp png_textp
	#define png_const_colorp png_colorp
#endif
#if PNG_LIBPNG_VER < 10600
	#define png_inforp png_infop
	#define png_const_inforp png_const_infop
	#define png_structrp png_structp
	#define png_const_structrp png_const_structp
#endif

static void (PNGAPI *qpng_error) PNGARG((png_const_structrp png_ptr, png_const_charp error_message)) PSTATIC(png_error);
static void (PNGAPI *qpng_read_end) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_end);
static void (PNGAPI *qpng_read_image) PNGARG((png_structp png_ptr, png_bytepp image)) PSTATIC(png_read_image);
static png_byte (PNGAPI *qpng_get_bit_depth) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_bit_depth);
static png_byte (PNGAPI *qpng_get_channels) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_channels);
#if PNG_LIBPNG_VER < 10400
	static png_uint_32 (PNGAPI *qpng_get_rowbytes) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_rowbytes);
#else
	static png_size_t (PNGAPI *qpng_get_rowbytes) PNGARG((png_const_structp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_get_rowbytes);
#endif
static void (PNGAPI *qpng_read_update_info) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_update_info);
static void (PNGAPI *qpng_set_strip_16) PNGARG((png_structp png_ptr)) PSTATIC(png_set_strip_16);
static void (PNGAPI *qpng_set_swap) PNGARG((png_structp png_ptr)) PSTATIC(png_set_swap);
static void (PNGAPI *qpng_set_expand) PNGARG((png_structp png_ptr)) PSTATIC(png_set_expand);
static void (PNGAPI *qpng_set_gray_to_rgb) PNGARG((png_structp png_ptr)) PSTATIC(png_set_gray_to_rgb);
static void (PNGAPI *qpng_set_tRNS_to_alpha) PNGARG((png_structp png_ptr)) PSTATIC(png_set_tRNS_to_alpha);
static png_uint_32 (PNGAPI *qpng_get_valid) PNGARG((png_const_structp png_ptr, png_const_infop info_ptr, png_uint_32 flag)) PSTATIC(png_get_valid);
#if PNG_LIBPNG_VER >= 10400
static void (PNGAPI *qpng_set_expand_gray_1_2_4_to_8) PNGARG((png_structp png_ptr)) PSTATIC(png_set_expand_gray_1_2_4_to_8);
#else
static void (PNGAPI *qpng_set_gray_1_2_4_to_8) PNGARG((png_structp png_ptr)) PSTATIC(png_set_gray_1_2_4_to_8);
#endif
static void (PNGAPI *qpng_set_bgr) PNGARG((png_structp png_ptr)) PSTATIC(png_set_bgr);
static void (PNGAPI *qpng_set_filler) PNGARG((png_structp png_ptr, png_uint_32 filler, int flags)) PSTATIC(png_set_filler);
static void (PNGAPI *qpng_set_palette_to_rgb) PNGARG((png_structp png_ptr)) PSTATIC(png_set_palette_to_rgb);
static png_uint_32 (PNGAPI *qpng_get_IHDR) PNGARG((png_const_structrp png_ptr, png_const_inforp info_ptr, png_uint_32 *width, png_uint_32 *height,
			int *bit_depth, int *color_type, int *interlace_method, int *compression_method, int *filter_method)) PSTATIC(png_get_IHDR);
static png_uint_32 (PNGAPI *qpng_get_PLTE) PNGARG((png_const_structrp png_ptr, png_inforp info_ptr, png_colorp *palette, int *num_palette)) PSTATIC(png_get_PLTE);
static png_uint_32 (PNGAPI *qpng_get_tRNS) PNGARG((png_const_structrp png_ptr, png_inforp info_ptr, png_bytep *trans_alpha, int *num_trans, png_color_16p *trans_color)) PSTATIC(png_get_tRNS);

static void (PNGAPI *qpng_read_info) PNGARG((png_structp png_ptr, png_infop info_ptr)) PSTATIC(png_read_info);
static void (PNGAPI *qpng_set_sig_bytes) PNGARG((png_structp png_ptr, int num_bytes)) PSTATIC(png_set_sig_bytes);
static void (PNGAPI *qpng_set_read_fn) PNGARG((png_structp png_ptr, png_voidp io_ptr, png_rw_ptr read_data_fn)) PSTATIC(png_set_read_fn);
static void (PNGAPI *qpng_destroy_read_struct) PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr)) PSTATIC(png_destroy_read_struct);
static png_infop (PNGAPI *qpng_create_info_struct) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_create_info_struct);
static png_structp (PNGAPI *qpng_create_read_struct) PNGARG((png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)) PSTATIC(png_create_read_struct);
static int (PNGAPI *qpng_sig_cmp) PNGARG((png_const_bytep sig, png_size_t start, png_size_t num_to_check)) PSTATIC(png_sig_cmp);

static void (PNGAPI *qpng_write_end) PNGARG((png_structrp png_ptr, png_inforp info_ptr)) PSTATIC(png_write_end);
static void (PNGAPI *qpng_write_image) PNGARG((png_structrp png_ptr, png_bytepp image)) PSTATIC(png_write_image);
static void (PNGAPI *qpng_write_info) PNGARG((png_structrp png_ptr, png_const_inforp info_ptr)) PSTATIC(png_write_info);
#ifdef PNG_TEXT_SUPPORTED
static void (PNGAPI *qpng_set_text) PNGARG((png_const_structrp png_ptr, png_infop info_ptr, png_const_textp text_ptr, int num_text)) PSTATIC(png_set_text);
#endif
static void (PNGAPI *qpng_set_IHDR) PNGARG((png_const_structrp png_ptr, png_infop info_ptr, png_uint_32 width, png_uint_32 height,
			int bit_depth, int color_type, int interlace_method, int compression_method, int filter_method)) PSTATIC(png_set_IHDR);
//static void png_set_PLTE(void);
static void (PNGAPI *qpng_set_PLTE) PNGARG((png_structrp png_ptr, png_inforp info_ptr, png_const_colorp palette, int num_palette)) PSTATIC(png_set_PLTE);
static void (PNGAPI *qpng_set_compression_level) PNGARG((png_structrp png_ptr, int level)) PSTATIC(png_set_compression_level);
static void (PNGAPI *qpng_init_io) PNGARG((png_structp png_ptr, png_FILE_p fp)) PSTATIC(png_init_io);
static png_voidp (PNGAPI *qpng_get_io_ptr) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_get_io_ptr);
static void (PNGAPI *qpng_destroy_write_struct) PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)) PSTATIC(png_destroy_write_struct);
static png_structp (PNGAPI *qpng_create_write_struct) PNGARG((png_const_charp user_png_ver, png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warn_fn)) PSTATIC(png_create_write_struct);
static void (PNGAPI *qpng_set_unknown_chunks) PNGARG((png_const_structrp png_ptr, png_inforp info_ptr, png_const_unknown_chunkp unknowns, int num_unknowns)) PSTATIC(png_set_unknown_chunks);

static png_voidp (PNGAPI *qpng_get_error_ptr) PNGARG((png_const_structrp png_ptr)) PSTATIC(png_get_error_ptr);

qboolean LibPNG_Init(void)
{
#ifdef DYNAMIC_LIBPNG
	static dllfunction_t pngfuncs[] =
	{
		{(void **) &qpng_error,							"png_error"},
		{(void **) &qpng_read_end,						"png_read_end"},
		{(void **) &qpng_read_image,					"png_read_image"},
		{(void **) &qpng_get_bit_depth,					"png_get_bit_depth"},
		{(void **) &qpng_get_channels,					"png_get_channels"},
		{(void **) &qpng_get_rowbytes,					"png_get_rowbytes"},
		{(void **) &qpng_read_update_info,				"png_read_update_info"},
		{(void **) &qpng_set_strip_16,					"png_set_strip_16"},
		{(void **) &qpng_set_swap,						"png_set_swap"},
		{(void **) &qpng_set_expand,					"png_set_expand"},
		{(void **) &qpng_set_gray_to_rgb,				"png_set_gray_to_rgb"},
		{(void **) &qpng_set_tRNS_to_alpha,				"png_set_tRNS_to_alpha"},
		{(void **) &qpng_get_valid,						"png_get_valid"},
#if PNG_LIBPNG_VER > 10400
		{(void **) &qpng_set_expand_gray_1_2_4_to_8,	"png_set_expand_gray_1_2_4_to_8"},
#else
		{(void **) &qpng_set_gray_1_2_4_to_8,	"png_set_gray_1_2_4_to_8"},
#endif
		{(void **) &qpng_set_bgr,						"png_set_bgr"},
		{(void **) &qpng_set_filler,					"png_set_filler"},
		{(void **) &qpng_set_palette_to_rgb,			"png_set_palette_to_rgb"},
		{(void **) &qpng_get_IHDR,						"png_get_IHDR"},
		{(void **) &qpng_get_PLTE,						"png_get_PLTE"},
		{(void **) &qpng_get_tRNS,						"png_get_tRNS"},
		{(void **) &qpng_read_info,						"png_read_info"},
		{(void **) &qpng_set_sig_bytes,					"png_set_sig_bytes"},
		{(void **) &qpng_set_read_fn,					"png_set_read_fn"},
		{(void **) &qpng_destroy_read_struct,			"png_destroy_read_struct"},
		{(void **) &qpng_create_info_struct,			"png_create_info_struct"},
		{(void **) &qpng_create_read_struct,			"png_create_read_struct"},
		{(void **) &qpng_sig_cmp,						"png_sig_cmp"},

#ifdef PNG_TEXT_SUPPORTED
		{(void **) &qpng_set_text,						"png_set_text"},
#endif
		{(void **) &qpng_write_end,						"png_write_end"},
		{(void **) &qpng_write_image,					"png_write_image"},
		{(void **) &qpng_write_info,					"png_write_info"},
		{(void **) &qpng_set_IHDR,						"png_set_IHDR"},
		{(void **) &qpng_set_PLTE,						"png_set_PLTE"},
		{(void **) &qpng_set_compression_level,			"png_set_compression_level"},
		{(void **) &qpng_init_io,						"png_init_io"},
		{(void **) &qpng_get_io_ptr,					"png_get_io_ptr"},
		{(void **) &qpng_destroy_write_struct,			"png_destroy_write_struct"},
		{(void **) &qpng_create_write_struct,			"png_create_write_struct"},
		{(void **) &qpng_set_unknown_chunks,			"png_set_unknown_chunks"},

		{(void **) &qpng_get_error_ptr,					"png_get_error_ptr"},
		{NULL, NULL}
	};
	static qboolean tried;
	if (!tried)
	{
		tried = true;

		if (!LIBPNG_LOADED())
		{
			char *libnames[] =
			{
			#ifdef _WIN32
				"libpng" STRINGIFY(PNG_LIBPNG_VER_DLLNUM)
			#else
				//linux...
				//lsb uses 'libpng12.so' specifically, so make sure that works.
				"libpng" STRINGIFY(PNG_LIBPNG_VER_MAJOR) STRINGIFY(PNG_LIBPNG_VER_MINOR) ".so." STRINGIFY(PNG_LIBPNG_VER_SONUM),
				"libpng" STRINGIFY(PNG_LIBPNG_VER_MAJOR) STRINGIFY(PNG_LIBPNG_VER_MINOR) ".so",
				"libpng.so." STRINGIFY(PNG_LIBPNG_VER_SONUM)
				"libpng.so",
			#endif
			};
			size_t i;
			for (i = 0; i < countof(libnames); i++)
			{
				if (libnames[i])
				{
					libpng_handle = Sys_LoadLibrary(libnames[i], pngfuncs);
					if (libpng_handle)
						break;
				}
			}
			if (!libpng_handle)
				Con_Printf("Unable to load %s\n", libnames[0]);
		}

//		if (!LIBPNG_LOADED())
//			libpng_handle = Sys_LoadLibrary("libpng", pngfuncs);
	}
#endif
	return LIBPNG_LOADED();
}

typedef struct {
	char *data;
	int readposition;
	int filelen;
} pngreadinfo_t;

static void VARGS readpngdata(png_structp png_ptr,png_bytep data,png_size_t len)
{
	pngreadinfo_t *ri = (pngreadinfo_t*)qpng_get_io_ptr(png_ptr);
	if (ri->readposition+len > ri->filelen)
	{
		qpng_error(png_ptr, "unexpected eof");
		return;
	}
	memcpy(data, &ri->data[ri->readposition], len);
	ri->readposition+=len;
}

struct pngerr
{
	const char *fname;
	jmp_buf jbuf;
};
static void VARGS png_onerror(png_structp png_ptr, png_const_charp error_msg)
{
	struct pngerr *err = qpng_get_error_ptr(png_ptr);
	Con_Printf("libpng %s: %s\n", err->fname, error_msg);
	longjmp(err->jbuf, 1);
	abort();
}

static void VARGS png_onwarning(png_structp png_ptr, png_const_charp warning_msg)
{
	struct pngerr *err = qpng_get_error_ptr(png_ptr);
	Con_DPrintf("libpng %s: %s\n", err->fname, warning_msg);
}

qbyte *ReadPNGFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean force_rgb32)
{
	qbyte header[8], **rowpointers = NULL, *data = NULL;
	png_structp png;
	png_infop pnginfo;
	int y, bitdepth, colortype, interlace, compression, filter, channels;
	unsigned long rowbytes;
	pngreadinfo_t ri;
	png_uint_32 pngwidth, pngheight;
	struct pngerr errctx;

	if (!LibPNG_Init())
	{
		Con_Printf("libpng not loaded\n");
		return NULL;
	}

	memcpy(header, buf, 8);

	errctx.fname = fname;
	if (setjmp(errctx.jbuf))
	{
error:
		if (data)
			BZ_Free(data);
		if (rowpointers)
			BZ_Free(rowpointers);
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		Con_Printf("libpng error\n");
		return NULL;
	}

	if (qpng_sig_cmp(header, 0, 8))
	{
		Con_Printf("libpng signature mismatch\n");	//we already checked the first four bytes so this shouldn't be spammy
		return NULL;
	}

	if (!(png = qpng_create_read_struct(PNG_LIBPNG_VER_STRING, &errctx, png_onerror, png_onwarning)))
	{
		Con_Printf("png_create_read_struct failed\n");	//we already checked the first four bytes so this shouldn't be spammy
		return NULL;
	}

	if (!(pnginfo = qpng_create_info_struct(png)))
	{
		Con_Printf("png_create_info_struct failed\n");	//we already checked the first four bytes so this shouldn't be spammy
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		return NULL;
	}

	ri.data=buf;
	ri.readposition=8;
	ri.filelen=length;
	qpng_set_read_fn(png, &ri, readpngdata);

	qpng_set_sig_bytes(png, 8);
	qpng_read_info(png, pnginfo);
	qpng_get_IHDR(png, pnginfo, &pngwidth, &pngheight, &bitdepth, &colortype, &interlace, &compression, &filter);

	*width = pngwidth;
	*height = pngheight;

	if (colortype == PNG_COLOR_TYPE_PALETTE)
	{
		int numpal = 0, numtrans = 0;
		png_colorp pal = NULL;
		png_bytep trans = NULL;
		if (bitdepth==8 && !force_rgb32)
		{
			qpng_get_tRNS(png, pnginfo, &trans, &numtrans, NULL);
			qpng_get_PLTE(png, pnginfo, &pal, &numpal);
		}
		if (numpal == 256)
		{
			for (numpal = 0; numpal < 256; numpal++)
			{
				if (pal[numpal].red == host_basepal[numpal*3+0] &&
					pal[numpal].green == host_basepal[numpal*3+1] &&
					pal[numpal].blue == host_basepal[numpal*3+2])
				{
					if (numpal < numtrans && trans[numpal] != 255)
						break; //we require it to be fully opaque, because our PTI_P8 has no transparency info(nor does quake) and we don't want to make assumptions here.
					continue;
				}
				else
					break; //bum
			}
		}
		if (numpal != 256)
		{	//the palette isn't ours. give up and just treat it as rgb(a)
			qpng_set_palette_to_rgb(png);
			qpng_set_filler(png, ~0u, PNG_FILLER_AFTER);
			colortype = PNG_COLOR_TYPE_RGB;
		}
	}

	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
	{	//don't handle small greyscale formats
		#if PNG_LIBPNG_VER > 10400
			qpng_set_expand_gray_1_2_4_to_8(png);
		#else
			qpng_set_gray_1_2_4_to_8(png);
		#endif
	}

	if (colortype != PNG_COLOR_TYPE_PALETTE && qpng_get_valid( png, pnginfo, PNG_INFO_tRNS))
	{
		qpng_set_tRNS_to_alpha(png);
		//for these types, the trns lump specifies a specific colour to treat as transparent.
		//make sure our resulting format reflects it.
		if (colortype == PNG_COLOR_TYPE_GRAY)
			colortype = PNG_COLOR_TYPE_GRAY_ALPHA;
		else if (colortype == PNG_COLOR_TYPE_RGB)
			colortype = PNG_COLOR_TYPE_RGB_ALPHA;
	}

	if (bitdepth >= 8 && colortype == PNG_COLOR_TYPE_RGB)
		qpng_set_filler(png, ~0u, PNG_FILLER_AFTER);

	//expand greyscale to rgb when we're forcing 32bit output (with padding, as appropriate).
	if ((colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA)&&force_rgb32)
	{
		qpng_set_gray_to_rgb( png );
		if (colortype == PNG_COLOR_TYPE_GRAY)
			qpng_set_filler(png, ~0u, PNG_FILLER_AFTER);
	}

	if (bitdepth < 8)
		qpng_set_expand (png);
	else if (bitdepth == 16 && (force_rgb32 || colortype!=PNG_COLOR_TYPE_RGB_ALPHA))
		qpng_set_strip_16(png);
	else if (bitdepth == 16)
        qpng_set_swap(png);

	qpng_read_update_info(png, pnginfo);
	rowbytes = qpng_get_rowbytes(png, pnginfo);
	channels = qpng_get_channels(png, pnginfo);
	bitdepth = qpng_get_bit_depth(png, pnginfo);

	if (colortype == PNG_COLOR_TYPE_PALETTE)
		*format = PTI_P8;
	else if (bitdepth == 8 && channels == 1 && !force_rgb32)
		*format = PTI_L8;
	else if (bitdepth == 8 && channels == 2 && !force_rgb32)
		*format = (colortype == PNG_COLOR_TYPE_GRAY_ALPHA)?PTI_L8A8:PTI_RG8;
	else if (bitdepth == 8 && channels == 3 && !force_rgb32)
		*format = PTI_RGB8;
	else if (bitdepth == 8 && channels == 4)
	{
		if (colortype == PNG_COLOR_TYPE_GRAY)
			*format = PTI_LLLX8;
		else if (colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
			*format = PTI_LLLA8;
		else if (colortype == PNG_COLOR_TYPE_RGB)
			*format = PTI_RGBX8;
		else //if (colortype == PNG_COLOR_TYPE_RGB_ALPHA)
			*format = PTI_RGBA8;
	}
	else if (bitdepth == 16 && channels == 4 && !force_rgb32)
		*format = PTI_RGBA16;
	else
	{
		Con_Printf ("Bad PNG color depth and/or bpp (%s)\n", fname);
		qpng_destroy_read_struct(&png, &pnginfo, NULL);
		return NULL;
	}

#if 0//defined(PNG_READ_APNG_SUPPORTED) && !defined(DYNAMIC_LIBPNG)
	if (depth)
		*depth = 1;
	if (depth && png_get_valid(png, pnginfo, PNG_INFO_acTL))	//looks like an apng
	{
		png_uint_32 numframes, numplays;
		png_uint_32 f;
		png_uint_32 framewidth, frameheight, framex, framey;
		png_uint_16 framedelay, framedelaydiv;
		png_byte framedispose, frameblend;
		png_get_acTL(png, pnginfo, &numframes, &numplays);
		data = BZF_Malloc(*height * rowbytes * numframes);
		rowpointers = BZF_Malloc(*height * sizeof(*rowpointers));
		if (!data || !rowpointers || !numframes)
			goto error;
		for (f = 0; f < numframes; f++)
		{
			png_read_frame_head(png, pnginfo);
			png_get_next_frame_fcTL(png, pnginfo, &framewidth, &frameheight, &framex, &framey, &framedelay, &framedelaydiv, &framedispose, &frameblend);

			for (y = 0; y < *height; y++)
				rowpointers[y] = data + f**height*rowbytes + y * rowbytes;

			qpng_read_image(png, rowpointers);
			//FIXME: merge in the preceding frame as appropriate...
		}

		*depth = numframes;
	}
	else
#endif
	{
		data = BZF_Malloc(*height * rowbytes);
		rowpointers = BZF_Malloc(*height * sizeof(*rowpointers));

		if (!data || !rowpointers)
			goto error;

		for (y = 0; y < *height; y++)
			rowpointers[y] = data + y * rowbytes;

		qpng_read_image(png, rowpointers);
	}
	qpng_read_end(png, NULL);

	qpng_destroy_read_struct(&png, &pnginfo, NULL);
	BZ_Free(rowpointers);
	return data;
}



int Image_WritePNG (const char *filename, enum fs_relative fsroot, int compression, void **buffers, int numbuffers, qintptr_t bufferstride, int width, int height, enum uploadfmt fmt, qboolean writemetadata)
{
	char systemname[MAX_OSPATH];	//FIXME: replace with png_set_write_fn
	int i;
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_byte **row_pointers;
	struct pngerr errctx;
	int pxsize;
	int outwidth = width;

	qbyte stereochunk = 0;	//cross-eyed
	png_unknown_chunk unknowns = {"sTER", &stereochunk, sizeof(stereochunk), PNG_HAVE_PLTE};

	int bw,bh,bd,chanbits;
	qboolean havepad, bgr;
	int colourtype;

	switch(fmt)
	{
	case PTI_BGR8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_RGB;
		chanbits = 8;
		bgr = true;
		break;
	case PTI_BGRX8:
		havepad = true;
		colourtype = PNG_COLOR_TYPE_RGB;
		chanbits = 8;
		bgr = true;
		break;
	case PTI_BGRA8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_RGB_ALPHA;
		chanbits = 8;
		bgr = true;
		break;

	case PTI_RGB8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_RGB;
		chanbits = 8;
		bgr = false;
		break;
	case PTI_RGBX8:
	case PTI_LLLX8:
		havepad = true;
		colourtype = PNG_COLOR_TYPE_RGB;
		chanbits = 8;
		bgr = false;
		break;

	case PTI_RGBA16:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_RGB_ALPHA;
		chanbits = 16;
		bgr = false;
		break;

	case PTI_P8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_PALETTE;
		chanbits = 8;
		bgr = false;
		break;
	case PTI_L8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_GRAY;
		chanbits = 8;
		bgr = false;
		break;
	case PTI_L8A8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_GRAY_ALPHA;
		chanbits = 8;
		bgr = false;
		break;

	case PTI_RGBA8:
	case PTI_LLLA8:
		havepad = false;
		colourtype = PNG_COLOR_TYPE_RGB_ALPHA;
		chanbits = 8;
		bgr = false;
		break;
	default:
		return false;
	}
	Image_BlockSizeForEncoding(fmt, &pxsize, &bw, &bh, &bd);

	if (!FS_SystemPath(filename, fsroot, systemname, sizeof(systemname)))
		return false;

	if (numbuffers == 2)
	{
		outwidth = width;
		if (outwidth & 7)	//standard stereo images must be padded to 8 pixels width padding between
			outwidth += 8-(outwidth & 7);
		outwidth += width;
	}
	else	//arrange them all horizontally
		outwidth = width * numbuffers;

	if (!LibPNG_Init())
		return false;

	if (!(fp = fopen (systemname, "wb")))
	{
		FS_CreatePath (systemname, FS_SYSTEM);
		if (!(fp = fopen (systemname, "wb")))
			return false;
	}

	errctx.fname = filename;
	if (setjmp(errctx.jbuf))
	{
err:
		qpng_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}

	if (!(png_ptr = qpng_create_write_struct(PNG_LIBPNG_VER_STRING, &errctx, png_onerror, png_onwarning)))
	{
		fclose(fp);
		return false;
	}

	if (!(info_ptr = qpng_create_info_struct(png_ptr)))
	{
		qpng_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fclose(fp);
		return false;
	}

	qpng_init_io(png_ptr, fp);
	compression = bound(0, compression, 100);

// had to add these when I migrated from libpng 1.4.x to 1.5.x
#ifndef Z_NO_COMPRESSION
#define Z_NO_COMPRESSION			0
#endif
#ifndef Z_BEST_COMPRESSION
#define Z_BEST_COMPRESSION			9
#endif
	qpng_set_compression_level(png_ptr, Z_NO_COMPRESSION + (compression*(Z_BEST_COMPRESSION-Z_NO_COMPRESSION))/100);

	if (bgr)
		qpng_set_bgr(png_ptr);
	qpng_set_IHDR(png_ptr, info_ptr, outwidth, height, chanbits, colourtype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	if (colourtype == PNG_COLOR_TYPE_PALETTE)
	{
		png_color pal[256];
		for (i = 0; i < countof(pal); i++) {
			pal[i].red   = host_basepal[i*3+0];
			pal[i].green = host_basepal[i*3+1];
			pal[i].blue  = host_basepal[i*3+2];
		}
		qpng_set_PLTE(png_ptr, info_ptr, pal, countof(pal));
		//png_set_tRNS
	}

#if defined(PNG_TEXT_SUPPORTED) && defined(PNG_ITXT_COMPRESSION_NONE)
	if (writemetadata)
	{
		char blob[8192];
		png_text pngtext = {PNG_ITXT_COMPRESSION_NONE, "XML:com.adobe.xmp"};
		pngtext.text = blob;
		GenerateXMPData(blob, sizeof(blob), width, height, writemetadata);
		pngtext.itxt_length = strlen(pngtext.text);
		qpng_set_text(png_ptr, info_ptr, &pngtext, 1);
	}
#endif

	if (numbuffers == 2)	//flag it as a standard stereographic image
		qpng_set_unknown_chunks(png_ptr, info_ptr, &unknowns, 1);

	qpng_write_info(png_ptr, info_ptr);
	if (havepad)
		qpng_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

	if (numbuffers == 2)
	{	//standard stereographic png image.
		qbyte *pixels, *left, *right;
		//we need to pack the data into a single image for libpng to use
		row_pointers = Z_Malloc (sizeof(png_byte *) * height + outwidth*height*pxsize);	//must be zeroed, because I'm too lazy to specially deal with padding.
		if (!row_pointers)
			goto err;
		pixels = (qbyte*)row_pointers + height;
		//png requires right then left, which is a bit weird.
		//they're meant to be viewable by going cross-eyed (if needed)
		right = pixels;
		left = right + (outwidth-width)*pxsize;

		for (i = 0; i < height; i++)
		{
			if ((qbyte*)buffers[1])
				memcpy(right + i*outwidth*pxsize, (qbyte*)buffers[1] + i*bufferstride, pxsize * width);
			if ((qbyte*)buffers[0])
				memcpy(left + i*outwidth*pxsize, (qbyte*)buffers[0] + i*bufferstride, pxsize * width);
			row_pointers[i] = pixels + i * outwidth * pxsize;
		}
	}
	else if (numbuffers == 1)
	{
		row_pointers = BZ_Malloc (sizeof(png_byte *) * height);
		if (!row_pointers)
			goto err;
		for (i = 0; i < height; i++)
			row_pointers[i] = (qbyte*)buffers[0] + i * bufferstride;
	}
	else
	{	//pack all images horizontally, because preventing people from doing the whole cross-eyed thing is cool, or something.
		qbyte *pixels;
		int j;
		//we need to pack the data into a single image for libpng to use
		row_pointers = BZ_Malloc (sizeof(png_byte *) * height + outwidth*height*pxsize);
		if (!row_pointers)
			goto err;
		pixels = (qbyte*)row_pointers + height;
		for (i = 0; i < height; i++)
		{
			for (j = 0; j < numbuffers; j++)
			{
				if (buffers[j])
					memcpy(pixels+(width*j + i*outwidth)*pxsize, (qbyte*)buffers[j] + i*bufferstride, pxsize * width);
			}
			row_pointers[i] = pixels + i * outwidth * pxsize;
		}
	}
	qpng_write_image(png_ptr, row_pointers);
	qpng_write_end(png_ptr, info_ptr);
	BZ_Free(row_pointers);
	qpng_destroy_write_struct(&png_ptr, &info_ptr);
	if (0==fclose(fp))
		return true;
	Con_Printf("File error writing %s\n", filename);
	return false;
}


#endif

#ifdef AVAIL_JPEGLIB
#define XMD_H	//fix for mingw

#if defined(_MSC_VER)
	#define JPEG_API VARGS
	#include "jpeglib.h"
	#include "jerror.h"
#else
	#include <jpeglib.h>
	#include <jerror.h>
#endif

#ifdef DYNAMIC_LIBJPEG
	#define JSTATIC(n)
	static dllhandle_t *libjpeg_handle;
	#define LIBJPEG_LOADED() (libjpeg_handle != NULL)
#else
	#ifdef _MSC_VER
		#ifdef _WIN64
			#pragma comment(lib, MSVCLIBSPATH "libjpeg64.lib")
		#else
			#pragma comment(lib, MSVCLIBSPATH "jpeg.lib")
		#endif
	#endif
	#define JSTATIC(n) = &n
	#define LIBJPEG_LOADED() (1)
#endif

#ifndef JPEG_FALSE
#define JPEG_boolean boolean
#endif

#define qjpeg_create_compress(cinfo) \
    qjpeg_CreateCompress((cinfo), JPEG_LIB_VERSION, \
			(size_t) sizeof(struct jpeg_compress_struct))
#define qjpeg_create_decompress(cinfo) \
    qjpeg_CreateDecompress((cinfo), JPEG_LIB_VERSION, \
			  (size_t) sizeof(struct jpeg_decompress_struct))

#ifdef DYNAMIC_LIBJPEG
static boolean (VARGS *qjpeg_resync_to_restart) JPP((j_decompress_ptr cinfo, int desired))									JSTATIC(jpeg_resync_to_restart);
static boolean (VARGS *qjpeg_finish_decompress) JPP((j_decompress_ptr cinfo))												JSTATIC(jpeg_finish_decompress);
static JDIMENSION (VARGS *qjpeg_read_scanlines) JPP((j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines))	JSTATIC(jpeg_read_scanlines);
static boolean (VARGS *qjpeg_start_decompress) JPP((j_decompress_ptr cinfo))												JSTATIC(jpeg_start_decompress);
static int (VARGS *qjpeg_read_header) JPP((j_decompress_ptr cinfo, boolean require_image))									JSTATIC(jpeg_read_header);
static void (VARGS *qjpeg_CreateDecompress) JPP((j_decompress_ptr cinfo, int version, size_t structsize))					JSTATIC(jpeg_CreateDecompress);
static void (VARGS *qjpeg_destroy_decompress) JPP((j_decompress_ptr cinfo))													JSTATIC(jpeg_destroy_decompress);

static struct jpeg_error_mgr * (VARGS *qjpeg_std_error) JPP((struct jpeg_error_mgr * err))									JSTATIC(jpeg_std_error);

static void (VARGS *qjpeg_finish_compress) JPP((j_compress_ptr cinfo))														JSTATIC(jpeg_finish_compress);
static JDIMENSION (VARGS *qjpeg_write_scanlines) JPP((j_compress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION num_lines))	JSTATIC(jpeg_write_scanlines);
static void (VARGS *qjpeg_write_marker) JPP((j_compress_ptr cinfo, int marker, const JOCTET *dataptr, unsigned int datalen))JSTATIC(jpeg_write_marker);
static void (VARGS *qjpeg_start_compress) JPP((j_compress_ptr cinfo, boolean write_all_tables))								JSTATIC(jpeg_start_compress);
static void (VARGS *qjpeg_set_quality) JPP((j_compress_ptr cinfo, int quality, boolean force_baseline))						JSTATIC(jpeg_set_quality);
static void (VARGS *qjpeg_set_defaults) JPP((j_compress_ptr cinfo))															JSTATIC(jpeg_set_defaults);
static void (VARGS *qjpeg_CreateCompress) JPP((j_compress_ptr cinfo, int version, size_t structsize))						JSTATIC(jpeg_CreateCompress);
static void (VARGS *qjpeg_destroy_compress) JPP((j_compress_ptr cinfo))														JSTATIC(jpeg_destroy_compress);
#endif

qboolean LibJPEG_Init(void)
{
	#ifdef DYNAMIC_LIBJPEG
	static dllfunction_t jpegfuncs[] =
	{
		{(void **) &qjpeg_resync_to_restart,		"jpeg_resync_to_restart"},
		{(void **) &qjpeg_finish_decompress,		"jpeg_finish_decompress"},
		{(void **) &qjpeg_read_scanlines,			"jpeg_read_scanlines"},
		{(void **) &qjpeg_start_decompress,			"jpeg_start_decompress"},
		{(void **) &qjpeg_read_header,				"jpeg_read_header"},
		{(void **) &qjpeg_CreateDecompress,			"jpeg_CreateDecompress"},
		{(void **) &qjpeg_destroy_decompress,		"jpeg_destroy_decompress"},

		{(void **) &qjpeg_std_error,				"jpeg_std_error"},

		{(void **) &qjpeg_finish_compress,			"jpeg_finish_compress"},
		{(void **) &qjpeg_write_scanlines,			"jpeg_write_scanlines"},
		{(void **) &qjpeg_write_marker,				"jpeg_write_marker"},
		{(void **) &qjpeg_start_compress,			"jpeg_start_compress"},
		{(void **) &qjpeg_set_quality,				"jpeg_set_quality"},
		{(void **) &qjpeg_set_defaults,				"jpeg_set_defaults"},
		{(void **) &qjpeg_CreateCompress,			"jpeg_CreateCompress"},
		{(void **) &qjpeg_destroy_compress,			"jpeg_destroy_compress"},

		{NULL, NULL}
	};

	if (!LIBJPEG_LOADED())
		libjpeg_handle = Sys_LoadLibrary("libjpeg", jpegfuncs);
#ifndef _WIN32
	if (!LIBJPEG_LOADED())
		libjpeg_handle = Sys_LoadLibrary("libjpeg"ARCH_DL_POSTFIX".8", jpegfuncs);
	if (!LIBJPEG_LOADED())
		libjpeg_handle = Sys_LoadLibrary("libjpeg"ARCH_DL_POSTFIX".62", jpegfuncs);
#endif
	#endif

	if (!LIBJPEG_LOADED())
		Con_Printf("Unable to init libjpeg\n");
	return LIBJPEG_LOADED();
}

/*begin jpeg read*/

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */




/* Expanded data source object for stdio input */

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */

  qbyte * infile;		/* source stream */
  int currentpos;
  int maxlen;
  JOCTET * buffer;		/* start of buffer */
  JPEG_boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */


METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  src->start_of_file = true;
}

METHODDEF(JPEG_boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	my_source_mgr *src = (my_source_mgr*) cinfo->src;
	size_t nbytes;

	nbytes = src->maxlen - src->currentpos;
	if (nbytes > INPUT_BUF_SIZE)
		nbytes = INPUT_BUF_SIZE;
	memcpy(src->buffer, &src->infile[src->currentpos], nbytes);
	src->currentpos+=nbytes;

	if (nbytes <= 0)
	{
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = false;

	return true;
}


METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  my_source_mgr *src = (my_source_mgr*) cinfo->src;

  if (num_bytes > 0) {
    while (num_bytes > (long) src->pub.bytes_in_buffer) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      (void) fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}



METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
}


#undef GLOBAL
#define GLOBAL(x) x

GLOBAL(void)
ftejpeg_mem_src (j_decompress_ptr cinfo, qbyte * infile, int maxlen)
{
  my_source_mgr *src;

  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_source_mgr));
    src = (my_source_mgr*) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  INPUT_BUF_SIZE * sizeof(JOCTET));
  }

  src = (my_source_mgr*) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  #ifdef DYNAMIC_LIBJPEG
  	src->pub.resync_to_restart = qjpeg_resync_to_restart; /* use default method */
  #else
  	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  #endif
  src->pub.term_source = term_source;
  src->infile = infile;
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */

  src->currentpos = 0;
  src->maxlen = maxlen;
}

qbyte *ReadJPEGFile(qbyte *infile, int length, int *width, int *height)
{
	qbyte *mem=NULL, *in, *out;
	int i;

	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	size_t size_stride;		/* physical row width in output buffer */

	memset(&cinfo, 0, sizeof(cinfo));

	if (!LIBJPEG_LOADED())
	{
		Con_DPrintf("libjpeg not available.\n");
		return NULL;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	#ifdef DYNAMIC_LIBJPEG
		cinfo.err = qjpeg_std_error(&jerr.pub);
	#else
		cinfo.err = jpeg_std_error(&jerr.pub);
	#endif
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		// If we get here, the JPEG code has signaled an error.
		Con_DPrintf("libjpeg failed to decode a file.\n");
badjpeg:
		#ifdef DYNAMIC_LIBJPEG
			qjpeg_destroy_decompress(&cinfo);
		#else
			jpeg_destroy_decompress(&cinfo);
		#endif

		if (mem)
			BZ_Free(mem);
		return NULL;
	}
	#ifdef DYNAMIC_LIBJPEG
		qjpeg_create_decompress(&cinfo);
	#else
		jpeg_create_decompress(&cinfo);
	#endif

	ftejpeg_mem_src(&cinfo, infile, length);

	#ifdef DYNAMIC_LIBJPEG
		(void) qjpeg_read_header(&cinfo, true);
	#else
		(void) jpeg_read_header(&cinfo, true);
	#endif

	#ifdef DYNAMIC_LIBJPEG
		(void) qjpeg_start_decompress(&cinfo);
	#else
		(void) jpeg_start_decompress(&cinfo);
	#endif


	if (cinfo.output_components == 0)
	{
		Con_DPrintf("No JPEG Components, not a JPEG.\n");
		goto badjpeg;
	}
	if (cinfo.output_components!=3 && cinfo.output_components != 1)
	{
		Con_DPrintf("Bad number of components in JPEG: '%d', should be '3'.\n",cinfo.output_components);
		goto badjpeg;
	}
	if (cinfo.output_height > ~0u / (cinfo.output_width*4))
	{	//even 64bit processes can suffer when oom.
		Con_Printf("Refusing to load excessively large jpeg of %u * %u.\n",cinfo.output_width, cinfo.output_width);
		goto badjpeg;
	}
	size_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, size_stride, 1);

	out=mem=BZF_Malloc(cinfo.output_height*(size_t)cinfo.output_width*4u);
	if (!mem)
		Con_Printf("Malloc failure on %u * %u jpeg image.\n", cinfo.output_height, cinfo.output_width);
	else
	{
//		memset(out, 0, cinfo.output_height*(size_t)cinfo.output_width*4u);
		if (cinfo.output_components == 1)
		{
			while (cinfo.output_scanline < cinfo.output_height)
			{
				#ifdef DYNAMIC_LIBJPEG
					(void) qjpeg_read_scanlines(&cinfo, buffer, 1);
				#else
					(void) jpeg_read_scanlines(&cinfo, buffer, 1);
				#endif

				in = buffer[0];
				for (i = 0; i < cinfo.output_width; i++)
				{//luminance to rgba
					*out++ = *in;
					*out++ = *in;
					*out++ = *in;
					*out++ = 255;
					in++;
				}
			}
		}
		else
		{
			while (cinfo.output_scanline < cinfo.output_height)
			{
				#ifdef DYNAMIC_LIBJPEG
					(void) qjpeg_read_scanlines(&cinfo, buffer, 1);
				#else
					(void) jpeg_read_scanlines(&cinfo, buffer, 1);
				#endif

				in = buffer[0];
				for (i = 0; i < cinfo.output_width; i++)
				{//rgb to rgba
					*out++ = *in++;
					*out++ = *in++;
					*out++ = *in++;
					*out++ = 255;
				}
			}
		}
	}

	#ifdef DYNAMIC_LIBJPEG
		(void) qjpeg_finish_decompress(&cinfo);
	#else
		(void) jpeg_finish_decompress(&cinfo);
	#endif

	#ifdef DYNAMIC_LIBJPEG
		qjpeg_destroy_decompress(&cinfo);
	#else
		jpeg_destroy_decompress(&cinfo);
	#endif

	*width = cinfo.output_width;
	*height = cinfo.output_height;

	return mem;

}
/*end read*/
/*begin write*/


#ifndef DYNAMIC_LIBJPEG
#define qjpeg_std_error			jpeg_std_error
#define qjpeg_destroy_compress	jpeg_destroy_compress
#define qjpeg_CreateCompress	jpeg_CreateCompress
#define qjpeg_set_defaults		jpeg_set_defaults
#define qjpeg_set_quality		jpeg_set_quality
#define qjpeg_write_marker		jpeg_write_marker
#define qjpeg_start_compress	jpeg_start_compress
#define qjpeg_write_scanlines	jpeg_write_scanlines
#define qjpeg_finish_compress	jpeg_finish_compress
#define qjpeg_destroy_compress	jpeg_destroy_compress
#endif
#define OUTPUT_BUF_SIZE 4096
typedef struct  {
	struct jpeg_error_mgr pub;

	jmp_buf setjmp_buffer;
} jpeg_error_mgr_wrapper;

typedef struct {
	struct jpeg_destination_mgr pub;

	vfsfile_t *vfs;


	JOCTET  buffer[OUTPUT_BUF_SIZE];		/* start of buffer */
} my_destination_mgr;

METHODDEF(void) init_destination (j_compress_ptr cinfo)
{
	my_destination_mgr *dest = (my_destination_mgr*) cinfo->dest;

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}
METHODDEF(JPEG_boolean) empty_output_buffer (j_compress_ptr cinfo)
{
	my_destination_mgr *dest = (my_destination_mgr*) cinfo->dest;

	VFS_WRITE(dest->vfs, dest->buffer, OUTPUT_BUF_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return true;
}
METHODDEF(void) term_destination (j_compress_ptr cinfo)
{
	my_destination_mgr *dest = (my_destination_mgr*) cinfo->dest;

	VFS_WRITE(dest->vfs, dest->buffer, OUTPUT_BUF_SIZE - dest->pub.free_in_buffer);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static void ftejpeg_mem_dest (j_compress_ptr cinfo, vfsfile_t *vfs)
{
	my_destination_mgr *dest;

	if (cinfo->dest == NULL)
	{	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
						(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
						sizeof(my_destination_mgr));
		dest = (my_destination_mgr*) cinfo->dest;
		//    dest->buffer = (JOCTET *)
		//      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
		//				  OUTPUT_BUF_SIZE * sizeof(JOCTET));
	}

	dest = (my_destination_mgr*) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->pub.free_in_buffer = 0; /* forces fill_input_buffer on first read */
	dest->pub.next_output_byte = NULL; /* until buffer loaded */
	dest->vfs = vfs;
}



METHODDEF(void) jpeg_error_exit (j_common_ptr cinfo)
{
	longjmp(((jpeg_error_mgr_wrapper *) cinfo->err)->setjmp_buffer, 1);
}
qboolean screenshotJPEG(char *filename, enum fs_relative fsroot, int compression, qbyte *screendata, qintptr_t stride, int screenwidth, int screenheight, enum uploadfmt fmt, unsigned int writemeta)
{
	vfsfile_t	*outfile;
	jpeg_error_mgr_wrapper jerr;
	struct jpeg_compress_struct cinfo;
	JSAMPROW row_pointer[1];
	qboolean ret = false;

	qbyte *tmpdata = NULL;

	int ic;
	qboolean byteswap;

	switch(fmt)
	{
	case PTI_RGB8:	//yay! nothing to do.
		byteswap = false;
		ic = 3;
		break;
	case PTI_BGR8:
		byteswap = true;
		ic = 3;
		break;
	case PTI_BGRA8:
	case PTI_BGRX8:
		byteswap = true;
		ic = 4;
		break;
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_LLLA8:
	case PTI_LLLX8:
		byteswap = false;
		ic = 4;
		break;
	case PTI_L8:
//	case PTI_L8A8:
		byteswap = false;
		ic = 1;
		break;
	default:	//nope, not happening.
		Con_Printf("screenshotJPEG: image format not supported\n");
		return false;
	}

	if (byteswap || ic == 4)
	{	//jpeg doesn't support many input layouts...
		qbyte *in=screendata;
		qbyte *out=tmpdata=BZ_Malloc(screenwidth*screenheight*3);
		size_t y, x;
		int rc = byteswap?2:0;
		int bc = byteswap?0:2;
		for (y = 0; y < screenheight; y++)
		{
			for (x = 0; x < screenwidth; x++, in+=ic, out+=3)
			{
				out[0] = in[rc];
				out[1] = in[1];
				out[2] = in[bc];
			}
			in-=screenwidth*ic;
			in+=stride;
		}
		ic = 3;
		stride = screenwidth*3;
		screendata = tmpdata;
	}

	if (LIBJPEG_LOADED())
	{
		FS_CreatePath (filename, fsroot);
		if ((outfile = FS_OpenVFS(filename, "wb", fsroot)))
		{
			cinfo.err = qjpeg_std_error(&jerr.pub);
			jerr.pub.error_exit = jpeg_error_exit;
			if (setjmp(jerr.setjmp_buffer))
			{
				qjpeg_destroy_compress(&cinfo);
				VFS_CLOSE(outfile);
				FS_Remove(filename, FS_GAME);
				Con_Printf("Failed to create jpeg\n");
				BZ_Free(tmpdata);
				return false;
			}
			else
			{
				qjpeg_create_compress(&cinfo);

				ftejpeg_mem_dest(&cinfo, outfile);
				cinfo.image_width = screenwidth;
				cinfo.image_height = screenheight;
				cinfo.input_components = ic;
				cinfo.in_color_space = (ic<3)?JCS_GRAYSCALE:JCS_RGB;
				qjpeg_set_defaults(&cinfo);
				qjpeg_set_quality (&cinfo, bound(0, compression, 100), true);
				qjpeg_start_compress(&cinfo, true);

				if (writemeta)
				{
					static const char header[] = "http://ns.adobe.com/xap/1.0/";
					char blob[8192];
					memcpy(blob, header, sizeof(header)); //MUST include the null terminator.
					GenerateXMPData(blob+sizeof(header), sizeof(blob)-sizeof(header), screenwidth, screenheight, writemeta);
					qjpeg_write_marker(&cinfo, JPEG_APP0+1, blob, sizeof(header)+strlen(blob+sizeof(header)));
				}

				while (cinfo.next_scanline < cinfo.image_height)
				{
					*row_pointer = screendata+(cinfo.next_scanline * stride);
					qjpeg_write_scanlines(&cinfo, row_pointer, 1);
				}
				qjpeg_finish_compress(&cinfo);
				qjpeg_destroy_compress(&cinfo);
				ret = true;
			}

			VFS_CLOSE(outfile);
		}
	}
	BZ_Free(tmpdata);
	return ret;
}
#endif

#ifdef IMAGEFMT_PCX
/*
==============
WritePCXfile
==============
*/
qboolean WritePCXfile (const char *filename, enum fs_relative fsroot, qbyte *data, int width, int height,
	int rowbytes, qbyte *palette, qboolean upload) //data is 8bit.
{
	int		i, j, length;
	pcx_t	*pcx;
	qbyte		*pack;
	qboolean ret;

	pcx = BZ_Malloc(width*height*2+1000);
	if (pcx == NULL)
	{
		Con_Printf("WritePCXfile: not enough memory\n");
		return false;
	}

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width-1));
	pcx->ymax = LittleShort((short)(height-1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	Q_memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	Q_memset (pcx->filler,0,sizeof(pcx->filler));

// pack the image
	pack = (qbyte *)(pcx+1);
	for (i=0 ; i<height ; i++)
	{
		for (j=0 ; j<width ; j++)
		{
			if ( (*data & 0xc0) != 0xc0)
				*pack++ = *data++;
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data += rowbytes - width;
	}

// write the palette
	*pack++ = 0x0c;	// palette ID qbyte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;

// write output file
	length = pack - (qbyte *)pcx;

#ifdef HAVE_CLIENT
	if (upload)
	{
		CL_StartUpload((void *)pcx, length);
		ret = true;
	}
	else
#endif
		ret = COM_WriteFile (filename, fsroot, pcx, length);
	BZ_Free(pcx);

	return ret;
}

/*
============
LoadPCX
============
*/
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height)
{
	pcx_t	*pcx;
//	pcx_t pcxbuf;
	qbyte	*palette;
	qbyte	*pix;
	int		x, y;
	int		dataByte, runLength;
	int		count;
	qbyte *data;

	qbyte	*pcx_rgb;

	unsigned short xmin, ymin, swidth, sheight;

//
// parse the PCX file
//

	if (length < sizeof(*pcx))
		return NULL;

	pcx = (pcx_t *)buf;

	xmin = LittleShort(pcx->xmin);
	ymin = LittleShort(pcx->ymin);
	swidth = LittleShort(pcx->xmax)-xmin+1;
	sheight = LittleShort(pcx->ymax)-ymin+1;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->color_planes != 1
		|| swidth >= 1024
		|| sheight >= 1024)
	{
		return NULL;
	}

	*width = swidth;
	*height = sheight;

#ifdef HAVE_CLIENT
	if (r_dodgypcxfiles.value)
		palette = host_basepal;
	else
#endif
		palette = buf + length-768;

	data = (char *)(pcx+1);

	count = (swidth) * (sheight);
	pcx_rgb = BZ_Malloc( count * 4);

	for (y=0 ; y<sheight ; y++)
	{
		pix = pcx_rgb + 4*y*(swidth);
		for (x=0 ; x<swidth ; )
		{
			dataByte = *data;
			data++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (x+runLength>swidth)
				{
					Con_Printf("corrupt pcx\n");
					BZ_Free(pcx_rgb);
					return NULL;
				}
				dataByte = *data;
				data++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = 255;
				if (dataByte == 255)
				{
					pix[0] = 0;	//linear filtering can mean transparent pixel colours are visible. black is a more neutral colour.
					pix[1] = 0;
					pix[2] = 0;
					pix[3] = 0;
				}
				pix += 4;
				x++;
			}
		}
	}

	return pcx_rgb;
}

qbyte *ReadPCXData(qbyte *buf, int length, int width, int height, qbyte *result)
{
	pcx_t	*pcx;
//	pcx_t pcxbuf;
//	qbyte	*palette;
	qbyte	*pix;
	int		x, y;
	int		dataByte, runLength;
//	int		count;
	qbyte *data;

	unsigned short xmin, ymin, swidth, sheight;

//
// parse the PCX file
//

	pcx = (pcx_t *)buf;

	xmin = LittleShort(pcx->xmin);
	ymin = LittleShort(pcx->ymin);
	swidth = LittleShort(pcx->xmax)-xmin+1;
	sheight = LittleShort(pcx->ymax)-ymin+1;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8)
	{
		return NULL;
	}

	if (width != swidth ||
		height > sheight)
	{
		Con_Printf("unsupported pcx size\n");
		return NULL;	//we can't feed the requester with enough info
	}

	data = (char *)(pcx+1);

	for (y=0 ; y<height ; y++)
	{
		pix = result + y*swidth;
		for (x=0 ; x<swidth ; )
		{
			dataByte = *data;
			data++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (x+runLength>swidth)
				{
					Con_Printf("corrupt pcx\n");
					return NULL;
				}
				dataByte = *data;
				data++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				*pix++ = dataByte;
				x++;
			}
		}
	}

	return result;
}

qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out)
{
	pcx_t	*pcx;


//
// parse the PCX file
//

	pcx = (pcx_t *)buf;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| LittleShort(pcx->xmax) >= 1024
		|| LittleShort(pcx->ymax) >= 1024)
	{
		return NULL;
	}

	memcpy(out, (qbyte *)pcx + len - 768, 768);

	return out;
}
#endif

#ifdef IMAGEFMT_BMP
typedef struct bmpheader_s
{
	unsigned int	SizeofBITMAPINFOHEADER;
	signed int		Width;
	signed int		Height;
	unsigned short	Planes;
	unsigned short	BitCount;
	unsigned int	Compression;
	unsigned int	ImageSize;
	signed int		TargetDeviceXRes;
	signed int		TargetDeviceYRes;
	unsigned int	NumofColorIndices;
	unsigned int	NumofImportantColorIndices;
} bmpheader_t;
typedef struct bmpheaderv4_s
{
	unsigned int	RedMask;
	unsigned int	GreenMask;
	unsigned int	BlueMask;
	unsigned int	AlphaMask;
	qbyte			ColourSpace[4];	//"Win " or "sRGB"
	qbyte			ColourSpaceCrap[12*3];
	unsigned int	Gamma[3];
} bmpheaderv4_t;

static qbyte *ReadRawBMPFile(qbyte *buf, int length, int *width, int *height, size_t OffsetofBMPBits)
{
	unsigned int i;
	bmpheader_t h;
	qbyte *data;

	memcpy(&h, buf, sizeof(h));	
	h.SizeofBITMAPINFOHEADER = LittleLong(h.SizeofBITMAPINFOHEADER);
	h.Width = LittleLong(h.Width);
	h.Height = LittleLong(h.Height);
	h.Planes = LittleShort(h.Planes);
	h.BitCount = LittleShort(h.BitCount);
	h.Compression = LittleLong(h.Compression);
	h.ImageSize = LittleLong(h.ImageSize);
	h.TargetDeviceXRes = LittleLong(h.TargetDeviceXRes);
	h.TargetDeviceYRes = LittleLong(h.TargetDeviceYRes);
	h.NumofColorIndices = LittleLong(h.NumofColorIndices);
	h.NumofImportantColorIndices = LittleLong(h.NumofImportantColorIndices);

	if (h.Compression)	//RLE? BITFIELDS (gah)?
		return NULL;

	if (!OffsetofBMPBits)
		h.Height /= 2;	//icons are weird.

	*width = h.Width;
	*height = h.Height;

	if (h.BitCount == 4)	//4 bit
	{
		int x, y;
		unsigned int *data32;
		unsigned int	pal[16];
		if (!h.NumofColorIndices)
			h.NumofColorIndices = (int)pow(2, h.BitCount);
		if (h.NumofColorIndices>16)
			return NULL;
		if (h.Width&1)
			return NULL;

		data = buf;
		data += sizeof(h);

		for (i = 0; i < h.NumofColorIndices; i++)
		{
			pal[i] = data[i*4+2] + (data[i*4+1]<<8) + (data[i*4+0]<<16) + (255u/*data[i*4+3]*/<<24);
		}

		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf = data+h.NumofColorIndices*4;
		data32 = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width/2; x++)
			{
				data32[i++] = pal[buf[x]>>4];
				data32[i++] = pal[buf[x]&15];
			}
			buf += (h.Width+1)>>1;
		}

		if (!OffsetofBMPBits)
		{
			for (y = 0; y < h.Height; y++)
			{
				i = (h.Height-1-y) * (h.Width);
				for (x = 0; x < h.Width; x++)
				{
					if (buf[x>>3]&(1<<(7-(x&7))))
						data32[i] &= 0x00ffffff;
					i++;
				}
				buf += (h.Width+7)>>3;
			}
		}

		return (qbyte *)data32;
	}
	else if (h.BitCount == 8)	//8 bit
	{
		int x, y;
		unsigned int *data32;
		unsigned int	pal[256];
		if (!h.NumofColorIndices)
			h.NumofColorIndices = (int)pow(2, h.BitCount);
		if (h.NumofColorIndices>256)
			return NULL;

		data = buf;
		data += sizeof(h);

		for (i = 0; i < h.NumofColorIndices; i++)
		{
			pal[i] = data[i*4+2] + (data[i*4+1]<<8) + (data[i*4+0]<<16) + (255u/*data[i*4+3]*/<<24);
		}

		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER + h.NumofColorIndices*4;
		data32 = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data32[i] = pal[buf[x]];
				i++;
			}
			//BMP rows are 32-bit aligned.
			buf += (h.Width+3)&~3;
		}

		if (!OffsetofBMPBits)
		{
			for (y = 0; y < h.Height; y++)
			{
				i = (h.Height-1-y) * (h.Width);
				for (x = 0; x < h.Width; x++)
				{
					if (buf[x>>3]&(1<<(7-(x&7))))
						data32[i] &= 0x00ffffff;
					i++;
				}
				buf += (h.Width+7)>>3;
			}
		}

		return (qbyte *)data32;
	}
	else if (h.BitCount == 24)	//24 bit... no 16?
	{
		int x, y;
		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER;
		data = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data[i*4+0] = buf[x*3+2];
				data[i*4+1] = buf[x*3+1];
				data[i*4+2] = buf[x*3+0];
				data[i*4+3] = 255;
				i++;
			}
			buf += h.Width*3;
		}

		return data;
	}
	else if (h.BitCount == 32)
	{
		int x, y;
		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER;
		data = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data[i*4+0] = buf[x*4+2];
				data[i*4+1] = buf[x*4+1];
				data[i*4+2] = buf[x*4+0];
				data[i*4+3] = buf[x*4+3];
				i++;
			}
			buf += h.Width*4;
		}

		return data;
	}
	else
		return NULL;

	return NULL;
}

static qbyte *ReadBMPFile(qbyte *buf, int length, int *width, int *height)
{
	unsigned short Type				= buf[0] | (buf[1]<<8);
	unsigned short Size				= buf[2] | (buf[3]<<8) | (buf[4]<<16) | (buf[5]<<24);
//	unsigned short Reserved1		= buf[6] | (buf[7]<<8);
//	unsigned short Reserved2		= buf[8] | (buf[9]<<8);
	unsigned short OffsetofBMPBits	= buf[10] | (buf[11]<<8) | (buf[12]<<16) | (buf[13]<<24);
	if (Type != ('B'|('M'<<8)))
		return NULL;
	if (Size > length)
		return NULL;	//it got truncated at some point
	return ReadRawBMPFile(buf + 14, length-14, width, height, OffsetofBMPBits - 14);
}

qboolean WriteBMPFile(char *filename, enum fs_relative fsroot, qbyte *in, qintptr_t instride, int width, int height, uploadfmt_t fmt)
{
	int y;
	bmpheader_t h;
	bmpheaderv4_t h4;
	qbyte *data;
	qbyte *out;
	int outstride;
	int bits = 32;
	int extraheadersize = sizeof(h4);
	size_t fsize;
	qboolean success;

	memset(&h4, 0, sizeof(h4));
	h4.ColourSpace[0] = 'W';
	h4.ColourSpace[1] = 'i';
	h4.ColourSpace[2] = 'n';
	h4.ColourSpace[3] = ' ';
	switch(fmt)
	{
	case TF_RGBA32:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0xff000000;
		break;
	case TF_BGRA32:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0xff000000;
		break;
	case TF_RGBX32:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0x00000000;
		break;
	case TF_BGRX32:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0x00000000;
		break;
	case TF_RGB24:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0x00000000;
		bits = 3;
		break;
	case TF_BGR24:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0x00000000;
		bits = 3;
		extraheadersize = 0;
		break;

	default:
		return false;
	}


	outstride = width * (bits/8);
	outstride = (outstride+3)&~3;	//bmp pads rows to a multiple of 4 bytes.

//	h.Size = 14+sizeof(h)+extraheadersize + outstride*height;
//	h.Reserved1 = 0;
//	h.Reserved2 = 0;
//	h.OffsetofBMPBits = 2+sizeof(h)+extraheadersize;	//yes, this is misaligned.
	h.SizeofBITMAPINFOHEADER = (sizeof(h)-12)+extraheadersize;
	h.Width = width;
	h.Height = height;
	h.Planes = 1;
	h.BitCount = bits;
	h.Compression = extraheadersize?3/*BI_BITFIELDS*/:0/*BI_RGB aka BGR...*/;
	h.ImageSize = outstride*height;
	h.TargetDeviceXRes = 2835;//72DPI
	h.TargetDeviceYRes = 2835;
	h.NumofColorIndices = 0;
	h.NumofImportantColorIndices = 0;

	//bmp is bottom-up so flip it now.
	in += instride*(height-1);
	instride *= -1;

	fsize = 14+sizeof(h)+extraheadersize + outstride*height;	//size
	out = data = BZ_Malloc(fsize);
	//Type
	*out++ = 'B';
	*out++ = 'M';
	//Size
	*out++ = fsize&0xff;
	*out++ = (fsize>>8)&0xff;
	*out++ = (fsize>>16)&0xff;
	*out++ = (fsize>>24)&0xff;
	//Reserved1
	y = 0;
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	//Reserved1
	y = 0;
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	//OffsetofBMPBits
	y = 2+sizeof(h)+extraheadersize;	//yes, this is misaligned.
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	*out++ = (y>>16)&0xff;
	*out++ = (y>>24)&0xff;
	//bmpheader
	memcpy(out, &h, sizeof(h));
	out += sizeof(h);
	//v4 header
	memcpy(out, &h4, extraheadersize);
	out += extraheadersize;

	//data
	for (y = 0; y < height; y++)
	{
		memcpy(out, in, width * (bits/8));
		memset(out+width*(bits/8), 0, outstride-width*(bits/8));
		out += outstride;
		in += instride;
	}

	success = COM_WriteFile(filename, fsroot, data, fsize);
	BZ_Free(data);

	return success;
}

static qbyte *ReadICOFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *fmt)
{
	qbyte *ret;
	size_t imgcount = buf[4] | (buf[5]<<8);
	struct
	{
		qbyte bWidth;
		qbyte bHeight;
		qbyte bColorCount;
		qbyte bReserved;
		unsigned short wPlanes;
		unsigned short wBitCount;
		unsigned short dwSize_low;
		unsigned short dwSize_high;
		unsigned short dwOffset_low;
		unsigned short dwOffset_high;
	} *img = (void*)(buf+6), *bestimg = NULL;
	size_t bestpixels = 0;
	size_t bestdepth = 0;

	//always favour the png first
	for (imgcount = buf[4] | (buf[5]<<8), img = (void*)(buf+6); imgcount-->0; img++)
	{
		size_t cc = img->wBitCount;
		size_t px = (img->bWidth?img->bWidth:256) * (img->bHeight?img->bHeight:256);
		if (!cc)	//if that was omitted, try and guess it based on raw image size. this is an over estimate.
			cc = 8 * (img->dwSize_low | (img->dwSize_high<<16)) / px;

		if (!bestimg || cc > bestdepth || (cc == bestdepth && px > bestpixels))
		{
			bestimg = img;
			bestdepth = cc;
			bestpixels = px;
		}
	}

	if (bestimg)
	{
		qbyte *indata = buf + (bestimg->dwOffset_low | (bestimg->dwOffset_high<<16));
		size_t insize = (bestimg->dwSize_low | (bestimg->dwSize_high<<16));
#ifdef AVAIL_PNGLIB
		if (insize > 4 && (indata[0] == 137 && indata[1] == 'P' && indata[2] == 'N' && indata[3] == 'G') && (ret = ReadPNGFile(fname, indata, insize, width, height, fmt, true)))
		{
			TRACE(("dbg: Read32BitImageFile: icon png\n"));
			return ret;
		}
		else
#endif
		if ((ret = ReadRawBMPFile(indata, insize, width, height, 0)))
		{
			if (fmt)
				*fmt = PTI_RGBA8;
			TRACE(("dbg: Read32BitImageFile: icon bmp\n"));
			return ret;
		}
	}

	return NULL;
}
#endif

#ifdef IMAGEFMT_PBM
static int PBM_ParseNum(qbyte **buf, qbyte *end)
{
	qbyte token[256];
	size_t l;
	while (*buf < end)
	{
		if (**buf <= ' ')
			(*buf)++;
		else if (**buf == '#')
		{
			while (*buf < end && **buf != '\n' && **buf != '\r')
				(*buf)++;
		}
		else
			break;
	}

	for (l = 0; *buf < end && **buf > ' ';)
		if (l < countof(token))
			token[l++] = *(*buf)++;
	token[l] = 0;
	return strtol(token, NULL, 0);
}
static qbyte *ReadPBMFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{	//this isn't expected to be fast.
	qbyte *end = buf+len;
	int maxval = *width = *height = 0;
	qbyte *r, *bo;
	unsigned short *so;
	size_t l, x, y;
	float m, *fo, *fi;
	int c = buf[1];
	buf+=2;
	switch(c)
	{
	case '7': //arbitrary
		//WIDTH HEIGHT DEPTH MAXVAL TUPLTYPE ENDHDR
		return NULL;
	case '6':	//raw ppm
	case '3':	//plain ppm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);
		maxval = PBM_ParseNum(&buf, end);

		if (maxval > 255)
		{
			r = BZ_Malloc(*width**height*8);
			for(y = 0; y < *height; y++)
			for(x = 0, so=(unsigned short*)r+(*height-y-1)*4**width; x < *height; x++)
			{
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = 65535u;
			}
			*format = PTI_RGBA16;
		}
		else
		{
			r = BZ_Malloc(*width**height*4);
			for(y = 0; y < *height; y++)
			for(x = 0, bo=(qbyte*)r+(*height-y-1)*4**width; x < *height; x++)
			{
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = 255u;
			}
			*format = PTI_RGBA8;
		}
		return r;
	case '5':	//raw pgm
	case '2':	//plain pgm
	case '4':	//raw pbm
	case '1':	//plain pbm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);

		if (c == '4' || c == '1')
			maxval = 1;
		else
			maxval = PBM_ParseNum(&buf, end);

		l = (size_t)*width*(size_t)*height;
		if (maxval > 255)
		{
			r = BZ_Malloc(*width**height*sizeof(*so));
			for(y = 0; y < *height; y++)
			for(x = 0, so=(unsigned short*)r+(*height-y-1)**width; x < *height; x++)
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
			*format = PTI_R16;
		}
		else
		{
			r = BZ_Malloc(*width**height*sizeof(*bo));
			for(y = 0; y < *height; y++)
			for(x = 0, bo=(qbyte*)r+(*height-y-1)**width; x < *height; x++)
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
			*format = PTI_R8;
		}
		return r;

	case 'F':	//rgb pfm
	case 'f':	//grey pfm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);
		m = PBM_ParseNum(&buf, end);

		if (*buf == '\n')
			buf++;
		fi = (float*)buf;

		l = (size_t)*width*(size_t)*height;
		if ((qbyte*)(fi+l*((c=='F')?3:1)) != end)
			return NULL;
		r = BZ_Malloc(l*sizeof(float) * ((c=='F')?4:1));
		if (c == 'F')
		{
			if (m < 0)
			{
				r = BZ_Malloc(*width**height*4*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)*4**width; x < *height; x++)
				{
					*fo++ = LittleFloat(*fi++);
					*fo++ = LittleFloat(*fi++);
					*fo++ = LittleFloat(*fi++);
					*fo++ = 1;
				}
			}
			else
			{
				r = BZ_Malloc(*width**height*4*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)*4**width; x < *height; x++)
				{
					*fo++ = BigFloat(*fi++);
					*fo++ = BigFloat(*fi++);
					*fo++ = BigFloat(*fi++);
					*fo++ = 1;
				}
			}
			*format = PTI_RGBA32F;
		}
		else
		{
			r = BZ_Malloc(l*sizeof(float));
			if (m < 0)
			{
				r = BZ_Malloc(*width**height*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)**width; x < *height; x++)
					*fo++ = LittleFloat(*fi++);
			}
			else
			{
				r = BZ_Malloc(*width**height*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)**width; x < *height; x++)
					*fo++ = BigFloat(*fi++);
			}
			*format = PTI_R32F;
		}
		return r; //erk?
	}
	return NULL;
}
#endif
#ifdef IMAGEFMT_HDR			//baselayer only.
//Radiance files are some weird BGRE8 hdr format that somehow managed to get reasonably well supported
static void *ReadRadianceFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{	//this isn't expected to be fast.
	qbyte *end = buf+len;
	size_t l, x, y, w, h, e;
	float *r, *o, m;
	qbyte rgbe[4];

	char fmt[128];
	char line[256];
	byte_vec4_t *row = NULL;
	w = h = 0;
	*fmt = 0;
	while (buf < end)
	{
		l = 0;
		while(buf < end && *buf != '\n')
		{
			if (*buf == '\r' && buf[1] == '\n')
				continue;
			if (l < countof(line)-1)
				line[l++] = *buf++;
		}
		line[l] = 0;
		buf++;
		if (!strncmp(line, "FORMAT=", 7))
			Q_strncpyz(fmt, line+7, sizeof(fmt));
		if (!l)
			break;
	}
	if (strncmp(buf, "-Y ", 3))
	{
		Con_Printf("%s uses unsupported orientation\n", fname);
		return NULL;
	}
	h = strtol(buf+3, (char**)&buf, 0);
	if (strncmp(buf, " +X ", 4))
	{
		Con_Printf("%s uses unsupported orientation\n", fname);
		return NULL;
	}
	w = strtol(buf+4, (char**)&buf, 0);
	if (*buf == '\r')
		buf++;
	if (*buf++ != '\n')
		return NULL;

	if (strcmp(fmt, "32-bit_rle_rgbe"))
	{
		Con_Printf("%s uses unsupported pixel format (%s)\n", fname, fmt);
		return NULL;
	}

	r = o = BZ_Malloc(sizeof(float)*4*w*h);
	if (!r)
		return NULL;
	for (y=0; y < h && buf+4<end; y++)
	{
		if (buf[0] == 2 && buf[1] == 2 && buf[2] < 127)
		{ //'new' rle logic
			int c, v;
			buf+=4;
			if (!row)
				row = BZ_Malloc(w*sizeof(*row));
			//encoded separately
			for (c = 0; c < 4; c++)
			{
				for (x=0; x < w; )
				{
					if (buf+1 >= end)
						goto fail;
					if (*buf > 128)
					{
						e = x + (*buf++ - 128);
						v = *buf++;
						if (e == x || e > w)
							goto fail;
						while (x < e)
							row[x++][c] = v;
					}
					else
					{
						e = x + *buf++;
						if (e == x || e > w || buf+(e-x) >= end)
							goto fail;
						while (x < e)
							row[x++][c] = *buf++;
					}
				}
			}

			//decompressed the entire line, can make sense of it now.
			for (x=0; x < w; x++)
			{
				m = ldexp(1,row[x][3]-136);
				*o++ = m * row[x][0];
				*o++ = m * row[x][1];
				*o++ = m * row[x][2];
				*o++ = 1;
			}
		}
		else if (buf[0] == 1 && buf[1] == 1 && buf[2] == 1)
		{ //old rle logic
			Con_Printf("%s uses unsupported (old) RLE compression\n", fname);
			goto fail;
		}
		else
		{
			for (x=0; x < w && buf+4<end ; x++)
			{
				rgbe[0] = *buf++;
				rgbe[1] = *buf++;
				rgbe[2] = *buf++;
				rgbe[3] = *buf++;

				m = ldexp(1,rgbe[3]-136);
				*o++ = m * rgbe[0];
				*o++ = m * rgbe[1];
				*o++ = m * rgbe[2];
				*o++ = 1;
			}
		}
	}
	if (row)
		BZ_Free(row);
	*width = w;
	*height = h;
	//FIXME: should probably convert to e5bgr9 or something.
	*format = PTI_RGBA32F;
	return r;
fail:
	if (row)
		BZ_Free(row);
	BZ_Free(r);
	return NULL;
}
#endif

#ifdef IMAGEFMT_PSD			//baselayer only.
// https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/
struct psdctx_s
{
	qbyte *buf;
	qbyte *end;
};
static qbyte PSD_Byte(struct psdctx_s *ctx)
{
	if (ctx->buf == ctx->end)
		return 0;
	return *ctx->buf++;
}
static unsigned short PSD_UShort(struct psdctx_s *ctx)
{
	return (PSD_Byte(ctx)<<8)|PSD_Byte(ctx);
}
static unsigned int PSD_UInt(struct psdctx_s *ctx)
{
	return (PSD_Byte(ctx)<<24)|(PSD_Byte(ctx)<<16)|(PSD_Byte(ctx)<<8)|PSD_Byte(ctx);
}
static void *PSD_Block(struct psdctx_s *ctx, size_t sz)
{
	void *r;
	if (ctx->buf+sz <= ctx->end)
	{
		r = ctx->buf;
		ctx->buf += sz;
	}
	else
		r = NULL;
	return r;
}
static void *ReadPSDFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat)
{
	unsigned short ver, chans, depth, clrmode, cmp;
	unsigned int width, height, clrsize, ressize, lyrsize;
	struct psdctx_s ctx;
	size_t l, c, y;
	ctx.buf = buf;
	ctx.end = buf+len;

	/*magic	=*/ PSD_UInt(&ctx);
	ver = PSD_UShort(&ctx);
	if (ver != 1)
	{
		Con_Printf("%s unsupported .psd version\n", fname);
		return NULL;
	}
	/*reserved*/PSD_Block(&ctx, 6);
	chans   = PSD_UShort(&ctx);
	width   = PSD_UInt(&ctx);
	height  = PSD_UInt(&ctx);
	depth   = PSD_UShort(&ctx);
	clrmode = PSD_UShort(&ctx);
	clrsize = PSD_UInt(&ctx);
	/*palette =*/ PSD_Block(&ctx, clrsize);
	ressize = PSD_UInt(&ctx);
	/*resdata =*/ PSD_Block(&ctx, ressize);
	lyrsize = PSD_UInt(&ctx);
	/*lyrdata =*/ PSD_Block(&ctx, lyrsize);
	cmp		= PSD_UShort(&ctx);

	if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
	{
		Con_Printf("%s too large dimensions (%u * %u)\n", fname, width, height);
		return NULL;
	}
	if (chans <= 0)
	{
		Con_Printf("%s has no colour channels\n", fname);
		return NULL;
	}
	if (clrmode == 3 && (depth == 8 || depth == 16)) //RGB
		;
	else
	{
		Con_Printf("%s not 8 or 16bpp RGB .psd image\n", fname);
		return NULL;
	}

	//the data is planer
	if (cmp == 0 || (cmp == 1 && depth==8))
	{
		if (cmp)
			PSD_Block(&ctx, 2*chans*height); //2 byte run size per plane*scanline
		if (depth == 16)
		{
			unsigned short *r, *o;
			r = o = BZ_Malloc(sizeof(*o)*4*width*height);
			for(c = 0; c < 4; c++)
			{
				o = r+c;
				if (c < chans)
				{
					for(l = 0; l < width*height; l++, o+=4)
						*o = PSD_UShort(&ctx);
				}
				else
				{	//pad colour to 0, alpha 1
					for(l = 0; l < width*height; l++, o+=4)
						*o = (c==3)?0xffff:0;
				}
			}
			*outwidth = width;
			*outheight = height;
			*outformat = PTI_RGBA16;
			return r;
		}
		else if (depth == 8)
		{
			qbyte *r, *o;
			r = o = BZ_Malloc(sizeof(*o)*4*width*height);
			for(c = 0; c < 4; c++)
			{
				o = r+c;
				if (c < chans)
				{
					if (cmp == 1)
					{
						for(y = 0; y < height; y++)
						{
							for(l = 0; l < width; )
							{
								qbyte run = PSD_Byte(&ctx), val;
								if (run < 128)
								{	//copy
									run++;
									for (; l < width && run --> 0; l++, o+=4)
										*o = PSD_Byte(&ctx);
								}
								else
								{
									run = 1-(char)run;
									val = PSD_Byte(&ctx);
									for (; l < width && run --> 0; l++, o+=4)
										*o = val;
								}
							}
						}
					}
					else for(l = 0; l < width*height; l++, o+=4)
						*o = PSD_Byte(&ctx);
				}
				else
				{	//pad colour to 0, alpha 1
					for(l = 0; l < width*height; l++, o+=4)
						*o = (c==3)?0xff:0;
				}
			}
			*outwidth = width;
			*outheight = height;
			if (chans >= 4)
				*outformat = PTI_RGBA8;
			else
				*outformat = PTI_RGBX8;
			return r;
		}
	}
	else if (cmp == 1)
	{
	}
	Con_Printf("%s unsupported compression type (%u)\n", fname, cmp);
	return NULL;
}
#endif

#ifdef IMAGEFMT_XCF
struct xcf_s
{
	const qbyte *filestart;
	qofs_t filesize;
	qofs_t offset;

	unsigned int version;
	qbyte compression;
	size_t width;
	size_t height;
	int basetype, precision;

	enum uploadfmt outformat;
	size_t numlayers;
	size_t *layeroffsets;
	qbyte *flat;
};
static size_t XCF_ReadData(struct xcf_s *f, qbyte *out, size_t len)
{
	size_t avail;
	if (f->offset >= f->filesize)
		avail = 0;
	else
		avail = f->filesize - f->offset;
	if (len > avail)
		memset(out+avail, 0, len-avail);
	else
		avail = len;
	memcpy(out, f->filestart+f->offset, avail);
	f->offset += avail;
	return avail;
}
static unsigned char XCF_ReadByte(struct xcf_s *f)
{
	if (f->offset >= f->filesize)
		return 0;
	return f->filestart[f->offset++];
}
static unsigned int XCF_Read32(struct xcf_s *f)
{	//XCF is natively big-endian.
	qbyte w[4];
	XCF_ReadData(f, w, sizeof(w));
	return (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
}
static float XCF_ReadFloat(struct xcf_s *f)
{	//XCF is natively big-endian.
	union
	{
		unsigned int u;
		float f;
	} u;
	qbyte w[4];
	XCF_ReadData(f, w, sizeof(w));
	u.u = (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
	return u.f;
}
static qofs_t XCF_ReadOffset(struct xcf_s *f)
{	//XCF is natively big-endian.
	qbyte w[8];
	size_t h, l;
	if (f->version >= 11)
	{
		XCF_ReadData(f, w, sizeof(w));
		h = (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
		l = (w[4]<<24)|(w[5]<<16)|(w[6]<<8)|(w[7]<<0);
		return qofs_Make(l, h);
	}
	return XCF_Read32(f);
}
static size_t XCF_ReadString(struct xcf_s *f, char *out, size_t outsize)
{
	size_t len = XCF_Read32(f);
	if (!len)
		*out = 0;
	else
	{
		if (outsize > len)
			outsize = len;
		outsize--;
		XCF_ReadData(f, out, outsize);
		out[outsize]=0;
		f->offset += len-outsize;
	}
	return len;
}
static void XCF_ReadHeaderProperties(struct xcf_s *f)
{	//XCF is natively big-endian.
	unsigned int proptype, propsize;

	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 17)	//compression
			XCF_ReadData(f, &f->compression,1);
		else if (proptype == 19)	//prop_resolution
			f->offset += propsize;
		else if (proptype == 20)	//tatoo
			f->offset += propsize;
		else if (proptype == 21)	//parasites
			f->offset += propsize;
		else if (proptype == 22)	//prop_unit
			f->offset += propsize;
		else
		{
			Con_Printf("Unknown image property %i\n", proptype);
			f->offset += propsize;
		}
	}
}
static qboolean XCF_ReadTile(struct xcf_s *f, qbyte *out, int pxsize, int bytestride, int bytew, int h)
{
	if (f->compression==0)
	{
		while(h --> 0)
		{
			XCF_ReadData(f, out, bytew);
			out += bytestride;
		}
		return true;
	}
	else if (f->compression == 1)
	{
		int runsize;
		int runtype;
		int runval;
		int rowbyte;
		int c = 0, y;
		qbyte *outdata = out;
		//data is ordered by channels
nextchan:
		if (c == pxsize)
			return true;
		out = outdata+c;
		rowbyte = 0;
		c++;
		for(y=0;;)
		{
			runsize = XCF_ReadByte(f);
			if (runsize <= 127)
			{	//compressed run
				if (runsize == 127)
				{
					runsize = XCF_ReadByte(f)<<8;
					runsize |= XCF_ReadByte(f);
				}
				else
					runsize++;
				runval = XCF_ReadByte(f);
				runtype = 0;
			}
			else
			{	//unpacked run
				if (runsize == 128)
				{
					runsize = XCF_ReadByte(f)<<8;
					runsize |= XCF_ReadByte(f);
				}
				else
					runsize = 256-runsize;
				runval = -1;
				runtype = 1;
			}
			if (!runsize)
				return false;	//buggy input

			while(runsize --> 0)
			{
				if (runtype)
					runval = XCF_ReadByte(f);
				out[rowbyte] = runval;
				rowbyte+=pxsize;

				if (rowbyte == bytew)
				{	//reached the end of the row, move on to the next
					if (++y == h)
					{
						if (runsize)
							return false;	//runsize was too long...
						goto nextchan; //reached the end of the tile.
					}
					rowbyte = 0;
					out += bytestride;
				}
			}
		}
	}
	else
		return false;
}
struct xcf_heirachy_s
{
	quint32_t width;
	quint32_t height;
	quint32_t bpp;
	qbyte	*data;
};
static struct xcf_heirachy_s XCF_ReadHeirachy(struct xcf_s *f)
{
	struct xcf_heirachy_s ctx;
	quint32_t x, y, lw, lh;
	qofs_t ofs, tofs;
	if (!f->offset)
	{
		memset(&ctx, 0, sizeof(ctx));
		return ctx;
	}
	ctx.width = XCF_Read32(f);
	ctx.height = XCF_Read32(f);
	ctx.bpp = XCF_Read32(f);
	ctx.data = NULL;

	ofs = XCF_ReadOffset(f);
	while (XCF_ReadOffset(f))
		;	//we don't care about these dummy offsets. we could use them for mipmaps but we don't expect them to be valid or bug-free

	f->offset = ofs;	//jump to level 0
	lw = XCF_Read32(f);
	lh = XCF_Read32(f);
	if (lw == ctx.width && lh == ctx.height)
	{
		ctx.data = BZ_Malloc(ctx.width*(size_t)ctx.bpp*ctx.height);

		for (y = 0; y < ctx.height; y+=lh)
		{
			lh = min(64, ctx.height-y);
			for (x = 0; x < ctx.width; x+=lw)
			{
				lw = min(64, ctx.width-x);
				tofs = XCF_ReadOffset(f);

				ofs = f->offset;
				f->offset = tofs;
				if (!XCF_ReadTile(f, ctx.data + x*ctx.bpp + y*ctx.width*ctx.bpp, ctx.bpp, ctx.width*ctx.bpp, lw*ctx.bpp, lh))
				{
					BZ_Free(ctx.data);
					ctx.data = NULL;
					return ctx;
				}
				f->offset = ofs;
			}
		}
	}
	return ctx;
}
static struct xcf_heirachy_s XCF_ReadChannel(struct xcf_s *f)
{
	struct xcf_heirachy_s h;
	char name[1024];
	quint32_t width = XCF_Read32(f);
	quint32_t height = XCF_Read32(f);
	unsigned int proptype, propsize;
	XCF_ReadString(f, name, sizeof(name));
	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 3) //prop_active_channel
			; //ui state
		else if (proptype == 16) //prop_colour
			f->offset += 3; //ui state
		else if (proptype == 38) //prop_colour
			f->offset += 3*4; //ui state
		else if (proptype == 4) //prop_selection
			; //ui state
		else if (proptype == 14) //prop_showmask
			f->offset += 4; //ui state
		else
		{
			Con_DPrintf("Unknown channel property %i\n", proptype);
			f->offset += propsize;
		}
	}
	f->offset = XCF_ReadOffset(f);
	h = XCF_ReadHeirachy(f);
	if (h.width != width || h.height != height)
	{
		BZ_Free(h.data);
		h.data = NULL;
	}
	return h;
}

static float XCF_BigFloat(void *in)
{
	union
	{
		qbyte b[4];
		float f;
	} u;
	u.b[0] = ((qbyte*)in)[3];
	u.b[1] = ((qbyte*)in)[2];
	u.b[2] = ((qbyte*)in)[1];
	u.b[3] = ((qbyte*)in)[0];
	return u.f;
}
static float XCF_BigShort(void *in)
{
	return (((qbyte*)in)[0]<<8)|(((qbyte*)in)[1]<<0);
}
static float XCF_HalfFloat(void *in)
{
	return HalfToFloat(XCF_BigShort(in));
}

static qboolean XCF_CombineLayer(struct xcf_s *f)
{
	struct xcf_heirachy_s h, l={0};
	unsigned int proptype, propsize;
	size_t width, height, type, heirachyoffset, layermaskoffset;
	char name[1024];
	quint32_t applylayermask = false;
	qint32_t blendmode = 0;
	qboolean unsupported = false;
	quint32_t x,y, ofsx=0,ofsy=0;
	quint32_t visible = true;
	float opacity = 1;
	width = XCF_Read32(f);
	height = XCF_Read32(f);
	type = XCF_Read32(f);
	XCF_ReadString(f, name, sizeof(name));

	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 2) //prop_active_layer
			; //ui state
		else if (proptype == 11) //prop_apply_mask
			applylayermask = XCF_Read32(f);
		else if (proptype == 35) //prop_composite_mode
			/*compositemode =*/ XCF_Read32(f);
		else if (proptype == 36) //prop_composite_space
			/*colourspace = */ XCF_Read32(f);
		else if (proptype == 12) //prop_edit_mask
			/*editmask = */ XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 5) //prop_floating_selection
		{	//just ignore this layer.
			//FIXME: the *other* layer needs to composite differently
			return true;
			XCF_ReadOffset(f);
		}
		else if (proptype == 6) //prop_opacity
			opacity = XCF_Read32(f)/255.0;
		else if (proptype == 33) //prop_float_opacity
			opacity = XCF_ReadFloat(f);
		else if (proptype == 34) //prop_colour_tag
			/*tag = */XCF_Read32(f);
		else if (proptype == 32) //prop_lock_content
			/*lockcontent = */XCF_Read32(f);
		else if (proptype == 29) //prop_group_item
		{
			Con_DPrintf("Unsupported layer property: prop_group_item\n");
			visible = false;
//			unsupported = true; //panic
		}
		else if (proptype == 30) //prop_item_path
		{
			Con_DPrintf("Unsupported layer property: prop_item_path\n");
			visible = false;
//			unsupported = true;
			while(XCF_ReadOffset(f))
				;
		}
		else if (proptype == 31) //prop_group_item_flags
			XCF_Read32(f);
		else if (proptype == 9) //prop_linked
			/*layerislinked = */ XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 10) //prop_lock_alpha
			XCF_Read32(f); //ui state
		else if (proptype == 7) //prop_mode
			blendmode = XCF_Read32(f);
		else if (proptype == 8) //prop_visible
			visible = XCF_Read32(f);
		else if (proptype == 15) //prop_offsets
		{
			ofsx = XCF_Read32(f);
			ofsy = XCF_Read32(f);
		}
		else if (proptype == 13) //prop_show_mask
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 26) //prop_text_layer_flags
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 28) //prop_lock_content
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 20) //prop_tattoo
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 37) //prop_blend_space
			/*blendspace = */XCF_Read32(f);
		else
		{
			Con_DPrintf("Unknown layer(\"%s\") property %i\n", name, proptype);
			f->offset += propsize;
		}
	}

	heirachyoffset = XCF_ReadOffset(f);
	layermaskoffset = XCF_ReadOffset(f);

	if (unsupported || !visible)
		heirachyoffset = layermaskoffset = 0;

	f->offset = heirachyoffset;
	h = XCF_ReadHeirachy(f);

	if (applylayermask)
	{	//blend the alpha...?
		f->offset = layermaskoffset;
		l = XCF_ReadChannel(f);
		if (!l.data || l.bpp!=1 || l.width != width || l.height != height)
			applylayermask = false; //erk?
	}

	if (!h.data)
	{	//its valid to have just a layermask...
		h.bpp = 0; //don't try reading anything.
		h.width = width;
		h.height = height;
	}
	else if (h.width != width || h.height != height)
		unsupported = true;	//level0 must match the layer's size

	if (unsupported || !visible)
		;
	else
	{
		width = min(h.width, f->width-ofsx);
		height = min(h.height, f->height-ofsy);
		if (ofsx < f->width && ofsy < f->height)
		{
			qbyte *in=h.data;
			vec4_t px;
			float sa, da, k;
			if (f->outformat == PTI_RGBA32F)
			{
				float *out = (float*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 16: Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+4), XCF_BigFloat(in+8), XCF_BigFloat(in+12)); break;
						case 12: Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+4), XCF_BigFloat(in+8), 1); break;
						case 8:	Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+4)); break;
						case 4:	Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+0), 1); break;
						//other bpp are invalid
						default:
						case 0: Vector4Set(px,  1,  1,  1,  1);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff; //always bytes.
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3];
							sa = px[3];
							k = 1-(1-da)*(1-sa);
							out[3] = k;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA16F)
			{
				unsigned short *out = (unsigned short*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 8: Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+2), XCF_HalfFloat(in+4), XCF_HalfFloat(in+6)); break;
						case 6: Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+2), XCF_HalfFloat(in+4), 1); break;
						case 4:	Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+2)); break;
						case 2:	Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), 1); break;
						//other bpp are invalid
						default:
						case 0: Vector4Set(px,  1,  1,  1,  1);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff; //always bytes.
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = HalfToFloat(out[3]);
							sa = px[3];
							k = 1-(1-da)*(1-sa);
							out[3] = FloatToHalf(k);
							k = sa/k;
							out[0] = FloatToHalf(HalfToFloat(out[0])*(1-k)+px[0]*k);
							out[1] = FloatToHalf(HalfToFloat(out[1])*(1-k)+px[1]*k);
							out[2] = FloatToHalf(HalfToFloat(out[2])*(1-k)+px[2]*k);
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA16)
			{
				unsigned short *out = (unsigned short*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 8: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+2), XCF_BigShort(in+4), XCF_BigShort(in+6));	break;
						case 6: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+2), XCF_BigShort(in+4),  0xffff);	break;
						case 4: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+2));	break;
						case 2: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+0),  0xffff);	break;
						default:
						case 0: Vector4Set(px,  0xffff,  0xffff,  0xffff,  0xffff);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff;
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3]/(float)0xffff;
							sa = px[3]/0xffff;
							k = 1-(1-da)*(1-sa);
							out[3] = k*0xffff;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA8)
			{
				qbyte *out = f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 4: Vector4Set(px, in[0], in[1], in[2], in[3]);	break;
						case 3: Vector4Set(px, in[0], in[1], in[2],  0xff);	break;
						case 2: Vector4Set(px, in[0], in[0], in[0], in[1]);	break;
						case 1: Vector4Set(px, in[0], in[0], in[0],  0xff);	break;
						default:
						case 0: Vector4Set(px,  0xff,  0xff,  0xff,  0xff);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff;
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3]/255.0;
							sa = px[3]/255;
							k = 1-(1-da)*(1-sa);
							out[3] = k*255;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else
			{
				Con_Printf("xcf: colour precision %i is not supported\n", f->precision);
				unsupported = true;
				goto parseerror;
			}
		}
	}

parseerror:
	BZ_Free(l.data);
	BZ_Free(h.data);

	(void)type;
	(void)layermaskoffset;

	return !unsupported;
}
static qbyte *ReadXCFFile(const qbyte *filedata, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{
	size_t offs;
	struct xcf_s ctx;
	unsigned int bb,bw,bh,bd;
	if (len < 14 || strncmp(filedata, "gimp xcf ", 9) || filedata[13])
		return NULL;
	memset(&ctx, 0, sizeof(ctx));
	ctx.version = atoi(filedata+10);
	ctx.filestart = filedata;
	ctx.filesize = len;
	ctx.offset = 14;

	ctx.precision = 150;
	ctx.outformat = PTI_RGBA8;
	ctx.width = XCF_Read32(&ctx);
	ctx.height = XCF_Read32(&ctx);
	ctx.basetype = XCF_Read32(&ctx);
	if (ctx.basetype != 0/*rgb*/ && ctx.basetype != 1/*grey*/)
	{
		Con_Printf("%s: xcf paletted mode is not supported\n", fname);
		return NULL; //doesn't really matter what format it is, we're going to output rgba regardless. we can just do it based upon the bytes per pixel.
	}
	if (ctx.version >= 4)
	{
		ctx.precision = XCF_Read32(&ctx);
		if (ctx.version < 7)
			ctx.precision = 150; //dev versions have different interpretations, just ignore it so that we don't have to handle that mess.
		switch(ctx.precision)
		{
		case 100:	ctx.outformat = PTI_RGBA8;	break;
		case 150:	ctx.outformat = PTI_RGBA8/*_SRGB*/;	break; //usually this one... but we don't care too much about srgb... for some reason.
		case 200:	ctx.outformat = PTI_RGBA16;	break;
		case 250:	ctx.outformat = PTI_RGBA16/*_SRGB*/;	break;
		//case 300:	ctx.outformat = PTI_RGBA32;	break;
		//case 350:	ctx.outformat = PTI_RGBA32/*_SRGB*/;	break;
		case 500:	ctx.outformat = PTI_RGBA16F;	break;
		case 550:	ctx.outformat = PTI_RGBA16F/*_SRGB*/;	break;
		case 600:	ctx.outformat = PTI_RGBA32F;	break;
		case 650:	ctx.outformat = PTI_RGBA32F/*_SRGB*/;	break;
		default:
			Con_Printf("%s: xcf colour precision is not supported\n", fname);
			return NULL;
		}
	}
	if (!format && ctx.outformat != PTI_RGBA8)
		return NULL;	//caller insists on rgba8 :(
	XCF_ReadHeaderProperties(&ctx);
	while((offs=XCF_ReadOffset(&ctx)))
	{
		ctx.layeroffsets = realloc(ctx.layeroffsets, sizeof(*ctx.layeroffsets)*(ctx.numlayers+1));
		ctx.layeroffsets[ctx.numlayers++] = offs;
	}
	//channels

	//without any layers, its fully transparent
	Image_BlockSizeForEncoding(ctx.outformat, &bb,&bw,&bh,&bd); //just for the bb...
	ctx.flat = Z_Malloc(ctx.width*ctx.height*bb);
	if (format)
		*format = ctx.outformat;
	*width = ctx.width;
	*height = ctx.height;

	while(ctx.numlayers --> 0)
	{
		ctx.offset = ctx.layeroffsets[ctx.numlayers];
		if (!XCF_CombineLayer(&ctx))
		{
			Z_Free(ctx.flat);
			ctx.flat = NULL;
			break;
		}
	}


	BZ_Free(ctx.layeroffsets);
	return ctx.flat;
}
#endif

#ifdef IMAGEFMT_EXR
#if 0
	#include "OpenEXR/ImfCRgbaFile.h"
#else
	typedef void ImfInputFile;
	typedef void ImfHeader;
	typedef void ImfRgba;
#endif
static struct
{
	void *handle;
	ImfInputFile   *(*OpenInputFile)		(const char name[]);
	int				(*CloseInputFile)		(ImfInputFile *in);
	int				(*InputSetFrameBuffer)	(ImfInputFile *in, ImfRgba *base, size_t xStride, size_t yStride);
	int				(*InputReadPixels)		(ImfInputFile *in, int scanLine1, int scanLine2);
	const ImfHeader*(*InputHeader)			(const ImfInputFile *in);

	void			(*HeaderDataWindow) (const ImfHeader *hdr, int *xMin, int *yMin, int *xMax, int *yMax);
} exr;
static void InitLibrary_OpenEXR(void)
{
#ifdef IMF_MAGIC
	exr.OpenInputFile		= ImfOpenInputFile;
	exr.CloseInputFile		= ImfCloseInputFile;
	exr.InputSetFrameBuffer	= ImfInputSetFrameBuffer;
	exr.InputReadPixels		= ImfInputReadPixels;
	exr.InputHeader			= ImfInputHeader;
	exr.HeaderDataWindow	= ImfHeaderDataWindow;
#else
	dllfunction_t funcs[] =
	{
		{(void**)&exr.OpenInputFile,		"ImfOpenInputFile"},
		{(void**)&exr.CloseInputFile,		"ImfCloseInputFile"},
		{(void**)&exr.InputSetFrameBuffer,	"ImfInputSetFrameBuffer"},
		{(void**)&exr.InputReadPixels,		"ImfInputReadPixels"},
		{(void**)&exr.InputHeader,			"ImfInputHeader"},
		{(void**)&exr.HeaderDataWindow,		"ImfHeaderDataWindow"},
		{NULL}
	};
	#ifdef __linux__
		//its some shitty c++ library, so while the C api that we use is stable, nothing else is so it has lots of random names.
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_3.so.24", funcs);	//debian sid(bullseye)
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_2.so.23", funcs);	//debian buster
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_2.so.22", funcs);	//debian stretch
	#endif

	//try the generic/dev name.
	if (!exr.handle)
		exr.handle = Sys_LoadLibrary("libIlmImf", funcs);
#endif
}
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif
static void *ReadEXRFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat)
{
	char tname[] = "/tmp/exr.XXXXXX";
#ifndef _WIN32
	int fd;
#endif
	ImfInputFile *ctx;
	const ImfHeader *hdr;
	void *result;

	if (!exr.handle)
	{
		Con_Printf("%s: libIlmImf not loaded\n", fname);
		return NULL;
	}

	//shitty API that only supports filenames, so now that we've read it from a file, write it to a file so that it can be read. See, shitty.
#ifdef _WIN32
	if (!_mktemp(tname))
	{
		DWORD sizewritten;
		HANDLE fd = CreateFileA(tname, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);
		WriteFile(fd, buf, len, &sizewritten, NULL); //assume we managed to write it all
		ctx = exr.OpenInputFile(tname); //is this be ansi or oem or utf-16 or what? lets assume that it has no idea either.
		CloseHandle(fd);
#else
	fd = mkstemp(tname);	//bsd4.3/posix1-2001
	if (fd >= 0)
	{
		if (write(fd, buf, len) == len)
			ctx = exr.OpenInputFile(tname);
		else
			ctx = NULL;
		close(fd);	//we don't need the input file now.
		unlink(tname);
#endif

		if (ctx)
		{
			int xmin, xmax, ymin, ymax;
			hdr = exr.InputHeader(ctx);
			exr.HeaderDataWindow(hdr, &xmin,&ymin, &xmax,&ymax);
			*outwidth = (xmax-xmin)+1;
			*outheight = (ymax-ymin)+1;
			result = BZ_Malloc(sizeof(short)*4u*(size_t)*outwidth**outheight);
			exr.InputSetFrameBuffer(ctx, (char*)result-xmin*8-ymin*(size_t)*outwidth*8, 1, *outwidth);
			exr.InputReadPixels(ctx, ymin, ymax);
			exr.CloseInputFile(ctx);
			*outformat = PTI_RGBA16F;	//output is always half-floats.
			return result;
		}
	}
	return NULL;
}
#endif

#ifdef HAVE_CLIENT


// saturate function, stolen from jitspoe
void SaturateR8G8B8(qbyte *data, int size, float sat)
{
	int i;
	float r, g, b, v;

	if (sat > 1)
	{
		for(i=0; i < size; i+=3)
		{
			r = data[i];
			g = data[i+1];
			b = data[i+2];

			v = r * NTSC_RED + g * NTSC_GREEN + b * NTSC_BLUE;
			r = v + (r - v) * sat;
			g = v + (g - v) * sat;
			b = v + (b - v) * sat;

			// bounds check
			if (r < 0)
				r = 0;
			else if (r > 255)
				r = 255;

			if (g < 0)
				g = 0;
			else if (g > 255)
				g = 255;

			if (b < 0)
				b = 0;
			else if (b > 255)
				b = 255;

			// scale down to avoid overbright lightmaps
			v = v / (r * NTSC_RED + g * NTSC_GREEN + b * NTSC_BLUE);
			if (v > NTSC_SUM)
				v = NTSC_SUM;
			else
				v *= v;

			data[i]   = r*v;
			data[i+1] = g*v;
			data[i+2] = b*v;
		}
	}
	else // avoid bounds check for desaturation
	{
		if (sat < 0)
			sat = 0;

		for(i=0; i < size; i+=3)
		{
			r = data[i];
			g = data[i+1];
			b = data[i+2];

			v = r * NTSC_RED + g * NTSC_GREEN + b * NTSC_BLUE;

			data[i]   = v + (r - v) * sat;
			data[i+1] = v + (g - v) * sat;
			data[i+2] = v + (b - v) * sat;
		}
	}
}

void BoostGamma(qbyte *rgba, int width, int height, uploadfmt_t fmt)
{
	//note: should not be used where hardware gamma is supported.
	int i;

	switch(fmt)
	{
	case PTI_L8:
		for (i=0 ; i<width*height ; i++)
			rgba[i] = gammatable[rgba[i]];
		break;
	case PTI_LLLX8:
	case PTI_LLLA8:
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8:
	case PTI_BGRX8:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
		for (i=0 ; i<width*height*4 ; i+=4)
		{
			rgba[i+0] = gammatable[rgba[i+0]];
			rgba[i+1] = gammatable[rgba[i+1]];
			rgba[i+2] = gammatable[rgba[i+2]];
			//and not alpha
		}
		break;
	default:
		break;
	}
}


static void Image_LoadTexture_Failed(void *ctx, void *data, size_t a, size_t b)
{
	texid_t tex = ctx;
	tex->status = TEX_FAILED;
}
static void Image_FixupImageSize(texid_t tex, unsigned int w, unsigned int h, unsigned int d)
{
	tex->width = w;
	tex->height = h;
	tex->depth = d;

	//ezhud breaks without this. I assume other things will too. this is why you shouldn't depend upon querying an image's size.
	if (!strncmp(tex->ident, "gfx/", 4))
	{
		size_t lumpsize;
		qbyte lumptype;
		qpic_t *pic = W_GetLumpName(tex->ident+4, &lumpsize, &lumptype);
		if (pic && lumptype == TYP_QPIC && lumpsize >= 8)
		{
			w = LittleLong(pic->width);
			h = LittleLong(pic->height);
			if (lumpsize == 8 + w*h)
			{
				tex->width = w;
				tex->height = h;
			}
		}
		else
		{
			vfsfile_t *f;
			const char *ext = COM_GetFileExtension(tex->ident, NULL);
			if (!strcmp(ext, ".lmp"))
				f = FS_OpenVFS(tex->ident, "rb", FS_GAME);
			else if (!*ext)
			{
				char nname[MAX_QPATH+4];
				Q_snprintfz(nname, sizeof(nname), "%s.lmp", tex->ident);
				f = FS_OpenVFS(nname, "rb", FS_GAME);
			}
			else
				f = NULL;
			if (f)
			{
				unsigned int wh[2];
				size_t size = VFS_GETLEN(f);
				VFS_READ(f, wh, 8);
				VFS_CLOSE(f);

				if (size == 8+wh[0]*wh[1])
				{
					tex->width = wh[0];
					tex->height = wh[1];
				}
			}
		}
	}
	//FIXME: check loaded wad files too.
}
static void Image_LoadTextureMips(void *ctx, void *data, size_t a, size_t b)
{
	int i;
	texid_t tex = ctx;
	struct pendingtextureinfo *mips = data;

	//setting the dimensions here can break npot textures, so lets not do that.
//	tex->width = mips->mip[0].width;
//	tex->height = mips->mip[0].height;

	//d3d9 needs to reconfigure samplers depending on whether the data is srgb or not.
	switch(mips->encoding)
	{
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
	case PTI_BC1_RGB_SRGB:
	case PTI_BC1_RGBA_SRGB:
	case PTI_BC2_RGBA_SRGB:
	case PTI_BC3_RGBA_SRGB:
	case PTI_BC7_RGBA_SRGB:
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_ASTC_4X4_SRGB:
	case PTI_ASTC_5X4_SRGB:
	case PTI_ASTC_5X5_SRGB:
	case PTI_ASTC_6X5_SRGB:
	case PTI_ASTC_6X6_SRGB:
	case PTI_ASTC_8X5_SRGB:
	case PTI_ASTC_8X6_SRGB:
	case PTI_ASTC_10X5_SRGB:
	case PTI_ASTC_10X6_SRGB:
	case PTI_ASTC_8X8_SRGB:
	case PTI_ASTC_10X8_SRGB:
	case PTI_ASTC_10X10_SRGB:
	case PTI_ASTC_12X10_SRGB:
	case PTI_ASTC_12X12_SRGB:
		tex->flags |= IF_SRGB;
		break;
	default:
		tex->flags &= ~IF_SRGB;
		break;
	}

	if ((tex->flags & IF_TEXTYPEMASK)==IF_TEXTYPE_ANY)
		tex->flags = (tex->flags&~IF_TEXTYPEMASK)|(mips->type<<IF_TEXTYPESHIFT);

	if (rf->IMG_LoadTextureMips(tex, mips))
	{
		tex->format = mips->encoding;
		tex->status = TEX_LOADED;
	}
	else
	{	//failure can happen because a) lost device. b) out of device memory. c) format not supported.
		//FIXME: handle oom properly.
		tex->format = TF_INVALID;
		tex->status = TEX_FAILED;
	}

	for (i = 0; i < mips->mipcount; i++)
		if (mips->mip[i].needfree)
			BZ_Free(mips->mip[i].data);
	if (mips->extrafree)
		BZ_Free(mips->extrafree);
	BZ_Free(mips);
}
#endif

#ifdef IMAGEFMT_KTX
typedef struct
{
	char magic[12];
	unsigned int endianness;

	unsigned int gltype;
	unsigned int gltypesize;
	unsigned int glformat;
	unsigned int glinternalformat;

	unsigned int glbaseinternalformat;
	unsigned int pixelwidth;
	unsigned int pixelheight;
	unsigned int pixeldepth;

	unsigned int numberofarrayelements;
	unsigned int numberoffaces;
	unsigned int numberofmipmaplevels;
	unsigned int bytesofkeyvaluedata;
} ktxheader_t;
qboolean Image_WriteKTXFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips)
{
	unsigned int bb,bw,bh,bd;
	vfsfile_t *file;
	ktxheader_t header = {{0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A}, 0x04030201/*endianness*/,
		0/*type*/, 1/*typesize*/, 0/*format*/, 0/*internalformat*/,
		0/*base*/, mips->mip[0].width, mips->mip[0].height, 0/*depth*/,
		0/*array elements*/, 1, mips->mipcount, 0/*kvdatasize*/};
	size_t mipnum;
	if (mips->type==PTI_CUBE_ARRAY)
	{
		if (!mips->mip[0].depth || mips->mip[0].depth % 6)
		{
			Con_Printf("Image_WriteKTXFile: malformed cube\n");
			return false;	//malformed...
		}
		header.numberoffaces = 6;
		header.numberofarrayelements = mips->mip[0].depth/6;
	}
	else if (mips->type==PTI_CUBE)
	{
		if (mips->mip[0].depth != 6)
		{
			Con_Printf("Image_WriteKTXFile: malformed cube\n");
			return false;	//malformed...
		}
		header.numberofarrayelements = 0;
		header.numberoffaces = 6;
	}
	else if (mips->type==PTI_2D_ARRAY)
	{
		if (!mips->mip[0].depth)
			return false;
		header.numberofarrayelements = mips->mip[0].depth;
	}
	else if (mips->type == PTI_3D)
		header.pixeldepth = mips->mip[0].depth;
	else if (mips->type == PTI_2D)
	{
		if (mips->mip[0].depth != 1)
			return false;
	}
	else
	{
		Con_Printf("Image_WriteKTXFile: unsupported texture type\n");
		return false;
	}

	Image_BlockSizeForEncoding(mips->encoding, &bb, &bw, &bh, &bd);

	safeswitch(mips->encoding)
	{
	case PTI_ETC1_RGB8:			header.glinternalformat = 0x8D64/*GL_ETC1_RGB8_OES*/; break;
	case PTI_ETC2_RGB8:			header.glinternalformat = 0x9274/*GL_COMPRESSED_RGB8_ETC2*/; break;
	case PTI_ETC2_RGB8_SRGB:	header.glinternalformat = 0x9275/*GL_COMPRESSED_SRGB8_ETC2*/; break;
	case PTI_ETC2_RGB8A1:		header.glinternalformat = 0x9276/*GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2*/; break;
	case PTI_ETC2_RGB8A1_SRGB:	header.glinternalformat = 0x9277/*GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2*/; break;
	case PTI_ETC2_RGB8A8:		header.glinternalformat = 0x9278/*GL_COMPRESSED_RGBA8_ETC2_EAC*/; break;
	case PTI_ETC2_RGB8A8_SRGB:	header.glinternalformat = 0x9279/*GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC*/; break;
	case PTI_EAC_R11:			header.glinternalformat = 0x9270/*GL_COMPRESSED_R11_EAC*/; break;
	case PTI_EAC_R11_SNORM:		header.glinternalformat = 0x9271/*GL_COMPRESSED_SIGNED_R11_EAC*/; break;
	case PTI_EAC_RG11:			header.glinternalformat = 0x9272/*GL_COMPRESSED_RG11_EAC*/; break;
	case PTI_EAC_RG11_SNORM:	header.glinternalformat = 0x9273/*GL_COMPRESSED_SIGNED_RG11_EAC*/; break;
	case PTI_BC1_RGB:			header.glinternalformat = 0x83F0/*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGB_SRGB:		header.glinternalformat = 0x8C4C/*GL_COMPRESSED_SRGB_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGBA:			header.glinternalformat = 0x83F1/*GL_COMPRESSED_RGBA_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGBA_SRGB:		header.glinternalformat = 0x8C4D/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT*/; break;
	case PTI_BC2_RGBA:			header.glinternalformat = 0x83F2/*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT*/; break;
	case PTI_BC2_RGBA_SRGB:		header.glinternalformat = 0x8C4E/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT*/; break;
	case PTI_BC3_RGBA:			header.glinternalformat = 0x83F3/*GL_COMPRESSED_RGBA_S3TC_DXT5_EXT*/; break;
	case PTI_BC3_RGBA_SRGB:		header.glinternalformat = 0x8C4F/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT*/; break;
	case PTI_BC4_R_SNORM:		header.glinternalformat = 0x8DBC/*GL_COMPRESSED_SIGNED_RED_RGTC1*/; break;
	case PTI_BC4_R:				header.glinternalformat = 0x8DBB/*GL_COMPRESSED_RED_RGTC1*/; break;
	case PTI_BC5_RG_SNORM:		header.glinternalformat = 0x8DBE/*GL_COMPRESSED_SIGNED_RG_RGTC2*/; break;
	case PTI_BC5_RG:			header.glinternalformat = 0x8DBD/*GL_COMPRESSED_RG_RGTC2*/; break;
	case PTI_BC6_RGB_UFLOAT:	header.glinternalformat = 0x8E8F/*GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB*/; break;
	case PTI_BC6_RGB_SFLOAT:	header.glinternalformat = 0x8E8E/*GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB*/; break;
	case PTI_BC7_RGBA:			header.glinternalformat = 0x8E8C/*GL_COMPRESSED_RGBA_BPTC_UNORM_ARB*/; break;
	case PTI_BC7_RGBA_SRGB:		header.glinternalformat = 0x8E8D/*GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB*/; break;
	case PTI_ASTC_4X4_HDR:		//sadly gl/ktx does not distinguish between ldr+hdr, which presents problems that will have to be handled by the loader.
	case PTI_ASTC_4X4_LDR:		header.glinternalformat = 0x93B0/*GL_COMPRESSED_RGBA_ASTC_4x4_KHR*/; break;
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X4_LDR:		header.glinternalformat = 0x93B1/*GL_COMPRESSED_RGBA_ASTC_5x4_KHR*/; break;
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_5X5_LDR:		header.glinternalformat = 0x93B2/*GL_COMPRESSED_RGBA_ASTC_5x5_KHR*/; break;
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X5_LDR:		header.glinternalformat = 0x93B3/*GL_COMPRESSED_RGBA_ASTC_6x5_KHR*/; break;
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_6X6_LDR:		header.glinternalformat = 0x93B4/*GL_COMPRESSED_RGBA_ASTC_6x6_KHR*/; break;
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X5_LDR:		header.glinternalformat = 0x93B5/*GL_COMPRESSED_RGBA_ASTC_8x5_KHR*/; break;
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_8X6_LDR:		header.glinternalformat = 0x93B6/*GL_COMPRESSED_RGBA_ASTC_8x6_KHR*/; break;
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_8X8_LDR:		header.glinternalformat = 0x93B7/*GL_COMPRESSED_RGBA_ASTC_8x8_KHR*/; break;
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X5_LDR:		header.glinternalformat = 0x93B8/*GL_COMPRESSED_RGBA_ASTC_10x5_KHR*/; break;
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_10X6_LDR:		header.glinternalformat = 0x93B9/*GL_COMPRESSED_RGBA_ASTC_10x6_KHR*/; break;
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X8_LDR:		header.glinternalformat = 0x93BA/*GL_COMPRESSED_RGBA_ASTC_10x8_KHR*/; break;
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_10X10_LDR:	header.glinternalformat = 0x93BB/*GL_COMPRESSED_RGBA_ASTC_10x10_KHR*/; break;
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X10_LDR:	header.glinternalformat = 0x93BC/*GL_COMPRESSED_RGBA_ASTC_12x10_KHR*/; break;
	case PTI_ASTC_12X12_HDR:
	case PTI_ASTC_12X12_LDR:	header.glinternalformat = 0x93BD/*GL_COMPRESSED_RGBA_ASTC_12x12_KHR*/; break;
	case PTI_ASTC_4X4_SRGB:		header.glinternalformat = 0x93D0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR*/; break;
	case PTI_ASTC_5X4_SRGB:		header.glinternalformat = 0x93D1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR*/; break;
	case PTI_ASTC_5X5_SRGB:		header.glinternalformat = 0x93D2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR*/; break;
	case PTI_ASTC_6X5_SRGB:		header.glinternalformat = 0x93D3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR*/; break;
	case PTI_ASTC_6X6_SRGB:		header.glinternalformat = 0x93D4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR*/; break;
	case PTI_ASTC_8X5_SRGB:		header.glinternalformat = 0x93D5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR*/; break;
	case PTI_ASTC_8X6_SRGB:		header.glinternalformat = 0x93D6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR*/; break;
	case PTI_ASTC_8X8_SRGB:		header.glinternalformat = 0x93D7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR*/; break;
	case PTI_ASTC_10X5_SRGB:	header.glinternalformat = 0x93D8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR*/; break;
	case PTI_ASTC_10X6_SRGB:	header.glinternalformat = 0x93D9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR*/; break;
	case PTI_ASTC_10X8_SRGB:	header.glinternalformat = 0x93DA/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR*/; break;
	case PTI_ASTC_10X10_SRGB:	header.glinternalformat = 0x93DB/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR*/; break;
	case PTI_ASTC_12X10_SRGB:	header.glinternalformat = 0x93DC/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR*/; break;
	case PTI_ASTC_12X12_SRGB:	header.glinternalformat = 0x93DD/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR*/; break;
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_3X3X3_LDR:	header.glinternalformat = 0x93C0/*GL_COMPRESSED_RGBA_ASTC_3x3x3_OES*/; break;
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X3X3_LDR:	header.glinternalformat = 0x93C1/*GL_COMPRESSED_RGBA_ASTC_4x3x3_OES*/; break;
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X3_LDR:	header.glinternalformat = 0x93C2/*GL_COMPRESSED_RGBA_ASTC_4x4x3_OES*/; break;
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_4X4X4_LDR:	header.glinternalformat = 0x93C3/*GL_COMPRESSED_RGBA_ASTC_4x4x5_OES*/; break;
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X4X4_LDR:	header.glinternalformat = 0x93C4/*GL_COMPRESSED_RGBA_ASTC_5x4x4_OES*/; break;
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X4_LDR:	header.glinternalformat = 0x93C5/*GL_COMPRESSED_RGBA_ASTC_5x5x4_OES*/; break;
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_5X5X5_LDR:	header.glinternalformat = 0x93C6/*GL_COMPRESSED_RGBA_ASTC_5x5x5_OES*/; break;
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X5X5_LDR:	header.glinternalformat = 0x93C7/*GL_COMPRESSED_RGBA_ASTC_6x5x5_OES*/; break;
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X5_LDR:	header.glinternalformat = 0x93C8/*GL_COMPRESSED_RGBA_ASTC_6x6x5_OES*/; break;
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_6X6X6_LDR:	header.glinternalformat = 0x93C9/*GL_COMPRESSED_RGBA_ASTC_6x6x6_OES*/; break;
	case PTI_ASTC_3X3X3_SRGB:	header.glinternalformat = 0x93E0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES*/; break;
	case PTI_ASTC_4X3X3_SRGB:	header.glinternalformat = 0x93E1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES*/; break;
	case PTI_ASTC_4X4X3_SRGB:	header.glinternalformat = 0x93E2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES*/; break;
	case PTI_ASTC_4X4X4_SRGB:	header.glinternalformat = 0x93E3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES*/; break;
	case PTI_ASTC_5X4X4_SRGB:	header.glinternalformat = 0x93E4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES*/; break;
	case PTI_ASTC_5X5X4_SRGB:	header.glinternalformat = 0x93E5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES*/; break;
	case PTI_ASTC_5X5X5_SRGB:	header.glinternalformat = 0x93E6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES*/; break;
	case PTI_ASTC_6X5X5_SRGB:	header.glinternalformat = 0x93E7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES*/; break;
	case PTI_ASTC_6X6X5_SRGB:	header.glinternalformat = 0x93E8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES*/; break;
	case PTI_ASTC_6X6X6_SRGB:	header.glinternalformat = 0x93E9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES*/; break;
#endif

	case PTI_BGRA8:				header.glinternalformat = 0x8058/*GL_RGBA8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBA8:				header.glinternalformat = 0x8058/*GL_RGBA8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGRA8_SRGB:		header.glinternalformat = 0x8C43/*GL_SRGB8_ALPHA8*/;		header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBA8_SRGB:		header.glinternalformat = 0x8C43/*GL_SRGB8_ALPHA8*/;		header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8:				header.glinternalformat = 0x8040/*GL_LUMINANCE8*/;			header.glbaseinternalformat = 0x1909/*GL_LUMINANCE*/;		header.glformat = 0x1909/*GL_LUMINANCE*/;		header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8A8:				header.glinternalformat = 0x8045/*GL_LUMINANCE8_ALPHA8*/;	header.glbaseinternalformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.glformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8_SRGB:			header.glinternalformat = 0x8C47/*GL_SLUMINANCE8*/;			header.glbaseinternalformat = 0x1909/*GL_LUMINANCE*/;		header.glformat = 0x1909/*GL_LUMINANCE*/;		header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8A8_SRGB:			header.glinternalformat = 0x8C45/*GL_SLUMINANCE8_ALPHA8*/;	header.glbaseinternalformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.glformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGR8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E0/*GL_BGR*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB8_SRGB:			header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGR8_SRGB:			header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E0/*GL_BGR*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_R16:				header.glinternalformat = 0x822A/*GL_R16*/;					header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_RGBA16:			header.glinternalformat = 0x805B/*GL_RGBA16*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_R16F:				header.glinternalformat = 0x822D/*GL_R16F*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x140B/*GL_HALF_FLOAT*/;					header.gltypesize = 2; break;
	case PTI_R32F:				header.glinternalformat = 0x822E/*GL_R32F*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_RGBA16F:			header.glinternalformat = 0x881A/*GL_RGBA16F*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x140B/*GL_HALF_FLOAT*/;					header.gltypesize = 2; break;
	case PTI_RGB32F:			header.glinternalformat = 0x8815/*GL_RGB32F*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_RGBA32F:			header.glinternalformat = 0x8814/*GL_RGBA32F*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_A2BGR10:			header.glinternalformat = 0x8059/*GL_RGB10_A2*/;			header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8368/*GL_UNSIGNED_INT_2_10_10_10_REV*/;	header.gltypesize = 4; break;
	case PTI_E5BGR9:			header.glinternalformat = 0x8C3D/*GL_RGB9_E5*/;				header.glbaseinternalformat = 0x8C3D/*GL_RGB9_E5*/;			header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8C3E/*GL_UNSIGNED_INT_5_9_9_9_REV*/;		header.gltypesize = 4; break;
	case PTI_B10G11R11F:		header.glinternalformat = 0x8C3A/*GL_R11F_G11F_B10F*/;		header.glbaseinternalformat = 0x8C3D/*GL_R11F_G11F_B10F*/;	header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8C3B/*GL_UNSIGNED_INT_10_11_11_REV*/;		header.gltypesize = 4; break;
	case PTI_P8:
	case PTI_R8:				header.glinternalformat = 0x8229/*GL_R8*/;					header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RG8:				header.glinternalformat = 0x822B/*GL_RG8*/;					header.glbaseinternalformat = 0x8227/*GL_RG*/;				header.glformat = 0x8227/*GL_RG*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_R8_SNORM:			header.glinternalformat = 0x8F94/*GL_R8_SNORM*/;			header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1400/*GL_BYTE*/;							header.gltypesize = 1; break;
	case PTI_RG8_SNORM:			header.glinternalformat = 0x8F95/*GL_RG8_SNORM*/;			header.glbaseinternalformat = 0x8227/*GL_RG*/;				header.glformat = 0x8227/*GL_RG*/;				header.gltype = 0x1400/*GL_BYTE*/;							header.gltypesize = 1; break;
	case PTI_BGRX8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBX8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGRX8_SRGB:		header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBX8_SRGB:		header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB565:			header.glinternalformat = 0x8D62/*GL_RGB565*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8363/*GL_UNSIGNED_SHORT_5_6_5*/;			header.gltypesize = 2; break;
	case PTI_RGBA4444:			header.glinternalformat = 0x8056/*GL_RGBA4*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8033/*GL_UNSIGNED_SHORT_4_4_4_4*/;		header.gltypesize = 2; break;
	case PTI_ARGB4444:			header.glinternalformat = 0x8056/*GL_RGBA4*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x8365/*GL_UNSIGNED_SHORT_4_4_4_4_REV*/;	header.gltypesize = 2; break;
	case PTI_RGBA5551:			header.glinternalformat = 0x8057/*GL_RGB5_A1*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8034/*GL_UNSIGNED_SHORT_5_5_5_1*/;		header.gltypesize = 2; break;
	case PTI_ARGB1555:			header.glinternalformat = 0x8057/*GL_RGB5_A1*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x8366/*GL_UNSIGNED_SHORT_1_5_5_5_REV*/;	header.gltypesize = 2; break;
	case PTI_DEPTH16:			header.glinternalformat = 0x81A5/*GL_DEPTH_COMPONENT16*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_DEPTH24:			header.glinternalformat = 0x81A6/*GL_DEPTH_COMPONENT24*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1405/*GL_UNSIGNED_INT*/;					header.gltypesize = 3; break;
	case PTI_DEPTH32:			header.glinternalformat = 0x81A7/*GL_DEPTH_COMPONENT32*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_DEPTH24_8:			header.glinternalformat = 0x88F0/*GL_DEPTH24_STENCIL8*/;	header.glbaseinternalformat = 0x84F9/*GL_DEPTH_STENCIL*/;	header.glformat = 0x84F9/*GL_DEPTH_STENCIL*/;	header.gltype = 0x84FA/*GL_UNSIGNED_INT_24_8*/;				header.gltypesize = 4; break;

#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:
#endif
	case PTI_EMULATED:
	case PTI_MAX:
		return false;

	safedefault:
		return false;
	}

	if (strchr(filename, '*') || strchr(filename, ':'))
		return false;

	file = FS_OpenVFS(filename, "wb", fsroot);
	if (!file)
		return false;
	VFS_WRITE(file, &header, sizeof(header));

	for (mipnum = 0; mipnum < mips->mipcount; )
	{
		unsigned int sz;
		//translate to blocks
		unsigned int browbytes = bb * ((mips->mip[mipnum].width+bw-1)/bh);
		unsigned int padbytes = (browbytes&3)?4-(browbytes&3):0;
		unsigned int brows = (mips->mip[mipnum].height+bh-1)/bh;
		unsigned int blayers = (mips->mip[mipnum].depth+bd-1)/bd;
		if (mips->mip[mipnum].datasize != browbytes*brows*blayers)
		{	//should probably be a sys_error
			Con_Printf("WriteKTX mip %u missized\n", (unsigned)mipnum);
			VFS_CLOSE(file);
			return false;
		}
		switch(mips->type)
		{
		case PTI_ANY:
			VFS_CLOSE(file);
			return false;
		case PTI_CUBE:	//special case, size is per-face
			sz = (browbytes+padbytes) * brows;
			break;
		case PTI_2D:
		case PTI_2D_ARRAY:
		case PTI_CUBE_ARRAY:
		case PTI_3D:
			sz = (browbytes+padbytes) * brows * blayers;
			break;
		}
		VFS_WRITE(file, &sz, 4);
		brows *= blayers;
		if (padbytes)
		{
			unsigned int pad = 0, y;
			for (y = 0; y < brows; y++)
			{
				VFS_WRITE(file, (qbyte*)mips->mip[mipnum].data + browbytes*y, browbytes);
				VFS_WRITE(file, &pad, 4-(browbytes&3));
			}
		}
		else
			VFS_WRITE(file, mips->mip[mipnum].data, browbytes*brows);
		mipnum++;
	}

	VFS_CLOSE(file);
	return true;
}

#define LongSwap(i) (((i&0xff000000) >> 24)|((i&0x00ff0000) >> 8)|((i&0x0000ff00) << 8)|((i&0x000000ff) << 24))
#define ShortSwap(i) (((i&0xff00) >> 8)|((i&0x00ff) << 8))
static struct pendingtextureinfo *Image_ReadKTX1File(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	static const char magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
	ktxheader_t header;
	int nummips;
	int mipnum;
	int datasize;
	unsigned int *swap, w, h, d, f, l, browbytes,padbytes,y,x,rows;
	struct pendingtextureinfo *mips;
	int encoding = TF_INVALID;
	const qbyte *fileend = filedata + filesize;

	unsigned int blockwidth, blockheight, blockdepth, blockbytes;

	if (filesize < sizeof(ktxheader_t) || memcmp(filedata, magic, sizeof(magic)))
		return NULL;	//not a ktx file

	header = *(const ktxheader_t*)filedata;
	if (header.endianness == 0x01020304)
	{	//swap the rest of the header.
		for (swap = &header.endianness; swap < (unsigned int*)(&header+1); swap++)
			*swap = LongSwap(*swap);
	}
	else if (header.endianness != 0x04030201)
		return NULL;

	nummips = header.numberofmipmaplevels;
	if (nummips < 1)
		nummips = 1;

//	if (header->numberofarrayelements != 0)
//		return NULL;	//don't support array textures
	if (header.numberoffaces == 1)
		;	//non-cubemap
	else if (header.numberoffaces == 6)
	{
		if (header.numberofarrayelements != 0)
			return NULL;	//don't support array textures

		if (header.pixeldepth != 0)
			return NULL;
//		if (header->numberofmipmaplevels != 1)
//			return false;	//only allow cubemaps that have no mips
	}
	else
		return NULL;	//don't allow weird cubemaps
//	if (header->pixeldepth && header->pixelwidth != header->pixeldepth && header->pixelheight != header->pixeldepth)
//		return NULL;	//we only support 3d textures where width+height+depth are the same. too lazy to change it now.

	/*FIXME: validate format+type for non-compressed formats*/
	switch(header.glinternalformat)
	{
	case 0x8D64/*GL_ETC1_RGB8_OES*/:							encoding = PTI_ETC1_RGB8;			break;
	case 0x9270/*GL_COMPRESSED_R11_EAC*/:						encoding = PTI_EAC_R11;				break;
	case 0x9271/*GL_COMPRESSED_SIGNED_R11_EAC*/:				encoding = PTI_EAC_R11_SNORM;		break;
	case 0x9272/*GL_COMPRESSED_RG11_EAC*/:						encoding = PTI_EAC_RG11;			break;
	case 0x9273/*GL_COMPRESSED_SIGNED_RG11_EAC*/:				encoding = PTI_EAC_RG11_SNORM;		break;
	case 0x9274/*GL_COMPRESSED_RGB8_ETC2*/:						encoding = PTI_ETC2_RGB8;			break;
	case 0x9275/*GL_COMPRESSED_SRGB8_ETC2*/:					encoding = PTI_ETC2_RGB8_SRGB;		break;
	case 0x9276/*GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2*/:	encoding = PTI_ETC2_RGB8A1;			break;
	case 0x9277/*GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2*/:encoding = PTI_ETC2_RGB8A1_SRGB;	break;
	case 0x9278/*GL_COMPRESSED_RGBA8_ETC2_EAC*/:				encoding = PTI_ETC2_RGB8A8;			break;
	case 0x9279/*GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC*/:			encoding = PTI_ETC2_RGB8A8_SRGB;	break;
	case 0x83F0/*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/:				encoding = PTI_BC1_RGB;				break;
	case 0x8C4C/*GL_COMPRESSED_SRGB_S3TC_DXT1_EXT*/:			encoding = PTI_BC1_RGB_SRGB;		break;
	case 0x83F1/*GL_COMPRESSED_RGBA_S3TC_DXT1_EXT*/:			encoding = PTI_BC1_RGBA;			break;
	case 0x8C4D/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT*/:		encoding = PTI_BC1_RGBA_SRGB;		break;
	case 0x83F2/*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT*/:			encoding = PTI_BC2_RGBA;			break;
	case 0x8C4E/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT*/:		encoding = PTI_BC2_RGBA_SRGB;		break;
	case 0x83F3/*GL_COMPRESSED_RGBA_S3TC_DXT5_EXT*/:			encoding = PTI_BC3_RGBA;			break;
	case 0x8C4F/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT*/:		encoding = PTI_BC3_RGBA_SRGB;		break;
	case 0x8DBC/*GL_COMPRESSED_SIGNED_RED_RGTC1*/:				encoding = PTI_BC4_R_SNORM;			break;
	case 0x8DBB/*GL_COMPRESSED_RED_RGTC1*/:						encoding = PTI_BC4_R;				break;
	case 0x8DBE/*GL_COMPRESSED_SIGNED_RG_RGTC2*/:				encoding = PTI_BC5_RG_SNORM;		break;
	case 0x8DBD/*GL_COMPRESSED_RG_RGTC2*/:						encoding = PTI_BC5_RG;				break;
	case 0x8E8F/*GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB*/:	encoding = PTI_BC6_RGB_UFLOAT;		break;
	case 0x8E8E/*GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB*/:		encoding = PTI_BC6_RGB_SFLOAT;		break;
	case 0x8E8C/*GL_COMPRESSED_RGBA_BPTC_UNORM_ARB*/:			encoding = PTI_BC7_RGBA;			break;
	case 0x8E8D/*GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB*/:		encoding = PTI_BC7_RGBA_SRGB;		break;
	case 0x93B0/*GL_COMPRESSED_RGBA_ASTC_4x4_KHR*/:				encoding = PTI_ASTC_4X4_LDR;		break;
	case 0x93B1/*GL_COMPRESSED_RGBA_ASTC_5x4_KHR*/:				encoding = PTI_ASTC_5X4_LDR;		break;
	case 0x93B2/*GL_COMPRESSED_RGBA_ASTC_5x5_KHR*/:				encoding = PTI_ASTC_5X5_LDR;		break;
	case 0x93B3/*GL_COMPRESSED_RGBA_ASTC_6x5_KHR*/:				encoding = PTI_ASTC_6X5_LDR;		break;
	case 0x93B4/*GL_COMPRESSED_RGBA_ASTC_6x6_KHR*/:				encoding = PTI_ASTC_6X6_LDR;		break;
	case 0x93B5/*GL_COMPRESSED_RGBA_ASTC_8x5_KHR*/:				encoding = PTI_ASTC_8X5_LDR;		break;
	case 0x93B6/*GL_COMPRESSED_RGBA_ASTC_8x6_KHR*/:				encoding = PTI_ASTC_8X6_LDR;		break;
	case 0x93B7/*GL_COMPRESSED_RGBA_ASTC_8x8_KHR*/:				encoding = PTI_ASTC_8X8_LDR;		break;
	case 0x93B8/*GL_COMPRESSED_RGBA_ASTC_10x5_KHR*/:			encoding = PTI_ASTC_10X5_LDR;		break;
	case 0x93B9/*GL_COMPRESSED_RGBA_ASTC_10x6_KHR*/:			encoding = PTI_ASTC_10X6_LDR;		break;
	case 0x93BA/*GL_COMPRESSED_RGBA_ASTC_10x8_KHR*/:			encoding = PTI_ASTC_10X8_LDR;		break;
	case 0x93BB/*GL_COMPRESSED_RGBA_ASTC_10x10_KHR*/:			encoding = PTI_ASTC_10X10_LDR;		break;
	case 0x93BC/*GL_COMPRESSED_RGBA_ASTC_12x10_KHR*/:			encoding = PTI_ASTC_12X10_LDR;		break;
	case 0x93BD/*GL_COMPRESSED_RGBA_ASTC_12x12_KHR*/:			encoding = PTI_ASTC_12X12_LDR;		break;
	case 0x93D0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR*/:		encoding = PTI_ASTC_4X4_SRGB;		break;
	case 0x93D1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR*/:		encoding = PTI_ASTC_5X4_SRGB;		break;
	case 0x93D2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR*/:		encoding = PTI_ASTC_5X5_SRGB;		break;
	case 0x93D3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR*/:		encoding = PTI_ASTC_6X5_SRGB;		break;
	case 0x93D4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR*/:		encoding = PTI_ASTC_6X6_SRGB;		break;
	case 0x93D5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR*/:		encoding = PTI_ASTC_8X5_SRGB;		break;
	case 0x93D6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR*/:		encoding = PTI_ASTC_8X6_SRGB;		break;
	case 0x93D7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR*/:		encoding = PTI_ASTC_8X8_SRGB;		break;
	case 0x93D8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR*/:	encoding = PTI_ASTC_10X5_SRGB;		break;
	case 0x93D9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR*/:	encoding = PTI_ASTC_10X6_SRGB;		break;
	case 0x93DA/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR*/:	encoding = PTI_ASTC_10X8_SRGB;		break;
	case 0x93DB/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR*/:	encoding = PTI_ASTC_10X10_SRGB;		break;
	case 0x93DC/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR*/:	encoding = PTI_ASTC_12X10_SRGB;		break;
	case 0x93DD/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR*/:	encoding = PTI_ASTC_12X12_SRGB;		break;
	case 0x80E1/*GL_BGRA_EXT*/:									encoding = PTI_BGRA8;				break;	//not even an internal format
	case 0x1908/*GL_RGBA*/:
	case 0x8058/*GL_RGBA8*/:									encoding = (header.glformat==0x80E1/*GL_BGRA*/)?PTI_BGRA8:PTI_RGBA8;			break;	//unsized types shouldn't really be here
	case 0x805B/*GL_RGBA16*/:									encoding = PTI_RGBA16;				break;
	case 0x8C43/*GL_SRGB8_ALPHA8*/:								encoding = (header.glformat==0x80E1/*GL_BGRA*/)?PTI_BGRA8_SRGB:PTI_RGBA8_SRGB;	break;
	case 0x8040/*GL_LUMINANCE8*/:								encoding = PTI_L8;					break;
	case 0x8045/*GL_LUMINANCE8_ALPHA8*/:						encoding = PTI_L8A8;				break;
	case 0x881A/*GL_RGBA16F_ARB*/:								encoding = PTI_RGBA16F;				break;
	case 0x8815/*GL_RGB32F_ARB*/:								encoding = PTI_RGB32F;				break;
	case 0x8814/*GL_RGBA32F_ARB*/:								encoding = PTI_RGBA32F;				break;
	case 0x8059/*GL_RGB10_A2*/:									encoding = PTI_A2BGR10;				break;
	case 0x8229/*GL_R8*/:										encoding = PTI_R8;					break;
	case 0x822A/*GL_R16*/:										encoding = PTI_R16;					break;
	case 0x822B/*GL_RG8*/:										encoding = PTI_RG8;					break;
	case 0x8F94/*GL_R8_SNORM*/:									encoding = PTI_R8_SNORM;			break;
	case 0x8F95/*GL_RG8_SNORM*/:								encoding = PTI_RG8_SNORM;			break;
	case 0x81A5/*GL_DEPTH_COMPONENT16*/:						encoding = PTI_DEPTH16;				break;
	case 0x81A6/*GL_DEPTH_COMPONENT24*/:						encoding = PTI_DEPTH24;				break;
	case 0x81A7/*GL_DEPTH_COMPONENT32*/:						encoding = PTI_DEPTH32;				break;
	case 0x88F0/*GL_DEPTH24_STENCIL8*/:							encoding = PTI_DEPTH24_8;			break;
	case 0x822D/*GL_R16F*/:										encoding = PTI_R16F;				break;
	case 0x822E/*GL_R32F*/:										encoding = PTI_R32F;				break;

	case 0x8C40/*GL_SRGB*/:
	case 0x8C41/*GL_SRGB8*/:
		if (header.glformat==0x80E1/*GL_BGRA*/)
			encoding = PTI_BGRX8_SRGB;
		else if (header.glformat==0x1908/*GL_RGBA*/)
			encoding = PTI_RGBX8_SRGB;
		break;

	case 0x1907/*GL_RGB*/:	//invalid sized format. treat as GL_RGB8, and do weird checks.
	case 0x8051/*GL_RGB8*/:	//other sized RGB formats are treated based upon the data format rather than the sized format, in case they were meant to be converted by the driver...
	case 0x8C3D/*GL_RGB9_E5*/:
	case 0x8D62/*GL_RGB565*/:
	case 0x8C3A/*GL_R11F_G11F_B10F*/:
		if (header.glformat == 0x80E0/*GL_BGR*/)
			encoding = PTI_BGR8;
		else if (header.glformat == 0x80E1/*GL_BGRA*/)
			encoding = PTI_BGRX8;
		else if (header.glformat == 0x1907/*GL_RGB*/)
		{
			if (header.gltype == 0x8C3B/*GL_UNSIGNED_INT_10F_11F_11F_REV*/)
				encoding = PTI_B10G11R11F;
			else if (header.gltype == 0x8C3E/*GL_UNSIGNED_INT_5_9_9_9_REV*/)
				encoding = PTI_E5BGR9;
			else if (header.gltype == 0x8363/*GL_UNSIGNED_SHORT_5_6_5*/)
				encoding = PTI_RGB565;
			else
				encoding = PTI_RGB8;
		}
		else if (header.glformat == 0x1908/*GL_RGBA*/)
			encoding = PTI_RGBX8;
		else
			encoding = PTI_RGB8;
		break;
	case 0x8056/*GL_RGBA4*/:
	case 0x8057/*GL_RGB5_A1*/:
		if (header.glformat == 0x1908/*GL_RGBA*/ && header.gltype == 0x8034/*GL_UNSIGNED_SHORT_5_5_5_1*/)
			encoding = PTI_RGBA5551;
		else if (header.glformat == 0x80E1/*GL_BGRA*/ && header.gltype == 0x8366/*GL_UNSIGNED_SHORT_1_5_5_5_REV*/)
			encoding = PTI_ARGB1555;
		else if (header.glformat == 0x1908/*GL_RGBA*/ && header.gltype == 0x8033/*GL_UNSIGNED_SHORT_4_4_4_4*/)
			encoding = PTI_RGBA4444;
		else if (header.glformat == 0x80E1/*GL_BGRA*/ && header.gltype == 0x8365/*GL_UNSIGNED_SHORT_4_4_4_4_REV*/)
			encoding = PTI_ARGB4444;
		break;

	default:
		encoding = TF_INVALID;
		break;
	}
	if (encoding == TF_INVALID)
	{
		Con_Printf("Unsupported ktx internalformat %x in %s\n", header.glinternalformat, fname);
		return NULL;
	}

//	if (!sh_config.texfmt[encoding])
//	{
//		Con_Printf("KTX %s: encoding %x not supported on this system\n", fname, header->glinternalformat);
//		return false;
//	}

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	if (header.pixeldepth)
		mips->type = PTI_3D;
	else if (header.numberoffaces==6)
	{
		if (header.numberofarrayelements)
		{
			header.pixeldepth = header.numberofarrayelements*6;
			mips->type = PTI_CUBE_ARRAY;
		}
		else
			mips->type = PTI_CUBE;
	}
	else
	{
		if (header.numberofarrayelements)
		{
			header.pixeldepth = header.numberofarrayelements;
			mips->type = PTI_2D_ARRAY;
		}
		else
		{
			header.pixeldepth = 1;
			mips->type = PTI_2D;
		}
	}
	mips->extrafree = filedata;
	mips->encoding = encoding;

	filedata += sizeof(header);			//skip the header...
	filedata += header.bytesofkeyvaluedata;	//skip the keyvalue stuff

	if (nummips * header.numberoffaces > countof(mips->mip))
		nummips = countof(mips->mip) / header.numberoffaces;

	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	w = header.pixelwidth;
	h = max(1, header.pixelheight);
	d = max(1, header.pixeldepth);
	f = max(1, header.numberoffaces);
	l = max(1, header.numberofarrayelements);

	for (mipnum = 0; mipnum < nummips; mipnum++)
	{
		datasize = *(int*)filedata;
		filedata += 4;

		if (header.endianness == 0x01020304)
			datasize = LongSwap(datasize);

		browbytes = blockbytes * ((w+blockwidth-1)/blockwidth);
		padbytes = (browbytes & 3)?4-(browbytes&3):0;
		rows = ((h+blockheight-1)/blockheight)*
			   ((d+blockdepth-1)/blockdepth);
		if (datasize != (browbytes+padbytes) * rows)
		{
			Con_Printf("%s: mip %i does not match expected size (%u, required %u)\n", fname, mipnum, datasize, (browbytes+padbytes) * rows);
			break;
		}

		if (filedata + datasize*f*l > fileend)
		{
			Con_Printf("%s: truncation at mip %i\n", fname, mipnum);
			break;
		}

		if (mips->mipcount >= countof(mips->mip))
			break;
		mips->mip[mips->mipcount].width = w;
		mips->mip[mips->mipcount].height = h;
		mips->mip[mips->mipcount].depth = d*l*f;

		if (padbytes || header.endianness == 0x01020304)
		{	//gah.
			//the ktx format is 4-byte aligned. our internal representation is tightly packed (consistent with everything but gl).
			//in the case of byteswapping, any data types should work out okay (no misaligned stuff).
			rows *= l*f;
			mips->mip[mips->mipcount].needfree = true;
			mips->mip[mips->mipcount].datasize = browbytes * rows;
			mips->mip[mips->mipcount].data = BZ_Malloc(mips->mip[mips->mipcount].datasize);
			if (header.gltypesize == 4 && header.endianness == 0x01020304)
			{
				for (y = 0; y < rows; y++)
					for (x = 0; x < browbytes>>2; x++)
						((int*)((qbyte*)mips->mip[mips->mipcount].data + y*browbytes))[x] = LongSwap(((int*)filedata + y*browbytes+padbytes)[x]);
			}
			else if (header.gltypesize == 2 && header.endianness == 0x01020304)
			{
				for (y = 0; y < rows; y++)
					for (x = 0; x < browbytes>>1; x++)
						((short*)((qbyte*)mips->mip[mips->mipcount].data + y*browbytes))[x] = ShortSwap(((short*)filedata + y*browbytes+padbytes)[x]);
			}
			else
			{	//erk, panic...
				for (y = 0; y < rows; y++)
					memcpy((qbyte*)mips->mip[mips->mipcount].data + y*browbytes, filedata + y*browbytes+padbytes, browbytes);
			}
		}
		else
		{
			mips->mip[mips->mipcount].datasize = datasize * l*f;
			mips->mip[mips->mipcount].data = filedata;
		}
		mips->mipcount++;

		filedata += datasize *l*f;

		w = max(1, w>>1);
		h = max(1, h>>1);
		if (mips->type == PTI_3D)
			d = max(1, d>>1);
	}

	if (!mips->mipcount)
	{
		Z_Free(mips);
		return NULL;
	}

#ifdef ASTC_WITH_HDRTEST
	if (encoding >= PTI_ASTC_4X4_LDR && encoding < PTI_ASTC_4X4_SRGB)
	{
		int face;
		for (face = 0; face < header.numberoffaces; face++)
		{
			if (ASTC_BlocksAreHDR(mips->mip[face].data, mips->mip[face].datasize, blockwidth, blockheight, 1))
			{	//convert it to one of the hdr formats if we can.
				mips->encoding = PTI_ASTC_4X4_HDR+(encoding-PTI_ASTC_4X4_LDR);
				break;
			}
		}
	}
#endif

	return mips;
}

typedef struct
{
	char magic[12];
	quint32_t vkFormat;
	quint32_t typesize;
	quint32_t pixelwidth;
	quint32_t pixelheight;
	quint32_t pixeldepth;
	quint32_t layercount;
	quint32_t facecount;
	quint32_t levelcount;
	quint32_t compressionscheme;

	quint32_t dfdoffset;
	quint32_t dfdsize;
	quint32_t kvdoffset;
	quint32_t kvdsize;
	quint64_t sgdoffset;
	quint64_t sgdsize;
} ktx2header_t;
typedef struct
{
	quint64_t offset;
	quint64_t compsize;
	quint64_t rawsize;
} ktx2lavelheader_t;
static struct pendingtextureinfo *Image_ReadKTX2File(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	static const char magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
	ktx2header_t header;
	const ktx2lavelheader_t *levelheader;
	int mipnum;
	unsigned int w, h, d;
	struct pendingtextureinfo *mips;
	int encoding = TF_INVALID, itype;

	unsigned int bw, bh, bd, bb;

	if (filesize < sizeof(ktxheader_t) || memcmp(filedata, magic, sizeof(magic)))
		return NULL;	//not a ktx file

	header = *(const ktx2header_t*)filedata;
	levelheader = (const ktx2lavelheader_t*)((const ktx2header_t*)filedata+1);

	header.vkFormat			= LittleLong(header.vkFormat);
	header.typesize			= LittleLong(header.typesize);
	header.pixelwidth		= LittleLong(header.pixelwidth);
	header.pixelheight		= LittleLong(header.pixelheight);
	header.pixeldepth		= LittleLong(header.pixeldepth);
	header.layercount		= LittleLong(header.layercount);
	header.facecount		= LittleLong(header.facecount);
	header.levelcount		= LittleLong(header.levelcount);
	header.compressionscheme= LittleLong(header.compressionscheme);
	header.dfdoffset		= LittleLong(header.dfdoffset);
	header.dfdsize			= LittleLong(header.dfdsize);
	header.kvdoffset		= LittleLong(header.kvdoffset);
	header.kvdsize			= LittleLong(header.kvdsize);
	header.sgdoffset		= LittleI64(header.sgdoffset);
	header.sgdsize			= LittleI64(header.sgdsize);

	if (!header.pixelheight)
		header.pixelheight = 1;	//we don't support 1D textures. force it to 2d.
	if (header.pixeldepth)
	{
		if (header.layercount || header.facecount!=1)
			return NULL;	//neither 3d arrays nor 3d cubes are supported, nor do they really make sense.
		header.layercount = 1;
		itype = PTI_3D;
	}
	else
	{
		header.pixeldepth = 1;

		if (header.facecount==6)
		{	//cube...
			if (header.layercount)
				itype = PTI_CUBE_ARRAY;
			else itype = PTI_CUBE, header.layercount=1;
		}
		else if (header.facecount == 1)
		{	//boring 2d
			if (header.layercount)
				itype = PTI_2D_ARRAY;
			else itype = PTI_2D, header.layercount=1;
		}
		else
			return NULL;	//not allowed
	}

	w = header.pixelwidth;
	h = header.pixelheight;
	d = header.pixeldepth*header.facecount*header.layercount;

	if (!header.levelcount)
		header.levelcount = 1;	//means we must auto-generate the mip pyramid. should warn if texflags doesn't match.

	switch (header.compressionscheme)
	{
	case 0:	//no compression
		break;
	case 1: //basis... w/e
	case 2:	//zstd... we have no decompression lib
	case 3: //zlib... we probably have zlib! but the docs imply zlib yet states raw deflate (so requires a passing the windowsize to zlib's inflateInit2 as negative, which is non-obvious).
	default:
		return NULL;	//we don't support this junk. gzip it. you'll get better compression than doing it per-level.
	}

	/*FIXME: validate format+type for non-compressed formats*/
	switch(header.vkFormat)
	{
//	case 1/*VK_FORMAT_R4G4_UNORM_PACK8*/:				encoding = PTI_RG4;			break;
	case 2/*VK_FORMAT_R4G4B4A4_UNORM_PACK16*/:			encoding = PTI_RGBA4444;	break;
//	case 3/*VK_FORMAT_B4G4R4A4_UNORM_PACK16*/:			encoding = PTI_BGRA4444;	break;
	case 4/*VK_FORMAT_R5G6B5_UNORM_PACK16*/:			encoding = PTI_RGB565;		break;
//	case 5/*VK_FORMAT_B5G6R5_UNORM_PACK16*/:			encoding = PTI_BGR565;		break;
	case 6/*VK_FORMAT_R5G5B5A1_UNORM_PACK16*/:			encoding = PTI_RGBA5551;	break;
//	case 7/*VK_FORMAT_B5G5R5A1_UNORM_PACK16*/:			encoding = PTI_BGRA5551;	break;
	case 8/*VK_FORMAT_A1R5G5B5_UNORM_PACK16*/:			encoding = PTI_ARGB1555;	break;
	case 9/*VK_FORMAT_R8_UNORM*/:						encoding = PTI_R8;			break;
	case 10/*VK_FORMAT_R8_SNORM*/:						encoding = PTI_R8_SNORM;	break;
//	case 11/*VK_FORMAT_R8_USCALED*/:
//	case 12/*VK_FORMAT_R8_SSCALED*/:
//	case 13/*VK_FORMAT_R8_UINT*/:
//	case 14/*VK_FORMAT_R8_SINT*/:
	case 15/*VK_FORMAT_R8_SRGB*/:						encoding = PTI_L8_SRGB;		break;	//erk
	case 16/*VK_FORMAT_R8G8_UNORM*/:					encoding = PTI_RG8;			break;
	case 17/*VK_FORMAT_R8G8_SNORM*/:					encoding = PTI_RG8_SNORM;	break;
//	case 18/*VK_FORMAT_R8G8_USCALED*/:
//	case 19/*VK_FORMAT_R8G8_SSCALED*/:
//	case 20/*VK_FORMAT_R8G8_UINT*/:
//	case 21/*VK_FORMAT_R8G8_SINT*/:
//	case 22/*VK_FORMAT_R8G8_SRGB*/:
	case 23/*VK_FORMAT_R8G8B8_UNORM*/:					encoding = PTI_RGB8;		break;
//	case 24/*VK_FORMAT_R8G8B8_SNORM*/:
//	case 25/*VK_FORMAT_R8G8B8_USCALED*/:
//	case 26/*VK_FORMAT_R8G8B8_SSCALED*/:
//	case 27/*VK_FORMAT_R8G8B8_UINT*/:
//	case 28/*VK_FORMAT_R8G8B8_SINT*/:
	case 29/*VK_FORMAT_R8G8B8_SRGB*/:					encoding = PTI_RGB8_SRGB;	break;
	case 30/*VK_FORMAT_B8G8R8_UNORM*/:					encoding = PTI_BGR8;		break;
//	case 31/*VK_FORMAT_B8G8R8_SNORM*/:
//	case 32/*VK_FORMAT_B8G8R8_USCALED*/:
//	case 33/*VK_FORMAT_B8G8R8_SSCALED*/:
//	case 34/*VK_FORMAT_B8G8R8_UINT*/:
//	case 35/*VK_FORMAT_B8G8R8_SINT*/:
	case 36/*VK_FORMAT_B8G8R8_SRGB*/:					encoding = PTI_BGR8_SRGB;	break;
	case 37/*VK_FORMAT_R8G8B8A8_UNORM*/:				encoding = PTI_RGBA8;		break;
//	case 38/*VK_FORMAT_R8G8B8A8_SNORM*/:
//	case 39/*VK_FORMAT_R8G8B8A8_USCALED*/:
//	case 40/*VK_FORMAT_R8G8B8A8_SSCALED*/:
//	case 41/*VK_FORMAT_R8G8B8A8_UINT*/:
//	case 42/*VK_FORMAT_R8G8B8A8_SINT*/:
	case 43/*VK_FORMAT_R8G8B8A8_SRGB*/:					encoding = PTI_RGBA8_SRGB;	break;
	case 44/*VK_FORMAT_B8G8R8A8_UNORM*/:				encoding = PTI_BGRA8;		break;
//	case 45/*VK_FORMAT_B8G8R8A8_SNORM*/:
//	case 46/*VK_FORMAT_B8G8R8A8_USCALED*/:
//	case 47/*VK_FORMAT_B8G8R8A8_SSCALED*/:
//	case 48/*VK_FORMAT_B8G8R8A8_UINT*/:
//	case 49/*VK_FORMAT_B8G8R8A8_SINT*/:
	case 50/*VK_FORMAT_B8G8R8A8_SRGB*/:					encoding = PTI_BGRA8_SRGB;	break;
//	case 51/*VK_FORMAT_A8B8G8R8_UNORM_PACK32*/:
//	case 52/*VK_FORMAT_A8B8G8R8_SNORM_PACK32*/:
//	case 53/*VK_FORMAT_A8B8G8R8_USCALED_PACK32*/:
//	case 54/*VK_FORMAT_A8B8G8R8_SSCALED_PACK32*/:
//	case 55/*VK_FORMAT_A8B8G8R8_UINT_PACK32*/:
//	case 56/*VK_FORMAT_A8B8G8R8_SINT_PACK32*/:
//	case 57/*VK_FORMAT_A8B8G8R8_SRGB_PACK32*/:
//	case 58/*VK_FORMAT_A2R10G10B10_UNORM_PACK32*/:
//	case 59/*VK_FORMAT_A2R10G10B10_SNORM_PACK32*/:
//	case 60/*VK_FORMAT_A2R10G10B10_USCALED_PACK32*/:
//	case 61/*VK_FORMAT_A2R10G10B10_SSCALED_PACK32*/:
//	case 62/*VK_FORMAT_A2R10G10B10_UINT_PACK32*/:
//	case 63/*VK_FORMAT_A2R10G10B10_SINT_PACK32*/:
	case 64/*VK_FORMAT_A2B10G10R10_UNORM_PACK32*/:		encoding = PTI_A2BGR10;		break;
//	case 65/*VK_FORMAT_A2B10G10R10_SNORM_PACK32*/:
//	case 66/*VK_FORMAT_A2B10G10R10_USCALED_PACK32*/:
//	case 67/*VK_FORMAT_A2B10G10R10_SSCALED_PACK32*/:
//	case 68/*VK_FORMAT_A2B10G10R10_UINT_PACK32*/:
//	case 69/*VK_FORMAT_A2B10G10R10_SINT_PACK32*/:
	case 70/*VK_FORMAT_R16_UNORM*/:						encoding = PTI_R16;			break;
//	case 71/*VK_FORMAT_R16_SNORM*/:
//	case 72/*VK_FORMAT_R16_USCALED*/:
//	case 73/*VK_FORMAT_R16_SSCALED*/:
//	case 74/*VK_FORMAT_R16_UINT*/:
//	case 75/*VK_FORMAT_R16_SINT*/:
	case 76/*VK_FORMAT_R16_SFLOAT*/:					encoding = PTI_R16F;		break;
//	case 77/*VK_FORMAT_R16G16_UNORM*/:
//	case 78/*VK_FORMAT_R16G16_SNORM*/:
//	case 79/*VK_FORMAT_R16G16_USCALED*/:
//	case 80/*VK_FORMAT_R16G16_SSCALED*/:
//	case 81/*VK_FORMAT_R16G16_UINT*/:
//	case 82/*VK_FORMAT_R16G16_SINT*/:
//	case 83/*VK_FORMAT_R16G16_SFLOAT*/:
//	case 84/*VK_FORMAT_R16G16B16_UNORM*/:
//	case 85/*VK_FORMAT_R16G16B16_SNORM*/:
//	case 86/*VK_FORMAT_R16G16B16_USCALED*/:
//	case 87/*VK_FORMAT_R16G16B16_SSCALED*/:
//	case 88/*VK_FORMAT_R16G16B16_UINT*/:
//	case 89/*VK_FORMAT_R16G16B16_SINT*/:
	case 90/*VK_FORMAT_R16G16B16_SFLOAT*/:				encoding = PTI_RGB32F;		break;
	case 91/*VK_FORMAT_R16G16B16A16_UNORM*/:			encoding = PTI_RGBA16;		break;
//	case 92/*VK_FORMAT_R16G16B16A16_SNORM*/:
//	case 93/*VK_FORMAT_R16G16B16A16_USCALED*/:
//	case 94/*VK_FORMAT_R16G16B16A16_SSCALED*/:
//	case 95/*VK_FORMAT_R16G16B16A16_UINT*/:
//	case 96/*VK_FORMAT_R16G16B16A16_SINT*/:
	case 97/*VK_FORMAT_R16G16B16A16_SFLOAT*/:			encoding = PTI_RGBA16F;		break;
//	case 98/*VK_FORMAT_R32_UINT*/:
//	case 99/*VK_FORMAT_R32_SINT*/:
	case 100/*VK_FORMAT_R32_SFLOAT*/:					encoding = PTI_R32F;		break;
//	case 101/*VK_FORMAT_R32G32_UINT*/:
//	case 102/*VK_FORMAT_R32G32_SINT*/:
//	case 103/*VK_FORMAT_R32G32_SFLOAT*/:
//	case 104/*VK_FORMAT_R32G32B32_UINT*/:
//	case 105/*VK_FORMAT_R32G32B32_SINT*/:
	case 106/*VK_FORMAT_R32G32B32_SFLOAT*/:				encoding = PTI_RGB32F;		break;
//	case 107/*VK_FORMAT_R32G32B32A32_UINT*/:
//	case 108/*VK_FORMAT_R32G32B32A32_SINT*/:
	case 109/*VK_FORMAT_R32G32B32A32_SFLOAT*/:			encoding = PTI_RGBA32F;		break;
//	case 110/*VK_FORMAT_R64_UINT*/:
//	case 111/*VK_FORMAT_R64_SINT*/:
//	case 112/*VK_FORMAT_R64_SFLOAT*/:
//	case 113/*VK_FORMAT_R64G64_UINT*/:
//	case 114/*VK_FORMAT_R64G64_SINT*/:
//	case 115/*VK_FORMAT_R64G64_SFLOAT*/:
//	case 116/*VK_FORMAT_R64G64B64_UINT*/:
//	case 117/*VK_FORMAT_R64G64B64_SINT*/:
//	case 118/*VK_FORMAT_R64G64B64_SFLOAT*/:
//	case 119/*VK_FORMAT_R64G64B64A64_UINT*/:
//	case 120/*VK_FORMAT_R64G64B64A64_SINT*/:
//	case 121/*VK_FORMAT_R64G64B64A64_SFLOAT*/:
	case 122/*VK_FORMAT_B10G11R11_UFLOAT_PACK32*/:		encoding = PTI_B10G11R11F;	break;
	case 123/*VK_FORMAT_E5B9G9R9_UFLOAT_PACK32*/:		encoding = PTI_E5BGR9;		break;
	//case 124/*VK_FORMAT_D16_UNORM*/:					encoding = PTI_DEPTH16;		break;
	//case 125/*VK_FORMAT_X8_D24_UNORM_PACK32*/:		encoding = PTI_DEPTH24;		break;
	//case 126/*VK_FORMAT_D32_SFLOAT*/:					encoding = PTI_DEPTH32;		break;
//	case 127/*VK_FORMAT_S8_UINT*/:
//	case 128/*VK_FORMAT_D16_UNORM_S8_UINT*/:
	//case 129/*VK_FORMAT_D24_UNORM_S8_UINT*/:			encoding = PTI_DEPTH24_8;	break;
//	case 130/*VK_FORMAT_D32_SFLOAT_S8_UINT*/:
	case 131/*VK_FORMAT_BC1_RGB_UNORM_BLOCK*/:			encoding = PTI_BC1_RGB;			break;
	case 132/*VK_FORMAT_BC1_RGB_SRGB_BLOCK*/:			encoding = PTI_BC1_RGB_SRGB;	break;
	case 133/*VK_FORMAT_BC1_RGBA_UNORM_BLOCK*/:			encoding = PTI_BC1_RGBA;		break;
	case 134/*VK_FORMAT_BC1_RGBA_SRGB_BLOCK*/:			encoding = PTI_BC1_RGBA_SRGB;	break;
	case 135/*VK_FORMAT_BC2_UNORM_BLOCK*/:				encoding = PTI_BC2_RGBA;		break;
	case 136/*VK_FORMAT_BC2_SRGB_BLOCK*/:				encoding = PTI_BC2_RGBA_SRGB;	break;
	case 137/*VK_FORMAT_BC3_UNORM_BLOCK*/:				encoding = PTI_BC3_RGBA;		break;
	case 138/*VK_FORMAT_BC3_SRGB_BLOCK*/:				encoding = PTI_BC1_RGBA_SRGB;	break;
	case 139/*VK_FORMAT_BC4_UNORM_BLOCK*/:				encoding = PTI_BC4_R;			break;
	case 140/*VK_FORMAT_BC4_SNORM_BLOCK*/:				encoding = PTI_BC4_R_SNORM;		break;
	case 141/*VK_FORMAT_BC5_UNORM_BLOCK*/:				encoding = PTI_BC5_RG;			break;
	case 142/*VK_FORMAT_BC5_SNORM_BLOCK*/:				encoding = PTI_BC5_RG_SNORM;	break;
	case 143/*VK_FORMAT_BC6H_UFLOAT_BLOCK*/:			encoding = PTI_BC6_RGB_UFLOAT;	break;
	case 144/*VK_FORMAT_BC6H_SFLOAT_BLOCK*/:			encoding = PTI_BC6_RGB_SFLOAT;	break;
	case 145/*VK_FORMAT_BC7_UNORM_BLOCK*/:				encoding = PTI_BC7_RGBA;		break;
	case 146/*VK_FORMAT_BC7_SRGB_BLOCK*/:				encoding = PTI_BC7_RGBA_SRGB;	break;
	case 147/*VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK*/:		encoding = PTI_ETC2_RGB8;		break;
	case 148/*VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8_SRGB;	break;
	case 149/*VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK*/:	encoding = PTI_ETC2_RGB8A1;		break;
	case 150/*VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8A1_SRGB;break;
	case 151/*VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK*/:	encoding = PTI_ETC2_RGB8A8;		break;
	case 152/*VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8A8_SRGB;break;
	case 153/*VK_FORMAT_EAC_R11_UNORM_BLOCK*/:			encoding = PTI_EAC_R11;			break;
	case 154/*VK_FORMAT_EAC_R11_SNORM_BLOCK*/:			encoding = PTI_EAC_R11_SNORM;	break;
	case 155/*VK_FORMAT_EAC_R11G11_UNORM_BLOCK*/:		encoding = PTI_EAC_RG11;		break;
	case 156/*VK_FORMAT_EAC_R11G11_SNORM_BLOCK*/:		encoding = PTI_EAC_RG11_SNORM;	break;
	case 157/*VK_FORMAT_ASTC_4x4_UNORM_BLOCK*/:			encoding = PTI_ASTC_4X4_LDR;	break;
	case 158/*VK_FORMAT_ASTC_4x4_SRGB_BLOCK*/:			encoding = PTI_ASTC_4X4_SRGB;	break;
	case 159/*VK_FORMAT_ASTC_5x4_UNORM_BLOCK*/:			encoding = PTI_ASTC_5X4_LDR;	break;
	case 160/*VK_FORMAT_ASTC_5x4_SRGB_BLOCK*/:			encoding = PTI_ASTC_5X4_SRGB;	break;
	case 161/*VK_FORMAT_ASTC_5x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_5X5_LDR;	break;
	case 162/*VK_FORMAT_ASTC_5x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_5X5_SRGB;	break;
	case 163/*VK_FORMAT_ASTC_6x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_6X5_LDR;	break;
	case 164/*VK_FORMAT_ASTC_6x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_6X5_SRGB;	break;
	case 165/*VK_FORMAT_ASTC_6x6_UNORM_BLOCK*/:			encoding = PTI_ASTC_6X6_LDR;	break;
	case 166/*VK_FORMAT_ASTC_6x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_6X6_SRGB;	break;
	case 167/*VK_FORMAT_ASTC_8x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X5_LDR;	break;
	case 168/*VK_FORMAT_ASTC_8x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X5_SRGB;	break;
	case 169/*VK_FORMAT_ASTC_8x6_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X6_LDR;	break;
	case 170/*VK_FORMAT_ASTC_8x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X6_SRGB;	break;
	case 171/*VK_FORMAT_ASTC_8x8_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X8_LDR;	break;
	case 172/*VK_FORMAT_ASTC_8x8_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X8_SRGB;	break;
	case 173/*VK_FORMAT_ASTC_10x5_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X5_LDR;	break;
	case 174/*VK_FORMAT_ASTC_10x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X5_SRGB;	break;
	case 175/*VK_FORMAT_ASTC_10x6_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X6_LDR;	break;
	case 176/*VK_FORMAT_ASTC_10x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X6_SRGB;	break;
	case 177/*VK_FORMAT_ASTC_10x8_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X8_LDR;	break;
	case 178/*VK_FORMAT_ASTC_10x8_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X8_SRGB;	break;
	case 179/*VK_FORMAT_ASTC_10x10_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X10_LDR;	break;
	case 180/*VK_FORMAT_ASTC_10x10_SRGB_BLOCK*/:		encoding = PTI_ASTC_10X10_SRGB;	break;
	case 181/*VK_FORMAT_ASTC_12x10_UNORM_BLOCK*/:		encoding = PTI_ASTC_12X10_LDR;	break;
	case 182/*VK_FORMAT_ASTC_12x10_SRGB_BLOCK*/:		encoding = PTI_ASTC_12X10_SRGB;	break;
	case 183/*VK_FORMAT_ASTC_12x12_UNORM_BLOCK*/:		encoding = PTI_ASTC_12X12_LDR;	break;
	case 184/*VK_FORMAT_ASTC_12x12_SRGB_BLOCK*/:		encoding = PTI_ASTC_12X12_SRGB;	break;

	case 0/*VK_FORMAT_UNDEFINED*/:
	default:
		encoding = PTI_INVALID;
		break;
	}
	if (encoding == PTI_INVALID)
	{
		//TODO: we might be able to make sense of these by decoding the DFD.
		Con_Printf(CON_WARNING"%s: Unsupported ktx2 vkformat %x\n", fname, header.vkFormat);
		return NULL;
	}

	if (header.kvdsize)
	{
		size_t kvd = header.kvdoffset;
		const qbyte *key;	//utf-8.
		const qbyte *val;	//often utf-8, but might be binary.
		size_t kvdend = kvd+header.kvdsize;
//		VALGRIND_MAKE_MEM_UNDEFINED(filedata+kvdend, 1);
		while(kvd+4 <= kvdend)
		{
			quint32_t len = (filedata[kvd+0]<<0)|(filedata[kvd+1]<<8)|(filedata[kvd+2]<<16)|(filedata[kvd+3]<<24), klen;
			quint32_t vlen;
			kvd+=4;
			if (kvd+len > kvdend)
				break;	//some sort of error
			for(klen = 0;;)
			{
				if (!filedata[kvd+klen++])
					break;
				if (klen >= len)
				{
					Con_Printf(CON_WARNING"%s: unterminated kvd key\n", fname);
					return NULL; //we NEED a null for it to be valid.
				}
			}
			key = filedata+kvd;
			val = filedata+kvd+klen;
			vlen = len-klen;
			if (!strcmp(key, "KTXwriter"))
				;
			else if (!strcmp(key, "KTXcubemapIncomplete"))
			{
				Con_Printf(CON_WARNING"%s: incomplete cubemaps are not supported\n", fname);
				return NULL;	//would be seen as a 2darray.
			}
			else if (!strcmp(key, "KTXorientation"))
			{
				if (vlen >= 2 && val[0] && val[1] == 'u')
					Con_Printf("%s: warning: image is bottom up\n", fname);
			}
			else if (!strcmp(key, "KTXglFormat"))
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXdxgiFormat__"))	//why do the docs say the trailing underscores?
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXmetalPixelFormat"))
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXswizzle"))
			{
				if (encoding == PTI_R8 && vlen >= 5 && !strcmp(val, "rrr1"))
					encoding = PTI_L8;
//				else if (encoding == PTI_R8_SRGB && vlen >= 5 && !strcmp(val, "rrr1"))
//					encoding = PTI_L8_SRGB;
				else if (encoding == PTI_RG8 && vlen >= 5 && !strcmp(val, "rrrg"))
					encoding = PTI_L8A8;
				else
					Con_Printf("%s: unsupported swizzle: %s\n", fname, val);
			}
			else if (!strcmp(key, "KTXwriterScParams"))
				;
			else if (!strcmp(key, "KTXastcDecodeMode"))
				;
			else if (!strcmp(key, "KTXanimData"))
				/*uint32_t duration, timescale, loopcount*/;
			else
				Con_Printf("%s: unhandled kvd: %s\n", fname, key);
			kvd+=(len+3)&~3;	//padding... strangely the start offset does not 'need' to be aligned. weird.
		}
		if (kvd != kvdend)
			Con_Printf(CON_WARNING"%s: misparsed kvd data\n", fname);
//		VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(filedata+kvdend, 1);
	}

	Image_BlockSizeForEncoding(encoding, &bb, &bw, &bh, &bd);

	mips = Z_Malloc(sizeof(*mips));
	mips->encoding = encoding;
	mips->type = itype;
	mips->mipcount = 0;
	mips->extrafree = filedata;
	mips->encoding = encoding;

	if (header.levelcount > countof(mips->mip))
		header.levelcount = countof(mips->mip);

	for (mipnum = 0; mipnum < header.levelcount; mipnum++, levelheader++)
	{
		size_t ofs = LittleI64(levelheader->offset);
		qbyte *src = filedata + ofs;
		size_t csz = LittleI64(levelheader->compsize);
		size_t rsz = LittleI64(levelheader->rawsize);
		size_t needsize =	((w+bw-1)/bw)*
							((h+bh-1)/bh)*
							((d+bd-1)/bd)*
							bb;
		if (rsz != needsize)
		{
			Con_Printf(CON_WARNING"%s: mip %i does not match expected size (%u, required %u)\n", fname, mipnum, (unsigned int)rsz, (unsigned int)needsize);
			break;
		}
		if (ofs+csz > filesize || csz > filesize || ofs+csz < ofs)
		{
			Con_Printf(CON_WARNING"%s: truncation at mip %i\n", fname, mipnum);
			break;
		}
		if (rsz != csz)
		{
			Con_Printf(CON_WARNING"%s: compression size mismatch\n", fname);
			break;
		}

		if (mips->mipcount >= countof(mips->mip))
			break;

		mips->mip[mips->mipcount].width = w;
		mips->mip[mips->mipcount].height = h;
		mips->mip[mips->mipcount].depth = d;
		mips->mip[mips->mipcount].datasize = rsz;
		mips->mip[mips->mipcount].data = src;
#ifndef FTE_LITTLE_ENDIAN
		switch(header.typesize)
		{
		case 1:
			break;
		case 4:
			{
				quint32_t *in = mips->mip[mips->mipcount].data;
				quint32_t *out = mips->mip[mips->mipcount].data = BZ_Malloc(rsz);
				mips->mip[mips->mipcount].needfree = true;
				for (csz = 0; csz < rsz; csz+=4)
					*out++ = LittleLong(*in++);
			}
			break;
		case 2:
			{
				quint16_t *in = mips->mip[mips->mipcount].data;
				quint16_t *out = mips->mip[mips->mipcount].data = BZ_Malloc(rsz);
				mips->mip[mips->mipcount].needfree = true;
				for (csz = 0; csz < rsz; csz+=2)
					*out++ = LittleShort(*in++);
			}
			break;
		default:
			Con_Printf(CON_WARNING"%s: unsupported type size.\n", fname);
			Z_Free(mips);
			return NULL;
		}
#endif
		mips->mipcount++;

		w = max(1, w>>1);
		h = max(1, h>>1);
		if (mips->type == PTI_3D)
			d = max(1, d>>1);
	}

	if (!mips->mipcount)
	{
		Z_Free(mips);
		return NULL;
	}

#ifdef ASTC_WITH_HDRTEST
	if (encoding >= PTI_ASTC_4X4_LDR && encoding < PTI_ASTC_4X4_SRGB)
	{	//assumption: if any levels are hdr then level0 will contain such a block. might be nicer to start mid-way though, for less blocks.
		if (ASTC_BlocksAreHDR(mips->mip[0].data, mips->mip[0].datasize, bw, bh, bd))
		{	//convert it to one of the hdr formats if we can.
			mips->encoding = PTI_ASTC_4X4_HDR+(encoding-PTI_ASTC_4X4_LDR);
		}
	}
#endif

	return mips;
}
static struct pendingtextureinfo *Image_ReadKTXFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	if (filesize >= 12)
	{
		if (filedata[0] == 0xAB && filedata[1] == 0x4B && filedata[2] == 0x54 && filedata[3] == 0x58 && filedata[4] == 0x20)
		{
			if (filedata[5] == '2')
				return Image_ReadKTX2File(flags, fname, filedata, filesize);
			else
				return Image_ReadKTX1File(flags, fname, filedata, filesize);
		}
	}
	return NULL;	//not enough size for the header.
}
#endif

#ifdef IMAGEFMT_ASTC
static struct pendingtextureinfo *Image_ReadASTCFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	struct pendingtextureinfo *mips;
	int encoding = PTI_INVALID, blockbytes, blockwidth, blockheight, blockdepth;
	static const struct {
		int w, h, d;
		int fmt;
	} sizes[] =
	{
		{4,4,1,PTI_ASTC_4X4_LDR},
		{5,4,1,PTI_ASTC_5X4_LDR},
		{5,5,1,PTI_ASTC_5X5_LDR},
		{6,5,1,PTI_ASTC_6X5_LDR},
		{6,6,1,PTI_ASTC_6X6_LDR},
		{8,5,1,PTI_ASTC_8X5_LDR},
		{8,6,1,PTI_ASTC_8X6_LDR},
		{10,5,1,PTI_ASTC_10X5_LDR},
		{10,6,1,PTI_ASTC_10X6_LDR},
		{8,8,1,PTI_ASTC_8X8_LDR},
		{10,8,1,PTI_ASTC_10X8_LDR},
		{10,10,1,PTI_ASTC_10X10_LDR},
		{12,10,1,PTI_ASTC_12X10_LDR},
		{12,12,1,PTI_ASTC_12X12_LDR},
#ifdef ASTC3D
		{3,3,3,PTI_ASTC_3X3X3_LDR},
		{4,3,3,PTI_ASTC_4X3X3_LDR},
		{4,4,3,PTI_ASTC_4X4X3_LDR},
		{4,4,4,PTI_ASTC_4X4X4_LDR},
		{5,4,4,PTI_ASTC_5X4X4_LDR},
		{5,5,4,PTI_ASTC_5X5X4_LDR},
		{5,5,5,PTI_ASTC_5X5X5_LDR},
		{6,5,5,PTI_ASTC_6X5X5_LDR},
		{6,6,5,PTI_ASTC_6X6X5_LDR},
		{6,6,6,PTI_ASTC_6X6X6_LDR},
#endif
	};
	int i;
	int size[3] = {
		filedata[7]|(filedata[8]<<8)|(filedata[9]<<16),
		filedata[10]|(filedata[11]<<8)|(filedata[12]<<16),
		filedata[13]|(filedata[14]<<8)|(filedata[15]<<16)};
	for (i = 0; i < countof(sizes); i++)
	{
		if (sizes[i].w == filedata[4] && sizes[i].h == filedata[5] && sizes[i].d == filedata[6])
		{
			encoding = sizes[i].fmt;
			break;
		}
	}
	if (!encoding)
		return NULL;	//block size not known
	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);
	if (16+blockbytes*((size[0]+blockwidth-1)/blockwidth)*((size[1]+blockheight-1)/blockheight)*((size[2]+blockdepth-1)/blockdepth) != filesize)
		return NULL;	//err, not the right size!

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 1;	//this format doesn't support mipmaps. so there's only one level.
	mips->type = PTI_2D;
	mips->extrafree = filedata;
	mips->encoding = encoding;
	mips->mip[0].data = filedata+16;
	mips->mip[0].datasize = filesize-16;
	mips->mip[0].width = size[0];
	mips->mip[0].height = size[1];
	mips->mip[0].depth = size[2];
	mips->mip[0].needfree = false;

#ifdef ASTC_WITH_HDRTEST
	if (ASTC_BlocksAreHDR(mips->mip[0].data, mips->mip[0].datasize, blockwidth, blockheight, 1))
	{	//convert it to one of the hdr formats if we can.
		mips->encoding = PTI_ASTC_4X4_HDR+(encoding-PTI_ASTC_4X4_LDR);
	}
#endif
	return mips;
}
#endif

#ifdef IMAGEFMT_PKM
static struct pendingtextureinfo *Image_ReadPKMFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	struct pendingtextureinfo *mips;
	unsigned int encoding, blockbytes, blockwidth, blockheight, blockdepth;
	unsigned short ver, dfmt;
	unsigned short datawidth, dataheight;
	unsigned short imgwidth, imgheight;
	if (filesize < 16 || filedata[0] != 'P' || filedata[1] != 'K' || filedata[2] != 'M' || filedata[3] != ' ')
		return NULL;
	ver = (filedata[4]<<8) | filedata[5];
	dfmt = (filedata[6]<<8) | filedata[7];
	datawidth = (filedata[8]<<8) | filedata[9];
	dataheight = (filedata[10]<<8) | filedata[11];
	imgwidth = (filedata[12]<<8) | filedata[13];
	imgheight = (filedata[14]<<8) | filedata[15];
	if (((imgwidth+3)&~3)!=datawidth || ((imgheight+3)&~3)!=dataheight)
		return NULL;	//these are all 4*4 blocks.
	if (ver == ((1<<8)|0) && ver == ((2<<8)|0))
	{
		if (dfmt == 0)	//should only be in v1
			encoding = PTI_ETC1_RGB8;
		//following should only be in v2, but we don't care.
		else if (dfmt == 1)
			encoding = PTI_ETC2_RGB8;
		else if (dfmt == 2)
			return NULL;	//'old' rgba8 format that's not supported.
		else if (dfmt == 3)
			encoding = PTI_ETC2_RGB8A8;
		else if (dfmt == 4)
			encoding = PTI_ETC2_RGB8A1;
		else if (dfmt == 5)
			encoding = PTI_EAC_R11;
		else if (dfmt == 6)
			encoding = PTI_EAC_RG11;
		else if (dfmt == 7)
			encoding = PTI_EAC_R11_SNORM;
		else if (dfmt == 8)
			encoding = PTI_EAC_RG11_SNORM;
		else if (dfmt == 9)
			encoding = PTI_ETC2_RGB8_SRGB;	//srgb
		else if (dfmt == 10)
			encoding = PTI_ETC2_RGB8A8_SRGB;	//srgb
		else if (dfmt == 11)
			encoding = PTI_ETC2_RGB8A1_SRGB;	//srgb
		else
			return NULL;	//unknown/unsupported
	}
	else
		return NULL;

	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);
	if (16+((datawidth+blockwidth-1)/blockwidth)*((dataheight+blockheight-1)/blockheight)*blockbytes != filesize)
		return NULL;	//err, not the right size!

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 1;	//this format doesn't support mipmaps. so there's only one level.
	mips->type = PTI_2D;
	mips->extrafree = filedata;
	mips->encoding = encoding;
	mips->mip[0].data = filedata+16;
	mips->mip[0].datasize = filesize-16;
	mips->mip[0].width = imgwidth;
	mips->mip[0].height = imgheight;
	mips->mip[0].depth = 1;
	mips->mip[0].needfree = false;
	return mips;
}
#endif

#ifdef IMAGEFMT_DDS
typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;

	unsigned int bitcount;
	unsigned int redmask;
	unsigned int greenmask;
	unsigned int bluemask;
	unsigned int alphamask;
} ddspixelformat_t;

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	ddspixelformat_t ddpfPixelFormat;
	unsigned int ddsCaps[4];
	unsigned int dwReserved2;
} ddsheader_t;
typedef struct {
	unsigned int dxgiformat;
	unsigned int resourcetype; //0=unknown, 1=buffer, 2=1d, 3=2d, 4=3d
	unsigned int miscflag;	//singular... yeah. 4=cubemap.
	unsigned int arraysize;
	unsigned int miscflags2;
} dds10header_t;

static struct pendingtextureinfo *Image_ReadDDSFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	int nummips;
	int mipnum;
	int datasize;
	unsigned int w, h, d;
	unsigned int blockwidth, blockheight, blockdepth, blockbytes;
	struct pendingtextureinfo *mips;
	int encoding;
	int layers = 1, layer;
	int ttype;

	ddsheader_t fmtheader;
	dds10header_t fmt10header;
	qbyte *fileend = filedata + filesize;

	if (filesize < sizeof(fmtheader) || *(int*)filedata != (('D'<<0)|('D'<<8)|('S'<<16)|(' '<<24)))
		return NULL;

	memcpy(&fmtheader, filedata+4, sizeof(fmtheader));
	if (fmtheader.dwSize != sizeof(fmtheader))
		return NULL;	//corrupt/different version
	fmtheader.dwSize += 4;
	memset(&fmt10header, 0, sizeof(fmt10header));

	fmt10header.arraysize = (fmtheader.ddsCaps[1] & 0x200)?6:1; //cubemaps need 6 faces...

	nummips = fmtheader.dwMipMapCount;
	if (nummips < 1)
		nummips = 1;
	if (nummips > countof(mips->mip))
		return NULL;

	if (!(fmtheader.ddpfPixelFormat.dwFlags & 4))
	{
#define IsPacked(bits,r,g,b,a)	fmtheader.ddpfPixelFormat.bitcount==bits&&fmtheader.ddpfPixelFormat.redmask==r&&fmtheader.ddpfPixelFormat.greenmask==g&&fmtheader.ddpfPixelFormat.bluemask==b&&fmtheader.ddpfPixelFormat.alphamask==a
		if (IsPacked(24, 0xff0000, 0x00ff00, 0x0000ff, 0))
			encoding = PTI_BGR8;
		else if (IsPacked(24, 0x000000, 0x00ff00, 0xff0000, 0))
			encoding = PTI_RGB8;
		else if (IsPacked(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			encoding = PTI_BGRA8;
		else if (IsPacked(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			encoding = PTI_RGBA8;
		else if (IsPacked(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0))
			encoding = PTI_BGRX8;
		else if (IsPacked(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0))
			encoding = PTI_RGBX8;
		else if (IsPacked(32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000))
			encoding = PTI_A2BGR10;
		else if (IsPacked(16, 0xf800, 0x07e0, 0x001f, 0))
			encoding = PTI_RGB565;
		else if (IsPacked(16, 0xf800, 0x07c0, 0x003e, 0x0001))
			encoding = PTI_RGBA5551;
		else if (IsPacked(16, 0x7c00, 0x03e0, 0x001f, 0x8000))
			encoding = PTI_ARGB1555;
		else if (IsPacked(16, 0xf000, 0x0f00, 0x00f0, 0x000f))
			encoding = PTI_RGBA4444;
		else if (IsPacked(16, 0x0f00, 0x00f0, 0x000f, 0xf000))
			encoding = PTI_ARGB4444;
		else if (IsPacked( 8, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
			encoding = (fmtheader.ddpfPixelFormat.dwFlags&0x20000)?PTI_L8:PTI_R8;
		else if (IsPacked(16, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
			encoding = PTI_L8A8;
		else
		{
			Con_Printf("Unsupported non-fourcc dds in %s\n", fname);
			Con_Printf(" bits: %u\n", fmtheader.ddpfPixelFormat.bitcount);
			Con_Printf("  red: %08x\n", fmtheader.ddpfPixelFormat.redmask);
			Con_Printf("green: %08x\n", fmtheader.ddpfPixelFormat.greenmask);
			Con_Printf(" blue: %08x\n", fmtheader.ddpfPixelFormat.bluemask);
			Con_Printf("alpha: %08x\n", fmtheader.ddpfPixelFormat.alphamask);
			Con_Printf(" used: %08x\n", fmtheader.ddpfPixelFormat.redmask^fmtheader.ddpfPixelFormat.greenmask^fmtheader.ddpfPixelFormat.bluemask^fmtheader.ddpfPixelFormat.alphamask);
			return NULL;
		}
#undef IsPacked
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24)))
		encoding = PTI_BC1_RGBA;	//alpha or not? Assume yes, and let the drivers decide.
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('2'<<24)))	//dx3 with premultiplied alpha
	{
//		if (!(tex->flags & IF_PREMULTIPLYALPHA))
//			return false;
		encoding = PTI_BC2_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('3'<<24)))
	{
//		if (tex->flags & IF_PREMULTIPLYALPHA)
//			return false;
		encoding = PTI_BC2_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('4'<<24)))	//dx5 with premultiplied alpha
	{
//		if (!(tex->flags & IF_PREMULTIPLYALPHA))
//			return false;
		encoding = PTI_BC3_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('5'<<24)))
	{
//		if (tex->flags & IF_PREMULTIPLYALPHA)
//			return false;
		encoding = PTI_BC3_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('A'<<0)|('T'<<8)|('I'<<16)|('1'<<24))
		||   *(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('4'<<16)|('U'<<24)))
		encoding = PTI_BC4_R;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('A'<<0)|('T'<<8)|('I'<<16)|('2'<<24))
		||   *(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('5'<<16)|('U'<<24)))
		encoding = PTI_BC5_RG;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('4'<<16)|('S'<<24)))
		encoding = PTI_BC4_R_SNORM;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('5'<<16)|('S'<<24)))
		encoding = PTI_BC5_RG_SNORM;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('E'<<0)|('T'<<8)|('C'<<16)|('2'<<24)))
		encoding = PTI_ETC2_RGB8;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('1'<<16)|('0'<<24)))
	{
		//this has some weird extra header with dxgi format types.
		memcpy(&fmt10header, filedata+fmtheader.dwSize, sizeof(fmt10header));
		fmtheader.dwSize += sizeof(fmt10header);
		switch(fmt10header.dxgiformat)
		{
		case 0x0/*DXGI_FORMAT_UNKNOWN*/:				encoding = PTI_INVALID;			break;
		case 0x1/*DXGI_FORMAT_R32G32B32A32_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0x2/*DXGI_FORMAT_R32G32B32A32_FLOAT*/:		encoding = PTI_RGBA32F;			break;
//		case 0x3/*DXGI_FORMAT_R32G32B32A32_UINT*/:		encoding = PTI_INVALID;			break;
//		case 0x4/*DXGI_FORMAT_R32G32B32A32_SINT*/:		encoding = PTI_INVALID;			break;
//		case 0x5/*DXGI_FORMAT_R32G32B32_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x6/*DXGI_FORMAT_R32G32B32_FLOAT*/:		encoding = PTI_RGB32F;			break;
//		case 0x7/*DXGI_FORMAT_R32G32B32_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x8/*DXGI_FORMAT_R32G32B32_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x9/*DXGI_FORMAT_R16G16B16A16_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0xa/*DXGI_FORMAT_R16G16B16A16_FLOAT*/:		encoding = PTI_RGBA16F;			break;
		case 0xb/*DXGI_FORMAT_R16G16B16A16_UNORM*/:		encoding = PTI_RGBA16;			break;
//		case 0xc/*DXGI_FORMAT_R16G16B16A16_UINT*/:		encoding = PTI_INVALID;			break;
//		case 0xd/*DXGI_FORMAT_R16G16B16A16_SNORM*/:		encoding = PTI_INVALID;			break;
//		case 0xe/*DXGI_FORMAT_R16G16B16A16_SINT*/:		encoding = PTI_INVALID;			break;
//		case 0xf/*DXGI_FORMAT_R32G32_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x10/*DXGI_FORMAT_R32G32_FLOAT*/:			encoding = PTI_INVALID;			break;
//		case 0x11/*DXGI_FORMAT_R32G32_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x12/*DXGI_FORMAT_R32G32_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x13/*DXGI_FORMAT_R32G8X24_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x14/*DXGI_FORMAT_D32_FLOAT_S8X24_UINT*/:	encoding = PTI_DEPTH32_8;		break;
//		case 0x15/*DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS*/:encoding = PTI_INVALID;		break;
//		case 0x16/*DXGI_FORMAT_X32_TYPELESS_G8X24_UINT*/:encoding = PTI_INVALID;		break;
//		case 0x17/*DXGI_FORMAT_R10G10B10A2_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0x18/*DXGI_FORMAT_R10G10B10A2_UNORM*/:		encoding = PTI_A2BGR10;			break;
//		case 0x19/*DXGI_FORMAT_R10G10B10A2_UINT*/:		encoding = PTI_INVALID;			break;
		case 0x1a/*DXGI_FORMAT_R11G11B10_FLOAT*/:		encoding = PTI_B10G11R11F;		break;
//		case 0x1b/*DXGI_FORMAT_R8G8B8A8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x1c/*DXGI_FORMAT_R8G8B8A8_UNORM*/:		encoding = PTI_RGBA8;			break;
		case 0x1d/*DXGI_FORMAT_R8G8B8A8_UNORM_SRGB*/:	encoding = PTI_RGBA8_SRGB;		break;
//		case 0x1e/*DXGI_FORMAT_R8G8B8A8_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x1f/*DXGI_FORMAT_R8G8B8A8_SNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x20/*DXGI_FORMAT_R8G8B8A8_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x21/*DXGI_FORMAT_R16G16_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x22/*DXGI_FORMAT_R16G16_FLOAT*/:			encoding = PTI_INVALID;			break;
//		case 0x23/*DXGI_FORMAT_R16G16_UNORM*/:			encoding = PTI_INVALID;			break;
//		case 0x24/*DXGI_FORMAT_R16G16_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x25/*DXGI_FORMAT_R16G16_SNORM*/:			encoding = PTI_INVALID;			break;
//		case 0x26/*DXGI_FORMAT_R16G16_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x27/*DXGI_FORMAT_R32_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x28/*DXGI_FORMAT_D32_FLOAT*/:				encoding = PTI_DEPTH32;			break;
		case 0x29/*DXGI_FORMAT_R32_FLOAT*/:				encoding = PTI_R32F;			break;
//		case 0x2a/*DXGI_FORMAT_R32_UINT*/:				encoding = PTI_INVALID;			break;
//		case 0x2b/*DXGI_FORMAT_R32_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x2c/*DXGI_FORMAT_R24G8_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x2d/*DXGI_FORMAT_D24_UNORM_S8_UINT*/:		encoding = PTI_DEPTH24_8;		break;
//		case 0x2e/*DXGI_FORMAT_R24_UNORM_X8_TYPELESS*/:	encoding = PTI_INVALID;			break;
//		case 0x2f/*DXGI_FORMAT_X24_TYPELESS_G8_UINT*/:	encoding = PTI_INVALID;			break;
//		case 0x30/*DXGI_FORMAT_R8G8_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x31/*DXGI_FORMAT_R8G8_UNORM*/:			encoding = PTI_RG8;				break;
//		case 0x32/*DXGI_FORMAT_R8G8_UINT*/:				encoding = PTI_INVALID;			break;
		case 0x33/*DXGI_FORMAT_R8G8_SNORM*/:			encoding = PTI_RG8_SNORM;		break;
//		case 0x34/*DXGI_FORMAT_R8G8_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x35/*DXGI_FORMAT_R16_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x36/*DXGI_FORMAT_R16_FLOAT*/:				encoding = PTI_R16F;			break;
		case 0x37/*DXGI_FORMAT_D16_UNORM*/:				encoding = PTI_DEPTH16;			break;
		case 0x38/*DXGI_FORMAT_R16_UNORM*/:				encoding = PTI_R16;				break;
//		case 0x39/*DXGI_FORMAT_R16_UINT*/:				encoding = PTI_INVALID;			break;
//		case 0x3a/*DXGI_FORMAT_R16_SNORM*/:				encoding = PTI_INVALID;			break;
//		case 0x3b/*DXGI_FORMAT_R16_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x3c/*DXGI_FORMAT_R8_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x3d/*DXGI_FORMAT_R8_UNORM*/:				encoding = PTI_R8;				break;
//		case 0x3e/*DXGI_FORMAT_R8_UINT*/:				encoding = PTI_INVALID;			break;
		case 0x3f/*DXGI_FORMAT_R8_SNORM*/:				encoding = PTI_R8_SNORM;		break;
//		case 0x40/*DXGI_FORMAT_R8_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x41/*DXGI_FORMAT_A8_UNORM*/:				encoding = PTI_A8;				break;
//		case 0x42/*DXGI_FORMAT_R1_UNORM*/:				encoding = PTI_INVALID;			break;
		case 0x43/*DXGI_FORMAT_R9G9B9E5_SHAREDEXP*/:	encoding = PTI_E5BGR9;			break;
//		case 0x44/*DXGI_FORMAT_R8G8_B8G8_UNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x45/*DXGI_FORMAT_G8R8_G8B8_UNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x46/*DXGI_FORMAT_BC1_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x47/*DXGI_FORMAT_BC1_UNORM*/:				encoding = PTI_BC1_RGBA;		break;
		case 0x48/*DXGI_FORMAT_BC1_UNORM_SRGB*/:		encoding = PTI_BC1_RGBA_SRGB;	break;
//		case 0x49/*DXGI_FORMAT_BC2_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x4a/*DXGI_FORMAT_BC2_UNORM*/:				encoding = PTI_BC2_RGBA;		break;
		case 0x4b/*DXGI_FORMAT_BC2_UNORM_SRGB*/:		encoding = PTI_BC2_RGBA_SRGB;	break;
//		case 0x4c/*DXGI_FORMAT_BC3_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x4d/*DXGI_FORMAT_BC3_UNORM*/:				encoding = PTI_BC3_RGBA;		break;
		case 0x4e/*DXGI_FORMAT_BC3_UNORM_SRGB*/:		encoding = PTI_BC3_RGBA_SRGB;	break;
//		case 0x4f/*DXGI_FORMAT_BC4_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x50/*DXGI_FORMAT_BC4_UNORM*/:				encoding = PTI_BC4_R;			break;
		case 0x51/*DXGI_FORMAT_BC4_SNORM*/:				encoding = PTI_BC4_R_SNORM;		break;
//		case 0x52/*DXGI_FORMAT_BC5_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x53/*DXGI_FORMAT_BC5_UNORM*/:				encoding = PTI_BC5_RG;			break;
		case 0x54/*DXGI_FORMAT_BC5_SNORM*/:				encoding = PTI_BC5_RG_SNORM;	break;
		case 0x55/*DXGI_FORMAT_B5G6R5_UNORM*/:			encoding = PTI_RGB565;			break;
		case 0x56/*DXGI_FORMAT_B5G5R5A1_UNORM*/:		encoding = PTI_ARGB1555;		break;
		case 0x57/*DXGI_FORMAT_B8G8R8A8_UNORM*/:		encoding = PTI_BGRA8;			break;
		case 0x58/*DXGI_FORMAT_B8G8R8X8_UNORM*/:		encoding = PTI_BGRX8;			break;
//		case 0x59/*DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM*/:encoding = PTI_INVALID;		break;
//		case 0x5a/*DXGI_FORMAT_B8G8R8A8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x5b/*DXGI_FORMAT_B8G8R8A8_UNORM_SRGB*/:	encoding = PTI_BGRA8_SRGB;		break;
//		case 0x5c/*DXGI_FORMAT_B8G8R8X8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x5d/*DXGI_FORMAT_B8G8R8X8_UNORM_SRGB*/:	encoding = PTI_BGRX8_SRGB;		break;
//		case 0x5e/*DXGI_FORMAT_BC6H_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x5f/*DXGI_FORMAT_BC6H_UF16*/:				encoding = PTI_BC6_RGB_UFLOAT;	break;
		case 0x60/*DXGI_FORMAT_BC6H_SF16*/:				encoding = PTI_BC6_RGB_SFLOAT;	break;
//		case 0x61/*DXGI_FORMAT_BC7_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x62/*DXGI_FORMAT_BC7_UNORM*/:				encoding = PTI_BC7_RGBA;		break;
		case 0x63/*DXGI_FORMAT_BC7_UNORM_SRGB*/:		encoding = PTI_BC7_RGBA_SRGB;	break;
//		case 0x64/*DXGI_FORMAT_AYUV*/:					encoding = PTI_INVALID;			break;
//		case 0x65/*DXGI_FORMAT_Y410*/:					encoding = PTI_INVALID;			break;
//		case 0x66/*DXGI_FORMAT_Y416*/:					encoding = PTI_INVALID;			break;
//		case 0x67/*DXGI_FORMAT_NV12*/:					encoding = PTI_INVALID;			break;
//		case 0x68/*DXGI_FORMAT_P010*/:					encoding = PTI_INVALID;			break;
//		case 0x69/*DXGI_FORMAT_P016*/:					encoding = PTI_INVALID;			break;
//		case 0x6a/*DXGI_FORMAT_420_OPAQUE*/:			encoding = PTI_INVALID;			break;
//		case 0x6b/*DXGI_FORMAT_YUY2*/:					encoding = PTI_INVALID;			break;
//		case 0x6c/*DXGI_FORMAT_Y210*/:					encoding = PTI_INVALID;			break;
//		case 0x6d/*DXGI_FORMAT_Y216*/:					encoding = PTI_INVALID;			break;
//		case 0x6e/*DXGI_FORMAT_NV11*/:					encoding = PTI_INVALID;			break;
//		case 0x6f/*DXGI_FORMAT_AI44*/:					encoding = PTI_INVALID;			break;
//		case 0x70/*DXGI_FORMAT_IA44*/:					encoding = PTI_INVALID;			break;
//		case 0x71/*DXGI_FORMAT_P8*/:					encoding = PTI_INVALID;			break;
//		case 0x72/*DXGI_FORMAT_A8P8*/:					encoding = PTI_INVALID;			break;
		case 0x73/*DXGI_FORMAT_B4G4R4A4_UNORM*/:		encoding = PTI_ARGB4444;		break;
//		case 0x82/*DXGI_FORMAT_P208*/:					encoding = PTI_INVALID;			break;
//		case 0x83/*DXGI_FORMAT_V208*/:					encoding = PTI_INVALID;			break;
//		case 0x84/*DXGI_FORMAT_V408*/:					encoding = PTI_INVALID;			break;
		case 134:	encoding = PTI_ASTC_4X4_LDR;	break;
		case 135:	encoding = PTI_ASTC_4X4_SRGB;	break;
		case 138:	encoding = PTI_ASTC_5X4_LDR;	break;
		case 139:	encoding = PTI_ASTC_5X4_SRGB;	break;
		case 142:	encoding = PTI_ASTC_5X5_LDR;	break;
		case 143:	encoding = PTI_ASTC_5X5_SRGB;	break;
		case 146:	encoding = PTI_ASTC_6X5_LDR;	break;
		case 147:	encoding = PTI_ASTC_6X5_SRGB;	break;
		case 150:	encoding = PTI_ASTC_6X6_LDR;	break;
		case 151:	encoding = PTI_ASTC_6X6_SRGB;	break;
		case 154:	encoding = PTI_ASTC_8X5_LDR;	break;
		case 155:	encoding = PTI_ASTC_8X5_SRGB;	break;
		case 158:	encoding = PTI_ASTC_8X6_LDR;	break;
		case 159:	encoding = PTI_ASTC_8X6_SRGB;	break;
		case 162:	encoding = PTI_ASTC_8X8_LDR;	break;
		case 163:	encoding = PTI_ASTC_8X8_SRGB;	break;
		case 166:	encoding = PTI_ASTC_10X5_LDR;	break;
		case 167:	encoding = PTI_ASTC_10X5_SRGB;	break;
		case 170:	encoding = PTI_ASTC_10X6_LDR;	break;
		case 171:	encoding = PTI_ASTC_10X6_SRGB;	break;
		case 174:	encoding = PTI_ASTC_10X8_LDR;	break;
		case 175:	encoding = PTI_ASTC_10X8_SRGB;	break;
		case 178:	encoding = PTI_ASTC_10X10_LDR;	break;
		case 179:	encoding = PTI_ASTC_10X10_SRGB;	break;
		case 182:	encoding = PTI_ASTC_12X10_LDR;	break;
		case 183:	encoding = PTI_ASTC_12X10_SRGB;	break;
		case 186:	encoding = PTI_ASTC_12X12_LDR;	break;
		case 187:	encoding = PTI_ASTC_12X12_SRGB;	break;

		default:
			Con_Printf("Unsupported dds10 dxgi in %s - %u\n", fname, fmt10header.dxgiformat);
			return NULL;
		}
	}
	else
	{
		Con_Printf("Unsupported dds fourcc in %s - \"%c%c%c%c\"\n", fname,
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[0],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[1],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[2],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[3]);
		return NULL;
	}

	if ((fmtheader.ddsCaps[1] & 0x200) && (fmtheader.ddsCaps[1] & 0xfc00) != 0xfc00)
		return NULL;	//cubemap without all 6 faces defined.

	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);
	if (!blockbytes)
		return NULL;	//werid/unsupported

	if (fmtheader.dwFlags & 8)
	{	//explicit pitch flag. we don't support any padding, so this check exists just to be sure none is required.
		w = max(1, fmtheader.dwWidth);
		if (fmtheader.dwPitchOrLinearSize != blockbytes*(w+blockwidth-1)/blockwidth)
			return NULL;
	}
	if (fmtheader.dwFlags & 0x80000)
	{	//linear size flag. we don't support any padding, so this check exists just to be sure none is required.
		//linear-size of the top-level mip.
		size_t linearsize;
		w = max(1, fmtheader.dwWidth);
		h = max(1, fmtheader.dwHeight);
		d = max(1, fmtheader.dwDepth);
		linearsize = ((w+blockwidth-1)/blockwidth)*
							((h+blockheight-1)/blockheight)*
							((d+blockdepth-1)/blockdepth)*
							blockbytes;
		if (fmtheader.dwPitchOrLinearSize != linearsize)
			return NULL;
	}

	if (fmtheader.ddsCaps[1] & 0x200)
	{
		if (fmt10header.arraysize % 6)	//weird number of faces.
			return NULL;

		if (fmt10header.arraysize == 6)
		{
			ttype = PTI_CUBE;
			layers = 6;
		}
		else
		{
			ttype = PTI_CUBE_ARRAY;
			layers = fmt10header.arraysize;
		}
	}
	else if (fmtheader.ddsCaps[1] & 0x200000)
	{
		if (fmt10header.arraysize != 1)	//no 2d arrays
			return NULL;
		ttype = PTI_3D;
	}
	else
	{
		if (fmt10header.arraysize == 1)
			ttype = PTI_2D;
		else
			ttype = PTI_2D_ARRAY;
		layers = fmt10header.arraysize;
	}

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	mips->type = ttype;
	mips->extrafree = filedata;
	mips->encoding = encoding;

	filedata += fmtheader.dwSize;

	w = max(1, fmtheader.dwWidth);
	h = max(1, fmtheader.dwHeight);
	d = max(1, fmtheader.dwDepth);

	if (layers == 1)
	{	//can just use the data without copying.
		for (mipnum = 0; mipnum < nummips; mipnum++)
		{
			datasize = ((w+blockwidth-1)/blockwidth) * ((h+blockheight-1)/blockheight) * ((d+blockdepth-1)/blockdepth) * blockbytes;

			mips->mip[mipnum].data = filedata;
			mips->mip[mipnum].datasize = datasize;
			mips->mip[mipnum].width = w;
			mips->mip[mipnum].height = h;
			mips->mip[mipnum].depth = d;
			filedata += datasize;

			w = max(1, w>>1);
			h = max(1, h>>1);
			d = max(1, d>>1);
		}
		mips->mipcount = mipnum;

		if (filedata > fileend)
		{	//overflow... corrupt dds?
			Z_Free(mips);
			return NULL;
		}
	}
	else
	{	//we need to copy stuff in order to pack it properly. :(
		//allocate space and calc mip sizes
		for (mipnum = 0; mipnum < nummips; mipnum++)
		{
			datasize = ((w+blockwidth-1)/blockwidth) * ((h+blockheight-1)/blockheight) * (layers*((d+blockdepth-1)/blockdepth)) * blockbytes;
			mips->mip[mipnum].data = BZ_Malloc(datasize);
			mips->mip[mipnum].datasize = datasize;
			mips->mip[mipnum].width = w;
			mips->mip[mipnum].height = h;
			mips->mip[mipnum].depth = layers*d;

			w = max(1, w>>1);
			h = max(1, h>>1);
			d = max(1, d>>1);
		}
		mips->mipcount = mipnum;
		//and now copy over the data
		for (layer = 0; layer < layers; layer++)
		{
			for (mipnum = 0; mipnum < nummips; mipnum++)
			{
				datasize = mips->mip[mipnum].datasize/layers;
				if (filedata+datasize > fileend)
				{	//overflow... corrupt dds?
					for (mipnum = 0; mipnum < nummips; mipnum++)
						Z_Free(mips->mip[mipnum].data);
					Z_Free(mips);
					return NULL;
				}
				memcpy((qbyte*)mips->mip[mipnum].data+datasize*layer, filedata, datasize);
				filedata += datasize;
			}
		}
		//and now we're done with the source file. we might as well free it early.
		BZ_Free(mips->extrafree);
		mips->extrafree = NULL;
	}

	return mips;
}

qboolean Image_WriteDDSFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips)
{
	vfsfile_t *file;
	size_t mipnum;
	size_t a;
	dds10header_t h10={0};
	ddsheader_t h9={0};
	int *endian;

	unsigned int blockbytes, blockwidth, blockheight, blockdepth;
	unsigned int arraysize;

	Image_BlockSizeForEncoding(mips->encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	h9.dwSize = sizeof(h9);
	h9.ddpfPixelFormat.dwSize = sizeof(h9.ddpfPixelFormat);
	h9.dwFlags = 0;
	h9.dwFlags |= 1;			//CAPS
	h9.dwFlags |= 2;			//HEIGHT
	h9.dwFlags |= 4;			//WIDTH
	h9.dwFlags |= 0x1000;		//PIXELFORMAT
	if (blockwidth != 1 || blockheight != 1)
	{
		h9.dwFlags |= 0x80000;	//LINEARSIZE
		h9.dwPitchOrLinearSize =	((mips->mip[0].width+blockwidth-1)/blockwidth)*
									((mips->mip[0].height+blockheight-1)/blockheight)*
									(mips->type==PTI_3D?((mips->mip[0].depth+blockdepth-1)/blockdepth):1)*
									blockbytes;
	}
	else
	{
		h9.dwFlags |= 8;		//PITCH
		h9.dwPitchOrLinearSize = mips->mip[0].width*blockbytes;
	}
	if (mips->mipcount > 1)
		h9.dwFlags |= 0x20000;	//MIPMAPCOUNT
	h9.dwWidth = mips->mip[0].width;
	h9.dwHeight = mips->mip[0].height;
	h9.dwDepth = mips->mip[0].depth;

	h9.ddpfPixelFormat.dwSize = 32;
	h9.ddsCaps[0] = 0x1000;		//TEXTURE
	if (mips->mipcount > 1)
		h9.ddsCaps[0] |= 0x8;		//COMPLEX
	h9.ddsCaps[1] = 0;
	h10.miscflag = 0;
	h10.miscflags2 = 0;
	h9.dwMipMapCount = mips->mipcount;

	arraysize = mips->mip[0].depth;
	switch(mips->type)
	{
	case PTI_ANY:
		return false;
	case PTI_3D:
		arraysize = 1;
		h9.ddsCaps[1] |= 0x200000;	//VOLUME
		h10.resourcetype = 4;	//3d
		break;
	case PTI_CUBE_ARRAY:
		if (mips->mip[0].depth <= 1)	//in dds arraysize=1 is NOT an array, leaving us with an ambiguity issue
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		h9.ddsCaps[1] |= 0x200|0xfc00;		//CUBEMAP+faces
		h10.miscflag |= 4;//DDS_RESOURCE_MISC_TEXTURECUBE - otherwise they're basicaly just 2d_arrays
		break;
	case PTI_CUBE:
		if (mips->mip[0].depth != 6)	//wut?!?
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		h9.ddsCaps[1] |= 0x200|0xfc00;		//CUBEMAP+faces
		h10.miscflag |= 4;//DDS_RESOURCE_MISC_TEXTURECUBE - otherwise they're basicaly just 2d_arrays
		break;
	case PTI_2D_ARRAY:
		if (mips->mip[0].depth <= 1)	//in dds arraysize=1 is NOT an array, leaving us with an ambiguity issue
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		break;
	case PTI_2D:
		if (mips->mip[0].depth != 1)	//wut?!?
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		break;
	}
	if (h9.dwMipMapCount > 1)
		h9.ddsCaps[0] |= 0x400000;	//MIPMAP

	h10.arraysize = arraysize;

	h10.dxgiformat = 0;

#define DX9FOURCC(a,b,c,d)		h9.ddpfPixelFormat.dwFlags=4/*DDPF_FOURCC*/,	\
								h9.ddpfPixelFormat.dwFourCC=(a<<0)|(b<<8)|(c<<16)|(d<<24)
#define DX9FMT(bits,r,g,b,a,fl) h9.ddpfPixelFormat.dwFlags=fl,		\
								h9.ddpfPixelFormat.bitcount=bits,	\
								h9.ddpfPixelFormat.redmask=r,		\
								h9.ddpfPixelFormat.greenmask=g,		\
								h9.ddpfPixelFormat.bluemask=b,		\
								h9.ddpfPixelFormat.alphamask=a
#define DX9RGB			0x40
#define DX9RGBA			(0x40|0x1)
#define DX9LUM			0x20000
#define DX9LUMALPHA		(0x20000|0x1)
	safeswitch(mips->encoding)
	{
//	case PTI_INVALID:			h10.dxgiformat = 0x0/*DXGI_FORMAT_UNKNOWN*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1/*DXGI_FORMAT_R32G32B32A32_TYPELESS*/;	break;
	case PTI_RGBA32F:			h10.dxgiformat = 0x2/*DXGI_FORMAT_R32G32B32A32_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3/*DXGI_FORMAT_R32G32B32A32_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4/*DXGI_FORMAT_R32G32B32A32_SINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5/*DXGI_FORMAT_R32G32B32_TYPELESS*/;		break;
	case PTI_RGB32F:			h10.dxgiformat = 0x6/*DXGI_FORMAT_R32G32B32_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x7/*DXGI_FORMAT_R32G32B32_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x8/*DXGI_FORMAT_R32G32B32_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x9/*DXGI_FORMAT_R16G16B16A16_TYPELESS*/;	break;
	case PTI_RGBA16F:			h10.dxgiformat = 0xa/*DXGI_FORMAT_R16G16B16A16_FLOAT*/;		break;
	case PTI_RGBA16:			h10.dxgiformat = 0xb/*DXGI_FORMAT_R16G16B16A16_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xc/*DXGI_FORMAT_R16G16B16A16_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xd/*DXGI_FORMAT_R16G16B16A16_SNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xe/*DXGI_FORMAT_R16G16B16A16_SINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xf/*DXGI_FORMAT_R32G32_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x10/*DXGI_FORMAT_R32G32_FLOAT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x11/*DXGI_FORMAT_R32G32_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x12/*DXGI_FORMAT_R32G32_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x13/*DXGI_FORMAT_R32G8X24_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x14/*DXGI_FORMAT_D32_FLOAT_S8X24_UINT*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x15/*DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS*/;break;
//	case PTI_INVALID:			h10.dxgiformat = 0x16/*DXGI_FORMAT_X32_TYPELESS_G8X24_UINT*/;break;
//	case PTI_INVALID:			h10.dxgiformat = 0x17/*DXGI_FORMAT_R10G10B10A2_TYPELESS*/;	break;
	case PTI_A2BGR10:			h10.dxgiformat = 0x18/*DXGI_FORMAT_R10G10B10A2_UNORM*/;		DX9FMT(32,0x000003ff,0x000ffc00,0x03ff0000,0xc0000000,DX9RGBA);	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x19/*DXGI_FORMAT_R10G10B10A2_UINT*/;		break;
	case PTI_B10G11R11F:		h10.dxgiformat = 0x1a/*DXGI_FORMAT_R11G11B10_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1b/*DXGI_FORMAT_R8G8B8A8_TYPELESS*/;		break;
	case PTI_RGBA8:				h10.dxgiformat = 0x1c/*DXGI_FORMAT_R8G8B8A8_UNORM*/;		DX9FMT(32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000,DX9RGBA);	break;
	case PTI_RGBX8_SRGB:		//fall through...
	case PTI_RGBA8_SRGB:		h10.dxgiformat = 0x1d/*DXGI_FORMAT_R8G8B8A8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1e/*DXGI_FORMAT_R8G8B8A8_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1f/*DXGI_FORMAT_R8G8B8A8_SNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x20/*DXGI_FORMAT_R8G8B8A8_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x21/*DXGI_FORMAT_R16G16_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x22/*DXGI_FORMAT_R16G16_FLOAT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x23/*DXGI_FORMAT_R16G16_UNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x24/*DXGI_FORMAT_R16G16_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x25/*DXGI_FORMAT_R16G16_SNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x26/*DXGI_FORMAT_R16G16_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x27/*DXGI_FORMAT_R32_TYPELESS*/;			break;
	case PTI_DEPTH32:			h10.dxgiformat = 0x28/*DXGI_FORMAT_D32_FLOAT*/;				break;
	case PTI_R32F:				h10.dxgiformat = 0x29/*DXGI_FORMAT_R32_FLOAT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2a/*DXGI_FORMAT_R32_UINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2b/*DXGI_FORMAT_R32_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2c/*DXGI_FORMAT_R24G8_TYPELESS*/;		break;
	case PTI_DEPTH24_8:			h10.dxgiformat = 0x2d/*DXGI_FORMAT_D24_UNORM_S8_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2e/*DXGI_FORMAT_R24_UNORM_X8_TYPELESS*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2f/*DXGI_FORMAT_X24_TYPELESS_G8_UINT*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x30/*DXGI_FORMAT_R8G8_TYPELESS*/;			break;
	case PTI_RG8:				h10.dxgiformat = 0x31/*DXGI_FORMAT_R8G8_UNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x32/*DXGI_FORMAT_R8G8_UINT*/;				break;
	case PTI_RG8_SNORM:			h10.dxgiformat = 0x33/*DXGI_FORMAT_R8G8_SNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x34/*DXGI_FORMAT_R8G8_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x35/*DXGI_FORMAT_R16_TYPELESS*/;			break;
	case PTI_R16F:				h10.dxgiformat = 0x36/*DXGI_FORMAT_R16_FLOAT*/;				break;
	case PTI_DEPTH16:			h10.dxgiformat = 0x37/*DXGI_FORMAT_D16_UNORM*/;				break;
	case PTI_R16:				h10.dxgiformat = 0x38/*DXGI_FORMAT_R16_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x39/*DXGI_FORMAT_R16_UINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3a/*DXGI_FORMAT_R16_SNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3b/*DXGI_FORMAT_R16_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3c/*DXGI_FORMAT_R8_TYPELESS*/;			break;
	case PTI_R8:				h10.dxgiformat = 0x3d/*DXGI_FORMAT_R8_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3e/*DXGI_FORMAT_R8_UINT*/;				break;
	case PTI_R8_SNORM:			h10.dxgiformat = 0x3f/*DXGI_FORMAT_R8_SNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x40/*DXGI_FORMAT_R8_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x41/*DXGI_FORMAT_A8_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x42/*DXGI_FORMAT_R1_UNORM*/;				break;
	case PTI_E5BGR9:			h10.dxgiformat = 0x43/*DXGI_FORMAT_R9G9B9E5_SHAREDEXP*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x44/*DXGI_FORMAT_R8G8_B8G8_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x45/*DXGI_FORMAT_G8R8_G8B8_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x46/*DXGI_FORMAT_BC1_TYPELESS*/;			break;
	case PTI_BC1_RGB:			//fall through...
	case PTI_BC1_RGBA:			h10.dxgiformat = 0x47/*DXGI_FORMAT_BC1_UNORM*/;				DX9FOURCC('D','X','T','1'); break;
	case PTI_BC1_RGB_SRGB:		//fall through...
	case PTI_BC1_RGBA_SRGB:		h10.dxgiformat = 0x48/*DXGI_FORMAT_BC1_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x49/*DXGI_FORMAT_BC2_TYPELESS*/;			break;
	case PTI_BC2_RGBA:			h10.dxgiformat = 0x4a/*DXGI_FORMAT_BC2_UNORM*/;				DX9FOURCC('D','X','T','3'); break;
	case PTI_BC2_RGBA_SRGB:		h10.dxgiformat = 0x4b/*DXGI_FORMAT_BC2_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4c/*DXGI_FORMAT_BC3_TYPELESS*/;			break;
	case PTI_BC3_RGBA:			h10.dxgiformat = 0x4d/*DXGI_FORMAT_BC3_UNORM*/;				DX9FOURCC('D','X','T','5'); break;
	case PTI_BC3_RGBA_SRGB:		h10.dxgiformat = 0x4e/*DXGI_FORMAT_BC3_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4f/*DXGI_FORMAT_BC4_TYPELESS*/;			break;
	case PTI_BC4_R:				h10.dxgiformat = 0x50/*DXGI_FORMAT_BC4_UNORM*/;				/*DX9FOURCC('B','C','4','U');*/ DX9FOURCC('A','T','I','1'); break;
	case PTI_BC4_R_SNORM:		h10.dxgiformat = 0x51/*DXGI_FORMAT_BC4_SNORM*/;				DX9FOURCC('B','C','4','S'); break;
//	case PTI_INVALID:			h10.dxgiformat = 0x52/*DXGI_FORMAT_BC5_TYPELESS*/;			break;
	case PTI_BC5_RG:			h10.dxgiformat = 0x53/*DXGI_FORMAT_BC5_UNORM*/;				/*DX9FOURCC('B','C','5','U');*/ DX9FOURCC('A','T','I','2'); break;
	case PTI_BC5_RG_SNORM:		h10.dxgiformat = 0x54/*DXGI_FORMAT_BC5_SNORM*/;				DX9FOURCC('B','C','5','S'); break;
	case PTI_RGB565:			h10.dxgiformat = 0x55/*DXGI_FORMAT_B5G6R5_UNORM*/;			DX9FMT(16,    0xf800,    0x07e0,    0x001f,    0x0000,DX9RGB);	break;
	case PTI_ARGB1555:			h10.dxgiformat = 0x56/*DXGI_FORMAT_B5G5R5A1_UNORM*/;		DX9FMT(16,    0x7c00,    0x03e0,    0x001f,    0x8000,DX9RGBA);	break;
	case PTI_BGRA8:				h10.dxgiformat = 0x57/*DXGI_FORMAT_B8G8R8A8_UNORM*/;		DX9FMT(32,0x00ff0000,0x0000ff00,0x000000ff,0xff000000,DX9RGBA);	break;
	case PTI_BGRX8:				h10.dxgiformat = 0x58/*DXGI_FORMAT_B8G8R8X8_UNORM*/;		DX9FMT(32,0x00ff0000,0x0000ff00,0x000000ff,0x00000000,DX9RGB);	break;	//WARNING: buggy in gimp (ends up alpha=0)
//	case PTI_INVALID:			h10.dxgiformat = 0x59/*DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORbreak;
//	case PTI_INVALID:			h10.dxgiformat = 0x5a/*DXGI_FORMAT_B8G8R8A8_TYPELESS*/;		break;
	case PTI_BGRA8_SRGB:		h10.dxgiformat = 0x5b/*DXGI_FORMAT_B8G8R8A8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5c/*DXGI_FORMAT_B8G8R8X8_TYPELESS*/;		break;
	case PTI_BGRX8_SRGB:		h10.dxgiformat = 0x5d/*DXGI_FORMAT_B8G8R8X8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5e/*DXGI_FORMAT_BC6H_TYPELESS*/;			break;
	case PTI_BC6_RGB_UFLOAT:	h10.dxgiformat = 0x5f/*DXGI_FORMAT_BC6H_UF16*/;				break;
	case PTI_BC6_RGB_SFLOAT:	h10.dxgiformat = 0x60/*DXGI_FORMAT_BC6H_SF16*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x61/*DXGI_FORMAT_BC7_TYPELESS*/;			break;
	case PTI_BC7_RGBA:			h10.dxgiformat = 0x62/*DXGI_FORMAT_BC7_UNORM*/;				break;
	case PTI_BC7_RGBA_SRGB:		h10.dxgiformat = 0x63/*DXGI_FORMAT_BC7_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x64/*DXGI_FORMAT_AYUV*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x65/*DXGI_FORMAT_Y410*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x66/*DXGI_FORMAT_Y416*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x67/*DXGI_FORMAT_NV12*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x68/*DXGI_FORMAT_P010*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x69/*DXGI_FORMAT_P016*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6a/*DXGI_FORMAT_420_OPAQUE*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6b/*DXGI_FORMAT_YUY2*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6c/*DXGI_FORMAT_Y210*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6d/*DXGI_FORMAT_Y216*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6e/*DXGI_FORMAT_NV11*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6f/*DXGI_FORMAT_AI44*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x70/*DXGI_FORMAT_IA44*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x71/*DXGI_FORMAT_P8*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x72/*DXGI_FORMAT_A8P8*/;					break;
	case PTI_ARGB4444:			h10.dxgiformat = 0x73/*DXGI_FORMAT_B4G4R4A4_UNORM*/;		DX9FMT(16,0x00000f00,0x000000f0,0x0000000f,0x0000f000,DX9RGB);	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x82/*DXGI_FORMAT_P208*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x83/*DXGI_FORMAT_V208*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x84/*DXGI_FORMAT_V408*/;					break;
	case PTI_ASTC_4X4_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_4X4_LDR:		h10.dxgiformat = 0x86/*DXGI_FORMAT_ASTC_4X4_UNORM*/;		break;
	case PTI_ASTC_4X4_SRGB:		h10.dxgiformat = 0x87/*DXGI_FORMAT_ASTC_4X4_SRGB*/;			break;
	case PTI_ASTC_5X4_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_5X4_LDR:		h10.dxgiformat = 0x8a/*DXGI_FORMAT_ASTC_5X4_UNORM*/;		break;
	case PTI_ASTC_5X4_SRGB:		h10.dxgiformat = 0x8b/*DXGI_FORMAT_ASTC_5X4_SRGB*/;			break;
	case PTI_ASTC_5X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_5X5_LDR:		h10.dxgiformat = 0x8e/*DXGI_FORMAT_ASTC_5X5_UNORM*/;		break;
	case PTI_ASTC_5X5_SRGB:		h10.dxgiformat = 0x8f/*DXGI_FORMAT_ASTC_5X5_SRGB*/;			break;
	case PTI_ASTC_6X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_6X5_LDR:		h10.dxgiformat = 0x92/*DXGI_FORMAT_ASTC_6X5_UNORM*/;		break;
	case PTI_ASTC_6X5_SRGB:		h10.dxgiformat = 0x93/*DXGI_FORMAT_ASTC_6X5_SRGB*/;			break;
	case PTI_ASTC_6X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_6X6_LDR:		h10.dxgiformat = 0x96/*DXGI_FORMAT_ASTC_6X6_UNORM*/;		break;
	case PTI_ASTC_6X6_SRGB:		h10.dxgiformat = 0x97/*DXGI_FORMAT_ASTC_6X6_SRGB*/;			break;
	case PTI_ASTC_8X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X5_LDR:		h10.dxgiformat = 0x9a/*DXGI_FORMAT_ASTC_8X5_UNORM*/;		break;
	case PTI_ASTC_8X5_SRGB:		h10.dxgiformat = 0x9b/*DXGI_FORMAT_ASTC_8X5_SRGB*/;			break;
	case PTI_ASTC_8X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X6_LDR:		h10.dxgiformat = 0x9e/*DXGI_FORMAT_ASTC_8X6_UNORM*/;		break;
	case PTI_ASTC_8X6_SRGB:		h10.dxgiformat = 0x9f/*DXGI_FORMAT_ASTC_8X6_SRGB*/;			break;
	case PTI_ASTC_8X8_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X8_LDR:		h10.dxgiformat = 0xa2/*DXGI_FORMAT_ASTC_8X8_UNORM*/;		break;
	case PTI_ASTC_8X8_SRGB:		h10.dxgiformat = 0xa3/*DXGI_FORMAT_ASTC_8X8_SRGB*/;			break;
	case PTI_ASTC_10X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X5_LDR:		h10.dxgiformat = 0xa6/*DXGI_FORMAT_ASTC_10X5_UNORM*/;		break;
	case PTI_ASTC_10X5_SRGB:	h10.dxgiformat = 0xa7/*DXGI_FORMAT_ASTC_10X5_SRGB*/;		break;
	case PTI_ASTC_10X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X6_LDR:		h10.dxgiformat = 0xaa/*DXGI_FORMAT_ASTC_10X6_UNORM*/;		break;
	case PTI_ASTC_10X6_SRGB:	h10.dxgiformat = 0xab/*DXGI_FORMAT_ASTC_10X6_SRGB*/;		break;
	case PTI_ASTC_10X8_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X8_LDR:		h10.dxgiformat = 0xae/*DXGI_FORMAT_ASTC_10X8_UNORM*/;		break;
	case PTI_ASTC_10X8_SRGB:	h10.dxgiformat = 0xaf/*DXGI_FORMAT_ASTC_10X8_SRGB*/;		break;
	case PTI_ASTC_10X10_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_10X10_LDR:	h10.dxgiformat = 0xb2/*DXGI_FORMAT_ASTC_10X10_UNORM*/;		break;
	case PTI_ASTC_10X10_SRGB:	h10.dxgiformat = 0xb3/*DXGI_FORMAT_ASTC_10X10_SRGB*/;		break;
	case PTI_ASTC_12X10_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_12X10_LDR:	h10.dxgiformat = 0xb6/*DXGI_FORMAT_ASTC_12X10_UNORM*/;		break;
	case PTI_ASTC_12X10_SRGB:	h10.dxgiformat = 0xb7/*DXGI_FORMAT_ASTC_12X10_SRGB*/;		break;
	case PTI_ASTC_12X12_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_12X12_LDR:	h10.dxgiformat = 0xba/*DXGI_FORMAT_ASTC_12X12_UNORM*/;		break;
	case PTI_ASTC_12X12_SRGB:	h10.dxgiformat = 0xbb/*DXGI_FORMAT_ASTC_12X12_SRGB*/;		break;
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_3X3X3_LDR:
	case PTI_ASTC_4X3X3_LDR:
	case PTI_ASTC_4X4X3_LDR:
	case PTI_ASTC_4X4X4_LDR:
	case PTI_ASTC_5X4X4_LDR:
	case PTI_ASTC_5X5X4_LDR:
	case PTI_ASTC_5X5X5_LDR:
	case PTI_ASTC_6X5X5_LDR:
	case PTI_ASTC_6X6X5_LDR:
	case PTI_ASTC_6X6X6_LDR:
	case PTI_ASTC_3X3X3_SRGB:
	case PTI_ASTC_4X3X3_SRGB:
	case PTI_ASTC_4X4X3_SRGB:
	case PTI_ASTC_4X4X4_SRGB:
	case PTI_ASTC_5X4X4_SRGB:
	case PTI_ASTC_5X5X4_SRGB:
	case PTI_ASTC_5X5X5_SRGB:
	case PTI_ASTC_6X5X5_SRGB:
	case PTI_ASTC_6X6X5_SRGB:
	case PTI_ASTC_6X6X6_SRGB:	return false;	//no dxgi format assigned that we know of
#endif

	case PTI_RGBX8:				DX9FMT(32,0x000000ff,0x0000ff00,0x00ff0000,0x00000000,DX9RGB);	break;	//WARNING: buggy in gimp (ends up alpha=0)
	case PTI_RGB8:				DX9FMT(24,0x000000ff,0x0000ff00,0x00ff0000,0x00000000,DX9RGB);	break;
	case PTI_BGR8:				DX9FMT(24,0x00ff0000,0x0000ff00,0x000000ff,0x00000000,DX9RGB);	break;
	case PTI_L8:				DX9FMT(8,0x000000ff,0x00000000,0x00000000,0x00000000,DX9LUM);	break;
	case PTI_L8A8:				DX9FMT(16,0x000000ff,0x00000000,0x00000000,0x0000ff00,DX9LUMALPHA);	break;
	case PTI_RGBA5551:			DX9FMT(16,0x0000f800,0x000007c0,0x0000003e,0x00000001,DX9RGBA);	break;	//WARNING: buggy in gimp (ends up greyscale)
	case PTI_RGBA4444:			DX9FMT(16,0x0000f000,0x00000f00,0x000000f0,0x0000000f,DX9RGBA);	break;	//WARNING: buggy in gimp (ends up greyscale)

	case PTI_ETC1_RGB8:			//fall through (etc2 is backwards compatible)
	case PTI_ETC2_RGB8:			DX9FOURCC('E','T','C','2');	break;	//not an official format, but we can understand it
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:	return false;	//unsupported

	case PTI_L8_SRGB:			return false;	//unsupported
	case PTI_L8A8_SRGB:			return false;	//unsupported
	case PTI_RGB8_SRGB:			return false;	//unsupported
	case PTI_BGR8_SRGB:			return false;	//unsupported
	case PTI_DEPTH24:			return false;	//unsupported, should fall back on dx9 formats.
	case PTI_P8:				return false;	//unsupported, technically R8_UNORM but would load back in wrongly.

#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:
#endif
	case PTI_EMULATED:
	case PTI_MAX:
		return false;

	safedefault:	//don't enable in debug builds, so we get warnings for any cases being missed.
		return false;
	}

	//truncate the mip chain if they're dodgy sizes.
	for (mipnum = 1; mipnum < h9.dwMipMapCount; mipnum++)
	{
		size_t m = mipnum;
		size_t p = (mipnum-1);
		if (mips->mip[m].width != max(1,(mips->mip[p].width)>>1) ||
			mips->mip[m].height != max(1,(mips->mip[p].height)>>1))
		{
			h9.dwMipMapCount = mipnum;
			break;
		}
	}

	if (strchr(filename, '*') || strchr(filename, ':'))
		return false;

	if (h9.ddpfPixelFormat.dwFlags && h10.arraysize == 1)
		h10.dxgiformat = 0;	//skip the dx10 header if we can express it as bitmasks. this generally gives better support in external tools (especially gimp)
	else if (h10.dxgiformat)
	{
		h9.ddpfPixelFormat.dwFlags = 0;	//don't get confused. always one or the other.
		DX9FOURCC('D','X','1','0');
	}
	else
		return false;	//legacy-only arrays are not supported.

	file = FS_OpenVFS(filename, "wb", FS_GAMEONLY);
	if (!file)
		return false;
	VFS_WRITE(file, "DDS ", 4);
	for (endian = (int*)&h9; endian < (int*)(&h9+1); endian++)
		*endian = LittleLong(*endian);
	VFS_WRITE(file, &h9, sizeof(h9));
	if (h10.dxgiformat)
	{
		for (endian = (int*)&h10; endian < (int*)(&h10+1); endian++)
			*endian = LittleLong(*endian);
		VFS_WRITE(file, &h10, sizeof(h10));
	}

	//our internal state uses width*height*layers for each mip level (gl-friendly).
	//DDS requires a0m0, a0m1, a1m0, a1m1, so reorder with two nested loops
	for (a = 0; a < arraysize; a++)
	{
		for (mipnum = 0; mipnum < h9.dwMipMapCount; mipnum++)
		{
			size_t sz = mips->mip[mipnum].datasize / arraysize;
			VFS_WRITE(file, (qbyte*)mips->mip[mipnum].data + sz*a, sz);
		}
	}

	VFS_CLOSE(file);
	return true;
}
#endif

#ifdef IMAGEFMT_BLP
static struct pendingtextureinfo *Image_ReadBLPFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	//FIXME: cba with endian.
	int miplevel;
	int w, h, i;
	struct blp_s
	{
		char blp2[4];
		int type;
		qbyte encoding;
		qbyte alphadepth;
		qbyte alphaencoding;
		qbyte hasmips;
		unsigned int xres;
		unsigned int yres;
		unsigned int mipoffset[16];
		unsigned int mipsize[16];
		unsigned int palette[256];
	} *blp;
	unsigned int *tmpmem = NULL;
	unsigned char *in;
	unsigned int inlen;

	struct pendingtextureinfo *mips;

	blp = (void*)filedata;

	if (memcmp(blp->blp2, "BLP2", 4) || blp->type != 1)
		return NULL;

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	mips->type = PTI_2D;

	w = LittleLong(blp->xres);
	h = LittleLong(blp->yres);

	if (blp->encoding == 2)
	{
		//s3tc/dxt
		switch(blp->alphaencoding)
		{
		default:
		case 0: //dxt1
			if (blp->alphadepth)
				mips->encoding = PTI_BC1_RGBA;
			else
				mips->encoding = PTI_BC1_RGB;
			break;
		case 1: //dxt2/3
			mips->encoding = PTI_BC2_RGBA;
			break;
		case 7: //dxt4/5
			mips->encoding = PTI_BC3_RGBA;
			break;
		}
		for (miplevel = 0; miplevel < 16; )
		{
			if (!w && !h)	//shrunk to no size
				break;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			if (!blp->mipoffset[miplevel] || !blp->mipsize[miplevel] || blp->mipoffset[miplevel]+blp->mipsize[miplevel] > filesize)	//no data
				break;
			mips->mip[miplevel].width = w;
			mips->mip[miplevel].height = h;
			mips->mip[miplevel].depth = 1;
			mips->mip[miplevel].data = filedata + LittleLong(blp->mipoffset[miplevel]);
			mips->mip[miplevel].datasize = LittleLong(blp->mipsize[miplevel]);

			miplevel++;
			if (!blp->hasmips || (flags & IF_NOMIPMAP))
				break;
			w >>= 1;
			h >>= 1;
		}
		mips->mipcount = miplevel;
		mips->extrafree = filedata;
	}
	else
	{
		mips->encoding = PTI_BGRA8;
		for (miplevel = 0; miplevel < 16; )
		{
			if (!w && !h)
				break;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			//if we ran out of mips to load, give up.
			if (!blp->mipoffset[miplevel] || !blp->mipsize[miplevel] || blp->mipoffset[miplevel]+blp->mipsize[miplevel] > filesize)
			{
				//if we got at least one mip, cap the mips. might help save some ram? naaah...
				//if this is the first mip, well, its completely fucked.
				break;
			}

			in = filedata + LittleLong(blp->mipoffset[miplevel]);
			inlen = LittleLong(blp->mipsize[miplevel]);

			if (inlen != w*h+((w*h*blp->alphadepth+7)>>3))
			{
				Con_Printf("%s: mip level %i does not contain the correct amount of data\n", fname, miplevel);
				break;
			}

			mips->mip[miplevel].width = w;
			mips->mip[miplevel].height = h;
			mips->mip[miplevel].depth = 1;
			mips->mip[miplevel].datasize = 4*w*h;
			mips->mip[miplevel].data = tmpmem = BZ_Malloc(4*w*h);
			mips->mip[miplevel].needfree = true;

			//load the rgb data first (8-bit paletted)
			for (i = 0; i < w*h; i++)
				tmpmem[i] = blp->palette[*in++] | 0xff000000;

			//and then change the alpha bits accordingly.
			switch(blp->alphadepth)
			{
			case 0:
				//BGRX palette, 8bit
				break;
			case 1:
				//BGRX palette, 8bit
				//1bit trailing alpha
				for (i = 0; i < w*h; i+=8, in++)
				{
					tmpmem[i+0] = (tmpmem[i+0] & 0xffffff) | ((*in&0x01)?0xff000000:0);
					tmpmem[i+1] = (tmpmem[i+1] & 0xffffff) | ((*in&0x02)?0xff000000:0);
					tmpmem[i+2] = (tmpmem[i+2] & 0xffffff) | ((*in&0x04)?0xff000000:0);
					tmpmem[i+3] = (tmpmem[i+3] & 0xffffff) | ((*in&0x08)?0xff000000:0);
					tmpmem[i+4] = (tmpmem[i+4] & 0xffffff) | ((*in&0x10)?0xff000000:0);
					tmpmem[i+5] = (tmpmem[i+5] & 0xffffff) | ((*in&0x20)?0xff000000:0);
					tmpmem[i+6] = (tmpmem[i+6] & 0xffffff) | ((*in&0x40)?0xff000000:0);
					tmpmem[i+7] = (tmpmem[i+7] & 0xffffff) | ((*in&0x80)?0xff000000:0);
				}
				break;
			case 4:
				//BGRX palette, 8bit
				//4bit trailing alpha
				for (i = 0; i < w*h; i++)
					tmpmem[i] = (tmpmem[i] & 0xffffff) | (*in++*0x11000000);
				break;
			case 8:
				//BGRX palette, 8bit
				//8bit trailing alpha
				for (i = 0; i < w*h; i++)
					tmpmem[i] = (tmpmem[i] & 0xffffff) | (*in++<<24);
				break;
			}

			miplevel++;
			if (!blp->hasmips || (flags & IF_NOMIPMAP))
				break;
			w = w>>1;
			h = h>>1;
		}
		BZ_Free(filedata);
		mips->mipcount = miplevel;
	}
	return mips;
}
#endif


//This is for the version command
void Image_PrintInputFormatVersions(void)
{
	int i;
#ifndef S_COLOR_YELLOW
	#define S_COLOR_YELLOW ""
	#define S_COLOR_WHITE ""
	#define S_COLOR_TRANS ""
#endif
	Con_Printf(S_COLOR_YELLOW"Image Formats:"S_COLOR_WHITE);

	#ifdef IMAGEFMT_DDS
		Con_Printf(" dds");
		#ifndef DECOMPRESS_S3TC
			Con_Printf("(hw-only)");
		#endif
	#endif
	#ifdef IMAGEFMT_KTX
		Con_Printf(" ktx");
	#endif
	#ifdef IMAGEFMT_TGA
		Con_Printf(" tga");
	#endif
	#ifdef AVAIL_PNGLIB
		Con_Printf(" png");
		#ifdef DYNAMIC_LIBPNG
			if (!LIBPNG_LOADED())
				Con_Printf(S_COLOR_TRANS"(unavailable, %s)", PNG_LIBPNG_VER_STRING);
			else
				Con_Printf(S_COLOR_TRANS"(dynamic, %s)", PNG_LIBPNG_VER_STRING);
		#else
			Con_Printf(S_COLOR_TRANS"(%s)", PNG_LIBPNG_VER_STRING);
		#endif
	#endif
	#ifdef IMAGEFMT_BMP
		Con_Printf(" bmp+ico");
	#endif
	#ifdef AVAIL_JPEGLIB
		Con_Printf(" jpeg");
		#ifdef DYNAMIC_LIBJPEG
			if (!LIBJPEG_LOADED())
				Con_Printf(S_COLOR_TRANS"(unavailable, %s)", PNG_LIBPNG_VER_STRING);
			else
				Con_Printf(S_COLOR_TRANS"(dynamic, %i, %d series)", JPEG_LIB_VERSION, ( JPEG_LIB_VERSION / 10 ) );
		#else
			Con_Printf(S_COLOR_TRANS"(%i, %d series)", JPEG_LIB_VERSION, ( JPEG_LIB_VERSION / 10 ) );
		#endif
	#endif
	#ifdef IMAGEFMT_BLP
		Con_Printf(" BLP");
	#endif
	#ifdef IMAGEFMT_PBM
		Con_Printf(" pfm+pbm+pgm+ppm"/*"+pam"*/);
	#endif
	#ifdef IMAGEFMT_PSD
		Con_Printf(" psd");
	#endif
	#ifdef IMAGEFMT_HDR
		Con_Printf(" hdr");
	#endif
	#ifdef IMAGEFMT_ASTC
		Con_Printf(" astc");
		#ifndef DECOMPRESS_ASTC
			Con_Printf("(hw-only)");
		#endif
	#endif
	#ifdef IMAGEFMT_PKM
		Con_Printf(" pkm");
	#endif
	#ifdef IMAGEFMT_EXR
		Con_Printf(" exr");
		if (!exr.handle)
			Con_Printf(S_COLOR_TRANS"(unavailable)");
	#endif
	#ifdef IMAGEFMT_PCX
		Con_Printf(" pcx");
	#endif

	#ifdef STB_IMAGE_IMPLEMENTATION
		#ifdef STBI_ONLY_PNG
			Con_Printf(" png"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_JPEG
			Con_Printf(" jpeg"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_BMP
			Con_Printf(" bmp"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_PSD
			Con_Printf(" psd"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_TGA
			Con_Printf(" tga"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_GIF
			Con_Printf(" gif"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_HDR
			Con_Printf(" hdr"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_PIC
			Con_Printf(" pic"S_COLOR_TRANS"(stbi)");
		#endif
		#ifdef STBI_ONLY_PNM
			Con_Printf(" pnm"S_COLOR_TRANS"(stbi)");
		#endif
	#endif

	#ifdef IMAGEFMT_LMP
		Con_Printf(" lmp");
	#endif

	//now properly registered ones.
	for (i = 0; i < imageloader_count;  i++)
		Con_Printf(" ^[%s^]", imageloader[i].funcs->loadername);

	Con_Printf("\n");
}

//if force_rgba8 then it guarentees rgba8 or rgbx8, otherwise can return l8, etc
qbyte *ReadRawImageFile(qbyte *buf, int len, int *width, int *height, uploadfmt_t *format, qboolean force_rgba8, const char *fname)
{
	size_t l, i;
	qbyte *data;
	*format = PTI_RGBX8;
#ifdef IMAGEFMT_TGA
	if ((data = ReadTargaFile(buf, len, width, height, format, false, force_rgba8?PTI_RGBA8:PTI_INVALID)))
		return data;
#endif

#ifdef AVAIL_PNGLIB
	if (len > 4 && (buf[0] == 137 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G') && (data = ReadPNGFile(fname, buf, len, width, height, format, force_rgba8)))
		return data;
#endif
#ifdef AVAIL_JPEGLIB
	//jpeg jfif only.
	if (len > 4 && (buf[0] == 0xff && buf[1] == 0xd8 && buf[2] == 0xff /*&& buf[3] == 0xe0*/) && (data = ReadJPEGFile(buf, len, width, height)))
	{
		*format = PTI_RGBX8;
		return data;
	}
#endif
#ifdef IMAGEFMT_PCX
	if ((data = ReadPCXFile(buf, len, width, height)))
	{
		*format = PTI_RGBA8;
		TRACE(("dbg: ReadRawImageFile: pcx\n"));
		return data;
	}
#endif

#ifdef IMAGEFMT_BMP
	if (len > 2 && (buf[0] == 'B' && buf[1] == 'M') && (data = ReadBMPFile(buf, len, width, height)))
	{
		*format = PTI_RGBA8;
		TRACE(("dbg: ReadRawImageFile: bmp\n"));
		return data;
	}

	if (len > 6 && buf[0]==0&&buf[1]==0 && buf[2]==1&&buf[3]==0 && (data = ReadICOFile(fname, buf, len, width, height, force_rgba8?NULL:format)))
	{
		TRACE(("dbg: ReadRawImageFile: ico\n"));
		return data;
	}
#endif

#ifdef IMAGEFMT_PBM
	if (!force_rgba8 && len > 2 && buf[0] == 'P' && ((buf[1] >= '1' && buf[1] <= '7') || buf[1] == 'F' || buf[1] == 'f') && (data = ReadPBMFile(buf, len, fname, width, height, format)))
		return data;
#endif
#ifdef IMAGEFMT_HDR
	if (!force_rgba8 && len > 10 && (!strncmp(buf, "#?RADIANCE", 10)||!strncmp(buf, "#?RGBE", 6)) && (data = ReadRadianceFile(buf, len, fname, width, height, format)))
		return data;
#endif
#ifdef IMAGEFMT_PSD
	if (!force_rgba8 && len > 26 && !strncmp(buf, "8BPS", 4) && (data = ReadPSDFile(buf, len, fname, width, height, format)))
		return data;
#endif
#ifdef IMAGEFMT_XCF
	if (len > 9 && !strncmp(buf, "gimp xcf ", 9) && (data = ReadXCFFile(buf, len, fname, width, height, force_rgba8?NULL:format)))
		return data;
#endif

#ifdef IMAGEFMT_EXR
	if (!force_rgba8 && len > 4 && (buf[0]|(buf[1]<<8)|(buf[2]<<16)|(buf[3]<<24)) == 20000630 && (data = ReadEXRFile(buf, len, fname, width, height, format)))
		return data;
#endif

#ifdef STB_IMAGE_IMPLEMENTATION
	{
		int components;
		//Note: I don't like passing STBI_default, because it'll return 24bit data which we'll then have to pad anyway.
		//but there's no other easy way to check how many channels are actually valid.
		if ((data = stbi_load_from_memory(buf, len, width, height, &components, force_rgba8?4:STBI_default)))
		{
			switch(components)
			{
			case STBI_grey:
				*format = PTI_L8;
				return data;
			case STBI_grey_alpha:
				*format = PTI_L8A8;
				return data;
			case STBI_rgb:
				*format = PTI_RGB8;
				return data;
			case STBI_rgb_alpha:
				*format = PTI_RGBA8;
				return data;
			}
			//erk...?
			stbi_image_free(data);
		}
	}
#endif

	for (l = 0; l < imageloader_count; l++)
	{
		struct pendingtextureinfo *mips = imageloader[l].funcs->ReadImageFile(0, fname, buf, len);
		if (mips)
		{
			if (mips->extrafree != buf)
				Sys_Error("Image loader did weird extrafree things.");
			mips->extrafree = NULL;

			//free any excess mips
			while (mips->mipcount > 1)
				if (mips->mip[--mips->mipcount].needfree)
					BZ_Free(mips->mip[mips->mipcount].data);

			if (mips->mipcount > 0 && mips->type == PTI_2D)
			{
				if (force_rgba8)
				{
					qboolean rgbx8only[PTI_MAX] = {0};
					rgbx8only[PTI_RGBX8] = true;
					rgbx8only[PTI_RGBA8] = true;
					Image_ChangeFormat(mips, rgbx8only, mips->encoding, fname);
				}

				if (mips->mip[0].needfree)
				{
					data = mips->mip[0].data;
					mips->mip[0].data = NULL;
					mips->mip[0].needfree = false;
				}
				else
				{
					data = BZ_Malloc(mips->mip[0].datasize);
					memcpy(data, mips->mip[0].data, mips->mip[0].datasize);
				}
				*width = mips->mip[0].width;
				*height = mips->mip[0].height;
				*format = mips->encoding;
			}
			for (i = 0; i < mips->mipcount; i++)
				if (mips->mip[i].needfree)
					BZ_Free(mips->mip[i].data);

			if (mips->extrafree && mips->extrafree)
				BZ_Free(mips->extrafree);
			BZ_Free(mips);

			if (data)
				return data;
		}
	}

#ifdef IMAGEFMT_LMP
	if (len >= 8)	//.lmp has no magic id. guess at it.
	{
		int w = LittleLong(((int*)buf)[0]);
		int h = LittleLong(((int*)buf)[1]);
		int i;
		if (((w >= 3 && h >= 4)
			||(w==26&&h==1) //hack for hexen2. stupid lack of a magic.
			) && w*h+sizeof(int)*2 == len)
		{	//quake lmp
			if (force_rgba8)
			{
				qboolean foundalpha = false;
				qbyte *in = buf+sizeof(int)*2;
				data = BZ_Malloc(w * h * sizeof(int));
				for (i = 0; i < w * h; i++)
				{
					if (in[i] == 255)
						foundalpha = true;
					((unsigned int*)data)[i] = d_8to24rgbtable[in[i]];
				}
				*width = w;
				*height = h;
				*format = foundalpha?PTI_RGBA8:PTI_RGBX8;
			}
			else
			{
				data = BZ_Malloc(w * h);
				memcpy(data, buf+8, w*h);
				*width = w;
				*height = h;
				*format = TF_TRANS8;
			}
			return data;
		}
		else if (w >= 3 && h >= 4 && w*h+sizeof(int)*2+768+2 == len)
		{	//halflife. should probably verify that those 2 extra bytes read as 256.
			qboolean foundalpha = false;
			qbyte *in = buf+sizeof(int)*2;
			qbyte *palette = in + w*h+2, *p;
			data = BZ_Malloc(w * h * sizeof(int));
			for (i = 0; i < w * h; i++)
			{
				if (in[i] == 255)
					foundalpha = true;
				p = palette + 3*in[i];
				data[(i<<2)+0] = p[2];
				data[(i<<2)+1] = p[1];
				data[(i<<2)+2] = p[0];
				data[(i<<2)+3] = 255;
			}
			*width = w;
			*height = h;
			*format = foundalpha?PTI_RGBA8:PTI_RGBX8;
			return data;
		}
		else if (len == 128*128 || len == 128*256)
		{	//conchars lump (or h2). 0 is transparent.
			qbyte *in = buf;
			h = 128;
			w = len/h;
			data = BZ_Malloc(w * h * sizeof(int));
			for (i = 0; i < w * h; i++)
			{
				((unsigned int*)data)[i] = d_8to24rgbtable[in[i]];
				data[i*4+3] = (in[i] == 0)?0:255;
			}
			*width = w;
			*height = h;
			*format = PTI_RGBA8;
			return data;
		}
	}
#endif

#ifdef Q2BSPS
	if (len >= sizeof(q2miptex_t))	//.lmp has no magic id. guess at it.
	{
		const q2miptex_t *wal = (const q2miptex_t *)buf;
		size_t w = LittleLong(wal->width), h = LittleLong(wal->height);
		size_t sz = sizeof(*wal) +
					(w>>0)*(h>>0) +
					(w>>1)*(h>>1) +
					(w>>2)*(h>>2) +
					(w>>3)*(h>>3);
		if (sz == len)
		{
			if (force_rgba8)
			{
				qboolean foundalpha = false;
				qbyte *in = buf+sizeof(*wal);
				data = BZ_Malloc(w * h * sizeof(int));
				for (i = 0; i < w * h; i++)
				{
//					if (in[i] == 255)
//						foundalpha = true;
					((unsigned int*)data)[i] = d_8to24rgbtable[in[i]];
				}
				*width = w;
				*height = h;
				*format = foundalpha?PTI_RGBA8:PTI_RGBX8;
			}
			else
			{
				data = BZ_Malloc(w * h);
				memcpy(data, buf+sizeof(*wal), w*h);
				*width = w;
				*height = h;
				*format = TF_SOLID8;
			}
			return data;
		}
	}
#endif

	TRACE(("dbg: Read32BitImageFile: life sucks\n"));

	return NULL;
}

static void Image_MipMap1X8 (qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight)
{
	int		i, j;
	qbyte	*inrow;

	int rowwidth = inwidth;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
		out[0] = in[0];
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
			out[0] = (in[0] + in[1])>>1;
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
				out[0] = (in[0] + in[rowwidth+0])>>1;
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
				out[0] = (in[0] + in[1] + in[rowwidth+0] + in[rowwidth+1])>>2;
	}
}

static void Image_MipMap2X8 (qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight)
{
	int		i, j;
	qbyte	*inrow;

	int rowwidth = inwidth*2;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=2, in+=4)
		{
			out[0] = (in[0] + in[2])>>1;
			out[1] = (in[1] + in[3])>>1;
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=2, in+=4)
			{
				out[0] = (in[0] + in[rowwidth+0])>>1;
				out[1] = (in[1] + in[rowwidth+1])>>1;
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=2, in+=4)
			{
				out[0] = (in[0] + in[2] + in[rowwidth+0] + in[rowwidth+2])>>2;
				out[1] = (in[1] + in[3] + in[rowwidth+1] + in[rowwidth+3])>>2;
			}
		}
	}
}

static void Image_MipMap3X8 (qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight)
{
	int		i, j;
	qbyte	*inrow;

	int rowwidth = inwidth*3;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=3, in+=6)
		{
			out[0] = (in[0] + in[3])>>1;
			out[1] = (in[1] + in[4])>>1;
			out[2] = (in[2] + in[5])>>1;
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=3, in+=6)
			{
				out[0] = (in[0] + in[rowwidth+0])>>1;
				out[1] = (in[1] + in[rowwidth+1])>>1;
				out[2] = (in[2] + in[rowwidth+2])>>1;
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=3, in+=6)
			{
				out[0] = (in[0] + in[3] + in[rowwidth+0] + in[rowwidth+3])>>2;
				out[1] = (in[1] + in[4] + in[rowwidth+1] + in[rowwidth+4])>>2;
				out[2] = (in[2] + in[5] + in[rowwidth+2] + in[rowwidth+5])>>2;
			}
		}
	}
}

static void Image_MipMap4X8 (qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight)
{
	int		i, j;
	qbyte	*inrow;

	int rowwidth = inwidth*4;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4])>>1;
			out[1] = (in[1] + in[5])>>1;
			out[2] = (in[2] + in[6])>>1;
			out[3] = (in[3] + in[7])>>1;
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[rowwidth+0])>>1;
				out[1] = (in[1] + in[rowwidth+1])>>1;
				out[2] = (in[2] + in[rowwidth+2])>>1;
				out[3] = (in[3] + in[rowwidth+3])>>1;
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[4] + in[rowwidth+0] + in[rowwidth+4])>>2;
				out[1] = (in[1] + in[5] + in[rowwidth+1] + in[rowwidth+5])>>2;
				out[2] = (in[2] + in[6] + in[rowwidth+2] + in[rowwidth+6])>>2;
				out[3] = (in[3] + in[7] + in[rowwidth+3] + in[rowwidth+7])>>2;
			}
		}
	}
}

//oh how I wish I had C++'s template stuff right now
static void Image_MipMap4X16 (unsigned short *in, int inwidth, int inheight, unsigned short *out, int outwidth, int outheight)
{
	int		i, j;
	unsigned short	*inrow;

	int rowwidth = inwidth*4;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4])>>1;
			out[1] = (in[1] + in[5])>>1;
			out[2] = (in[2] + in[6])>>1;
			out[3] = (in[3] + in[7])>>1;
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[rowwidth+0])>>1;
				out[1] = (in[1] + in[rowwidth+1])>>1;
				out[2] = (in[2] + in[rowwidth+2])>>1;
				out[3] = (in[3] + in[rowwidth+3])>>1;
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[4] + in[rowwidth+0] + in[rowwidth+4])>>2;
				out[1] = (in[1] + in[5] + in[rowwidth+1] + in[rowwidth+5])>>2;
				out[2] = (in[2] + in[6] + in[rowwidth+2] + in[rowwidth+6])>>2;
				out[3] = (in[3] + in[7] + in[rowwidth+3] + in[rowwidth+7])>>2;
			}
		}
	}
}

//unsigned normalised floats, for gl_ext_packed_float.
static float SmallToFloat(unsigned int val, unsigned int size)
{
	unsigned int mantissabits = size-5;
	union
	{
		float f;
		unsigned int u;
	} u;
	unsigned int b;
	unsigned int mmask = (1<<mantissabits)-1;
	u.u = (((val>>mantissabits)&0x1f)-15+127)<<23;	//read exponent, rebias it, and reshift.

	//fold the mantissa multiple times, to try to preserve as much precision as we can.
	for (b = 23; b >= mantissabits; b-= mantissabits)
		u.u |= (val&mmask)<<(b-mantissabits);
	if (b)
		u.u |= (val&mmask)>>(mantissabits-b);
	return u.f;
}
//unsigned normalised floats, for gl_ext_packed_float.
static unsigned int FloatToSmall(float val, unsigned int size)
{
	unsigned int mantissabits = size-5;
	union
	{
		float f;
		unsigned int u;
	} u = {val};
	int e = 0;
	int m;

	e = ((u.u>>23)&0xff) - 127;
	if (e < -15)
		return 0; //too small exponent, treat it as a 0 denormal
	if (e > 15)
		m = 0; //infinity instead of a nan
	else
		m = (u.u&((1<<23)-1))>>(23-mantissabits);
	return ((e+15)<<mantissabits) | m;
}

static float HalfToFloat(unsigned short val)
{
	union
	{
		float f;
		unsigned int u;
	} u;
	if (val&0x7c00)
		u.u = (((val&0x7c00)>>10)-15+127)<<23;	//read exponent, rebias it, and reshift.
	else
		u.u = 0;	//denormal (or 0).
	u.u |= ((val & 0x3ff)<<13);//shift up the mantissa, but don't fold
	u.u |= (val&0x8000)<<16;	//retain the sign bit.
	return u.f;
}
static unsigned short FloatToHalf(float val)
{
	union
	{
		float f;
		unsigned int u;
	} u = {val};
	int e = 0;
	int m;

	e = ((u.u>>23)&0xff) - 127;
	if (e < -15)
		return 0; //too small exponent, treat it as a 0 denormal
	if (e > 15)
		m = 0; //infinity instead of a nan
	else
		m = (u.u&((1<<23)-1))>>13;
	return ((u.u>>16)&0x8000) | ((e+15)<<10) | m;
}
fte_inlinestatic unsigned short HalfFloatBlend2(unsigned short a, unsigned short b)
{
	return FloatToHalf((HalfToFloat(a) + HalfToFloat(b))/2);
}
fte_inlinestatic unsigned short HalfFloatBlend4(unsigned short a, unsigned short b, unsigned short c, unsigned short d)
{
	return FloatToHalf((HalfToFloat(a) + HalfToFloat(b) + HalfToFloat(c) + HalfToFloat(d))/4);
}
static void Image_MipMap4X16F (unsigned short *in, int inwidth, int inheight, unsigned short *out, int outwidth, int outheight)
{
	int		i, j;
	unsigned short	*inrow;

	int rowwidth = inwidth*4;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
		{
			out[0] = HalfFloatBlend2(in[0], in[4]);
			out[1] = HalfFloatBlend2(in[1], in[5]);
			out[2] = HalfFloatBlend2(in[2], in[6]);
			out[3] = HalfFloatBlend2(in[3], in[7]);
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = HalfFloatBlend2(in[0], in[rowwidth+0]);
				out[1] = HalfFloatBlend2(in[1], in[rowwidth+1]);
				out[2] = HalfFloatBlend2(in[2], in[rowwidth+2]);
				out[3] = HalfFloatBlend2(in[3], in[rowwidth+3]);
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = HalfFloatBlend4(in[0], in[4], in[rowwidth+0], in[rowwidth+4]);
				out[1] = HalfFloatBlend4(in[1], in[5], in[rowwidth+1], in[rowwidth+5]);
				out[2] = HalfFloatBlend4(in[2], in[6], in[rowwidth+2], in[rowwidth+6]);
				out[3] = HalfFloatBlend4(in[3], in[7], in[rowwidth+3], in[rowwidth+7]);
			}
		}
	}
}

static void Image_MipMap4X32F (float *in, int inwidth, int inheight, float *out, int outwidth, int outheight)
{
	int		i, j;
	float	*inrow;

	int rowwidth = inwidth*4;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
		out[3] = in[3];
	}
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4])/2;
			out[1] = (in[1] + in[5])/2;
			out[2] = (in[2] + in[6])/2;
			out[3] = (in[3] + in[7])/2;
		}
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[rowwidth+0])/2;
				out[1] = (in[1] + in[rowwidth+1])/2;
				out[2] = (in[2] + in[rowwidth+2])/2;
				out[3] = (in[3] + in[rowwidth+3])/2;
			}
		}
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
		{
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=4, in+=8)
			{
				out[0] = (in[0] + in[4] + in[rowwidth+0] + in[rowwidth+4])/4;
				out[1] = (in[1] + in[5] + in[rowwidth+1] + in[rowwidth+5])/4;
				out[2] = (in[2] + in[6] + in[rowwidth+2] + in[rowwidth+6])/4;
				out[3] = (in[3] + in[7] + in[rowwidth+3] + in[rowwidth+7])/4;
			}
		}
	}
}

static qbyte Image_BlendPalette_2(qbyte a, qbyte b)
{
	return a;
}
static qbyte Image_BlendPalette_4(qbyte a, qbyte b, qbyte c, qbyte d)
{
	return a;
}
//this is expected to be slow, thanks to those two expensive helpers.
static void Image_MipMap8Pal (qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight)
{
	int		i, j;
	qbyte	*inrow;

	int rowwidth = inwidth;	//rowwidth is the byte width of the input
	inrow = in;

	//mips round down, except for when the input is 1. which bugs out.
	if (inwidth <= 1 && inheight <= 1)
		out[0] = in[0];
	else if (inheight <= 1)
	{
		//single row, don't peek at the next
		for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
			out[0] = Image_BlendPalette_2(in[0], in[1]);
	}
	else if (inwidth <= 1)
	{
		//single colum, peek only at this pixel
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
				out[0] = Image_BlendPalette_2(in[0], in[rowwidth]);
	}
	else
	{
		for (i=0 ; i<outheight ; i++, inrow+=rowwidth*2)
			for (in = inrow, j=0 ; j<outwidth ; j++, out+=1, in+=2)
				out[0] = Image_BlendPalette_4(in[0], in[1], in[rowwidth+0], in[rowwidth+1]);
	}
}

void Image_GenerateMips(struct pendingtextureinfo *mips, unsigned int flags)
{
	int mip;

	if (mips->type == PTI_3D)
		return;	//3d mipmaps are more complicated to compute.

	if (flags & IF_NOMIPMAP)
		return;

	if (sh_config.can_genmips && mips->encoding != PTI_P8)
		return;

	if (mips->mip[0].depth != 1)
		return;	//blurgh. we can't deal with layers.

	switch(mips->encoding)
	{
	case TF_TRANS8:
	case TF_H2_TRANS8_0:
	case PTI_P8:
		if (sh_config.can_mipcap)
			return;	//if we can cap mips, do that. it'll save lots of expensive lookups and uglyness.
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap8Pal(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		return;
	case PTI_R8:
	case PTI_R8_SNORM:
	case PTI_L8:
	case PTI_L8_SRGB:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap1X8(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		return;	
	case PTI_RG8:
	case PTI_RG8_SNORM:
	case PTI_L8A8:
	case PTI_L8A8_SRGB:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * 2;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap2X8(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		return;
	case PTI_RGBA32F:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * sizeof(float)*4;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap4X32F(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		break;
	case PTI_RGBA16F:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * sizeof(unsigned short)*4;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap4X16F(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		break;
	case PTI_RGBA16:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * sizeof(unsigned short)*4;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap4X16(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		break;
	case PTI_RGB8_SRGB:
	case PTI_BGR8_SRGB:
	case PTI_RGB8:
	case PTI_BGR8:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * sizeof(qbyte)*3;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap3X8(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		break;
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_BGRA8:
	case PTI_BGRX8:
		for (mip = mips->mipcount; mip < countof(mips->mip); mip++)
		{
			mips->mip[mip].width = mips->mip[mip-1].width >> 1;
			mips->mip[mip].height = mips->mip[mip-1].height >> 1;
			mips->mip[mip].depth = 1;
			if (mips->mip[mip].width < 1 && mips->mip[mip].height < 1)
				break;
			if (mips->mip[mip].width < 1)
				mips->mip[mip].width = 1;
			if (mips->mip[mip].height < 1)
				mips->mip[mip].height = 1;
			mips->mip[mip].datasize = mips->mip[mip].width * mips->mip[mip].height * sizeof(qbyte)*4;
			mips->mip[mip].data = BZ_Malloc(mips->mip[mip].datasize);
			mips->mip[mip].needfree = true;

			Image_MipMap4X8(mips->mip[mip-1].data, mips->mip[mip-1].width, mips->mip[mip-1].height, mips->mip[mip].data, mips->mip[mip].width, mips->mip[mip].height);
			mips->mipcount = mip+1;
		}
		break;
	case PTI_RGBA4444:
	case PTI_RGB565:
	case PTI_RGBA5551:
		return;	//convert to 16bit afterwards. always mipmap at 8 bit, to try to preserve what little precision there is.
	default:
		return;	//not supported.
	}
}

//stolen from DP
//FIXME: optionally support borders as 0,0,0,0
static void Image_Resample32LerpLine (const qbyte *in, qbyte *out, int inwidth, int outwidth)
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
			*out++ = (qbyte) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (qbyte) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (qbyte) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (qbyte) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
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

//yes, this is lordhavok's code too.
//superblur away!
//FIXME: optionally support borders as 0,0,0,0
#define LERPBYTE(i) r = row1[i];out[i] = (qbyte) ((((row2[i] - r) * lerp) >> 16) + r)
static void Image_Resample32Lerp(const void *indata, int inwidth, int inheight, void *outdata, int outwidth, int outheight)
{
	int i, j, r, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth4 = inwidth*4, outwidth4 = outwidth*4;
	qbyte *out;
	const qbyte *inrow;
	qbyte *row1, *row2;

	row1 = alloca(2*(outwidth*4));
	row2 = row1 + (outwidth * 4);

	out = outdata;
	fstep = (int) (inheight*65536.0f/outheight);

	inrow = indata;
	oldy = 0;
	Image_Resample32LerpLine (inrow, row1, inwidth, outwidth);
	Image_Resample32LerpLine (inrow + inwidth4, row2, inwidth, outwidth);
	for (i = 0, f = 0;i < outheight;i++,f += fstep)
	{
		yi = f >> 16;
		if (yi < endy)
		{
			lerp = f & 0xFFFF;
			if (yi != oldy)
			{
				inrow = (const qbyte *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(row1, row2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, row1, inwidth, outwidth);
				Image_Resample32LerpLine (inrow + inwidth4, row2, inwidth, outwidth);
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
				row1 += 16;
				row2 += 16;
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
				row1 += 8;
				row2 += 8;
			}
			if (j & 1)
			{
				LERPBYTE( 0);
				LERPBYTE( 1);
				LERPBYTE( 2);
				LERPBYTE( 3);
				out += 4;
				row1 += 4;
				row2 += 4;
			}
			row1 -= outwidth4;
			row2 -= outwidth4;
		}
		else
		{
			yi = endy;	//don't read off the end
			if (yi != oldy)
			{
				inrow = (const qbyte *)indata + inwidth4*yi;
				if (yi == oldy+1)
					memcpy(row1, row2, outwidth4);
				else
					Image_Resample32LerpLine (inrow, row1, inwidth, outwidth);
				oldy = yi;
			}
			memcpy(out, row1, outwidth4);
			out += outwidth4;	//Fixes a bug from DP.
		}
	}
}


/*
================
GL_ResampleTexture
================
*/
static void Image_ResampleTexture32Nearest (const unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	const unsigned	*inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = outwidth*fracstep;
		j=outwidth;
		while ((j)&3)
		{
			j--;
			frac -= fracstep;
			out[j] = inrow[frac>>16];
		}
		for ( ; j>=4 ;)
		{
			j-=4;
			frac -= fracstep;
			out[j+3] = inrow[frac>>16];
			frac -= fracstep;
			out[j+2] = inrow[frac>>16];
			frac -= fracstep;
			out[j+1] = inrow[frac>>16];
			frac -= fracstep;
			out[j+0] = inrow[frac>>16];
		}
	}
}

static void Image_ResampleTexture8Nearest (const unsigned char *in, int inwidth, int inheight, unsigned char *out,  int outwidth, int outheight)
{
	int		i, j;
	const unsigned	char *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = outwidth*fracstep;
		j=outwidth;
		while ((j)&3)
		{
			j--;
			frac -= fracstep;
			out[j] = inrow[frac>>16];
		}
		for ( ; j>=4 ;)
		{
			j-=4;
			frac -= fracstep;
			out[j+3] = inrow[frac>>16];
			frac -= fracstep;
			out[j+2] = inrow[frac>>16];
			frac -= fracstep;
			out[j+1] = inrow[frac>>16];
			frac -= fracstep;
			out[j+0] = inrow[frac>>16];
		}
	}
}
static void Image_ResampleTexture16Nearest (const unsigned short *in, int inwidth, int inheight, unsigned short *out,  int outwidth, int outheight)
{
	int		i, j;
	const unsigned	short *inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = outwidth*fracstep;
		j=outwidth;
		while ((j)&3)
		{
			j--;
			frac -= fracstep;
			out[j] = inrow[frac>>16];
		}
		for ( ; j>=4 ;)
		{
			j-=4;
			frac -= fracstep;
			out[j+3] = inrow[frac>>16];
			frac -= fracstep;
			out[j+2] = inrow[frac>>16];
			frac -= fracstep;
			out[j+1] = inrow[frac>>16];
			frac -= fracstep;
			out[j+0] = inrow[frac>>16];
		}
	}
}

//returns out on success. if out is null then returns a new BZ_Malloced buffer
void *Image_ResampleTexture (uploadfmt_t format, const void *in, int inwidth, int inheight, void *out,  int outwidth, int outheight)
{
	switch(format)
	{
	case PTI_INVALID:
	default:
		return NULL;
	//we don't care about byte order etc, just channels+sizes.
	case PTI_LLLX8:
	case PTI_LLLA8:
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_BGRA8:
	case PTI_BGRX8:
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
		if (!out)
			out = BZ_Malloc(((outwidth+3)&~3)*outheight*4);
#ifdef HAVE_CLIENT
		if (gl_lerpimages.ival)
#else
		if (1)
#endif
			Image_Resample32Lerp(in, inwidth, inheight, out, outwidth, outheight);	//FIXME: should be sRGB-aware, but probably not a common path on hardware that can actually do srgb.
		else
			Image_ResampleTexture32Nearest(in, inwidth, inheight, out, outwidth, outheight);
		return out;
	case PTI_R32F:
	case PTI_A2BGR10:
	case PTI_E5BGR9:
		if (!out)
			out = BZ_Malloc(((outwidth+3)&~3)*outheight*4);
		Image_ResampleTexture32Nearest(in, inwidth, inheight, out, outwidth, outheight);
		return out;
	case PTI_P8:
	case PTI_L8:
	case PTI_R8:
	case PTI_L8_SRGB:
	case PTI_R8_SNORM:
		if (!out)
			out = BZ_Malloc((outwidth+3)*outheight);
		Image_ResampleTexture8Nearest(in, inwidth, inheight, out, outwidth, outheight);
		return out;
	case PTI_RG8:
	case PTI_RG8_SNORM:
	case PTI_L8A8:
	case PTI_L8A8_SRGB:
	case PTI_R16:
	case PTI_R16F:
	case PTI_RGB565:
	case PTI_RGBA4444:
	case PTI_ARGB4444:
	case PTI_RGBA5551:
	case PTI_ARGB1555:
		if (!out)
			out = BZ_Malloc((outwidth+3)*outheight*2);
		Image_ResampleTexture16Nearest(in, inwidth, inheight, out, outwidth, outheight);
		return out;
	case PTI_RGB8:
	case PTI_BGR8:
	case PTI_RGBA16:
	case PTI_RGBA16F:
	case PTI_RGBA32F:
		return NULL;	//fixmes
	}
}

//ripped from tenebrae
static unsigned int * Image_GenerateNormalMap(qbyte *pixels, unsigned int *nmap, int w, int h, float scale, float offsetscale)
{
	int i, j, wr, hr;
	unsigned char r, g, b, height;
	float sqlen, reciplen, nx, ny, nz;

	const float oneOver255 = 1.0f/255.0f;

	float c, cx, cy, dcx, dcy;

	wr = w;
	hr = h;

	for (i=0; i<h; i++)
	{
		for (j=0; j<w; j++)
		{
			/* Expand [0,255] texel values to the [0,1] range. */
			c = pixels[i*wr + j] * oneOver255;
			/* Expand the texel to its right. */
			cx = pixels[i*wr + (j+1)%wr] * oneOver255;
			/* Expand the texel one up. */
			cy = pixels[((i+1)%hr)*wr + j] * oneOver255;
			dcx = scale * (c - cx);
			dcy = scale * (c - cy);

			/* Normalize the vector. */
			sqlen = dcx*dcx + dcy*dcy + 1;
			reciplen = 1.0f/(float)sqrt(sqlen);
			nx = dcx*reciplen;
			ny = -dcy*reciplen;
			nz = reciplen;

			/* Repack the normalized vector into an RGB unsigned qbyte
			   vector in the normal map image. */
			r = (qbyte) (128 + 127*nx);
			g = (qbyte) (128 + 127*ny);
			b = (qbyte) (128 + 127*nz);

			/* The highest resolution mipmap level always has a
			   unit length magnitude. */
			height = bound(0, (pixels[i*wr + j]*offsetscale)+(255*(1-offsetscale)), 255);
			nmap[i*w+j] = LittleLong((height << 24)|(b << 16)|(g << 8)|(r));	// <AWE> Added support for big endian.
		}
	}

	return &nmap[0];
}

static int Image_GetPicMip(unsigned int flags)
{
	int picmip = 0;
#ifdef HAVE_CLIENT
	if (flags & IF_NOMIPMAP)
		picmip += gl_picmip2d.ival;	//2d stuff gets its own picmip cvar.
	else
	{
		picmip += gl_picmip.ival;

		if (flags & IF_WORLDTEX)
			picmip += gl_picmip_world.ival;
		else if (flags & IF_SPRITETEX)
			picmip += gl_picmip_sprites.ival;
		else
			picmip += gl_picmip_other.ival;
	}
	if (picmip < 0)
		picmip = 0;
#endif
	return picmip;
}

static void Image_RoundDimensions(int *scaled_width, int *scaled_height, int *scaled_depth, unsigned int flags)
{
	if (sh_config.texture_non_power_of_two)	//NPOT is a simple extension that relaxes errors.
	{
		//lax form
		TRACE(("dbg: GL_RoundDimensions: GL_ARB_texture_non_power_of_two\n"));
	}
	else if ((flags & IF_CLAMP) && (flags & IF_NOMIPMAP) && sh_config.texture_non_power_of_two_pic)
	{
		//more strict form
		TRACE(("dbg: GL_RoundDimensions: GL_OES_texture_npot\n"));
	}
	else
	{
		int width = *scaled_width;
		int height = *scaled_height;
		int depth = *scaled_depth;
		for (*scaled_width = 1 ; *scaled_width < width ; *scaled_width<<=1)
			;
		for (*scaled_height = 1 ; *scaled_height < height ; *scaled_height<<=1)
			;
		if ((flags&IF_TEXTYPEMASK) == IF_TEXTYPE_3D)
			for (*scaled_depth = 1 ; *scaled_depth < depth ; *scaled_depth<<=1)
				;

		/*round npot textures down if we're running on an embedded system*/
		if (sh_config.npot_rounddown)
		{
			if (*scaled_width != width)
				*scaled_width >>= 1;
			if (*scaled_height != height)
				*scaled_height >>= 1;
			if ((flags&IF_TEXTYPEMASK) == IF_TEXTYPE_3D)
				if (*scaled_depth != depth)
					*scaled_depth >>= 1;
		}
	}

	if (!(flags & IF_NOPICMIP))
	{
		int picmip = Image_GetPicMip(flags);
		*scaled_width >>= picmip;
		*scaled_height >>= picmip;
		if ((flags&IF_TEXTYPEMASK) == IF_TEXTYPE_3D)
			*scaled_depth >>= picmip;
	}

	TRACE(("dbg: GL_RoundDimensions: %f\n", gl_max_size.value));

	switch((flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT)
	{
	default:
		break;
	case PTI_CUBE:
	case PTI_CUBE_ARRAY:
		if (sh_config.texturecube_maxsize)
		{
			if (*scaled_width > sh_config.texturecube_maxsize)
				*scaled_width = sh_config.texturecube_maxsize;
			if (*scaled_height > sh_config.texturecube_maxsize)
				*scaled_height = sh_config.texturecube_maxsize;
		}
		if (sh_config.texture2darray_maxlayers)
			if (*scaled_depth > sh_config.texture2darray_maxlayers)
				*scaled_depth = sh_config.texture2darray_maxlayers;
		break;
	case PTI_3D:
		if (sh_config.texture3d_maxsize)
		{
			if (*scaled_width > sh_config.texture3d_maxsize)
				*scaled_width = sh_config.texture3d_maxsize;
			if (*scaled_height > sh_config.texture3d_maxsize)
				*scaled_height = sh_config.texture3d_maxsize;
			if (*scaled_depth > sh_config.texture3d_maxsize)
				*scaled_depth = sh_config.texture3d_maxsize;
		}
		break;
	case PTI_2D:
	case PTI_2D_ARRAY:
		if (sh_config.texture2d_maxsize)
		{
			if (*scaled_width > sh_config.texture2d_maxsize)
				*scaled_width = sh_config.texture2d_maxsize;
			if (*scaled_height > sh_config.texture2d_maxsize)
				*scaled_height = sh_config.texture2d_maxsize;
		}
		if (sh_config.texture2darray_maxlayers)
			if (*scaled_depth > sh_config.texture2darray_maxlayers)
				*scaled_depth = sh_config.texture2darray_maxlayers;

#ifdef HAVE_CLIENT
		if (!(flags & (IF_UIPIC|IF_RENDERTARGET)))
		{
			if (gl_max_size.value)
			{
				if (*scaled_width > gl_max_size.value)
					*scaled_width = gl_max_size.value;
				if (*scaled_height > gl_max_size.value)
					*scaled_height = gl_max_size.value;
				if (*scaled_height > gl_max_size.value)
					*scaled_height = gl_max_size.value;
			}
		}
#endif
		break;
	}

	if (*scaled_width < 1)
		*scaled_width = 1;
	if (*scaled_height < 1)
		*scaled_height = 1;
	if (*scaled_depth < 1)
		*scaled_depth = 1;
}

static void Image_Tr_NoTransform(struct pendingtextureinfo *mips, int dummy)
{
}

static void Image_Tr_PalettedtoRGBX8(struct pendingtextureinfo *mips, int alphapix)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		qbyte *out;
		size_t datasize = sizeof(*out)*4*p;
		void *newdata = out = BZ_Malloc(datasize);

		while(p-->0)
		{
			unsigned int l = *in++;
			*out++ = host_basepal[l*3+0];
			*out++ = host_basepal[l*3+1];
			*out++ = host_basepal[l*3+2];
			*out++ = (l==alphapix)?0:255;
		}
		if (mips->mip[mip].needfree)
			BZ_Free(mips->mip[mip].data);
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = newdata;
		mips->mip[mip].datasize = datasize;
	}
}

//may operate in place
static void Image_Tr_RGBX8toPaletted(struct pendingtextureinfo *mips, int args)
{
	unsigned int mip;
	int first=args&0xffff;
	int stop=(args>>16)&0xffff;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		for(; p-->0; in += 4)
			*out++ = GetPaletteIndexRange(first, stop, in[0], in[1], in[2]);
	}
}
static void Image_Tr_RGBA8toPaletted(struct pendingtextureinfo *mips, int args)
{
	unsigned int mip;
	int first=args&0xffff;
	int stop=(args>>16)&0xffff;
	int tr = first?0:255;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		for(; p-->0; in += 4)
		{
			if (in[3] < 128)
				*out++ = tr;
			else
				*out++ = GetPaletteIndexRange(first, stop, in[0], in[1], in[2]);
		}
	}
}

//may operate in place
static void Image_Tr_8888toLuminence(struct pendingtextureinfo *mips, int channels)
{	//channels==1?L8
	//channels==2?L8A8
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte l, a;
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p*channels);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*channels;

		for(; p-->0; in += 4)
		{
			l = (in[0]+in[1]+in[2])/3;
			a = in[3];

			*out++ = l;
			if (channels == 2)
				*out++ = a;
		}
	}
}
//may operate in place
static void Image_Tr_8888to565(struct pendingtextureinfo *mips, int bgra)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bgra)
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 0);//b
				tmp |= ((*in++>>2) << 5);//g
				tmp |= ((*in++>>3) << 11);//r
				in++;
				*out++ = tmp;
			}
		}
		else
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 11);//r
				tmp |= ((*in++>>2) << 5);//g
				tmp |= ((*in++>>3) << 0);//b
				in++;
				*out++ = tmp;
			}
		}
	}
}

static void Image_Tr_8888to1555(struct pendingtextureinfo *mips, int bgra)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bgra)
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 0);//b
				tmp |= ((*in++>>3) << 5);//g
				tmp |= ((*in++>>3) << 10);//r
				tmp |= ((*in++>>7) << 15);//a
				*out++ = tmp;
			}
		}
		else
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 10);//r
				tmp |= ((*in++>>3) << 5);//g
				tmp |= ((*in++>>3) << 0);//b
				tmp |= ((*in++>>7) << 15);//a
				*out++ = tmp;
			}
		}
	}
}

static void Image_Tr_8888to5551(struct pendingtextureinfo *mips, int bgra)	//zomg
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bgra)
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 1);//b
				tmp |= ((*in++>>3) << 6);//g
				tmp |= ((*in++>>3) << 11);//r
				tmp |= ((*in++>>7) << 0);//a
				*out++ = tmp;
			}
		}
		else
		{
			while(p-->0)
			{
				tmp  = ((*in++>>3) << 11);//r
				tmp |= ((*in++>>3) << 6);//g
				tmp |= ((*in++>>3) << 1);//b
				tmp |= ((*in++>>7) << 0);//a
				*out++ = tmp;
			}
		}
	}
}

static void Image_Tr_RGBA5551to8888(struct pendingtextureinfo *mips, int dummy)
{	//expands
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		unsigned char *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			unsigned short l = *in++;
			unsigned char r = (l>> 11)&0x1f;
			unsigned char g = (l>> 6)&0x1f;
			unsigned char b = (l>> 1)&0x1f;
			unsigned char a = (l>> 0)&0x1;
			*out++ = (r<<3)|(r>>2);
			*out++ = (g<<3)|(g>>2);
			*out++ = (b<<3)|(b>>2);
			*out++ = a * 0xff;
		}
		BZ_Free(dofree);
	}
}
static void Image_Tr_ARGB1555to8888(struct pendingtextureinfo *mips, int dummy)
{	//expands
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		unsigned char *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			unsigned short l = *in++;
			unsigned char r = (l>> 0)&0x1f;
			unsigned char g = (l>> 5)&0x1f;
			unsigned char b = (l>>10)&0x1f;
			unsigned char a = (l>>15)&0x1;
			*out++ = (r<<3)|(r>>2);
			*out++ = (g<<3)|(g>>2);
			*out++ = (b<<3)|(b>>2);
			*out++ = a * 0xff;
		}
		BZ_Free(dofree);
	}
}



static void Image_Tr_8888to4444(struct pendingtextureinfo *mips, int bgra)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bgra)
		{
			while(p-->0)
			{
				tmp  = ((*in++>>4) << 4);//b
				tmp |= ((*in++>>4) << 8);//g
				tmp |= ((*in++>>4) << 12);//r
				tmp |= ((*in++>>4) << 0);//a
				*out++ = tmp;
			}
		}
		else
		{
			while(p-->0)
			{
				tmp  = ((*in++>>4) << 12);//r
				tmp |= ((*in++>>4) << 8);//g
				tmp |= ((*in++>>4) << 4);//b
				tmp |= ((*in++>>4) << 0);//a
				*out++ = tmp;
			}
		}
	}
}
//may operate in place
static void Image_Tr_8888toARGB4444(struct pendingtextureinfo *mips, int bgra)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		unsigned short tmp;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(tmp)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bgra)
		{
			while(p-->0)
			{
				tmp  = ((*in++>>4) << 0);//b
				tmp |= ((*in++>>4) << 4);//g
				tmp |= ((*in++>>4) << 8);//r
				tmp |= ((*in++>>4) << 12);//a
				*out++ = tmp;
			}
		}
		else
		{
			while(p-->0)
			{
				tmp  = ((*in++>>4) << 8);//r
				tmp |= ((*in++>>4) << 4);//g
				tmp |= ((*in++>>4) << 0);//b
				tmp |= ((*in++>>4) << 12);//a
				*out++ = tmp;
			}
		}
	}
}

static void Image_Tr_4444to8888(struct pendingtextureinfo *mips, int dummy)
{	//expands
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		unsigned char *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			unsigned short l = *in++;
			*out++ = ((l>>12)&0xf) * 0x11;
			*out++ = ((l>> 8)&0xf) * 0x11;
			*out++ = ((l>> 4)&0xf) * 0x11;
			*out++ = ((l>> 0)&0xf) * 0x11;
		}
		BZ_Free(dofree);
	}
}
static void Image_Tr_ARGB4444to8888(struct pendingtextureinfo *mips, int dummy)
{	//expands
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		unsigned char *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			unsigned short l = *in++;
			*out++ = ((l>> 0)&0xf) * 0x11;
			*out++ = ((l>> 4)&0xf) * 0x11;
			*out++ = ((l>> 8)&0xf) * 0x11;
			*out++ = ((l>>12)&0xf) * 0x11;
		}
		BZ_Free(dofree);
	}
}

static void Image_Tr_4X16to8888(struct pendingtextureinfo *mips, int unused)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*4;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		while(p-->0)
			*out++ = *in++>>8;
	}
}

//in place: E5BGR9->RGBA8
static void Image_Tr_E5BGR9ToByte(struct pendingtextureinfo *mips, int bgr)
{
	unsigned int mip;
	int rs = bgr?18:0;
	int bs = bgr?0:18;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned int *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*4*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*4;

		while(p-->0)
		{
			unsigned int l = *in++;
			float e = rgb9e5tab[l>>27];
			e *= 255; //prescale to bytes.
			*out++ = bound(0, e * ((l>>rs)&0x1ff), 255);
			*out++ = bound(0, e * ((l>> 9)&0x1ff), 255);
			*out++ = bound(0, e * ((l>>bs)&0x1ff), 255);
			*out++ = 255;
		}
	}
}
//expands the data so must always realloc
static void Image_Tr_E5BGR9ToFloat(struct pendingtextureinfo *mips, int dummy)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned int *in = mips->mip[mip].data;
		float *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (1)//!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*4*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*4;

		while(p-->0)
		{
			unsigned int l = *in++;
			float e = rgb9e5tab[l>>27];
			*out++ = e * ((l>> 0)&0x1ff);
			*out++ = e * ((l>> 9)&0x1ff);
			*out++ = e * ((l>>18)&0x1ff);
			*out++ = 1.0;
		}
	}
}
//always out of place
static void Image_Tr_FloatToE5BGR9(struct pendingtextureinfo *mips, int dummy)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		float *in = mips->mip[mip].data;
		unsigned int *out = mips->mip[mip].data;
		float *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].datasize = p*sizeof(*out);
		mips->mip[mip].data = out = BZ_Malloc(mips->mip[mip].datasize);
		for (; p-->0; out++, in+=4)
		{
			int e = 0;
			float m = max(max(in[0], in[1]), in[2]);
			float scale;
			if (m >= 0.5)
			{	//positive exponent
				while (m >= (1<<(e)) && e < 30-15)	//don't do nans.
					e++;
			}
			else
			{	//negative exponent...
				while (m < 1/(1<<-e) && e > -15)	//don't do denormals.
					e--;
			}
			scale = pow(2, e-9);
			*out = ((e+15)<<27);
			*out |= bound(0, (int)(in[0]/scale + 0.5), 0x1ff)<<0;
			*out |= bound(0, (int)(in[1]/scale + 0.5), 0x1ff)<<9;
			*out |= bound(0, (int)(in[2]/scale + 0.5), 0x1ff)<<18;
		}
		BZ_Free(dofree);
	}
}

static void Image_Tr_PackedToFloat(struct pendingtextureinfo *mips, int dummy)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned int *in = mips->mip[mip].data;
		float *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			unsigned int l = *in++;
			*out++ = SmallToFloat(l>> 0, 11);
			*out++ = SmallToFloat(l>>11, 11);
			*out++ = SmallToFloat(l>>22, 10);
			*out++ = 1.0;
		}
		BZ_Free(dofree);
	}
}
//always out of place
static void Image_Tr_FloatToPacked(struct pendingtextureinfo *mips, int dummy)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		float *in = mips->mip[mip].data;
		unsigned int *out = mips->mip[mip].data;
		float *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		mips->mip[mip].datasize = p*sizeof(*out);
		for (; p-->0; out++, in+=4)
		{
			*out  = FloatToSmall(in[0], 11)<< 0;
			*out |= FloatToSmall(in[1], 11)<<11;
			*out |= FloatToSmall(in[2], 10)<<22;
		}
		BZ_Free(dofree);
	}
}

//in place: RGBA16F->RGBA8, or R16F->R8
static void Image_Tr_HalfToByte(struct pendingtextureinfo *mips, int channels)
{
	unsigned int mip;
	qboolean bs = channels == -4;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		int v;
		unsigned short *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*abs(channels);
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bs)
		{	//to bgra
			for (; p>0; p-=4, out+=4, in+=4)
			{
				v = HalfToFloat(in[2])*255;
				out[0] = bound(0, v, 255);
				v = HalfToFloat(in[1])*255;
				out[1] = bound(0, v, 255);
				v = HalfToFloat(in[0])*255;
				out[2] = bound(0, v, 255);
				v = HalfToFloat(in[3])*255;
				out[3] = bound(0, v, 255);
			}
		}
		else
		{
			while(p-->0)
			{
				v = HalfToFloat(*in++)*255;
				*out++ = bound(0, v, 255);
			}
		}
	}
}

//always out of place
static void Image_Tr_RGB32FToFloat(struct pendingtextureinfo *mips, int dummy)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		float *in = mips->mip[mip].data;
		float *out = mips->mip[mip].data;
		float *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = 1.0;
		}
		BZ_Free(dofree);
	}
}

//always out of place
static void Image_Tr_HalfToFloat(struct pendingtextureinfo *mips, int channels)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned short *in = mips->mip[mip].data;
		float *out = mips->mip[mip].data;
		unsigned short *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*channels;
		mips->mip[mip].needfree = true;
		mips->mip[mip].datasize = p*sizeof(*out);
		mips->mip[mip].data = out = BZ_Malloc(mips->mip[mip].datasize);
		while(p-->0)
			*out++ = HalfToFloat(*in++);
		BZ_Free(dofree);
	}
}

//in place: RGBA32F->RGBA16F
static void Image_Tr_FloatToHalf(struct pendingtextureinfo *mips, int channels)
{
	unsigned int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		float *in = mips->mip[mip].data;
		unsigned short *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*channels;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		while(p-->0)
			*out++ = FloatToHalf(*in++);
	}
}

//in place: RGBA32F->RGBA8, or R32F->R8
static void Image_Tr_FloatToByte(struct pendingtextureinfo *mips, int channels)
{
	unsigned int mip;
	int bs = channels == -4;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		int v;
		float *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*abs(channels);
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		if (bs)
		{	//to bgra
			for (; p>0; p-=4, out+=4, in+=4)
			{
				v = in[2]*255;
				out[0] = bound(0, v, 255);
				v = in[1]*255;
				out[1] = bound(0, v, 255);
				v = in[0]*255;
				out[2] = bound(0, v, 255);
				v = in[3]*255;
				out[3] = bound(0, v, 255);
			}
		}
		else
		{
			while(p-->0)
			{
				v = *in++*255;
				*out++ = bound(0, v, 255);
			}
		}
	}
}

static void Image_Tr_ByteToFloat(struct pendingtextureinfo *mips, int channels)
{	//expands
	int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		float *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth*channels;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		mips->mip[mip].datasize = p*sizeof(*out);
		while(p-->0)
			*out++ = *in++/255.0;
		BZ_Free(dofree);
	}
}

//in place, drops trailing bytes
static void Image_Tr_DropBytes(struct pendingtextureinfo *mips, int srcbitsdstbits)
{
	int srcbytes=srcbitsdstbits>>16;
	int dstbytes=srcbitsdstbits&0xffff;
	int mip, b;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*dstbytes);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*dstbytes;

		while(p-->0)
		{
			for (b = 0; b < dstbytes; b++)
				*out++ = *in++;
			in += srcbytes-dstbytes;
		}
	}
}

static void Image_Tr_RG8ToRGXX8(struct pendingtextureinfo *mips, int dummy)
{
	int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		void *dofree = mips->mip[mip].needfree?in:NULL;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		mips->mip[mip].needfree = true;
		mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		mips->mip[mip].datasize = p*sizeof(*out)*4;
		while(p-->0)
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = 0;
			*out++ = 0xff;
			in+=2;
		}
		BZ_Free(dofree);
	}
}

static void Image_Tr_8To10(struct pendingtextureinfo *mips, int dummy)
{
	int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte *in = mips->mip[mip].data;
		unsigned int *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p);
		}
		mips->mip[mip].datasize = p*sizeof(*out);

		while(p-->0)
		{
			*out++= (((in[0]<<2)|(in[0]>>6))<< 0) |
					(((in[1]<<2)|(in[1]>>6))<<10) |
					(((in[2]<<2)|(in[2]>>6))<<20) |
					((in[3]>>6)<<30);
			in+=4;
		}
	}
}
static void Image_Tr_10To8(struct pendingtextureinfo *mips, int dummy)
{
	int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		unsigned int *in = mips->mip[mip].data, v;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*4;

		while(p-->0)
		{
			v = *in++;

			*out++ = (v>>2)&0xff;
			*out++ = (v>>12)&0xff;
			*out++ = (v>>22)&0xff;
			*out++ = ((v>>30)&0x3)*0x55;
		}
	}
}

static void Image_Tr_Swap8888(struct pendingtextureinfo *mips, int dummy)
{
	int mip;
	for (mip = 0; mip < mips->mipcount; mip++)
	{
		qbyte rb, g, br, a;
		qbyte *in = mips->mip[mip].data;
		qbyte *out = mips->mip[mip].data;
		unsigned int p = mips->mip[mip].width*mips->mip[mip].height*mips->mip[mip].depth;
		if (!mips->mip[mip].needfree && !mips->extrafree)
		{
			mips->mip[mip].needfree = true;
			mips->mip[mip].data = out = BZ_Malloc(sizeof(*out)*p*4);
		}
		mips->mip[mip].datasize = p*sizeof(*out)*4;

		while(p-->0)
		{
			a = in[3];
			rb = in[2];
			g = in[1];
			br = in[0];
			in+=4;

			*out++ = rb;
			*out++ = g;
			*out++ = br;
			*out++ = a;
		}
	}
}

typedef union
{
	byte_vec4_t v;
	unsigned int u;
} pixel32_t;
typedef union
{
	unsigned short v[4];
	quint64_t u;
} pixel64_t;
#define etc_expandv(p,x,y,z) p.v[0]|=p.v[0]>>(x),p.v[1]|=p.v[1]>>(y),p.v[2]|=p.v[2]>>(z)
#ifdef DECOMPRESS_ETC2
//FIXME: this is littleendian only...
static void Image_Decode_ETC2_Block_TH_Internal(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, pixel32_t base1, pixel32_t base2, int d, qboolean tmode)
{
	pixel32_t painttable[4];
	int dtab[] = {3,6,11,16,23,32,41,64};	//writing that felt like giving out lottery numbers.
#define etc_addclamptopixel(r,p,d) r.v[0]=bound(0,p.v[0]+d, 255),r.v[1]=bound(0,p.v[1]+d, 255),r.v[2]=bound(0,p.v[2]+d, 255),r.v[3]=0xff
	d = dtab[d];
	if (tmode)
	{
		painttable[0].u = base1.u;
		etc_addclamptopixel(painttable[1], base2, d);
		painttable[2].u = base2.u;
		etc_addclamptopixel(painttable[3], base2, -d);
	}
	else
	{
		etc_addclamptopixel(painttable[0], base1, d);
		etc_addclamptopixel(painttable[1], base1, -d);
		etc_addclamptopixel(painttable[2], base2, d);
		etc_addclamptopixel(painttable[3], base2, -d);
	}
#undef etc_addclamptoint
	//yay, we have our painttable. now use it. also, screw that msb/lsb split.

#define etc2_th_pix(r,i)								\
	if (in[5-(i/8)]&(1<<(i&7))) {						\
		if (in[7-(i/8)]&(1<<(i&7)))	r.u = painttable[3].u;	\
		else						r.u = painttable[2].u;	\
	} else {											\
		if (in[7-(i/8)]&(1<<(i&7)))	r.u = painttable[1].u;	\
		else						r.u = painttable[0].u;	\
	}

	etc2_th_pix(out[0], 0);
	etc2_th_pix(out[1], 4);
	etc2_th_pix(out[2], 8);
	etc2_th_pix(out[3], 12);
	out += w;
	etc2_th_pix(out[0], 1);
	etc2_th_pix(out[1], 5);
	etc2_th_pix(out[2], 9);
	etc2_th_pix(out[3], 13);
	out += w;
	etc2_th_pix(out[0], 2);
	etc2_th_pix(out[1], 6);
	etc2_th_pix(out[2], 10);
	etc2_th_pix(out[3], 14);
	out += w;
	etc2_th_pix(out[0], 3);
	etc2_th_pix(out[1], 7);
	etc2_th_pix(out[2], 11);
	etc2_th_pix(out[3], 15);
#undef etc2_th_pix
}
static void Image_Decode_ETC2_Block_Internal(qbyte *fte_restrict in, pixel32_t *fte_restrict out0, int w, int alphamode)
{
	//the overflow modes are only valid with ETC2.
	//alphamode=1 is used for punchthrough-alpha (which also forces the diff mode)
	static const char tab[8][2] =
	{
		{2,8},
		{5,17},
		{9,29},
		{13,42},
		{18,60},
		{24,80},
		{33,106},
		{47,183}
	};
	int tv;
	pixel32_t base1, base2, base3;
	const char *cw1, *cw2;
	unsigned char R1,G1,B1;
	pixel32_t *out1, *out2, *out3;

	qboolean opaque;

	if (alphamode)
		opaque = in[3]&2;
	else
		opaque = 1;

	if (alphamode || (in[3]&2))	//diffbit, bit 33
	{
		R1=(in[0]>>3)&31;//59+5
		G1=(in[1]>>3)&31;//51+5
		B1=(in[2]>>3)&31;//43+5
		VectorSet(base1.v, (R1<<3)+(R1>>2), (G1<<3)+(G1>>2), (B1<<3)+(B1>>2));
		R1 += (char)((in[0]&3)|((in[0]&4)*0x3f));	//56+3
		if (R1&~0x1f) //R2 overflow = T mode
		{
			Vector4Set(base1.v, ((in[0]&0x18)<<3) | ((in[0]&0x3)<<4), (in[1]&0xf0), ((in[1]&0x0f)<<4), 0xff);
			Vector4Set(base2.v, (in[2]&0xf0), ((in[2]&0x0f)<<4), (in[3]&0xf0), 0xff);
			tv = ((in[3]&0x0c)>>1)|(in[3]&0x01);
			etc_expandv(base1,4,4,4);
			etc_expandv(base2,4,4,4);
			Image_Decode_ETC2_Block_TH_Internal(in, out0, w, base1, base2, tv, true);
			return;
		}
		G1 += (char)((in[1]&3)|((in[1]&4)*0x3f));	//48+3
		if (G1&~0x1f) //G2 overflow = H mode
		{
			Vector4Set(base1.v, ((in[0]&0x78)<<1), ((in[0]&0x07)<<5)|((in[1]&0x10)<<0), ((in[1]&0x08)<<4)|((in[1]&0x03)<<5)|((in[2]&0x80)>>3), 0xff);
			Vector4Set(base2.v, ((in[2]&0x78)<<1), ((in[2]&0x07)<<5)|((in[3]&0x80)>>3), ((in[3]&0x78)<<1), 0xff);
			tv = ((in[3]&0x04)>>1)|(in[3]&0x01);
			etc_expandv(base1,4,4,4);
			etc_expandv(base2,4,4,4);
			Image_Decode_ETC2_Block_TH_Internal(in, out0, w, base1, base2, tv, false);
			return;
		}
		B1 += (char)((in[2]&3)|((in[2]&4)*0x3f));	//40+3
		if (B1&~0x1f) //B2 overflow = Planar mode
		{//origin horizontal, vertical delas, interpolated across the 16 pixels
			VectorSet(base1.v, ((in[0]&0x7f)<<1),((in[0]&0x01)<<7)|((in[1])&0x7e),(in[1]<<7)|((in[2]&0x18)<<2)|((in[2]&0x3)<<3)|((in[3]&0x80)>>5));
			VectorSet(base2.v, ((in[3]&0x7c)<<1)|((in[3]&0x01)<<2),(in[4]&0xfe),((in[4]&1)<<7)|((in[5]&0xf8)>>1));
			VectorSet(base3.v, ((in[5]&0x07)<<5)|((in[6]&0xe0)>>3),((in[6]&0x1f)<<3)|((in[7]&0xc0)>>5),(in[7]&0x3f)<<2);
			etc_expandv(base1,6,7,6);
			etc_expandv(base2,6,7,6);
			etc_expandv(base3,6,7,6);
#define etc2_planar2(r,x,y)				\
	r[x].v[0] =	bound(0,(4*base1.v[0] + x*((short)base2.v[0]-base1.v[0]) + y*((short)base3.v[0]-base1.v[0]) + 2)>>2,0xff),	\
	r[x].v[1] = bound(0,(4*base1.v[1] + x*((short)base2.v[1]-base1.v[1]) + y*((short)base3.v[1]-base1.v[1]) + 2)>>2,0xff),	\
	r[x].v[2] = bound(0,(4*base1.v[2] + x*((short)base2.v[2]-base1.v[2]) + y*((short)base3.v[2]-base1.v[2]) + 2)>>2,0xff),	\
	r[x].v[3] = 0xff
#define etc2_planar(r,y)				\
					etc2_planar2(r,0,y);		\
					etc2_planar2(r,1,y);		\
					etc2_planar2(r,2,y);		\
					etc2_planar2(r,3,y);
			etc2_planar(out0,0);out0 += w;
			etc2_planar(out0,1);out0 += w;
			etc2_planar(out0,2);out0 += w;
			etc2_planar(out0,3);
			return;
		}
		//they should still be 5 bits.
		VectorSet(base2.v, (R1<<3)+(R1>>2), (G1<<3)+(G1>>2), (B1<<3)+(B1>>2));
	}
	else
	{
		VectorSet(base1.v, ((in[0]>>4)&15)*0x11,	/*60+4*/
						 ((in[1]>>4)&15)*0x11,	/*52+4*/
						 ((in[2]>>4)&15)*0x11);	/*44+4*/
		VectorSet(base2.v, ((in[0]>>0)&15)*0x11,	/*56+4*/
						 ((in[1]>>0)&15)*0x11,	/*48+4*/
						 ((in[2]>>0)&15)*0x11);	/*40+4*/
	}

	cw1 = tab[(in[3]>>5)&7];	//37+3
	cw2 = tab[(in[3]>>2)&7];	//34+3

	out1 = out0+w*1;
	out2 = out0+w*2;
	out3 = out0+w*3;

#define etc1_pix(r, base,cw,i)	\
	if (in[7-(i/8)]&(1<<(i&7)))						\
		tv = (in[5-(i/8)]&(1<<(i&7)))?-cw[1]:cw[1];	\
	else if (opaque)								\
		tv = (in[5-(i/8)]&(1<<(i&7)))?-cw[0]:cw[0];	\
	else /*punchthrough alpha mode*/				\
		tv = (in[5-(i/8)]&(1<<(i&7)))?-255:0;		\
	if (tv==-255)									\
		r.u = 0;									\
	else											\
		r.v[0] = bound(0,base.v[0]+tv,0xff),			\
		r.v[1] = bound(0,base.v[1]+tv,0xff),			\
		r.v[2] = bound(0,base.v[2]+tv,0xff),			\
		r.v[3] = 0xff

	etc1_pix(out0[0], base1,cw1,0);
	etc1_pix(out0[1], base1,cw1,4);
	etc1_pix(out1[0], base1,cw1,1);
	etc1_pix(out1[1], base1,cw1,5);

	etc1_pix(out2[2], base2,cw2,10);
	etc1_pix(out2[3], base2,cw2,14);
	etc1_pix(out3[2], base2,cw2,11);
	etc1_pix(out3[3], base2,cw2,15);
	if (in[3]&1)	//flipbit bit 32 - blocks are vertical
	{
		etc1_pix(out0[2], base1,cw1,8);
		etc1_pix(out0[3], base1,cw1,12);
		etc1_pix(out1[2], base1,cw1,9);
		etc1_pix(out1[3], base1,cw1,13);

		etc1_pix(out2[0], base2,cw2,2);
		etc1_pix(out2[1], base2,cw2,6);
		etc1_pix(out3[0], base2,cw2,3);
		etc1_pix(out3[1], base2,cw2,7);
	}
	else
	{
		etc1_pix(out0[2], base2,cw2,8);
		etc1_pix(out0[3], base2,cw2,12);
		etc1_pix(out1[2], base2,cw2,9);
		etc1_pix(out1[3], base2,cw2,13);

		etc1_pix(out2[0], base1,cw1,2);
		etc1_pix(out2[1], base1,cw1,6);
		etc1_pix(out3[0], base1,cw1,3);
		etc1_pix(out3[1], base1,cw1,7);
	}
#undef etc1_pix
}
static void Image_Decode_EAC8U_Block_Internal(qbyte *fte_restrict in, qbyte *fte_restrict out, int stride, qboolean goestoeleven)
{
	static const char tabs[16][8] =
	{
		{-3,-6, -9,-15,2,5,8,14},
		{-3,-7,-10,-13,2,6,9,12},
		{-2,-5, -8,-13,1,4,7,12},
		{-2,-4, -6,-13,1,3,5,12},
		{-3,-6, -8,-12,2,5,7,11},
		{-3,-7, -9,-11,2,6,8,10},
		{-4,-7, -8,-11,3,6,7,10},
		{-3,-5, -8,-11,2,4,7,10},
		{-2,-6, -8,-10,1,5,7,9},
		{-3,-5, -8,-10,1,4,7,9},
		{-2,-4, -8,-10,1,3,7,9},
		{-2,-5, -7,-10,1,4,6,9},
		{-3,-4, -7,-10,2,3,6,9},
		{-1,-2, -3,-10,0,1,2,9},
		{-4,-6, -8, -9,3,5,7,8},
		{-3,-5, -7, -9,2,4,6,8},
	};

	const qbyte base = in[0];
	const qbyte mul = in[1]>>4;
	const char *tab = tabs[in[1]&0xf];
	const quint64_t bits = in[2] | (in[3]<<8) | (in[4]<<16) | (in[5]<<24) | ((quint64_t)in[6]<<32) | ((quint64_t)in[7]<<40);

#define EAC_Pix(r,x,y)	r = bound(0, base + tab[(bits>>((x*4+y)*3))&7] * mul, 255);
#define EAC_Row(y)	EAC_Pix(out[0], 0,y);EAC_Pix(out[1], 1,y);EAC_Pix(out[2], 2,y);EAC_Pix(out[3], 3,y);
	EAC_Row(0);out += stride;EAC_Row(1);out += stride;EAC_Row(2);out += stride;EAC_Row(3);
#undef EAC_Row
#undef EAC_Pix
}
static void Image_Decode_ETC2_RGB8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_ETC2_Block_Internal(in, out, w, false);
}
//punchthrough alpha works by removing interleaved mode releasing a bit that says whether a block can have alpha=0, .
static void Image_Decode_ETC2_RGB8A1_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_ETC2_Block_Internal(in, out, w, true);
}
//ETC2 RGBA's alpha and R11(and RG11) work the same way as each other, but with varying extra blocks with either 8 or 11 bits of valid precision.
static void Image_Decode_ETC2_RGB8A8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_ETC2_Block_Internal(in+8, out, w, false);
	Image_Decode_EAC8U_Block_Internal(in, out->v+3, w*4, false);
}
static void Image_Decode_EAC_R11U_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	pixel32_t r;
	int i = 0;
	Vector4Set(r.v, 0, 0, 0, 0xff);
	for (i = 0; i < 4; i++)
		out[w*0+i] = out[w*1+i] = out[w*2+i] = out[w*3+i] = r;
	Image_Decode_EAC8U_Block_Internal(in, out->v, w*4, false);
}
static void Image_Decode_EAC_RG11U_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	pixel32_t r;
	int i = 0;
	Vector4Set(r.v, 0, 0, 0, 0xff);
	for (i = 0; i < 4; i++)
		out[w*0+i] = out[w*1+i] = out[w*2+i] = out[w*3+i] = r;
	Image_Decode_EAC8U_Block_Internal(in, out->v+0, w*4, false);
	Image_Decode_EAC8U_Block_Internal(in+8, out->v+1, w*4, false);
}
#endif

#ifdef DECOMPRESS_S3TC
static void Image_Decode_S3TC_Block_Internal(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, qbyte blackalpha)
{
	pixel32_t tab[4];
	unsigned int bits;

	Vector4Set(tab[0].v, (in[1]&0xf8), ((in[0]&0xe0)>>3)|((in[1]&7)<<5), (in[0]&0x1f)<<3, 0xff);
	etc_expandv(tab[0],5,6,5);
	Vector4Set(tab[1].v, (in[3]&0xf8), ((in[2]&0xe0)>>3)|((in[3]&7)<<5), (in[2]&0x1f)<<3, 0xff);
	etc_expandv(tab[1],5,6,5);

#define BC1_Lerp(a,as,b,bs,div,c)		((c)[0]=((a)[0]*(as)+(b)[0]*(bs))/(div),(c)[1]=((a)[1]*(as)+(b)[1]*(bs))/(div), (c)[2]=((a)[2]*(as)+(b)[2]*(bs))/(div), (c)[3] = 0xff)
	if ((in[0]|(in[1]<<8)) > (in[2]|(in[3]<<8)))
	{
		BC1_Lerp(tab[0].v,2, tab[1].v,1, 3,tab[2].v);
		BC1_Lerp(tab[0].v,1, tab[1].v,2, 3,tab[3].v);
	}
	else
	{
		BC1_Lerp(tab[0].v,1, tab[1].v,1, 2,tab[2].v);
		Vector4Set(tab[3].v, 0, 0, 0, blackalpha);
	}

	bits = in[4] | (in[5]<<8) | (in[6]<<16) | (in[7]<<24);

#define BC1_Pix(r,i)	r.u = tab[(bits>>((i)*2))&3].u
#define BC1_Row(r,i)	BC1_Pix(r[0],i+0);BC1_Pix(r[1],i+1);BC1_Pix(r[2],i+2);BC1_Pix(r[3],i+3)

	BC1_Row(out, 0);
	out += w;
	BC1_Row(out, 4);
	out += w;
	BC1_Row(out, 8);
	out += w;
	BC1_Row(out, 12);
}
static void Image_Decode_BC1_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_S3TC_Block_Internal(in, out, w, 0xff);
}
static void Image_Decode_BC1A_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_S3TC_Block_Internal(in, out, w, 0);
}
static void Image_Decode_BC2_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_S3TC_Block_Internal(in+8, out, w, 0xff);

	//BC2 has straight 4-bit alpha.
#define BC2_AlphaRow()	\
	out[0].v[3] = in[0]&0x0f; out[0].v[3] |= out[0].v[3]<<4;	\
	out[1].v[3] = in[0]&0xf0; out[1].v[3] |= out[1].v[3]>>4;	\
	out[2].v[3] = in[1]&0x0f; out[2].v[3] |= out[2].v[3]<<4;	\
	out[3].v[3] = in[1]&0xf0; out[3].v[3] |= out[3].v[3]>>4

	BC2_AlphaRow();
	in += 2;out += w;
	BC2_AlphaRow();
	in += 2;out += w;
	BC2_AlphaRow();
	in += 2;out += w;
	BC2_AlphaRow();
#undef BC2_AlphaRow
}
#endif
#ifdef DECOMPRESS_RGTC
static void Image_Decode_RGTC_Block_Internal(qbyte *fte_restrict in, qbyte *fte_restrict out, int stride, qboolean issigned)
{
	quint64_t bits;
	union
	{
		qbyte u;
		char s;
	} tab[8];
	tab[0].u = in[0];
	tab[1].u = in[1];
	if (issigned)
	{
		if (tab[0].s > tab[1].s)
		{
			tab[2].s = (tab[0].s*6 + tab[1].s*1)/7;
			tab[3].s = (tab[0].s*5 + tab[1].s*2)/7;
			tab[4].s = (tab[0].s*4 + tab[1].s*3)/7;
			tab[5].s = (tab[0].s*3 + tab[1].s*4)/7;
			tab[6].s = (tab[0].s*2 + tab[1].s*5)/7;
			tab[7].s = (tab[0].s*1 + tab[1].s*6)/7;
		}
		else
		{
			tab[2].s = (tab[0].s*4 + tab[1].s*1)/5;
			tab[3].s = (tab[0].s*3 + tab[1].s*2)/5;
			tab[4].s = (tab[0].s*2 + tab[1].s*3)/5;
			tab[5].s = (tab[0].s*1 + tab[1].s*4)/5;
			tab[6].s = -128;
			tab[7].s = 127;
		}
	}
	else
	{
		if (tab[0].u > tab[1].u)
		{
			tab[2].u = (tab[0].u*6 + tab[1].u*1)/7;
			tab[3].u = (tab[0].u*5 + tab[1].u*2)/7;
			tab[4].u = (tab[0].u*4 + tab[1].u*3)/7;
			tab[5].u = (tab[0].u*3 + tab[1].u*4)/7;
			tab[6].u = (tab[0].u*2 + tab[1].u*5)/7;
			tab[7].u = (tab[0].u*1 + tab[1].u*6)/7;
		}
		else
		{
			tab[2].u = (tab[0].u*4 + tab[1].u*1)/5;
			tab[3].u = (tab[0].u*3 + tab[1].u*2)/5;
			tab[4].u = (tab[0].u*2 + tab[1].u*3)/5;
			tab[5].u = (tab[0].u*1 + tab[1].u*4)/5;
			tab[6].u = 0;
			tab[7].u = 0xff;
		}
	}

	bits = in[2] | (in[3]<<8) | (in[4]<<16) | (in[5]<<24) | ((quint64_t)in[6]<<32) | ((quint64_t)in[7]<<40);

#define BC3AU_Pix(r,i)	r = tab[(bits>>((i)*3))&7].u;
#define BC3AU_Row(i)	BC3AU_Pix(out[0], i+0);BC3AU_Pix(out[4], i+1);BC3AU_Pix(out[8], i+2);BC3AU_Pix(out[12], i+3);
	BC3AU_Row(0);out += stride;BC3AU_Row(4);out += stride;BC3AU_Row(8);out += stride;BC3AU_Row(12);
#undef BC3AU_Pix
}
#ifdef DECOMPRESS_S3TC
//s3tc rgb channel, with an rgtc alpha channel that depends upon both encodings (really the origin of rgtc, but mneh).
static void Image_Decode_BC3_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	Image_Decode_S3TC_Block_Internal(in+8, out, w, 0xff);
	Image_Decode_RGTC_Block_Internal(in, out->v+3, w*4, false);
}
#endif
static void Image_Decode_BC4_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{	//BC4: BC3's alpha channel but used as red only.
	pixel32_t r;
	int i = 0;
	Vector4Set(r.v, 0, 0, 0, 0xff);
	for (i = 0; i < 4; i++)
		out[w*0+i] = out[w*1+i] = out[w*2+i] = out[w*3+i] = r;
	Image_Decode_RGTC_Block_Internal(in, out->v+0, w*4, fmt==PTI_BC4_R_SNORM);
}
static void Image_Decode_BC5_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{	//BC5: two of BC3's alpha channels but used as red+green only.
	pixel32_t r;
	int i = 0;
	Vector4Set(r.v, 0, 0, 0, 0xff);
	for (i = 0; i < 4; i++)
		out[w*0+i] = out[w*1+i] = out[w*2+i] = out[w*3+i] = r;
	Image_Decode_RGTC_Block_Internal(in+0, out->v+0, w*4, fmt==PTI_BC5_RG_SNORM);
	Image_Decode_RGTC_Block_Internal(in+8, out->v+1, w*4, fmt==PTI_BC5_RG_SNORM);
}
#endif

#ifdef DECOMPRESS_BPTC
fte_inlinestatic int ReadBits(qbyte *in, int *bit, int n)
{
	int mask = (1<<n)-1;
	int second;
	int first = *bit;
	*bit += n;
	in += first>>3;
	first &= 7;
	second = max(0,n - (8-first));
	return ((in[0]>>first)&mask) ^ (in[1]<<(n-second)&mask);
}
fte_inlinestatic int ReadBitsL(qbyte *in, int *bit, int n)
{
	int r = ReadBits(in, bit, 8);
	return (r)|(ReadBits(in, bit, n-8)<<8);
}
static void Image_Decode_BC6_Block(qbyte *fte_restrict in, pixel64_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	qboolean signextend = fmt==PTI_BC6_RGB_SFLOAT;
	static const int anchors[32] =
	{
		15,15,15,15,
		15,15,15,15,
		15,15,15,15,
		15,15,15,15,
		15, 2, 8, 2,
		2, 8, 8,15,
		2, 8, 2, 2,
		8, 8, 2, 2,
	};
	static const qbyte p2[] = {
		0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
		0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
		0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,
		0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1,
		0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,
		0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,
		0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
		0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
		0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1,
		0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,
		0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0,
		0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
		0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0,
		0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,
		0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,
		0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0,
		0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,
		0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,
		0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,
		0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0,
		0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
		0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,
		0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0,
		0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
		0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
		0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,
		0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
		0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,
		0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
		0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,
		0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0,
		0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0,
		0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,
		0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0,
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
		0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1,
		0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1,
		0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,
		0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0,
		0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,
		0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0,
		0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1,
		0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0,
		0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,
		0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1,
		0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1,
		0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1,
		0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1,
		0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1,
		0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,
		0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0,
		0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1,
    };
    static const int w3[] = {0, 9, 18, 27, 37, 46, 55, 64};
    static const int w4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};
    static const int *wsz[] = {w3,w3,w3, w3,w4};
    const int *weight;

	unsigned short rgb[4][3] = {{0}};
	unsigned short palette[16][3];
	int i, j, cb, ib;
	int bit = 0, cbits[4];
	int ss;
	int shapeindex;
	int tr;
	int mode = ReadBits(in, &bit, 2);
	if (mode >= 2)
		mode |=ReadBits(in, &bit, 3)<<2;

	//n[a:b]
	//n is the output name (number=group, r/g/b=0/1/2 channel)
	//a:b is the inclusive bit range, typically little-endian input
	//so bit counts are a+1-b, shifted up by b.
	//if b is ommitted then its equivelent to a (read as a:a, or in other words a single bit shifted up by b)
	//if its backwards then the bits are big-endian for some reason and need to be switched (rare, handled here by reading individual bits)
	//bit counts larger than 9 may need to read from more than two bytes, which requires special handling, hence the alternative function.
	//it is hoped that the compiler will be able to inline these into trivial mask+shifts for less code than it would take to call ReadBits, at least when its from a single byte.
	switch(mode)
	{
	case 0:	//1
		Vector4Set(cbits, 5,5,5, 10);
		tr = 1;
		ss = 2;
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 5)<<0;	//r1[4:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 5)<<0;	//g1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 5)<<0;	//b1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 5)<<0;	//r2[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 5)<<0;	//r3[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 1:	//2
		Vector4Set(cbits, 6,6,6, 7);
		tr = 1;
		ss = 2;
		rgb[2][1] |= ReadBits(in, &bit, 1)<<5;	//g2[5],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<5;	//g3[5],
		rgb[0][0] |= ReadBits(in, &bit, 7)<<0;	//r0[6:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 7)<<0;	//g0[6:0],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<5;	//b2[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 7)<<0;	//b0[6:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<5;	//b3[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 6)<<0;	//r1[5:0],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 6)<<0;	//g1[5:0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 6)<<0;	//b1[5:0],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 6)<<0;	//r2[5:0],
		rgb[3][0] |= ReadBits(in, &bit, 6)<<0;	//r3[5:0]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 2:	//3
		Vector4Set(cbits, 5,4,4, 11);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 5)<<0;	//r1[4:0],
		rgb[0][0] |= ReadBits(in, &bit, 1)<<10;	//r0[10],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 4)<<0;	//g1[3:0],
		rgb[0][1] |= ReadBits(in, &bit, 1)<<10;	//g0[10],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 4)<<0;	//b1[3:0],
		rgb[0][2] |= ReadBits(in, &bit, 1)<<10;	//b0[10],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 5)<<0;	//r2[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 5)<<0;	//r3[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 6:	//4
		Vector4Set(cbits, 4,5,4, 11);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 4)<<0;	//r1[3:0],
		rgb[0][0] |= ReadBits(in, &bit, 1)<<10;	//r0[10],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 5)<<0;	//g1[4:0],
		rgb[0][1] |= ReadBits(in, &bit, 1)<<10;	//g0[10],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 4)<<0;	//b1[3:0],
		rgb[0][2] |= ReadBits(in, &bit, 1)<<10;	//b0[10],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 4)<<0;	//r2[3:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 4)<<0;	//r3[3:0],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 10: //5
		Vector4Set(cbits, 4,4,5, 11);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 4)<<0;	//r1[3:0],
		rgb[0][0] |= ReadBits(in, &bit, 1)<<10;	//r0[10],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 4)<<0;	//g1[3:0],
		rgb[0][1] |= ReadBits(in, &bit, 1)<<10;	//g0[10],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 5)<<0;	//b1[4:0],
		rgb[0][2] |= ReadBits(in, &bit, 1)<<10;	//b0[10],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 4)<<0;	//r2[3:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 4)<<0;	//r3[3:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 14:	//6
		Vector4Set(cbits, 5,5,5, 9);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBits(in, &bit, 9)<<0;	//r0[8:0],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 9)<<0;	//g0[8:0],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 9)<<0;	//b0[8:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 5)<<0;	//r1[4:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 5)<<0;	//g1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 5)<<0;	//b1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 5)<<0;	//r2[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 5)<<0;	//r3[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 18:	//7
		Vector4Set(cbits, 6,5,5, 8);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBits(in, &bit, 8)<<0;	//r0[7:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 8)<<0;	//g0[7:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 8)<<0;	//b0[7:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 6)<<0;	//r1[5:0],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 5)<<0;	//g1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 5)<<0;	//b1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 6)<<0;	//r2[5:0],
		rgb[3][0] |= ReadBits(in, &bit, 6)<<0;	//r3[5:0]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 22:	//8
		Vector4Set(cbits, 5,6,5, 8);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBits(in, &bit, 8)<<0;	//r0[7:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 8)<<0;	//g0[7:0],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<5;	//g2[5],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 8)<<0;	//b0[7:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<5;	//g3[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 5)<<0;	//r1[4:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 6)<<0;	//g1[5:0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 5)<<0;	//b1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 5)<<0;	//r2[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 5)<<0;	//r3[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 26:	//9
		Vector4Set(cbits, 5,5,6, 8);
		tr = 1;
		ss = 2;
		rgb[0][0] |= ReadBits(in, &bit, 8)<<0;	//r0[7:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 8)<<0;	//g0[7:0],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<5;	//b2[5],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 8)<<0;	//b0[7:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<5;	//b3[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 5)<<0;	//r1[4:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 5)<<0;	//g1[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 6)<<0;	//b1[5:0],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 5)<<0;	//r2[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[3][0] |= ReadBits(in, &bit, 5)<<0;	//r3[4:0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 30:	//10
		Vector4Set(cbits, 6,6,6, 6);
		tr = 0;
		ss = 2;
		rgb[0][0] |= ReadBits(in, &bit, 6)<<0;	//r0[5:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<4;	//g3[4],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<0;	//b3[0],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<1;	//b3[1],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<4;	//b2[4],
		rgb[0][1] |= ReadBits(in, &bit, 6)<<0;	//g0[5:0],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<5;	//g2[5],
		rgb[2][2] |= ReadBits(in, &bit, 1)<<5;	//b2[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<2;	//b3[2],
		rgb[2][1] |= ReadBits(in, &bit, 1)<<4;	//g2[4],
		rgb[0][2] |= ReadBits(in, &bit, 6)<<0;	//b0[5:0],
		rgb[3][1] |= ReadBits(in, &bit, 1)<<5;	//g3[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<3;	//b3[3],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<5;	//b3[5],
		rgb[3][2] |= ReadBits(in, &bit, 1)<<4;	//b3[4],
		rgb[1][0] |= ReadBits(in, &bit, 6)<<0;	//r1[5:0],
		rgb[2][1] |= ReadBits(in, &bit, 4)<<0;	//g2[3:0],
		rgb[1][1] |= ReadBits(in, &bit, 6)<<0;	//g1[5:0],
		rgb[3][1] |= ReadBits(in, &bit, 4)<<0;	//g3[3:0],
		rgb[1][2] |= ReadBits(in, &bit, 6)<<0;	//b1[5:0],
		rgb[2][2] |= ReadBits(in, &bit, 4)<<0;	//b2[3:0],
		rgb[2][0] |= ReadBits(in, &bit, 6)<<0;	//r2[5:0],
		rgb[3][0] |= ReadBits(in, &bit, 6)<<0;	//r3[5:0]
		shapeindex = ReadBits(in, &bit, 5);
		break;
	case 3: //11
		Vector4Set(cbits, 10,10,10, 10);
		tr = 0;
		ss = 1;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0]
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0]
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0]
		rgb[1][0] |= ReadBitsL(in, &bit,10)<<0;	//r1[9:0]
		rgb[1][1] |= ReadBitsL(in, &bit,10)<<0;	//g1[9:0]
		rgb[1][2] |= ReadBitsL(in, &bit,10)<<0;	//b1[9:0]
		shapeindex = 0;
		break;
	case 7: //12
		Vector4Set(cbits, 9,9,9, 11);
		tr = 1;
		ss = 1;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 9)<<0;	//r1[8:0],
		rgb[0][0] |= ReadBits(in, &bit, 1)<<10;	//r0[10],
		rgb[1][1] |= ReadBits(in, &bit, 9)<<0;	//g1[8:0],
		rgb[0][1] |= ReadBits(in, &bit, 1)<<10;	//g0[10],
		rgb[1][2] |= ReadBits(in, &bit, 9)<<0;	//b1[8:0],
		rgb[0][2] |= ReadBits(in, &bit, 1)<<10;	//b0[10]
		shapeindex = 0;
		break;
	case 11: //13
		Vector4Set(cbits, 8,8,8, 12);
		tr = 1;
		ss = 1;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 8)<<0;	//r1[7:0],
		rgb[0][0] |=(ReadBits(in, &bit, 1)<<11)	//r0[10:11],
		          | (ReadBits(in, &bit, 1)<<10);
		rgb[1][1] |= ReadBits(in, &bit, 8)<<0;	//g1[7:0],
		rgb[0][1] |=(ReadBits(in, &bit, 1)<<11)	//g0[10:11],
		          | (ReadBits(in, &bit, 1)<<10);
		rgb[1][2] |= ReadBits(in, &bit, 8)<<0;	//b1[7:0],
		rgb[0][2] |=(ReadBits(in, &bit, 1)<<11)	//b0[10:11],
		          | (ReadBits(in, &bit, 1)<<10);
		shapeindex = 0;
		break;
	case 15: //14
		//UNTESTED
		Vector4Set(cbits, 4,4,4, 16);
		tr = 1;
		ss = 1;
		rgb[0][0] |= ReadBitsL(in, &bit,10)<<0;	//r0[9:0],
		rgb[0][1] |= ReadBitsL(in, &bit,10)<<0;	//g0[9:0],
		rgb[0][2] |= ReadBitsL(in, &bit,10)<<0;	//b0[9:0],
		rgb[1][0] |= ReadBits(in, &bit, 4)<<0;	//r1[3:0],
		rgb[0][0] |=(ReadBits(in, &bit, 1)<<15)	//r0[10:15],
		          | (ReadBits(in, &bit, 1)<<14)
		          | (ReadBits(in, &bit, 1)<<13)
		          | (ReadBits(in, &bit, 1)<<12)
		          | (ReadBits(in, &bit, 1)<<11)
		          | (ReadBits(in, &bit, 1)<<10);
		rgb[1][1] |= ReadBits(in, &bit, 4)<<0;	//g1[3:0],
		rgb[0][1] |=(ReadBits(in, &bit, 1)<<15)	//g0[10:15],
		          | (ReadBits(in, &bit, 1)<<14)
		          | (ReadBits(in, &bit, 1)<<13)
		          | (ReadBits(in, &bit, 1)<<12)
		          | (ReadBits(in, &bit, 1)<<11)
		          | (ReadBits(in, &bit, 1)<<10);
		rgb[1][2] |= ReadBits(in, &bit, 4)<<0;	//b1[3:0],
		rgb[0][2] |=(ReadBits(in, &bit, 1)<<15)	//b0[10:15]
		          | (ReadBits(in, &bit, 1)<<14)
		          | (ReadBits(in, &bit, 1)<<13)
		          | (ReadBits(in, &bit, 1)<<12)
		          | (ReadBits(in, &bit, 1)<<11)
		          | (ReadBits(in, &bit, 1)<<10);
		shapeindex = 0;
		break;

	default:
		//reserved modes
		for (i = 0; i < 16; )
		{
			Vector4Set(out[i].v, 0, 0, 0, 0xf<<10);
			i++;
			if (!(i & 3))
				out += w-4;
		}
		return;
	}

	ib = (ss==2)?3:4;
	cb = 1<<ib;
	weight = wsz[ib];
	if (signextend)
	{
		int v[2], q, k, g, s;
		for (g = 0; g < ss; g++)	//subsets
		{
			for (j = 0; j < 3; j++)	//channels
			{
				for (k = 0; k < 2; k++)	//start/end
				{
					v[k] = rgb[k+g*2][j];
					if ((k || g) && tr)
					{	//the specified colour values are deltas from the first colour value
						if (v[k] & (1<<(cbits[j]-1)))	//high bit set
							v[k] |= ~0<<cbits[j];		//sign extend it...
						v[k] = (int)rgb[0][j] + v[k];		//add it to the base
						v[k] &= ((1<<cbits[3])-1);		//and mask it to avoid overflows...
					}
					if (v[k] & (1<<(cbits[3]-1)))	//high bit set
						v[k] |= ~0<<cbits[3];		//sign extend it

					if (cbits[3] >= 16)
						;
					else
					{
						s = v[k] < 0;
						if (s)
							v[k]=-v[k];
						if (v[k] == 0)
							v[k] = 0;
						else if (v[k] >= (1<<(cbits[3]-1))-1)
							v[k] = 0x7fff;
						else
							v[k] = (v[k] * (0x7fff+1) + (0x7fff+1)/2) >> (cbits[3]-1);
						if (s)
							v[k] = -v[k];
					}
				}
				for (i = 0; i < cb; i++)	//indexbits
				{
					q = (v[0]*(64-weight[i]) + v[1]*weight[i])>>6;	//this ignores sign, which seems somewhat dodgy.
					q = (q < 0) ? -(((-q) * 31) >> 5) : (q * 31) >> 5;
					palette[i+g*8][j] = q;
				}
			}
		}
	}
	else
	{
		int v[2], q, k, g;
		for (g = 0; g < ss; g++)	//subsets
		{
			for (j = 0; j < 3; j++)	//channels
			{
				for (k = 0; k < 2; k++)	//start/end
				{
					v[k] = rgb[k+g*2][j];
					if ((k || g) && tr)
					{	//the specified colour values are deltas from the first colour value
						if (v[k] & (1<<(cbits[j]-1)))	//high bit set
							v[k] |= ~0<<cbits[j];		//sign extend it...
						v[k] = (int)rgb[0][j] + v[k];		//add it to the base
						v[k] &= ((1<<cbits[3])-1);		//and mask it to avoid overflows...
//						if (v[k] & (1<<(bits-1)))	//high bit set
//							v[k] |= ~0<<bits;		//sign extend it
					}

					if (cbits[3] >= 15)
						;
					else if (v[k] == 0)
						v[k] = 0;
					else if (v[k] == (1<<cbits[3])-1)
						v[k] = 0xffff;
					else
						v[k] = (v[k] * (0xffff+1) + (0xffff+1)/2) >> cbits[3];
				}
				for (i = 0; i < cb; i++)	//indexbits
				{
					q = (v[0]*(64-weight[i]) + v[1]*weight[i])>>6;
					q = (q*31)>>6;
					palette[i+g*8][j] = q;
				}
			}
		}
	}

	if (ss == 2)
	{
		const qbyte *p = p2+shapeindex*16;
		for (i = 0; i < 16; )
		{
			int pidx = p[i];
			int idx;
			if (i == 0 || i == anchors[shapeindex])
				idx = ReadBits(in, &bit, ib-1);
			else
				idx = ReadBits(in, &bit, ib);
			if (pidx)
				idx += 8;
			out[i].v[0] = palette[idx][0];
			out[i].v[1] = palette[idx][1];
			out[i].v[2] = palette[idx][2];
			out[i].v[3] = 0xf<<10;	//must be 1
			i++;
			if (!(i & 3))
				out += w-4;
		}
	}
	else
	{
		for (i = 0; i < 16; )
		{
			int idx;
			if (i == 0)
				idx = ReadBits(in, &bit, ib-1);
			else
				idx = ReadBits(in, &bit, ib);
			out[i].v[0] = palette[idx][0];
			out[i].v[1] = palette[idx][1];
			out[i].v[2] = palette[idx][2];
			out[i].v[3] = 0xf<<10;	//must be 1
			i++;
			if (!(i & 3))
				out += w-4;
		}
	}
}
static void Image_Decode_BC7_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t fmt)
{
	static const qbyte p1[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	};
	static const qbyte p2[] = {
		0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
		0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
		0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,
		0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1,
		0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,
		0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,
		0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
		0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,
		0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
		0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1,
		0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,
		0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0,
		0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0,
		0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0,
		0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,
		0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1,
		0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0,
		0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,
		0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,
		0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0,
		0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0,
		0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
		0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0,
		0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0,
		0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,
		0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
		0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,
		0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0,
		0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
		0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,
		0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,
		0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,
		0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0,
		0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0,
		0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0,
		0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0,
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
		0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1,
		0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1,
		0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,
		0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0,
		0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,
		0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,
		0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0,
		0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,
		0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1,
		0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0,
		0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0,
		0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1,
		0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1,
		0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1,
		0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1,
		0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1,
		0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,
		0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0,
		0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1,
    };
    static const qbyte p3[] = {
		0,0,1,1,0,0,1,1,0,2,2,1,2,2,2,2,
		0,0,0,1,0,0,1,1,2,2,1,1,2,2,2,1,
		0,0,0,0,2,0,0,1,2,2,1,1,2,2,1,1,
		0,2,2,2,0,0,2,2,0,0,1,1,0,1,1,1,
		0,0,0,0,0,0,0,0,1,1,2,2,1,1,2,2,
		0,0,1,1,0,0,1,1,0,0,2,2,0,0,2,2,
		0,0,2,2,0,0,2,2,1,1,1,1,1,1,1,1,
		0,0,1,1,0,0,1,1,2,2,1,1,2,2,1,1,
		0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
		0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,
		0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,
		0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,
		0,1,1,2,0,1,1,2,0,1,1,2,0,1,1,2,
		0,1,2,2,0,1,2,2,0,1,2,2,0,1,2,2,
		0,0,1,1,0,1,1,2,1,1,2,2,1,2,2,2,
		0,0,1,1,2,0,0,1,2,2,0,0,2,2,2,0,
		0,0,0,1,0,0,1,1,0,1,1,2,1,1,2,2,
		0,1,1,1,0,0,1,1,2,0,0,1,2,2,0,0,
		0,0,0,0,1,1,2,2,1,1,2,2,1,1,2,2,
		0,0,2,2,0,0,2,2,0,0,2,2,1,1,1,1,
		0,1,1,1,0,1,1,1,0,2,2,2,0,2,2,2,
		0,0,0,1,0,0,0,1,2,2,2,1,2,2,2,1,
		0,0,0,0,0,0,1,1,0,1,2,2,0,1,2,2,
		0,0,0,0,1,1,0,0,2,2,1,0,2,2,1,0,
		0,1,2,2,0,1,2,2,0,0,1,1,0,0,0,0,
		0,0,1,2,0,0,1,2,1,1,2,2,2,2,2,2,
		0,1,1,0,1,2,2,1,1,2,2,1,0,1,1,0,
		0,0,0,0,0,1,1,0,1,2,2,1,1,2,2,1,
		0,0,2,2,1,1,0,2,1,1,0,2,0,0,2,2,
		0,1,1,0,0,1,1,0,2,0,0,2,2,2,2,2,
		0,0,1,1,0,1,2,2,0,1,2,2,0,0,1,1,
		0,0,0,0,2,0,0,0,2,2,1,1,2,2,2,1,
		0,0,0,0,0,0,0,2,1,1,2,2,1,2,2,2,
		0,2,2,2,0,0,2,2,0,0,1,2,0,0,1,1,
		0,0,1,1,0,0,1,2,0,0,2,2,0,2,2,2,
		0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0,
		0,0,0,0,1,1,1,1,2,2,2,2,0,0,0,0,
		0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0,
		0,1,2,0,2,0,1,2,1,2,0,1,0,1,2,0,
		0,0,1,1,2,2,0,0,1,1,2,2,0,0,1,1,
		0,0,1,1,1,1,2,2,2,2,0,0,0,0,1,1,
		0,1,0,1,0,1,0,1,2,2,2,2,2,2,2,2,
		0,0,0,0,0,0,0,0,2,1,2,1,2,1,2,1,
		0,0,2,2,1,1,2,2,0,0,2,2,1,1,2,2,
		0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1,
		0,2,2,0,1,2,2,1,0,2,2,0,1,2,2,1,
		0,1,0,1,2,2,2,2,2,2,2,2,0,1,0,1,
		0,0,0,0,2,1,2,1,2,1,2,1,2,1,2,1,
		0,1,0,1,0,1,0,1,0,1,0,1,2,2,2,2,
		0,2,2,2,0,1,1,1,0,2,2,2,0,1,1,1,
		0,0,0,2,1,1,1,2,0,0,0,2,1,1,1,2,
		0,0,0,0,2,1,1,2,2,1,1,2,2,1,1,2,
		0,2,2,2,0,1,1,1,0,1,1,1,0,2,2,2,
		0,0,0,2,1,1,1,2,1,1,1,2,0,0,0,2,
		0,1,1,0,0,1,1,0,0,1,1,0,2,2,2,2,
		0,0,0,0,0,0,0,0,2,1,1,2,2,1,1,2,
		0,1,1,0,0,1,1,0,2,2,2,2,2,2,2,2,
		0,0,2,2,0,0,1,1,0,0,1,1,0,0,2,2,
		0,0,2,2,1,1,2,2,1,1,2,2,0,0,2,2,
		0,0,0,0,0,0,0,0,0,0,0,0,2,1,1,2,
		0,0,0,2,0,0,0,1,0,0,0,2,0,0,0,1,
		0,2,2,2,1,2,2,2,0,2,2,2,1,2,2,2,
		0,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
		0,1,1,1,2,0,1,1,2,2,0,1,2,2,2,0,
    };

    static const qbyte *psz[] = {p1,p1,p2,p3};

	static const qbyte anchortable[][64] = {{
		15,15,15,15,15,15,15,15,
		15,15,15,15,15,15,15,15,
		15, 2, 8, 2, 2, 8, 8,15,
		 2, 8, 2, 2, 8, 8, 2, 2,
		15,15, 6, 8, 2, 8,15,15,
		 2, 8, 2, 2, 2,15,15, 6,
		 6, 2, 6, 8,15,15, 2, 2,
		15,15,15,15,15, 2, 2,15,
    },{
		 3, 3,15,15, 8, 3,15,15,
		 8, 8, 6, 6, 6, 5, 3, 3,
		 3, 3, 8,15, 3, 3, 6,10,
		 5, 8, 8, 6, 8, 5,15,15,
		 8,15, 3, 5, 6,10, 8,15,
		15, 3,15, 5,15,15,15,15,
		 3,15, 5, 5, 5, 8, 5,10,
		 5,10, 8,13,15,12, 3, 3,
	},{
		15, 8, 8, 3,15,15, 3, 8,
		15,15,15,15,15,15,15, 8,
		15, 8,15, 3,15, 8,15, 8,
		 3,15, 6,10,15,15,10, 8,
		15, 3,15,10,10, 8, 9,10,
		 6,15, 8,15, 3, 6, 6, 8,
		15, 3,15,15,15,15,15,15,
		15,15,15,15, 3,15,15, 8,
    }};
	int anchor[3];

	static const int w1[] = {0, 64};
    static const int w2[] = {0, 21, 43, 64};
    static const int w3[] = {0, 9, 18, 27, 37, 46, 55, 64};
    static const int w4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};
    static const int *wsz[] = {w1,w1, w2,w3,w4};
    static const struct {
		int numsubsets;
		int partitionbits;
		int rotationbits;
		int indexselectionbits;
		int colourbits;
		int alphabits;
		int pmode;
		int indexbits[2];
    } m[9] =
    {
		{3, 4, 0, 0, 4, 0, 1, {3, 0}},	//mode 0 - 3*rgb4
		{2, 6, 0, 0, 6, 0, 2, {3, 0}},	//mode 1 - 2*rgb6
		{3, 6, 0, 0, 5, 0, 0, {2, 0}},	//mode 2 - 3*rgb5
		{2, 6, 0, 0, 7, 0, 1, {2, 0}},	//mode 3 - 2*rgb7
		{1, 0, 2, 1, 5, 6, 0, {2, 3}},	//mode 4 - 1*rgb5+a6
		{1, 0, 2, 0, 7, 8, 0, {2, 2}},	//mode 5 - 1*rgb7+a8
		{1, 0, 0, 0, 7, 7, 1, {4, 0}},	//mode 6 - 1*rgb7a7
		{2, 6, 0, 0, 5, 5, 1, {2, 0}},	//mode 7 - 2*rgb5a5
		{1, 0, 0, 0, 0, 0, 0, {0, 0}},	//mode 8 - reserved
    };

	pixel32_t palette[3][2];
	pixel32_t tab[3][16];
	int mode, i, j, bit, partition, ss, cb;
	const int *weight;
	const qbyte *p;
	int rot;
	int idxsel;
	for (mode = 0; mode < 8; mode++)
		if (*in & (1u<<mode))
			break;
	ss = m[mode].numsubsets;
	bit = mode+1;
	partition = ReadBits(in, &bit, m[mode].partitionbits);
	rot = ReadBits(in, &bit, m[mode].rotationbits);
	idxsel = ReadBits(in, &bit, m[mode].indexselectionbits);
	p = psz[ss] + partition*16;
	for (j = 0; j < 3; j++)
		for (i = 0; i < ss; i++)
		{
			palette[i][0].v[j] = ReadBits(in, &bit, m[mode].colourbits) << (8-m[mode].colourbits);
			palette[i][1].v[j] = ReadBits(in, &bit, m[mode].colourbits) << (8-m[mode].colourbits);
		}
	for (i = 0; i < ss; i++)
	{
		if (m[mode].alphabits)
		{
			palette[i][0].v[3] = ReadBits(in, &bit, m[mode].alphabits) << (8-m[mode].alphabits);
			palette[i][1].v[3] = ReadBits(in, &bit, m[mode].alphabits) << (8-m[mode].alphabits);
		}
		else palette[i][0].v[3] = palette[i][1].v[3] = 255;
	}

	if(m[mode].pmode)
	{
		for (i = 0; i < m[mode].numsubsets; i++)
		{
			qbyte p = ReadBits(in, &bit, 1);
			for (j = 0; j < 3; j++)
				palette[i][0].v[j] |= p<<(7-m[mode].colourbits);
			palette[i][0].v[3] |= p<<(7-m[mode].alphabits);

			if (m[mode].pmode!=2)
				p = ReadBits(in, &bit, 1);
			for (j = 0; j < 3; j++)
				palette[i][1].v[j] |= p<<(7-m[mode].colourbits);
			palette[i][1].v[3] |= p<<(7-m[mode].alphabits);
		}
		etc_expandv(palette[i][0], m[mode].colourbits+1, m[mode].colourbits+1, m[mode].colourbits+1); palette[i][0].v[3]|=palette[i][0].v[3]>>(m[mode].alphabits+1);
		etc_expandv(palette[i][1], m[mode].colourbits+1, m[mode].colourbits+1, m[mode].colourbits+1); palette[i][0].v[3]|=palette[i][0].v[3]>>(m[mode].alphabits+1);
	}
	else
	{
		etc_expandv(palette[i][0], m[mode].colourbits, m[mode].colourbits, m[mode].colourbits); palette[i][0].v[3]|=palette[i][0].v[3]>>m[mode].alphabits;
		etc_expandv(palette[i][1], m[mode].colourbits, m[mode].colourbits, m[mode].colourbits);	palette[i][1].v[3]|=palette[i][0].v[3]>>m[mode].alphabits;
	}

	cb = m[mode].indexbits[idxsel];
	weight = wsz[cb];

	//build lerps table (could do it per pixel?)
	for (i = 0; i < m[mode].numsubsets; i++)
	{
		for (j = 0; j < (1<<cb); j++)
		{
			tab[i][j].v[0] = (palette[i][0].v[0]*(64-weight[j]) + palette[i][1].v[0]*weight[j] + 32)>>6;
			tab[i][j].v[1] = (palette[i][0].v[1]*(64-weight[j]) + palette[i][1].v[1]*weight[j] + 32)>>6;
			tab[i][j].v[2] = (palette[i][0].v[2]*(64-weight[j]) + palette[i][1].v[2]*weight[j] + 32)>>6;
			tab[i][j].v[3] = (palette[i][0].v[3]*(64-weight[j]) + palette[i][1].v[3]*weight[j] + 32)>>6;
		}

		//this stuff is annoying, but saves a bit or two
		if (!cb)
			anchor[i] = 16;	//don't allow the cb-1 to read negative bits!
		else if (i)
			anchor[i] = anchortable[(ss>2)?i:0][partition];
		else
			anchor[i] = 0;
	}

	//okay, tables are all set up, spew out the pixels
	for (i = 0; i < 16; )
	{
		int pidx = p[i];
		int idx;
		if (i == anchor[pidx])
			idx = ReadBits(in, &bit, cb-1);
		else
			idx = ReadBits(in, &bit, cb);
		out[i].u = tab[pidx][idx].u;
		i++;
		if (!(i & 3))
			out += w-4;
	}

	//mode has separate alpha indexes, spew those out too, clobbering any alpha from dodgy rgb blends
	if (m[mode].indexbits[idxsel^1])
	{	//FIXME: untested
		out -= w*4;
		cb = m[mode].indexbits[idxsel^1];
		weight = wsz[cb];
		for (i = 0; i < m[mode].numsubsets; i++)
		{
			for (j = 0; j < (1<<cb); j++)
				tab[i][j].v[3] = (palette[i][0].v[3]*(64-weight[j]) + palette[i][1].v[3]*weight[j] + 32)>>6;
		}

		for (i = 0; i < 16; )
		{
			int idx;
			if (i == 0)
				idx = ReadBits(in, &bit, cb-1);
			else
				idx = ReadBits(in, &bit, cb);
			out[i].v[3] = tab[p[i]][idx].v[3];
			i++;
			if (!(i & 3))
				out += w-4;
		}
	}

	//some modes allow swapping the alpha with an rgb channel (per block)
	if (rot)
	{	//FIXME: untested
		qbyte t;
		rot--; //0=disable, 1=red, 2=green, 3=blue
		out -= w*4;
		for (i = 0; i < 16; )
		{
			t = out[i].v[3];
			out[i].v[3] = out[i].v[rot];
			out[i].v[rot] = t;

			i++;
			if (!(i & 3))
				out += w-4;
		}
	}
}
#endif

#ifdef DECOMPRESS_ASTC
#ifdef ASTC_WITH_LDR
static void Image_Decode_ASTC_LDR_U8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int stride, uploadfmt_t fmt)
{
	int bw, bh, bd, blockbytes;
	Image_BlockSizeForEncoding(fmt, &blockbytes, &bw, &bh, &bd);
	ASTC_Decode_LDR8(in, out->v, stride, 0/*w*h*/, bw, bh, bd);
}
#endif
#ifdef ASTC_WITH_HDR
static void Image_Decode_ASTC_HDR_HF_Block(qbyte *fte_restrict in, pixel64_t *fte_restrict out, int stride, uploadfmt_t fmt)
{
	int bw, bh, bd, blockbytes;
	Image_BlockSizeForEncoding(fmt, &blockbytes, &bw, &bh, &bd);
	ASTC_Decode_HDR(in, out->v, stride, 0/*w*h*/, bw, bh, bd);
}

/*static unsigned int RGB16F_to_E5BGR9(unsigned short Cr, unsigned short Cg, unsigned short Cb)
{
	int Re,Ge,Be, Rex,Gex,Bex, Xm, Xe;
	quint32_t rshift, gshift, bshift, expo;
	int Rm, Gm, Bm;

	if( Cr > 0x7c00 ) Cr = 0; else if( Cr == 0x7c00 ) Cr = 0x7bff;
	if( Cg > 0x7c00 ) Cg = 0; else if( Cg == 0x7c00 ) Cg = 0x7bff;
	if( Cb > 0x7c00 ) Cb = 0; else if( Cb == 0x7c00 ) Cb = 0x7bff;
	Re = (Cr >> 10) & 0x1F;
	Ge = (Cg >> 10) & 0x1F;
	Be = (Cb >> 10) & 0x1F;
	Rex = Re == 0 ? 1 : Re;
	Gex = Ge == 0 ? 1 : Ge;
	Bex = Be == 0 ? 1 : Be;
	Xm = ((Cr | Cg | Cb) & 0x200) >> 9;
	Xe = Re | Ge | Be;

	if (Xe == 0)
	{
		expo = rshift = gshift = bshift = Xm;
	}
	else if (Re >= Ge && Re >= Be)
	{
		expo = Rex + 1;
		rshift = 2;
		gshift = Rex - Gex + 2;
		bshift = Rex - Bex + 2;
	}
	else if (Ge >= Be)
	{
		expo = Gex + 1;
		rshift = Gex - Rex + 2;
		gshift = 2;
		bshift = Gex - Bex + 2;
	}
	else
	{
		expo = Bex + 1;
		rshift = Bex - Rex + 2;
		gshift = Bex - Gex + 2;
		bshift = 2;
	}

	Rm = (Cr & 0x3FF) | (Re == 0 ? 0 : 0x400);
	Gm = (Cg & 0x3FF) | (Ge == 0 ? 0 : 0x400);
	Bm = (Cb & 0x3FF) | (Be == 0 ? 0 : 0x400);
	Rm = (Rm >> rshift) & 0x1FF;
	Gm = (Gm >> gshift) & 0x1FF;
	Bm = (Bm >> bshift) & 0x1FF;

	return (expo << 27) | (Bm << 18) | (Gm << 9) | (Rm << 0);
}
static void Image_Decode_ASTC_HDR_E5_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int stride, uploadfmt_t fmt)
{
	unsigned short hdr[12][12][4];
	int bw, bh, blockbytes;
	int x, y;
	Image_BlockSizeForEncoding(fmt, &blockbytes, &bw, &bh);
	ASTC_Decode_HDR(in, hdr[0][0], 12, bw, bh);

	for (y = 0; y < bh; y++, out += stride)
		for (x = 0; x < bw; x++)
			out[x].u = RGB16F_to_E5BGR9(hdr[y][x][0], hdr[y][x][1], hdr[y][x][2]);
}*/
#endif
#endif

static void Image_Decode_RGB8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t srcfmt)
{
	Vector4Set(out->v, in[0], in[1], in[2], 0xff);
}
static void Image_Decode_L8A8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t srcfmt)
{
	Vector4Set(out->v, in[0], in[0], in[0], in[1]);
}
static void Image_Decode_L8_Block(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t srcfmt)
{
	Vector4Set(out->v, in[0], in[0], in[0], 0xff);
}

void Image_BlockSizeForEncoding(uploadfmt_t encoding, unsigned int *blockbytes, unsigned int *blockwidth, unsigned int *blockheight, unsigned int *blockdepth)
{
	unsigned int b = 0, w = 1, h = 1, d = 1;
	switch(encoding)
	{
	case PTI_RGB565:
	case PTI_RGBA4444:
	case PTI_ARGB4444:
	case PTI_RGBA5551:
	case PTI_ARGB1555:
		b = 2;	//16bit formats
		break;
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_BGRA8:
	case PTI_BGRX8:
	case PTI_RGBA8_SRGB:
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
	case PTI_A2BGR10:
	case PTI_E5BGR9:
	case PTI_B10G11R11F:
		b = 4;
		break;

	case PTI_R16:
	case PTI_R16F:
		b = 1*2;
		break;
	case PTI_R32F:
		b = 1*4;
		break;
	case PTI_RGBA16:
	case PTI_RGBA16F:
		b = 4*2;
		break;
	case PTI_RGBA32F:
		b = 4*4;
		break;
	case PTI_RGB32F:
		b = 3*4;
		break;
	case PTI_P8:
	case PTI_R8:
	case PTI_R8_SNORM:
		b = 1;
		break;
	case PTI_RG8:
	case PTI_RG8_SNORM:
		b = 2;
		break;

	case PTI_DEPTH16:
		b = 2;
		break;
	case PTI_DEPTH24:
		b = 3;
		break;
	case PTI_DEPTH32:
		b = 4;
		break;
	case PTI_DEPTH24_8:
		b = 4;
		break;

	case PTI_RGB8:
	case PTI_BGR8:
	case PTI_RGB8_SRGB:
	case PTI_BGR8_SRGB:
		b = 3;
		break;
	case PTI_L8:
	case PTI_L8_SRGB:
		b = 1;
		break;
	case PTI_L8A8:
	case PTI_L8A8_SRGB:
		b = 2;
		break;

	case PTI_BC1_RGB:
	case PTI_BC1_RGB_SRGB:
	case PTI_BC1_RGBA:
	case PTI_BC1_RGBA_SRGB:
	case PTI_BC4_R:
	case PTI_BC4_R_SNORM:
	case PTI_ETC1_RGB8:
	case PTI_ETC2_RGB8:
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
		w = h = 4;
		b = 8;
		break;
	case PTI_BC2_RGBA:
	case PTI_BC2_RGBA_SRGB:
	case PTI_BC3_RGBA:
	case PTI_BC3_RGBA_SRGB:
	case PTI_BC5_RG:
	case PTI_BC5_RG_SNORM:
	case PTI_BC6_RGB_UFLOAT:
	case PTI_BC6_RGB_SFLOAT:
	case PTI_BC7_RGBA:
	case PTI_BC7_RGBA_SRGB:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:
		w = h = 4;
		b = 16;
		break;

// ASTC is crazy with its format subtypes... note that all are potentially rgba, selected on a per-block basis
	case PTI_ASTC_4X4_HDR:
	case PTI_ASTC_4X4_SRGB:
	case PTI_ASTC_4X4_LDR:		w = 4; h = 4; b = 16; break;
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X4_SRGB:
	case PTI_ASTC_5X4_LDR:		w = 5; h = 4; b = 16; break;
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_5X5_SRGB:
	case PTI_ASTC_5X5_LDR:		w = 5; h = 5; b = 16; break;
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X5_SRGB:
	case PTI_ASTC_6X5_LDR:		w = 6; h = 5; b = 16; break;
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_6X6_SRGB:
	case PTI_ASTC_6X6_LDR:		w = 6; h = 6; b = 16; break;
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X5_SRGB:
	case PTI_ASTC_8X5_LDR:		w = 8; h = 5; b = 16; break;
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_8X6_SRGB:
	case PTI_ASTC_8X6_LDR:		w = 8; h = 6; b = 16; break;
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X5_SRGB:
	case PTI_ASTC_10X5_LDR:		w = 10; h = 5; b = 16; break;
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_10X6_SRGB:
	case PTI_ASTC_10X6_LDR:		w = 10; h = 6; b = 16; break;
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_8X8_SRGB:
	case PTI_ASTC_8X8_LDR:		w = 8; h = 8; b = 16; break;
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X8_SRGB:
	case PTI_ASTC_10X8_LDR:		w = 10; h = 8; b = 16; break;
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_10X10_SRGB:
	case PTI_ASTC_10X10_LDR:	w = 10; h = 10; b = 16; break;
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X10_SRGB:
	case PTI_ASTC_12X10_LDR:	w = 12; h = 10; b = 16; break;
	case PTI_ASTC_12X12_HDR:
	case PTI_ASTC_12X12_SRGB:
	case PTI_ASTC_12X12_LDR:	w = 12; h = 12; b = 16; break;

#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_3X3X3_SRGB:
	case PTI_ASTC_3X3X3_LDR:	w = 3; h = 3; d = 3; b = 16; break;
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X3X3_SRGB:
	case PTI_ASTC_4X3X3_LDR:	w = 4; h = 3; d = 3; b = 16; break;
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X3_SRGB:
	case PTI_ASTC_4X4X3_LDR:	w = 4; h = 4; d = 3; b = 16; break;
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_4X4X4_SRGB:
	case PTI_ASTC_4X4X4_LDR:	w = 4; h = 4; d = 4; b = 16; break;
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X4X4_SRGB:
	case PTI_ASTC_5X4X4_LDR:	w = 5; h = 4; d = 4; b = 16; break;
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X4_SRGB:
	case PTI_ASTC_5X5X4_LDR:	w = 5; h = 5; d = 4; b = 16; break;
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_5X5X5_SRGB:
	case PTI_ASTC_5X5X5_LDR:	w = 5; h = 5; d = 5; b = 16; break;
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X5X5_SRGB:
	case PTI_ASTC_6X5X5_LDR:	w = 6; h = 5; d = 5; b = 16; break;
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X5_SRGB:
	case PTI_ASTC_6X6X5_LDR:	w = 6; h = 6; d = 5; b = 16; break;
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_6X6X6_SRGB:
	case PTI_ASTC_6X6X6_LDR:	w = 6; h = 6; d = 6; b = 16; break;
#endif

	case TF_BGR24_FLIP:
	case TF_SOLID8:
	case TF_TRANS8:
	case TF_TRANS8_FULLBRIGHT:
	case TF_HEIGHT8:
	case TF_HEIGHT8PAL:
	case TF_H2_T7G1:
	case TF_H2_TRANS8_0:
	case TF_H2_T4A4:
		b=1;
		break;
	case PTI_LLLX8:
	case PTI_LLLA8:
		b=4;
		break;
	case TF_8PAL24:
	case TF_8PAL32:
	case TF_INVALID:
	case TF_MIP4_P8:
	case TF_MIP4_SOLID8:
	case TF_MIP4_8PAL24:
	case TF_MIP4_8PAL24_T255:
#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE: //UNKNOWN!
#endif
	case PTI_MAX:
		break;
	}

	*blockbytes = b;
	*blockwidth = w;
	*blockheight = h;
	*blockdepth = d;
}

qboolean Image_FormatHasAlpha(uploadfmt_t encoding)
{
	switch(encoding)
	{
	case PTI_RGB565:
	case PTI_RGBX8:
	case PTI_BGRX8:
	case PTI_RGBX8_SRGB:
	case PTI_BGRX8_SRGB:
	case PTI_E5BGR9:
	case PTI_B10G11R11F:
	case PTI_R16:
	case PTI_R16F:
	case PTI_R32F:
	case PTI_RGB32F:
	case PTI_P8:
	case PTI_R8:
	case PTI_R8_SNORM:
	case PTI_RG8:
	case PTI_RG8_SNORM:
	case PTI_DEPTH16:
	case PTI_DEPTH24:
	case PTI_DEPTH32:
	case PTI_DEPTH24_8:
	case PTI_RGB8:
	case PTI_BGR8:
	case PTI_RGB8_SRGB:
	case PTI_BGR8_SRGB:
	case PTI_L8:
	case PTI_L8_SRGB:
	case PTI_BC1_RGB:
	case PTI_BC1_RGB_SRGB:
	case PTI_BC4_R:
	case PTI_BC4_R_SNORM:
	case PTI_BC5_RG:
	case PTI_BC5_RG_SNORM:
	case PTI_BC6_RGB_UFLOAT:
	case PTI_BC6_RGB_SFLOAT:
	case PTI_ETC1_RGB8:
	case PTI_ETC2_RGB8:
	case PTI_ETC2_RGB8_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:
		return false;

	case PTI_RGBA4444:
	case PTI_ARGB4444:
	case PTI_RGBA5551:
	case PTI_ARGB1555:
	case PTI_RGBA8:
	case PTI_BGRA8:
	case PTI_RGBA8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_A2BGR10:
	case PTI_RGBA16:
	case PTI_RGBA16F:
	case PTI_RGBA32F:
	case PTI_L8A8:
	case PTI_L8A8_SRGB:
	case PTI_BC1_RGBA:
	case PTI_BC1_RGBA_SRGB:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_BC2_RGBA:
	case PTI_BC2_RGBA_SRGB:
	case PTI_BC3_RGBA:
	case PTI_BC3_RGBA_SRGB:
	case PTI_BC7_RGBA:
	case PTI_BC7_RGBA_SRGB:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_ASTC_4X4_HDR:
	case PTI_ASTC_4X4_SRGB:
	case PTI_ASTC_4X4_LDR:
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X4_SRGB:
	case PTI_ASTC_5X4_LDR:
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_5X5_SRGB:
	case PTI_ASTC_5X5_LDR:
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X5_SRGB:
	case PTI_ASTC_6X5_LDR:
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_6X6_SRGB:
	case PTI_ASTC_6X6_LDR:
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X5_SRGB:
	case PTI_ASTC_8X5_LDR:
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_8X6_SRGB:
	case PTI_ASTC_8X6_LDR:
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X5_SRGB:
	case PTI_ASTC_10X5_LDR:
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_10X6_SRGB:
	case PTI_ASTC_10X6_LDR:
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_8X8_SRGB:
	case PTI_ASTC_8X8_LDR:
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X8_SRGB:
	case PTI_ASTC_10X8_LDR:
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_10X10_SRGB:
	case PTI_ASTC_10X10_LDR:
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X10_SRGB:
	case PTI_ASTC_12X10_LDR:
	case PTI_ASTC_12X12_HDR:
	case PTI_ASTC_12X12_SRGB:
	case PTI_ASTC_12X12_LDR:
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_3X3X3_SRGB:
	case PTI_ASTC_3X3X3_LDR:
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X3X3_SRGB:
	case PTI_ASTC_4X3X3_LDR:
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X3_SRGB:
	case PTI_ASTC_4X4X3_LDR:
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_4X4X4_SRGB:
	case PTI_ASTC_4X4X4_LDR:
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X4X4_SRGB:
	case PTI_ASTC_5X4X4_LDR:
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X4_SRGB:
	case PTI_ASTC_5X5X4_LDR:
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_5X5X5_SRGB:
	case PTI_ASTC_5X5X5_LDR:
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X5X5_SRGB:
	case PTI_ASTC_6X5X5_LDR:
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X5_SRGB:
	case PTI_ASTC_6X6X5_LDR:
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_6X6X6_SRGB:
	case PTI_ASTC_6X6X6_LDR:
#endif
		return true;

	case PTI_EMULATED:
#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE: //UNKNOWN!
#endif
	case PTI_MAX:
		return false;
	}
	return false;
}

const char *Image_FormatName(uploadfmt_t fmt)
{
	switch(fmt)
	{
	case PTI_RGB565:			return "RGB565";
	case PTI_RGBA4444:			return "RGBA4444";
	case PTI_ARGB4444:			return "ARGB4444";
	case PTI_RGBA5551:			return "RGBA5551";
	case PTI_ARGB1555:			return "ARGB1555";
	case PTI_RGBA8:				return "RGBA8";
	case PTI_RGBX8:				return "RGBX8";
	case PTI_BGRA8:				return "BGRA8";
	case PTI_BGRX8:				return "BGRX8";
	case PTI_RGBA8_SRGB:		return "RGBA8_SRGB";
	case PTI_RGBX8_SRGB:		return "RGBX8_SRGB";
	case PTI_BGRA8_SRGB:		return "BGRA8_SRGB";
	case PTI_BGRX8_SRGB:		return "BGRX8_SRGB";
	case PTI_A2BGR10:			return "A2BGR10";
	case PTI_E5BGR9:			return "E5BGR9_UF";
	case PTI_B10G11R11F:		return "B10G11R11_UF";
	case PTI_R16F:				return "R16_SF";
	case PTI_R32F:				return "R32_SF";
	case PTI_RGBA16F:			return "RGBA16_SF";
	case PTI_RGBA32F:			return "RGBA32_SF";
	case PTI_RGB32F:			return "RGB32_SF";
	case PTI_R16:				return "R16";
	case PTI_RGBA16:			return "RGBA16";
	case PTI_P8:				return "P8";
	case PTI_R8:				return "R8";
	case PTI_R8_SNORM:			return "R8_SNORM";
	case PTI_RG8:				return "RG8";
	case PTI_RG8_SNORM:			return "RG8_SNORM";
	case PTI_DEPTH16:			return "DEPTH16";
	case PTI_DEPTH24:			return "DEPTH24";
	case PTI_DEPTH32:			return "DEPTH32";
	case PTI_DEPTH24_8:			return "DEPTH24_8";
	case PTI_RGB8:				return "RGB8";
	case PTI_BGR8:				return "BGR8";
	case PTI_RGB8_SRGB:			return "RGB8_SRGB";
	case PTI_BGR8_SRGB:			return "BGR8_SRGB";
	case PTI_L8:				return "L8";
	case PTI_L8_SRGB:			return "L8_SRGB";
	case PTI_L8A8:				return "L8A8";
	case PTI_L8A8_SRGB:			return "L8A8_SRGB";
	case PTI_BC1_RGB:			return "BC1_RGB";
	case PTI_BC1_RGB_SRGB:		return "BC1_RGB_SRGB";
	case PTI_BC1_RGBA:			return "BC1_RGBA";
	case PTI_BC1_RGBA_SRGB:		return "BC1_RGBA_SRGB";
	case PTI_BC2_RGBA:			return "BC2_RGBA";
	case PTI_BC2_RGBA_SRGB:		return "BC2_RGBA_SRGB";
	case PTI_BC3_RGBA:			return "BC3_RGBA";
	case PTI_BC3_RGBA_SRGB:		return "BC3_RGBA_SRGB";
	case PTI_BC4_R:				return "BC4_R";
	case PTI_BC4_R_SNORM:		return "BC4_R_SNORM";
	case PTI_BC5_RG:			return "BC5_RG";
	case PTI_BC5_RG_SNORM:		return "BC5_RG_SNORM";
	case PTI_BC6_RGB_UFLOAT:	return "BC6_RGB_UF";
	case PTI_BC6_RGB_SFLOAT:	return "BC6_RGB_SF";
	case PTI_BC7_RGBA:			return "BC7_RGBA";
	case PTI_BC7_RGBA_SRGB:		return "BC7_RGBA_SRGB";
	case PTI_ETC1_RGB8:			return "ETC1_RGB8";
	case PTI_ETC2_RGB8:			return "ETC2_RGB8";
	case PTI_ETC2_RGB8_SRGB:	return "ETC2_RGB8_SRGB";
	case PTI_ETC2_RGB8A1:		return "ETC2_RGB8A1";
	case PTI_ETC2_RGB8A1_SRGB:	return "ETC2_RGB8A1_SRGB";
	case PTI_EAC_R11:			return "EAC_R11";
	case PTI_EAC_R11_SNORM:		return "EAC_R11_SNORM";
	case PTI_ETC2_RGB8A8:		return "ETC2_RGB8A8";
	case PTI_ETC2_RGB8A8_SRGB:	return "ETC2_RGB8A8_SRGB";
	case PTI_EAC_RG11:			return "EAC_RG11";
	case PTI_EAC_RG11_SNORM:	return "EAC_RG11_SNORM";
	case PTI_ASTC_4X4_HDR:		return "ASTC_4X4_HDR";
	case PTI_ASTC_4X4_SRGB:		return "ASTC_4X4_SRGB";
	case PTI_ASTC_4X4_LDR:		return "ASTC_4X4_LDR";
	case PTI_ASTC_5X4_HDR:		return "ASTC_5X4_HDR";
	case PTI_ASTC_5X4_SRGB:		return "ASTC_5X4_SRGB";
	case PTI_ASTC_5X4_LDR:		return "ASTC_5X4_LDR";
	case PTI_ASTC_5X5_HDR:		return "ASTC_5X5_HDR";
	case PTI_ASTC_5X5_SRGB:		return "ASTC_5X5_SRGB";
	case PTI_ASTC_5X5_LDR:		return "ASTC_5X5_LDR";
	case PTI_ASTC_6X5_HDR:		return "ASTC_6X5_HDR";
	case PTI_ASTC_6X5_SRGB:		return "ASTC_6X5_SRGB";
	case PTI_ASTC_6X5_LDR:		return "ASTC_6X5_LDR";
	case PTI_ASTC_6X6_HDR:		return "ASTC_6X6_HDR";
	case PTI_ASTC_6X6_SRGB:		return "ASTC_6X6_SRGB";
	case PTI_ASTC_6X6_LDR:		return "ASTC_6X6_LDR";
	case PTI_ASTC_8X5_HDR:		return "ASTC_8X5_HDR";
	case PTI_ASTC_8X5_SRGB:		return "ASTC_8X5_SRGB";
	case PTI_ASTC_8X5_LDR:		return "ASTC_8X5_LDR";
	case PTI_ASTC_8X6_HDR:		return "ASTC_8X6_HDR";
	case PTI_ASTC_8X6_SRGB:		return "ASTC_8X6_SRGB";
	case PTI_ASTC_8X6_LDR:		return "ASTC_8X6_LDR";
	case PTI_ASTC_10X5_HDR:		return "ASTC_10X5_HDR";
	case PTI_ASTC_10X5_SRGB:	return "ASTC_10X5_SRGB";
	case PTI_ASTC_10X5_LDR:		return "ASTC_10X5_LDR";
	case PTI_ASTC_10X6_HDR:		return "ASTC_10X6_HDR";
	case PTI_ASTC_10X6_SRGB:	return "ASTC_10X6_SRGB";
	case PTI_ASTC_10X6_LDR:		return "ASTC_10X6_LDR";
	case PTI_ASTC_8X8_HDR:		return "ASTC_8X8_HDR";
	case PTI_ASTC_8X8_SRGB:		return "ASTC_8X8_SRGB";
	case PTI_ASTC_8X8_LDR:		return "ASTC_8X8_LDR";
	case PTI_ASTC_10X8_HDR:		return "ASTC_10X8_HDR";
	case PTI_ASTC_10X8_SRGB:	return "ASTC_10X8_SRGB";
	case PTI_ASTC_10X8_LDR:		return "ASTC_10X8_LDR";
	case PTI_ASTC_10X10_HDR:	return "ASTC_10X10_HDR";
	case PTI_ASTC_10X10_SRGB:	return "ASTC_10X10_SRGB";
	case PTI_ASTC_10X10_LDR:	return "ASTC_10X10_LDR";
	case PTI_ASTC_12X10_HDR:	return "ASTC_12X10_HDR";
	case PTI_ASTC_12X10_SRGB:	return "ASTC_12X10_SRGB";
	case PTI_ASTC_12X10_LDR:	return "ASTC_12X10_LDR";
	case PTI_ASTC_12X12_HDR:	return "ASTC_12X12_HDR";
	case PTI_ASTC_12X12_SRGB:	return "ASTC_12X12_SRGB";
	case PTI_ASTC_12X12_LDR:	return "ASTC_12X12_LDR";
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:	return "ASTC_3X3X3_HDR";
	case PTI_ASTC_3X3X3_SRGB:	return "ASTC_3X3X3_SRGB";
	case PTI_ASTC_3X3X3_LDR:	return "ASTC_3X3X3_LDR";
	case PTI_ASTC_4X3X3_HDR:	return "ASTC_4X3X3_HDR";
	case PTI_ASTC_4X3X3_SRGB:	return "ASTC_4X3X3_SRGB";
	case PTI_ASTC_4X3X3_LDR:	return "ASTC_4X3X3_LDR";
	case PTI_ASTC_4X4X3_HDR:	return "ASTC_4X4X3_HDR";
	case PTI_ASTC_4X4X3_SRGB:	return "ASTC_4X4X3_SRGB";
	case PTI_ASTC_4X4X3_LDR:	return "ASTC_4X4X3_LDR";
	case PTI_ASTC_4X4X4_HDR:	return "ASTC_4X4X4_HDR";
	case PTI_ASTC_4X4X4_SRGB:	return "ASTC_4X4X4_SRGB";
	case PTI_ASTC_4X4X4_LDR:	return "ASTC_4X4X4_LDR";
	case PTI_ASTC_5X4X4_HDR:	return "ASTC_5X4X4_HDR";
	case PTI_ASTC_5X4X4_SRGB:	return "ASTC_5X4X4_SRGB";
	case PTI_ASTC_5X4X4_LDR:	return "ASTC_5X4X4_LDR";
	case PTI_ASTC_5X5X4_HDR:	return "ASTC_5X5X4_HDR";
	case PTI_ASTC_5X5X4_SRGB:	return "ASTC_5X5X4_SRGB";
	case PTI_ASTC_5X5X4_LDR:	return "ASTC_5X5X4_LDR";
	case PTI_ASTC_5X5X5_HDR:	return "ASTC_5X5X5_HDR";
	case PTI_ASTC_5X5X5_SRGB:	return "ASTC_5X5X5_SRGB";
	case PTI_ASTC_5X5X5_LDR:	return "ASTC_5X5X5_LDR";
	case PTI_ASTC_6X5X5_HDR:	return "ASTC_6X5X5_HDR";
	case PTI_ASTC_6X5X5_SRGB:	return "ASTC_6X5X5_SRGB";
	case PTI_ASTC_6X5X5_LDR:	return "ASTC_6X5X5_LDR";
	case PTI_ASTC_6X6X5_HDR:	return "ASTC_6X6X5_HDR";
	case PTI_ASTC_6X6X5_SRGB:	return "ASTC_6X6X5_SRGB";
	case PTI_ASTC_6X6X5_LDR:	return "ASTC_6X6X5_LDR";
	case PTI_ASTC_6X6X6_HDR:	return "ASTC_6X6X6_HDR";
	case PTI_ASTC_6X6X6_SRGB:	return "ASTC_6X6X6_SRGB";
	case PTI_ASTC_6X6X6_LDR:	return "ASTC_6X6X6_LDR";
#endif

#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:			return "Whole File";
#endif
	case TF_INVALID:			return "INVALID";
	case TF_BGR24_FLIP:			return "BGR24_FLIP";
	case TF_MIP4_P8:			return "MIP4_P8";
	case TF_MIP4_SOLID8:		return "MIP4_SOLID8";
	case TF_MIP4_8PAL24:		return "MIP4_8PAL24";
	case TF_MIP4_8PAL24_T255:	return "MIP4_8PAL24_T255";
	case TF_SOLID8:				return "SOLID8";
	case TF_TRANS8:				return "TRANS8_255";
	case TF_TRANS8_FULLBRIGHT:	return "TRANS8_FULLBRIGHT";
	case TF_HEIGHT8:			return "HEIGHT8";
	case TF_HEIGHT8PAL:			return "HEIGHT8PAL";
	case TF_H2_T7G1:			return "H2_T7G1";
	case TF_H2_TRANS8_0:		return "TRANS8_0";
	case TF_H2_T4A4:			return "H2_T4A4";
	case TF_8PAL24:				return "8PAL24";
	case TF_8PAL32:				return "8PAL32";
	case PTI_LLLX8:				return "LLLX8";
	case PTI_LLLA8:				return "LLLA8";
	case PTI_MAX:
		break;
	}
	return "Unknown";
}

static pixel32_t *Image_Block_Decode(qbyte *fte_restrict in, size_t insize, int w, int h, int d, void(*decodeblock)(qbyte *fte_restrict in, pixel32_t *fte_restrict out, int w, uploadfmt_t srcfmt), uploadfmt_t encoding)
{
#define TMPBLOCKSIZE 16u
	pixel32_t *ret, *out;
	pixel32_t tmp[TMPBLOCKSIZE*TMPBLOCKSIZE];
	int x, y, z, i, j;
	int sizediff;
	int rows, columns, layers;

	unsigned int blockbytes, blockwidth, blockheight, blockdepth;
	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	if (blockwidth > TMPBLOCKSIZE || blockheight > TMPBLOCKSIZE || blockdepth != 1)
		Sys_Error("Image_Block_Decode only supports up to %u*%u blocks.\n", TMPBLOCKSIZE,TMPBLOCKSIZE);

	sizediff = insize - blockbytes*((w+blockwidth-1)/blockwidth)*((h+blockheight-1)/blockheight)*d;
	if (sizediff)
	{
		Con_Printf("Image_Block_Decode: %s data size is %u, expected %u\n\n", Image_FormatName(encoding), (unsigned int)insize, (unsigned int)(insize-sizediff));
		if (sizediff < 0)
			return NULL;
	}

	ret = out = BZ_Malloc(w*h*d*sizeof(*out));

	rows = h/blockheight;
	rows *= blockheight;
	columns = w/blockwidth;
	columns *= blockwidth;
	layers = d;
	for (z = 0; z < layers; z++)
	{
		for (y = 0; y < rows; y+=blockheight, out += w*(blockheight-1))
		{
			for (x = 0; x < columns; x+=blockwidth, in+=blockbytes, out+=blockwidth)
				decodeblock(in, out, w, encoding);
			if (w%blockwidth)
			{
				decodeblock(in, tmp, TMPBLOCKSIZE, encoding);
				for (i = 0; x < w; x++, out++, i++)
				{
					for (j = 0; j < blockheight; j++)
						out[w*j] = tmp[i+TMPBLOCKSIZE*j];
				}
				in+=blockbytes;
			}
		}
		if (h%blockheight)
		{	//now walk along the bottom of the image
			h %= blockheight;
			for (x = 0; x < w; )
			{
				decodeblock(in, tmp, TMPBLOCKSIZE, encoding);
				i = 0;
				do
				{
					if (x == w)
						break;
					for (y = 0; y < h; y++)
						out[w*y] = tmp[i+TMPBLOCKSIZE*y];
					out++;
					i++;
				} while (++x % blockwidth);
				in+=blockbytes;
			}
		}
	}
	return ret;
}
static pixel64_t *Image_Block_Decode64(qbyte *fte_restrict in, size_t insize, int w, int h, int d, void(*decodeblock)(qbyte *fte_restrict in, pixel64_t *fte_restrict out, int w, uploadfmt_t srcfmt), uploadfmt_t encoding)
{
#define TMPBLOCKSIZE 16u
	pixel64_t *ret, *out;
	pixel64_t tmp[TMPBLOCKSIZE*TMPBLOCKSIZE];
	int x, y, z, i, j;
	int sizediff;
	int rows, columns, layers;

	unsigned int blockbytes, blockwidth, blockheight, blockdepth;
	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	if (blockwidth > TMPBLOCKSIZE || blockheight > TMPBLOCKSIZE || blockdepth != 1)
		Sys_Error("Image_Block_Decode only supports up to %u*%u*%u blocks.\n", TMPBLOCKSIZE,TMPBLOCKSIZE,1);

	sizediff = insize - blockbytes*((w+blockwidth-1)/blockwidth)*((h+blockheight-1)/blockheight)*((d+blockdepth-1)/blockdepth);
	if (sizediff)
	{
		Con_Printf("Image_Block_Decode: %s data size is %u, expected %u\n\n", Image_FormatName(encoding), (unsigned int)insize, (unsigned int)(insize-sizediff));
		if (sizediff < 0)
			return NULL;
	}

	ret = out = BZ_Malloc(w*h*d*sizeof(*out));

	rows = h/blockheight;
	rows *= blockheight;
	columns = w/blockwidth;
	columns *= blockwidth;
	layers = d/blockdepth;
	layers *= blockdepth;
	for (z = 0; z < layers; z+=blockdepth)
	{
		for (y = 0; y < rows; y+=blockheight, out += w*(blockheight-1))
		{
			for (x = 0; x < columns; x+=blockwidth, in+=blockbytes, out+=blockwidth)
				decodeblock(in, out, w, encoding);
			if (w%blockwidth)
			{
				decodeblock(in, tmp, TMPBLOCKSIZE, encoding);
				for (i = 0; x < w; x++, out++, i++)
				{
					for (j = 0; j < blockheight; j++)
						out[w*j] = tmp[i+TMPBLOCKSIZE*j];
				}
				in+=blockbytes;
			}
		}
		if (h%blockheight)
		{	//now walk along the bottom of the image
			h %= blockheight;
			for (x = 0; x < w; )
			{
				decodeblock(in, tmp, TMPBLOCKSIZE, encoding);
				i = 0;
				do
				{
					if (x == w)
						break;
					for (y = 0; y < h; y++)
						out[w*y] = tmp[i+TMPBLOCKSIZE*y];
					out++;
					i++;
				} while (++x % blockwidth);
				in+=blockbytes;
			}
		}
	}
	return ret;
}

static qboolean Image_DecompressFormat(struct pendingtextureinfo *mips, const char *imagename)
{
	//various compressed formats might not be supported by various gpus/apis.
	//sometimes the gpu might only partially support the format (eg: d3d requires mip 0 be a multiple of the block size)
	//and sometimes we want the actual rgb data (eg: so that we can palettize it)
	//so this is still useful even if every driver ever created supported the format.
	//as a general rule, decompressing is fairly straight forward, but not free. yay threads.

	//iiuc any basic s3tc patents have now expired, so it is legally safe to decode (though fancy compression logic may still have restrictions, but we don't compress).
	static float throttle;
	void (*decodefunc)(qbyte *fte_restrict, pixel32_t *fte_restrict, int, uploadfmt_t) = NULL;
	void (*decodefunc64)(qbyte *fte_restrict, pixel64_t *fte_restrict, int, uploadfmt_t) = NULL;
	int rcoding = mips->encoding;
	int mip;
	switch(mips->encoding)
	{
	default:
		break;
	case PTI_RGB8:
		decodefunc = Image_Decode_RGB8_Block;
		rcoding = PTI_RGBX8;
		break;
	case PTI_L8A8:
		decodefunc = Image_Decode_L8A8_Block;
		rcoding = PTI_RGBA8;
		break;
	case PTI_L8:
		decodefunc = Image_Decode_L8_Block;
		rcoding = PTI_RGBA8;
		break;
#ifdef DECOMPRESS_ETC2
	case PTI_ETC1_RGB8:
	case PTI_ETC2_RGB8: //backwards compatible, so we just treat them the same
	case PTI_ETC2_RGB8_SRGB:
		decodefunc = Image_Decode_ETC2_RGB8_Block;
		rcoding = (mips->encoding==PTI_ETC2_RGB8_SRGB)?PTI_RGBX8_SRGB:PTI_RGBX8;
		break;
	case PTI_ETC2_RGB8A1:	//weird hack mode
	case PTI_ETC2_RGB8A1_SRGB:
		decodefunc = Image_Decode_ETC2_RGB8A1_Block;
		rcoding = (mips->encoding==PTI_ETC2_RGB8A1_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
		break;
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
		decodefunc = Image_Decode_ETC2_RGB8A8_Block;
		rcoding = (mips->encoding==PTI_ETC2_RGB8A8_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
		break;
	case PTI_EAC_R11:
		decodefunc = Image_Decode_EAC_R11U_Block;
		rcoding = PTI_RGBX8;
		break;
/*	case PTI_EAC_R11_SNORM:
		decodefunc = Image_Decode_EAC_R11S_Block;
		rcoding = PTI_RGBX8;
		break;*/
	case PTI_EAC_RG11:
		decodefunc = Image_Decode_EAC_RG11U_Block;
		rcoding = PTI_RGBX8;
		break;
/*	case PTI_EAC_RG11_SNORM:
		decodefunc = Image_Decode_EAC_RG11S_Block;
		rcoding = PTI_RGBX8;
		break;*/
#else
	case PTI_ETC1_RGB8:
	case PTI_ETC2_RGB8:
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:
		Con_ThrottlePrintf(&throttle, 0, "ETC1/ETC2/EAC decompression is not supported in this build\n");
		break;
#endif

	case PTI_BC1_RGB:
	case PTI_BC1_RGB_SRGB:
#ifdef DECOMPRESS_S3TC
		decodefunc = Image_Decode_BC1_Block;
		rcoding = (mips->encoding==PTI_BC1_RGB_SRGB)?PTI_RGBX8_SRGB:PTI_RGBX8;
#else
		Con_ThrottlePrintf(&throttle, 0, "BC1 decompression is not supported in this build\n");
#endif
		break;
	case PTI_BC1_RGBA:
	case PTI_BC1_RGBA_SRGB:
#ifdef DECOMPRESS_S3TC
		decodefunc = Image_Decode_BC1A_Block;
		rcoding = (mips->encoding==PTI_BC1_RGBA_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
#else
		Con_ThrottlePrintf(&throttle, 0, "BC1A decompression is not supported in this build\n");
#endif
		break;
	case PTI_BC2_RGBA:
	case PTI_BC2_RGBA_SRGB:
#ifdef DECOMPRESS_S3TC
		decodefunc = Image_Decode_BC2_Block;
		rcoding = (mips->encoding==PTI_BC2_RGBA_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
#else
		Con_ThrottlePrintf(&throttle, 0, "BC2 decompression is not supported in this build\n");
#endif
		break;
	case PTI_BC3_RGBA:
	case PTI_BC3_RGBA_SRGB:
#if defined(DECOMPRESS_RGTC) && defined(DECOMPRESS_S3TC)
		decodefunc = Image_Decode_BC3_Block;
		rcoding = (mips->encoding==PTI_BC3_RGBA_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
#else
		Con_ThrottlePrintf(&throttle, 0, "Fallback BC3 decompression is not supported in this build\n");
#endif
		break;
#ifdef DECOMPRESS_RGTC
	case PTI_BC4_R_SNORM:
	case PTI_BC4_R:
		decodefunc = Image_Decode_BC4_Block;
		rcoding = PTI_RGBX8;
		break;
	case PTI_BC5_RG_SNORM:
	case PTI_BC5_RG:
		decodefunc = Image_Decode_BC5_Block;
		rcoding = PTI_RGBX8;
		break;
#else
	case PTI_BC4_R_SNORM:
	case PTI_BC4_R:
	case PTI_BC5_RG_SNORM:
	case PTI_BC5_RG:
		Con_ThrottlePrintf(&throttle, 0, "Fallback BC4/BC5 decompression is not supported in this build\n");
		break;
#endif
	case PTI_BC6_RGB_UFLOAT:
#ifdef DECOMPRESS_BPTC
		decodefunc64 = Image_Decode_BC6_Block;
		rcoding = PTI_RGBA16F;
#else
		Con_ThrottlePrintf(&throttle, 0, "Fallback BC6_UFLOAT decompression is not supported\n");
#endif
		break;
	case PTI_BC6_RGB_SFLOAT:
#ifdef DECOMPRESS_BPTC
		decodefunc64 = Image_Decode_BC6_Block;
		rcoding = PTI_RGBA16F;
#else
		Con_ThrottlePrintf(&throttle, 0, "Fallback BC6_SFLOAT decompression is not supported\n");
#endif
		break;
	case PTI_BC7_RGBA:
	case PTI_BC7_RGBA_SRGB:
#ifdef DECOMPRESS_BPTC
		decodefunc = Image_Decode_BC7_Block;
		rcoding = (mips->encoding==PTI_BC7_RGBA_SRGB)?PTI_RGBA8_SRGB:PTI_RGBA8;
#else
		Con_ThrottlePrintf(&throttle, 0, "Fallback BC7 decompression is not supported\n");
#endif
		break;

	case PTI_ASTC_4X4_HDR:
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X12_HDR:
#if defined(DECOMPRESS_ASTC) && defined(ASTC_WITH_HDR)
//		decodefunc = Image_Decode_ASTC_HDR_E5_Block;
//		rcoding = PTI_E5BGR9;

		decodefunc64 = Image_Decode_ASTC_HDR_HF_Block;
		rcoding = PTI_RGBA16F;
		break;
#endif
	case PTI_ASTC_4X4_LDR:
	case PTI_ASTC_5X4_LDR:
	case PTI_ASTC_5X5_LDR:
	case PTI_ASTC_6X5_LDR:
	case PTI_ASTC_6X6_LDR:
	case PTI_ASTC_8X5_LDR:
	case PTI_ASTC_8X6_LDR:
	case PTI_ASTC_10X5_LDR:
	case PTI_ASTC_10X6_LDR:
	case PTI_ASTC_8X8_LDR:
	case PTI_ASTC_10X8_LDR:
	case PTI_ASTC_10X10_LDR:
	case PTI_ASTC_12X10_LDR:
	case PTI_ASTC_12X12_LDR:
#ifdef DECOMPRESS_ASTC
#ifdef ASTC_WITH_LDR
		decodefunc = Image_Decode_ASTC_LDR_U8_Block;
		rcoding = PTI_RGBA8;
#else
		decodefunc64 = Image_Decode_ASTC_HDR_HF_Block;
		rcoding = PTI_RGBA16F;
#endif
		break;
#endif
	case PTI_ASTC_4X4_SRGB:
	case PTI_ASTC_5X4_SRGB:
	case PTI_ASTC_5X5_SRGB:
	case PTI_ASTC_6X5_SRGB:
	case PTI_ASTC_6X6_SRGB:
	case PTI_ASTC_8X5_SRGB:
	case PTI_ASTC_8X6_SRGB:
	case PTI_ASTC_10X5_SRGB:
	case PTI_ASTC_10X6_SRGB:
	case PTI_ASTC_8X8_SRGB:
	case PTI_ASTC_10X8_SRGB:
	case PTI_ASTC_10X10_SRGB:
	case PTI_ASTC_12X10_SRGB:
	case PTI_ASTC_12X12_SRGB:
#if defined(DECOMPRESS_ASTC) && defined(ASTC_WITH_LDR)
		decodefunc = Image_Decode_ASTC_LDR_U8_Block;
		rcoding = PTI_RGBA8_SRGB;
#else
		Con_ThrottlePrintf(&throttle, 0, "Fallback ASTC decompression is not supported\n");
#endif
		break;
	case PTI_INVALID:
		Con_ThrottlePrintf(&throttle, 0, "Attempting to decompress invalid format\n");
		break;
	}
	if (decodefunc || decodefunc64)
	{
#ifndef IMGTOOL
		if (imagename)
			Con_DPrintf("Software-decoding %s (%s)\r", imagename, Image_FormatName(mips->encoding));
#endif
		for (mip = 0; mip < mips->mipcount; mip++)
		{
			size_t sz;
			void *out;
			if (decodefunc64)
			{
				sz = sizeof(pixel64_t);
				out = Image_Block_Decode64(mips->mip[mip].data, mips->mip[mip].datasize, mips->mip[mip].width, mips->mip[mip].height, mips->mip[mip].depth, decodefunc64, mips->encoding);
			}
			else
			{
				sz = sizeof(pixel32_t);
				out = Image_Block_Decode(mips->mip[mip].data, mips->mip[mip].datasize, mips->mip[mip].width, mips->mip[mip].height, mips->mip[mip].depth, decodefunc, mips->encoding);
			}
			if (mips->mip[mip].needfree)
				BZ_Free(mips->mip[mip].data);
			mips->mip[mip].data = out;
			mips->mip[mip].needfree = true;
			mips->mip[mip].datasize = mips->mip[mip].width*mips->mip[mip].height*sz;
		}
		if (mips->extrafree)
			BZ_Free(mips->extrafree);	//might as well free this now, as nothing is poking it any more.
		mips->extrafree = NULL;
		mips->encoding = rcoding;
		return true;
	}
	return false;
}

static struct
{
	uploadfmt_t src;
	uploadfmt_t dest;
	void (*dotransform) (struct pendingtextureinfo *mips, int arg);
	int arg;
	qboolean onebitalpha;
} formattransforms[] =
{	//more preferable ones should be first.
	//lossy transforms will hopefully not be automatically used, but are relevant for the imgtool.
	{PTI_LLLX8,		PTI_RGBX8,		Image_Tr_NoTransform},
	{PTI_LLLA8,		PTI_RGBA8,		Image_Tr_NoTransform},
	{PTI_LLLX8,		PTI_BGRX8,		Image_Tr_NoTransform},
	{PTI_LLLA8,		PTI_BGRA8,		Image_Tr_NoTransform},
	{PTI_RGBA8,		PTI_BGRA8,		Image_Tr_Swap8888},
	{PTI_BGRA8,		PTI_RGBA8,		Image_Tr_Swap8888},
	{PTI_RGBX8,		PTI_BGRX8,		Image_Tr_Swap8888},
	{PTI_BGRX8,		PTI_RGBX8,		Image_Tr_Swap8888},

	{PTI_RGBA16,	PTI_RGBA8,		Image_Tr_4X16to8888},

	//float transforms
	{PTI_RGB32F,	PTI_RGBA32F,	Image_Tr_RGB32FToFloat},
	{PTI_RGBA32F,	PTI_RGBA16F,	Image_Tr_FloatToHalf,	4},
	{PTI_R32F,		PTI_R16F,		Image_Tr_FloatToHalf,	1},
	{PTI_RGBA16F,	PTI_RGBA32F,	Image_Tr_HalfToFloat,	4},
	{PTI_R16F,		PTI_R32F,		Image_Tr_HalfToFloat,	1},
	{PTI_RGBA16F,	PTI_BGRA8,		Image_Tr_HalfToByte,	-4},
	{PTI_RGBA16F,	PTI_RGBA8,		Image_Tr_HalfToByte,	4},
	{PTI_R16F,		PTI_R8,			Image_Tr_HalfToByte,	1},
	{PTI_RGBA32F,	PTI_BGRA8,		Image_Tr_FloatToByte,	-4},
	{PTI_RGBA32F,	PTI_RGBA8,		Image_Tr_FloatToByte,	4},
	{PTI_R32F,		PTI_R8,			Image_Tr_FloatToByte,	1},
	{PTI_E5BGR9,	PTI_RGBX8,		Image_Tr_E5BGR9ToByte, false},
	{PTI_E5BGR9,	PTI_BGRX8,		Image_Tr_E5BGR9ToByte, true},
	{PTI_E5BGR9,	PTI_RGBA32F,	Image_Tr_E5BGR9ToFloat},
	{PTI_RGBA32F,	PTI_E5BGR9,		Image_Tr_FloatToE5BGR9},
	{PTI_B10G11R11F,PTI_RGBA32F,	Image_Tr_PackedToFloat},
	{PTI_RGBA32F,	PTI_B10G11R11F,	Image_Tr_FloatToPacked},

	{PTI_LLLA8,		PTI_RGBA5551,	Image_Tr_8888to5551,	false,	true},
	{PTI_RGBA8,		PTI_RGBA5551,	Image_Tr_8888to5551,	false,	true},
	{PTI_BGRA8,		PTI_RGBA5551,	Image_Tr_8888to5551,	true,	true},
	{PTI_LLLA8,		PTI_ARGB1555,	Image_Tr_8888to1555,	false,	true},
	{PTI_RGBA8,		PTI_ARGB1555,	Image_Tr_8888to1555,	false,	true},
	{PTI_BGRA8,		PTI_ARGB1555,	Image_Tr_8888to1555,	true,	true},
	{PTI_LLLA8,		PTI_RGBA4444,	Image_Tr_8888to4444,	false},
	{PTI_RGBA8,		PTI_RGBA4444,	Image_Tr_8888to4444,	false},
	{PTI_BGRA8,		PTI_RGBA4444,	Image_Tr_8888to4444,	true},
	{PTI_LLLA8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	false},
	{PTI_RGBA8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	false},
	{PTI_BGRA8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	true},

	{PTI_LLLX8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_RGBX8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_BGRX8,		PTI_RGB565,		Image_Tr_8888to565,	true,	true},
	{PTI_LLLX8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_RGBX8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_BGRX8,		PTI_RGB565,		Image_Tr_8888to565,	true,	true},

	{PTI_RGBA5551,	PTI_RGBA8,		Image_Tr_RGBA5551to8888,	false},
	{PTI_ARGB1555,	PTI_RGBA8,		Image_Tr_ARGB1555to8888,	false},
	{PTI_RGBA4444,	PTI_RGBA8,		Image_Tr_4444to8888,		false},
	{PTI_ARGB4444,	PTI_RGBA8,		Image_Tr_ARGB4444to8888,	false},

	{PTI_A2BGR10,	PTI_RGBA8,		Image_Tr_10To8},
	{PTI_RGBA8,		PTI_A2BGR10,	Image_Tr_8To10,		false,	true},
	{PTI_RGBA8,		PTI_RGBA32F,	Image_Tr_ByteToFloat,	4},

	//24bit formats are probably slow
	{PTI_LLLX8,		PTI_RGB8,		Image_Tr_DropBytes, (4<<16)|3},
	{PTI_RGBX8,		PTI_RGB8,		Image_Tr_DropBytes, (4<<16)|3},
	{PTI_BGRX8,		PTI_BGR8,		Image_Tr_DropBytes, (4<<16)|3},
	//these are last-resort. and will result in the alpha channel getting lost.
	{PTI_LLLA8,		PTI_RGB8,		Image_Tr_DropBytes, (4<<16)|3},
	{PTI_RGBA8,		PTI_RGB8,		Image_Tr_DropBytes, (4<<16)|3},
	{PTI_BGRA8,		PTI_BGR8,		Image_Tr_DropBytes, (4<<16)|3},
	{PTI_LLLA8,		PTI_RGBX8,		Image_Tr_NoTransform},
	{PTI_RGBA8,		PTI_RGBX8,		Image_Tr_NoTransform},
	{PTI_BGRA8,		PTI_BGRX8,		Image_Tr_NoTransform},
	{PTI_LLLA8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_RGBA8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_BGRA8,		PTI_RGB565,		Image_Tr_8888to565,	true,	true},
	{PTI_LLLA8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_RGBA8,		PTI_RGB565,		Image_Tr_8888to565,	false,	true},
	{PTI_BGRA8,		PTI_RGB565,		Image_Tr_8888to565,	true,	true},
	{PTI_RGBX8,		PTI_L8,			Image_Tr_8888toLuminence,		1,	true},
	{PTI_RGBA8,		PTI_L8A8,		Image_Tr_8888toLuminence,		2,	true},

	//FIXME: these don't pad alphas properly.
	{PTI_RGBX8,		PTI_RGBA8,		Image_Tr_NoTransform},
	{PTI_BGRX8,		PTI_BGRA8,		Image_Tr_NoTransform},
	{PTI_LLLX8,		PTI_RGBA5551,	Image_Tr_8888to5551,	false,	true},
	{PTI_RGBX8,		PTI_RGBA5551,	Image_Tr_8888to5551,	false,	true},
	{PTI_BGRX8,		PTI_RGBA5551,	Image_Tr_8888to5551,	true,	true},
	{PTI_LLLX8,		PTI_ARGB1555,	Image_Tr_8888to1555,	false,	true},
	{PTI_RGBX8,		PTI_ARGB1555,	Image_Tr_8888to1555,	false,	true},
	{PTI_BGRX8,		PTI_ARGB1555,	Image_Tr_8888to1555,	true,	true},
	{PTI_LLLX8,		PTI_RGBA4444,	Image_Tr_8888to4444,	false},
	{PTI_RGBX8,		PTI_RGBA4444,	Image_Tr_8888to4444,	false},
	{PTI_BGRX8,		PTI_RGBA4444,	Image_Tr_8888to4444,	true},
	{PTI_LLLX8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	false},
	{PTI_RGBX8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	false},
	{PTI_BGRX8,		PTI_ARGB4444,	Image_Tr_8888toARGB4444,	true},

	{PTI_RGBX8,		PTI_R8,			Image_Tr_DropBytes, (4<<16)|1, true},
	{PTI_RGBX8,		PTI_RG8,		Image_Tr_DropBytes, (4<<16)|2, true},	//for small normalmaps (b can be inferred, a isn't available so avoid offsetmapping)
	{PTI_RGBA16,	PTI_R16,		Image_Tr_DropBytes, (8<<16)|2, true},
	{PTI_RGBA16F,	PTI_R16F,		Image_Tr_DropBytes, (8<<16)|2, true},
	{PTI_RGBA32F,	PTI_R32F,		Image_Tr_DropBytes, (16<<16)|4, true},
	{PTI_RGBA32F,	PTI_RGB32F,		Image_Tr_DropBytes, (16<<16)|12, true},

	{PTI_RG8,		PTI_RGBX8,		Image_Tr_RG8ToRGXX8},
	{PTI_RGBX8,		PTI_P8,			Image_Tr_RGBX8toPaletted, 0|(256<<16)},
	{PTI_RGBX8,		TF_H2_TRANS8_0,	Image_Tr_RGBX8toPaletted, 1|(256<<16)},
	{PTI_RGBA8,		TF_H2_TRANS8_0,	Image_Tr_RGBA8toPaletted, 1|(256<<16)},
	{PTI_RGBX8,		TF_TRANS8,		Image_Tr_RGBX8toPaletted, 0|(255<<16)},
	{PTI_RGBA8,		TF_TRANS8,		Image_Tr_RGBA8toPaletted, 0|(255<<16)},
	{PTI_P8,		PTI_RGBX8,		Image_Tr_PalettedtoRGBX8, -1},
	{TF_SOLID8,		PTI_RGBX8,		Image_Tr_PalettedtoRGBX8, -1},
	{TF_H2_TRANS8_0,PTI_RGBA8,		Image_Tr_PalettedtoRGBX8, 0},
	{TF_TRANS8,		PTI_RGBA8,		Image_Tr_PalettedtoRGBX8, 255},
};
void Image_ChangeFormat(struct pendingtextureinfo *mips, qboolean *allowedformats, uploadfmt_t origfmt, const char *imagename)
{
	if (!allowedformats)
		allowedformats = sh_config.texfmt;

	//if that format isn't supported/desired, try converting it.
	if (allowedformats[mips->encoding])
	{
		if (mips->encoding >= PTI_ASTC_FIRST && mips->encoding <= PTI_ASTC_LAST)
			return;	//ignore texture_allow_block_padding for astc.
		else if (!sh_config.texture_allow_block_padding && mips->mipcount && mips->encoding)
		{	//direct3d is annoying, and will reject any block-compressed format with a base mip size that is not a multiple of the block size.
			//its fine with weirdly sized mips though. I have no idea why there's this restriction, but whatever.
			//we need to manually decompress in order to correctly handle such images
			int blockbytes, blockwidth, blockheight, blockdepth;
			Image_BlockSizeForEncoding(mips->encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);
			if (!(mips->mip[0].width % blockwidth) && !(mips->mip[0].height % blockheight) && !(mips->mip[0].depth % blockdepth))
				return;
			//else encoding isn't supported for this size. fall through.
		}
		else
			return;
	}

	//when the format can't be used, decompress it if its one of those awkward compressed formats.
	Image_DecompressFormat(mips, imagename);
	if (allowedformats[mips->encoding])
		return;	//okay, that got it.


	{
		qboolean onebitokay = (origfmt == TF_TRANS8 || origfmt == TF_TRANS8_FULLBRIGHT || origfmt == TF_H2_TRANS8_0 || !(sh_config.texfmt[PTI_RGBA4444] || sh_config.texfmt[PTI_ARGB4444]));
		uploadfmt_t src = mips->encoding;
		int i, j, first = -1, sec = -1;
		for (i = 0; i < countof(formattransforms); i++)
		{
			if (formattransforms[i].src == src)
			{
				if (allowedformats[formattransforms[i].dest])
				{
					if (formattransforms[i].onebitalpha && !onebitokay)
					{
						if (first < 0)	//as a last resort perhaps.
						{
							first = i;
							sec = -1;
						}
						continue;
					}

					//this is a direct conversion. yay.
					first = i;
					sec = -1;
					break;
				}

				//check if we can chain it to a second transform
				if (first != -1)
					continue;
				for (j = 0; j < countof(formattransforms); j++)
				{
					if (formattransforms[j].src == formattransforms[i].dest)
						if (allowedformats[formattransforms[j].dest])
						{
							first = i;
							sec = j;
							break;
						}
				}

				//FIXME: add a third transform, for final byteswaps?...
			}
		}

		if (first >= 0)
		{
			formattransforms[first].dotransform(mips, formattransforms[first].arg);
			mips->encoding = formattransforms[first].dest;
		}
		if (sec >= 0)
		{
			formattransforms[sec].dotransform(mips, formattransforms[sec].arg);
			mips->encoding = formattransforms[sec].dest;
		}

		if (allowedformats[mips->encoding])
			return;	//okay, that got it.
	}
}

static void Image_ChangeFormatFlags(struct pendingtextureinfo *mips, unsigned int flags, uploadfmt_t origfmt, const char *imagename)
{
	if (flags & IF_PALETTIZE)
	{	//paletizing things
		qboolean p8only[PTI_MAX] = {0};
		p8only[PTI_P8] = true;
		Image_ChangeFormat(mips, p8only, origfmt, imagename);
	}
	else
	{	//don't allow r8-as-indexes if its fed as an input. it won't make sense unless its explicit.
		qboolean p8 = sh_config.texfmt[PTI_P8];
		sh_config.texfmt[PTI_P8] = false;
		Image_ChangeFormat(mips, sh_config.texfmt, origfmt, imagename);
		sh_config.texfmt[PTI_P8] = p8;
	}
}

//operates in place...
void Image_Premultiply(struct pendingtextureinfo *mips)
{
	//works for rgba or bgra
	int i;
	switch(mips->encoding)
	{
	case PTI_RGBA32F:
		{
			float *fte_restrict premul = (float*)mips->mip[0].data;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++, premul+=4)
			{
				premul[0] = (premul[0] * premul[3]);
				premul[1] = (premul[1] * premul[3]);
				premul[2] = (premul[2] * premul[3]);
			}
		}
		break;
	case PTI_RGBA16F:
		{
			unsigned short *fte_restrict premul = (unsigned short*)mips->mip[0].data;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++, premul+=4)
			{
				float a = HalfToFloat(premul[3]);
				premul[0] = FloatToHalf(HalfToFloat(premul[0]) * a);
				premul[1] = FloatToHalf(HalfToFloat(premul[1]) * a);
				premul[2] = FloatToHalf(HalfToFloat(premul[2]) * a);
			}
		}
		break;
	case PTI_RGBA16:
		{
			unsigned short *fte_restrict premul = (unsigned short*)mips->mip[0].data;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++, premul+=4)
			{
				premul[0] = (premul[0] * premul[3])>>16;
				premul[1] = (premul[1] * premul[3])>>16;
				premul[2] = (premul[2] * premul[3])>>16;
			}
		}
		break;
	case PTI_A2BGR10:
		{
			unsigned int *fte_restrict premul = (unsigned int*)mips->mip[0].data, r,g,b,a;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++)
			{
				a =   (*premul>>30)&0x3;
				b = (((*premul>>20)&0x3ff)*a)>>2;
				g = (((*premul>>10)&0x3ff)*a)>>2;
				r = (((*premul>> 0)&0x3ff)*a)>>2;
				*premul++ = (a<<30)|(b<<20)|(g<<20)|(r<<0);
			}
		}
		break;
	case PTI_LLLX8:	//FIXME: why the Xs?
	case PTI_LLLA8:
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_BGRA8:
	case PTI_BGRX8:
	case PTI_RGBA8_SRGB:	//fixme: what's the correct multiplication for srgb?
	case PTI_RGBX8_SRGB:
	case PTI_BGRA8_SRGB:
	case PTI_BGRX8_SRGB:
		{
			qbyte *fte_restrict premul = (qbyte*)mips->mip[0].data;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++, premul+=4)
			{
				premul[0] = (premul[0] * premul[3])>>8;
				premul[1] = (premul[1] * premul[3])>>8;
				premul[2] = (premul[2] * premul[3])>>8;
			}
		}
		break;
	case PTI_L8A8:
	case PTI_L8A8_SRGB:
		{
			qbyte *fte_restrict premul = (qbyte*)mips->mip[0].data;
			for (i = 0; i < mips->mip[0].width*mips->mip[0].height; i++, premul+=2)
				premul[0] = (premul[0] * premul[1])>>8;
			break;
		}
	default:
		break;	//format not known, so no idea how to premultiply it. bc2/3 might already be premultiplied or not...
	}
}

//resamples and depalettes as required
//ALWAYS frees rawdata, even on failure (but never mips).
static qboolean Image_GenMip0(struct pendingtextureinfo *mips, unsigned int flags, void *rawdata, void *palettedata, int imgwidth, int imgheight, int imgdepth, uploadfmt_t fmt, qboolean freedata)
{
	unsigned int *rgbadata = rawdata;
	int i;
	qboolean valid;
	unsigned int bb, bw, bh, bd;

	mips->mip[0].width = imgwidth;
	mips->mip[0].height = imgheight;
	mips->mip[0].depth = imgdepth;
	mips->mipcount = 1;

	switch(fmt)
	{
	default:
		if (fmt&PTI_FULLMIPCHAIN)
		{
			fmt = fmt&~PTI_FULLMIPCHAIN;
			Image_RoundDimensions(&mips->mip[0].width, &mips->mip[0].height, &mips->mip[0].depth, flags);
			if (mips->mip[0].width == imgwidth && mips->mip[0].height == imgheight && mips->mip[0].depth == imgdepth)	//make sure its okay
			{
				size_t sz = 0;
				int is3d = (mips->type == PTI_3D)?1:0;
				Image_BlockSizeForEncoding(fmt, &bb, &bw, &bh, &bd);
				for (i = 0; i < countof(mips->mip) && (imgwidth || imgheight || (is3d && imgdepth)); i++, imgwidth>>=1, imgheight>>=1, imgdepth>>=is3d)
				{
					mips->mip[i].width = max(1,imgwidth);
					mips->mip[i].height = max(1,imgheight);
					mips->mip[i].depth = max(1,imgdepth);
					mips->mip[i].datasize = bb * ((mips->mip[i].width+bw-1)/bw) * ((mips->mip[i].height+bh-1)/bh) * ((mips->mip[i].depth+bd-1)/bd);
					mips->mip[i].needfree = false;
					sz += mips->mip[i].datasize;
				}
				mips->mipcount = i;
				mips->encoding = fmt;
				if (!freedata)
				{
					rgbadata = BZ_Malloc(sz);
					memcpy(rgbadata, rawdata, sz);
				}
				mips->extrafree = rawdata = rgbadata;
				for (i = 0; i < mips->mipcount; i++)
				{
					mips->mip[i].data = rawdata;
					rawdata = (qbyte*)rawdata+mips->mip[i].datasize;
				}
				return true;
			}
		}
		mips->encoding = fmt;
		break;

	baddepth:
		Con_Printf("R_LoadRawTexture: bad depth for format\n");
		if (freedata)
			BZ_Free(rawdata);
		return false;
	case TF_INVALID:
		Con_Printf("R_LoadRawTexture: bad format\n");
		if (freedata)
			BZ_Free(rawdata);
		return false;

	case TF_MIP4_P8:
		//8bit indexed data.
		Image_RoundDimensions(&mips->mip[0].width, &mips->mip[0].height, &mips->mip[0].depth, flags);
		flags |= IF_NOPICMIP;
		if (/*!r_dodgymiptex.ival &&*/ mips->mip[0].width == imgwidth && mips->mip[0].height == imgheight && mips->mip[0].depth == 1)
		{
			unsigned int pixels =
				(imgwidth>>0) * (imgheight>>0) + 
				(imgwidth>>1) * (imgheight>>1) +
				(imgwidth>>2) * (imgheight>>2) +
				(imgwidth>>3) * (imgheight>>3);

			mips->encoding = PTI_P8;
			rgbadata = BZ_Malloc(pixels);
			memcpy(rgbadata, rawdata, pixels);

			for (i = 0; i < 4; i++)
			{
				mips->mip[i].width = imgwidth>>i;
				mips->mip[i].height = imgheight>>i;
				mips->mip[i].depth = 1;
				mips->mip[i].datasize = mips->mip[i].width * mips->mip[i].height;
				mips->mip[i].needfree = false;
			}
			mips->mipcount = i;
			mips->mip[0].data = rgbadata;
			mips->mip[1].data = (qbyte*)mips->mip[0].data + mips->mip[0].datasize;
			mips->mip[2].data = (qbyte*)mips->mip[1].data + mips->mip[1].datasize;
			mips->mip[3].data = (qbyte*)mips->mip[2].data + mips->mip[2].datasize;

			mips->extrafree = rgbadata;
			if (freedata)
				BZ_Free(rawdata);
			return true;
		}
		//fall through
	case PTI_LLLX8:
		if (sh_config.texfmt[((vid.flags & VID_SRGBAWARE) /*&& (flags & IF_SRGB)*/ && !(flags & IF_NOSRGB))?PTI_L8_SRGB:PTI_L8])
		{	//if we can compact it, then do so!
			mips->encoding = PTI_L8;
			//can just do this in-place.
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
				((qbyte*)rgbadata)[i] = ((qbyte*)rgbadata)[i*4];
		}
		//otherwise treat it as whatever the gpu prefers
		else if (sh_config.texfmt[PTI_BGRX8] || sh_config.texfmt[PTI_BGRA8])
			mips->encoding = PTI_BGRX8;
		else
			mips->encoding = PTI_RGBX8;
		break;
	case PTI_LLLA8:
		//take special care here, because L8A8_SRGB doesn't exist in core gl, nor can it be easily faked.
		if (sh_config.texfmt[((vid.flags & VID_SRGBAWARE) /*&& (flags & IF_SRGB)*/ && !(flags & IF_NOSRGB))?PTI_L8A8_SRGB:PTI_L8A8])
		{	//if we can compact it, then do so!
			mips->encoding = PTI_L8A8;
			//can just do this in-place.
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			{
				((qbyte*)rgbadata)[i*2+0] = ((qbyte*)rgbadata)[i*4+0];
				((qbyte*)rgbadata)[i*2+1] = ((qbyte*)rgbadata)[i*4+3];
			}
		}
		else if (sh_config.texfmt[PTI_BGRA8])
			mips->encoding = PTI_BGRA8;
		else
			mips->encoding = PTI_RGBA8;
		break;
	case TF_MIP4_SOLID8:
		//8bit opaque data
		Image_RoundDimensions(&mips->mip[0].width, &mips->mip[0].height, &mips->mip[0].depth, flags);
		flags |= IF_NOPICMIP;
#ifdef HAVE_CLIENT
		if (!r_dodgymiptex.ival && mips->mip[0].width == imgwidth && mips->mip[0].height == imgheight && mips->mip[0].depth == 1)
		{	//special hack required to preserve the hand-drawn lower mips.
			unsigned int pixels =
				(imgwidth>>0) * (imgheight>>0) + 
				(imgwidth>>1) * (imgheight>>1) +
				(imgwidth>>2) * (imgheight>>2) +
				(imgwidth>>3) * (imgheight>>3);

			mips->encoding = PTI_RGBX8;
			rgbadata = BZ_Malloc(pixels*4);
			for (i = 0; i < pixels; i++)
				rgbadata[i] = d_8to24rgbtable[((qbyte*)rawdata)[i]];

			for (i = 0; i < 4; i++)
			{
				mips->mip[i].width = imgwidth>>i;
				mips->mip[i].height = imgheight>>i;
				mips->mip[i].depth = 1;
				mips->mip[i].datasize = mips->mip[i].width * mips->mip[i].height * 4;
				mips->mip[i].needfree = false;
			}
			mips->mipcount = i;
			mips->mip[0].data = rgbadata;
			mips->mip[1].data = (qbyte*)mips->mip[0].data + mips->mip[0].datasize;
			mips->mip[2].data = (qbyte*)mips->mip[1].data + mips->mip[1].datasize;
			mips->mip[3].data = (qbyte*)mips->mip[2].data + mips->mip[2].datasize;

			mips->extrafree = rgbadata;
			if (freedata)
				BZ_Free(rawdata);
			return true;
		}
#endif
		//fall through
	case TF_SOLID8:
		rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
		if (sh_config.texfmt[PTI_BGRX8])
		{	//bgra8 is typically faster when supported.
			mips->encoding = PTI_BGRX8;
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
				rgbadata[i] = d_8to24bgrtable[((qbyte*)rawdata)[i]];
		}
		else
		{
			mips->encoding = PTI_RGBX8;
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
				rgbadata[i] = d_8to24rgbtable[((qbyte*)rawdata)[i]];
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;
	case TF_TRANS8:
		{
			mips->encoding = PTI_RGBX8;
			rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			{
				if (((qbyte*)rawdata)[i] == 0xff)
				{//fixme: blend non-0xff neighbours. no, just use premultiplied alpha instead, where it matters.
					rgbadata[i] = 0;
					mips->encoding = PTI_RGBA8;
				}
				else
					rgbadata[i] = d_8to24rgbtable[((qbyte*)rawdata)[i]];
			}
			if (freedata)
				BZ_Free(rawdata);
			freedata = true;
		}
		break;
	case TF_H2_TRANS8_0:
		{
			mips->encoding = PTI_RGBX8;
			rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			{
				qbyte px = ((qbyte*)rawdata)[i];
				//Note: The proper value here is 0.
				//However, hexen2 has a bug that ALSO treats 255 the same way, but ONLY in the GL version.
				//So allow both.
				if (px == 0xff || px == 0)
				{//fixme: blend opaque neighbours? no, just use premultiplied alpha instead, where it matters.
					rgbadata[i] = 0;
					mips->encoding = PTI_RGBA8;
				}
				else
					rgbadata[i] = d_8to24rgbtable[px];
			}
			if (freedata)
				BZ_Free(rawdata);
			freedata = true;
		}
		break;
	case TF_TRANS8_FULLBRIGHT:
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
		for (i = 0, valid = false; i < imgwidth * imgheight * imgdepth; i++)
		{
			if (((qbyte*)rawdata)[i] == 255 || ((qbyte*)rawdata)[i] < 256-vid.fullbright)
				rgbadata[i] = 0;
			else
			{
				rgbadata[i] = d_8to24rgbtable[((qbyte*)rawdata)[i]];
				valid = true;
			}
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		if (!valid)
		{
			BZ_Free(rgbadata);
			return false;
		}
		break;

	case TF_HEIGHT8PAL:
		if (imgdepth != 1)
			goto baddepth;
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgwidth * imgheight*5);
		{
			qbyte *heights = (qbyte*)(rgbadata + (imgwidth*imgheight * imgdepth));
			for (i = 0; i < imgwidth * imgheight; i++)
			{
				unsigned int rgb = d_8to24rgbtable[((qbyte*)rawdata)[i]];
				heights[i] = (((rgb>>16)&0xff) + ((rgb>>8)&0xff) + ((rgb>>0)&0xff))/3;
			}
#ifndef HAVE_CLIENT
			Image_GenerateNormalMap(heights, rgbadata, imgwidth, imgheight, 4, 0);
#else
			Image_GenerateNormalMap(heights, rgbadata, imgwidth, imgheight, r_shadow_bumpscale_basetexture.value?r_shadow_bumpscale_basetexture.value:4, r_shadow_heightscale_basetexture.value);
#endif
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;
	case TF_HEIGHT8:
		if (imgdepth != 1)
			goto baddepth;
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgwidth * imgheight*4);
#ifndef HAVE_CLIENT
		Image_GenerateNormalMap(rawdata, rgbadata, imgwidth, imgheight, 4, 1);
#else
		Image_GenerateNormalMap(rawdata, rgbadata, imgwidth, imgheight, r_shadow_bumpscale_bumpmap.value, r_shadow_heightscale_bumpmap.value);
#endif
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;

	case TF_BGR24_FLIP:
		if (imgdepth != 1)
			goto baddepth;
		mips->encoding = PTI_RGBX8;
		rgbadata = BZ_Malloc(imgwidth * imgheight*4);
		for (i = 0; i < imgheight; i++)
		{
			int x;
			qbyte *in = (qbyte*)rawdata + (imgheight-i-1) * imgwidth * 3;
			qbyte *out = (qbyte*)rgbadata + i * imgwidth * 4;
			for (x = 0; x < imgwidth; x++, in+=3, out+=4)
			{
				out[0] = in[2];
				out[1] = in[1];
				out[2] = in[0];
				out[3] = 0xff;
			}
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;

	case TF_MIP4_8PAL24_T255:
	case TF_MIP4_8PAL24:
		//8bit opaque data
		{
			unsigned int pixels =
					(imgwidth>>0) * (imgheight>>0) + 
					(imgwidth>>1) * (imgheight>>1) +
					(imgwidth>>2) * (imgheight>>2) +
					(imgwidth>>3) * (imgheight>>3);
			palettedata = (qbyte*)rawdata + pixels;
			Image_RoundDimensions(&mips->mip[0].width, &mips->mip[0].height, &mips->mip[0].depth, flags);
			flags |= IF_NOPICMIP;
#ifdef HAVE_CLIENT
			if (!r_dodgymiptex.ival && mips->mip[0].width == imgwidth && mips->mip[0].height == imgheight && mips->mip[0].depth == 1)
			{
				unsigned int pixels =
					(imgwidth>>0) * (imgheight>>0) + 
					(imgwidth>>1) * (imgheight>>1) +
					(imgwidth>>2) * (imgheight>>2) +
					(imgwidth>>3) * (imgheight>>3);

				rgbadata = BZ_Malloc(pixels*4);
				if (fmt == TF_MIP4_8PAL24_T255)
				{
					mips->encoding = PTI_RGBA8;
					for (i = 0; i < pixels; i++)
					{
						qbyte idx = ((qbyte*)rawdata)[i];
						if (idx == 255)
							rgbadata[i] = 0;
						else
						{
							qbyte *p = ((qbyte*)palettedata) + idx*3;
							rgbadata[i] = 0xff000000 | (p[0]<<0) | (p[1]<<8) | (p[2]<<16);	//FIXME: endian
						}
					}
				}
				else
				{
					mips->encoding = PTI_RGBX8;
					for (i = 0; i < pixels; i++)
					{
						qbyte *p = ((qbyte*)palettedata) + ((qbyte*)rawdata)[i]*3;
						//FIXME: endian
						rgbadata[i] = 0xff000000 | (p[0]<<0) | (p[1]<<8) | (p[2]<<16);
					}
				}

				for (i = 0; i < 4; i++)
				{
					mips->mip[i].width = imgwidth>>i;
					mips->mip[i].height = imgheight>>i;
					mips->mip[i].depth = 1;
					mips->mip[i].datasize = mips->mip[i].width * mips->mip[i].height * 4;
					mips->mip[i].needfree = false;
				}
				mips->mipcount = i;
				mips->mip[0].data = rgbadata;
				mips->mip[1].data = (qbyte*)mips->mip[0].data + mips->mip[0].datasize;
				mips->mip[2].data = (qbyte*)mips->mip[1].data + mips->mip[1].datasize;
				mips->mip[3].data = (qbyte*)mips->mip[2].data + mips->mip[2].datasize;

				mips->extrafree = rgbadata;
				if (freedata)
					BZ_Free(rawdata);
				return true;
			}
#endif
		}
		//fall through
	case TF_8PAL24:
		if (!palettedata)
		{
			Con_Printf("TF_8PAL24: no palette");
			if (freedata)
				BZ_Free(rawdata);
			return false;
		}
		rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
		if (fmt == TF_MIP4_8PAL24_T255)
		{
			mips->encoding = PTI_RGBA8;
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			{
				qbyte idx = ((qbyte*)rawdata)[i];
				if (idx == 255)
					rgbadata[i] = 0;
				else
				{
					qbyte *p = ((qbyte*)palettedata) + idx*3;
					rgbadata[i] = 0xff000000 | (p[0]<<0) | (p[1]<<8) | (p[2]<<16);	//FIXME: endian
				}
			}
		}
		else
		{
			mips->encoding = PTI_RGBX8;
			for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			{
				qbyte *p = ((qbyte*)palettedata) + ((qbyte*)rawdata)[i]*3;
				//FIXME: endian
				rgbadata[i] = 0xff000000 | (p[0]<<0) | (p[1]<<8) | (p[2]<<16);
			}
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;
	case TF_8PAL32:
		if (!palettedata)
		{
			Con_Printf("TF_8PAL32: no palette");
			if (freedata)
				BZ_Free(rawdata);
			return false;
		}
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
		for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
			rgbadata[i] = ((unsigned int*)palettedata)[((qbyte*)rawdata)[i]];
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;

#ifdef HEXEN2
	case TF_H2_T7G1: /*8bit data, odd indexes give greyscale transparence*/
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgdepth * imgwidth * imgheight*4);
		for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
		{
			qbyte p = ((qbyte*)rawdata)[i];
			rgbadata[i] = d_8to24rgbtable[p] & 0x00ffffff;
			if (p == 0)
				;
			else if (p&1)
				rgbadata[i] |= 0x80000000;
			else
				rgbadata[i] |= 0xff000000;
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;
	case TF_H2_T4A4:     /*8bit data, weird packing*/
		mips->encoding = PTI_RGBA8;
		rgbadata = BZ_Malloc(imgdepth * imgheight * imgwidth*4);
		for (i = 0; i < imgwidth * imgheight * imgdepth; i++)
		{
			static const int ColorIndex[16] = {0x00, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f, 0x8f, 0x9f, 0xaf, 0xbf, 0xc7, 0xcf, 0xdf, 0xe7};
			static const unsigned ColorPercent[16] = {25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247};
			qbyte p = ((qbyte*)rawdata)[i];
			rgbadata[i] = d_8to24rgbtable[ColorIndex[p>>4]] & 0x00ffffff;
			rgbadata[i] |= ( int )ColorPercent[p&15] << 24;
		}
		if (freedata)
			BZ_Free(rawdata);
		freedata = true;
		break;
#endif
	}

	if (flags & IF_NOALPHA)
	{
		safeswitch(mips->encoding)
		{
		case PTI_RGBA8:
			mips->encoding = PTI_RGBX8;
			break;
		case PTI_BGRA8:
			mips->encoding = PTI_BGRX8;
			break;
		case PTI_RGBA8_SRGB:
			mips->encoding = PTI_RGBX8_SRGB;
			break;
		case PTI_BGRA8_SRGB:
			mips->encoding = PTI_BGRX8_SRGB;
			break;
		case PTI_RGBA16:
		case PTI_RGBA16F:
		case PTI_RGBA32F:
		case PTI_ARGB4444:
		case PTI_ARGB1555:
		case PTI_RGBA4444:
		case PTI_RGBA5551:
		case PTI_A2BGR10:
		case PTI_L8A8: //could strip.
		case PTI_L8A8_SRGB: //could strip.
			break;	//erk
		case PTI_BC1_RGBA:
			mips->encoding = PTI_BC1_RGB;
			break;
		case PTI_BC1_RGBA_SRGB:
			mips->encoding = PTI_BC1_RGB_SRGB;
			break;
		case PTI_BC2_RGBA:	//could strip to PTI_BC1_RGB
		case PTI_BC2_RGBA_SRGB:	//could strip to PTI_BC1_RGB
		case PTI_BC3_RGBA:	//could strip to PTI_BC1_RGB
		case PTI_BC3_RGBA_SRGB:	//could strip to PTI_BC1_RGB
		case PTI_BC7_RGBA:	//much too messy...
		case PTI_BC7_RGBA_SRGB:
		case PTI_ETC2_RGB8A1: //would need to force the 'opaque' bit in each block and treat as PTI_ETC2_RGB8.
		case PTI_ETC2_RGB8A1_SRGB: //would need to force the 'opaque' bit in each block and treat as PTI_ETC2_RGB8.
		case PTI_ETC2_RGB8A8: //could strip to PTI_ETC2_RGB8
		case PTI_ETC2_RGB8A8_SRGB: //could strip to PTI_ETC2_SRGB8
		case PTI_ASTC_4X4_LDR:
		case PTI_ASTC_4X4_SRGB:
		case PTI_ASTC_4X4_HDR:
		case PTI_ASTC_5X4_LDR:
		case PTI_ASTC_5X4_SRGB:
		case PTI_ASTC_5X4_HDR:
		case PTI_ASTC_5X5_LDR:
		case PTI_ASTC_5X5_SRGB:
		case PTI_ASTC_5X5_HDR:
		case PTI_ASTC_6X5_LDR:
		case PTI_ASTC_6X5_SRGB:
		case PTI_ASTC_6X5_HDR:
		case PTI_ASTC_6X6_LDR:
		case PTI_ASTC_6X6_SRGB:
		case PTI_ASTC_6X6_HDR:
		case PTI_ASTC_8X5_LDR:
		case PTI_ASTC_8X5_SRGB:
		case PTI_ASTC_8X5_HDR:
		case PTI_ASTC_8X6_LDR:
		case PTI_ASTC_8X6_SRGB:
		case PTI_ASTC_8X6_HDR:
		case PTI_ASTC_10X5_LDR:
		case PTI_ASTC_10X5_SRGB:
		case PTI_ASTC_10X5_HDR:
		case PTI_ASTC_10X6_LDR:
		case PTI_ASTC_10X6_SRGB:
		case PTI_ASTC_10X6_HDR:
		case PTI_ASTC_8X8_LDR:
		case PTI_ASTC_8X8_SRGB:
		case PTI_ASTC_8X8_HDR:
		case PTI_ASTC_10X8_LDR:
		case PTI_ASTC_10X8_SRGB:
		case PTI_ASTC_10X8_HDR:
		case PTI_ASTC_10X10_LDR:
		case PTI_ASTC_10X10_SRGB:
		case PTI_ASTC_10X10_HDR:
		case PTI_ASTC_12X10_LDR:
		case PTI_ASTC_12X10_SRGB:
		case PTI_ASTC_12X10_HDR:
		case PTI_ASTC_12X12_LDR:
		case PTI_ASTC_12X12_SRGB:
		case PTI_ASTC_12X12_HDR:
#ifdef ASTC3D
		case PTI_ASTC_3X3X3_HDR:
		case PTI_ASTC_3X3X3_SRGB:
		case PTI_ASTC_3X3X3_LDR:
		case PTI_ASTC_4X3X3_HDR:
		case PTI_ASTC_4X3X3_SRGB:
		case PTI_ASTC_4X3X3_LDR:
		case PTI_ASTC_4X4X3_HDR:
		case PTI_ASTC_4X4X3_SRGB:
		case PTI_ASTC_4X4X3_LDR:
		case PTI_ASTC_4X4X4_HDR:
		case PTI_ASTC_4X4X4_SRGB:
		case PTI_ASTC_4X4X4_LDR:
		case PTI_ASTC_5X4X4_HDR:
		case PTI_ASTC_5X4X4_SRGB:
		case PTI_ASTC_5X4X4_LDR:
		case PTI_ASTC_5X5X4_HDR:
		case PTI_ASTC_5X5X4_SRGB:
		case PTI_ASTC_5X5X4_LDR:
		case PTI_ASTC_5X5X5_HDR:
		case PTI_ASTC_5X5X5_SRGB:
		case PTI_ASTC_5X5X5_LDR:
		case PTI_ASTC_6X5X5_HDR:
		case PTI_ASTC_6X5X5_SRGB:
		case PTI_ASTC_6X5X5_LDR:
		case PTI_ASTC_6X6X5_HDR:
		case PTI_ASTC_6X6X5_SRGB:
		case PTI_ASTC_6X6X5_LDR:
		case PTI_ASTC_6X6X6_HDR:
		case PTI_ASTC_6X6X6_SRGB:
		case PTI_ASTC_6X6X6_LDR:
#endif
#ifdef FTE_TARGET_WEB
		case PTI_WHOLEFILE:
#endif
			//erk. meh.
			break;
		case PTI_L8:
		case PTI_L8_SRGB:
		case PTI_R16:
		case PTI_P8:
		case PTI_R8:
		case PTI_R8_SNORM:
		case PTI_RG8:
		case PTI_RG8_SNORM:
		case PTI_RGB565:
		case PTI_RGB8:
		case PTI_BGR8:
		case PTI_RGB8_SRGB:
		case PTI_BGR8_SRGB:
		case PTI_RGB32F:
		case PTI_E5BGR9:
		case PTI_B10G11R11F:
		case PTI_RGBX8:
		case PTI_BGRX8:
		case PTI_RGBX8_SRGB:
		case PTI_BGRX8_SRGB:
		case PTI_BC1_RGB:
		case PTI_BC1_RGB_SRGB:
		case PTI_BC4_R:
		case PTI_BC4_R_SNORM:
		case PTI_BC5_RG:
		case PTI_BC5_RG_SNORM:
		case PTI_BC6_RGB_UFLOAT:
		case PTI_BC6_RGB_SFLOAT:
		case PTI_ETC1_RGB8:
		case PTI_ETC2_RGB8:
		case PTI_ETC2_RGB8_SRGB:
		case PTI_EAC_R11:
		case PTI_EAC_R11_SNORM:
		case PTI_EAC_RG11:
		case PTI_EAC_RG11_SNORM:
		case PTI_R16F:
		case PTI_R32F:
			break;	//already no alpha in these formats
		case PTI_DEPTH16:
		case PTI_DEPTH24:
		case PTI_DEPTH32:
		case PTI_DEPTH24_8:
			break;
		case PTI_EMULATED:
		case PTI_MAX:
		safedefault:
			break;	//stfu
		}
		//FIXME: fill alpha channel with 255?
	}

	if ((vid.flags & VID_SRGBAWARE) /*&& (flags & IF_SRGB)*/ && !(flags & IF_NOSRGB))
	{	//most modern editors write srgb images.
		//however, that might not be supported.
		uploadfmt_t nf = PTI_MAX;
		switch(mips->encoding)
		{
		case PTI_R8:			nf = PTI_INVALID; break;
		case PTI_L8:			nf = PTI_L8_SRGB; break;
		case PTI_L8A8:			nf = PTI_L8A8_SRGB; break;
		case PTI_LLLX8:			nf = PTI_RGBX8_SRGB; break;
		case PTI_LLLA8:			nf = PTI_RGBA8_SRGB; break;
		case PTI_RGBA8:			nf = PTI_RGBA8_SRGB; break;
		case PTI_RGBX8:			nf = PTI_RGBX8_SRGB; break;
		case PTI_BGRA8:			nf = PTI_BGRA8_SRGB; break;
		case PTI_BGRX8:			nf = PTI_BGRX8_SRGB; break;
		case PTI_RGB8:			nf = PTI_RGB8_SRGB; break;
		case PTI_BGR8:			nf = PTI_BGR8_SRGB; break;
		case PTI_BC1_RGB:		nf = PTI_BC1_RGB_SRGB; break;
		case PTI_BC1_RGBA:		nf = PTI_BC1_RGBA_SRGB; break;
		case PTI_BC2_RGBA:		nf = PTI_BC2_RGBA_SRGB; break;
		case PTI_BC3_RGBA:		nf = PTI_BC3_RGBA_SRGB; break;
		case PTI_BC7_RGBA:		nf = PTI_BC7_RGBA_SRGB; break;
		case PTI_ETC1_RGB8:		nf = PTI_ETC2_RGB8_SRGB; break;
		case PTI_ETC2_RGB8:		nf = PTI_ETC2_RGB8_SRGB; break;
		case PTI_ETC2_RGB8A1:	nf = PTI_ETC2_RGB8A1_SRGB; break;
		case PTI_ETC2_RGB8A8:	nf = PTI_ETC2_RGB8A8_SRGB; break;
		case PTI_ASTC_4X4_LDR:	nf = PTI_ASTC_4X4_SRGB; break;
		case PTI_ASTC_5X4_LDR:	nf = PTI_ASTC_5X4_SRGB; break;
		case PTI_ASTC_5X5_LDR:	nf = PTI_ASTC_5X5_SRGB; break;
		case PTI_ASTC_6X5_LDR:	nf = PTI_ASTC_6X5_SRGB; break;
		case PTI_ASTC_6X6_LDR:	nf = PTI_ASTC_6X6_SRGB; break;
		case PTI_ASTC_8X5_LDR:	nf = PTI_ASTC_8X5_SRGB; break;
		case PTI_ASTC_8X6_LDR:	nf = PTI_ASTC_8X6_SRGB; break;
		case PTI_ASTC_10X5_LDR:	nf = PTI_ASTC_10X5_SRGB; break;
		case PTI_ASTC_10X6_LDR:	nf = PTI_ASTC_10X6_SRGB; break;
		case PTI_ASTC_8X8_LDR:	nf = PTI_ASTC_8X8_SRGB; break;
		case PTI_ASTC_10X8_LDR:	nf = PTI_ASTC_10X8_SRGB; break;
		case PTI_ASTC_10X10_LDR:nf = PTI_ASTC_10X10_SRGB; break;
		case PTI_ASTC_12X10_LDR:nf = PTI_ASTC_12X10_SRGB; break;
		case PTI_ASTC_12X12_LDR:nf = PTI_ASTC_12X12_SRGB; break;

		//these formats are inherantly linear. oh well.
		case PTI_R16:
		case PTI_R16F:
		case PTI_R32F:
		case PTI_RGBA16:
		case PTI_RGBA16F:
		case PTI_RGBA32F:
			nf = mips->encoding;
			break;
		default:
			if (freedata)
				BZ_Free(rgbadata);
			return false;
		}
		if (sh_config.texfmt[nf])
			mips->encoding = nf;
		else
		{	//srgb->linear
			int m = mips->mip[0].width*mips->mip[0].height*mips->mip[0].depth;

			switch(mips->encoding)
			{
			case PTI_RGB8:
			case PTI_BGR8:
				m*=3;
				//fallthrough
			case PTI_R8:
			case PTI_L8:
				for (i = 0; i < m; i++)
					((qbyte*)rgbadata)[i+0] = 255*Image_LinearFloatFromsRGBFloat(((qbyte*)rgbadata)[i+0] * (1.0/255));
				break;
			case PTI_L8A8:
				m*=2;
				for (i = 0; i < m; i+=2)
					((qbyte*)rgbadata)[i+0] = 255*Image_LinearFloatFromsRGBFloat(((qbyte*)rgbadata)[i+0] * (1.0/255));
				break;
			case PTI_R16:
				for (i = 0; i < m; i++)
					((unsigned short*)rgbadata)[i+0] = 0xffff*Image_LinearFloatFromsRGBFloat(((unsigned short*)rgbadata)[i+0] * (1.0/0xffff));
				break;
			case PTI_RGBA16:
				m*=4;
				for (i = 0; i < m; i+=4)
				{
					((unsigned short*)rgbadata)[i+0] = 0xffff*Image_LinearFloatFromsRGBFloat(((unsigned short*)rgbadata)[i+0] * (1.0/0xffff));
					((unsigned short*)rgbadata)[i+1] = 0xffff*Image_LinearFloatFromsRGBFloat(((unsigned short*)rgbadata)[i+1] * (1.0/0xffff));
					((unsigned short*)rgbadata)[i+2] = 0xffff*Image_LinearFloatFromsRGBFloat(((unsigned short*)rgbadata)[i+2] * (1.0/0xffff));
				}
				break;
			case PTI_RGBA8:
			case PTI_RGBX8:
			case PTI_BGRA8:
			case PTI_BGRX8:
				m*=4;
				for (i = 0; i < m; i+=4)
				{
					((qbyte*)rgbadata)[i+0] = 255*Image_LinearFloatFromsRGBFloat(((qbyte*)rgbadata)[i+0] * (1.0/255));
					((qbyte*)rgbadata)[i+1] = 255*Image_LinearFloatFromsRGBFloat(((qbyte*)rgbadata)[i+1] * (1.0/255));
					((qbyte*)rgbadata)[i+2] = 255*Image_LinearFloatFromsRGBFloat(((qbyte*)rgbadata)[i+2] * (1.0/255));
				}
				break;
			case PTI_BC1_RGB:
			case PTI_BC1_RGBA:
			case PTI_BC2_RGBA:
			case PTI_BC3_RGBA:
				//FIXME: bc1/2/3 has two leading 16bit 565 values per block.
			default:
				//these formats are weird. we can't just fiddle with the rgbdata
				//FIXME: etc2 has all sorts of weird encoding tables...
				if (freedata)
					BZ_Free(rgbadata);
				return false;
			}
		}
	}


	Image_RoundDimensions(&mips->mip[0].width, &mips->mip[0].height, &mips->mip[0].depth, flags);
	if (rgbadata)
	{
		if (mips->mip[0].width == imgwidth && mips->mip[0].height == imgheight && mips->mip[0].depth == imgdepth)
			mips->mip[0].data = rgbadata;
		else
		{
			if (imgdepth == 1 &&
				(mips->mip[0].data=Image_ResampleTexture(mips->encoding, rgbadata, imgwidth, imgheight, NULL, mips->mip[0].width, mips->mip[0].height))	//actually rescale it here.
				)
			{
				if (freedata)
					BZ_Free(rgbadata);
				freedata = true;
			}
			else
			{	//rescaling unsupported
				mips->mip[0].data = rgbadata;
				mips->mip[0].width = imgwidth;
				mips->mip[0].height = imgheight;
				mips->mip[0].depth = imgdepth;
			}
		}
	}
	else
		mips->mip[0].data = NULL;
	Image_BlockSizeForEncoding(mips->encoding, &bb, &bw, &bh, &bd);
	mips->mip[0].datasize = ((mips->mip[0].width+bw-1)/bw) * ((mips->mip[0].height+bh-1)/bh) * ((mips->mip[0].depth+bd-1)/bd) * bb;

	if (mips->type == PTI_3D)
	{
		qbyte *data2d = mips->mip[0].data, *data3d;
		mips->mip[0].data = NULL;
		/*our 2d input image is interlaced as y0z0,y0z1,y1z0,y1z1
		  however, hardware uses the more logical y0z0,y1z0,y0z1,y1z1 ordering (xis ordered properly already)*/
		if (mips->mip[0].height*mips->mip[0].height == mips->mip[0].width && mips->mip[0].depth == 1 && (bb==4&&bw==1&&bh==1&&bd==1))
		{
			int d, r;
			int size = mips->mip[0].height;
			mips->mip[0].width = size;
			mips->mip[0].height = size;
			mips->mip[0].depth = size;
			mips->mip[0].data = data3d = BZ_Malloc(size*size*size);
			for (d = 0; d < size; d++)
				for (r = 0; r < size; r++)
					memcpy(data3d + (r + d*size) * size, data2d + (r*size + d) * size, size*4);
			mips->mip[0].datasize = size*size*size*4;
		}
		if (freedata)
			BZ_Free(data2d);
		if (!mips->mip[0].data)
			return false;
	}

	if (flags & IF_PREMULTIPLYALPHA)
		Image_Premultiply(mips);

	mips->mip[0].needfree = freedata;
	return true;
}

//for ???X8 formats, replaces the alpha channel with the colours from an additional greyscale _alpha image (typically a jpeg)
//this stuff exists only for compat with DP. Note that DP reads only the blue channel.
//writes to rgbdata+format on success
void Image_ReadExternalAlpha(qbyte *rgbadata, size_t imgwidth, size_t imgheight, const char *fname, uploadfmt_t *format)
{
#ifdef HAVE_CLIENT
	unsigned int alpha_width, alpha_height, p;
	char aname[MAX_QPATH];
	qbyte *alphadata, *srcchan;
	char *alph;
	size_t alphsize;
	char ext[8];
	uploadfmt_t alphaformat;
	int srcstride;

	switch(*format)
	{
	default:
		break;
	case PTI_BGRX8_SRGB:
	case PTI_BGRX8:
	case PTI_RGBX8_SRGB:
	case PTI_RGBX8:
	case PTI_LLLX8:
		COM_StripExtension(fname, aname, sizeof(aname));
		COM_FileExtension(fname, ext, sizeof(ext));
		Q_strncatz(aname, "_alpha.", sizeof(aname));
		Q_strncatz(aname, ext, sizeof(aname));
		if (!strchr(aname, ':') && (alph = FS_LoadMallocFile (aname, &alphsize)))
		{
			if ((alphadata = ReadRawImageFile(alph, alphsize, &alpha_width, &alpha_height, &alphaformat, false, aname)))
			{
				if (alpha_width == imgwidth && alpha_height == imgheight)
				{
					rgbadata += 3;	//might as well do this in advance
					switch(alphaformat)
					{
					case PTI_RGBA8:
					case PTI_RGBX8:
					case PTI_LLLA8:
					case PTI_LLLX8:
					case PTI_RGBA8_SRGB:
					case PTI_RGBX8_SRGB:
						srcstride = 4;
						srcchan = alphadata+2;
						break;
					case PTI_RGB8:
					case PTI_RGB8_SRGB:
						srcstride = 4;
						srcchan = alphadata+2;
						break;
					case PTI_BGRA8:
					case PTI_BGRX8:
					case PTI_BGRA8_SRGB:
					case PTI_BGRX8_SRGB:
						srcstride = 4;
						srcchan = alphadata+0;
						break;
					case PTI_BGR8:
					case PTI_BGR8_SRGB:
						srcstride = 3;
						srcchan = alphadata+0;
						break;
					case PTI_R8:
					case PTI_L8:
					case PTI_L8_SRGB:
						srcstride = 1;
						srcchan = alphadata+0;
						break;
					case PTI_L8A8:
					case PTI_L8A8_SRGB:
					case PTI_RG8:
						srcstride = 2;
						srcchan = alphadata+0;
						break;
					default:
						Con_Printf("%s: Unable to read luminance (\"%s\" has unsupported pixelformat)\n", fname, aname);
						srcchan = "\xff";
						srcstride = 0;
						break;
					}

					for (p = 0; p < alpha_width*alpha_height; p++)
						rgbadata[(p<<2)] = srcchan[p*srcstride];
					switch(*format)
					{
					case PTI_LLLX8:		*format = PTI_RGBA8;		break;
					case PTI_RGBX8:		*format = PTI_RGBA8;		break;
					case PTI_BGRX8:		*format = PTI_BGRA8;		break;
					case PTI_RGBX8_SRGB:*format = PTI_RGBA8_SRGB;	break;
					case PTI_BGRX8_SRGB:*format = PTI_BGRA8_SRGB;	break;
					default:										break;
					}
				}
				BZ_Free(alphadata);
			}
			BZ_Free(alph);
		}
		break;
	}
#endif
}

//always frees filedata, even on failure.
//also frees the textures fallback data, but only on success
struct pendingtextureinfo *Image_LoadMipsFromMemory(int flags, const char *iname, const char *fname, qbyte *filedata, int filesize)
{
	uploadfmt_t format;
	qbyte *rgbadata;

	int imgwidth, imgheight;
	size_t l;

	struct pendingtextureinfo *mips = NULL;

	//these formats have special handling, because they cannot be implemented via Read32BitImageFile - they don't result in rgba images.
#ifdef IMAGEFMT_KTX
	if (!mips)
		mips = Image_ReadKTXFile(flags, fname, filedata, filesize);
#endif
#ifdef IMAGEFMT_PKM
	if (!mips)
		mips = Image_ReadPKMFile(flags, fname, filedata, filesize);
#endif
#ifdef IMAGEFMT_DDS
	if (!mips)
		mips = Image_ReadDDSFile(flags, fname, filedata, filesize);
#endif
#ifdef IMAGEFMT_BLP
	if (!mips && filedata[0] == 'B' && filedata[1] == 'L' && filedata[2] == 'P' && filedata[3] == '2') 
		mips = Image_ReadBLPFile(flags, fname, filedata, filesize);
#endif
	for (l = 0; !mips && l < imageloader_count; l++)
		mips = imageloader[l].funcs->ReadImageFile(flags, fname, filedata, filesize);
#ifdef IMAGEFMT_ASTC
	if (!mips && filesize>= 16 && filedata[0] == 0x13 && filedata[1] == 0xab && filedata[2] == 0xa1 && filedata[3] == 0x5c)
		mips = Image_ReadASTCFile(flags, fname, filedata, filesize);
#endif

#ifdef STBI_ONLY_GIF
	if (!mips)
	{
		int *delays = NULL;
		int w, h, d;
		int comp;
		stbi_uc *data = stbi_load_gif_from_memory(filedata, filesize, &delays, &w, &h, &d, &comp, 4); //force 4, we're not really ready for other types of 2d arrays.
		if (data)
		{
			mips = Z_Malloc(sizeof(*mips));
			mips->mipcount = 1;	//this format doesn't support mipmaps. so there's only one level.
			mips->type = PTI_2D_ARRAY;	//2d arrays are basically just a 3d texture with weird mips (meaning they can load with gl's glTexStorage3D
			mips->extrafree = delays;
			switch(comp)
			{
			case 1:		mips->encoding = PTI_L8;		break;
			case 2:		mips->encoding = PTI_L8A8;		break;
			case 3:		mips->encoding = PTI_RGB8;		break;
			case 4:		mips->encoding = PTI_RGBA8;		break;
			default:	mips->encoding = PTI_INVALID;	break;
			}
			mips->mip[0].data = data;
			mips->mip[0].datasize = comp*w*h*d;
			mips->mip[0].width = w;
			mips->mip[0].height = h;
			mips->mip[0].depth = d;
			mips->mip[0].needfree = true;
		}
	}
#endif

	//the above formats are assumed to have consumed filedata somehow (probably storing into mips->extradata)
	if (mips)
	{
		unsigned int picmip = min(Image_GetPicMip(flags), mips->mipcount-1), i;
		if (picmip < mips->mipcount)
		{
			for (i = 0; i < picmip; i++)
				if (mips->mip[i].needfree)
					BZ_Free(mips->mip[i].data);
			mips->mipcount -= i;
			memmove(mips->mip, mips->mip+i, sizeof(*mips->mip)*mips->mipcount);
		}

		Image_ChangeFormatFlags(mips, flags, TF_INVALID, fname);
		return mips;
	}

	if ((rgbadata = ReadRawImageFile(filedata, filesize, &imgwidth, &imgheight, &format, false, fname)))
	{
#ifdef HAVE_CLIENT
		extern cvar_t vid_hardwaregamma;
		if (!(flags&IF_NOGAMMA) && !vid_hardwaregamma.value)
			BoostGamma(rgbadata, imgwidth, imgheight, format);
#endif

		switch(format)
		{
		default:
			break;
		case PTI_RGBA32F:
		case PTI_RGBA16F:
		case PTI_L8A8:
		case PTI_RGBA8:
		case PTI_RGBA4444:
		case PTI_ARGB4444:
		case PTI_RGBA5551:
		case PTI_ARGB1555:
			flags &= ~IF_NOALPHA;
			break;
		case PTI_BGRX8_SRGB:
		case PTI_BGRX8:
		case PTI_RGBX8_SRGB:
		case PTI_RGBX8:
		case PTI_LLLX8:
			if (!(flags & IF_NOALPHA))
				Image_ReadExternalAlpha(rgbadata, imgwidth, imgheight, fname, &format);
			break;
		}

		mips = Z_Malloc(sizeof(*mips));
		mips->type = (flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT;
		if (mips->type == PTI_ANY)
			mips->type = PTI_2D;	//d
		if (Image_GenMip0(mips, flags, rgbadata, NULL, imgwidth, imgheight, 1, format, true))
		{
			Image_GenerateMips(mips, flags);
			Image_ChangeFormatFlags(mips, flags, format, fname);
			BZ_Free(filedata);
			return mips;
		}
		Z_Free(mips);
	}
#ifdef FTE_TARGET_WEB
	else if (1)
	{
		struct pendingtextureinfo *mips;
		mips = Z_Malloc(sizeof(*mips));
		mips->type = (flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT;
		mips->mipcount = 1;
		mips->encoding = PTI_WHOLEFILE;
		mips->extrafree = NULL;
		//evil ensues:
		if (filesize >= 32 && !strncmp(filedata, "\x89PNG", 4) && !strncmp(filedata+12, "IHDR", 4))
		{	//need to do this to get png sizes working right for the quake rerelease's content
			mips->mip[0].width  = (filedata[0x13]<<0)|(filedata[0x12]<<8)|(filedata[0x11]<<16)|(filedata[0x10]<<24);
			mips->mip[0].height = (filedata[0x17]<<0)|(filedata[0x16]<<8)|(filedata[0x15]<<16)|(filedata[0x14]<<24);
		}
		else
		{
			mips->mip[0].width  = 1;
			mips->mip[0].height = 1;
		}
		mips->mip[0].depth = 1;
		mips->mip[0].data = filedata;
		mips->mip[0].datasize = filesize;
		mips->mip[0].needfree = true;
		//width+height are not yet known. bah.
		return mips;
	}
#endif
	else
		Con_TPrintf("Unable to load file %s (format unsupported)\n", fname);

	BZ_Free(filedata);
	return NULL;
}

void *Image_FlipImage(const void *inbuffer, void *outbuffer, int *inoutwidth, int *inoutheight, int pixelbytes, qboolean flipx, qboolean flipy, qboolean flipd)
{
	int x, y, b;
	qbyte *outb;
	const qbyte *inb, *inr;
	int inwidth = *inoutwidth;
	int inheight = *inoutheight;
	int rowstride = inwidth;
	int colstride = 1;

	//simply return if no operation
	if (!flipx && !flipy && !flipd)
		memcpy(outbuffer, inbuffer, inwidth*inheight*pixelbytes);
	else
	{
		inr = inbuffer;
		outb = outbuffer;

		if (flipy)
		{
			inr += (inwidth*inheight-inwidth)*pixelbytes;//start on the bottom row
			rowstride *= -1;	//and we need to move up instead
		}
		if (flipx)
		{
			colstride *= -1;	//move backwards
			inr += (inwidth-1)*pixelbytes;	//start at the end of the row
		}
		if (flipd)
		{
			//switch the dimensions
			int tmp = inwidth;
			inwidth = inheight;
			inheight = tmp;
			//make sure the caller gets the new dimensions
			*inoutwidth = inwidth;
			*inoutheight = inheight;
			//switch the strides
			tmp = colstride;
			colstride = rowstride;
			rowstride = tmp;
		}

		colstride *= pixelbytes;
		rowstride *= pixelbytes;

		//rows->rows, columns->columns
		for (y = 0; y < inheight; y++)
		{
			inb = inr;	//reset the input after each row, so we have truely independant row+column strides
			inr += rowstride;
			for (x = 0; x < inwidth; x++)
			{
				for (b = 0; b < pixelbytes; b++)
					*outb++ = inb[b];
				inb += colstride;
			}
		}
	}
	return outbuffer;
}
#ifdef HAVE_CLIENT
static int tex_extensions_count;
#define tex_extensions_max 15
static struct
{
	char name[6];
} tex_extensions[tex_extensions_max];
static void QDECL R_ImageExtensions_Callback(struct cvar_s *var, char *oldvalue)
{
	char *v = var->string;
	tex_extensions_count = 0;

	while (tex_extensions_count < tex_extensions_max)
	{
		v = COM_Parse(v);
		if (!v)
			break;
		Q_snprintfz(tex_extensions[tex_extensions_count].name, sizeof(tex_extensions[tex_extensions_count].name), ".%s", com_token);
		tex_extensions_count++;
	}

	if (tex_extensions_count < tex_extensions_max)
	{
		Q_snprintfz(tex_extensions[tex_extensions_count].name, sizeof(tex_extensions[tex_extensions_count].name), "");
		tex_extensions_count++;
	}
}
static struct pendingtextureinfo *Image_LoadCubemapTextureData(const char *nicename, char *subpath, unsigned int texflags)
{
	static struct
	{
		const char *suffix;
		qboolean flipx, flipy, flipd;
		int pad;
	} cmscheme[][6] =
	{
		{
			{"rt", false,  false, true},
			{"lf", true, true,  true},
			{"bk", false, true, false},
			{"ft", true,  false,  false},
			{"up", false,  false, true},
			{"dn", false,  false, true}
		},

		{
			{"px", false, false, false},
			{"nx", false, false, false},
			{"py", false, false, false},
			{"ny", false, false, false},
			{"pz", false, false, false},
			{"nz", false, false, false}
		},

		{
			{"posx", false, false, false},
			{"negx", false, false, false},
			{"posy", false, false, false},
			{"negy", false, false, false},
			{"posz", false, false, false},
			{"negz", false, false, false}
		}
	};
	int i, j, e;
	struct pendingtextureinfo *mips = NULL;
	char fname[MAX_QPATH];
	size_t filesize;
	int width, height;
	uploadfmt_t format;
	char *nextprefix, *prefixend;
	size_t prefixlen;

	for (i = 0; i < 6; i++)
	{
		prefixlen = 0;
		nextprefix = subpath;
		for(;;)
		{
			for (e = (texflags & IF_EXACTEXTENSION)?tex_extensions_count-1:0; e < tex_extensions_count; e++)
			{
				//try and open one
				qbyte *buf = NULL, *data;
				filesize = 0;

				for (j = 0; j < countof(cmscheme); j++)
				{
					Q_snprintfz(fname+prefixlen, sizeof(fname)-prefixlen, "%s_%s%s", nicename, cmscheme[j][i].suffix, tex_extensions[e].name);
					buf = FS_LoadMallocFile(fname, &filesize);
					if (buf)
						break;

					Q_snprintfz(fname+prefixlen, sizeof(fname)-prefixlen, "%s%s%s", nicename, cmscheme[j][i].suffix, tex_extensions[e].name);
					buf = FS_LoadMallocFile(fname, &filesize);
					if (buf)
						break;
				}

				//now read it
				if (buf)
				{
					qboolean needsflipping = cmscheme[j][i].flipx||cmscheme[j][i].flipy||cmscheme[j][i].flipd;
					if ((data = ReadRawImageFile(buf, filesize, &width, &height, &format, true, fname)))
					{
						extern cvar_t vid_hardwaregamma;
						int bb,bw,bh, bd;
						Image_BlockSizeForEncoding(format, &bb, &bw, &bh, &bd);
						if (needsflipping && (bw!=1 || bh!=1 || bd!=1))
							Con_Printf(CON_WARNING"%s: %s requires flipping, which is unsupported with pixel format %s\n", nicename, fname, Image_FormatName(format));	/*can't do it*/
						else if (width == height && (!mips || width == mips->mip[0].width))	//cubemaps must be square and all the same size (npot is fine though)
						{	//(skies have a fallback for invalid sizes, but it'll run a bit slower)

							if (!mips)
							{
								mips = Z_Malloc(sizeof(*mips));
								mips->type = PTI_CUBE;
								mips->mipcount = 1;
								mips->encoding = format;
								mips->extrafree = NULL;
								mips->mip[0].datasize = width*height*bb*6;
								mips->mip[0].data = BZ_Malloc(mips->mip[0].datasize);
								mips->mip[0].width = width;
								mips->mip[0].height = height;
								mips->mip[0].depth = 6;
								mips->mip[0].needfree = true;
							}

							if (!(texflags&IF_NOGAMMA) && !vid_hardwaregamma.value)
								BoostGamma(data, width, height, format);
							Image_FlipImage(data, (qbyte*)mips->mip[0].data + i*width*height*bb, &width, &height, bb, cmscheme[j][i].flipx, cmscheme[j][i].flipy, cmscheme[j][i].flipd);
							BZ_Free(data);

							BZ_Free(buf);
							goto nextface;
						}
						else
						{
							if (mips)
								Con_Printf(CON_WARNING"%s: %s has inconsistent dimensions (%i*%i, must be %i*%i)\n", nicename, fname, width, height, mips->mip[0].width, mips->mip[0].height);
							else
								Con_Printf(CON_WARNING"%s: %s has inconsistent dimensions (%i*%i, must be square)\n", nicename, fname, width, height);
						}
						BZ_Free(data);
					}
					BZ_Free(buf);
				}
			}

			//get ready for the next prefix...
			if (!nextprefix || !*nextprefix)
				break;	//no more...
			prefixend = strchr(nextprefix, ':');
			if (!prefixend)
				prefixend = nextprefix+strlen(nextprefix);

			prefixlen = prefixend-nextprefix;
			if (prefixlen >= sizeof(fname)-2)
				prefixlen = sizeof(fname)-2;
			memcpy(fname, nextprefix, prefixlen);
			fname[prefixlen++] = '/';

			if (*prefixend)
				prefixend++;
			nextprefix = prefixend;
		}

		while(i>0)
			BZ_Free(mips->mip[i--].data);
		Z_Free(mips);
		return NULL;
nextface:;
	}
	return mips;
}

//loads from a single mip. takes ownership of the data.
static qboolean Image_LoadRawTexture(texid_t tex, unsigned int flags, void *rawdata, void *palettedata, int imgwidth, int imgheight, uploadfmt_t fmt)
{
	struct pendingtextureinfo *mips;
	mips = Z_Malloc(sizeof(*mips));
	mips->type = (flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT;

	if (!Image_GenMip0(mips, flags, rawdata, palettedata, imgwidth, imgheight, 1, fmt, true))
	{
		Z_Free(mips);
		if (flags & IF_NOWORKER)
			Image_LoadTexture_Failed(tex, NULL, 0, 0);
		else
			COM_AddWork(WG_MAIN, Image_LoadTexture_Failed, tex, NULL, 0, 0);
		return false;
	}
	fmt &= ~PTI_FULLMIPCHAIN;
	Image_GenerateMips(mips, flags);
	Image_ChangeFormatFlags(mips, flags, fmt, tex->ident);

	Image_FixupImageSize(tex, imgwidth, imgheight, mips->mip[0].depth);
	if (flags & IF_NOWORKER)
		Image_LoadTextureMips(tex, mips, 0, 0);
	else
		COM_AddWork(WG_MAIN, Image_LoadTextureMips, tex, mips, 0, 0);
	return true;
}

//always frees filedata, even on failure.
//also frees the textures fallback data, but only on success
qboolean Image_LoadTextureFromMemory(texid_t tex, int flags, const char *iname, const char *fname, qbyte *filedata, int filesize)
{
	struct pendingtextureinfo *mips = Image_LoadMipsFromMemory(flags, iname, fname, filedata, filesize);
	if (mips)
	{
		BZ_Free(tex->fallbackdata);
		tex->fallbackdata = NULL;

		Image_FixupImageSize(tex, mips->mip[0].width, mips->mip[0].height, mips->mip[0].depth);
		if ((flags & IF_NOWORKER) || Sys_IsMainThread())
			Image_LoadTextureMips(tex, mips, 0, 0);
		else
			COM_AddWork(WG_MAIN, Image_LoadTextureMips, tex, mips, 0, 0);
		return true;
	}
	return false;
}

static struct
{
	char *path;
	int args;

	int enabled;
} tex_path[] =
{
	/*if three args, first is the subpath*/
	/*the last two args are texturename then extension*/
	{"%s%s",			2, 1},	/*directly named texture*/
	{"textures/%s/%s%s",3, 1},	/*fuhquake compatibility*/
	{"%s/%s%s",			3, 1},	/*fuhquake compatibility*/
	{"textures/%s%s",	2, 1},	/*directly named texture with textures/ prefix*/
#ifdef HAVE_LEGACY
	{"override/%s%s",	2, 1}	/*tenebrae compatibility*/
#endif
};
qboolean Image_LocateHighResTexture(image_t *tex, flocation_t *bestloc, char *bestname, size_t bestnamesize, unsigned int *bestflags)
{
	char fname[MAX_QPATH], nicename[MAX_QPATH];
	int i, e;
	char *altname;
	char *nextalt;
	qboolean exactext = !!(tex->flags & IF_EXACTEXTENSION);
	qboolean exactpath = false;

	int locflags = FSLF_DEPTH_INEXPLICIT|FSLF_DEEPONFAILURE;
	int bestdepth = 0x7fffffff, depth;
	int firstex = (tex->flags & IF_EXACTEXTENSION)?tex_extensions_count-1:0;

	flocation_t loc;
	
	if (strncmp(tex->ident, "http:", 5) && strncmp(tex->ident, "https:", 6))
	for(altname = tex->ident;altname;altname = nextalt)
	{
		if (!strncmp(altname, "file:", 5))
		{
			nextalt = strchr(altname+5, ':');
			exactpath = true;
		}
		else
			nextalt = strchr(altname, ':');
		if (nextalt)
		{
			nextalt++;
			if (nextalt-altname >= sizeof(fname))
				continue;	//too long...
			memcpy(fname, altname, nextalt-altname-1);
			fname[nextalt-altname-1] = 0;
			altname = fname;
		}

		//see if we recognise the extension, and only strip it if we do.
		if (exactext)
			e = tex_extensions_count;
		else
		{
			COM_FileExtension(altname, nicename, sizeof(nicename));
			e = 0;
			if (Q_strcasecmp(nicename, "lmp") && Q_strcasecmp(nicename, "wal"))
				for (; e < tex_extensions_count; e++)
				{
					if (!Q_strcasecmp(nicename, (*tex_extensions[e].name=='.')?tex_extensions[e].name+1:tex_extensions[e].name))
						break;
				}
		}

		//strip it and try replacements if we do, otherwise assume that we're meant to be loading progs/foo.mdl_0.tga or whatever
		if (e == tex_extensions_count || exactext)
		{
			exactext = true;
			Q_strncpyz(nicename, altname, sizeof(nicename));
		}
		else
			COM_StripExtension(altname, nicename, sizeof(nicename));

		if (!tex->fallbackdata || (gl_load24bit.ival && !(tex->flags & IF_NOREPLACE)))
		{
#ifdef IMAGEFMT_DDS
			if (!exactpath)
			{
				Q_snprintfz(fname, sizeof(fname), "dds/%s.dds", nicename);
				depth = FS_FLocateFile(fname, locflags, &loc);
				if (depth < bestdepth)
				{
					Q_strncpyz(bestname, fname, bestnamesize);
					bestdepth = depth;
					*bestloc = loc;
					*bestflags = 0;
				}
			}
#endif

			if (exactpath || strchr(nicename, '/') || strchr(nicename, '\\'))	//never look in a root dir for the pic
				i = 0;
			else
				i = 1;

			for (; i < sizeof(tex_path)/sizeof(tex_path[0]); i++)
			{
				if (!tex_path[i].enabled)
					continue;
				if (exactpath && i)
					break;
				if (tex_path[i].args >= 3)
				{	//this is a path that needs subpaths
					char subpath[MAX_QPATH];
					char basename[MAX_QPATH];
					char *s, *n;
					if (!tex->subpath || !*nicename)
						continue;

					s = COM_SkipPath(nicename);
					if (!*s)
						continue;
					n = basename;
					while (*s && (*s != '.'||exactext) && n < basename+sizeof(basename)-5)
						*n++ = *s++;
					s = strchr(s, '_');
					if (s)
					{
						while (*s && n < basename+sizeof(basename)-5)
							*n++ = *s++;
					}
					*n = 0;

					for(s = tex->subpath; s; s = n)
					{
						//subpath a:b:c tries multiple possible sub paths, for compatibility
						n = strchr(s, ':');
						if (n)
						{
							if (n-s >= sizeof(subpath))
								*subpath = 0;
							else
							{
								memcpy(subpath, s, n-s);
								subpath[n-s] = 0;
							}
							n++;
						}
						else
							Q_strncpyz(subpath, s, sizeof(subpath));
						for (e = firstex; e < tex_extensions_count; e++)
						{
							if (tex->flags & IF_NOPCX)
								if (!Q_strcasecmp(tex_extensions[e].name, ".pcx"))
									continue;
							Q_snprintfz(fname, sizeof(fname), tex_path[i].path, subpath, basename, tex_extensions[e].name);
							depth = FS_FLocateFile(fname, locflags, &loc);
							if (depth < bestdepth)
							{
								Q_strncpyz(bestname, fname, bestnamesize);
								bestdepth = depth;
								*bestloc = loc;
								*bestflags = 0;
							}
						}
					}
				}
				else
				{
					for (e = firstex; e < tex_extensions_count; e++)
					{
						if (tex->flags & IF_NOPCX)
							if (!Q_strcasecmp(tex_extensions[e].name, ".pcx"))
								continue;
						Q_snprintfz(fname, sizeof(fname), tex_path[i].path, nicename, tex_extensions[e].name);
						depth = FS_FLocateFile(fname, locflags, &loc);
						if (depth < bestdepth)
						{
							Q_strncpyz(bestname, fname, bestnamesize);
							bestdepth = depth;
							*bestloc = loc;
							*bestflags = 0;
						}
					}
				}

				//support expansion of _bump textures to _norm textures.
				if (tex->flags & IF_TRYBUMP)
				{
					if (tex_path[i].args >= 3)
					{
						/*no legacy compat needed, hopefully*/
					}
					else
					{
						char bumpname[MAX_QPATH], *b;
						const char *n;
						b = bumpname;
						n = nicename;
						while(*n)
						{
							if (*n == '_' && !strcmp(n, "_norm"))
							{
								strcpy(b, "_bump");
								b += 5;
								n += 5;
								break;
							}
							*b++ = *n++;
						}
						if (*n)	//no _norm, give up with that
							continue;
						*b = 0;
						for (e = firstex; e < tex_extensions_count; e++)
						{
							if (!Q_strcasecmp(tex_extensions[e].name, ".tga"))
							{
								Q_snprintfz(fname, sizeof(fname), tex_path[i].path, bumpname, tex_extensions[e].name);
								depth = FS_FLocateFile(fname, locflags, &loc);
								if (depth < bestdepth)
								{
									Q_strncpyz(bestname, fname, bestnamesize);
									bestdepth = depth;
									*bestloc = loc;
									*bestflags = IF_TRYBUMP;
								}
							}
						}
					}
				}
			}


			/*still failed? attempt to load quake lmp files, which have no real format id (hence why they're not above)*/
			Q_strncpyz(fname, nicename, sizeof(fname));
			COM_DefaultExtension(fname, ".lmp", sizeof(fname));
			if (!(tex->flags & IF_NOPCX))
			{
				depth = FS_FLocateFile(fname, locflags, &loc);
				if (depth < bestdepth)
				{
					Q_strncpyz(bestname, fname, bestnamesize);
					bestdepth = depth;
					*bestloc = loc;
					*bestflags = 0;
				}
			}
		}
	}

	return bestdepth != 0x7fffffff;
}

static void Image_LoadHiResTextureWorker(void *ctx, void *data, size_t a, size_t b)
{
	image_t *tex = ctx;
	char fname[MAX_QPATH];
	char fname2[MAX_QPATH];
	char *altname;
	char *nextalt;

	flocation_t loc;
	unsigned int locflags = 0;

	vfsfile_t *f;
	size_t fsize, l;
	char *buf;

	int i, j;
	int imgwidth;
	int imgheight;
	uploadfmt_t format;
	int ttype = (tex->flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT;

	if (ttype == PTI_CUBE)
	{	//cubemaps require special handling because they are (normally) 6 files instead of 1.
		//the exception is single-file dds/ktx/etc cubemaps.
		//FIXME: handle via Image_LocateHighResTexture.
		for(altname = tex->ident;altname;altname = nextalt)
		{
			struct pendingtextureinfo *mips = NULL;
			static const char *cubeexts[] =
			{
				""
#ifdef IMAGEFMT_KTX
				, ".ktx"
#endif
#ifdef IMAGEFMT_DDS
				, ".dds"
#endif
			};

			nextalt = strchr(altname, ':');
			if (nextalt)
			{
				nextalt++;
				if (nextalt-altname >= sizeof(fname))
					continue;	//too long...
				memcpy(fname, altname, nextalt-altname-1);
				fname[nextalt-altname-1] = 0;
				altname = fname;
			}

			for (i = 0; i < countof(tex_path) && !mips; i++)
			{
				if (!tex_path[i].enabled)
					continue;
				buf = NULL;
				fsize = 0;
				if (tex_path[i].args == 3 && tex->subpath)
				{
					char subpath[MAX_QPATH];
					char *n = tex->subpath, *e;
					while (*n)
					{
						e = strchr(n, ':');
						if (!e)
							e = n+strlen(n);
						Q_strncpyz(subpath, n, min(sizeof(subpath), (e-n)+1));
						n = e;
						while(*n == ':')
							n++;
						for (j = 0; !buf && j < countof(cubeexts); j++)
						{
							Q_snprintfz(fname2, sizeof(fname2), tex_path[i].path, subpath, altname, cubeexts[j]);
							buf = FS_LoadMallocFile(fname2, &fsize);
						}
					}
				}
				else if (tex_path[i].args == 2)
				{
					for (j = 0; !buf && j < countof(cubeexts); j++)
					{
						Q_snprintfz(fname2, sizeof(fname2), tex_path[i].path, altname, cubeexts[j]);
						buf = FS_LoadMallocFile(fname2, &fsize);
					}
				}
				if (buf)
				{
#ifdef IMAGEFMT_KTX
					if (!mips)
						mips = Image_ReadKTXFile(tex->flags, altname, buf, fsize);
#endif
#ifdef IMAGEFMT_DDS
					if (!mips)
						mips = Image_ReadDDSFile(tex->flags, altname, buf, fsize);
#endif
					for (l = 0; !mips && l < imageloader_count; l++)
					{
						if (!imageloader[l].funcs->canloadcubemaps)
							continue;
						mips = imageloader[l].funcs->ReadImageFile(tex->flags, altname, buf, fsize);
					}
					if (!mips)
						BZ_Free(buf);
				}
			}

			if (!mips)	//try to load multiple images
				mips = Image_LoadCubemapTextureData(altname, tex->subpath, tex->flags);

			if (mips)
			{
				Image_FixupImageSize(tex, mips->mip[0].width, mips->mip[0].height, mips->mip[0].depth);
				if (tex->flags & IF_NOWORKER)
					Image_LoadTextureMips(tex, mips, 0, 0);
				else
					COM_AddWork(WG_MAIN, Image_LoadTextureMips, tex, mips, 0, 0);
				return;
			}
		}
		if (tex->flags & IF_NOWORKER)
			Image_LoadTexture_Failed(tex, NULL, 0, 0);
		else
			COM_AddWork(WG_MAIN, Image_LoadTexture_Failed, tex, NULL, 0, 0);
		return;
	}

	

	if (Image_LocateHighResTexture(tex, &loc, fname, sizeof(fname), &locflags))
	{
		f = FS_OpenReadLocation(fname, &loc);
		if (f)
		{
			fsize = VFS_GETLEN(f);
			buf = BZ_Malloc(fsize);
			if (buf)
			{
				VFS_READ(f, buf, fsize);
				VFS_CLOSE(f);

#ifdef IMAGEFMT_TGA
				if (locflags & IF_TRYBUMP)
				{	//it was supposed to be a heightmap image (that we need to convert to normalmap)
					qbyte *d;
					int w, h;
					uploadfmt_t fmt;
					if ((d = ReadTargaFile(buf, fsize, &w, &h, &fmt, true, PTI_L8)))	//Only load a greyscale image.
					{
						BZ_Free(buf);
						if (Image_LoadRawTexture(tex, tex->flags, d, NULL, w, h, TF_HEIGHT8))
						{
							BZ_Free(tex->fallbackdata);
							tex->fallbackdata = NULL;	
							return;
						}
					}
					else
						Con_Printf(CON_WARNING "%s: bumpmaps must be greyscale tga.\n", fname);
					//guess not, fall back to normalmaps
				}
#endif

				if (Image_LoadTextureFromMemory(tex, tex->flags, tex->ident, fname, buf, fsize))
				{
					BZ_Free(tex->fallbackdata);
					tex->fallbackdata = NULL;
					return;
				}
			}
			else
				VFS_CLOSE(f);
		}
	}

	if (!tex->fallbackdata)
	{
		//now look in wad files and swap over the fallback. (halflife compatability)
		COM_StripExtension(tex->ident, fname, sizeof(fname));
		buf = W_GetTexture(fname, &imgwidth, &imgheight, &format);
		if (buf)
		{
			BZ_Free(tex->fallbackdata);
			tex->fallbackdata = buf;
			tex->fallbackfmt = format;
			tex->fallbackwidth = imgwidth;
			tex->fallbackheight = imgheight;
		}
	}

	if (tex->fallbackdata)
	{
		if (tex->fallbackfmt == TF_INVALID)
		{
			void *data = tex->fallbackdata;
			tex->fallbackdata = NULL;
			if (Image_LoadTextureFromMemory(tex, tex->flags, tex->ident, fname, data, tex->fallbackwidth))
				return;
		}
		else if (Image_LoadRawTexture(tex, tex->flags, tex->fallbackdata, (char*)tex->fallbackdata+(tex->fallbackwidth*tex->fallbackheight), tex->fallbackwidth, tex->fallbackheight, tex->fallbackfmt))
		{
			tex->fallbackdata = NULL;
			return;
		}
		tex->fallbackdata = NULL;	
	}

//	Sys_Printf("Texture %s failed\n", nicename);
	//signal the main thread to set the final status instead of just setting it to avoid deadlock (it might already be waiting for it).
	if (tex->flags & IF_NOWORKER)
		Image_LoadTexture_Failed(tex, NULL, 0, 0);
	else
		COM_AddWork(WG_MAIN, Image_LoadTexture_Failed, tex, NULL, 0, 0);
}

//returns the pointer if its valid, otherwise null
//this is to pass pointers via the console
image_t *Image_TextureIsValid(qintptr_t address)
{
	image_t *img;
	for (img = imagelist; img; img = img->next)
	{
		if (img == (image_t*)address)
			break;
	}
	return img;
}

//find an existing texture
image_t *Image_FindTexture(const char *identifier, const char *subdir, unsigned int flags)
{
	image_t *tex;
	if (!subdir)
		subdir = "";
	tex = Hash_GetInsensitive(&imagetable, identifier);
	while(tex)
	{
		if (!((tex->flags ^ flags) & (IF_CLAMP|IF_PALETTIZE|IF_PREMULTIPLYALPHA)))
		{
			if (r_ignoremapprefixes.ival || !Q_strcasecmp(subdir, tex->subpath?tex->subpath:"") || ((flags|tex->flags) & IF_INEXACT))
			{
				tex->regsequence = r_regsequence;
				return tex;
			}
		}
		tex = Hash_GetNextInsensitive(&imagetable, identifier, tex);
	}
	return NULL;
}
//create a texture, with dupes. you'll need to load something into it too.
static image_t *Image_CreateTexture_Internal (const char *identifier, const char *subdir, unsigned int flags)
{
	image_t *tex;
	bucket_t *buck;

	tex = Z_Malloc(sizeof(*tex) + sizeof(bucket_t) + strlen(identifier)+1 + (subdir?strlen(subdir)+1:0));
	buck = (bucket_t*)(tex+1);
	tex->ident = (char*)(buck+1);
	strcpy(tex->ident, identifier);
#ifdef _DEBUG
	Q_strncpyz(tex->dbgident, identifier, sizeof(tex->dbgident));
#endif
	if (subdir && *subdir)
	{
		tex->subpath = tex->ident + strlen(identifier)+1;
		strcpy(tex->subpath, subdir);
	}

	tex->next = imagelist;
	imagelist = tex;

	if ((vid.flags & VID_SRGBAWARE) && !(flags & IF_NOSRGB))
		tex->flags = flags | IF_SRGB;	//guess...
	else
		tex->flags = flags;
	tex->width = 0;
	tex->height = 0;
	tex->regsequence = r_regsequence;
	tex->status = TEX_NOTLOADED;
	tex->fallbackdata = NULL;
	tex->fallbackwidth = 0;
	tex->fallbackheight = 0;
	tex->fallbackfmt = TF_INVALID;
	if (*tex->ident)
		Hash_AddInsensitive(&imagetable, tex->ident, tex, buck);
	return tex;
}

image_t *Image_CreateTexture (const char *identifier, const char *subdir, unsigned int flags)
{
	image_t *image;
#ifdef LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
#endif
	image = Image_CreateTexture_Internal(identifier, subdir, flags);
#ifdef LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	return image;
}

#ifdef WEBCLIENT
//called on main thread. oh well.
static void Image_Downloaded(struct dl_download *dl)
{
	qboolean success = false;
	image_t *tex = dl->user_ctx;
	image_t *p;

	//make sure the renderer wasn't restarted mid-download
	for (p = imagelist; p; p = p->next)
		if (p == tex)
			break;
	if (p)
	{
		if (dl->status == DL_FINISHED)
		{
			size_t fsize = VFS_GETLEN(dl->file);
			char *buf = BZ_Malloc(fsize);
			if (VFS_READ(dl->file, buf, fsize) == fsize)
				if (Image_LoadTextureFromMemory(tex, tex->flags, tex->ident, dl->url, buf, fsize))
					success = true;
		}
		if (!success)
			Image_LoadTexture_Failed(tex, NULL, 0, 0);
	}
}
#endif

//find a texture. will try to load it from disk, using the fallback if it would fail.
image_t *QDECL Image_GetTexture(const char *identifier, const char *subpath, unsigned int flags, void *fallbackdata, void *fallbackpalette, int fallbackwidth, int fallbackheight, uploadfmt_t fallbackfmt)
{
	image_t *tex;

	qboolean dontposttoworker = (flags & (IF_NOWORKER | IF_LOADNOW));
	qboolean lowpri = (flags & IF_LOWPRIORITY);
	qboolean highpri = (flags & IF_HIGHPRIORITY);
	flags &= ~(IF_LOADNOW | IF_LOWPRIORITY | IF_HIGHPRIORITY);

#ifdef LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
#endif
	tex = Image_FindTexture(identifier, subpath, flags);
	if (tex)
	{
		//FIXME: race condition is possible here
		//if a non-replaced texture is given a fallback while a non-fallback version is still loading, it can still fail.
		if (tex->status == TEX_FAILED && fallbackdata)
			tex->status = TEX_LOADING;
		else if (tex->status != TEX_NOTLOADED)
		{
#ifdef LOADERTHREAD
			Sys_UnlockMutex(com_resourcemutex);
#endif
			return tex;	//already exists
		}
		tex->flags = flags;
	}
	else
		tex = Image_CreateTexture_Internal(identifier, subpath, flags);

	tex->status = TEX_LOADING;
	if (fallbackdata)
	{
		int b = fallbackwidth*fallbackheight, pb = 0;
		switch(fallbackfmt)
		{
		case TF_INVALID:
			b = fallbackwidth;
			pb = fallbackheight;
			break;
		case TF_8PAL24:
			pb = 3*256;
			b *= 1;
			break;
		case TF_8PAL32:
			pb = 4*256;
			b *= 1;
			break;
		case PTI_R8:
		case PTI_P8:
		case TF_SOLID8:
		case TF_TRANS8:
		case TF_TRANS8_FULLBRIGHT:
		case TF_H2_T7G1:
		case TF_H2_TRANS8_0:
		case TF_H2_T4A4:
		case TF_HEIGHT8:
		case TF_HEIGHT8PAL:	//we don't care about the actual palette.
			b *= 1;
			break;
//		case PTI_LLLX8:
//		case PTI_LLLA8:
		case TF_RGBX32:
		case TF_RGBA32:
		case TF_BGRX32:
		case TF_BGRA32:
			b *= 4;
			break;
		case TF_MIP4_8PAL24:
		case TF_MIP4_8PAL24_T255:
			pb = 3*256;
		case TF_MIP4_P8:
		case TF_MIP4_SOLID8:
			b = (fallbackwidth>>0)*(fallbackheight>>0) +
				(fallbackwidth>>1)*(fallbackheight>>1) +
				(fallbackwidth>>2)*(fallbackheight>>2) +
				(fallbackwidth>>3)*(fallbackheight>>3);
			break;
		default:
			{
				unsigned int bb, bw, bh, bd;
				unsigned int lev;
				Image_BlockSizeForEncoding(fallbackfmt&~PTI_FULLMIPCHAIN, &bb, &bw, &bh, &bd);
				for (b=0, lev = 0; fallbackwidth>>lev||fallbackheight>>lev; lev++)
				{
					b += bb * (max(1,fallbackwidth>>lev)+bw-1)/bw * (max(1,fallbackheight>>lev)+bh-1)/bh;// * (max(1,fallbackdepth>>lev)+bd-1)/bd;
					if (!(fallbackfmt&PTI_FULLMIPCHAIN))
						break;
				}
			}
			break;
		}
		tex->fallbackdata = BZ_Malloc(b + pb);
		memcpy(tex->fallbackdata, fallbackdata, b);
		if (pb)
			memcpy((qbyte*)tex->fallbackdata + b, fallbackpalette, pb);
		tex->fallbackwidth = fallbackwidth;
		tex->fallbackheight = fallbackheight;
		tex->fallbackfmt = fallbackfmt;
	}
	else
	{
		tex->fallbackdata = NULL;
		tex->fallbackwidth = 0;
		tex->fallbackheight = 0;
		tex->fallbackfmt = TF_INVALID;
	}
#ifdef LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	//FIXME: pass fallback through this way instead?

	if (dontposttoworker)
		Image_LoadHiResTextureWorker(tex, NULL, 0, 0);
	else
	{
#ifdef WEBCLIENT
		if (!strncmp(tex->ident, "http://", 7) || !strncmp(tex->ident, "https://", 8))
		{
			struct dl_download *dl;
			size_t sizelimit = max(0,r_image_downloadsizelimit.ival);
			if (sizelimit>0 || !*r_image_downloadsizelimit.string)
				dl = HTTP_CL_Get(tex->ident, NULL, Image_Downloaded);
			else
			{
				Con_Printf("r_image_downloadsizelimit: image downloading is blocked\n");
				dl = NULL;
			}
			if (dl)
			{
				if (sizelimit)
					dl->sizelimit = sizelimit;
				dl->user_ctx = tex;
				dl->file = VFSPIPE_Open(1, false);
				dl->isquery = true;
			}
#ifdef MULTITHREAD
			DL_CreateThread(dl, NULL, NULL);
#else
			tex->status = TEX_FAILED;	//HACK: so nothing waits for it.
#endif
		}
		else
#endif
			if (highpri)
			COM_InsertWork(WG_LOADER, Image_LoadHiResTextureWorker, tex, NULL, 0, 0);
		else if (lowpri)
			COM_AddWork(WG_LOADER, Image_LoadHiResTextureWorker, tex, NULL, 0, 0);
		else
			COM_AddWork(WG_LOADER, Image_LoadHiResTextureWorker, tex, NULL, 0, 0);
	}
	return tex;
}
void Image_Upload			(texid_t tex, uploadfmt_t fmt, void *data, void *palette, int width, int height, int depth, unsigned int flags)
{
	struct pendingtextureinfo mips;
	size_t i;

	//skip if we're not actually changing the data/size/format.
	if (!data && tex->format == fmt && tex->width == width && tex->height == height && tex->depth == depth && tex->status == TEX_LOADED)
		return;

	mips.extrafree = NULL;
	mips.type = (flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT;
	if (!Image_GenMip0(&mips, flags, data, palette, width, height, depth, fmt, false))
		return;
	Image_GenerateMips(&mips, flags);
	Image_ChangeFormatFlags(&mips, flags, fmt, tex->ident);
	rf->IMG_LoadTextureMips(tex, &mips);
	tex->format = fmt;
	tex->width = width;
	tex->height = height;
	tex->depth = depth;
	tex->status = TEX_LOADED;

	for (i = 0; i < mips.mipcount; i++)
		if (mips.mip[i].needfree)
			BZ_Free(mips.mip[i].data);
	if (mips.extrafree)
		BZ_Free(mips.extrafree);
}

typedef struct
{
	char *name;
	char *legacyname;
	int	maximize, minmip, minimize;
	int pad;
} texmode_t;
static texmode_t texmodes[] = {
	{"n",	"GL_NEAREST",					0,	-1,	0},
	{"l",	"GL_LINEAR",					1,	-1,	1},
	{"nn",	"GL_NEAREST_MIPMAP_NEAREST",	0,	0,	0},
	{"ln",	"GL_LINEAR_MIPMAP_NEAREST",		1,	0,	1},
	{"nl",	"GL_NEAREST_MIPMAP_LINEAR",		0,	1,	0},
	{"ll",	"GL_LINEAR_MIPMAP_LINEAR",		1,	1,	1},

	//more explicit names (dupes of the above)
	{"n.n",	NULL,							0,	-1,	0},
	{"l.l",	NULL,							1,	-1,	1},
	{"nnn",	NULL,							0,	0,	0},
	{"lnl",	NULL,							1,	0,	1},
	{"nln",	NULL,							0,	1,	0},
	{"lll",	NULL,							1,	1,	1},

	//inverted mag filters
	{"n.l",	NULL,							0,	-1,	1},
	{"l.n",	NULL,							1,	-1,	0},
	{"nnl",	NULL,							0,	0,	1},
	{"lnn",	NULL,							1,	0,	0},
	{"nll",	NULL,							0,	1,	1},
	{"lln",	NULL,							1,	1,	0}
};
static void Image_ParseTextureMode(char *cvarname, char *modename, int modes[3])
{
	int i;
	modes[0] = 1;
	modes[1] = 0;
	modes[2] = 1;
	for (i = 0; i < sizeof(texmodes) / sizeof(texmodes[0]); i++)
	{
		if (!Q_strcasecmp(modename, texmodes[i].name) || (texmodes[i].legacyname && !Q_strcasecmp(modename, texmodes[i].legacyname)))
		{
			modes[0] = texmodes[i].minimize;
			modes[1] = texmodes[i].minmip;
			modes[2] = texmodes[i].maximize;
			return;
		}
	}
	Con_Printf("%s: mode %s was not recognised\n", cvarname, modename);
}
void QDECL Image_TextureMode_Callback (struct cvar_s *var, char *oldvalue)
{
	int mip[3]={1,0,1}, pic[3]={1,-1,1}, mipcap[2] = {0, 1000};
	float anis = 1, lodbias = 0;
	char *s;
	extern cvar_t gl_texturemode, gl_texturemode2d, gl_texture_anisotropic_filtering, gl_texture_lodbias, gl_mipcap;

	Image_ParseTextureMode(gl_texturemode.name, gl_texturemode.string, mip);
	Image_ParseTextureMode(gl_texturemode2d.name, gl_texturemode2d.string, pic);
	anis = gl_texture_anisotropic_filtering.value;
	//parse d_mipcap (two values, nearest furthest)
	s = COM_Parse(gl_mipcap.string);
	mipcap[0] = *com_token?atoi(com_token):0;
//	if (mipcap[0] > 3)	/*cap it to 3, so no 16*16 textures get bugged*/
//		mipcap[0] = 3;
	s = COM_Parse(s);
	mipcap[1] = *com_token?atoi(com_token):1000;
	if (mipcap[1] < mipcap[0])
		mipcap[1] = mipcap[0];
	lodbias = gl_texture_lodbias.value;

	if (rf && rf->IMG_UpdateFiltering)
		rf->IMG_UpdateFiltering(imagelist, mip, pic, mipcap, lodbias, anis);
}
qboolean Image_UnloadTexture(image_t *tex)
{
	if (tex->status == TEX_LOADED)
	{
		rf->IMG_DestroyTexture(tex);
		tex->status = TEX_NOTLOADED;
		return true;
	}
	return false;
}

//nukes an existing texture, destroying all traces. any lingering references will cause problems, so be careful about how you access these.
void Image_DestroyTexture(image_t *tex)
{
	image_t **link;
	if (!tex)
		return;
	TEXDOWAIT(tex);	//just in case.
#ifdef LOADERTHREAD
	Sys_LockMutex(com_resourcemutex);
#endif
	Image_UnloadTexture(tex);

	for (link = &imagelist; *link; link = &(*link)->next)
	{
		if (*link == tex)
		{
			*link = tex->next;
			break;
		}
	}
#ifdef LOADERTHREAD
	Sys_UnlockMutex(com_resourcemutex);
#endif
	if (*tex->ident)
		Hash_RemoveDataInsensitive(&imagetable, tex->ident, tex);
	Z_Free(tex);
}

void Image_Purge(void)
{
	image_t *tex;
	if (r_keepimages.ival)
		return;
	Shader_TouchTextures();
	for (tex = imagelist; tex; tex = tex->next)
	{
		if (tex->flags & IF_NOPURGE)
			continue;
		if (tex->regsequence != r_regsequence)
			Image_UnloadTexture(tex);
	}
}


void Image_List_f(void)
{
	flocation_t loc;
	image_t *tex, *a;
	int loaded = 0, aliases = 0, failed = 0, total = 0;
	size_t drivermem = 0;
	size_t aliasedmem = 0;
	size_t imgmem;
	unsigned int loadflags;
	char fname[MAX_QPATH];
	const char *filter = Cmd_Argv(1);
	qboolean hasdupes;
	for (tex = imagelist; tex; tex = tex->next)
	{
		total++;
		if (*filter && !strstr(tex->ident, filter))
			continue;
		if (tex->status == TEX_FAILED && !developer.ival)
		{	//don't show all the failures.
			failed++;
			continue;
		}

		a = Hash_GetInsensitive(&imagetable, tex->ident);
		hasdupes = (a != tex || Hash_GetNextInsensitive(&imagetable, tex->ident, a));

		if (((tex->flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT) == PTI_2D || ((tex->flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT) == PTI_CUBE)
			Con_Printf("^[\\imgptr\\%#"PRIxSIZE"^]", (size_t)tex);
		if (tex->subpath)
			Con_Printf("^h(%s)^h", tex->subpath);
//		Con_DLPrintf(1, " %x", tex->flags);

		if (Image_LocateHighResTexture(tex, &loc, fname, sizeof(fname), &loadflags))
		{
			char defuck[MAX_OSPATH], *bullshit;
			Q_strncpyz(defuck, loc.search->logicalpath, sizeof(defuck));
			while((bullshit=strchr(defuck, '\\')))
				*bullshit = '/';

			if (((tex->flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT) == PTI_2D||((tex->flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT) == PTI_CUBE || tex->format == PTI_P8)
				Con_Printf("^[%s\\tip\\%s/%s\n\n%gkb\\tipimgptr\\%#"PRIxSIZE"^]: ", tex->ident, defuck, fname, loc.len/1024.0f, (size_t)tex);
			else
				Con_Printf("^[%s\\tip\\%s/%s\n\n%gkb^]: ", tex->ident, defuck, fname, loc.len/1024.0f);
		}
		else
		{
			loc.len = 0;
			Con_Printf("^[%s\\tipimgptr\\%#"PRIxSIZE"^]: ", tex->ident, (size_t)tex);
		}

		for (a = tex->aliasof; a; a = a->aliasof)
		{
			if (a->subpath)
				Con_Printf("^3^h(%s)^h%s: ", a->subpath, a->ident);
			else
				Con_Printf("^3%s: ", a->ident);
		}

		if (developer.ival || hasdupes)
		{
			if (tex->flags & IF_CLAMP)			Con_Printf("^[^8CLAMP\\tip\\clamp to edge^] ");
			if (tex->flags & IF_NOMIPMAP)		Con_Printf("^[^8NOMIPMAP\\tip\\disable mipmaps.^] ");
			if (tex->flags & IF_NEAREST)		Con_Printf("^[^8NEAREST\\tip\\force nearest^] ");
			if (tex->flags & IF_LINEAR)			Con_Printf("^[^8LINEAR\\tip\\force linear^] ");
			if (tex->flags & IF_UIPIC)			Con_Printf("^[^8UIPIC\\tip\\subject to texturemode2d^] ");
			if (tex->flags & IF_SRGB)			Con_Printf("^[^8SRGB\\tip\\texture data is srgb (read-as-linear)^] ");
			if (tex->flags & IF_NOPICMIP)		Con_Printf("^[^8NOPICMIP\\tip\\ignores picmip settings^] ");
			if (tex->flags & IF_NOALPHA)		Con_Printf("^[^8NOALPHA\\tip\\hint rather than requirement^] ");
			if (tex->flags & IF_NOGAMMA)		Con_Printf("^[^8NOGAMMA\\tip\\do not apply legacy texture-based gamma^] ");
			switch((tex->flags&IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT)
			{
			case PTI_2D:						Con_Printf("^[^82D\\tip\\image is a standard 2d image^] ");break;
			case PTI_3D:						Con_Printf("^[^83D\\tip\\image is a 3d image^] ");break;
			case PTI_CUBE:						Con_Printf("^[^8CUBE\\tip\\image is a cubemap^] ");break;
			case PTI_2D_ARRAY:					Con_Printf("^[^82D_ARRAY\\tip\\image is an array of 2d images^] ");break;
			case PTI_CUBE_ARRAY:				Con_Printf("^[^8CUBE_ARRAY\\tip\\image is an array of cubemaps^] ");break;
			case PTI_ANY:						Con_Printf("^[^8ANY\\tip\\image type is unspecified, allowing any^] ");break;
			default:							Con_Printf("^[^8<ERROR>\\tip\\image type not defined^] ");break;
			}
			if (tex->flags & IF_MIPCAP)			Con_Printf("^[^8MIPCAP\\tip\\allow the use of d_mipcap^] ");
			if (tex->flags & IF_PREMULTIPLYALPHA)Con_Printf("^[^8PREMULTIPLYALPHA\\tip\\rgb *= alpha^] ");
			if (tex->flags & IF_UNUSED15)		Con_Printf("^[^8UNUSED15\\tip\\...^] ");
			if (tex->flags & IF_UNUSED16)		Con_Printf("^[^8UNUSED16\\tip\\...^] ");
			if (tex->flags & IF_INEXACT)		Con_Printf("^[^8INEXACT\\tip\\subdir info isn't to be used for matching^] ");
			if (tex->flags & IF_WORLDTEX)		Con_Printf("^[^8WORLDTEX\\tip\\gl_picmip_world^] ");
			if (tex->flags & IF_SPRITETEX)		Con_Printf("^[^8SPRITETEX\\tip\\gl_picmip_sprites^] ");
			if (tex->flags & IF_NOSRGB)			Con_Printf("^[^8NOSRGB\\tip\\ignore srgb when loading. this is guarenteed to be linear, for normalmaps etc.^] ");
			if (tex->flags & IF_PALETTIZE)		Con_Printf("^[^8PALETTIZE\\tip\\convert+load it as an RTI_P8 texture for the current palette/colourmap^] ");
			if (tex->flags & IF_NOPURGE)		Con_Printf("^[^8NOPURGE\\tip\\texture is not flushed when no more shaders refer to it (for C code that holds a permanant reference to it - still purged on vid_reloads though)^] ");
			if (tex->flags & IF_HIGHPRIORITY)	Con_Printf("^[^8HIGHPRIORITY\\tip\\pushed to start of worker queue instead of end...^] ");
			if (tex->flags & IF_LOWPRIORITY)	Con_Printf("^[^8LOWPRIORITY\\tip\\load slowly to favour others instead^] ");
			if (tex->flags & IF_LOADNOW)		Con_Printf("^[^8LOADNOW\\tip\\hit the disk now, and delay the gl load until its actually needed. this is used only so that the width+height are known in advance. valid on worker threads.^] ");
			if (tex->flags & IF_NOPCX)			Con_Printf("^[^8NOPCX\\tip\\block pcx format. meaning qw skins can use team colours and cropping^] ");
			if (tex->flags & IF_TRYBUMP)		Con_Printf("^[^8TRYBUMP\\tip\\attempt to load _bump if the specified _norm texture wasn't found^] ");
			if (tex->flags & IF_RENDERTARGET)	Con_Printf("^[^8RENDERTARGET\\tip\\never loaded from disk, loading can't fail^] ");
			if (tex->flags & IF_EXACTEXTENSION)	Con_Printf("^[^8EXACTEXTENSION\\tip\\don't mangle extensions, use what is specified and ONLY that^] ");
			if (tex->flags & IF_NOREPLACE)		Con_Printf("^[^8NOREPLACE\\tip\\don't load a replacement, for some reason^] ");
			if (tex->flags & IF_NOWORKER)		Con_Printf("^[^8NOWORKER\\tip\\don't pass the work to a loader thread. this gives fully synchronous loading. only valid from the main thread.^] ");
		}

		if (tex->status == TEX_LOADED)
		{
			char *type;
			unsigned int blockbytes, blockwidth, blockheight, blockdepth;
			Image_BlockSizeForEncoding(tex->format, &blockbytes, &blockwidth, &blockheight, &blockdepth);
			imgmem = blockbytes * (tex->width+blockwidth-1)/blockwidth * (tex->height+blockheight-1)/blockheight * (tex->depth+blockdepth-1)/blockdepth;
			switch((tex->flags & IF_TEXTYPEMASK)>>IF_TEXTYPESHIFT)
			{
			case PTI_2D:		type = "";			break;
			case PTI_3D:		type = "3D ";		break;
			case PTI_CUBE:		type = "Cube ";		break;
			case PTI_2D_ARRAY:	type = "Array ";		break;
			case PTI_CUBE_ARRAY:type = "CubeArray ";	break;
			default:			type = "UNKNOWN ";	break;
			}
			if (!(tex->flags & IF_NOMIPMAP))
				imgmem += imgmem/3;	//mips take about a third extra mem.

			//FIXME: not showing sizes here, because they show the '8bit' size rather than any replacementment's size.

			if (tex->depth != 1)
				Con_Printf("^2loaded (%s%i*%i*%i ^[^4%s\\tip\\%g bits per texel^])\n", type, tex->width, tex->height, tex->depth, Image_FormatName(tex->format), (8.f*blockbytes)/(blockwidth*blockheight*blockdepth));
			else
				Con_Printf("^2loaded (%s%i*%i ^[^4%s\\tip\\%g bits per texel^])\n", type, tex->width, tex->height, Image_FormatName(tex->format), (8.f*blockbytes)/(blockwidth*blockheight*blockdepth));
			if (tex->aliasof)
			{
				aliasedmem += imgmem;
				aliases++;
			}
			else
			{
				drivermem += imgmem;
				loaded++;
			}
		}
		else if (tex->status == TEX_FAILED)
		{
			Con_Printf("^1failed\n");
			failed++;
		}
		else if (tex->status == TEX_NOTLOADED)
			Con_Printf("^5not loaded\n");
		else
			Con_Printf("^bloading\n");
	}

	Con_Printf("%i images total\n", total);
	Con_Printf("%i images loaded (%gMB)\n", loaded, drivermem/(1024*1024.0));
	Con_Printf("%i image alises (%gMB)\n", aliases, aliasedmem/(1024*1024.0));
	Con_Printf("%i images failed\n", failed);
}

void Image_Formats_f(void)
{
	size_t i;
	float bpp;
	int blockbytes, blockwidth, blockheight, blockdepth;

#ifdef GLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		Con_Printf("OpenGL info:\n");
		Con_Printf("OpenGL Version: %s%g\n", gl_config.gles?"ES ":"", gl_config.glversion);
		Con_Printf("OpenGLSL Version: %i\n", gl_config.maxglslversion);
		Con_Printf("OpenGLSL Attributes: %u\n", gl_config.maxattribs);

		Con_Printf("arb_texture_env_combine: %u\n", gl_config.arb_texture_env_combine);
		Con_Printf("arb_texture_env_dot3: %u\n", gl_config.arb_texture_env_dot3);
		Con_Printf("arb_texture_compression: %u\n", gl_config.arb_texture_compression);
		Con_Printf("geometryshaders: %u\n", gl_config.geometryshaders);
		Con_Printf("ext_framebuffer_objects: %u\n", gl_config.ext_framebuffer_objects);
		Con_Printf("arb_framebuffer_srgb: %u\n", gl_config.arb_framebuffer_srgb);
		Con_Printf("arb_shader_objects: %u\n", gl_config.arb_shader_objects);
		Con_Printf("arb_shadow: %u\n", gl_config.arb_shadow);
		Con_Printf("arb_depth_texture: %u\n", gl_config.arb_depth_texture);
		Con_Printf("ext_stencil_wrap: %u\n", gl_config.ext_stencil_wrap);
		Con_Printf("ext_packed_depth_stencil: %u\n", gl_config.ext_packed_depth_stencil);
		Con_Printf("arb_depth_clamp: %u\n", gl_config.arb_depth_clamp);
		Con_Printf("ext_texture_filter_anisotropic: %u\n", gl_config.ext_texture_filter_anisotropic);
	}
#endif

	Con_Printf(		"            Programs: "S_COLOR_GREEN"%s\n", sh_config.progs_supported?va(sh_config.progpath, "*"):S_COLOR_RED"Unsupported");
	if (sh_config.progs_supported)
	{
		Con_Printf(	"     Shader versions: %u - %u\n", sh_config.minver, sh_config.maxver);
		Con_Printf(	"       Max GPU Bones: %s%u\n", sh_config.max_gpu_bones?S_COLOR_GREEN:S_COLOR_RED, sh_config.max_gpu_bones);
	}
	Con_Printf(		"     Legacy Pipeline: %s\n", sh_config.progs_required?S_COLOR_RED"Unsupported":S_COLOR_GREEN"Supported");
	if (!sh_config.progs_required)
	{
		Con_Printf(	"       Env_Combiners: %s\n", sh_config.nv_tex_env_combine4?S_COLOR_GREEN"Extended":sh_config.tex_env_combine?S_COLOR_GREEN"Supported":S_COLOR_RED"Unsupported");
		Con_Printf(	"             Env_Add: %s\n", sh_config.env_add?S_COLOR_GREEN"Supported":S_COLOR_RED"Unsupported");
	}
	Con_Printf(		"  Max Texture2d Size: %s%u*%u\n", S_COLOR_GREEN, sh_config.texture2d_maxsize, sh_config.texture2d_maxsize);
	Con_Printf(		"Max Texture2d Layers: %s%u\n", sh_config.texture2darray_maxlayers?S_COLOR_GREEN:S_COLOR_RED, sh_config.texture2darray_maxlayers);
	Con_Printf(		"  Max Texture3d Size: %s%u*%u*%u\n", sh_config.texture3d_maxsize?S_COLOR_GREEN:S_COLOR_RED, sh_config.texture3d_maxsize, sh_config.texture3d_maxsize, sh_config.texture3d_maxsize);
	Con_Printf(		"Max TextureCube Size: %s%u*%u\n", sh_config.havecubemaps?S_COLOR_GREEN:S_COLOR_RED, sh_config.texturecube_maxsize, sh_config.texturecube_maxsize);
	Con_Printf(		"    Non-Power-Of-Two: %s%s\n", sh_config.texture_non_power_of_two?S_COLOR_GREEN"Supported":(sh_config.texture_non_power_of_two_pic?S_COLOR_YELLOW"Limited":S_COLOR_RED"Unsupported"), sh_config.npot_rounddown?" (rounded down)":"");
	Con_Printf(		"  Block Size Padding: %s\n", sh_config.texture_allow_block_padding?S_COLOR_GREEN"Supported":S_COLOR_RED"Unsupported");
	Con_Printf(		"              Mipcap: %s\n", sh_config.can_mipcap?S_COLOR_GREEN"Supported":S_COLOR_RED"Unsupported");

	Con_Printf(		"\n      Driver Support:\n");
	for (i = 0; i < PTI_MAX; i++)
	{
		switch(i)
		{
		case PTI_EMULATED:
			continue;
		default:
			break;
		}
		Image_BlockSizeForEncoding(i, &blockbytes, &blockwidth, &blockheight, &blockdepth);
		bpp = blockbytes*8.0/(blockwidth*blockheight*blockdepth);
		if (blockbytes)
			Con_Printf("%20s: %s"S_COLOR_GRAY" (%s%.3g-bpp)\n", Image_FormatName(i), sh_config.texfmt[i]?S_COLOR_GREEN"Enabled":S_COLOR_RED"Disabled", (blockdepth!=1)?"3d, ":"", bpp);
		else
			Con_Printf("%20s: %s\n", Image_FormatName(i), sh_config.texfmt[i]?S_COLOR_GREEN"Enabled":S_COLOR_RED"Disabled");
	}
}


//load the named file, without failing.
texid_t R_LoadHiResTexture(const char *name, const char *subpath, unsigned int flags)
{
	char nicename[MAX_QPATH], *data;
	if (!*name)
		return r_nulltex;
	Q_strncpyz(nicename, name, sizeof(nicename));
	while((data = strchr(nicename, '*')))
		*data = '#';
	return Image_GetTexture(nicename, subpath, flags, NULL, NULL, 0, 0, TF_INVALID);	//queues the texture creation.
}

//attempt to load the named texture
//will not load external textures if gl_load24bit is set (failing instantly if its just going to fail later on anyway)
//the specified data will be used if the high-res image is blocked/not found.
texid_t R_LoadReplacementTexture(const char *name, const char *subpath, unsigned int flags, void *lowres, int lowreswidth, int lowresheight, uploadfmt_t format)
{
	char nicename[MAX_QPATH], *data;
	if (!*name)
		return r_nulltex;
	if (!gl_load24bit.ival && !lowres)
		return r_nulltex;
	Q_strncpyz(nicename, name, sizeof(nicename));
	while((data = strchr(nicename, '*')))
		*data = '#';
	return Image_GetTexture(nicename, subpath, flags, lowres, NULL, lowreswidth, lowresheight, format);	//queues the texture creation.
}
#ifdef RTLIGHTS
void R_LoadNumberedLightTexture(dlight_t *dl, int cubetexnum)
{
	Q_snprintfz(dl->cubemapname, sizeof(dl->cubemapname), "cubemaps/%i", cubetexnum);
	if (!gl_load24bit.ival)
		dl->cubetexture = r_nulltex;
	else
		dl->cubetexture = Image_GetTexture(dl->cubemapname, NULL, IF_TEXTYPE_CUBE, NULL, NULL, 0, 0, TF_INVALID);
}
#endif

//destroys all textures
void Image_Shutdown(void)
{
	image_t *tex;
	int i = 0, j = 0;
	Cmd_RemoveCommand("r_imagelist");
	Cmd_RemoveCommand("r_imageformats");
	while (imagelist)
	{
		tex = imagelist;
		if (*tex->ident)
			Hash_RemoveDataInsensitive(&imagetable, tex->ident, tex);
		imagelist = tex->next;
		if (tex->status == TEX_LOADED)
			j++;
		rf->IMG_DestroyTexture(tex);
		Z_Free(tex);
		i++;
	}
	if (i)
		Con_DPrintf("Destroyed %i/%i images\n", j, i);

	if (wadmutex)
		Sys_DestroyMutex(wadmutex);
	wadmutex = NULL;
}
#endif

//may not create any images yet.
void Image_Init(void)
{
	static qboolean initedlibs;
	if (!initedlibs)
	{
		initedlibs = true;
		#ifdef AVAIL_JPEGLIB
			LibJPEG_Init();
		#endif
		#ifdef AVAIL_PNGLIB
			LibPNG_Init();
		#endif
		#ifdef IMAGEFMT_EXR
			InitLibrary_OpenEXR();
		#endif
	}

#ifdef HAVE_CLIENT
	wadmutex = Sys_CreateMutex();
	memset(imagetablebuckets, 0, sizeof(imagetablebuckets));
	Hash_InitTable(&imagetable, sizeof(imagetablebuckets)/sizeof(imagetablebuckets[0]), imagetablebuckets);

	Cmd_AddCommandD("r_imagelist", Image_List_f, "Prints out a list of the currently-known textures.");
	Cmd_AddCommandD("r_imageformats", Image_Formats_f, "Prints out a list of the usable hardware pixel formats.");
#endif
}

#ifdef HAVE_CLIENT
// ocrana led functions
static int ledcolors[8][3] =
{
	// green
	{ 0, 255, 0 },
	{ 0, 127, 0 },
	// red
	{ 255, 0, 0 },
	{ 127, 0, 0 },
	// yellow
	{ 255, 255, 0 },
	{ 127, 127, 0 },
	// blue
	{ 0, 0, 255 },
	{ 0, 0, 127 }
};

void AddOcranaLEDsIndexed (qbyte *image, int h, int w)
{
	int tridx[8]; // transition indexes
	qbyte *point;
	int i, idx, x, y, rs;
	int r, g, b, rd, gd, bd;

	// calc row size, character size
	rs = w;
	h /= 16;
	w /= 16;

	// generate palettes
	for (i = 0; i < 4; i++)
	{
		// get palette
		r = ledcolors[i*2][0];
		g = ledcolors[i*2][1];
		b = ledcolors[i*2][2];
		rd = (r - ledcolors[i*2+1][0]) / 8;
		gd = (g - ledcolors[i*2+1][1]) / 8;
		bd = (b - ledcolors[i*2+1][2]) / 8;
		for (idx = 0; idx < 8; idx++)
		{
			tridx[idx] = GetPaletteIndex(r, g, b);
			r -= rd;
			g -= gd;
			b -= bd;
		}

		// generate LED into image
		b = (w * w + h * h) / 16;
		if (b < 1)
			b = 1;
		rd = w + 1;
		gd = h + 1;

		point = image + (8 * rs * h) + ((6 + i) * w);
		for (y = 1; y <= h; y++)
		{
			for (x = 1; x <= w; x++)
			{
				r = rd - (x*2); r *= r;
				g = gd - (y*2); g *= g;
				idx = (r + g) / b;

				if (idx > 7)
					*point++ = 0;
				else
					*point++ = tridx[idx];
			}
			point += rs - w;
		}
	}
}

/*
Find closest color in the palette for named color
*/
int MipColor(int r, int g, int b)
{
	int i;
	float dist;
	int best=15;
	float bestdist;
	int r1, g1, b1;
	static int lr = -1, lg = -1, lb = -1;
	static int lastbest;

	if (r == lr && g == lg && b == lb)
		return lastbest;

	bestdist = 256*256*3;

	for (i = 0; i < 256; i++)
	{
		r1 = host_basepal[i*3] - r;
		g1 = host_basepal[i*3+1] - g;
		b1 = host_basepal[i*3+2] - b;
		dist = r1*r1 + g1*g1 + b1*b1;
		if (dist < bestdist) {
			bestdist = dist;
			best = i;
		}
	}
	lr = r; lg = g; lb = b;
	lastbest = best;
	return best;
}

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION
static void stbiwritefunc(void *context, void *data, int size)
{
	vfsfile_t *f = context;
	VFS_WRITE(f, data, size);
}
#endif

qboolean SCR_ScreenShot (char *filename, enum fs_relative fsroot, void **buffer, int numbuffers, qintptr_t bytestride, int width, int height, enum uploadfmt fmt, qboolean writemeta)
{
	char ext[8];
	void *nbuffers[2];

	switch(fmt)
	{	//nuke any alpha channel...
	case TF_RGBA32: fmt = TF_RGBX32; break;
	case TF_BGRA32: fmt = TF_BGRX32; break;
	default: break;
	}

	if (!bytestride)
		bytestride = width*4;
	if (bytestride < 0)
	{	//fix up the buffers so callers don't have to.
		int nb = numbuffers;
		for (numbuffers = 0; numbuffers < nb && numbuffers < countof(nbuffers); numbuffers++)
		nbuffers[numbuffers] = (char*)buffer[numbuffers] - bytestride*(height-1);
		buffer = nbuffers;
	}

	COM_FileExtension(filename, ext, sizeof(ext));

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION
	if (numbuffers == 1)
	{
		vfsfile_t *outfile = NULL;
		qboolean ret = false;
		int comp = 0;
		if (comp == PTI_L8)
			comp = 1;
		else if (comp == PTI_L8A8)
			comp = 2;
		else if (comp == PTI_RGB8)
			comp = 3;
		else if (comp == PTI_RGBA8)
			comp = 4;
		else if (comp == PTI_RGBA32F)
			comp = -4;
		else if (comp == PTI_R32F)
			comp = -1;

#ifdef STBIW_ONLY_PNG
		if (!Q_strcasecmp(ext, "png") && comp>0)
		{	//does NOT support pngs
			//lacks metadata
			outfile=FS_OpenVFS(filename, "wb", FS_GAMEONLY);
			ret = stbi_write_png_to_func(stbiwritefunc, outfile, width, height, comp, buffer[0], bytestride);
		}
#endif
#ifdef STBIW_ONLY_JPEG
		if ((!Q_strcasecmp(ext, "jpg") || !Q_strcasecmp(ext, "jpeg")) && comp>0 && bytestride == width*sizeof(float)*comp)
		{
			//lacks metadata
			outfile=FS_OpenVFS(filename, "wb", FS_GAMEONLY);
			ret = stbi_write_jpg_to_func(stbiwritefunc, outfile, width, height, comp, buffer[0], scr_sshot_compression.value/*its in percent*/);
		}
#endif
#ifdef STBIW_ONLY_HDR
		if (!Q_strcasecmp(ext, "hdr") && comp<0 && bytestride == width*sizeof(float)*-comp)
		{
			outfile=FS_OpenVFS(filename, "wb", FS_GAMEONLY);
			ret = stbi_write_hdr_to_func(stbiwritefunc, outfile, width, height, -comp, buffer[0]);
		}
#endif
#ifdef STBIW_ONLY_TGA
		if (!Q_strcasecmp(ext, "tga") && comp>0 && bytestride == width*sizeof(float)*comp)
		{
			outfile=FS_OpenVFS(filename, "wb", FS_GAMEONLY);
			ret = stbi_write_tga_to_func(stbiwritefunc, outfile, width, height, comp, buffer[0]);
		}
#endif
#ifdef STBIW_ONLY_BMP
		if (!Q_strcasecmp(ext, "bmp") && comp>0 && bytestride == width*sizeof(float)*comp)
		{
			outfile=FS_OpenVFS(filename, "wb", FS_GAMEONLY);
			ret = stbi_write_bmp_to_func(stbiwritefunc, outfile, width, height, comp, buffer[0]);
		}
#endif

		if (outfile)
		{
			VFS_CLOSE(outfile);
			return ret;
		}
	}
#endif
#ifdef AVAIL_PNGLIB
	if (!Q_strcasecmp(ext, "png") || !Q_strcasecmp(ext, "pns"))
	{
		//png can do bgr+rgb
		//rgba bgra will result in an extra alpha chan
		//actual stereo is also supported. huzzah.
		return Image_WritePNG(filename, fsroot, scr_sshot_compression.value, buffer, numbuffers, bytestride, width, height, fmt, writemeta);
	}
#endif
#ifdef AVAIL_JPEGLIB
	if (!Q_strcasecmp(ext, "jpeg") || !Q_strcasecmp(ext, "jpg") || !Q_strcasecmp(ext, "jps"))
	{
		return screenshotJPEG(filename, fsroot, scr_sshot_compression.value, buffer[0], bytestride, width, height, fmt, writemeta);
	}
#endif
#ifdef IMAGEFMT_BMP
	if (!Q_strcasecmp(ext, "bmp"))
	{
		return WriteBMPFile(filename, fsroot, buffer[0], bytestride, width, height, fmt);
	}
#endif
#ifdef IMAGEFMT_PCX
	if (!Q_strcasecmp(ext, "pcx"))
	{
		int y, x, s;
		qbyte *src, *dest;
		qbyte *srcbuf = buffer[0], *dstbuf;
		if (fmt == TF_RGB24 || fmt == TF_RGBA32 || fmt == TF_RGBX32 || fmt == PTI_LLLA8 || fmt == PTI_LLLX8)
		{
			dstbuf = malloc((intptr_t)width*height);
			s = (fmt == TF_RGB24)?3:4;
			// convert in-place to eight bit
			for (y = 0; y < height; y++)
			{
				src = srcbuf + (bytestride * y);
				dest = dstbuf + (width * y);

				for (x = 0; x < width; x++) {
					*dest++ = MipColor(src[0], src[1], src[2]);
					src += s;
				}
			}
		}
		else if (fmt == TF_BGR24 || fmt == TF_BGRA32 || fmt == TF_BGRX32)
		{
			dstbuf = malloc((intptr_t)width*height);
			s = (fmt == TF_BGR24)?3:4;
			// convert in-place to eight bit
			for (y = 0; y < height; y++)
			{
				src = srcbuf + (bytestride * y);
				dest = dstbuf + (width * y);

				for (x = 0; x < width; x++) {
					*dest++ = MipColor(src[2], src[1], src[0]);
					src += s;
				}
			}
		}
		else
			return false;

		WritePCXfile (filename, fsroot, dstbuf, width, height, width, host_basepal, false);
		free(dstbuf);
		return true;
	}
#endif
#ifdef IMAGEFMT_TGA
	if (!Q_strcasecmp(ext, "tga"))	//tga
		return WriteTGA(filename, fsroot, buffer[0], bytestride, width, height, fmt);
#endif
#ifdef IMAGEFMT_KTX
	if (!Q_strcasecmp(ext, "ktx") && bytestride > 0)	//ktx
	{
		struct pendingtextureinfo out = {PTI_2D};
		out.encoding = fmt;
		out.mipcount = 1;
		out.mip[0].data = buffer[0];
		out.mip[0].datasize = bytestride*height;
		out.mip[0].width = width;
		out.mip[0].height = height;
		out.mip[0].depth = 1;
		return Image_WriteKTXFile(filename, fsroot, &out);
	}
#endif
#ifdef IMAGEFMT_DDS
	if (!Q_strcasecmp(ext, "dds") && bytestride > 0)	//dds
	{
		struct pendingtextureinfo out = {PTI_2D};
		out.encoding = fmt;
		out.mipcount = 1;
		out.mip[0].data = buffer[0];
		out.mip[0].datasize = bytestride*height;
		out.mip[0].width = width;
		out.mip[0].height = height;
		out.mip[0].depth = 1;
		return Image_WriteDDSFile(filename, fsroot, &out);
	}
#endif

	//extension / type not recognised.
	return false;
}
#endif
