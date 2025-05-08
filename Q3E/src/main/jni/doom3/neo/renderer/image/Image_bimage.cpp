/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2021 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../idlib/precompiled.h"
#pragma hdrstop
#include "Image_bimage.h"
#include "../DXT/DXTCodec.h"

/*
================================================================================================

	idBinaryImage

================================================================================================
*/

#define WriteBig(x) WriteInt( BigLong(x) )
#define Big(x) x = BigLong(x)

void Bimage_GetGeneratedFileName( idStr& gfn, const char* name )
{
	gfn = "generated/images";
	gfn.AppendPath(name);
	gfn.StripFileExtension();
	gfn.Append("#__0400.bimage");
}

/*
========================
idBinaryImage::WriteGeneratedFile
========================
*/
bool Image_WriteGeneratedFile( const char *binaryFileName, const bimageFile_t &fileData )
{
	idFile *file = fileSystem->OpenFileWrite( binaryFileName, "fs_basepath" );
	if( file == NULL )
	{
		idLib::Warning( "idBinaryImage: Could not open file '%s'", binaryFileName );
		return false;
	}
	Sys_Printf( "Writing %s: %ix%i\n", binaryFileName, fileData.width, fileData.height );

	// fileData.headerMagic = BIMAGE_MAGIC;
	// fileData.sourceFileTime = Sys_Milliseconds();

	file->WriteBig( fileData.sourceFileTime );
	file->WriteBig( fileData.headerMagic );
	file->WriteBig( fileData.textureType );
	file->WriteBig( fileData.format );
	file->WriteBig( fileData.colorFormat );
	file->WriteBig( fileData.width );
	file->WriteBig( fileData.height );
	file->WriteBig( fileData.numLevels );

	for( int i = 0; i < fileData.numLevels; i++ )
	{
		bimageImage_t& img = fileData.images[ i ];
		file->WriteBig( img.level );
		file->WriteBig( img.destZ );
		file->WriteBig( img.width );
		file->WriteBig( img.height );
		file->WriteBig( img.dataSize );
		file->Write( img.data, img.dataSize );
	}
	fileSystem->CloseFile(file);
	return true;
}

/*
==========================
idBinaryImage::LoadFromGeneratedFile

Load the preprocessed image from the generated folder.
==========================
*/
bool Image_LoadFromGeneratedFile( const char *binaryFileName, bimageFile_t &fileData )
{
	idFile *bFile = fileSystem->OpenFileRead( binaryFileName );
	if(!bFile)
		return false;
	if( bFile->Read( &fileData, sizeof( fileData ) - sizeof(fileData.images) ) <= 0 )
	{
		return false;
	}
	Big( fileData.sourceFileTime );
	Big( fileData.headerMagic );
	Big( fileData.textureType );
	Big( fileData.format );
	Big( fileData.colorFormat );
	Big( fileData.width );
	Big( fileData.height );
	Big( fileData.numLevels );

	if( BIMAGE_MAGIC != fileData.headerMagic )
	{
		return false;
	}

	int numImages = fileData.numLevels;
	if( fileData.textureType == TT_CUBIC )
	{
		numImages *= 6;
	}

	bimageImage_t *images = (bimageImage_t *)calloc( numImages, sizeof(images[0]) );
	bool hasError = false;

	for( int i = 0; i < numImages; i++ )
	{
		bimageImage_t& img = images[ i ];
		if( bFile->Read( &img, sizeof( img ) - sizeof(img.data) ) <= 0 )
		{
			hasError = true;
			break;
		}
		Big( img.level );
		Big( img.destZ );
		Big( img.width );
		Big( img.height );
		Big( img.dataSize );
		assert( img.level >= 0 && img.level < fileData.numLevels );
		assert( img.destZ == 0 || fileData.textureType == TT_CUBIC );
		assert( img.dataSize > 0 );
		// DXT images need to be padded to 4x4 block sizes, but the original image
		// sizes are still retained, so the stored data size may be larger than
		// just the multiplication of dimensions
		assert( img.dataSize >= img.width * img.height * BitsForFormat( ( textureFormat_t )fileData.format ) / 8 );
#if 1 //karin: force convert light texture format from RGB565 to RGBA8888
		int imgfile_dataSize = img.dataSize;
		// SRS - Allocate 2x memory to prepare for in-place conversion from FMT_RGB565 to FMT_RGBA8
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			img.data = (byte *)calloc(img.dataSize, 2);
		}
		else
		{
			img.data = (byte *)calloc(img.dataSize, 1);
		}
		if( img.data == NULL )
		{
			hasError = true;
			break;
		}

		// SRS - Read image data using actual on-disk data size
		if( bFile->Read( img.data, imgfile_dataSize ) <= 0 )
		{
			hasError = true;
			break;
		}

		// SRS - Convert FMT_RGB565 16-bits to FMT_RGBA8 32-bits in place using pre-allocated space
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			//SRS - Make sure we have an integer number of RGBA8 storage slots
			assert( img.dataSize % 4 == 0 );
			for( int pixelIndex = img.dataSize / 2 - 2; pixelIndex >= 0; pixelIndex -= 2 )
			{
#if 1
				// SRS - Option 1: Scale and shift algorithm
				uint16_t pixelValue_rgb565 = img.data[pixelIndex + 0] << 8 | img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( ( ( pixelValue_rgb565 ) >> 11 ) * 527 + 23 ) >> 6;
				img.data[pixelIndex * 2 + 1] = ( ( ( pixelValue_rgb565 & 0x07E0 ) >>  5 ) * 259 + 33 ) >> 6;
				img.data[pixelIndex * 2 + 2] = ( ( ( pixelValue_rgb565 & 0x001F ) ) * 527 + 23 ) >> 6;
#else
				// SRS - Option 2: Shift and combine algorithm - is this faster?
				uint8 pixelValue_rgb565_hi = img.data[pixelIndex + 0];
				uint8 pixelValue_rgb565_lo = img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( pixelValue_rgb565_hi & 0xF8 ) | ( pixelValue_rgb565_hi          >> 5 );
				img.data[pixelIndex * 2 + 1] = ( pixelValue_rgb565_hi        << 5 ) | ( ( pixelValue_rgb565_lo & 0xE0 ) >> 3 ) | ( ( pixelValue_rgb565_hi & 0x07 ) >> 1 );
				img.data[pixelIndex * 2 + 2] = ( pixelValue_rgb565_lo        << 3 ) | ( ( pixelValue_rgb565_lo & 0x1F ) >> 2 );
#endif
				img.data[pixelIndex * 2 + 3] = 0xFF;
			}
		}
		else if( ( textureFormat_t )fileData.format == FMT_DXT1 )
		{
			idDxtDecoder decoder;
			byte* decodedImageData = (byte *)calloc(img.width * img.height * 4, 1);
			decoder.DecompressImageDXT1(img.data, decodedImageData, img.width, img.height);
			free(img.data);
			img.data = decodedImageData;
		}
		else if( ( textureFormat_t )fileData.format == FMT_DXT5 )
		{
			idDxtDecoder decoder;
			byte* decodedImageData = (byte *)calloc(img.width * img.height * 4, 1);
			decoder.DecompressImageDXT5(img.data, decodedImageData, img.width, img.height);
			free(img.data);
			img.data = decodedImageData;
		}
#else
		img.data = (byte *)calloc(img.dataSize, 1);
		if( img.data == NULL )
		{
			hasError = true;
			break;
		}

		if( bFile->Read( img.data, img.dataSize ) <= 0 )
		{
			hasError = true;
			break;
		}
#endif
	}

	fileSystem->CloseFile(bFile);

	if(hasError)
	{
		for( int i = 0; i < numImages; i++ )
		{
			if(images[ i ].data)
				free(images[i].data);
		}
		free(images);
		return false;
	}

	fileData.images = images;
	return true;
}

void LoadBimage(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp)
{
    if (pic) {
        *pic = NULL;		// until proven otherwise
    }

	idStr binaryFileName;
	Bimage_GetGeneratedFileName(binaryFileName, filename);
	idFile *f = fileSystem->OpenFileRead( binaryFileName.c_str() );
	if(!f)
		return;

	if (timestamp) {
		*timestamp = f->Timestamp();
	}
	if (!pic) {
		fileSystem->CloseFile(f);
		return;	// just getting timestamp
	}

	bimageFile_t fileData;
	if( f->Read( &fileData, sizeof( fileData ) - sizeof(fileData.images) ) <= 0 )
	{
		fileSystem->CloseFile(f);
		return;
	}
	Big( fileData.sourceFileTime );
	Big( fileData.headerMagic );
	Big( fileData.textureType );
	Big( fileData.format );
	Big( fileData.colorFormat );
	Big( fileData.width );
	Big( fileData.height );
	Big( fileData.numLevels );

	if( BIMAGE_MAGIC != fileData.headerMagic )
	{
		fileSystem->CloseFile(f);
		return;
	}

	int numImages = fileData.numLevels;
	if( fileData.textureType == TT_CUBIC )
	{
		fileSystem->CloseFile(f);
		return;
	}

	bimageImage_t img;
	int i;

	for( i = 0; i < numImages; i++ )
	{
		if( f->Read( &img, sizeof( img ) - sizeof(img.data) ) <= 0 )
		{
			fileSystem->CloseFile(f);
			return;
		}
		Big( img.level );
		Big( img.destZ );
		Big( img.width );
		Big( img.height );
		Big( img.dataSize );
		if(img.level != 0)
		{
			f->Seek(img.dataSize, FS_SEEK_CUR);
			continue;
		}
		assert( img.level >= 0 && img.level < fileData.numLevels );
		assert( img.destZ == 0 || fileData.textureType == TT_CUBIC );
		assert( img.dataSize > 0 );
		// DXT images need to be padded to 4x4 block sizes, but the original image
		// sizes are still retained, so the stored data size may be larger than
		// just the multiplication of dimensions
		assert( img.dataSize >= img.width * img.height * BitsForFormat( ( textureFormat_t )fileData.format ) / 8 );

		int imgfile_dataSize = img.dataSize;
		// SRS - Allocate 2x memory to prepare for in-place conversion from FMT_RGB565 to FMT_RGBA8
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			img.data = (byte *)calloc(img.dataSize, 2);
		}
		else
		{
			img.data = (byte *)calloc(img.dataSize, 1);
		}
		if( img.data == NULL )
		{
			fileSystem->CloseFile(f);
			return;
		}

		// SRS - Read image data using actual on-disk data size
		if( f->Read( img.data, imgfile_dataSize ) <= 0 )
		{
			free(img.data);
			fileSystem->CloseFile(f);
			return;
		}

		// SRS - Convert FMT_RGB565 16-bits to FMT_RGBA8 32-bits in place using pre-allocated space
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			//SRS - Make sure we have an integer number of RGBA8 storage slots
			assert( img.dataSize % 4 == 0 );
			for( int pixelIndex = img.dataSize / 2 - 2; pixelIndex >= 0; pixelIndex -= 2 )
			{
#if 1
				// SRS - Option 1: Scale and shift algorithm
				uint16_t pixelValue_rgb565 = img.data[pixelIndex + 0] << 8 | img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( ( ( pixelValue_rgb565 ) >> 11 ) * 527 + 23 ) >> 6;
				img.data[pixelIndex * 2 + 1] = ( ( ( pixelValue_rgb565 & 0x07E0 ) >>  5 ) * 259 + 33 ) >> 6;
				img.data[pixelIndex * 2 + 2] = ( ( ( pixelValue_rgb565 & 0x001F ) ) * 527 + 23 ) >> 6;
#else
				// SRS - Option 2: Shift and combine algorithm - is this faster?
				uint8 pixelValue_rgb565_hi = img.data[pixelIndex + 0];
				uint8 pixelValue_rgb565_lo = img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( pixelValue_rgb565_hi & 0xF8 ) | ( pixelValue_rgb565_hi          >> 5 );
				img.data[pixelIndex * 2 + 1] = ( pixelValue_rgb565_hi        << 5 ) | ( ( pixelValue_rgb565_lo & 0xE0 ) >> 3 ) | ( ( pixelValue_rgb565_hi & 0x07 ) >> 1 );
				img.data[pixelIndex * 2 + 2] = ( pixelValue_rgb565_lo        << 3 ) | ( ( pixelValue_rgb565_lo & 0x1F ) >> 2 );
#endif
				img.data[pixelIndex * 2 + 3] = 0xFF;
			}
		}
		else if( ( textureFormat_t )fileData.format == FMT_DXT1 )
		{
			int size = img.width * img.height * 4;
			idDxtDecoder decoder;
			byte* decodedImageData = (byte *)calloc(size, 1);
			decoder.DecompressImageDXT1(img.data, decodedImageData, img.width, img.height);
			free(img.data);
			img.data = decodedImageData;
		}
		else if( ( textureFormat_t )fileData.format == FMT_DXT5 )
		{
			idDxtDecoder decoder;
			byte* decodedImageData = (byte *)calloc(img.width * img.height * 4, 1);
			decoder.DecompressImageDXT5(img.data, decodedImageData, img.width, img.height);
			free(img.data);
			img.data = decodedImageData;
		}
		else
		{
			free(img.data);
			return;
		}

		int size = img.width * img.height * 4;

		if(fileData.colorFormat == CFM_GREEN_ALPHA ) // e.g. newfonts file
		{
			// copy green component to alpha component
			for(int m = 0; m < size; m += 4)
			{
				img.data[m + 3] = img.data[m + 1];
				img.data[m] = 255;
				img.data[m + 1] = 255;
				img.data[m + 2] = 255;
			}
		}

		*pic = (byte *)R_StaticAlloc( size );
		memcpy( *pic, img.data, size );
		*width = img.width;
		*height = img.height;
		free(img.data);

		break;
	}

	fileSystem->CloseFile(f);
}

bool R_GetBimageSize(const char *filename, int &width, int &height)
{
	idStr binaryFileName;
	Bimage_GetGeneratedFileName(binaryFileName, filename);
	idFile *f = fileSystem->OpenFileRead( binaryFileName.c_str() );
	if(!f)
		return false;

	bimageFile_t fileData;
	if( f->Read( &fileData, sizeof( fileData ) - sizeof(fileData.images) ) <= 0 )
	{
		fileSystem->CloseFile(f);
		return false;
	}
	Big( fileData.sourceFileTime );
	Big( fileData.headerMagic );
	Big( fileData.textureType );
	Big( fileData.format );
	Big( fileData.colorFormat );
	Big( fileData.width );
	Big( fileData.height );
	Big( fileData.numLevels );

	if( BIMAGE_MAGIC != fileData.headerMagic )
	{
		fileSystem->CloseFile(f);
		return false;
	}

	int numImages = fileData.numLevels;
	if( fileData.textureType == TT_CUBIC )
	{
		fileSystem->CloseFile(f);
		return false;
	}

	bimageImage_t img;
	int i;

	for( i = 0; i < numImages; i++ )
	{
		if( f->Read( &img, sizeof( img ) - sizeof(img.data) ) <= 0 )
		{
			fileSystem->CloseFile(f);
			return false;
		}
		Big( img.level );
		Big( img.destZ );
		Big( img.width );
		Big( img.height );
		Big( img.dataSize );
		if(img.level != 0)
		{
			f->Seek(img.dataSize, FS_SEEK_CUR);
			continue;
		}
		assert( img.level >= 0 && img.level < fileData.numLevels );
		assert( img.destZ == 0 || fileData.textureType == TT_CUBIC );
		assert( img.dataSize > 0 );
		// DXT images need to be padded to 4x4 block sizes, but the original image
		// sizes are still retained, so the stored data size may be larger than
		// just the multiplication of dimensions
		assert( img.dataSize >= img.width * img.height * BitsForFormat( ( textureFormat_t )fileData.format ) / 8 );

		width = img.width;
		height = img.height;
		break;
	}

	fileSystem->CloseFile(f);

	return i < numImages;
}

void R_ExtractBImage_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf("[Usage]: %s <bimage file> [<output font folder name. default to base folder']\n", args.Argv(0));
        return;
    }

    const char *bimagePath = args.Argv(1);
    const char *outPath = args.Argc() > 2 ? args.Argv(2) : NULL;

    if(outPath && outPath[0] == '\0')
        outPath = NULL;

    common->Printf("Extract bimage file(bimage=%s, out=%s)......\n", bimagePath, outPath ? outPath : "");

    bimageFile_t bimage;
    if(!Image_LoadFromGeneratedFile(bimagePath, bimage))
    {
        common->Warning("Extract bimage fail.");
        return;
    }

    common->Printf("bimage: timestamp=%ld, magic=%ud, type=%d, format=%d, color=%d, size=%d x %d images=%d\n", bimage.sourceFileTime, bimage.headerMagic, bimage.textureType, bimage.format, bimage.colorFormat, bimage.width, bimage.height, bimage.numLevels);

    idStr filePath;
    int headerLen = strlen("generated/images");
    if(!idStr::Icmpn(bimagePath, "generated/images", headerLen))
        filePath.AppendPath(bimagePath + headerLen);
    else
        filePath.AppendPath(bimagePath);
    idStr filename;
    filePath.ExtractFileName(filename);
    filename.StripFileExtension();
    int index = filename.Find('#');
    if(index != -1)
        filename = filename.Left(index);

    filePath.StripFilename();

    idStr dp;
    if(outPath && outPath[0])
        dp.AppendPath(outPath);
    dp.AppendPath(filePath);

    for( int i = 0; i < bimage.numLevels; i++ )
    {
        bimageImage_t &img = bimage.images[i];
        common->Printf("bimage layer %d: level=%d, size=%d x %d x %d size=%d\n", i, img.level, img.width, img.height, img.destZ, img.dataSize);

        idStr fn(dp);
        fn.AppendPath(filename);
        if(i > 0)
            fn.Append(va("_%d", img.level));
        fn.SetFileExtension(".tga");

        common->Printf("Extract bimage layer %d: '%s'\n", i, fn.c_str());
        R_WriteTGA(fn.c_str(), img.data, img.width, img.height, false);
    }

    common->Printf("Extract bimage file done(bimage=%s, out=%s)......\n", bimagePath, outPath ? outPath : "");
}
