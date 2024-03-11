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

#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"
#include <thread>
#include <mutex>          // std::mutex, std::unique_lock, std::defer_lock
#include <stack>
#include <condition_variable>
#include "framework/LoadStack.h"
#include "tests/testing.h"

#ifdef __ANDROID__ //karin: debug glTexImage2D
#if 0
#define myqglTexImage2D(a, b, c, d, e, f, g, h, i) \
{                                             \
qglGetError();                                \
glad_glTexImage2D(a, b, c, d, e, f, g, h, i);        \
r_ignoreGLErrors.SetBool(false);              \
printf("glTexImage2D %s %X %X %X\n", imgName.c_str(), c, g, h); \
GL_CheckErrors();                             \
r_ignoreGLErrors.SetBool(true);                 \
}
#undef qglTexImage2D
#define qglTexImage2D myqglTexImage2D
#endif
#endif

/*
PROBLEM: compressed textures may break the zero clamp rule!
*/

//=======================================================================

static byte	mipBlendColors[16][4] = {
	{0, 0, 0, 0},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
};

/*
===============
SelectInternalFormat

This may need to scan six cube map images
===============
*/
GLenum idImageAsset::SelectInternalFormat( byte const* const* dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth, GLint const* *swizzleMask ) {
#ifdef __ANDROID__ //karin: only GL_RGBA
    return GL_RGBA;
#else
	int			i, c;
	const byte	*scan;
	int			rgbOr, aOr, aAnd;
	int			rgbDiffer, rgbaDiffer;
	const bool allowCompress = globalImages->image_useCompression.GetBool();//&& globalImages->image_preload.GetBool();

	if ( swizzleMask )
		*swizzleMask = nullptr;

	// determine if the rgb channels are all the same
	// and if either all rgb or all alpha are 255
	c = width * height;
	rgbDiffer = 0;
	rgbaDiffer = 0;
	rgbOr = 0;
	aOr = 0;
	aAnd = -1;

	for ( int side = 0 ; side < numDataPtrs ; side++ ) {
		scan = dataPtrs[side];
		for ( i = 0; i < c; i++, scan += 4 ) {
			int		cor, cand;

			aOr |= scan[3];
			aAnd &= scan[3];

			cor = scan[0] | scan[1] | scan[2];
			cand = scan[0] & scan[1] & scan[2];

			// if rgb are all the same, the or and and will match
			rgbDiffer |= ( cor ^ cand );

			rgbOr |= cor;

			cor |= scan[3];
			cand &= scan[3];

			rgbaDiffer |= ( cor ^ cand );
		}
	}

	// we assume that all 0 implies that the alpha channel isn't needed,
	// because some tools will spit out 32 bit images with a 0 alpha instead
	// of 255 alpha, but if the alpha actually is referenced, there will be
	// different behavior in the compressed vs uncompressed states.
	bool needAlpha;

	if ( aAnd == 255 || aOr == 0 ) {
		needAlpha = false;
	} else {
		needAlpha = true;
	}

	// catch normal maps first
	if ( minimumDepth == TD_BUMP ) {
		if ( globalImages->image_useNormalCompression.GetBool() ) {
			return GL_COMPRESSED_RG_RGTC2;
		}
		// discarding blue channel already decreases quality of normal maps,
		// which is not what an artist wants when he disables normal maps compression
		return GL_RGB8;
	}

	// allow a complete override of image compression with a cvar
	if ( !globalImages->image_useCompression.GetBool() ) {
		minimumDepth = TD_HIGH_QUALITY;
	}

	if ( minimumDepth == TD_SPECULAR ) {
		// we are assuming that any alpha channel is unintentional
		if ( allowCompress ) {
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		} else {
			return GL_RGB5;
		}
	}
	if ( minimumDepth == TD_DIFFUSE ) {
		// we might intentionally have an alpha channel for alpha tested textures
		if ( allowCompress ) {
			if ( !needAlpha ) {
				return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			} else {
				return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			}
		} else if ( ( aAnd == 255 || aOr == 0 ) ) {
			return GL_RGB5;
		} else {
			return GL_RGBA4;
		}
	}

	// cases without alpha
	if ( !needAlpha ) {
		if ( minimumDepth == TD_HIGH_QUALITY ) {
			return glConfig.srgb ? GL_SRGB8 : GL_RGB8;	// four bytes
		}
		if ( allowCompress ) {
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;		// half byte
		}
		return glConfig.srgb ? GL_SRGB8 : GL_RGB565;	// two bytes
	}

	// cases with alpha
	if ( !rgbaDiffer ) {
		if ( minimumDepth != TD_HIGH_QUALITY && allowCompress ) {
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;	// one byte
		}
		//return GL_INTENSITY8;							// single byte for all channels
		if ( swizzleMask ) {
			static const GLint SWIZZLE[] = { GL_RED, GL_RED, GL_RED, GL_RED };
			*swizzleMask = SWIZZLE;
		}
		return GL_R8;									// single byte for all channels
	}

	if ( minimumDepth == TD_HIGH_QUALITY ) {
		return GL_RGBA8;								// four bytes
	}
	if ( allowCompress ) {
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;		// one byte
	}
	if ( !rgbDiffer ) {
		//return GL_LUMINANCE8_ALPHA8;					// two bytes, max quality
		if ( swizzleMask ) {
			static const GLint SWIZZLE[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
			*swizzleMask = SWIZZLE;
		}
		return GL_RG8;									// two bytes, max quality
	}
	return GL_RGBA4;									// two bytes
#endif
}

/*
==================
SetImageFilterAndRepeat
==================
*/
void idImage::SetImageFilterAndRepeat() const {
	// set the minimize / maximize filtering
	switch ( filter ) {
		case TF_DEFAULT:
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
			break;
		case TF_LINEAR:
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_NEAREST:
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;
		default:
			common->FatalError( "R_CreateImage: bad texture filter" );
	}

	if ( glConfig.anisotropicAvailable ) {
		// only do aniso filtering on mip mapped images
		if ( filter == TF_DEFAULT ) {
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, globalImages->textureAnisotropy );
		} else {
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
		}
	}
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, globalImages->textureLODBias );

	// set the wrap/clamp modes
	static const int trCtZA[] = { 255,255,255,0 };
	switch ( repeat ) {
		case TR_REPEAT:
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
#if !defined(__ANDROID__) //karin: only GL_CLAMP_TO_EDGE
			qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trCtZA );
#endif
		case TR_CLAMP_TO_ZERO:
#if !defined(__ANDROID__) //karin: only GL_CLAMP_TO_EDGE
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			break;
#endif
		case TR_CLAMP:
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		default:
			common->FatalError( "R_CreateImage: bad texture repeat" );
	}
	if ( swizzleMask != NULL ) {
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
}

/*
================
idImage::Downsize
helper function that takes the current width/height and might make them smaller
================
*/
void idImageAsset::GetDownsize( int &scaled_width, int &scaled_height ) const {
	int size = 0;

	// perform optional picmip operation to save texture memory
	if ( depth == TD_SPECULAR && globalImages->image_downSizeSpecular.GetInteger() ) {
		size = globalImages->image_downSizeSpecularLimit.GetInteger();
		if ( size == 0 ) {
			size = 64;
		}
	} else if ( depth == TD_BUMP && globalImages->image_downSizeBump.GetInteger() ) {
		size = globalImages->image_downSizeBumpLimit.GetInteger();
		if ( size == 0 ) {
			size = 64;
		}
	} else if ( ( allowDownSize || globalImages->image_forceDownSize.GetBool() ) && globalImages->image_downSize.GetInteger() ) {
		size = globalImages->image_downSizeLimit.GetInteger();
		if ( size == 0 ) {
			size = 256;
		}
	}

	if ( size > 0 ) {
		while ( scaled_width > size || scaled_height > size ) {
			if ( scaled_width > 1 ) {
				scaled_width >>= 1;
			}
			if ( scaled_height > 1 ) {
				scaled_height >>= 1;
			}
		}
	}

	// clamp to minimum size
	if ( scaled_width < 1 ) {
		scaled_width = 1;
	}
	if ( scaled_height < 1 ) {
		scaled_height = 1;
	}

	// clamp size to the hardware specific upper limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	// This causes a 512*256 texture to sample down to
	// 256*128 on a voodoo3, even though it could be 256*256
	while ( scaled_width > glConfig.maxTextureSize || scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}
}

bool loading, uploading;

struct Routine {
	bool* b;
	Routine( bool* watch ) : b( watch ) {
		*b = true;
	}
	~Routine() {
		*b = false;
	}
};

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
idCVar image_useTexStorage( "image_useTexStorage", "1", CVAR_BOOL|CVAR_ARCHIVE, "Use glTexStorage to create image storage. Only disable if you run into issues." );
idCVar image_mipmapMode( "image_mipmapMode", "0", CVAR_INTEGER|CVAR_ARCHIVE, "0 - generate mipmaps on CPU, 2 - use glGenerateMipmap." );

void idImageAsset::GenerateImage( const byte *pic, int width, int height,
                             textureFilter_t filterParm, bool allowDownSizeParm,
                             textureRepeat_t repeatParm, textureDepth_t depthParm, imageResidency_t residencyParm
) {
	byte		*scaledBuffer;
	int			scaled_width=width, scaled_height=height;
	byte		*shrunk;

	bool loadingFromItself = (pic == cpuData.pic[0]);
	if (loadingFromItself) {
		// HACK: purge GPU texture, but retain cpu image data
		idImage::PurgeImage();
		assert(pic == cpuData.pic[0]);
	} else {
		PurgeImage();
	}

	precompressedFile = false;

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	repeat = repeatParm;
	depth = depthParm;
	residency = residencyParm;

	if ( residency & IR_CPU ) {
		if ( !loadingFromItself ) {
			cpuData.width = width;
			cpuData.height = height;
			cpuData.pic[0] = ( byte* ) R_StaticAlloc( cpuData.GetSizeInBytes() );
			memcpy(cpuData.pic[0], pic, cpuData.GetSizeInBytes() );
			cpuData.sides = 1;
		}
		assert( cpuData.width == width && cpuData.height == height );
	}
	if ( !(residency & IR_GRAPHICS) )
		return;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !glConfig.isInitialized ) {
		return;
	}

	// Optionally modify our width/height based on options/hardware
	GetDownsize( scaled_width, scaled_height );

	scaledBuffer = NULL;

	// generate the texture number
	qglGenTextures( 1, &texnum );

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat( &pic, 1, width, height, depth, &swizzleMask );

	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && ( scaled_height == height ) ) {
		// we must copy even if unchanged, because the border zeroing
		// would otherwise modify const data
		if ( 1 ) { // duzenko #4401
			scaledBuffer = ( byte * ) pic;
		} else {
			scaledBuffer = ( byte * ) R_StaticAlloc( sizeof( unsigned ) * scaled_width * scaled_height );
			memcpy( scaledBuffer, pic, width * height * 4 );
		}
	} else {
		// resample down as needed (FIXME: this doesn't seem like it resamples anymore!)
		scaledBuffer = R_MipMap( pic, width, height);
		width = idMath::Imax( width >> 1, 1 );
		height = idMath::Imax( height >> 1, 1 );

		while ( width > scaled_width || height > scaled_height ) {
			shrunk = R_MipMap( scaledBuffer, width, height);
			R_StaticFree( scaledBuffer );
			scaledBuffer = shrunk;

			width = idMath::Imax( width >> 1, 1 );
			height = idMath::Imax( height >> 1, 1 );
		}

		// one might have shrunk down below the target size
		scaled_width = width;
		scaled_height = height;
	}
	uploadHeight = scaled_height;
	uploadWidth = scaled_width;
	type = TT_2D;

	if ( source.generatorFunction == NULL && ( ( depth == TD_BUMP && globalImages->image_writeNormalTGA.GetBool() ) || ( depth != TD_BUMP && globalImages->image_writeTGA.GetBool() ) ) ) {
		// Optionally write out the texture to a .tga
		char filename[MAX_IMAGE_NAME];
		ImageProgramStringToCompressedFileName( imgName, filename );
		char *ext = strrchr( filename, '.' );
		if ( ext ) {
			strcpy( ext, ".tga" );
			R_WriteTGA( filename, scaledBuffer, scaled_width, scaled_height, false );
		}
	}

	if ( internalFormat == GL_RG8 && swizzleMask ) // 2.08 swizzle alpha to green, replacement for GL_LUMINANCE8_ALPHA8
		for ( int i = 0; i < scaled_width * scaled_height * 4; i += 4 )
			scaledBuffer[i + 1] = scaledBuffer[i + 3];

	// upload the main image level
	GL_CheckErrors();
	this->Bind();
	GL_CheckErrors();
	GL_SetDebugLabel( GL_TEXTURE, texnum, imgName );

	//stgatilov: OpenGL does not guarantee compressing glTexSubImage to work for sizes not divisible by 4, and AMD driver crashes on it
	//https://forums.thedarkmod.com/index.php?/topic/21073-a-house-of-locked-secrets-crashes-tdm-on-startup/
	bool crossesCompressionBlocks = IsImageFormatCompressed( internalFormat ) && ( scaled_width % 4 || scaled_height % 4 );
	bool useTexStorage = GLAD_GL_ARB_texture_storage && !crossesCompressionBlocks && image_useTexStorage.GetBool();

	//Routine test( &uploading );
	auto start = Sys_Milliseconds();
	int mipLevels = 1 + idMath::ILog2( Max( scaled_width, scaled_height ) );
	if ( useTexStorage ) {
		qglTexStorage2D( GL_TEXTURE_2D, mipLevels, internalFormat, scaled_width, scaled_height);
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, scaled_width, scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
	} else {
		qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
	}

	// stgatilov: uploading mipmaps using glTexImage for compressed NPOT textures results in black textures on AMD drivers
	if ( image_mipmapMode.GetInteger() == 0 && useTexStorage ) {
		TRACE_CPU_SCOPE ("GenerateMipmap" );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipLevels - 1 );
		byte *currentMip = scaledBuffer;
		int w = scaled_width, h = scaled_height;
		for ( int lvl = 1; lvl < mipLevels; lvl++ ) {
			byte *nextMip = R_MipMap( currentMip, w, h );
			w = idMath::Imax( w >> 1, 1 );
			h = idMath::Imax( h >> 1, 1 );
			qglTexSubImage2D( GL_TEXTURE_2D, lvl, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nextMip );
			if ( currentMip != scaledBuffer )
				R_StaticFree( currentMip );
			currentMip = nextMip;
		}
		if ( currentMip != scaledBuffer )
			R_StaticFree( currentMip );
	}
	else {
		TRACE_CPU_SCOPE( "GenerateMipmap" );
		qglGenerateMipmap( GL_TEXTURE_2D );
	}
	GL_CheckErrors();
	backEnd.pc.textureUploadTime += (Sys_Milliseconds() - start);

	if ( scaledBuffer != 0 && pic != scaledBuffer ) { // duzenko #4401
		R_StaticFree( scaledBuffer );
	}
	SetImageFilterAndRepeat();

	// see if we messed anything up
#ifdef _DEBUG
	GL_CheckErrors();
#endif
}

// FBO attachments need specific setup, rarely changed
void idImageScratch::GenerateAttachment( int width, int height, GLenum format, GLenum filter, GLenum wrapMode, int lodLevel ) {
	GLenum dataFormat = GL_RGBA;
	GLenum dataType = GL_FLOAT;
	switch ( format ) {
	case GL_DEPTH32F_STENCIL8:
		dataFormat = GL_DEPTH_STENCIL;
		dataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
	case GL_DEPTH24_STENCIL8:
		dataFormat = GL_DEPTH_STENCIL;
		dataType = GL_UNSIGNED_INT_24_8;
		break;
	case GL_DEPTH_COMPONENT32F: case GL_DEPTH_COMPONENT24: case GL_DEPTH_COMPONENT16: case GL_DEPTH_COMPONENT:
		dataFormat = GL_DEPTH_COMPONENT;
		break;
	case GL_RED: case GL_R8: case GL_R16: case GL_R16F: case GL_R32F:
		dataFormat = GL_RED;
		break;
	case GL_RG: case GL_RG8: case GL_RG16: case GL_RG16F: case GL_RG32F:
		dataFormat = GL_RG;
		break;
	case GL_RGB: case GL_RGB8: case GL_RGB16: case GL_RGB16F: case GL_RGB32F:
#ifdef __ANDROID__ //karin: only GL_RGBA
case GL_RGBA: case GL_RGBA8:
            dataFormat = GL_RGBA;
            dataType = GL_UNSIGNED_BYTE;
#else
            dataFormat = GL_RGB;
#endif
		break;
	}

	if ( texnum == TEXTURE_NOT_LOADED ) {
		qglGenTextures( 1, &texnum );
	}
	type = TT_2D;
	this->Bind();
	GL_SetDebugLabel( GL_TEXTURE, texnum, imgName );

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode );
	qglTexImage2D( GL_TEXTURE_2D, lodLevel, format, width, height, 0, dataFormat, dataType, nullptr );

	internalFormat = format;

	// update base/max LOD level
	int oldBaseLevel = 0, oldMaxLevel = 1000;
	qglGetTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &oldBaseLevel );
	qglGetTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &oldMaxLevel );
	if ( oldMaxLevel == 1000 ) {
		// first call: set both base and max to this single level
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &lodLevel );
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &lodLevel );
		// save size of level 0 texture in members
		uploadWidth = width << lodLevel;
		uploadHeight = height << lodLevel;
	} else if ( oldBaseLevel > lodLevel ) {
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &lodLevel );
	} else if ( oldMaxLevel < lodLevel ) {
		qglTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &lodLevel );
	}

	if ( lodLevel == 0 ) {
		// this must be recreation of image without purging it first
		// make sure to assign width/height
		uploadWidth = width;
		uploadHeight = height;
		// note: if your image does not have 0-th LOD level, then you MUST purge it!
		// otherwise, these members won't update with bad consequences
	}
}

/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void idImageAsset::GenerateCubeImage( const byte *pic[6], int size,
                                 textureFilter_t filterParm, bool allowDownSizeParm,
                                 textureDepth_t depthParm ) {
	int			scaled_width, scaled_height;
	int			width, height;
	int			i;

	bool loadingFromItself = (pic[0] == cpuData.pic[0]);
	if (loadingFromItself) {
		// HACK: purge GPU texture, but retain cpu image data
		idImage::PurgeImage();
		assert(pic[0] == cpuData.pic[0]);
	} else {
		PurgeImage();
	}

	precompressedFile = false;

	filter = filterParm;
	allowDownSize = allowDownSizeParm;
	depth = depthParm;

	type = TT_CUBIC;

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if ( !glConfig.isInitialized ) {
		return;
	}
	width = height = size;

	// generate the texture number
	qglGenTextures( 1, &texnum );

	// select proper internal format before we resample
	internalFormat = SelectInternalFormat( pic, 6, width, height, depth, &swizzleMask );

	// don't bother with downsample for now
	scaled_width = width;
	scaled_height = height;

	uploadHeight = scaled_height;
	uploadWidth = scaled_width;

	this->Bind();
	GL_SetDebugLabel( GL_TEXTURE, texnum, imgName );

	// no other clamp mode makes sense
	qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// set the minimize / maximize filtering
	switch ( filter ) {
		case TF_DEFAULT:
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
			break;
		case TF_LINEAR:
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_NEAREST:
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			qglTexParameterf( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;
		default:
			common->FatalError( "R_CreateImage: bad texture filter" );
	}

	// upload the base level
	for ( i = 0 ; i < 6 ; i++ ) {
		qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, scaled_width, scaled_height, 0,
		               GL_RGBA, GL_UNSIGNED_BYTE, pic[i] );
	}

	// create and upload the mip map levels
	int		miplevel;
	byte	*shrunk[6];

	for ( i = 0 ; i < 6 ; i++ ) {
		shrunk[i] = R_MipMap( pic[i], scaled_width, scaled_height);
	}
	miplevel = 1;

	while ( scaled_width > 1 || scaled_height > 1 ) {
		int newWidth  = idMath::Imax(scaled_width  >> 1, 1);
		int newHeight = idMath::Imax(scaled_height >> 1, 1);
		for ( i = 0 ; i < 6 ; i++ ) {
			byte	*shrunken;
			qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, miplevel, internalFormat,
			               newWidth, newHeight, 0,
			               GL_RGBA, GL_UNSIGNED_BYTE, shrunk[i] );

			if ( newWidth > 1 || newHeight > 1 ) {
				shrunken = R_MipMap( shrunk[i], newWidth, newHeight );
			} else {
				shrunken = NULL;
			}
			R_StaticFree( shrunk[i] );
			shrunk[i] = shrunken;
		}
		scaled_width = newWidth;
		scaled_height = newHeight;
		miplevel++;
	}
	for ( i = 0; i < 6; i++ ) assert(shrunk[i] == NULL);	//check for leak

#ifdef _DEBUG
	// see if we messed anything up
	GL_CheckErrors();
#endif
}

/*
================
ImageProgramStringToFileCompressedFileName
================
*/
/*
Serpentine : This should be adapted to better allow dds files and normal textures into a single tree.
*/
void idImageAsset::ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const {
	const char	*s;
	char	*f;

	strcpy( fileName, "dds/" );
	f = fileName + strlen( fileName );

	// convert all illegal characters to underscores
	// this could conceivably produce a duplicated mapping, but we aren't going to worry about it
	for ( s = imageProg ; *s ; s++ ) {
		// tels: TODO: Why the strange difference between "/ ", ')', ',' and all the other special characters?
		if ( *s == '/' || *s == '\\' || *s == '(' ) {
			*f = '/';
			f++;
		} else if ( *s == '<' || *s == '>' || *s == ':' || *s == '|' || *s == '"' || *s == '.' ) {
			*f = '_';
			f++;
		} else if ( *s == ' ' && *( f - 1 ) == '/' ) {	// ignore a space right after a slash
		} else if ( *s == ')' || *s == ',' ) {			// always ignore these
		} else {
			*f = *s;
			f++;
		}
	}
	*f++ = 0;
	strcat( fileName, ".dds" );
}

/*
==================
NumLevelsForImageSize
==================
*/
int	idImage::NumLevelsForImageSize( int width, int height ) {
	int	numLevels = 1;

	while ( width > 1 || height > 1 ) {
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
/*
Serpentine : This should either be refactored to be useful (comp location configurable) or removed. If preserved, it should also be simplified.
*/
void idImageAsset::WritePrecompressedImage() {

	// Always write the precompressed image if we're making a build
	if ( !com_makingBuild.GetBool() ) {
		if ( !globalImages->image_writePrecompressedTextures.GetBool() || !globalImages->image_usePrecompressedTextures.GetBool() ) {
			return;
		}
	}

	if ( !glConfig.isInitialized ) {
		return;
	}
	char filename[MAX_IMAGE_NAME];

	ImageProgramStringToCompressedFileName( imgName, filename );

	int numLevels = NumLevelsForImageSize( uploadWidth, uploadHeight );
	if ( numLevels > MAX_TEXTURE_LEVELS ) {
		common->Warning( "R_WritePrecompressedImage: level > MAX_TEXTURE_LEVELS for image %s", filename );
		return;
	}

	// glGetTexImage only supports a small subset of all the available internal formats
	// We have to use BGRA because DDS is a windows based format
	int altInternalFormat = 0;
	int bitSize = 0;
	switch ( internalFormat ) {
	case 1:
	//case GL_INTENSITY8:
	case GL_R8:
	case 3:
	case GL_RGB8:
		altInternalFormat = GL_BGR_EXT;
		bitSize = 24;
		break;
	//case GL_LUMINANCE8_ALPHA8:
	case GL_RG8:
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
		if ( IsImageFormatCompressed( internalFormat ) ) {
			altInternalFormat = internalFormat;
		} else {
			common->Warning( "Unknown or unsupported format for %s", filename );
			return;
		}
	}

	if ( globalImages->image_useOffLineCompression.GetBool() && IsImageFormatCompressed( altInternalFormat ) ) {
		idStr outFile = fileSystem->RelativePathToOSPath( filename, "fs_basepath" );
		idStr inFile = outFile;
		inFile.StripFileExtension();
		inFile.SetFileExtension( "tga" );
		idStr format;
		if ( depth == TD_BUMP ) {
			format = "RXGB +red 0.0 +green 0.5 +blue 0.5";
		} else {
			switch ( altInternalFormat ) {
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
		globalImages->AddDDSCommand( va( "z:/d3xp/compressonator/thecompressonator -convert \"%s\" \"%s\" %s -mipmaps\n", inFile.c_str(), outFile.c_str(), format.c_str() ) );
		return;
	}
	ddsFileHeader_t header;
	memset( &header, 0, sizeof( header ) );
	header.dwSize = sizeof( header );
	header.dwFlags = DDSF_CAPS | DDSF_PIXELFORMAT | DDSF_WIDTH | DDSF_HEIGHT;
	header.dwHeight = uploadHeight;
	header.dwWidth = uploadWidth;

	if ( IsImageFormatCompressed( altInternalFormat ) ) {
		// size (in bytes) of the compressed base image
		header.dwFlags |= DDSF_LINEARSIZE;
		header.dwPitchOrLinearSize = ( ( uploadWidth + 3 ) / 4 ) * ( ( uploadHeight + 3 ) / 4 ) *
		                             ( altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
	} else {
		// 4 Byte aligned line width (from nv_dds)
		header.dwFlags |= DDSF_PITCH;
		header.dwPitchOrLinearSize = ( ( uploadWidth * bitSize + 31 ) & -32 ) >> 3;
	}
	header.dwCaps1 = DDSF_TEXTURE;

	if ( numLevels > 1 ) {
		header.dwMipMapCount = numLevels;
		header.dwFlags |= DDSF_MIPMAPCOUNT;
		header.dwCaps1 |= DDSF_MIPMAP | DDSF_COMPLEX;
	}
	header.ddspf.dwSize = sizeof( header.ddspf );

	if ( IsImageFormatCompressed( altInternalFormat ) ) {
		header.ddspf.dwFlags = DDSF_FOURCC;
		switch ( altInternalFormat ) {
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '1' );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
			header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '1' );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '3' );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '5' );
			break;
		}
	} else {
		header.ddspf.dwFlags = DDSF_RGB;
		header.ddspf.dwRGBBitCount = bitSize;
		switch ( altInternalFormat ) {
		case GL_BGRA_EXT:
		case GL_LUMINANCE_ALPHA:
			header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
			header.ddspf.dwABitMask = 0xFF000000;
		// Fall through
		case GL_BGR_EXT:
		case GL_LUMINANCE:
			header.ddspf.dwRBitMask = 0x00FF0000;
			header.ddspf.dwGBitMask = 0x0000FF00;
			header.ddspf.dwBBitMask = 0x000000FF;
			break;
		case GL_ALPHA:
			header.ddspf.dwFlags = DDSF_ALPHAPIXELS;
			header.ddspf.dwABitMask = 0xFF000000;
			break;
		default:
			common->Warning( "Unknown or unsupported format for %s", filename );
			return;
		}
	}
	idFile *f = fileSystem->OpenFileWrite( filename );

	if ( f == NULL ) {
		common->Warning( "Could not open %s trying to write precompressed image", filename );
		return;
	}
	common->Printf( "Writing precompressed image: %s\n", filename );

	f->Write( "DDS ", 4 );
	f->Write( &header, sizeof( header ) );

	// bind to the image so we can read back the contents
	this->Bind();

	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits

	int uw = uploadWidth;
	int uh = uploadHeight;

	// Will be allocated first time through the loop
	byte *data = NULL;

	for ( int level = 0 ; level < numLevels ; level++ ) {

		int size = 0;
		if ( IsImageFormatCompressed( altInternalFormat ) ) {
			size = ( ( uw + 3 ) / 4 ) * ( ( uh + 3 ) / 4 ) *
			       ( altInternalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
		} else {
			size = uw * uh * ( bitSize / 8 );
		}

		if ( data == NULL ) {
			data = ( byte * )R_StaticAlloc( size );
		}

		if ( IsImageFormatCompressed( altInternalFormat ) ) {
			qglGetCompressedTexImage( GL_TEXTURE_2D, level, data );
		} else {
			qglGetTexImage( GL_TEXTURE_2D, level, altInternalFormat, GL_UNSIGNED_BYTE, data );
		}
		f->Write( data, size );

		uw /= 2;
		uh /= 2;
		if ( uw < 1 ) {
			uw = 1;
		}
		if ( uh < 1 ) {
			uh = 1;
		}
	}

	if ( data != NULL ) {
		R_StaticFree( data );
	}
	fileSystem->CloseFile( f );
}

/*
================
CheckPrecompressedImage

If fullLoad is false, only the small mip levels of the image will be loaded
================
*/
bool idImageAsset::CheckPrecompressedImage( bool fullLoad ) {
	if ( !glConfig.isInitialized ) {
		return false;
	}

	// Allow grabbing of DDS's from original Doom pak files
	// compressed light images may look ugly
	if ( imgName.Icmpn( "lights/", 7 ) == 0 ) {
		return false;
	}
	char filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName( imgName, filename );

	// get the file timestamp
	ID_TIME_T precompTimestamp;
	fileSystem->ReadFile( filename, NULL, &precompTimestamp );
	if ( precompTimestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		return false;
	}
	if ( !source.generatorFunction && timestamp != FILE_NOT_FOUND_TIMESTAMP ) {
		if ( precompTimestamp < timestamp ) {
			// The image has changed after being precompressed
			return false;
		}
	}
	timestamp = precompTimestamp;

	// load compressed data from file
	R_StaticFree( compressedData );
	R_LoadCompressedImage( filename, &compressedData, nullptr );
	if ( !compressedData )
		return false;
	cpuData.Purge();

	if ( idImageManager::image_forceRecompress.GetBool() ) {
		// debug only: decompress DDS on read, so that we can test our compression code
		cpuData.sides = 1;
		cpuData.pic[0] = compressedData->ComputeUncompressedData();
		cpuData.width = compressedData->GetWidth();
		cpuData.height = compressedData->GetHeight();
		R_StaticFree( compressedData );
		compressedData = nullptr;
	}

	return true;
}

/*
===================
UploadPrecompressedImage

This can be called by the front end during normal loading,
or by the backend after a background read of the file
has completed
===================
*/
void idImageAsset::UploadPrecompressedImage() {
	if ( !glConfig.isInitialized ) {
		return;
	}

	ddsFileHeader_t	*header = &compressedData->header;

	// generate the texture number
	qglGenTextures( 1, &texnum );

	int externalFormat = 0;

	precompressedFile = true;

	uploadWidth = header->dwWidth;
	uploadHeight = header->dwHeight;
	if ( header->ddspf.dwFlags & DDSF_FOURCC ) {
		switch ( header->ddspf.dwFourCC ) {
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '1' ):
			if ( header->ddspf.dwFlags & DDSF_ALPHAPIXELS ) {
				internalFormat = glConfig.srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			} else {
				internalFormat = glConfig.srgb ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			break;
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '3' ):
			internalFormat = glConfig.srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case DDS_MAKEFOURCC( 'D', 'X', 'T', '5' ):
			internalFormat = glConfig.srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		case DDS_MAKEFOURCC( 'R', 'X', 'G', 'B' ):
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		case DDS_MAKEFOURCC( 'A', 'T', 'I', '1' ):
			internalFormat = GL_COMPRESSED_LUMINANCE_LATC1_EXT;
			break;
		case DDS_MAKEFOURCC( 'A', 'T', 'I', '2' ):
			internalFormat = GL_COMPRESSED_RG_RGTC2;
			break;
		default:
			common->Warning( "Invalid compressed internal format: %s", imgName.c_str() );
			return;
		}
	} else if ( ( header->ddspf.dwFlags & DDSF_RGBA ) && header->ddspf.dwRGBBitCount == 32 ) {
		externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	} else if ( ( header->ddspf.dwFlags & DDSF_RGB ) && header->ddspf.dwRGBBitCount == 32 ) {
		externalFormat = GL_BGRA_EXT;
		internalFormat = GL_RGBA8;
	} else if ( ( header->ddspf.dwFlags & DDSF_RGB ) && header->ddspf.dwRGBBitCount == 24 ) {
		externalFormat = GL_BGR_EXT;
		internalFormat = GL_RGB8;
	} else if ( header->ddspf.dwRGBBitCount == 8 ) {
		externalFormat = GL_ALPHA;
		internalFormat = GL_ALPHA8;
	} else {
		common->Warning( "Invalid uncompressed internal format: %s", imgName.c_str() );
		return;
	}
	type = TT_2D;			// FIXME: we may want to support pre-compressed cube maps in the future

	this->Bind();
	GL_SetDebugLabel( GL_TEXTURE, texnum, imgName );

	int numMipmaps = 1;
	if ( header->dwFlags & DDSF_MIPMAPCOUNT ) {
		numMipmaps = header->dwMipMapCount;
	}
	int uw = uploadWidth;
	int uh = uploadHeight;

	// We may skip some mip maps if we are downsizing
	int skipMip = 0;
	GetDownsize( uploadWidth, uploadHeight );

	byte *imagedata = compressedData->contents;

	for ( int i = 0 ; i < numMipmaps; i++ ) {
		int size = 0;
		if ( IsImageFormatCompressed( internalFormat ) ) {
			size = ( ( uw + 3 ) / 4 ) * ( ( uh + 3 ) / 4 ) *
			       ( internalFormat <= GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
			if(glConfig.srgb)
				size = ( ( uw + 3 ) / 4 ) * ( ( uh + 3 ) / 4 ) *
				( internalFormat <= GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ? 8 : 16 );
		} else {
			size = uw * uh * ( header->ddspf.dwRGBBitCount / 8 );
		}

		if ( uw > uploadWidth || uh > uploadHeight ) {
			skipMip++;
		} else {
			//Routine test( &uploading );
			auto start = Sys_Milliseconds();
			if ( IsImageFormatCompressed( internalFormat ) ) {
				qglCompressedTexImage2D( GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, size, imagedata );
			} else {
				qglTexImage2D( GL_TEXTURE_2D, i - skipMip, internalFormat, uw, uh, 0, externalFormat, GL_UNSIGNED_BYTE, imagedata );
			}
			backEnd.pc.textureUploadTime += (Sys_Milliseconds() - start);
		}
		imagedata += size;

		uw /= 2;
		uh /= 2;
		if ( uw < 1 ) {
			uw = 1;
		}
		if ( uh < 1 ) {
			uh = 1;
		}
	}

	// ensure mip levels are properly set or generated if missing
	int actualMips = numMipmaps - skipMip;
	if ( actualMips > 1 ) {
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, actualMips - 1 );
	} else {
		qglGenerateMipmap( GL_TEXTURE_2D );
	}

	SetImageFilterAndRepeat();
}

void R_HandleImageCompression( idImageAsset& image ) {
	if ( !(image.residency & IR_GRAPHICS) )
		return;		// not needed on GPU, so no point in compressing
	if ( image.compressedData )
		return;		// output data already computed/provided
	if ( !image.cpuData.IsValid() )
		return;		// have no input data?

	GLenum internalFormat = idImageAsset::SelectInternalFormat( image.cpuData.pic, image.cpuData.sides, image.cpuData.width, image.cpuData.height, image.depth );
	if ( !IsImageFormatCompressed(internalFormat) )
		return;		// don't want to compress it

	// proper DXT1 with alpha can only be prepared by artist, never compress to it automatically
	assert( internalFormat != GL_COMPRESSED_RGBA_S3TC_DXT1_EXT );

	TRACE_CPU_SCOPE_STR("Compress:Image", image.imgName)
	int blockSize = SizeOfCompressedImage(4, 4, internalFormat);

	// Compute size of compressed output
	int levw = image.cpuData.width;
	int levh = image.cpuData.height;
	int totalBytes = 0, mainBytes = 0;
	int mipLevels = 0;
	while (1) {
		int bw = (levw + 3) >> 2;
		int bh = (levh + 3) >> 2;
		int currSz = bw * bh * blockSize;
		if (mainBytes == 0)
			mainBytes = currSz;
		mipLevels++;
		totalBytes += currSz;
		if (levw == 1 && levh == 1)
			break;
		levw = idMath::Imax(levw >> 1, 1);
		levh = idMath::Imax(levh >> 1, 1);
	}

	// Allocate DDS data
	int allocSize = imageCompressedData_t::TotalSizeFromContentSize(totalBytes);
	imageCompressedData_t *compData = (imageCompressedData_t*)R_StaticAlloc(allocSize);
	// Fill header
	compData->fileSize = imageCompressedData_t::FileSizeFromContentSize(totalBytes);
	compData->magic = DDS_MAKEFOURCC( 'D', 'D', 'S', ' ' );
	memset(&compData->header, 0, sizeof(compData->header));
	compData->header.dwSize = sizeof(compData->header);
	compData->header.dwFlags = DDSF_CAPS | DDSF_PIXELFORMAT | DDSF_WIDTH | DDSF_HEIGHT;
	compData->header.dwFlags |= DDSF_LINEARSIZE | DDSF_MIPMAPCOUNT;
	compData->header.dwWidth = image.cpuData.width;
	compData->header.dwHeight = image.cpuData.height;
	compData->header.dwPitchOrLinearSize = mainBytes;
	compData->header.dwMipMapCount = mipLevels;
	compData->header.ddspf.dwSize = sizeof(compData->header.ddspf);
	compData->header.ddspf.dwFlags = DDSF_FOURCC;
	compData->header.dwCaps1 = DDSF_TEXTURE | DDSF_MIPMAP | DDSF_COMPLEX;

	if (internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) {
		compData->header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '1' );
	} else if (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) {
		compData->header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '3' );
		compData->header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
	} else if (internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) {
		compData->header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'D', 'X', 'T', '5' );
		compData->header.ddspf.dwFlags |= DDSF_ALPHAPIXELS;
	} else if (internalFormat == GL_COMPRESSED_RG_RGTC2) {
		compData->header.ddspf.dwFourCC = DDS_MAKEFOURCC( 'A', 'T', 'I', '2' );
	} else {
		common->Error("R_HandleImageCompression: unknown compressed image format %d", internalFormat);
	}

	// Generate mipmaps and compress them
	byte *dstData = compData->contents;
	byte *srcData = image.cpuData.GetPic();
	levw = image.cpuData.width;
	levh = image.cpuData.height;
	while (1) {
		int bw = (levw + 3) >> 2;
		int bh = (levh + 3) >> 2;
		CompressImage(
			internalFormat,
			dstData, srcData, levw, levh
		);
		dstData += bw * bh * blockSize;
		if (levw == 1 && levh == 1)
			break;
		byte *newMip = R_MipMap(srcData, levw, levh);
		levw = idMath::Imax(levw >> 1, 1);
		levh = idMath::Imax(levh >> 1, 1);
		if (srcData != image.cpuData.GetPic())
			R_StaticFree(srcData);
		srcData = newMip;
	} 
	if (srcData != image.cpuData.GetPic())
		R_StaticFree(srcData);

	// Save compressed DDS in image
	assert(!image.compressedData);
	image.compressedData = compData;
}

void R_LoadImageData( idImageAsset& image ) {
	TRACE_CPU_SCOPE_STR( "Load:Image", image.imgName )
	imageBlock_t& cpuData = image.cpuData;

	if ( image.source.generatorFunction ) {
		// this is the ONLY place generatorFunction will ever be called
		// Note from SteveL: Not true. generatorFunction is called during image reloading too.
		cpuData.Purge();
		cpuData = image.source.generatorFunction();
	}
	else {
		if ( image.source.cubeFiles != CF_2D ) {
			cpuData.Purge();
			cpuData.sides = 6;

			// we don't check for pre-compressed cube images currently
			R_LoadImageProgramCubeMap( image.imgName, image.source.cubeFiles, cpuData.pic, &cpuData.width, &image.timestamp );
			cpuData.height = cpuData.width;
			TRACE_ATTACH_FORMAT( "cube %d x [%d x %d]", cpuData.sides, cpuData.width, cpuData.height );
	
			if ( cpuData.pic[0] == NULL ) {
				//note: warning will be printed in R_UploadImageData due to cpuData.pic[0] == NULL
				return;
			}
		}
		else {
			// see if we have a pre-generated image file that is
			// already image processed and compressed
			if ( globalImages->image_usePrecompressedTextures.GetBool() && !(image.residency & IR_CPU) ) {
				if ( image.CheckPrecompressedImage( true ) ) {
					if ( !image.compressedData && image.cpuData.IsValid() )	// image_forceRecompress --- debug only
						goto normalImageLoaded;
					// we got the precompressed image
					const char *fourcc = image.compressedData->header.dwFlags & DDSF_FOURCC ? (char*)&image.compressedData->header.ddspf.dwFourCC : "    ";
					TRACE_ATTACH_FORMAT( "DDS %d x %d (%c%c%c%c)", image.compressedData->header.dwWidth, image.compressedData->header.dwHeight, fourcc[0], fourcc[1], fourcc[2], fourcc[3] );
					return;
				}
				// fall through to load the normal image
			}
			cpuData.Purge();
			R_LoadImageProgram( image.imgName, &cpuData.pic[0], &cpuData.width, &cpuData.height, &image.timestamp, &image.depth );
			cpuData.sides = 1;
		normalImageLoaded:
			TRACE_ATTACH_FORMAT( "%d x %d", cpuData.width, cpuData.height );
		}
	}

	// stgatilov: software compression/decompression of texture if needed
	R_HandleImageCompression( image );
}

void R_UploadImageData( idImageAsset& image ) {
	TRACE_CPU_SCOPE_STR("Upload:Image", image.imgName)
	auto& cpuData = image.cpuData;

	// check if all sides of uncompressed image are available
	bool cpuDataValid = false;
	if (cpuData.sides > 0) {
		cpuDataValid = true;
		for (int s = 0; s < cpuData.sides; s++)
			if ( cpuData.pic[s] == NULL )
				cpuDataValid = false;
	}

	int loadedMask = IR_NONE;

	// verify CPU-side availability
	if (image.residency & IR_CPU) {
		if (cpuDataValid)
			loadedMask |= IR_CPU;
	}

	// upload to GPU side
	if (image.residency & IR_GRAPHICS) {
		if (cpuDataValid && cpuData.IsCubemap()) {
			image.GenerateCubeImage( ( const byte ** )cpuData.pic, cpuData.width, image.filter, image.allowDownSize, image.depth );
			loadedMask |= IR_GRAPHICS;
		}
		else {
			if ( image.compressedData ) {
				// upload all the levels
				image.UploadPrecompressedImage();
				loadedMask |= IR_GRAPHICS;
			}
			else if ( cpuDataValid ) {
				// build a hash for checking duplicate image files
				// NOTE: takes about 10% of image load times (SD)
				// may not be strictly necessary, but some code uses it, so let's leave it in
				image.GenerateImage( cpuData.pic[0], cpuData.width, cpuData.height, image.filter, image.allowDownSize, image.repeat, image.depth, image.residency );
				loadedMask |= IR_GRAPHICS;
				// write out the precompressed version of this file if needed
				image.WritePrecompressedImage();
			}
		}
	}

	if (loadedMask != image.residency) {
		common->Warning( "Couldn't load image: %s", image.imgName.c_str() );
		if (image.loadStack)
			image.loadStack->PrintStack(2, LoadStack::LevelOf(&image));
		image.MakeDefault();
		return;
	}

	if (!(image.residency & IR_CPU))
		cpuData.Purge();
	if (image.compressedData) {
		R_StaticFree(image.compressedData);
		image.compressedData = nullptr;
	}
}

/*
===============
ActuallyLoadImage

Absolutely every image goes through this path
On exit, the idImage will have a valid OpenGL texture number that can be bound
===============
*/
void idImageAsset::ActuallyLoadImage( void ) {
	if ( session->IsFrontend() && !(residency & IR_CPU) ) {
		common->Printf( "Trying to load image %s from frontend, deferring...\n", imgName.c_str() );
		return;
	}

	// load the image from disk
	R_LoadImageData( *this );
	R_UploadImageData( *this );
}

//=========================================================================================================

/*
===============
PurgeImage
===============
*/
void idImage::PurgeImage() {
	if ( texnum != TEXTURE_NOT_LOADED ) {
		// sometimes is NULL when exiting with an error
		if ( qglDeleteTextures ) {
			qglDeleteTextures( 1, &texnum );	// this should be the ONLY place it is ever called!
		}
		texnum = static_cast< GLuint >( TEXTURE_NOT_LOADED );
	}

	// clear all the current binding caches, so the next bind will do a real one
	for ( int i = 0 ; i < MAX_MULTITEXTURE_UNITS ; i++ ) {
		backEnd.glState.tmu[i].current2DMap = -1;
		backEnd.glState.tmu[i].currentCubeMap = -1;
	}
}

void idImageAsset::PurgeImage() {
	idImage::PurgeImage();

	if ( cpuData.IsValid() )
		cpuData.Purge();
	if ( compressedData ) {
		R_StaticFree( compressedData );
		compressedData = nullptr;
	}

	delete loadStack;
	loadStack = nullptr;
}

/*
==============
Bind

Automatically enables 2D mapping, cube mapping, or 3D texturing if needed
==============
*/
void idImage::Bind() {
	// load the image if necessary
	if ( texnum == TEXTURE_NOT_LOADED && GetType() == IT_ASSET ) {
		auto start = Sys_Milliseconds();
		AsAsset()->ActuallyLoadImage();	// check for precompressed, load is from back end
		backEnd.pc.textureLoadTime += (Sys_Milliseconds() - start);
		backEnd.pc.textureLoads++;
	}

	if ( texnum == TEXTURE_NOT_LOADED ) {
		globalImages->whiteImage->Bind();
		return;
	}

	// bump our statistic counters
	if ( r_showPrimitives.GetBool() && backEnd.viewDef && !backEnd.viewDef->IsLightGem() ) { // backEnd.viewDef is null when changing map
		frameUsed = backEnd.frameCount;
		bindCount++;
	} else
		if ( com_developer.GetBool() ) // duzenko: very rarely used, but sometimes without r_showPrimitives. Otherwise avoid unnecessary memory writes
			bindCount++;
	tmu_t *tmu = &backEnd.glState.tmu[backEnd.glState.currenttmu];

	// bind the texture
	if ( type == TT_2D ) {
		if ( tmu->current2DMap != texnum ) {
			tmu->current2DMap = texnum;
			qglBindTexture( GL_TEXTURE_2D, texnum );
		}
	} else if ( type == TT_CUBIC ) {
		if ( tmu->currentCubeMap != texnum ) {
			tmu->currentCubeMap = texnum;
			qglBindTexture( GL_TEXTURE_CUBE_MAP, texnum );
		}
	}
}

bool idImage::IsBound( int textureUnit ) const {
	if ( texnum == TEXTURE_NOT_LOADED ) {
		return false;
	}

	tmu_t *tmu = &backEnd.glState.tmu[textureUnit];
	if ( type == TT_2D ) {
		return tmu->current2DMap == texnum;
	} else if ( type == TT_CUBIC ) {
		return tmu->currentCubeMap == texnum;
	}

	return false;
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void idImageScratch::UploadScratch( const byte *data, int cols, int rows ) {
	int			i;

	// if rows = cols * 6, assume it is a cube map animation
	if ( rows == cols * 6 ) {
		if ( type != TT_CUBIC ) {
			type = TT_CUBIC;
			uploadWidth = -1;	// for a non-sub upload
		}
		this->Bind();

		rows /= 6;

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if ( cols != uploadWidth || rows != uploadHeight ) {
			uploadWidth = cols;
			uploadHeight = rows;

			// upload the base level
			for ( i = 0 ; i < 6 ; i++ ) {
#ifdef __ANDROID__ //karin: only GL_RGBA
				qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, cols, rows, 0,
				               GL_RGBA, GL_UNSIGNED_BYTE, data + cols * rows * 4 * i );
#else
                qglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, cols, rows, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, data + cols * rows * 4 * i );
#endif
			}
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			for ( i = 0 ; i < 6 ; i++ ) {
				qglTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, cols, rows,
				                  GL_RGBA, GL_UNSIGNED_BYTE, data + cols * rows * 4 * i );
			}
		}
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		// no other clamp mode makes sense
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	} else {
		// otherwise, it is a 2D image
		if ( type != TT_2D ) {
			type = TT_2D;
			uploadWidth = -1;	// for a non-sub upload
		}

		if ( texnum == idImage::TEXTURE_NOT_LOADED ) {
			// load for first data upload
#ifdef __ANDROID__ //karin: only GL_RGBA
			GenerateAttachment( rows, cols, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE );
#else
            GenerateAttachment( rows, cols, GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE );
#endif
		}
		this->Bind();

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if ( cols != uploadWidth || rows != uploadHeight ) {
			uploadWidth = cols;
			uploadHeight = rows;
#ifdef __ANDROID__ //karin: only GL_RGBA
			qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
#else
            qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
#endif
		} else {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		// these probably should be clamp, but we have a lot of issues with editor
		// geometry coming out with texcoords slightly off one side, resulting in
		// a smear across the entire polygon
#if 1
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
#else
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif
	}
}

/*
==================
SetClassification
==================
*/
void idImageAsset::SetClassification( int tag ) {
	classification = tag;
}

/*
==================
StorageSize

stgatilov: only GPU memory estimate is returned here.
Note that CPU-resident textures also occupy CPU memory, but it is not reported.
==================
*/
int idImage::StorageSize() const {
	int		baseSize;

	if ( texnum == TEXTURE_NOT_LOADED ) {
		return 0;
	}

	switch ( type ) {
		default:
		case TT_2D:
			baseSize = uploadWidth * uploadHeight;
			break;
		case TT_CUBIC:
			baseSize = 6 * uploadWidth * uploadHeight;
			break;
	}
	baseSize *= BitsForInternalFormat( internalFormat, true );

	baseSize /= 8;

	// account for mip mapping
	if ( imgName[0] != '_' )
		baseSize = baseSize * 4 / 3;

	return baseSize;
}

/*
==================
Print
==================
*/
void idImage::Print() const {

	switch ( type ) {
		case TT_2D:
			common->Printf( " " );
			break;
		case TT_CUBIC:
			common->Printf( "C" );
			break;
		default:
			common->Printf( "<BAD TYPE:%i>", type );
			break;
	}

	common->Printf( "%4i %4i ",	uploadWidth, uploadHeight );

	switch ( internalFormat ) {
	// --- color uncompressed ---
		case GL_R8:
			common->Printf( "R8     " );
			break;
		case GL_RG8:
			common->Printf( "RG8    " );
			break;
		case GL_RGB8:
			common->Printf( "RGB8   " );
			break;
		case GL_RGBA8:
			common->Printf( "RGBA8  " );
			break;
		case GL_RGBA16F:
			common->Printf( "RGBA16F" );
			break;
		case GL_R32F:
			common->Printf( "R32F   " );
			break;
		case GL_RGB5:
			common->Printf( "RGB5   " );
			break;
		case GL_RGB565:
			common->Printf( "RGB565 " );
			break;
		case GL_RGBA4:
			common->Printf( "RGBA4  " );
			break;
		case GL_ALPHA8:
			common->Printf( "A8     " );
			break;
	// --- color compressed ---
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			common->Printf( "DXT1   " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			common->Printf( "DXT1A  " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			common->Printf( "DXT3   " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			common->Printf( "DXT5   " );
			break;
		case GL_COMPRESSED_RG_RGTC2:
			common->Printf( "RGTC2  " );
			break;
	// --- depth/stencil ---
		case GL_DEPTH24_STENCIL8:
			common->Printf( "D24S8  " );
			break;
		case GL_DEPTH_COMPONENT32F:
			common->Printf( "D32F   " );
			break;
	// ---------------------
		case 0:
			common->Printf( "       " );
			break;
		default:
			common->Printf( "<BAD FORMAT:%i>", internalFormat );
			break;
	}

	switch ( filter ) {
		case TF_DEFAULT:
			common->Printf( "dflt " );
			break;
		case TF_LINEAR:
			common->Printf( "linr " );
			break;
		case TF_NEAREST:
			common->Printf( "nrst " );
			break;
		default:
			common->Printf( "<BAD FILTER:%i>", filter );
			break;
	}

	switch ( repeat ) {
		case TR_REPEAT:
			common->Printf( "rept " );
			break;
		case TR_CLAMP_TO_ZERO:
			common->Printf( "zero " );
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			common->Printf( "azro " );
			break;
		case TR_CLAMP:
			common->Printf( "clmp " );
			break;
		default:
			common->Printf( "<BAD REPEAT:%i>", repeat );
			break;
	}

	int storSize = StorageSize();
	for ( int i = 0; i < 3; i++ ) {
		if ( storSize < 1024 ) {
			common->Printf( "%4i%s ", storSize, i == 0 ? "B" : i == 1 ? "K" : "M" );
			break;
		}
		storSize /= 1024;
	}

	common->Printf( " %s\n", imgName.c_str() );
}

void idImageAsset::Print() const {
	if ( precompressedFile ) {
		common->Printf( "P" );
	} else if ( source.generatorFunction ) {
		common->Printf( "F" );
	} else {
		common->Printf( " " );
	}

	switch ( residency ) {
		case IR_GRAPHICS:
			common->Printf( " " );
			break;
		case IR_CPU:
			common->Printf( "C" );
			break;
		case IR_BOTH:
			common->Printf( "B" );
			break;
		default:
			common->Printf( "<BAD RES:%i>", residency );
			break;
	}

	// print basic stats and finish the line
	idImage::Print();
}

void idImageScratch::Print() const {
	common->Printf( "==" );

	// print basic stats and finish the line
	idImage::Print();
}

/*
================
BitsForInternalFormat

Returns number of bits per pixel in an image of this internal format.
If gpu = false (default), then size of data on CPU is computed.
If gpu = true, then amount of GPU memory is estimated --- never use the result for anything but debug prints!
Also note that any compressed image is internally padded to integer number of blocks.
================
*/
int idImage::BitsForInternalFormat( int internalFormat, bool gpu ) {
	switch ( internalFormat ) {
	// --- color uncompressed ---
		case GL_R8:
			return 8;
		case GL_RG8:
			return 16;
		case GL_RGB8:
			return (gpu ? 32 : 24);	// RGB is usually padded to RGBA internally
		case GL_RGBA8:
			return 32;
		case GL_RGBA16F:
			return 64;
		case GL_R32F:
			return 32;
		case GL_RGB5:
		case GL_RGB565:
		case GL_RGBA4:
			return 16;
		case GL_ALPHA8:
			return 8;
	// --- color compressed ---
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			return 4;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		case GL_COMPRESSED_RG_RGTC2:
			return 8;
	// --- depth/stencil ---
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH_COMPONENT32F:
			return 32;
	// ---------------------
		default:
			common->Warning( "\nR_BitsForInternalFormat: bad internalFormat (%i)", internalFormat );
	}
	return 0;
}
