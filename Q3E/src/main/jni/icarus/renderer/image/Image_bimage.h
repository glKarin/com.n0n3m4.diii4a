/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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
#ifndef __BINARYIMAGEDATA_H__
#define __BINARYIMAGEDATA_H__

/*
================================================================================================

This is where the Binary image headers go that are also included by external tools such as the cloud.

================================================================================================
*/

// These structures are used for memory mapping bimage files, but
// not for the normal loading, so be careful making changes.
// Values are big endien to reduce effort on consoles.
#define BIMAGE_VERSION 10
#define BIMAGE_MAGIC (unsigned int)( ('B'<<0)|('I'<<8)|('M'<<16)|(BIMAGE_VERSION<<24) )

enum textureFormat_t
{
	FMT_NONE,

	//------------------------
	// Standard color image formats
	//------------------------

	FMT_RGBA8,			// 32 bpp
	FMT_XRGB8,			// 32 bpp

	//------------------------
	// Alpha channel only
	//------------------------

	// Alpha ends up being the same as L8A8 in our current implementation, because straight
	// alpha gives 0 for color, but we want 1.
	FMT_ALPHA,

	//------------------------
	// Luminance replicates the value across RGB with a constant A of 255
	// Intensity replicates the value across RGBA
	//------------------------

	FMT_L8A8,			// 16 bpp
	FMT_LUM8,			//  8 bpp
	FMT_INT8,			//  8 bpp

	//------------------------
	// Compressed texture formats
	//------------------------

	FMT_DXT1,			// 4 bpp
	FMT_DXT5,			// 8 bpp

	//------------------------
	// Depth buffer formats
	//------------------------

	FMT_DEPTH,			// 24 bpp

	//------------------------
	//
	//------------------------

	FMT_X16,			// 16 bpp
	FMT_Y16_X16,		// 32 bpp
	FMT_RGB565,			// 16 bpp

#if 0
	// RB: don't change above for legacy .bimage compatibility
	FMT_ETC1_RGB8_OES,	// 4 bpp
	FMT_SHADOW_ARRAY,	// 32 bpp * 6
	FMT_RG16F,			// 32 bpp
	FMT_RGBA16F,		// 64 bpp
	FMT_RGBA32F,		// 128 bpp
	FMT_R32F,			// 32 bpp
	FMT_R11G11B10F,		// 32 bpp
	FMT_R8,
	FMT_DEPTH_STENCIL,  // 32 bpp
	// RB end
#endif
};

enum textureColor_t
{
	CFM_DEFAULT,			// RGBA
	CFM_NORMAL_DXT5,		// XY format and use the fast DXT5 compressor
	CFM_YCOCG_DXT5,			// convert RGBA to CoCg_Y format
	CFM_GREEN_ALPHA,		// Copy the alpha channel to green

#if 0
	// RB: don't change above for legacy .bimage compatibility
	CFM_YCOCG_RGBA8,
	// RB end
#endif
};


#pragma pack( push, 1 )
struct bimageImage_t
{
	int		level;		// mip
	int		destZ;		// array slice
	int		width;
	int		height;
	int		dataSize;	// dataSize bytes follow

	byte* data;
};

struct bimageFile_t
{
	ID_TIME_T	sourceFileTime;
	int		headerMagic;
	int		textureType;
	int		format;
	int		colorFormat;
	int		width;
	int		height;
	int		numLevels;
	// one or more bimageImage_t structures follow
	
	bimageImage_t *images;
};
#pragma pack( pop )

bool Image_LoadFromGeneratedFile( const char *binaryFileName, bimageFile_t &fileData );
bool Image_WriteGeneratedFile( const char *binaryFileName, const bimageFile_t *fileData );

#endif // __BINARYIMAGEDATA_H__

