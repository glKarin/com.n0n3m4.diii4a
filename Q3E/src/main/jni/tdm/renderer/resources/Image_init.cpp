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
#include "framework/LoadStack.h"
#include "renderer/backend/FrameBufferManager.h"

#define	DEFAULT_SIZE		16
#define	NORMAL_MAP_SIZE		32
#define FOG_SIZE			128
#define	RAMP_RANGE			8.0
#define	DEEP_RANGE			-30.0
#define QUADRATIC_WIDTH		32
#define QUADRATIC_HEIGHT	4
//#define BORDER_CLAMP_SIZE	32

#define LOAD_KEY_IMAGE_GRANULARITY 10 // grayman #3763

const char *imageFilter[] = {
	"GL_LINEAR_MIPMAP_NEAREST",
	"GL_LINEAR_MIPMAP_LINEAR",
	"GL_NEAREST",
	"GL_LINEAR",
	"GL_NEAREST_MIPMAP_NEAREST",
	"GL_NEAREST_MIPMAP_LINEAR",
	NULL
};

idCVar idImageManager::image_filter( "image_filter", imageFilter[1], CVAR_RENDERER | CVAR_ARCHIVE, "changes texture filtering on mipmapped images", imageFilter, idCmdSystem::ArgCompletion_String<imageFilter> );
idCVar idImageManager::image_anisotropy( "image_anisotropy", "1", CVAR_RENDERER | CVAR_ARCHIVE, "set the maximum texture anisotropy if available" );
idCVar idImageManager::image_lodbias( "image_lodbias", "0", CVAR_RENDERER | CVAR_ARCHIVE, "change lod bias on mipmapped images" );
idCVar idImageManager::image_downSize( "image_downSize", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls texture downsampling" );
idCVar idImageManager::image_forceDownSize( "image_forceDownSize", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "" );
idCVar idImageManager::image_colorMipLevels( "image_colorMipLevels", "0", CVAR_RENDERER | CVAR_BOOL, "development aid to see texture mip usage" );
idCVar idImageManager::image_preload( "image_preload", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "if 0, dynamically load all images" );
#ifdef __ANDROID__
idCVar idImageManager::image_useCompression( "image_useCompression", "0", CVAR_RENDERER | CVAR_INIT | CVAR_BOOL | CVAR_ROM, "1 = load compressed (DDS) images, 0 = force everything to high quality. 0 does not work for TDM as all our textures are DDS.(Disabled on Android, always using GL_RGBA non-compression)" );
idCVar idImageManager::image_useNormalCompression( "image_useNormalCompression", "0", CVAR_RENDERER | CVAR_INIT | CVAR_BOOL | CVAR_ROM, "use compression for normal maps if available, 0 = no, 1 = GL_COMPRESSED_RG_RGTC2(Disabled on Android, always using GL_RGBA non-compression)" );
idCVar idImageManager::image_usePrecompressedTextures( "image_usePrecompressedTextures", "0", CVAR_RENDERER | CVAR_INIT | CVAR_BOOL | CVAR_ROM, "Use .dds files if present.(Disabled on Android, always using GL_RGBA non-compression)" );
#else
idCVar idImageManager::image_useCompression( "image_useCompression", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "1 = load compressed (DDS) images, 0 = force everything to high quality. 0 does not work for TDM as all our textures are DDS." );
idCVar idImageManager::image_useNormalCompression( "image_useNormalCompression", "1", CVAR_RENDERER | CVAR_ARCHIVE, "use compression for normal maps if available, 0 = no, 1 = GL_COMPRESSED_RG_RGTC2" );
idCVar idImageManager::image_usePrecompressedTextures( "image_usePrecompressedTextures", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "Use .dds files if present." );
#endif
idCVar idImageManager::image_forceRecompress( "image_forceRecompress", "0", CVAR_RENDERER | CVAR_BOOL, "Uncompress contents of .dds files and apply automatic compression to them (debug only)" );
idCVar idImageManager::image_writePrecompressedTextures( "image_writePrecompressedTextures", "0", CVAR_RENDERER | CVAR_BOOL, "write .dds files if necessary" );
idCVar idImageManager::image_writeNormalTGA( "image_writeNormalTGA", "0", CVAR_RENDERER | CVAR_BOOL, "write .tgas of the final normal maps for debugging" );
idCVar idImageManager::image_writeTGA( "image_writeTGA", "0", CVAR_RENDERER | CVAR_BOOL, "write .tgas of the non normal maps for debugging" );
idCVar idImageManager::image_useOffLineCompression( "image_useOfflineCompression", "0", CVAR_RENDERER | CVAR_BOOL, "write a batch file for offline compression of DDS files" );
idCVar idImageManager::image_downSizeSpecular( "image_downSizeSpecular", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls specular downsampling" );
idCVar idImageManager::image_downSizeBump( "image_downSizeBump", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls normal map downsampling" );
idCVar idImageManager::image_downSizeSpecularLimit( "image_downSizeSpecularLimit", "64", CVAR_RENDERER | CVAR_ARCHIVE, "controls specular downsampled limit" );
idCVar idImageManager::image_downSizeBumpLimit( "image_downSizeBumpLimit", "128", CVAR_RENDERER | CVAR_ARCHIVE, "controls normal map downsample limit" );
idCVar idImageManager::image_ignoreHighQuality( "image_ignoreHighQuality", "0", CVAR_RENDERER | CVAR_ARCHIVE, "ignore high quality setting on materials" );
idCVar idImageManager::image_downSizeLimit( "image_downSizeLimit", "256", CVAR_RENDERER | CVAR_ARCHIVE, "controls diffuse map downsample limit" );

// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
idImageManager	imageManager;
idImageManager	*globalImages = &imageManager;

enum IMAGE_CLASSIFICATION {
	IC_NPC,
	IC_WEAPON,
	IC_MONSTER,
	IC_MODELGEOMETRY,
	IC_ITEMS,
	IC_MODELSOTHER,
	IC_GUIS,
	IC_WORLDGEOMETRY,
	IC_OTHER,
	IC_COUNT
};

struct imageClassificate_t {
	const char *rootPath;
	const char *desc;
	int type;
	int maxWidth;
	int maxHeight;
};

typedef idList< int > intList;

const imageClassificate_t IC_Info[] = {
	{ "models/characters", "Characters", IC_NPC, 512, 512 },
	{ "models/weapons", "Weapons", IC_WEAPON, 512, 512 },
	{ "models/monsters", "Monsters", IC_MONSTER, 512, 512 },
	{ "models/mapobjects", "Model Geometry", IC_MODELGEOMETRY, 512, 512 },
	{ "models/items", "Items", IC_ITEMS, 512, 512 },
	{ "models", "Other model textures", IC_MODELSOTHER, 512, 512 },
	{ "guis/assets", "Guis", IC_GUIS, 256, 256 },
	{ "textures", "World Geometry", IC_WORLDGEOMETRY, 256, 256 },
	{ "", "Other", IC_OTHER, 256, 256 }
};


static int ClassifyImage( const char *name ) {
	const idStr str = name;

	for ( int i = 0; i < IC_COUNT; i++ ) {
		if ( str.Find( IC_Info[i].rootPath, false ) == 0 ) {
			return IC_Info[i].type;
		}
	}
	return IC_OTHER;
}

/*
================
R_RampImage

Creates a 0-255 ramp image
================
*/
/*static void R_RampImage( idImage *image ) {
	byte	data[256][4];

	for ( int x = 0 ; x < 256 ; x++ ) {
		data[x][0] =
		data[x][1] =
		data[x][2] =
		data[x][3] = x;
	}
	image->GenerateImage( ( byte * )data, 256, 1,
	                      TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY );
}*/

/*
================
R_SpecularTableImage

Creates a ramp that matches our fudged specular calculation
================
*/
/*static void R_SpecularTableImage( idImage *image ) {
	byte	data[256][4];
	float	f;
	int		b;

	for ( int x = 0 ; x < 256 ; x++ ) {
		f = x / 255.0f;
#if 0
		f = pow( f, 16 );
#else
		// this is the behavior of the hacked up fragment programs that
		// can't really do a power function
		// tried with powf ? revelator
		f = ( f - 0.75 ) * 4.0f;
		if ( f < 0.0f ) {
			f = 0.0f;
		}
		f = f * f;
#endif
		b = ( byte )( f * 255.0f );

		data[x][0] =
		data[x][1] =
		data[x][2] =
		data[x][3] = b;
	}
	image->GenerateImage( ( byte * )data, 256, 1,
	                      TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}*/


/*
================
R_Specular2DTableImage

Create a 2D table that calculates ( reflection dot , specularity )
================
*/
/*static void R_Specular2DTableImage( idImage *image ) {
	byte	data[256][256][4];
	float	f;
	int		b;

	memset( data, 0, sizeof( data ) );
	for ( int x = 0 ; x < 256 ; x++ ) {
		f = x / 255.0f;
		for ( int y = 0; y < 256; y++ ) {
			b = ( byte )( pow( f, y ) * 255.0f );
			if ( b == 0 ) {
				// as soon as b equals zero all remaining values in this column are going to be zero
				// we early out to avoid pow() underflows
				break;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] =
			data[y][x][3] = b;
		}
	}
	image->GenerateImage( ( byte * )data, 256, 256, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
}*/

void imageBlock_s::Purge() {
	for (int s = 0; s < sides; s++)
		R_StaticFree(pic[s]);
	memset(this, 0, sizeof(imageBlock_s));
}

imageBlock_t imageBlock_s::Alloc2D(int w, int h) {
	imageBlock_t res;
	memset(&res, 0, sizeof(res));
	res.sides = 1;
	res.width = w;
	res.height = h;
	for (int s = 0; s < res.sides; s++)
		res.pic[s] = (byte*)R_StaticAlloc(res.GetSizeInBytes());
	return res;
}

imageBlock_t imageBlock_s::AllocCube(int size) {
	imageBlock_t res;
	memset(&res, 0, sizeof(res));
	res.sides = 6;
	res.width = size;
	res.height = size;
	for (int s = 0; s < res.sides; s++)
		res.pic[s] = (byte*)R_StaticAlloc(res.GetSizeInBytes());
	return res;
}

idVec4 imageBlock_s::Sample(float s, float t, textureFilter_t filter, textureRepeat_t repeat, int side) const {
	assert(dword(side) < dword(sides));

	auto GetPixelRepeat = [&](int is, int it) -> idVec4 {
		bool inBounds = (dword(is) < dword(width) && dword(it) < dword(height));
		if (!inBounds) {
			assert(is >= -width && is < 2 * width && it >= -height && it < 2 * height);
			if (repeat == TR_REPEAT) {
				if (is < 0) is += width;
				if (is >= width) is -= width;
				if (it < 0) it += height;
				if (it >= height) it -= height;
			}
			else if (repeat == TR_CLAMP) {
				is = idMath::ClampInt(0, width - 1, is);
				it = idMath::ClampInt(0, height - 1, it);
			}
			else if (repeat == TR_CLAMP_TO_ZERO)
				return idVec4(0.0f, 0.0f, 0.0f, 1.0f);
			else if (repeat == TR_CLAMP_TO_ZERO_ALPHA)
				return idVec4(0.0f, 0.0f, 0.0f, 0.0f);
			else assert(0);
		}
		return Fetch(is, it, side);
	};

	if (repeat == TR_REPEAT) {
		s = s - (int(s) - (s < 0.0f));	// fast equivalent to fmodf(s, 1.0f);
		t = t - (int(t) - (t < 0.0f));
	}
	// sanity clamp: at most half-size out of bounds
	s = idMath::ClampFloat(-0.5f, 1.5f - idMath::FLT_EPS * 4.0f, s);
	t = idMath::ClampFloat(-0.5f, 1.5f - idMath::FLT_EPS * 4.0f, t);
	assert(width >= 2 && height >= 2);	// tiny textures not supported, sorry
	// scale to texels
	s *= width;
	t *= height;

	if (filter == TF_NEAREST) {
		int is = int(s) - (s < 0.0f);	// fast floor
		int it = int(t) - (t < 0.0f);	//
		return GetPixelRepeat(is, it);
	}
	else if (filter == TF_LINEAR) {
		s += 0.5f;
		t += 0.5f;
		int is = int(s) - (s < 0.0f);	// fast floor
		int it = int(t) - (t < 0.0f);	//
		float fs = s - is;
		float ft = t - it;

		idVec4 res(0.0f);
		res += (1.0f - fs) * (1.0 - ft) * GetPixelRepeat(is - 1, it - 1);
		res += (       fs) * (1.0 - ft) * GetPixelRepeat(is    , it - 1);
		res += (1.0f - fs) * (      ft) * GetPixelRepeat(is - 1, it    );
		res += (       fs) * (      ft) * GetPixelRepeat(is    , it    );
		return res;
	}
	else {
		assert(0);
		return idVec4(0.0f);
	}
}

const idVec3 cubeMapAxes[6][3] = {
	{
		idVec3(0, 0, -1),
		idVec3(0, -1, 0),
		idVec3(1, 0, 0)
	},
	{
		idVec3(0, 0, 1),
		idVec3(0, -1, 0),
		idVec3(-1, 0, 0)
	},
	{
		idVec3(1, 0, 0),
		idVec3(0, 0, 1),
		idVec3(0, 1, 0)
	},
	{
		idVec3(1, 0, 0),
		idVec3(0, 0, -1),
		idVec3(0, -1, 0)
	},
	{
		idVec3(1, 0, 0),
		idVec3(0, -1, 0),
		idVec3(0, 0, 1)
	},
	{
		idVec3(-1, 0, 0),
		idVec3(0, -1, 0),
		idVec3(0, 0, -1)
	}
};

idVec4 imageBlock_s::SampleCube(const idVec3 &dir, textureFilter_t filter) const {
	idVec3 adir;
	adir[0] = idMath::Fabs(dir[0]);
	adir[1] = idMath::Fabs(dir[1]);
	adir[2] = idMath::Fabs(dir[2]);

	// select component with maximum absolute value
	float amax = adir.Max();
	int axis;
	if (adir[0] == amax)
		axis = 0;
	else if (adir[1] == amax)
		axis = 1;
	else {
		assert(adir[2] == amax);
		axis = 2;
	}
	// look if direction along it is positive or negative
	float localZ = dir[axis];
	// compute face index
	int face = axis * 2 + (localZ < 0.0f);
	assert(cubeMapAxes[face][2][axis] == (localZ < 0.0f ? -1.0f : 1.0f));
	assert((cubeMapAxes[face][2] * dir) == idMath::Fabs(localZ));

	// compute texcoords on the chosen face
	float invZ = 1.0f / idMath::Fabs(localZ);
	float fx = (dir * cubeMapAxes[face][0]) * invZ;
	float fy = (dir * cubeMapAxes[face][1]) * invZ;
	fx = 0.5f * (fx + 1.0f);
	fy = 0.5f * (fy + 1.0f);

	return Sample(fx, fy, filter, TR_CLAMP, face);
}

/*
===============
idImage::Sample
===============
*/
idVec4 idImageAsset::Sample(float s, float t) const {
	assert(residency & IR_CPU);
	assert(!cpuData.IsCubemap());
	return cpuData.Sample(s, t, filter, repeat, 0);
}

idImage::idImage() {
	texnum = static_cast< GLuint >( TEXTURE_NOT_LOADED );
	type = TT_DISABLED;
	frameUsed = 0;
	imgName[0] = '\0';
	filter = TF_DEFAULT;
	repeat = TR_REPEAT;
	depth = TD_DEFAULT;
	bindCount = 0;
	uploadWidth = uploadHeight = 0;
	internalFormat = 0;
	hashNext = NULL;
	refCount = 0;
	swizzleMask = NULL;
}

idImageAsset::idImageAsset() {
	classification = 0;
	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	precompressedFile = false;
	defaulted = false;
	timestamp = 0;
	allowDownSize = false;
	memset( &cpuData, 0, sizeof( cpuData ) );
	compressedData = nullptr;
	residency = IR_GRAPHICS;
	loadStack = nullptr;
}


/*
==================
R_CreateDefaultImage

the default image will be grey with a white box outline
to allow you to see the mapping coordinates on a surface
==================
*/
static imageBlock_t R_DefaultImage() {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	static const int CELL_SIZE = 2;

	if ( com_developer.GetBool() ) {
		// grey center
		for ( int y = 1 ; y < DEFAULT_SIZE - 1 ; y++ ) {
			for ( int x = 1 ; x < DEFAULT_SIZE - 1 ; x++ ) {
				int cell = (((x - 1) / CELL_SIZE) + ((y - 1) / CELL_SIZE)) % 2;
				int value = (cell ? 32 : 160);	// checkerboard!
				data[y][x][0] = value;
				data[y][x][1] = value;
				data[y][x][2] = value;
				data[y][x][3] = 255;
			}
		}
		// white border
		for ( int x = 0 ; x < DEFAULT_SIZE ; x++ ) {
			for ( int c = 0; c < 4; c++ ) {
				data[0][x][c] = 255;
				data[x][0][c] = 255;
				data[DEFAULT_SIZE - 1][x][c] = 255;
				data[x][DEFAULT_SIZE - 1][c] = 255;
			}
		}
	} else {
		for ( int y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			for ( int x = 0 ; x < DEFAULT_SIZE ; x++ ) {
				data[y][x][0] = 0;
				data[y][x][1] = 0;
				data[y][x][2] = 0;
				data[y][x][3] = 0;
			}
		}
	}

	imageBlock_t res = imageBlock_t::Alloc2D( DEFAULT_SIZE, DEFAULT_SIZE );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}

void idImageAsset::MakeDefault() {
	PurgeImage();
	defaulted = true;
	cpuData = R_DefaultImage();
	GenerateImage( cpuData.pic[0], cpuData.width, cpuData.height, filter, allowDownSize, repeat, depth, residency );
}

static imageBlock_t R_WhiteImage() {
	imageBlock_t res = imageBlock_t::Alloc2D( DEFAULT_SIZE, DEFAULT_SIZE );
	// solid white texture
	memset( res.pic[0], 255, res.GetSizeInBytes() );
	return res;
}

static imageBlock_t R_BlackImage() {
	imageBlock_t res = imageBlock_t::Alloc2D( DEFAULT_SIZE, DEFAULT_SIZE );
	// solid white texture
	memset( res.pic[0], 0, res.GetSizeInBytes() );
	return res;
}

static imageBlock_t R_AlphaNotchImage() {
	byte data[2][4];
	// this is used for alpha test clip planes
	data[0][0] = data[0][1] = data[0][2] = 255;
	data[0][3] = 0;
	data[1][0] = data[1][1] = data[1][2] = 255;
	data[1][3] = 255;

	imageBlock_t res = imageBlock_t::Alloc2D( 2, 1 );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}

static imageBlock_t R_FlatNormalImage() {
	imageBlock_t res = imageBlock_t::Alloc2D( DEFAULT_SIZE, DEFAULT_SIZE );
	byte *data = res.GetPic();
	// flat normal map for default bump mapping
	for ( int i = 0; i < DEFAULT_SIZE * DEFAULT_SIZE; i++) {
		data[4 * i + 0] = 128;
		data[4 * i + 1] = 128;
		data[4 * i + 2] = 255;
		data[4 * i + 3] = 255;
	}
	return res;
}

static imageBlock_t R_AmbientNormalImage() {
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	// flat normal map for default bunp mapping
	for ( int i = 0 ; i < 4 ; i++ ) {
		data[0][i][0] = ( byte )( 255 * tr.ambientLightVector[0] );
		data[0][i][1] = ( byte )( 255 * tr.ambientLightVector[1] );
		data[0][i][2] = ( byte )( 255 * tr.ambientLightVector[2] );
		data[0][i][3] = 255;
	}

	// this must be a cube map for fragment programs to simply substitute for the normalization cube map
	imageBlock_t res = imageBlock_t::AllocCube( DEFAULT_SIZE );
	for ( int i = 0 ; i < 6 ; i++ ) {
		memcpy( res.pic[i], data, sizeof(data) );
	}
	return res;
}

/*
===============
CreatePitFogImage
===============
*/
static void CreatePitFogImage( void ) {
	byte	data[16][16][4];
	int		a;

	memset( data, 0, sizeof( data ) );

	for ( int i = 0 ; i < 16 ; i++ ) {
		a = i * ( 255 / 15 );
		if ( a > 255 ) {
			a = 255;
		}

		for ( int j = 0 ; j < 16 ; j++ ) {
			data[j][i][0] =
			data[j][i][1] =
			data[j][i][2] = 255;
			data[j][i][3] = a;
		}
	}
	R_WriteTGA( "shapes/pitFalloff.tga", data[0][0], 16, 16 );
}

static imageBlock_t R_MakeConstCubeMap( const byte value[4] ) {
	static const int size = 16;
	imageBlock_t res = imageBlock_t::AllocCube( size );

	for ( int i = 0; i < 6; i++ ) {
		for ( int p = 0; p < size * size; p++ ) {
			res.pic[i][4 * p + 0] = value[0];
			res.pic[i][4 * p + 1] = value[1];
			res.pic[i][4 * p + 2] = value[2];
			res.pic[i][4 * p + 3] = value[3];
		}
	}
	return res;
}
static imageBlock_t R_MakeWhiteCubeMap() {
	static const byte WHITE[4] = {255, 255, 255, 255};
	return R_MakeConstCubeMap( WHITE );
}
static imageBlock_t R_MakeBlackCubeMap() {
	static const byte BLACK[4] = {0, 0, 0, 0};
	return R_MakeConstCubeMap( BLACK );
}

static imageBlock_t R_CosPowerCubeMap( idVec3 axis, int power, float scale, idVec3 add ) {
	static const int size = 32;
	imageBlock_t res = imageBlock_t::AllocCube( size );

	for ( int f = 0; f < 6; f++ ) {
		for ( int y = 0; y < size; y++ ) {
			for ( int x = 0; x < size; x++ ) {
				float ratioX = ( x + 0.5f ) / size;
				float ratioY = ( y + 0.5f ) / size;
				// direction to center of texel
				idVec3 dir = (
					cubeMapAxes[f][2] + 
					cubeMapAxes[f][0] * ( 2.0f * ratioX - 1.0f ) +
					cubeMapAxes[f][1] * ( 2.0f * ratioY - 1.0f )
				);
				dir.Normalize();

				float cosA = idMath::Fmax(axis * dir, 0.0f);
				float cosPwr = cosA;
				for (int q = 1; q < power; q++)
					cosPwr *= cosA;

				idVec3 light = idVec3(cosPwr) * scale + add;

				int p = (y * size + x);
				res.pic[f][4 * p + 0] = int(idMath::ClampFloat(0.0f, 1.0f, light[0]) * 255.0f + 0.5f);
				res.pic[f][4 * p + 1] = int(idMath::ClampFloat(0.0f, 1.0f, light[1]) * 255.0f + 0.5f);
				res.pic[f][4 * p + 2] = int(idMath::ClampFloat(0.0f, 1.0f, light[2]) * 255.0f + 0.5f);
				res.pic[f][4 * p + 3] = 255;
			}
		}
	}

	return res;
}

static imageBlock_t R_AmbientWorldDiffuseCubeMap() {
	// mix diffuse and ambient in 1:1 proportion
	return R_CosPowerCubeMap( idVec3(0, 0, 1), 1, 0.5f, idVec3(0.5f) );
}
static imageBlock_t R_AmbientWorldSpecularCubeMap() {
	// take 50% of pure specular
	return R_CosPowerCubeMap( idVec3(0, 0, 1), 4, 0.5f, idVec3(0.0f) );
}


/*
================
R_CreateNoFalloffImage

This is a solid white texture that is zero clamped.
================
*/
static imageBlock_t R_CreateNoFalloffImage() {
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];

	memset( data, 0, sizeof( data ) );
	for ( int x = 1 ; x < FALLOFF_TEXTURE_SIZE - 1 ; x++ ) {
		for ( int y = 1 ; y < 15 ; y++ ) {
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}

	imageBlock_t res = imageBlock_t::Alloc2D( FALLOFF_TEXTURE_SIZE, 16 );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}


/*
================
R_FogImage

We calculate distance correctly in two planes, but the
third will still be projection based
================
*/
static imageBlock_t R_FogImage() {
	byte	data[FOG_SIZE][FOG_SIZE][4];
	int		b;
	float	d;
	float	step[256];
	float	remaining = 1.0f;

	for ( int i = 0 ; i < 256 ; i++ ) {
		step[i] = remaining;
		remaining *= 0.982f;
	}

	for ( int x = 0 ; x < FOG_SIZE ; x++ ) {
		for ( int y = 0 ; y < FOG_SIZE ; y++ ) {
			d = idMath::Sqrt( ( x - FOG_SIZE / 2 ) * ( x - FOG_SIZE / 2 ) + ( y - FOG_SIZE / 2 ) * ( y - FOG_SIZE / 2 ) ) / ( ( FOG_SIZE / 2 ) - 1.0f );
			b = ( byte )( d * 255 );

			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			b = ( byte )( 255 * ( 1.0f - step[b] ) );

			if ( x == 0 || x == FOG_SIZE - 1 || y == 0 || y == FOG_SIZE - 1 ) {
				b = 255;		// avoid clamping issues
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	imageBlock_t res = imageBlock_t::Alloc2D( FOG_SIZE, FOG_SIZE );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}


/*
================
FogFraction

Height values below zero are inside the fog volume
================
*/
static float FogFraction( float viewHeight, float targetHeight ) {
	float	total = idMath::Fabs( targetHeight - viewHeight );

	// only ranges that cross the ramp range are special
	if ( targetHeight > 0 && viewHeight > 0 ) {
		return 0.0f;
	} else if ( targetHeight < -RAMP_RANGE && viewHeight < -RAMP_RANGE ) {
		return 1.0f;
	}
	float rampSlope = 1.0f / RAMP_RANGE;

	if ( !total ) {
		return -viewHeight * rampSlope;
	}
	float above;

	if ( targetHeight > 0.0f ) {
		above = targetHeight;
	} else if ( viewHeight > 0.0f ) {
		above = viewHeight;
	} else {
		above = 0.0f;
	}
	float rampTop, rampBottom;

	if ( viewHeight > targetHeight ) {
		rampTop = viewHeight;
		rampBottom = targetHeight;
	} else {
		rampTop = targetHeight;
		rampBottom = viewHeight;
	}

	if ( rampTop > 0.0f ) {
		rampTop = 0.0f;
	}

	if ( rampBottom < -RAMP_RANGE ) {
		rampBottom = -RAMP_RANGE;
	}
	float ramp = ( 1.0f - ( rampTop * rampSlope + rampBottom * rampSlope ) * -0.5f ) * ( rampTop - rampBottom );
	float frac = ( total - above - ramp ) / total;

	// after it gets moderately deep, always use full value
	float deepest = viewHeight < targetHeight ? viewHeight : targetHeight;
	float deepFrac = deepest / DEEP_RANGE;

	if ( deepFrac >= 1.0f ) {
		return 1.0f;
	}
	frac = frac * ( 1.0f - deepFrac ) + deepFrac;

	return frac;
}

/*
================
R_FogEnterImage

Modulate the fog alpha density based on the distance of the
start and end points to the terminator plane
================
*/
static imageBlock_t R_FogEnterImage() {
	byte	data[FOG_ENTER_SIZE][FOG_ENTER_SIZE][4];
	int		b;
	float	d;

	for ( int x = 0 ; x < FOG_ENTER_SIZE ; x++ ) {
		for ( int y = 0 ; y < FOG_ENTER_SIZE ; y++ ) {
			d = FogFraction( x - ( FOG_ENTER_SIZE / 2 ), y - ( FOG_ENTER_SIZE / 2 ) );
			b = ( byte )( d * 255.0f );

			if ( b < 1 ) {			// Rounding issues
				b = 0;
			} else if ( b > 254 ) { // Rounding issues
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	// if mipmapped, acutely viewed surfaces fade wrong
	imageBlock_t res = imageBlock_t::Alloc2D( FOG_ENTER_SIZE, FOG_ENTER_SIZE );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}


/*
================
R_QuadraticImage

================
*/
static imageBlock_t R_QuadraticImage() {
	byte	data[QUADRATIC_HEIGHT][QUADRATIC_WIDTH][4];
	int		b;
	float	d;

	for ( int x = 0 ; x < QUADRATIC_WIDTH ; x++ ) {
		for ( int y = 0 ; y < QUADRATIC_HEIGHT ; y++ ) {

			d = x - ( QUADRATIC_WIDTH / 2 - 0.5f );
			d = idMath::Fabs( d );
			d -= 0.5f;
			d /= QUADRATIC_WIDTH / 2;

			d = 1.0f - d;
			d = d * d;

			b = ( byte )( d * 255 );
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}

	imageBlock_t res = imageBlock_t::Alloc2D( QUADRATIC_WIDTH, QUADRATIC_HEIGHT );
	memcpy( res.pic[0], data, sizeof(data) );
	return res;
}

//=====================================================================

typedef struct {
	const char *name;
	int	minimize, maximize;
} filterName_t;

static filterName_t textureFilters[] = {
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR},
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST}
};

/*
===============
ChangeTextureFilter

This resets filtering on all loaded images
New images will automatically pick up the current values.
===============
*/
void idImageManager::ChangeTextureFilter( void ) {
	int		i;
	idImage	*glt;

	// if these are changed dynamically, it will force another ChangeTextureFilter
	image_filter.ClearModified();
	image_anisotropy.ClearModified();
	image_lodbias.ClearModified();

	const char *string = image_filter.GetString();
	for ( i = 0; i < 6; i++ ) {
		if ( !idStr::Icmp( textureFilters[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		common->Warning( "bad r_textureFilter: '%s'", string );
		// default to LINEAR_MIPMAP_NEAREST
		i = 0;
	}

	// set the values for future images
	textureMinFilter = textureFilters[i].minimize;
	textureMaxFilter = textureFilters[i].maximize;
	textureAnisotropy = image_anisotropy.GetFloat();
	if ( textureAnisotropy < 1 ) {
		textureAnisotropy = 1;
	} else if ( textureAnisotropy > glConfig.maxTextureAnisotropy ) {
		textureAnisotropy = glConfig.maxTextureAnisotropy;
	}
	textureLODBias = image_lodbias.GetFloat();

	// change all the existing mipmap texture objects with default filtering
	unsigned int texEnum;
	for ( i = 0 ; i < images.Num() ; i++ ) {
		glt = images[ i ];

		switch ( glt->type ) {
			case TT_2D:
				texEnum = GL_TEXTURE_2D;
				break;
			case TT_CUBIC:
				texEnum = GL_TEXTURE_CUBE_MAP;
				break;
			default:
				texEnum = GL_TEXTURE_2D;
		}

		// make sure we don't start a background load
		if ( glt->texnum == idImage::TEXTURE_NOT_LOADED ) {
			continue;
		}

		glt->Bind();
		if ( glt->filter == TF_DEFAULT ) {
			qglTexParameterf( texEnum, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
			qglTexParameterf( texEnum, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
		}
		if ( glConfig.anisotropicAvailable ) {
			qglTexParameterf( texEnum, GL_TEXTURE_MAX_ANISOTROPY_EXT, globalImages->textureAnisotropy );
		}
		qglTexParameterf( texEnum, GL_TEXTURE_LOD_BIAS, globalImages->textureLODBias );
	}

	// if any framebuffers are using render textures, they will need to be recreated after this
	frameBuffers->PurgeAll();
}

/*
===============
idImage::Reload
===============
*/
void idImageAsset::Reload( bool checkPrecompressed, bool force ) {
	if ( !force ) {
		if ( !source.filename.IsEmpty() ) {
			// check file times
			ID_TIME_T	current;

			if ( source.cubeFiles != CF_2D ) {
				R_LoadImageProgramCubeMap( imgName, source.cubeFiles, nullptr, nullptr, &current );
			} else { // get the current values
				R_LoadImageProgram( imgName, nullptr, nullptr, nullptr, &current );
			}

			if ( current <= timestamp ) {
				return;
			}
		} else if ( source.generatorFunction ) { 
			// always regenerate functional images
		} else {
			assert( false );
		}
	}
	common->DPrintf( "reloading %s.\n", imgName.c_str() );

	PurgeImage();

	// force no precompressed image check, which will cause it to be reloaded
	// from source, and another precompressed file generated.
	// Load is from the front end, so the back end must be synced
	ActuallyLoadImage();
}

/*
===============
R_ReloadImages_f

Regenerate all images that came directly from files that have changed, so
any saved changes will show up in place.

New r_texturesize/r_texturedepth variables will take effect on reload

reloadImages <all>
===============
*/
void R_ReloadImages_f( const idCmdArgs &args ) {
	// FIXME - this probably isn't necessary... // Serp - this is a comment from the gpl release, check if it's really not needed
	globalImages->ChangeTextureFilter();

	bool		normalsOnly = false, force = false;
	bool		checkPrecompressed = false;		// if we are doing this as a vid_restart, look for precompressed like normal
	static int	msaaCheck = 0;

	if ( r_multiSamples.GetInteger() > 0 ) {
		msaaCheck = r_multiSamples.GetInteger();
		r_multiSamples.SetInteger( 0 );
	}

	if ( args.Argc() == 2 ) {
		if ( !idStr::Icmp( args.Argv( 1 ), "all" ) ) {
			force = true;
		} else if ( !idStr::Icmp( args.Argv( 1 ), "bump" ) ) {
			force = true;
			normalsOnly = true;
		} else if ( !idStr::Icmp( args.Argv( 1 ), "reload" ) ) {
			force = true;
			checkPrecompressed = true;
		} else {
			common->Printf( "USAGE: reloadImages <all>\n" );
			return;
		}
	}

	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		if ( idImageAsset *image = globalImages->images[i]->AsAsset() ) {
			if ( image->depth != TD_BUMP && normalsOnly ) {
				continue;
			}
			image->Reload( checkPrecompressed, force );
		}
	}

	if ( game ) {
		game->OnReloadImages();
	}

	if ( msaaCheck > 0 ) {
		r_multiSamples.SetInteger( msaaCheck );
	}
	msaaCheck = 0;

	// FBOs may need to be recreated since their render textures may have been recreated
	frameBuffers->PurgeAll();
}

typedef struct {
	idImage	*image;
	int		size;
} sortedImage_t;

/*
=======================
R_QsortImageSizes
=======================
*/
static int R_QsortImageSizes( const void *a, const void *b ) {

	const sortedImage_t	*ea = ( sortedImage_t * )a;
	const sortedImage_t	*eb = ( sortedImage_t * )b;

	if ( ea->size > eb->size ) {
		return -1;
	} else if ( ea->size < eb->size ) {
		return 1;
	} else {
		return idStr::Icmp( ea->image->imgName, eb->image->imgName );
	}
}

/*
===============
R_ListImages_f
===============
*/
void R_ListImages_f( const idCmdArgs &args ) {
	int		i, j, partialSize;
	idImage	*image;
	int		totalSize;
	int		count = 0;
	int		matchTag = 0;
	bool	uncompressedOnly = false;
	bool	unloaded = false;
	bool	failed = false;
	bool	touched = false;
	bool	sorted = false;
	bool	duplicated = false;
	bool	byClassification = false;
	bool	overSized = false;

	for ( int i = 1; i < args.Argc(); i++ ) {
		if ( idStr::Icmp( args.Argv( i ), "uncompressed" ) == 0 ) {
			uncompressedOnly = true;
		} else if ( idStr::Icmp( args.Argv( i ), "sorted" ) == 0 ) {
			sorted = true;
		} else if ( idStr::Icmp( args.Argv( i ), "unloaded" ) == 0 ) {
			unloaded = true;
		} else if ( idStr::Icmp( args.Argv( i ), "tagged" ) == 0 ) {
			matchTag = 1;
		} else if ( idStr::Icmp( args.Argv( i ), "duplicated" ) == 0 ) {
			duplicated = true;
		} else if ( idStr::Icmp( args.Argv( i ), "touched" ) == 0 ) {
			touched = true;
		} else if ( idStr::Icmp( args.Argv( i ), "classify" ) == 0 ) {
			byClassification = true;
			sorted = true;
		} else if ( idStr::Icmp( args.Argv( i ), "oversized" ) == 0 ) {
			byClassification = true;
			sorted = true;
			overSized = true;
		} else {
			failed = true;
		}
	}

	if ( failed ) {
		common->Printf( "usage: listImages [ sorted | partial | unloaded | cached | uncached | tagged | duplicated | touched | classify | showOverSized ]\n" );
		return;
	}
	const char *header = "        -w-- -h-- -fmt-- filt wrap  size  --name-------\n";

	common->Printf( "\n%s", header );

	totalSize = 0;

	sortedImage_t	*sortedArray = ( sortedImage_t * )alloca( sizeof( sortedImage_t ) * globalImages->images.Num() );

	for ( i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[ i ];

		if ( uncompressedOnly && IsImageFormatCompressed( image->internalFormat ) ) {
			continue;
		}

		if ( matchTag && image->AsAsset() && image->AsAsset()->classification != matchTag ) {
			continue;
		}

		if ( unloaded && image->texnum != idImage::TEXTURE_NOT_LOADED ) {
			continue;
		}

		// only print duplicates (from mismatched wrap / clamp, etc)
		if ( duplicated ) {
			for ( j = i + 1 ; j < globalImages->images.Num() ; j++ ) {
				if ( idStr::Icmp( image->imgName, globalImages->images[ j ]->imgName ) == 0 ) {
					break;
				}
			}
			if ( j == globalImages->images.Num() ) {
				continue;
			}
		}

		// "listimages touched" will list only images bound since the last "listimages touched" call
		if ( touched ) {
			if ( image->bindCount == 0 ) {
				continue;
			}
			image->bindCount = 0;
		}

		if ( sorted ) {
			sortedArray[count].image = image;
			sortedArray[count].size = image->StorageSize();
		} else {
			common->Printf( "%4i:",	i );
			image->Print();
		}
		totalSize += image->StorageSize();
		count++;
	}

	if ( sorted ) {
		qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageSizes );
		partialSize = 0;
		for ( i = 0 ; i < count ; i++ ) {
			common->Printf( "%4i:",	i );
			sortedArray[i].image->Print();
			partialSize += sortedArray[i].image->StorageSize();
			if ( ( ( i + 1 ) % 10 ) == 0 ) {
				common->Printf( "-------- %5.1f of %5.1f megs --------\n",
				                partialSize / ( 1024 * 1024.0 ), totalSize / ( 1024 * 1024.0 ) );
			}
		}
	}
	common->Printf( "%s", header );
	common->Printf( " %i images (%i total)\n", count, globalImages->images.Num() );
	common->Printf( " %5.1f total megabytes of images\n\n\n", totalSize / ( 1024 * 1024.0 ) );

	if ( byClassification ) {
		idList< int > classifications[IC_COUNT];

		for ( i = 0 ; i < count ; i++ ) {
			int cl = ClassifyImage( sortedArray[i].image->imgName );
			classifications[ cl ].Append( i );
		}

		for ( i = 0; i < IC_COUNT; i++ ) {
			partialSize = 0;
			idList< int > overSizedList;
			for ( j = 0; j < classifications[ i ].Num(); j++ ) {
				partialSize += sortedArray[ classifications[ i ][ j ] ].image->StorageSize();
				if ( overSized ) {
					if ( sortedArray[ classifications[ i ][ j ] ].image->uploadWidth > IC_Info[i].maxWidth && sortedArray[ classifications[ i ][ j ] ].image->uploadHeight > IC_Info[i].maxHeight ) {
						overSizedList.Append( classifications[ i ][ j ] );
					}
				}
			}
			common->Printf( " Classification %s contains %i images using %5.1f megabytes\n", IC_Info[i].desc, classifications[i].Num(), partialSize / ( 1024 * 1024.0 ) );

			if ( overSized && overSizedList.Num() ) {
				common->Printf( "  The following images may be oversized\n" );
				for ( j = 0; j < overSizedList.Num(); j++ ) {
					common->Printf( "    " );
					sortedArray[ overSizedList[ j ] ].image->Print();
					common->Printf( "\n" );
				}
			}
		}
	}

}

/*
==================
SetNormalPalette

Create a 256 color palette to be used by compressed normal maps
==================
*/
void idImageManager::SetNormalPalette( void ) {
	idVec3	v;
	byte	*temptable = compressedPalette;
	int		j, compressedToOriginal[16];
	float	t, f, y;

	// make an ad-hoc separable compression mapping scheme
	for ( int i = 0 ; i < 8 ; i++ ) {
		f = ( i + 1 ) / 8.5f;
		y =  1.0f - idMath::Sqrt( 1.0f - f * f );

		compressedToOriginal[7 - i] = 127 - ( int )( y * 127 + 0.5f );
		compressedToOriginal[8 + i] = 128 + ( int )( y * 127 + 0.5f );
	}

	for ( int i = 0 ; i < 256 ; i++ ) {
		if ( i <= compressedToOriginal[0] ) {
			originalToCompressed[i] = 0;
		} else if ( i >= compressedToOriginal[15] ) {
			originalToCompressed[i] = 15;
		} else {
			for ( j = 0 ; j < 14 ; j++ ) {
				if ( i <= compressedToOriginal[j + 1] ) {
					break;
				}
			}
			if ( i - compressedToOriginal[j] < compressedToOriginal[j + 1] - i ) {
				originalToCompressed[i] = j;
			} else {
				originalToCompressed[i] = j + 1;
			}
		}
	}

	for ( int i = 0; i < 16; i++ ) {
		for ( j = 0 ; j < 16 ; j++ ) {

			v[0] = ( compressedToOriginal[i] - 127.5 ) / 128;
			v[1] = ( compressedToOriginal[j] - 127.5 ) / 128;

			t = 1.0 - ( v[0] * v[0] + v[1] * v[1] );

			if ( t < 0 ) {
				t = 0;
			}
			v[2] = idMath::Sqrt( t );

			temptable[( i * 16 + j ) * 3 + 0] = ( byte )( 128 + floor( 127 * v[0] + 0.5 ) );
			temptable[( i * 16 + j ) * 3 + 1] = ( byte )( 128 + floor( 127 * v[1] ) );
			temptable[( i * 16 + j ) * 3 + 2] = ( byte )( 128 + floor( 127 * v[2] ) );
		}
	}

	// color 255 will be the "nullnormal" color for no reflection
	temptable[255 * 3 + 0] =
	temptable[255 * 3 + 1] =
	temptable[255 * 3 + 2] = 128;
}

/*
==============
AllocImage

Allocates an idImage, adds it to the list,
copies the name, and adds it to the hash chain.
==============
*/
idImageAsset *idImageManager::AllocImageAsset( const char *name ) {
	if ( strlen( name ) >= MAX_IMAGE_NAME || strlen( name ) < MIN_IMAGE_NAME ) {
		const char *warnp  = ( strlen( name ) >= MAX_IMAGE_NAME ) ? "long" : "short";
		common->Warning( "Image name \"%s\" is too %s", name, warnp );
	}
	idImageAsset *image = new idImageAsset;

	images.Append( image );
	image->loadStack = new LoadStack(declManager->GetLoadStack());

	const int hash = idStr( name ).FileNameHash();
	image->hashNext = imageHashTable[hash];
	imageHashTable[hash] = image;

	image->imgName = name;

	return image;
}
// TODO: remove copy/paste!
idImageScratch *idImageManager::AllocImageScratch( const char *name ) {
	if ( strlen( name ) >= MAX_IMAGE_NAME || strlen( name ) < MIN_IMAGE_NAME ) {
		const char *warnp  = ( strlen( name ) >= MAX_IMAGE_NAME ) ? "long" : "short";
		common->Warning( "Image name \"%s\" is too %s", name, warnp );
	}
	idImageScratch *image = new idImageScratch;

	images.Append( image );

	const int hash = idStr( name ).FileNameHash();
	image->hashNext = imageHashTable[hash];
	imageHashTable[hash] = image;

	image->imgName = name;

	return image;
}

/*
==================
ImageScratch

Generate scratch image to be used as e.g. FBO attachment.
==================
*/
idImageScratch *idImageManager::ImageScratch( const char *_name ) {
	if ( !_name ) {
		common->FatalError( "idImageManager::ImageScratch: NULL name" );
	}

	idStr name = _name;
	// see if the image already exists
	int	hash = name.FileNameHash();
	for ( idImage *baseimg = imageHashTable[hash] ; baseimg; baseimg = baseimg->hashNext ) {
		if ( name.Icmp( baseimg->imgName ) == 0 ) {
			idImageScratch *image = baseimg->AsScratch();
			if ( !image ) {
				common->Error( "Image name '%s' used both for scratch and asset", name.c_str() );
			}
			return image;
		}
	}

	// create the image
	idImageScratch *image = AllocImageScratch( name );
	image->type = TT_2D;

	return image;
}

/*
===============
ImageFromSource

Finds or loads the given image, always returning a valid image pointer.
Loading of the image may be deferred for dynamic loading.
==============
*/
idImageAsset * idImageManager::ImageFromSource(
	const idStr &name, ImageAssetSource source,
	textureFilter_t filter, bool allowDownSize,
	textureRepeat_t repeat, textureDepth_t depth,
	imageResidency_t residency
) {
	if ( name.IsEmpty() ) {
		common->Warning( "Image with empty name requested" );
		declManager->GetLoadStack().PrintStack(2);
		return defaultImage;
	}

	int numSources = ( source.generatorFunction != nullptr ) + ( !source.filename.IsEmpty() );
	if ( numSources != 1 ) {
		common->Error( "Image '%s' has %d sources specified", name.c_str(), numSources );
	}

	if ( source.generatorFunction && source.cubeFiles != CF_NONE ) {
		// it is either CF_2D or CF_NATIVE, depending on whether generator returns 1 or 6 sides
		source.cubeFiles = CF_NONE;
	}
	if ( !source.filename.IsEmpty() && source.cubeFiles == CF_NONE ) {
		// perhaps some error? let's assume 2D texture by default
		source.cubeFiles = CF_2D;
	}

	// this can be changed if we query generated image by name
	ImageAssetSource overrideSource = source;

	// see if the image is already loaded, unless we are in a reloadImages call
	int hash = name.FileNameHash();

	for ( idImage *baseimg = imageHashTable[hash]; baseimg; baseimg = baseimg->hashNext ) {
		if ( name.Icmp( baseimg->imgName ) == 0 ) {
			idImageAsset *image = baseimg->AsAsset();
			if ( !image ) {
				common->Error( "Image name '%s' used both for scratch and asset", name.c_str() );
			}

			if ( !source.filename.IsEmpty() && image->source.filename.IsEmpty() ) {
				// special case: it is allowed to call ImageFromFile get find existing builtin image by name
				// in this case copy generator function from ANY found image
				overrideSource = image->source;
			}
			else {
				if ( image->source.filename.Icmp( source.filename ) != 0 ) {
					// this never happens actually, because imgName == source.filename =)
					common->Error( "Image '%s' has different source files: '%s' and '%s'", name.c_str(), image->source.filename.c_str(), source.filename.c_str() );
				}
				if ( image->source.generatorFunction != source.generatorFunction ) {
					common->Error( "Image '%s' has different generator function pointer: %p and %p", name.c_str(), image->source.generatorFunction, source.generatorFunction );
				}
				if ( image->source.cubeFiles != source.cubeFiles ) {
					common->Error( "Image '%s' has been referenced with conflicting cube map states", name.c_str() );
				}
			}

			if ( image->filter != filter || image->repeat != repeat ) {
				// don't reuse this image, better create a new one instead
				// note: we might want to have the system reset these parameters on every bind and share the image data?
				continue;
			}

			// the same image is being requested, but with a different allowDownSize or depth
			// so pick the highest of the two and reload the old image with those parameters
			if ( allowDownSize && !image->allowDownSize ) {
				allowDownSize = false;
			}
			if ( depth < image->depth ) {
				depth = image->depth;
			}
			if ( image->residency & (~residency) ) {
				residency = imageResidency_t( residency | image->residency );
			}

			if ( image->allowDownSize == allowDownSize && image->depth == depth && image->residency == residency ) {
				// the already created one is already the highest quality
				image->levelLoadReferenced = true;
				return image;
			}

			image->allowDownSize = allowDownSize;
			image->depth = depth;
			image->residency = residency;
			image->levelLoadReferenced = true;

			if ( image_preload.GetBool() && !insideLevelLoad ) {
				image->referencedOutsideLevelLoad = true;
				image->ActuallyLoadImage();	// check for precompressed, load is from front end
				declManager->MediaPrint( "%ix%i %s (reload for mixed references)\n", image->uploadWidth, image->uploadHeight, image->imgName.c_str() );
			}
			return image;
		}
	}

	//
	// create a new image
	//
	idImageAsset *image = AllocImageAsset( name );

	image->source = overrideSource;

	// HACK: to allow keep fonts from being mip'd, as new ones will be introduced with localization
	// this keeps us from having to make a material for each font tga
	if ( name.Find( "fontImage_" ) >= 0
		//nbohr1more: 4358 blacklist texture paths to prevent image_downsize from making fonts, guis, and background images blurry
		|| name.Find( "fonts/" ) >= 0
		|| name.Find( "guis/" ) >= 0
		|| name.Find( "postprocess/" ) >= 0
		|| name.Find( "_cookedMath" ) >= 0
		|| name.Find( "_currentRender" ) >= 0
		|| name.Find( "_currentDepth" ) >= 0
		|| name.Find( "/consolefont" ) >= 0
		|| name.Find( "/bigchars" ) >= 0
		|| name.Find( "/entityGui" ) >= 0
		|| name.Find( "video/" ) >= 0
		|| name.Find( "fsfx" ) >= 0
		|| name.Find( "/AFX" ) >= 0
		|| name.Find( "_afxweight" ) >= 0 ) {
		allowDownSize = false;
	}
	image->allowDownSize = allowDownSize;
	image->repeat = repeat;
	image->depth = depth;
	image->type = TT_2D;
	image->filter = filter;
	image->residency = residency;

	image->levelLoadReferenced = true;

	// load it if we aren't in a level preload
	// FIXME assume CPU residency as a flag that we need the image data immediately, rather than maybe load in background for GPU uploads
	if ( image_preload.GetBool() && !insideLevelLoad || ( residency & IR_CPU ) ) { 
		image->referencedOutsideLevelLoad = true;
		image->ActuallyLoadImage();	// check for precompressed, load is from front end
		declManager->MediaPrint( "%ix%i %s\n", image->uploadWidth, image->uploadHeight, image->imgName.c_str() );
	} else {
		declManager->MediaPrint( "%s\n", image->imgName.c_str() );
	}
	return image;
}

/*
===============
idImageManager::GetImage
===============
*/
idImage *idImageManager::GetImage( const char *_name ) const {

	if ( !_name || !_name[0] || idStr::Icmp( _name, "default" ) == 0 || idStr::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}
	idImage	*image;

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	idStr name = _name;
	name.Remove( ".tga" );
	name.BackSlashesToSlashes();

	// look in loaded images
	int hash = name.FileNameHash();
	for ( image = imageHashTable[hash]; image; image = image->hashNext ) {
		if ( name.Icmp( image->imgName ) == 0 ) {
			return image;
		}
	}
	return NULL;
}

/*
===============
PurgeAllImages
===============
*/
void idImageManager::PurgeAllImages() {
	idImage	*image;
	for ( int i = 0; i < images.Num() ; i++ ) {
		image = images[i];
		image->PurgeImage();
	}
}

/*
===============
ReloadAllImages
===============
*/
void idImageManager::ReloadAllImages() {
	idCmdArgs args;

	// build the compressed normal map palette
	SetNormalPalette();

	args.TokenizeString( "reloadImages reload", false );
	R_ReloadImages_f( args );
}

/*
===============
R_CombineCubeImages_f

Used to combine animations of six separate tga files into
a serials of 6x taller tga files, for preparation to roq compress

FIXME : member vars could do with more scope definition
===============
*/
void R_CombineCubeImages_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: combineCubeImages <baseName>\n" );
		common->Printf( " combines basename[1-6][0001-9999].tga to basenameCM[0001-9999].tga\n" );
		common->Printf( " 1: forward 2:right 3:back 4:left 5:up 6:down\n" );
		return;
	}
	idStr	baseName = args.Argv( 1 );

	common->SetRefreshOnPrint( true );

	for ( int frameNum = 1 ; frameNum < 10000 ; frameNum++ ) {
		char	filename[MAX_IMAGE_NAME];
		byte	*pics[6];
		int		width, height;
		int		side;
		int		orderRemap[6] = { 1, 3, 4, 2, 5, 6 };
		for ( side = 0 ; side < 6 ; side++ ) {
			sprintf( filename, "%s%i%04i.tga", baseName.c_str(), orderRemap[side], frameNum );

			common->Printf( "reading %s\n", filename );
			R_LoadImage( filename, &pics[side], &width, &height, NULL );

			if ( !pics[side] ) {
				common->Printf( "not found.\n" );
				break;
			}

			// convert from "camera" images to native cube map images
			switch ( side ) {
				case 0:	// forward
					R_RotatePic( pics[side], width );
					break;
				case 1:	// back
					R_RotatePic( pics[side], width );
					R_HorizontalFlip( pics[side], width, height );
					R_VerticalFlip( pics[side], width, height );
					break;
				case 2:	// left
					R_VerticalFlip( pics[side], width, height );
					break;
				case 3:	// right
					R_HorizontalFlip( pics[side], width, height );
					break;
				case 4:	// up
					R_RotatePic( pics[side], width );
					break;
				case 5: // down
					R_RotatePic( pics[side], width );
					break;
			}
		}

		if ( side != 6 ) {
			for ( int i = 0 ; i < side ; side++ ) {
				Mem_Free( pics[side] );
			}
			break;
		}
		byte *combined = ( byte * )Mem_Alloc( width * height * 6 * 4 );

		for ( side = 0 ; side < 6 ; side++ ) {
			memcpy( combined + width * height * 4 * side, pics[side], width * height * 4 );
			Mem_Free( pics[side] );
		}
		sprintf( filename, "%sCM%04i.tga", baseName.c_str(), frameNum );
		common->Printf( "writing %s\n", filename );
		R_WriteTGA( filename, combined, width, height * 6 );

		Mem_Free( combined );
	}
	common->SetRefreshOnPrint( false );
}

/*
===============
CheckCvars
===============
*/
void idImageManager::CheckCvars() {
	// textureFilter stuff
	if ( image_filter.IsModified() || image_anisotropy.IsModified() || image_lodbias.IsModified() ) {
		ChangeTextureFilter();
		image_filter.ClearModified();
		image_anisotropy.ClearModified();
		image_lodbias.ClearModified();
	}
}

/*
===============
SumOfUsedImages
===============
*/
int idImageManager::SumOfUsedImages(int *numberOfUsed) {
	idImage	*image;
	int	total = 0, used = 0;

	for ( int i = 0; i < images.Num(); i++ ) {
		image = images[i];
		if ( image->frameUsed == backEnd.frameCount ) {
			total += image->StorageSize();
			used++;
		}
	}
	if (numberOfUsed)
		*numberOfUsed = used;
	return total;
}

/*
===============
Init
===============
*/
void idImageManager::Init() {

	memset( imageHashTable, 0, sizeof( imageHashTable ) );

	images.Resize( 1024, 1024 );

	// set default texture filter modes
	ChangeTextureFilter();

	blueNoise1024rgbaImage = ImageFromFile(
		"textures/internal/blue_noise_1024_rgba.tga",
		TF_NEAREST, false,	// must always be used in pixel-perfect way
		TR_REPEAT,			// tileable
		TD_HIGH_QUALITY 	// never compress
	);

	// create built in images
	defaultImage = ImageFromFunction( "_default", R_DefaultImage, TF_DEFAULT, true, TR_REPEAT, TD_HIGH_QUALITY, IR_BOTH );
	defaultImage->defaulted = true;
	whiteImage = ImageFromFunction( "_white", R_WhiteImage, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	blackImage = ImageFromFunction( "_black", R_BlackImage, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	flatNormalMap = ImageFromFunction( "_flat", R_FlatNormalImage, TF_DEFAULT, true, TR_REPEAT, TD_BUMP );
	ambientNormalMap = ImageFromFunction( "_ambient", R_AmbientNormalImage, TF_DEFAULT, true, TR_REPEAT, TD_BUMP );
	alphaNotchImage = ImageFromFunction( "_alphaNotch", R_AlphaNotchImage, TF_NEAREST, false, TR_CLAMP, TD_HIGH_QUALITY );
	fogImage = ImageFromFunction( "_fog", R_FogImage, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
	fogEnterImage = ImageFromFunction( "_fogEnter", R_FogEnterImage, TF_LINEAR, false, TR_CLAMP, TD_HIGH_QUALITY );
	noFalloffImage = ImageFromFunction( "_noFalloff", R_CreateNoFalloffImage, TF_DEFAULT, false, TR_CLAMP_TO_ZERO, TD_HIGH_QUALITY );
	ImageFromFunction( "_quadratic", R_QuadraticImage, TF_DEFAULT, false, TR_CLAMP, TD_HIGH_QUALITY );
	whiteCubeMapImage = ImageFromFunction( "_whiteCubeMap", R_MakeWhiteCubeMap, TF_LINEAR, false, TR_REPEAT, TD_HIGH_QUALITY );
	blackCubeMapImage = ImageFromFunction( "_blackCubeMap", R_MakeBlackCubeMap, TF_LINEAR, false, TR_REPEAT, TD_HIGH_QUALITY );
	ImageFromFunction( "_ambientWorldDiffuseCubeMap", R_AmbientWorldDiffuseCubeMap, TF_LINEAR, false, TR_REPEAT, TD_HIGH_QUALITY );
	ImageFromFunction( "_ambientWorldSpecularCubeMap", R_AmbientWorldSpecularCubeMap, TF_LINEAR, false, TR_REPEAT, TD_HIGH_QUALITY );

	// cinematicImage is used for cinematic drawing
	// scratchImage is used for screen wipes/doublevision etc..
	cinematicImage = ImageScratch( "_cinematic" );
	scratchImage = ImageScratch( "_scratch" );
	scratchImage2 = ImageScratch( "_scratch2" );
	// cameraN is used in security camera (see materials/tdm_camera.mtr)
	memset(cameraImages, 0, sizeof(cameraImages));
	for (int k = 1; k <= 9; k++)
		cameraImages[k] = ImageScratch( ("_camera" + idStr(k)).c_str() );
	xrayImage = ImageScratch( "_xray" );
	currentRenderImage = ImageScratch( "_currentRender" );
	guiRenderImage = ImageScratch( "_guiRender" );
	currentDepthImage = ImageScratch( "_currentDepth" ); // #3877. Allow shaders to access scene depth
	shadowDepthFbo = ImageScratch( "_shadowDepthFbo" );
	shadowAtlas = ImageScratch( "_shadowAtlas" );
	currentStencilFbo = ImageScratch( "_currentStencilFbo" );

	cmdSystem->AddCommand( "reloadImages", R_ReloadImages_f, CMD_FL_RENDERER, "reloads images" );
	cmdSystem->AddCommand( "listImages", R_ListImages_f, CMD_FL_RENDERER, "lists images" );
	cmdSystem->AddCommand( "combineCubeImages", R_CombineCubeImages_f, CMD_FL_RENDERER, "combines six images for roq compression" );

	// should forceLoadImages be here?
}

/*
===============
Shutdown
===============
*/
void idImageManager::Shutdown() {
	images.DeleteContents( true );
}

/*
====================
BeginLevelLoad

Mark all file based images as currently unused,
but don't free anything.  Calls to ImageFromFile() will
either mark the image as used, or create a new image without
loading the actual data.
====================
*/
void idImageManager::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0 ; i < images.Num() ; i++ ) {
		idImageAsset *image = images[ i ]->AsAsset();
		if ( !image )
			continue;	
		if ( com_purgeAll.GetBool() ) {
			image->PurgeImage();
		}
		image->levelLoadReferenced = false;
	}
}

/*
====================
EndLevelLoad

Free all images marked as unused, and load all images that are necessary.
This architecture prevents us from having the union of two level's
worth of data present at one time.

preload everything, never free
preload everything, free unused after level load
blocking load on demand
preload low mip levels, background load remainder on demand
====================
*/

static void R_LoadSingleImage( idImageAsset *image ) {
	R_LoadImageData( *image );
}
REGISTER_PARALLEL_JOB( R_LoadSingleImage, "R_LoadSingleImage" );

idCVar image_levelLoadParallel( "image_levelLoadParallel", "1", CVAR_BOOL|CVAR_ARCHIVE, "Parallelize texture creation during level load by fetching images from disk in the background" );

void idImageManager::EndLevelLoad() {
	const int start = Sys_Milliseconds();
	insideLevelLoad = false;

	common->Printf( "----- idImageManager::EndLevelLoad -----\n" );

	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge the ones we don't need
	for ( int i = 0 ; i < images.Num() ; i++ ) {
		idImageAsset *image = images[i]->AsAsset();
		if ( !image )
			continue;
		if ( !image->levelLoadReferenced && !image->referencedOutsideLevelLoad ) {
			//common->Printf( "Purging %s\n", image->imgName.c_str() );
			purgeCount++;
			image->PurgeImage();
		} else if ( image->texnum != idImage::TEXTURE_NOT_LOADED ) {
			//common->Printf( "Keeping %s\n", image->imgName.c_str() );
			keepCount++;
		}
	}
	common->PacifierUpdate( LOAD_KEY_IMAGES_START, images.Num() / LOAD_KEY_IMAGE_GRANULARITY ); // grayman #3763

	// load the ones we do need, if we are preloading
	idList<idImageAsset*> imagesToLoad;
	for ( int i = 0 ; i < images.Num() ; i++ ) {
		if ( idImageAsset *image = images[i]->AsAsset() ) {
			if ( image->levelLoadReferenced && ( image->texnum == idImage::TEXTURE_NOT_LOADED ) && image_preload.GetBool() ) {
				loadCount++;
				imagesToLoad.AddGrow( image );
			}
		}
	}

	// Process images in batches. If parallel load is enabled, we give the upcoming batch to the job queue so that the image
	// data is loaded and prepared in the background while we simultaneously upload the current batch to GPU memory.
	// Background loading is restricted to 2 threads. On SSDs and during hot loads, this has a considerable advantage over just
	// a single background thread, since decompression and calculation of image functions do take some of the time. SSDs do see
	// slight improvements with additional threads, but the difference is small. On HDDs, the additional thread does not offer
	// any advantages, but it should also not overload the disk, so that 2 threads is an acceptable compromise for all disk types.
	const int BATCH_SIZE = 16;
	idParallelJobList *imageLoadJobs = parallelJobManager->AllocJobList( JOBLIST_UTILITY, JOBLIST_PRIORITY_MEDIUM, BATCH_SIZE, 0, nullptr );

	for ( int curBatch = -BATCH_SIZE; curBatch < imagesToLoad.Num(); curBatch += BATCH_SIZE ) {
		for ( int i = curBatch + BATCH_SIZE; i < imagesToLoad.Num() && i < curBatch + 2 * BATCH_SIZE; ++i ) {
			idImageAsset *image = imagesToLoad[i];
			imageLoadJobs->AddJob((jobRun_t)R_LoadSingleImage, image);
		}

		int parallelism = 2;
		if ( !image_levelLoadParallel.GetBool() ) {
			parallelism = 0;
		}
		imageLoadJobs->Submit( nullptr, parallelism );

		for ( int i = idMath::Imax(curBatch, 0); i < imagesToLoad.Num() && i < curBatch + BATCH_SIZE; ++i ) {
			idImageAsset *image = imagesToLoad[i];
			R_UploadImageData( *image );

			// grayman #3763 - update the loading bar every LOAD_KEY_IMAGE_GRANULARITY images
			if ( ( i % LOAD_KEY_IMAGE_GRANULARITY ) == 0 ) {
				common->PacifierUpdate( LOAD_KEY_IMAGES_INTERIM, i );
			}
		}

		imageLoadJobs->Wait();
	}

	parallelJobManager->FreeJobList( imageLoadJobs );

	const int end = Sys_Milliseconds();
	common->Printf( "%5i purged from previous\n", purgeCount );
	common->Printf( "%5i kept from previous\n", keepCount );
	common->Printf( "%5i new loaded\n", loadCount );
	common->Printf( "all images loaded in %5.1f seconds\n", ( end - start ) * 0.001f );
	common->PacifierUpdate( LOAD_KEY_DONE, 0 ); // grayman #3763
	common->Printf( "----------------------------------------\n" );
}

/*
===============
idImageManager::StartBuild
===============
*/
void idImageManager::StartBuild() {
	ddsList.ClearFree();
	ddsHash.ClearFree();
}

/*
===============
idImageManager::FinishBuild
===============
*/
void idImageManager::FinishBuild( bool removeDups ) {
	idFile *batchFile;

	if ( removeDups ) {
		ddsList.Clear();
		char *buffer = NULL;
		fileSystem->ReadFile( "makedds.bat", ( void ** )&buffer );
		if ( buffer ) {
			idStr str = buffer;
			while ( str.Length() ) {
				int n = str.Find( '\n' );
				if ( n > 0 ) {
					idStr line = str.Left( n + 1 );
					idStr right;
					str.Right( str.Length() - n - 1, right );
					str = right;
					ddsList.AddUnique( line );
				} else {
					break;
				}
			}
		}
	}
	batchFile = fileSystem->OpenFileWrite( ( removeDups ) ? "makedds2.bat" : "makedds.bat" );

	if ( batchFile ) {
		int ddsNum = ddsList.Num();

		for ( int i = 0; i < ddsNum; i++ ) {
			batchFile->WriteFloatString( "%s", ddsList[ i ].c_str() );
			batchFile->Printf( "@echo Finished compressing %d of %d.  %.1f percent done.\n", i + 1, ddsNum, ( ( float )( i + 1 ) / ( float )ddsNum ) * 100.0f );
		}
		fileSystem->CloseFile( batchFile );
	}
	ddsList.ClearFree();
	ddsHash.ClearFree();
}

/*
===============
idImageManager::AddDDSCommand
===============
*/
void idImageManager::AddDDSCommand( const char *cmd ) {
	if ( !( cmd && *cmd ) ) {
		return;
	}
	const int key = ddsHash.GenerateKey( cmd, false );
	int	i;

	for ( i = ddsHash.First( key ); i != -1; i = ddsHash.Next( i ) ) {
		if ( ddsList[i].Icmp( cmd ) == 0 ) {
			break;
		}
	}

	if ( i == -1 ) {
		ddsList.Append( cmd );
	}
}

/*
===============
idImageManager::PrintMemInfo
===============
*/
void idImageManager::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, total = 0;
	int *sortIndex;
	idFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_images.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	sortIndex = new int[images.Num()];

	for ( i = 0; i < images.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i < images.Num() - 1; i++ ) {
		for ( j = i + 1; j < images.Num(); j++ ) {
			if ( images[sortIndex[i]]->StorageSize() < images[sortIndex[j]]->StorageSize() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( i = 0; i < images.Num(); i++ ) {
		idImage *im = images[sortIndex[i]];
		int size;

		size = im->StorageSize();
		total += size;

		f->Printf( "%s %3i %s\n", idStr::FormatNumber( size ).c_str(), im->refCount, im->imgName.c_str() );

		if ( idImageAsset *asset = im->AsAsset() ) {
			int cpuSize = 0;
			if (asset->cpuData.IsValid())
				cpuSize += asset->cpuData.GetTotalSizeInBytes();
			if (asset->compressedData)
				cpuSize += asset->compressedData->GetTotalSize();
			if (cpuSize)
				f->Printf( "%s %3i %s~CPU\n", idStr::FormatNumber( cpuSize ).c_str(), asset->refCount, asset->imgName.c_str() );
		}
	}
	delete[] sortIndex;
	mi->imageAssetsTotal = total;

	f->Printf( "\nTotal image bytes allocated: %s\n", idStr::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
}
