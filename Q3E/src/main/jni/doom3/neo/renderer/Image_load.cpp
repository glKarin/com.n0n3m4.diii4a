/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*
PROBLEM: compressed textures may break the zero clamp rule!
*/

static bool FormatIsDXT(int internalFormat)
{
	if (internalFormat < GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	    || internalFormat > GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
		return false;
	}

	return true;
}

int MakePowerOfTwo(int num)
{
	int		pot;

	for (pot = 1 ; pot < num ; pot<<=1) {
	}

	return pot;
}

/*
================
BitsForInternalFormat

Used for determining memory utilization
================
*/
int idImage::BitsForInternalFormat(int internalFormat) const
{
			return 8;
}

/*
==================
UploadCompressedNormalMap

Create a 256 color palette to be used by compressed normal maps
==================
*/
void idImage::UploadCompressedNormalMap(int width, int height, const byte *rgba, int mipLevel)
{
	byte	*normals;
	const byte	*in;
	byte	*out;
	int		i, j;
	int		x, y, z;
	int		row;

	// OpenGL's pixel packing rule
	row = width < 4 ? 4 : width;

	normals = (byte *)_alloca(row * height);

	if (!normals) {
		common->Error("R_UploadCompressedNormalMap: _alloca failed");
	}

	in = rgba;
	out = normals;

	for (i = 0 ; i < height ; i++, out += row, in += width * 4) {
		for (j = 0 ; j < width ; j++) {
			x = in[ j * 4 + 0 ];
			y = in[ j * 4 + 1 ];
			z = in[ j * 4 + 2 ];

			int c;

			if (x == 128 && y == 128 && z == 128) {
				// the "nullnormal" color
				c = 255;
			} else {
				c = (globalImages->originalToCompressed[x] << 4) | globalImages->originalToCompressed[y];

				if (c == 255) {
					c = 254;	// don't use the nullnormal color
				}
			}

			out[j] = c;
		}
	}

	if (mipLevel == 0) {
		// Optionally write out the paletized normal map to a .tga
		if (globalImages->image_writeNormalTGAPalletized.GetBool()) {
			char filename[MAX_IMAGE_NAME];
			ImageProgramStringToCompressedFileName(imgName, filename);
			char *ext = strrchr(filename, '.');

			if (ext) {
				strcpy(ext, "_pal.tga");
				R_WritePalTGA(filename, normals, globalImages->compressedPalette, width, height);
			}
		}
	}

#if !defined(GL_ES_VERSION_2_0)
	if (glConfig.sharedTexturePaletteAvailable) {
		qglTexImage2D(GL_TEXTURE_2D,
		              mipLevel,
		              GL_COLOR_INDEX8_EXT,
		              width,
		              height,
		              0,
		              GL_COLOR_INDEX,
		              GL_UNSIGNED_BYTE,
		              normals);
	}
#endif
}


//=======================================================================


static byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};

/*
===============
SelectInternalFormat

This may need to scan six cube map images
===============
*/
GLenum idImage::SelectInternalFormat(const byte **dataPtrs, int numDataPtrs, int width, int height,
                                     textureDepth_t minimumDepth) const
{
	return GL_RGBA8;	// four bytes
}

/*
==================
SetImageFilterAndRepeat
==================
*/
void idImage::SetImageFilterAndRepeat() const
{
	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture filter");
	}

#if !defined(GL_ES_VERSION_2_0)
	if (glConfig.anisotropicAvailable) {
		// only do aniso filtering on mip mapped images
		if (filter == TF_DEFAULT) {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, globalImages->textureAnisotropy);
		} else {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		}
	}
#endif

#if !defined(GL_ES_VERSION_2_0)
	if (glConfig.textureLODBiasAvailable) {
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS_EXT, globalImages->textureLODBias);
	}
#endif

	// set the wrap/clamp modes
	switch (repeat) {
		case TR_REPEAT:
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		case TR_CLAMP_TO_BORDER:
		case TR_CLAMP_TO_ZERO:
		case TR_CLAMP_TO_ZERO_ALPHA:
		case TR_CLAMP:
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture repeat");
	}
}

/*
================
idImage::Downsize
helper function that takes the current width/height and might make them smaller
================
*/
void idImage::GetDownsize(int &scaled_width, int &scaled_height) const
{
	int size = 0;

	// perform optional picmip operation to save texture memory
	if (depth == TD_SPECULAR && globalImages->image_downSizeSpecular.GetInteger()) {
		size = globalImages->image_downSizeSpecularLimit.GetInteger();

		if (size == 0) {
			size = 64;
		}
	} else if (depth == TD_BUMP && globalImages->image_downSizeBump.GetInteger()) {
		size = globalImages->image_downSizeBumpLimit.GetInteger();

		if (size == 0) {
			size = 64;
		}
	} else if ((allowDownSize || globalImages->image_forceDownSize.GetBool()) && globalImages->image_downSize.GetInteger()) {
		size = globalImages->image_downSizeLimit.GetInteger();

		if (size == 0) {
			size = 256;
		}
	}

	if (size > 0) {
		while (scaled_width > size || scaled_height > size) {
			if (scaled_width > 1) {
				scaled_width >>= 1;
			}

			if (scaled_height > 1) {
				scaled_height >>= 1;
			}
		}
	}

	// clamp to minimum size
	if (scaled_width < 1) {
		scaled_width = 1;
	}

	if (scaled_height < 1) {
		scaled_height = 1;
	}

	// clamp size to the hardware specific upper limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	// This causes a 512*256 texture to sample down to
	// 256*128 on a voodoo3, even though it could be 256*256
	while (scaled_width > glConfig.maxTextureSize
	       || scaled_height > glConfig.maxTextureSize) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}
}

/*
================
GenerateImage

The alpha channel bytes should be 255 if you don't
want the channel.

We need a material characteristic to ask for specific texture modes.

Designed limitations of flexibility:

No support for texture borders.

No support for texture border color.

No support for texture environment colors or GL_BLEND or GL_DECAL
texture environments, because the automatic optimization to single
or dual component textures makes those modes potentially undefined.

No non-power-of-two images.

No palettized textures.

There is no way to specify separate wrap/clamp values for S and T

There is no way to specify explicit mip map levels

================
*/

//Code from raspberrypi q3

static int isopaque(GLint width, GLint height, const GLvoid *pixels)
{
   unsigned char const *cpixels = (unsigned char const *)pixels;
   int i;
   for (i = 0; i < width * height; i++) {
      if (cpixels[i*4+3] != 0xff)
         return 0;
   }
   return 1;
}

void rgba4444_convert_tex_image(
   char* cachefname,
   GLenum target,
   GLint level,
   GLenum internalformat,
   GLsizei width,
   GLsizei height,
   GLint border,
   GLenum format,
   GLenum type,
   const GLvoid *pixels)
{
   unsigned char const *cpixels = (unsigned char const *)pixels;
   unsigned short *rgba4444data = 
	   (unsigned short *)
	   malloc(2*width*height+1);
   ((unsigned char*)rgba4444data)[0]=1;
   rgba4444data=(unsigned short *)((unsigned char*)rgba4444data+1);
   int i;
   for (i = 0; i < width * height; i++) {
      unsigned char r,g,b,a;
      r = cpixels[4*i]>>4;
      g = cpixels[4*i+1]>>4;
      b = cpixels[4*i+2]>>4;
      a = cpixels[4*i+3]>>4;
      rgba4444data[i] = r << 12 | g << 8 | b << 4 | a;
   }
   qglTexImage2D(target, level, format, width, height,border,format,GL_UNSIGNED_SHORT_4_4_4_4,rgba4444data);
   rgba4444data=(unsigned short *)((unsigned char*)rgba4444data-1);
   if (cachefname!=0)
   {
	fileSystem->WriteFile(cachefname, rgba4444data, width*height*2+1);
   }
   free(rgba4444data);
}
//#define USE_RG_ETC1
#ifdef USE_RG_ETC1
#include "etc_rg_etc1.h"
#else
#include "etc/etc1_android.h"
#endif

unsigned int etc1_data_size(unsigned int width, unsigned int height) {
    return (((width + 3) & ~3) * ((height + 3) & ~3)) >> 1;
}

void etc1_compress_tex_image(
   char* cachefname,
   GLenum target,
   GLint level,
   GLenum internalformat,
   GLsizei width,
   GLsizei height,
   GLint border,
   GLenum format,
   GLenum type,
   const GLvoid *pixels)
{
   unsigned char const *cpixels = (unsigned char const *)pixels;
   unsigned char *etc1data;
   unsigned int size=etc1_data_size(width,height);
   etc1data = 
	   (unsigned char *)
	   malloc(size+1);
   etc1data[0]=0;
   etc1data++;
   #ifdef USE_RG_ETC1
   rg_etc1::etc1_encode_image(cpixels, width, height,
        4, width*4, etc1data);
   #else
   etc1_encode_image(cpixels, width, height,
        4, width*4, etc1data);
   #endif
   qglCompressedTexImage2D(
      target,
      level,
      GL_ETC1_RGB8_OES,
      width,
      height,
      0,
      size,
      etc1data);
   etc1data--;
   if (cachefname!=0)
   {
	fileSystem->WriteFile(cachefname, etc1data, size+1);
   }
   free(etc1data);
}

int etcavail(char* cachefname)
{
return (r_useETC1Cache.GetBool())&&(r_useETC1.GetBool())&&(cachefname!=0)&&(fileSystem->ReadFile(cachefname,0,0)!=-1);
}

int uploadetc(char* cachefname,GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type)
{
	char* tmp;
	int failed=0;
	int sz=fileSystem->ReadFile(cachefname,
			(void **)
			&tmp,0);
	if (tmp[0]==0)
	{
	if (sz==etc1_data_size(width,height)+1)
	{
	tmp++;
	qglCompressedTexImage2D(target,level,GL_ETC1_RGB8_OES,width,height,0,etc1_data_size(width,height),tmp);
	}
	else
		failed=1;
	}
	else
	{
	if (sz==width*height*2+1)
	{
	tmp++;
	qglTexImage2D(target,level,format,width,height,border,format,GL_UNSIGNED_SHORT_4_4_4_4,tmp);
	}
	else
		failed=1;
	}
	tmp--;
	fileSystem->FreeFile(tmp);
	return failed;
}

void myglTexImage2D(char* cachefname,GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   static int opaque = 0;
   if (r_useETC1.GetBool() && format == GL_RGBA && type == GL_UNSIGNED_BYTE) {

      if (level == 0)
         opaque = isopaque(width, height, pixels);

      if (!r_useETC1Cache.GetBool())
	cachefname=0;

      if (opaque)
         etc1_compress_tex_image(cachefname,target, level, format, width, height, border, format, type, pixels);
      else
         rgba4444_convert_tex_image(cachefname,target, level, format, width, height, border, format, type, pixels);
   }
   else
   qglTexImage2D(target,level,internalformat,width,height,border,format,type,pixels);
}

//end

void idImage::GenerateImage(const byte *pic, int width, int height,
                            textureFilter_t filterParm, bool allowDownSizeParm,
                            textureRepeat_t repeatParm, textureDepth_t depthParm)
{
	bool	preserveBorder;
	byte		*scaledBuffer;
	int			scaled_width, scaled_height;
	byte		*shrunk;

	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = depthParm;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture

	if (!glConfig.isInitialized) {
		return;
	}

	// don't let mip mapping smear the texture into the clamped border
	if (repeat == TR_CLAMP_TO_ZERO) {
		preserveBorder = true;
	} else {
		preserveBorder = false;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(width);
	scaled_height = MakePowerOfTwo(height);

	if (scaled_width != width || scaled_height != height) {
		common->Error("R_CreateImage: not a power of 2 image");
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	scaledBuffer = NULL;

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat(&pic, 1, width, height, depth);

	// copy or resample data as appropriate for first MIP level
	if ((scaled_width == width) && (scaled_height == height)) {
		// we must copy even if unchanged, because the border zeroing
		// would otherwise modify const data
		scaledBuffer = (byte *)R_StaticAlloc(sizeof(unsigned) * scaled_width * scaled_height);
		memcpy(scaledBuffer, pic, width*height*4);
	} else {
		// resample down as needed (FIXME: this doesn't seem like it resamples anymore!)
		// scaledBuffer = R_ResampleTexture( pic, width, height, width >>= 1, height >>= 1 );
		scaledBuffer = R_MipMap(pic, width, height, preserveBorder);
		width >>= 1;
		height >>= 1;

		if (width < 1) {
			width = 1;
		}

		if (height < 1) {
			height = 1;
		}

		while (width > scaled_width || height > scaled_height) {
			shrunk = R_MipMap(scaledBuffer, width, height, preserveBorder);
			R_StaticFree(scaledBuffer);
			scaledBuffer = shrunk;

			width >>= 1;
			height >>= 1;

			if (width < 1) {
				width = 1;
			}

			if (height < 1) {
				height = 1;
			}
		}

		// one might have shrunk down below the target size
		scaled_width = width;
		scaled_height = height;
	}

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;
	type = TT_2D;

	// zero the border if desired, allowing clamped projection textures
	// even after picmip resampling or careless artists.
	if (repeat == TR_CLAMP_TO_ZERO) {
		byte	rgba[4];

		rgba[0] = rgba[1] = rgba[2] = 0;
		rgba[3] = 255;
		R_SetBorderTexels((byte *)scaledBuffer, width, height, rgba);
	}

	if (repeat == TR_CLAMP_TO_ZERO_ALPHA) {
		byte	rgba[4];

		rgba[0] = rgba[1] = rgba[2] = 255;
		rgba[3] = 0;
		R_SetBorderTexels((byte *)scaledBuffer, width, height, rgba);
	}

	if (generatorFunction == NULL && (depth == TD_BUMP && globalImages->image_writeNormalTGA.GetBool() || depth != TD_BUMP && globalImages->image_writeTGA.GetBool())) {
		// Optionally write out the texture to a .tga
		char filename[MAX_IMAGE_NAME];
		ImageProgramStringToCompressedFileName(imgName, filename);
		char *ext = strrchr(filename, '.');

		if (ext) {
			strcpy(ext, ".tga");
			// swap the red/alpha for the write
			/*
			if ( depth == TD_BUMP ) {
				for ( int i = 0; i < scaled_width * scaled_height * 4; i += 4 ) {
					scaledBuffer[ i ] = scaledBuffer[ i + 3 ];
					scaledBuffer[ i + 3 ] = 0;
				}
			}
			*/
			R_WriteTGA(filename, scaledBuffer, scaled_width, scaled_height, false);

			// put it back
			/*
			if ( depth == TD_BUMP ) {
				for ( int i = 0; i < scaled_width * scaled_height * 4; i += 4 ) {
					scaledBuffer[ i + 3 ] = scaledBuffer[ i ];
					scaledBuffer[ i ] = 0;
				}
			}
			*/
		}
	}

	// swap the red and alpha for rxgb support
	// do this even on tga normal maps so we only have to use
	// one fragment program
	// if the image is precompressed ( either in palletized mode or true rxgb mode )
	// then it is loaded above and the swap never happens here	
	if (depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 1) {
#if 1 //k2023 texture2D().agb
		if(1)
#else
		if (glConfig.textureCompressionAvailable)
#endif
		{
		for (int i = 0; i < scaled_width * scaled_height * 4; i += 4) {
			scaledBuffer[ i + 3 ] = scaledBuffer[ i ];
			scaledBuffer[ i ] = 0;
		}
		}
		else // texture2D().rgb
		{
		for (int i = 0; i < scaled_width * scaled_height * 4; i += 4) {
			scaledBuffer[ i + 3 ] = 255;
		}
		}
	}

	// upload the main image level
	Bind();


#if !defined(GL_ES_VERSION_2_0)
	if (internalFormat == GL_COLOR_INDEX8_EXT) {
		/*
		if ( depth == TD_BUMP ) {
			for ( int i = 0; i < scaled_width * scaled_height * 4; i += 4 ) {
				scaledBuffer[ i ] = scaledBuffer[ i + 3 ];
				scaledBuffer[ i + 3 ] = 0;
			}
		}
		*/
		UploadCompressedNormalMap(scaled_width, scaled_height, scaledBuffer, 0);
	} else
#endif
	{
		char filename[MAX_IMAGE_NAME];
		char*fptr=&filename[0];
		ImageProgramStringToCompressedFileName(imgName, filename);
		char *ext = strrchr(filename, '.');
		if (ext) {
		strcpy(ext, ".etc");
		}
		else
		fptr=0;
		myglTexImage2D(fptr,GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
	}

	// create and upload the mip map levels, which we do in all cases, even if we don't think they are needed
	int		miplevel;

	miplevel = 0;

	while (scaled_width > 1 || scaled_height > 1) {
		// preserve the border after mip map unless repeating
		shrunk = R_MipMap(scaledBuffer, scaled_width, scaled_height, preserveBorder);
		R_StaticFree(scaledBuffer);
		scaledBuffer = shrunk;

		scaled_width >>= 1;
		scaled_height >>= 1;

		if (scaled_width < 1) {
			scaled_width = 1;
		}

		if (scaled_height < 1) {
			scaled_height = 1;
		}

		miplevel++;

		// this is a visualization tool that shades each mip map
		// level with a different color so you can see the
		// rasterizer's texture level selection algorithm
		// Changing the color doesn't help with lumminance/alpha/intensity formats...
		if (depth == TD_DIFFUSE && globalImages->image_colorMipLevels.GetBool()) {
			R_BlendOverTexture((byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel]);
		}

		// upload the mip map
#if !defined(GL_ES_VERSION_2_0)
		if (internalFormat == GL_COLOR_INDEX8_EXT) {
			UploadCompressedNormalMap(scaled_width, scaled_height, scaledBuffer, miplevel);
		} else
#endif
		{
			char filename[MAX_IMAGE_NAME];
			char*fptr=&filename[0];
			ImageProgramStringToCompressedFileName(imgName, filename);
			char *ext = strrchr(filename, '.');
			if (ext) {
			strcpy(ext, ".e");
			ext[2]='0'+miplevel/10;ext[3]='0'+miplevel%10;
			}
			else
			fptr=0;
			myglTexImage2D(fptr,GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
		}
	}

	if (scaledBuffer != 0) {
		R_StaticFree(scaledBuffer);
	}

	SetImageFilterAndRepeat();

	// see if we messed anything up
	GL_CheckErrors();
}


#if !defined(GL_ES_VERSION_2_0)
/*
==================
Generate3DImage
==================
*/
void idImage::Generate3DImage(const byte *pic, int width, int height, int picDepth,
                              textureFilter_t filterParm, bool allowDownSizeParm,
                              textureRepeat_t repeatParm, textureDepth_t minDepthParm)
{
	int			scaled_width, scaled_height, scaled_depth;

	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = minDepthParm;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if (!glConfig.isInitialized) {
		return;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(width);
	scaled_height = MakePowerOfTwo(height);
	scaled_depth = MakePowerOfTwo(picDepth);

	if (scaled_width != width || scaled_height != height || scaled_depth != picDepth) {
		common->Error("R_Create3DImage: not a power of 2 image");
	}

	// FIXME: allow picmip here

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	// this function doesn't need to know it is 3D, so just make it very "tall"
	internalFormat = SelectInternalFormat(&pic, 1, width, height * picDepth, minDepthParm);

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;
	uploadDepth = scaled_depth;


	type = TT_3D;

	// upload the main image level
	Bind();

	qglTexImage3D(GL_TEXTURE_3D, 0, internalFormat, scaled_width, scaled_height, scaled_depth,
	              0, GL_RGBA, GL_UNSIGNED_BYTE, pic);

	// create and upload the mip map levels
	int		miplevel;
	byte	*scaledBuffer, *shrunk;

	scaledBuffer = (byte *)R_StaticAlloc(scaled_width * scaled_height * scaled_depth * 4);
	memcpy(scaledBuffer, pic, scaled_width * scaled_height * scaled_depth * 4);
	miplevel = 0;

	while (scaled_width > 1 || scaled_height > 1 || scaled_depth > 1) {
		// preserve the border after mip map unless repeating
		shrunk = R_MipMap3D(scaledBuffer, scaled_width, scaled_height, scaled_depth,
		                    (bool)(repeat != TR_REPEAT));
		R_StaticFree(scaledBuffer);
		scaledBuffer = shrunk;

		scaled_width >>= 1;
		scaled_height >>= 1;
		scaled_depth >>= 1;

		if (scaled_width < 1) {
			scaled_width = 1;
		}

		if (scaled_height < 1) {
			scaled_height = 1;
		}

		if (scaled_depth < 1) {
			scaled_depth = 1;
		}

		miplevel++;

		// upload the mip map
		qglTexImage3D(GL_TEXTURE_3D, miplevel, internalFormat, scaled_width, scaled_height, scaled_depth,
		              0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
	}

	R_StaticFree(scaledBuffer);

	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture filter");
	}

	// set the wrap/clamp modes
	switch (repeat) {
		case TR_REPEAT:
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
			break;
		case TR_CLAMP_TO_BORDER:
		case TR_CLAMP_TO_ZERO:
		case TR_CLAMP_TO_ZERO_ALPHA:
		case TR_CLAMP:
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			qglTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture repeat");
	}

	// see if we messed anything up
	GL_CheckErrors();
}
#endif


/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void idImage::GenerateCubeImage(const byte *pic[6], int size,
                                textureFilter_t filterParm, bool allowDownSizeParm,
                                textureDepth_t depthParm)
{
	int			scaled_width, scaled_height;
	int			width, height;
	int			i;

	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	depth = depthParm;

	type = TT_CUBIC;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if (!glConfig.isInitialized) {
		return;
	}

#if !defined(GL_ES_VERSION_2_0)
	if (! glConfig.cubeMapAvailable) {
		return;
	}
#endif

	width = height = size;

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat(pic, 6, width, height, depth);

	// don't bother with downsample for now
	scaled_width = width;
	scaled_height = height;

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;

	Bind();

	// no other clamp mode makes sense
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture filter");
	}

	// upload the base level
	// FIXME: support GL_COLOR_INDEX8_EXT?
	for (i = 0 ; i < 6 ; i++) {
		myglTexImage2D(0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalFormat, scaled_width, scaled_height, 0,
		              GL_RGBA, GL_UNSIGNED_BYTE, pic[i]);
	}


	// create and upload the mip map levels
	int		miplevel;
	byte	*shrunk[6];

	for (i = 0 ; i < 6 ; i++) {
		shrunk[i] = R_MipMap(pic[i], scaled_width, scaled_height, false);
	}

	miplevel = 1;

	while (scaled_width > 1) {
		for (i = 0 ; i < 6 ; i++) {
			byte	*shrunken;

			myglTexImage2D(0,GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, miplevel, internalFormat,
			              scaled_width / 2, scaled_height / 2, 0,
			              GL_RGBA, GL_UNSIGNED_BYTE, shrunk[i]);

			if (scaled_width > 2) {
				shrunken = R_MipMap(shrunk[i], scaled_width/2, scaled_height/2, false);
			} else {
				shrunken = NULL;
			}

			R_StaticFree(shrunk[i]);
			shrunk[i] = shrunken;
		}

		scaled_width >>= 1;
		scaled_height >>= 1;
		miplevel++;
	}

	// see if we messed anything up
	GL_CheckErrors();
}


/*
================
ImageProgramStringToFileCompressedFileName
================
*/
void idImage::ImageProgramStringToCompressedFileName(const char *imageProg, char *fileName) const
{
	const char	*s;
	char	*f;

	strcpy(fileName, "dds/");
	f = fileName + strlen(fileName);

	int depth = 0;

	// convert all illegal characters to underscores
	// this could conceivably produce a duplicated mapping, but we aren't going to worry about it
	for (s = imageProg ; *s ; s++) {
		if (*s == '/' || *s == '\\' || *s == '(') {
			if (depth < 4) {
				*f = '/';
				depth ++;
			} else {
				*f = ' ';
			}

			f++;
		} else if (*s == '<' || *s == '>' || *s == ':' || *s == '|' || *s == '"' || *s == '.') {
			*f = '_';
			f++;
		} else if (*s == ' ' && *(f-1) == '/') {	// ignore a space right after a slash
		} else if (*s == ')' || *s == ',') {		// always ignore these
		} else {
			*f = *s;
			f++;
		}
	}

	*f++ = 0;
	strcat(fileName, ".dds");
}

/*
==================
NumLevelsForImageSize
==================
*/
int	idImage::NumLevelsForImageSize(int width, int height) const
{
	int	numLevels = 1;

	while (width > 1 || height > 1) {
		numLevels++;
		width >>= 1;
		height >>= 1;
	}

	return numLevels;
}

/*
================
WritePrecompressedImage

When we are happy with our source data, we can write out precompressed
versions of everything to speed future load times.
================
*/
void idImage::WritePrecompressedImage()
{
#if !defined(GL_ES_VERSION_2_0)

	// Always write the precompressed image if we're making a build
	if (!com_makingBuild.GetBool()) {
		if (!globalImages->image_writePrecompressedTextures.GetBool() || !globalImages->image_usePrecompressedTextures.GetBool()) {
			return;
		}
	}

	if (!glConfig.isInitialized) {
		return;
	}

	char filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName(imgName, filename);



	int numLevels = NumLevelsForImageSize(uploadWidth, uploadHeight);

	if (numLevels > MAX_TEXTURE_LEVELS) {
		common->Warning("R_WritePrecompressedImage: level > MAX_TEXTURE_LEVELS for image %s", filename);
		return;
	}

	// glGetTexImage only supports a small subset of all the available internal formats
	// We have to use BGRA because DDS is a windows based format
	int altInternalFormat = 0;
	int bitSize = 0;

	switch (internalFormat) {
		case GL_COLOR_INDEX8_EXT:
		case GL_COLOR_INDEX:
			// this will not work with dds viewers but we need it in this format to save disk
			// load speed ( i.e. size )
			altInternalFormat = GL_COLOR_INDEX;
			bitSize = 24;
			break;
		case 1:
		case GL_INTENSITY8:
		case GL_LUMINANCE8:
		case 3:
		case GL_RGB8:
			altInternalFormat = GL_BGR_EXT;
			bitSize = 24;
			break;
		case GL_LUMINANCE8_ALPHA8:
		case 4:
		case GL_RGBA8:
			altInternalFormat = GL_BGRA_EXT;
			bitSize = 32;
			break;
		case GL_ALPHA8:
			altInternalFormat = GL_ALPHA;
			bitSize = 8;
			break;
		default:

			if (FormatIsDXT(internalFormat)) {
				altInternalFormat = internalFormat;
			} else {
				common->Warning("Unknown or unsupported format for %s", filename);
				return;
			}
	}

	if (globalImages->image_useOffLineCompression.GetBool() && FormatIsDXT(altInternalFormat)) {
		idStr outFile = fileSystem->RelativePathToOSPath(filename, "fs_basepath");
		idStr inFile = outFile;
		inFile.StripFileExtension();
		inFile.SetFileExtension("tga");
		idStr format;

		if (depth == TD_BUMP) {
			format = "RXGB +red 0.0 +green 0.5 +blue 0.5";
		} else {
			switch (altInternalFormat) {
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
					format = "DXT1";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
					format = "DXT1 -alpha_threshold";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
					format = "DXT3";
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
					format = "DXT5";
					break;
			}
		}

		globalImages->AddDDSCommand(va("z:/d3xp/compressonator/thecompressonator -convert \"%s\" \"%s\" %s -mipmaps\n", inFile.c_str(), outFile.c_str(), format.c_str()));
		return;
	}


	ddsFileHeader_t header;
	memset(&header, 0, sizeof(header));
	header.dwSize = sizeof(header);
	header.dwFlags = DDSF_CAPS | DDSF_PIXELFORMAT | DDSF_WIDTH | DDSF_HEIGHT;
	header.dwHeight = uploadHeight;
	header.dwWidth = uploadWidth;

	if (FormatIsDXT(altInternalFormat)) {
		// size (in bytes) of the compressed base image
		header.dwFlags |= DDSF_LINEARSIZE;
		header.dwPitchOrLinearSize = ((uploadWidth + 3) / 4) * ((uploadHeight + 3) / 4)*
		                             (altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
	} else {
		// 4 Byte aligned line width (from nv_dds)
		header.dwFlags |= DDSF_PITCH;
		header.dwPitchOrLinearSize = ((uploadWidth * bitSize + 31) & -32) >> 3;
	}

	header.dwCaps1 = DDSF_TEXTURE;

	if (numLevels > 1) {
		header.dwMipMapCount = numLevels;
		header.dwFlags |= DDSF_MIPMAPCOUNT;
		header.dwCaps1 |= DDSF_MIPMAP | DDSF_COMPLEX;
	}

	header.ddspf.dwSize = sizeof(header.ddspf);

	if (FormatIsDXT(altInternalFormat)) {
		header.ddspf.dwFlags = DDSF_FOURCC;

		switch (altInternalFormat) {
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','1');
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
				header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','1');
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','3');
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				header.ddspf.dwFourCC = DDS_MAKEFOURCC('D','X','T','5');
				break;
		}
	} else {
		header.ddspf.dwFlags = (internalFormat == GL_COLOR_INDEX8_EXT) ? DDSF_RGB | DDSF_ID_INDEXCOLOR : DDSF_RGB;
		header.ddspf.dwRGBBitCount = bitSize;

		switch (altInternalFormat) {
			case GL_BGRA_EXT:
			case GL_LUMINANCE_ALPHA:
				header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
				header.ddspf.dwABitMask = 0xFF000000;
				// Fall through
			case GL_BGR_EXT:
			case GL_LUMINANCE:
			case GL_COLOR_INDEX:
				header.ddspf.dwRBitMask = 0x00FF0000;
				header.ddspf.dwGBitMask = 0x0000FF00;
				header.ddspf.dwBBitMask = 0x000000FF;
				break;
			case GL_ALPHA:
				header.ddspf.dwFlags = DDSF_ALPHAPIXELS;
				header.ddspf.dwABitMask = 0xFF000000;
				break;
			default:
				common->Warning("Unknown or unsupported format for %s", filename);
				return;
		}
	}

	idFile *f = fileSystem->OpenFileWrite(filename);

	if (f == NULL) {
		common->Warning("Could not open %s trying to write precompressed image", filename);
		return;
	}

	common->Printf("Writing precompressed image: %s\n", filename);

	f->Write("DDS ", 4);
	f->Write(&header, sizeof(header));

	// bind to the image so we can read back the contents
	Bind();

	qglPixelStorei(GL_PACK_ALIGNMENT, 1);	// otherwise small rows get padded to 32 bits

	int uw = uploadWidth;
	int uh = uploadHeight;

	// Will be allocated first time through the loop
	byte *data = NULL;

	for (int level = 0 ; level < numLevels ; level++) {

		int size = 0;

		if (FormatIsDXT(altInternalFormat)) {
			size = ((uw + 3) / 4) * ((uh + 3) / 4) *
			       (altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
		} else {
			size = uw * uh * (bitSize / 8);
		}

		if (data == NULL) {
			data = (byte *)R_StaticAlloc(size);
		}

		if (FormatIsDXT(altInternalFormat)) {
			qglGetCompressedTexImageARB(GL_TEXTURE_2D, level, data);
		} else {
			qglGetTexImage(GL_TEXTURE_2D, level, altInternalFormat, GL_UNSIGNED_BYTE, data);
		}

		f->Write(data, size);

		uw /= 2;
		uh /= 2;

		if (uw < 1) {
			uw = 1;
		}

		if (uh < 1) {
			uh = 1;
		}
	}

	if (data != NULL) {
		R_StaticFree(data);
	}

	fileSystem->CloseFile(f);
#endif
}

/*
================
ShouldImageBePartialCached

Returns true if there is a precompressed image, and it is large enough
to be worth caching
================
*/
bool idImage::ShouldImageBePartialCached()
{
	if (!glConfig.textureCompressionAvailable) {
		return false;
	}

	if (!globalImages->image_useCache.GetBool()) {
		return false;
	}

	// the allowDownSize flag does double-duty as don't-partial-load
	if (!allowDownSize) {
		return false;
	}

	if (globalImages->image_cacheMinK.GetInteger() <= 0) {
		return false;
	}

	// if we are doing a copyFiles, make sure the original images are referenced
	if (fileSystem->PerformingCopyFiles()) {
		return false;
	}

	char	filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName(imgName, filename);

	// get the file timestamp
	fileSystem->ReadFile(filename, NULL, &timestamp);

	if (timestamp == FILE_NOT_FOUND_TIMESTAMP) {
		return false;
	}

	// open it and get the file size
	idFile *f;

	f = fileSystem->OpenFileRead(filename);

	if (!f) {
		return false;
	}

	int	len = f->Length();
	fileSystem->CloseFile(f);

	if (len <= globalImages->image_cacheMinK.GetInteger() * 1024) {
		return false;
	}

	// we do want to do a partial load
	return true;
}

/*
================
CheckPrecompressedImage

If fullLoad is false, only the small mip levels of the image will be loaded
================
*/
bool idImage::CheckPrecompressedImage(bool fullLoad)
{
	if (!glConfig.isInitialized || !glConfig.textureCompressionAvailable) {
		return false;
	}

#if 1 // ( _D3XP had disabled ) - Allow grabbing of DDS's from original Doom pak files
	// if we are doing a copyFiles, make sure the original images are referenced
	if (fileSystem->PerformingCopyFiles()) {
		return false;
	}

#endif

	if (depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 2) {
		return false;
	}

	// god i love last minute hacks :-)
	if (com_machineSpec.GetInteger() >= 1 && com_videoRam.GetInteger() >= 128 && imgName.Icmpn("lights/", 7) == 0) {
		return false;
	}

	char filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName(imgName, filename);

	// get the file timestamp
	ID_TIME_T precompTimestamp;
	fileSystem->ReadFile(filename, NULL, &precompTimestamp);


	if (precompTimestamp == FILE_NOT_FOUND_TIMESTAMP) {
		return false;
	}

	if (!generatorFunction && timestamp != FILE_NOT_FOUND_TIMESTAMP) {
		if (precompTimestamp < timestamp) {
			// The image has changed after being precompressed
			return false;
		}
	}

	timestamp = precompTimestamp;

	// open it and just read the header
	idFile *f;

	f = fileSystem->OpenFileRead(filename);

	if (!f) {
		return false;
	}

	int	len = f->Length();

	if (len < sizeof(ddsFileHeader_t)) {
		fileSystem->CloseFile(f);
		return false;
	}

	if (!fullLoad && len > globalImages->image_cacheMinK.GetInteger() * 1024) {
		len = globalImages->image_cacheMinK.GetInteger() * 1024;
	}

	byte *data = (byte *)R_StaticAlloc(len);

	f->Read(data, len);

	fileSystem->CloseFile(f);

	unsigned int magic = LittleLong(*(unsigned int *)data); //k: 64long???
	ddsFileHeader_t	*_header = (ddsFileHeader_t *)(data + 4);
	int ddspf_dwFlags = LittleLong(_header->ddspf.dwFlags);

	if (magic != DDS_MAKEFOURCC('D', 'D', 'S', ' ')) {
		common->Printf("CheckPrecompressedImage( %s ): magic != 'DDS '\n", imgName.c_str());
		R_StaticFree(data);
		return false;
	}

	// if we don't support color index textures, we must load the full image
	// should we just expand the 256 color image to 32 bit for upload?
	if (ddspf_dwFlags & DDSF_ID_INDEXCOLOR && !glConfig.sharedTexturePaletteAvailable) {
		R_StaticFree(data);
		return false;
	}

	if (!(ddspf_dwFlags & DDSF_FOURCC))
	{
		R_StaticFree(data);
		return false;
	}

	// upload all the levels
	UploadPrecompressedImage(data, len);

	R_StaticFree(data);

	return true;
}

/*
===================
UploadPrecompressedImage

This can be called by the front end during nromal loading,
or by the backend after a background read of the file
has completed
===================
*/
void idImage::UploadPrecompressedImage(byte *data, int len)
{
	ddsFileHeader_t	*header = (ddsFileHeader_t *)(data + 4);

	// ( not byte swapping dwReserved1 dwReserved2 )
	header->dwSize = LittleLong(header->dwSize);
	header->dwFlags = LittleLong(header->dwFlags);
	header->dwHeight = LittleLong(header->dwHeight);
	header->dwWidth = LittleLong(header->dwWidth);
	header->dwPitchOrLinearSize = LittleLong(header->dwPitchOrLinearSize);
	header->dwDepth = LittleLong(header->dwDepth);
	header->dwMipMapCount = LittleLong(header->dwMipMapCount);
	header->dwCaps1 = LittleLong(header->dwCaps1);
	header->dwCaps2 = LittleLong(header->dwCaps2);

	header->ddspf.dwSize = LittleLong(header->ddspf.dwSize);
	header->ddspf.dwFlags = LittleLong(header->ddspf.dwFlags);
	header->ddspf.dwFourCC = LittleLong(header->ddspf.dwFourCC);
	header->ddspf.dwRGBBitCount = LittleLong(header->ddspf.dwRGBBitCount);
	header->ddspf.dwRBitMask = LittleLong(header->ddspf.dwRBitMask);
	header->ddspf.dwGBitMask = LittleLong(header->ddspf.dwGBitMask);
	header->ddspf.dwBBitMask = LittleLong(header->ddspf.dwBBitMask);
	header->ddspf.dwABitMask = LittleLong(header->ddspf.dwABitMask);

	// generate the texture number
	qglGenTextures(1, &texnum);

	int externalFormat = 0;

	precompressedFile = true;

	uploadWidth = header->dwWidth;
	uploadHeight = header->dwHeight;
	if (header->ddspf.dwFlags & DDSF_FOURCC) {
		switch (header->ddspf.dwFourCC) {
			case DDS_MAKEFOURCC('D', 'X', 'T', '1'):

				if (header->ddspf.dwFlags & DDSF_ALPHAPIXELS) {
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				} else {
					internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				}

				break;
			case DDS_MAKEFOURCC('D', 'X', 'T', '3'):
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case DDS_MAKEFOURCC('D', 'X', 'T', '5'):
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			case DDS_MAKEFOURCC('R', 'X', 'G', 'B'):
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			default:
				common->Warning("Invalid compressed internal format\n");
				return;
		}
	}
	#if 0
	else if ((header->ddspf.dwFlags & DDSF_RGBA) && header->ddspf.dwRGBBitCount == 32) {
		externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	} else if ((header->ddspf.dwFlags & DDSF_RGB) && header->ddspf.dwRGBBitCount == 32) {
		externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	} else if ((header->ddspf.dwFlags & DDSF_RGB) && header->ddspf.dwRGBBitCount == 24) {
		if (header->ddspf.dwFlags & DDSF_ID_INDEXCOLOR) {
			externalFormat = GL_COLOR_INDEX;
			internalFormat = GL_COLOR_INDEX8_EXT;
			return;
		} else {
			externalFormat = GL_BGR_EXT;
			internalFormat = GL_RGB8;
			return;
		}
	} else if (header->ddspf.dwRGBBitCount == 8) {
		externalFormat = GL_ALPHA;
		internalFormat = GL_ALPHA8;
	}
	#endif
	else {
		common->Warning("Invalid uncompressed internal format\n");
		return;
	}

	type = TT_2D;			// FIXME: we may want to support pre-compressed cube maps in the future

	Bind();

	int numMipmaps = 1;

	if (header->dwFlags & DDSF_MIPMAPCOUNT) {
		numMipmaps = header->dwMipMapCount;
	}

	int uw = uploadWidth;
	int uh = uploadHeight;

	// We may skip some mip maps if we are downsizing
	int skipMip = 0;
	GetDownsize(uploadWidth, uploadHeight);

	byte *imagedata = data + sizeof(ddsFileHeader_t) + 4;

	for (int i = 0 ; i < numMipmaps; i++) {
		int size = 0;

		if (FormatIsDXT(internalFormat)) {
			size = ((uw + 3) / 4) * ((uh + 3) / 4) *
			       (internalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);
		} else {
			size = uw * uh * (header->ddspf.dwRGBBitCount / 8);
		}

		if (uw > uploadWidth || uh > uploadHeight) {
			skipMip++;
		} else {
			if (FormatIsDXT(internalFormat)) {
				qglCompressedTexImage2D(GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, size, imagedata);
			} 
			#if 0			
			else {
				qglTexImage2D(GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, externalFormat, GL_UNSIGNED_BYTE, imagedata);
			}
			#endif
		}

		imagedata += size;
		uw /= 2;
		uh /= 2;

		if (uw < 1) {
			uw = 1;
		}

		if (uh < 1) {
			uh = 1;
		}
	}

	SetImageFilterAndRepeat();
}

/*
===============
ActuallyLoadImage

Absolutely every image goes through this path
On exit, the idImage will have a valid OpenGL texture number that can be bound
===============
*/

int idImage::GenerateImageETC(int width, int height,
                      textureFilter_t filterParm, bool allowDownSizeParm,
                      textureRepeat_t repeatParm, textureDepth_t depthParm)
{
	bool	preserveBorder;
	int	scaled_width, scaled_height;

	char filename[MAX_IMAGE_NAME];
	char*fptr=&filename[0];
	ImageProgramStringToCompressedFileName(imgName, filename);
	char *ext = strrchr(filename, '.');
	if (ext) {
	strcpy(ext, ".etc");
	}
	else
	return 0;
	if (!etcavail(fptr))
	return 0;

	PurgeImage();

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = depthParm;

	if (!glConfig.isInitialized) {
		return 0;
	}

	// don't let mip mapping smear the texture into the clamped border
	if (repeat == TR_CLAMP_TO_ZERO) {
		preserveBorder = true;
	} else {
		preserveBorder = false;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(width);
	scaled_height = MakePowerOfTwo(height);

	if (scaled_width != width || scaled_height != height) {
		common->Error("R_CreateImage: not a power of 2 image");
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	internalFormat = GL_RGBA;

	// copy or resample data as appropriate for first MIP level
	if ((scaled_width == width) && (scaled_height == height)) {
		// we must copy even if unchanged, because the border zeroing
		// would otherwise modify const data
	} else {
		// resample down as needed (FIXME: this doesn't seem like it resamples anymore!)
		// scaledBuffer = R_ResampleTexture( pic, width, height, width >>= 1, height >>= 1 );
		width >>= 1;
		height >>= 1;
		if (width < 1) {
			width = 1;
		}
		if (height < 1) {
			height = 1;
		}
		while (width > scaled_width || height > scaled_height) {
			width >>= 1;
			height >>= 1;
			if (width < 1) {
				width = 1;
			}
			if (height < 1) {
				height = 1;
			}
		}
		scaled_width = width;
		scaled_height = height;
	}

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;
	type = TT_2D;
	// upload the main image level
	Bind();

	int failed=0;

	{
		char filename[MAX_IMAGE_NAME];
		char*fptr=&filename[0];
		ImageProgramStringToCompressedFileName(imgName, filename);
		char *ext = strrchr(filename, '.');
		if (ext) {
		strcpy(ext, ".etc");
		}
		else
		fptr=0;
		failed+=uploadetc(fptr,GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE);
	}

	// create and upload the mip map levels, which we do in all cases, even if we don't think they are needed
	int		miplevel;

	miplevel = 0;

	while (scaled_width > 1 || scaled_height > 1) {

		scaled_width >>= 1;
		scaled_height >>= 1;

		if (scaled_width < 1) {
			scaled_width = 1;
		}

		if (scaled_height < 1) {
			scaled_height = 1;
		}

		miplevel++;
		{
			char filename[MAX_IMAGE_NAME];
			char*fptr=&filename[0];
			ImageProgramStringToCompressedFileName(imgName, filename);
			char *ext = strrchr(filename, '.');
			if (ext) {
			strcpy(ext, ".e");
			ext[2]='0'+miplevel/10;ext[3]='0'+miplevel%10;
			}
			else
			fptr=0;
			failed+=uploadetc(fptr,GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE);
		}
	}

	SetImageFilterAndRepeat();
	GL_CheckErrors();
	return (failed==0);
}

#ifdef _MULTITHREAD
void	idImage::ActuallyLoadImage(bool checkForPrecompressed, bool fromBackEnd, bool pending)
#else
void	idImage::ActuallyLoadImage(bool checkForPrecompressed, bool fromBackEnd)
#endif
{
	int		width, height;
	byte	*pic;

#ifdef _MULTITHREAD
	if(multithreadActive)
	{
		if(pending)
		{
			globalImages->AddAllocList(this, checkForPrecompressed, fromBackEnd);
			return;
		}
		if(fromBackEnd)
		{
			//LOGI("ERROR!! CAN NOT LOAD IMAGE FROM BIND");
			globalImages->AddAllocList( this, checkForPrecompressed, fromBackEnd );
			return;
		}

		if( cinematic )
		{
			cinData_t	cin;

			cin = cinematic->ImageForTime( cinmaticNextTime );

			if( texnum == TEXTURE_NOT_LOADED )
				qglGenTextures( 1, &texnum );

			if ( cin.image ) {
				UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
			} else {
				//globalImages->blackImage->Bind();
			}

			return;
		}
	}
#endif

	// this is the ONLY place generatorFunction will ever be called
	if (generatorFunction) {
		generatorFunction(this);
		return;
	}

	// if we are a partial image, we are only going to load from a compressed file
	if (isPartialImage) {
		if (CheckPrecompressedImage(false)) {
			return;
		}

		// this is an error -- the partial image failed to load
		MakeDefault();
		return;
	}

	//
	// load the image from disk
	//
	if (cubeFiles != CF_2D) {
		byte	*pics[6];

		// we don't check for pre-compressed cube images currently
		R_LoadCubeImages(imgName, cubeFiles, pics, &width, &timestamp);

		if (pics[0] == NULL) {
#ifdef _MULTITHREADxxx
			if(multithreadActive && !pending) //karin: render thread don't call Print to console
				Sys_Printf("Couldn't load cube image: %s\n", imgName.c_str());
			else
#endif
			common->Warning("Couldn't load cube image: %s", imgName.c_str());
			MakeDefault();
			return;
		}

		GenerateCubeImage((const byte **)pics, width, filter, allowDownSize, depth);
		precompressedFile = false;

		for (int i = 0 ; i < 6 ; i++) {
			if (pics[i]) {
				R_StaticFree(pics[i]);
			}
		}
	} else {
		// see if we have a pre-generated image file that is
		// already image processed and compressed
		if (checkForPrecompressed && globalImages->image_usePrecompressedTextures.GetBool()) {
			if (CheckPrecompressedImage(true)) {
				// we got the precompressed image
				return;
			}

			// fall through to load the normal image
		}

		R_LoadImageProgram(imgName, &pic, &width, &height, &timestamp, &depth);

		if (pic == NULL) {
#ifdef _MULTITHREADxxx
			if(multithreadActive && !pending) //karin: render thread don't call Print to console
				Sys_Printf("Couldn't load image: %s\n", imgName.c_str());
			else
#endif
			common->Warning("Couldn't load image: %s", imgName.c_str());
			MakeDefault();
			return;
		}

		/*
				// swap the red and alpha for rxgb support
				// do this even on tga normal maps so we only have to use
				// one fragment program
				// if the image is precompressed ( either in palletized mode or true rxgb mode )
				// then it is loaded above and the swap never happens here
				if ( depth == TD_BUMP && globalImages->image_useNormalCompression.GetInteger() != 1 ) {
					for ( int i = 0; i < width * height * 4; i += 4 ) {
						pic[ i + 3 ] = pic[ i ];
						pic[ i ] = 0;
					}
				}
		*/
		// build a hash for checking duplicate image files
		// NOTE: takes about 10% of image load times (SD)
		// may not be strictly necessary, but some code uses it, so let's leave it in

		// not necessary =)
		// imageHash = MD4_BlockChecksum(pic, width * height * 4);
		imageHash = imgName.FileNameHash();
		if (!GenerateImageETC(width,height,filter,allowDownSize,repeat,depth))
		GenerateImage(pic, width, height, filter, allowDownSize, repeat, depth);
		timestamp = timestamp;
		precompressedFile = false;

		R_StaticFree(pic);

		// write out the precompressed version of this file if needed
		WritePrecompressedImage();
	}
}

//=========================================================================================================

/*
===============
PurgeImage
===============
*/
#ifdef _MULTITHREAD
void idImage::PurgeImage(bool pending)
#else
void idImage::PurgeImage()
#endif
{
#ifdef _MULTITHREAD
	if(multithreadActive && pending)
	{
		globalImages->AddPurgeList(this);
		return;
	}
#endif
	if (texnum != TEXTURE_NOT_LOADED) {
		qglDeleteTextures(1, &texnum);	// this should be the ONLY place it is ever called!
		texnum = TEXTURE_NOT_LOADED;
	}

	// clear all the current binding caches, so the next bind will do a real one
	for (int i = 0 ; i < MAX_MULTITEXTURE_UNITS ; i++) {
		backEnd.glState.tmu[i].current2DMap = -1;
		backEnd.glState.tmu[i].current3DMap = -1;
		backEnd.glState.tmu[i].currentCubeMap = -1;
	}
}

/*
==============
Bind

Automatically enables 2D mapping, cube mapping, or 3D texturing if needed
==============
*/
#ifdef _MULTITHREAD
void idImage::Bind(bool *res)
#else
void idImage::Bind()
#endif
{
	if (tr.logFile) {
		RB_LogComment("idImage::Bind( %s )\n", imgName.c_str());
	}

	// if this is an image that we are caching, move it to the front of the LRU chain
	if (partialImage) {
		if (cacheUsageNext) {
			// unlink from old position
			cacheUsageNext->cacheUsagePrev = cacheUsagePrev;
			cacheUsagePrev->cacheUsageNext = cacheUsageNext;
		}

		// link in at the head of the list
		cacheUsageNext = globalImages->cacheLRU.cacheUsageNext;
		cacheUsagePrev = &globalImages->cacheLRU;

		cacheUsageNext->cacheUsagePrev = this;
		cacheUsagePrev->cacheUsageNext = this;
	}

	// load the image if necessary (FIXME: not SMP safe!)
	if (texnum == TEXTURE_NOT_LOADED) {
		if (partialImage) {
			// if we have a partial image, go ahead and use that
			this->partialImage->Bind();

			// start a background load of the full thing if it isn't already in the queue
			if (!backgroundLoadInProgress) {
				StartBackgroundImageLoad();
			}

			return;
		}

		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage(true, true);	// check for precompressed, load is from back end
#ifdef _MULTITHREAD
		if(multithreadActive)
		{
			// Load a black image to reduce flicker
			if(this != globalImages->blackImage)
				globalImages->blackImage->Bind();
			if(res)
				*res = false;
			return;
		}
#endif
	}


	// bump our statistic counters
	frameUsed = backEnd.frameCount;
	bindCount++;

	tmu_t			*tmu = &backEnd.glState.tmu[backEnd.glState.currenttmu];

	// enable or disable apropriate texture modes
	if (tmu->textureType != type && (backEnd.glState.currenttmu <	glConfig.maxTextureUnits)) {
#if !defined(GL_ES_VERSION_2_0)
		if (tmu->textureType == TT_CUBIC) {
			qglDisable(GL_TEXTURE_CUBE_MAP);
		} else if (tmu->textureType == TT_3D) {
			qglDisable(GL_TEXTURE_3D);
		} else if (tmu->textureType == TT_2D) {
			qglDisable(GL_TEXTURE_2D);
		} else if (tmu->textureType == TT_2D_ARRAY) {
			qglDisable(GL_TEXTURE_2D_ARRAY);
		}

		if (type == TT_CUBIC) {
			qglEnable(GL_TEXTURE_CUBE_MAP);
		} else if (type == TT_3D) {
			qglEnable(GL_TEXTURE_3D);
		} else if (type == TT_2D) {
			qglEnable(GL_TEXTURE_2D);
		} else if (type == TT_2D_ARRAY) {
			qglEnable(GL_TEXTURE_2D_ARRAY);
		}
#endif

		tmu->textureType = type;
	}

	// bind the texture
	if (type == TT_2D) {
		if (tmu->current2DMap != texnum) {
			tmu->current2DMap = texnum;
			qglBindTexture(GL_TEXTURE_2D, texnum);
		}
	} else if (type == TT_CUBIC) {
		if (tmu->currentCubeMap != texnum) {
			tmu->currentCubeMap = texnum;
			qglBindTexture(GL_TEXTURE_CUBE_MAP, texnum);
		}
	}
#if !defined(GL_ES_VERSION_2_0)
	else if (type == TT_3D) {
		if (tmu->current3DMap != texnum) {
			tmu->current3DMap = texnum;
			qglBindTexture(GL_TEXTURE_3D, texnum);
		}
	}
#endif
#ifdef GL_ES_VERSION_3_0
	else if (type == TT_2D_ARRAY) {
		if (tmu->current2DArrayMap != texnum) {
			tmu->current2DArrayMap = texnum;
			qglBindTexture(GL_TEXTURE_2D_ARRAY, texnum);
		}
	}
#endif
}

/*
==============
BindFragment

Fragment programs explicitly say which type of map they want, so we don't need to
do any enable / disable changes
==============
*/
void idImage::BindFragment()
{
	if (tr.logFile) {
		RB_LogComment("idImage::BindFragment %s )\n", imgName.c_str());
	}

	// if this is an image that we are caching, move it to the front of the LRU chain
	if (partialImage) {
		if (cacheUsageNext) {
			// unlink from old position
			cacheUsageNext->cacheUsagePrev = cacheUsagePrev;
			cacheUsagePrev->cacheUsageNext = cacheUsageNext;
		}

		// link in at the head of the list
		cacheUsageNext = globalImages->cacheLRU.cacheUsageNext;
		cacheUsagePrev = &globalImages->cacheLRU;

		cacheUsageNext->cacheUsagePrev = this;
		cacheUsagePrev->cacheUsageNext = this;
	}

	// load the image if necessary (FIXME: not SMP safe!)
	if (texnum == TEXTURE_NOT_LOADED) {
		if (partialImage) {
			// if we have a partial image, go ahead and use that
			this->partialImage->BindFragment();

			// start a background load of the full thing if it isn't already in the queue
			if (!backgroundLoadInProgress) {
				StartBackgroundImageLoad();
			}

			return;
		}

		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage(true, true);	// check for precompressed, load is from back end
	}


	// bump our statistic counters
	frameUsed = backEnd.frameCount;
	bindCount++;

	// bind the texture
	if (type == TT_2D) {
		qglBindTexture(GL_TEXTURE_2D, texnum);
	}
#if defined(GL_NV_texture_rectangle)
	else if (type == TT_RECT) {
		qglBindTexture(GL_TEXTURE_RECTANGLE_NV, texnum);
	}
#endif
	else if (type == TT_CUBIC) {
		qglBindTexture(GL_TEXTURE_CUBE_MAP, texnum);
	}
#if !defined(GL_ES_VERSION_2_0)
	else if (type == TT_3D) {
		qglBindTexture(GL_TEXTURE_3D, texnum);
	}
#endif
#ifdef GL_ES_VERSION_3_0
	else if (type == TT_2D_ARRAY) {
		qglBindTexture(GL_TEXTURE_2D_ARRAY, texnum);
	}
#endif
}


/*
====================
CopyFramebuffer
====================
*/
void idImage::CopyFramebuffer(int x, int y, int imageWidth, int imageHeight, bool useOversizedBuffer)
{
	Bind();

	if (cvarSystem->GetCVarBool("g_lowresFullscreenFX")) {
		imageWidth = 512;
		imageHeight = 512;
	}

	// if the size isn't a power of 2, the image must be increased in size
	int	potWidth, potHeight;

	potWidth = MakePowerOfTwo(imageWidth);
	potHeight = MakePowerOfTwo(imageHeight);

	//k: fix berserk/double/grabber/helltime vision
#if !defined(GL_ES_VERSION_2_0)
	GetDownsize(imageWidth, imageHeight);
	GetDownsize(potWidth, potHeight);
#endif

#if !defined(GL_ES_VERSION_2_0)
	if(USING_GLES3)
		qglReadBuffer(GL_BACK);
#endif

	// only resize if the current dimensions can't hold it at all,
	// otherwise subview renderings could thrash this
	if ((useOversizedBuffer && (uploadWidth < potWidth || uploadHeight < potHeight))
	    || (!useOversizedBuffer && (uploadWidth != potWidth || uploadHeight != potHeight))) {
		uploadWidth = potWidth;
		uploadHeight = potHeight;

		if (potWidth == imageWidth && potHeight == imageHeight) {
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, imageWidth, imageHeight, 0);
		} else {
			byte	*junk;
			// we need to create a dummy image with power of two dimensions,
			// then do a glCopyTexSubImage2D of the data we want
			// this might be a 16+ meg allocation, which could fail on _alloca
			junk = (byte *)Mem_Alloc(potWidth * potHeight * 3);
			memset(junk, 0, potWidth * potHeight * 3);		//!@#
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, potWidth, potHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, junk);

			Mem_Free(junk);

			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight);
		}
	} else {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression or some other silliness
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight);
	}

	// if the image isn't a full power of two, duplicate an extra row and/or column to fix bilerps
	if (imageWidth != potWidth) {
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, imageWidth, 0, x+imageWidth-1, y, 1, imageHeight);
	}

	if (imageHeight != potHeight) {
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, imageHeight, x, y+imageHeight-1, imageWidth, 1);
	}

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	backEnd.c_copyFrameBuffer++;
}

/*
====================
CopyDepthbuffer

This should just be part of copyFramebuffer once we have a proper image type field
====================
*/
void idImage::CopyDepthbuffer(int x, int y, int imageWidth, int imageHeight)
{
	Bind();

	// if the size isn't a power of 2, the image must be increased in size
	int	potWidth, potHeight;

	GLenum depthComponent = GL_DEPTH_COMPONENT;

	potWidth = MakePowerOfTwo(imageWidth);
	potHeight = MakePowerOfTwo(imageHeight);

	if (uploadWidth != potWidth || uploadHeight != potHeight) {
		uploadWidth = potWidth;
		uploadHeight = potHeight;

		if (potWidth == imageWidth && potHeight == imageHeight) {
			qglCopyTexImage2D(GL_TEXTURE_2D, 0, depthComponent, x, y, imageWidth, imageHeight, 0);
		} else {
			// we need to create a dummy image with power of two dimensions,
			// then do a glCopyTexSubImage2D of the data we want
			qglTexImage2D(GL_TEXTURE_2D, 0, depthComponent, potWidth, potHeight, 0, depthComponent, GL_UNSIGNED_BYTE, NULL);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight);
		}
	} else {
		// otherwise, just subimage upload it so that drivers can tell we are going to be changing
		// it and don't try and do a texture compression or some other silliness
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight);
	}

//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
//	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void idImage::UploadScratch(const byte *data, int cols, int rows)
{
	int			i;

	// if rows = cols * 6, assume it is a cube map animation
	if (rows == cols * 6) {
		if (type != TT_CUBIC) {
			type = TT_CUBIC;
			uploadWidth = -1;	// for a non-sub upload
		}

		Bind();

		rows /= 6;

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if (cols != uploadWidth || rows != uploadHeight) {
			uploadWidth = cols;
			uploadHeight = rows;

			// upload the base level
			for (i = 0 ; i < 6 ; i++) {
				qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGBA, cols, rows, 0,
				              GL_RGBA, GL_UNSIGNED_BYTE, data + cols*rows*4*i);
			}
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			for (i = 0 ; i < 6 ; i++) {
				qglTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, 0, 0, cols, rows,
				                 GL_RGBA, GL_UNSIGNED_BYTE, data + cols*rows*4*i);
			}
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// no other clamp mode makes sense
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		// otherwise, it is a 2D image
		if (type != TT_2D) {
			type = TT_2D;
			uploadWidth = -1;	// for a non-sub upload
		}

		Bind();

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if (cols != uploadWidth || rows != uploadHeight) {
			uploadWidth = cols;
			uploadHeight = rows;
			qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// these probably should be clamp, but we have a lot of issues with editor
		// geometry coming out with texcoords slightly off one side, resulting in
		// a smear across the entire polygon
#if 1
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#else
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif
	}
}


void idImage::SetClassification(int tag)
{
	classification = tag;
}

/*
==================
StorageSize
==================
*/
int idImage::StorageSize() const
{
	int		baseSize;

	if (texnum == TEXTURE_NOT_LOADED) {
		return 0;
	}

	switch (type) {
		default:
		case TT_2D:
			baseSize = uploadWidth*uploadHeight;
			break;
		case TT_3D:
			baseSize = uploadWidth*uploadHeight*uploadDepth;
			break;
		case TT_CUBIC:
			baseSize = 6 * uploadWidth*uploadHeight;
			break;
#ifdef GL_ES_VERSION_3_0
		case TT_2D_ARRAY:
			baseSize = 6 * uploadWidth*uploadHeight;
			break;
#endif
	}

	baseSize *= BitsForInternalFormat(internalFormat);

	baseSize /= 8;

	// account for mip mapping
	baseSize = baseSize * 4 / 3;

	return baseSize;
}

/*
==================
Print
==================
*/
void idImage::Print() const
{
	if (precompressedFile) {
		common->Printf("P");
	} else if (generatorFunction) {
		common->Printf("F");
	} else {
		common->Printf(" ");
	}

	switch (type) {
		case TT_2D:
			common->Printf(" ");
			break;
		case TT_3D:
			common->Printf("3");
			break;
		case TT_CUBIC:
			common->Printf("C");
			break;
		case TT_RECT:
			common->Printf("R");
			break;
#ifdef GL_ES_VERSION_3_0
		case TT_2D_ARRAY:
			common->Printf("A");
			break;
#endif
		default:
			common->Printf("<BAD TYPE:%i>", type);
			break;
	}

	common->Printf("%4i %4i ",	uploadWidth, uploadHeight);

	switch (filter) {
		case TF_DEFAULT:
			common->Printf("dflt ");
			break;
		case TF_LINEAR:
			common->Printf("linr ");
			break;
		case TF_NEAREST:
			common->Printf("nrst ");
			break;
		default:
			common->Printf("<BAD FILTER:%i>", filter);
			break;
	}

	switch (internalFormat) {
		case 1:
			common->Printf("I     ");
			break;
		case 2:
		case GL_LUMINANCE_ALPHA:
			common->Printf("LA    ");
			break;
		case 3:
			common->Printf("RGB   ");
			break;
		case 4:
			common->Printf("RGBA  ");
			break;
		case GL_LUMINANCE:
			common->Printf("L     ");
			break;
		case GL_ALPHA:
			common->Printf("A     ");
			break;
		case GL_RGBA:
			common->Printf("RGBA  ");
			break;
		case GL_RGB:
			common->Printf("RGB   ");
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			common->Printf("DXT1  ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			common->Printf("DXT1A ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			common->Printf("DXT3  ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			common->Printf("DXT5  ");
			break;
		case GL_RGBA4:
			common->Printf("RGBA4 ");
			break;
		case GL_RGB5:
			common->Printf("RGB5  ");
			break;
		case 0:
			common->Printf("      ");
			break;
		default:
			common->Printf("<BAD FORMAT:%i>", internalFormat);
			break;
	}

	switch (repeat) {
		case TR_REPEAT:
			common->Printf("rept ");
			break;
		case TR_CLAMP_TO_ZERO:
			common->Printf("zero ");
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			common->Printf("azro ");
			break;
		case TR_CLAMP:
			common->Printf("clmp ");
			break;
		default:
			common->Printf("<BAD REPEAT:%i>", repeat);
			break;
	}

	common->Printf("%4ik ", StorageSize() / 1024);

	common->Printf(" %s\n", imgName.c_str());
}

#ifdef _SHADOW_MAPPING
void		idImage::GenerateShadow2DDepthImage(int width, int height, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm, int component, bool compare)
{
    byte		*scaledBuffer;
    int			scaled_width, scaled_height;

    PurgeImage();

    filter = filterParm;
    //filter = TF_NEAREST;
    allowDownSize = allowDownSizeParm;
    repeat = repeatParm;
    depth = TD_HIGH_QUALITY;

    // if we don't have a rendering context, just return after we
    // have filled in the parms.  We must have the values set, or
    // an image match from a shader before OpenGL starts would miss
    // the generated texture

    if (!glConfig.isInitialized) {
        return;
    }

    // make sure it is a power of 2
    scaled_width = MakePowerOfTwo(width);
    scaled_height = MakePowerOfTwo(height);

    if (scaled_width != width || scaled_height != height) {
        common->Error("GenerateShadow2DDepthImage: not a power of 2 image");
    }

    // Optionally modify our width/height based on options/hardware
    GetDownsize(scaled_width, scaled_height);

    scaledBuffer = NULL;

    // generate the texture number
    qglGenTextures(1, &texnum);

    // select proper internal format before we resample
	GLenum dataType;
	switch (component) {
		case 16:
			internalFormat = GL_DEPTH_COMPONENT16;
			dataType = GL_UNSIGNED_SHORT;
			break;
		case 32:
			internalFormat = GL_DEPTH_COMPONENT32F;
			dataType = GL_FLOAT;
			break;
		case 24:
		default:
			internalFormat = GL_DEPTH_COMPONENT24;
			dataType = GL_UNSIGNED_INT;
			break;
	}

    uploadHeight = scaled_height;
    uploadWidth = scaled_width;
    type = TT_2D;

    // upload the main image level
    Bind();

    SetImageFilterAndRepeat();

    //GL_CheckErrors("GenerateShadow2DDepthImage::start");
    qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0,
                 GL_DEPTH_COMPONENT, dataType, NULL);
    //GL_CheckErrors("GenerateShadow2DDepthImage::end");

#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3 && compare)
	{
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}
#endif
    // see if we messed anything up
    GL_CheckErrors();
}

void		idImage::GenerateShadow2DRGBAImage(int width, int height, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm)
{
	byte		*scaledBuffer;
	int			scaled_width, scaled_height;

	PurgeImage();

	filter = filterParm;
	//filter = TF_NEAREST;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = TD_HIGH_QUALITY;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture

	if (!glConfig.isInitialized) {
		return;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(width);
	scaled_height = MakePowerOfTwo(height);

	if (scaled_width != width || scaled_height != height) {
		common->Error("GenerateShadow2DRGBAImage: not a power of 2 image");
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	scaledBuffer = NULL;

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	internalFormat = GL_RGBA;

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;
	type = TT_2D;

	// upload the main image level
	Bind();

	SetImageFilterAndRepeat();

    //GL_CheckErrors("GenerateShadow2DRGBAImage::start");
	qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    //GL_CheckErrors("GenerateShadow2DRGBAImage::end");

	// see if we messed anything up
	GL_CheckErrors();
}

void idImage::GenerateShadowCubeRGBAImage(int size, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm)
{
	int			scaled_width, scaled_height;
	int			width, height;
	int			i;

	PurgeImage();

	filter = filterParm;
	//filter = TF_NEAREST;
	allowDownSize = allowDownSizeParm;
	depth = TD_HIGH_QUALITY;
	repeat = repeatParm;

	type = TT_CUBIC;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if (!glConfig.isInitialized) {
		return;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(size);
	scaled_height = MakePowerOfTwo(size);

	if (scaled_width != size || scaled_height != size) {
		common->Error("GenerateShadowCubeRGBAImage: not a power of 2 image");
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	width = height = size;

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	internalFormat = GL_RGBA;

	// don't bother with downsample for now
	scaled_width = width;
	scaled_height = height;

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;

	Bind();

	// no other clamp mode makes sense
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R_OES, GL_CLAMP_TO_EDGE);

	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture filter");
	}

	// upload the base level
	// FIXME: support GL_COLOR_INDEX8_EXT?
	//GL_CheckErrors("GenerateShadowCubeRGBAImage::start");
	for (i = 0 ; i < 6 ; i++) {
		qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalFormat, scaled_width, scaled_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		//GL_CheckErrors(va("GenerateShadowCubeRGBAImage::part_%d", i));
	}
	// see if we messed anything up
	GL_CheckErrors();
}

void idImage::GenerateShadowCubeDepthImage(int size, textureFilter_t filterParm, bool allowDownSizeParm, textureRepeat_t repeatParm, int component, bool compare)
{
	int			scaled_width, scaled_height;
	int			width, height;
	int			i;

	PurgeImage();

	filter = filterParm;
	//filter = TF_NEAREST;
	allowDownSize = allowDownSizeParm;
	depth = TD_HIGH_QUALITY;
	repeat = repeatParm;

	type = TT_CUBIC;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if (!glConfig.isInitialized) {
		return;
	}

	// make sure it is a power of 2
	scaled_width = MakePowerOfTwo(size);
	scaled_height = MakePowerOfTwo(size);

	if (scaled_width != size || scaled_height != size) {
		common->Error("GenerateShadowCubeDepthImage: not a power of 2 image");
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	width = height = size;

	// generate the texture number
	qglGenTextures(1, &texnum);

	// select proper internal format before we resample
	GLenum dataType;
	switch (component) {
		case 16:
			internalFormat = GL_DEPTH_COMPONENT16;
			dataType = GL_UNSIGNED_SHORT;
			break;
		case 32:
			internalFormat = GL_DEPTH_COMPONENT32F;
			dataType = GL_FLOAT;
			break;
		case 24:
		default:
			internalFormat = GL_DEPTH_COMPONENT24;
			dataType = GL_UNSIGNED_INT;
			break;
	}

	// don't bother with downsample for now
	scaled_width = width;
	scaled_height = height;

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;

	Bind();

	// no other clamp mode makes sense
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// qglTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R_OES, GL_CLAMP_TO_EDGE);

	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("R_CreateImage: bad texture filter");
	}

	// upload the base level
	//GL_CheckErrors("GenerateShadowCubeDepthImage::start");
	for (i = 0 ; i < 6 ; i++) {
		qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalFormat, scaled_width, scaled_height, 0,
					  GL_DEPTH_COMPONENT, dataType, NULL);
		//GL_CheckErrors(va("GenerateShadowCubeDepthImage::part_%d", i));
	}
#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3 && compare)
	{
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}
#endif
	// see if we messed anything up
	GL_CheckErrors();
}

#ifdef GL_ES_VERSION_3_0
void idImage::GenerateShadowArray( int width, int height, int numSides, textureFilter_t filterParm, textureRepeat_t repeatParm, int component, bool compare )
{
	int			scaled_width, scaled_height;
	int			i;

	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	depth = TD_SHADOW_ARRAY;
	cubeFiles = CF_2D_ARRAY;

	type = TT_2D_ARRAY;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if (!glConfig.isInitialized) {
		return;
	}

	// make sure it is a power of 2
	scaled_width = width;
	scaled_height = height;

	// Optionally modify our width/height based on options/hardware
	GetDownsize(scaled_width, scaled_height);

	scaled_width = width;
	scaled_height = height;
	uploadHeight = scaled_height;
	uploadWidth = scaled_width;

	GLenum dataType;
	switch (component) {
		case 16:
			internalFormat = GL_DEPTH_COMPONENT16;
			dataType = GL_UNSIGNED_SHORT;
			break;
		case 32:
			internalFormat = GL_DEPTH_COMPONENT32F;
			dataType = GL_FLOAT;
			break;
		case 24:
		default:
			internalFormat = GL_DEPTH_COMPONENT24;
			dataType = GL_UNSIGNED_INT;
			break;
	}

	qglGenTextures(1, &texnum);

	Bind();

	// no other clamp mode makes sense
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// set the minimize / maximize filtering
	switch (filter) {
		case TF_DEFAULT:
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter);
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter);
			break;
		case TF_LINEAR:
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case TF_NEAREST:
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		default:
			common->FatalError("GenerateShadowArray: bad texture filter");
	}

	// upload the base level

	//GL_CheckErrors("GenerateShadowCubeRGBAImage::start");
	qglTexImage3D( GL_TEXTURE_2D_ARRAY, 0, internalFormat, scaled_width, scaled_height, numSides, 0, GL_DEPTH_COMPONENT, dataType, NULL );
	//GL_CheckErrors("GenerateShadowCubeRGBAImage::end");

	if(compare)
	{
		qglTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		qglTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}
	// see if we messed anything up

	GL_CheckErrors();
}
#endif
#endif
