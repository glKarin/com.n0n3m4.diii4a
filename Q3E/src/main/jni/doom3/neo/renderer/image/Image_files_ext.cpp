// basic defines and includes
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ONLY_JPEG // at least for now, only use it for JPEG
#define STBI_NO_STDIO  // images are passed as buffers
#define STBI_ONLY_PNG

#include "../../externlibs/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../externlibs/stb/stb_image_write.h"

#include "../../externlibs/soil/image_dxt.h"
#include "../../externlibs/soil/soil_dds_image.h"

// Load functions
void LoadJPG_stb(const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp)
{
    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load JPG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

void LoadPNG(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp)
{

    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load PNG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

void LoadDDS(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp)
{

    byte	*fbuffer;
    int	len;

    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

    {
        idFile *f;

        f = fileSystem->OpenFileRead(filename);

        if (!f) {
            return;
        }

        len = f->Length();

        if (timestamp) {
            *timestamp = f->Timestamp();
        }

        if (!pic) {
            fileSystem->CloseFile(f);
            return;	// just getting timestamp
        }

        fbuffer = (byte *)Mem_ClearedAlloc(len);
        f->Read(fbuffer, len);
        fileSystem->CloseFile(f);
    }

    int w=0, h=0, comp=0;
    byte* decodedImageData = stbi_dds_load_from_memory( fbuffer, len, &w, &h, &comp, STBI_rgb_alpha );

    Mem_Free( fbuffer );

    if ( decodedImageData == NULL ) {
        common->Warning( "stb_image was unable to load PNG %s : %s\n",
                         filename, stbi_failure_reason());
        return;
    }

    // *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
    // (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
    // so the decoded data must be copied once
    int size = w*h*4;
    *pic = (byte *)R_StaticAlloc( size );
    memcpy( *pic, decodedImageData, size );
    *width = w;
    *height = h;
    // now that decodedImageData has been copied into *pic, it's not needed anymore
    stbi_image_free( decodedImageData );
}

// Utility
static void R_ImageFlipVertical(byte *data, int width, int height, int comp)
{
    int		i;
    byte		*line1, *line2;
    byte *swapBuffer;
    int lineSize = width * comp;

    swapBuffer = (byte *)malloc(width * comp);
    for (i = 0; i < height / 2; ++i) {
        line1 = &data[i * lineSize];
        line2 = &data[(height - i - 1) * lineSize];
        memcpy(swapBuffer, line1, lineSize);
        memcpy(line1, line2, lineSize);
        memcpy(line2, swapBuffer, lineSize);
    }
    free(swapBuffer);
}

#ifdef _HARM_IMAGE_RGBA_TO_RGB
static void R_ImageRGBA8888ToRGB888(const byte *data, int width, int height, byte *to)
{
	int		i, j;
	const byte		*temp;
	byte *target;

	for (i = 0 ; i < width ; i++) {
		for (j = 0 ; j < height ; j++) {
			temp = data + j * 4 * width + i * 4;
			target = to + j * 3 * width + i * 3;
			memcpy(target, temp, 3);
		}
	}
}
#endif

// Save functions
static void R_StbWriteImageFile(void *context, void *data, int size)
{
    idFile *f = (idFile *)context;
    f->Write(data, size);
}

void R_WritePNG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, int quality, const char *basePath)
{
    stbi_write_png_compression_level = idMath::ClampInt(0, 9, quality);
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_png_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data, width * comp);
        if(!r)
            common->Warning("R_WritePNG fail: %d", r);
        fileSystem->CloseFile(f);
    }
    free(ndata);
}

void R_WriteJPG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, int compression, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_jpg_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data, idMath::ClampInt(1, 100, compression));
        if(!r)
            common->Warning("R_WriteJPG fail: %d", r);
        fileSystem->CloseFile(f);
    }
    free(ndata);
}

void R_WriteBMP(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = stbi_write_bmp_to_func(R_StbWriteImageFile, (void *)f, width, height, comp, data);
        if(!r)
            common->Warning("R_WriteBMP fail: %d", r);
        fileSystem->CloseFile(f);
    }
    free(ndata);
}

/*
=========================================================

EXR LOADING

Interfaces with tinyexr
=========================================================
*/

/*
=======================
LoadEXR
=======================
*/
void LoadEXR(const char* filename, byte **pic, int *width, int *height, ID_TIME_T* timestamp)
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	float* rgba;
	const char* err;

	{
		int ret = LoadEXRFromMemory( &rgba, width, height, fbuffer, fileSize, &err );
		if( ret != 0 )
		{
			common->Error( "LoadEXR( %s ): %s\n", filename, err );
			return;
		}
	}

#if 0
	// dump file as .hdr for testing - this works
	{
		idStrStatic< MAX_OSPATH > hdrFileName = "test";
		//hdrFileName.AppendPath( filename );
		hdrFileName.SetFileExtension( ".hdr" );

		int ret = stbi_write_hdr( hdrFileName.c_str(), *width, *height, 4, rgba );

		if( ret == 0 )
		{
			return; // fail
		}
	}
#endif

	if( rgba )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4 );

		*pic = out;

		// convert to packed R11G11B10F as uint32 for each pixel

		const float* src = rgba;
		byte* dst = out;
		for( int i = 0; i < pixelCount; i++ )
		{
#if 0 //karin: R11G11B10F
			// read 3 floats and ignore the alpha channel
			float p[3];

			p[0] = src[0];
			p[1] = src[1];
			p[2] = src[2];

			// convert
			uint32_t value = float3_to_r11g11b10f( p );
			value = (((byte)(src[0] * 255.0f))) 
				| (((byte)(src[1] * 255.0f)) << 8) 
				| (((byte)(src[2] * 255.0f)) << 16)
				| (((byte)(src[3] * 255.0f)) << 24)
				;
			*( uint32_t* )dst = value;
#else // RGBA8888
			dst[0] = (byte)(src[0] * 255.0f);
			dst[1] = (byte)(src[1] * 255.0f);
			dst[2] = (byte)(src[2] * 255.0f);
			dst[3] = (byte)(src[3] * 255.0f);
#endif

			src += 4;
			dst += 4;
		}

		free( rgba );
	}

	// RB: EXR needs to be flipped to match the .tga behavior
	//R_VerticalFlip( *pic, *width, *height );

	Mem_Free( ( void* )fbuffer );
}

/*
================
R_WriteEXR
================
*/
void R_WriteEXR( const char* filename, const byte* rgba, int width, int height, int channelsPerPixel, bool flipVertical, const char* basePath )
{
#if 0
	// miniexr.cpp - v0.2 - public domain - 2013 Aras Pranckevicius / Unity Technologies
	//
	// Writes OpenEXR RGB files out of half-precision RGBA or RGB data.
	//
	// Only tested on Windows (VS2008) and Mac (clang 3.3), little endian.
	// Testing status: "works for me".
	//
	// History:
	// 0.2 Source data can be RGB or RGBA now.
	// 0.1 Initial release.

	const unsigned ww = width - 1;
	const unsigned hh = height - 1;
	const unsigned char kHeader[] =
	{
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 0, // version, scanline
		// channels
		'c', 'h', 'a', 'n', 'n', 'e', 'l', 's', 0,
		'c', 'h', 'l', 'i', 's', 't', 0,
		55, 0, 0, 0,
		'B', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // B, half
		'G', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // G, half
		'R', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // R, half
		0,
		// compression
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		1, 0, 0, 0,
		0, // no compression
		// dataWindow
		'd', 'a', 't', 'a', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// displayWindow
		'd', 'i', 's', 'p', 'l', 'a', 'y', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// lineOrder
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		1, 0, 0, 0,
		0, // increasing Y
		// pixelAspectRatio
		'p', 'i', 'x', 'e', 'l', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// screenWindowCenter
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'C', 'e', 'n', 't', 'e', 'r', 0,
		'v', '2', 'f', 0,
		8, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		// screenWindowWidth
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'W', 'i', 'd', 't', 'h', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// end of header
		0,
	};
	const int kHeaderSize = sizeof( kHeader );

	const int kScanlineTableSize = 8 * height;
	const unsigned pixelRowSize = width * 3 * 2;
	const unsigned fullRowSize = pixelRowSize + 8;

	unsigned bufSize = kHeaderSize + kScanlineTableSize + height * fullRowSize;
	unsigned char* buf = ( unsigned char* )Mem_Alloc( bufSize, TAG_TEMP );
	if( !buf )
	{
		return;
	}

	// copy in header
	memcpy( buf, kHeader, kHeaderSize );

	// line offset table
	unsigned ofs = kHeaderSize + kScanlineTableSize;
	unsigned char* ptr = buf + kHeaderSize;
	for( int y = 0; y < height; ++y )
	{
		*ptr++ = ofs & 0xFF;
		*ptr++ = ( ofs >> 8 ) & 0xFF;
		*ptr++ = ( ofs >> 16 ) & 0xFF;
		*ptr++ = ( ofs >> 24 ) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		ofs += fullRowSize;
	}

	// scanline data
	const unsigned char* src = ( const unsigned char* )rgba16f;
	const int stride = channelsPerPixel * 2;
	for( int y = 0; y < height; ++y )
	{
		// coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = ( y >> 8 ) & 0xFF;
		*ptr++ = ( y >> 16 ) & 0xFF;
		*ptr++ = ( y >> 24 ) & 0xFF;
		// data size
		*ptr++ = pixelRowSize & 0xFF;
		*ptr++ = ( pixelRowSize >> 8 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 16 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 24 ) & 0xFF;
		// B, G, R
		const unsigned char* chsrc;
		chsrc = src + 4;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 2;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 0;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}

		src += width * stride;
	}

	assert( ptr - buf == bufSize );

	fileSystem->WriteFile( filename, buf, bufSize, basePath );

	Mem_Free( buf );

#else

	// TinyEXR version with compression to save disc size

	if( channelsPerPixel != 3 && channelsPerPixel != 4 )
	{
		common->Error( "R_WriteEXR( %s ): channelsPerPixel = %i not supported", filename, channelsPerPixel );
	}

    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * channelsPerPixel);
        memcpy(ndata, rgba, width * height * channelsPerPixel);
        R_ImageFlipVertical(ndata, width, height, channelsPerPixel);
        rgba = ndata;
    }

    const bool HasAlpha = channelsPerPixel == 4;

	EXRHeader header;
	InitEXRHeader( &header );

	EXRImage image;
	InitEXRImage( &image );

	image.num_channels = channelsPerPixel;

	idList<float> images[4];
    for (int i = 0; i < channelsPerPixel; i++)
	    images[i].Resize( width * height );

	for( int i = 0; i < width * height; i++ )
	{
        for (int m = 0; m < channelsPerPixel; m++)
            images[m][i] = ( float(rgba[channelsPerPixel * i + m]) / 255.0f );
	}

	float* image_ptr[4];
	image_ptr[0] = &( images[2].operator[]( 0 ) ); // B
	image_ptr[1] = &( images[1].operator[]( 0 ) ); // G
	image_ptr[2] = &( images[0].operator[]( 0 ) ); // R
    if(HasAlpha)
        image_ptr[3] = &( images[3].operator[]( 0 ) ); // A

	image.images = ( unsigned char** )image_ptr;
	image.width = width;
	image.height = height;

	header.num_channels = channelsPerPixel;
	header.channels = ( EXRChannelInfo* )malloc( sizeof( EXRChannelInfo ) * header.num_channels );

	// Must be BGR(A) order, since most of EXR viewers expect this channel order.
	strncpy( header.channels[0].name, "B", 255 );
	header.channels[0].name[strlen( "B" )] = '\0';
	strncpy( header.channels[1].name, "G", 255 );
	header.channels[1].name[strlen( "G" )] = '\0';
	strncpy( header.channels[2].name, "R", 255 );
	header.channels[2].name[strlen( "R" )] = '\0';
    if(HasAlpha)
    {
        strncpy(header.channels[3].name, "A", 255);
        header.channels[3].name[strlen("A")] = '\0';
    }

	header.pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	header.requested_pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	for( int i = 0; i < header.num_channels; i++ )
	{
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
	}

	header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;

	byte* buffer = NULL;
	const char* err;
	size_t size = SaveEXRImageToMemory( &image, &header, &buffer, &err );
	if( size == 0 )
	{
		common->Error( "R_WriteEXR( %s ): Save EXR err: %s\n", filename, err );

		goto cleanup;
	}

    if(!basePath)
        basePath = "fs_savepath";
	fileSystem->WriteFile( filename, buffer, size, basePath );

cleanup:
	free( header.channels );
	free( header.pixel_types );
	free( header.requested_pixel_types );
    free(ndata);

#endif
}
// RB end

void R_WriteDDS(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    byte *ndata = NULL;
    if(flipVertical)
    {
        ndata = (byte *) malloc(width * height * comp);
        memcpy(ndata, data, width * height * comp);
        R_ImageFlipVertical(ndata, width, height, comp);
        data = ndata;
    }
    if(!basePath)
        basePath = "fs_savepath";
    idFile *f = fileSystem->OpenFileWrite(filename, basePath);
    if(f)
    {
        int r = soil_save_image_as_func(R_StbWriteImageFile, (void *)f, width, height, comp, data);
        if(!r)
            common->Warning("R_WriteDDS fail: %d", r);
        fileSystem->CloseFile(f);
    }
    free(ndata);
}

void R_WriteScreenshotImage(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath)
{
    idStr fn(filename);
    switch(r_screenshotFormat.GetInteger())
    {
        case SSFE_BMP: {// bmp
            fn.SetFileExtension("bmp");
            R_WriteBMP(fn.c_str(), data, width, height, comp, flipVertical,  basePath);
        }
            break;
        case SSFE_PNG: {
            fn.SetFileExtension("png");
            R_WritePNG(fn.c_str(), data, width, height, comp, flipVertical, idMath::ClampInt(0, 9, r_screenshotPngCompression.GetInteger()), basePath);
        }
            break;
        case SSFE_JPG: {
            fn.SetFileExtension("jpg");
            R_WriteJPG(fn.c_str(), data, width, height, comp, flipVertical, idMath::ClampInt(1, 100, r_screenshotJpgQuality.GetInteger()), basePath);
        }
            break;
        case SSFE_DDS: {
            fn.SetFileExtension("dds");
            R_WriteDDS(fn.c_str(), data, width, height, comp, flipVertical, basePath);
        }
            break;
        case SSFE_EXR: {
            fn.SetFileExtension("exr");
            R_WriteEXR(fn.c_str(), data, width, height, comp, flipVertical, basePath);
        }
            break;
        case SSFE_TGA:
        default:
            fn.SetFileExtension("tga");
            R_WriteTGA(fn.c_str(), data, width, height, flipVertical); //TODO: 4 comp
            break;
    }
}

// Convert image
bool R_ConvertImage(const char *filename, const char *toFormat, idStr &ret, int comp = 4, int compression = -1, bool flipVertical = false, bool makePowerOf2 = false, const char *basePath = NULL, int *rwidth = NULL, int *rheight = NULL)
{
    idStr fileNameStr(filename);
    idStr srcExt;
    int width = 0, height = 0;
    byte *pic = NULL;
    ID_TIME_T timestamp;
    bool res;

    if(rwidth)
        *rwidth = -1;
    if(rheight)
        *rheight = -1;

    // PreCheck
    fileNameStr.ExtractFileExtension(srcExt);
    if(srcExt.IsEmpty())
    {
        ret = idStr("Can not get source image extension: ") + filename;
        return false;
    }
    if(!idStr::Icmp(srcExt, toFormat))
    {
        ret = idStr("Source image extension same as target: ") + srcExt;
        return false;
    }

    // Load
    if(!idStr::Icmp(srcExt, "tga"))
    {
        LoadTGA(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "jpg") || !idStr::Icmp(srcExt, "jpeg") )
    {
        LoadJPG(filename, &pic, &width, &height, &timestamp);
    }
    else if (!idStr::Icmp(srcExt, "pcx"))
    {
        LoadPCX32(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "png"))
    {
        LoadPNG(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "dds"))
    {
        LoadDDS(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "bmp"))
    {
        LoadBMP(filename, &pic, &width, &height, &timestamp);
    }
    else if(!idStr::Icmp(srcExt, "exr"))
    {
        LoadEXR(filename, &pic, &width, &height, &timestamp);
    }
    else
    {
        ret = idStr("Unsupport source image format: ") + srcExt;
        return false;
    }

    // Check load
    if ((width < 1) || (height < 1)) {
        if (pic) {
            R_StaticFree(pic);
            ret = idStr("Unable get source image size");
            return false;
        }
    }

    if ((!pic) || (timestamp == -1)) {
        if (pic) {
            R_StaticFree(pic);
        }
        ret = idStr("Unable get source image data");
        return false;
    }

    //
    // convert to exact power of 2 sizes
    //
    if (pic && makePowerOf2) {
        int		w, h;
        int		scaled_width, scaled_height;
        byte	*resampledBuffer;

        w = width;
        h = height;

        for (scaled_width = 1 ; scaled_width < w ; scaled_width<<=1)
            ;

        for (scaled_height = 1 ; scaled_height < h ; scaled_height<<=1)
            ;

        if (scaled_width != w || scaled_height != h) {
            if (globalImages->image_roundDown.GetBool() && scaled_width > w) {
                scaled_width >>= 1;
            }

            if (globalImages->image_roundDown.GetBool() && scaled_height > h) {
                scaled_height >>= 1;
            }

            resampledBuffer = R_ResampleTexture(pic, w, h, scaled_width, scaled_height);
            R_StaticFree(pic);
            pic = resampledBuffer;
            width = scaled_width;
            height = scaled_height;
        }
    }

    // Save
    idStr targetPath;
    if(basePath && basePath[0])
    {
        targetPath = basePath;
        targetPath.AppendPath(filename);
    }
    else
    {
        targetPath = filename;
    }
    targetPath.SetFileExtension(toFormat);
    res = false;

    if(!idStr::Icmp(toFormat, "tga"))
    {
        R_WriteTGA(targetPath.c_str(), pic, width, height, flipVertical);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "jpg") || !idStr::Icmp(toFormat, "jpeg") )
    {
        if(compression < 0 || compression > 100)
            compression = 100;
        R_WriteJPG(targetPath.c_str(), pic, width, height, comp, flipVertical, compression);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "png"))
    {
        if(compression < 0 || compression > 9)
            compression = 9;
        R_WritePNG(targetPath.c_str(), pic, width, height, comp, flipVertical, compression);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "dds"))
    {
        R_WriteDDS(targetPath.c_str(), pic, width, height, comp, flipVertical);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "bmp"))
    {
        R_WriteBMP(targetPath.c_str(), pic, width, height, comp, flipVertical);
        res = true;
    }
    else if(!idStr::Icmp(toFormat, "exr"))
    {
        R_WriteEXR(targetPath.c_str(), pic, width, height, comp, flipVertical);
        res = true;
    }
    else
    {
        ret = idStr("Unsupport target image format: ") + toFormat;
    }

    // Free
    if (pic)
        R_StaticFree(pic);
    if(res)
    {
        ret = targetPath;
        if(rwidth)
            *rwidth = width;
        if(rheight)
            *rheight = height;
    }

    return true;
}

// Command functions
/*
 * convertImage <src_image> <dest_format> [
 * 	--directory=<save_directory>
 * 	-d <save_directory>
 *
 * 	--flipVertical
 * 	-f
 *
 * 	--compression=<jpg_compression | png_quality>
 * 	-q <jpg_compression | png_quality>
 *
 * 	--makePowerOf2
 * 	-p
 *
 * 	--component=<component>
 * 	-c <component>
 * ]
 */
void R_ConvertImage_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf("Usage: convertImage <src_image> <dest_format> [\n"
                       "\t-d <save_directory> --directory=<save_directory> \n"
                       "\t-f --flipVertical \n"
                       "\t-q <jpg_compression | png_quality> --compression=<jpg_compression | png_quality> \n"
                       "\t-c <component> --component=<component> \n"
                       "\t-p --makePowerOf2 \n"
                       "]");
        return;
    }

    const char *filename = args.Argv(1);
    const char *toFormat = args.Argv(2);
    bool flipVertical = false;
    bool makePowerOf2 = false;
    int compression = -1;
    int component = 4;
    idStr basePath;

    int width;
    int height;
    idStr ret;

    for(int i = 3; i < args.Argc(); i++)
    {
        idStr arg = args.Argv(i);
        int argType = 0;
        if(!arg.Cmp("-"))
        {
            i++;
            if(i >= args.Argc())
            {
                common->Warning("Missing short argument");
                break;
            }
            arg = args.Argv(i);
            argType = 1;
        }
        else if(!arg.Cmp("--"))
        {
            i++;
            if(i >= args.Argc())
            {
                common->Warning("Missing long argument");
                break;
            }
            arg = args.Argv(i);
            argType = 2;
        }

#define PARSE_LONG_ARG(what) \
		if(argType != 2) \
		{ \
			common->Warning("Required long argument"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing long argument `=` token"); \
			break; \
		} \
		arg = args.Argv(i); \
		if(arg != "=") \
		{ \
			common->Warning("Expected long argument `=` token"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing long argument `" what "` value"); \
			break; \
		} \
		arg = args.Argv(i);

#define PARSE_SHORT_ARG(what) \
		if(argType != 1) \
		{ \
			common->Warning("Required short argument"); \
            continue; \
		} \
		i++; \
		if(i >= args.Argc()) \
		{ \
			common->Warning("Missing short argument `" what "` value"); \
			break; \
		} \
		arg = args.Argv(i);

        if(arg == "directory")
        {
            PARSE_LONG_ARG("directory");
            basePath = arg;
        }
        else if(arg == "d")
        {
            PARSE_SHORT_ARG("d");
            basePath = arg;
        }
        else if(arg == "flipVertical" || arg == "f")
        {
            flipVertical = true;
        }
        else if(arg == "makePowerOf2" || arg == "p")
        {
            makePowerOf2 = true;
        }
        else if(arg == "compression")
        {
            PARSE_LONG_ARG("compression");
            compression = atoi(arg.c_str());
        }
        else if(arg == "q")
        {
            PARSE_SHORT_ARG("q");
            compression = atoi(arg.c_str());
        }
        else if(arg == "component")
        {
            PARSE_LONG_ARG("component");
            component = atoi(arg.c_str());
        }
        else if(!arg.Cmp("c"))
        {
            PARSE_SHORT_ARG("c");
            component = atoi(arg.c_str());
        }
        else
        {
            common->Warning("Unknown argument: %s", arg.c_str());
        }
    }

#undef PARSE_LONG_ARG
#undef PARSE_SHORT_ARG

    common->Printf("ConvertImage: %s -> %s\n", filename, toFormat);
    common->Printf("\tdirectory: %s\n", basePath.c_str());
    common->Printf("\tflip vertical: %s\n", flipVertical ? "ON" : "OFF");
    common->Printf("\tmake power of 2: %s\n", makePowerOf2 ? "ON" : "OFF");
    common->Printf("\tcompression: %d\n", compression);
    common->Printf("\tcomponent: %d\n", component);

    if(!idStr::Icmp(toFormat, "png"))
    {
        if(compression < 0 || compression > 9)
            compression = 9;
    }
    else if(!idStr::Icmp(toFormat, "jpg") || !idStr::Icmp(toFormat, "jpeg"))
    {
        if(compression < 0 || compression > 100)
            compression = 100;
    }
    if(component != 3 && component != 4)
        component = 4;

    if(R_ConvertImage(filename, toFormat, ret, component, compression, flipVertical, makePowerOf2, basePath, &width, &height))
    {
        common->Printf("Target image save to %s(%d x %d)\n", ret.c_str(), width, height);
    }
    else
    {
        common->Printf("Convert error: %s\n", ret.c_str());
    }
}

void R_Image_AddCommand(void)
{
	cmdSystem->AddCommand("convertImage", R_ConvertImage_f, CMD_FL_RENDERER, "convert image format", idCmdSystem::ArgCompletion_ImageName);
    cmdSystem->AddCommand("extractBimage", R_ExtractBImage_f, CMD_FL_RENDERER, "extract DOOM3-BFG's bimage image");
}
