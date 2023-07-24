// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IMAGE_H__
#define __IMAGE_H__

/*
====================================================================

IMAGE

idImage have a one to one correspondance with OpenGL textures.

No texture is ever used that does not have a corresponding idImage.

No code outside this unit should call any of these OpenGL functions:

qglGenTextures
qglDeleteTextures
qglBindTexture

qglTexParameter

qglTexImage
qglTexSubImage

qglCopyTexImage
qglCopyTexSubImage

qglEnable( GL_TEXTURE_* )
qglDisable( GL_TEXTURE_* )

====================================================================
*/

#include "../libs/qglLib/qgl.h"

#include "../framework/FileSystem.h"
#include "../renderer/RendererEnums.h"

class idCVar;
class idMegaTexture;

typedef enum {
	IS_UNLOADED,	// no gl texture number
	IS_PARTIAL,		// has a texture number and the low mip levels loaded
	IS_LOADED		// has a texture number and the full mip hierarchy
} imageState_t;

static const int	MAX_TEXTURE_LEVELS = 14;

// surface description flags
const unsigned long DDSF_CAPS           = 0x00000001l;
const unsigned long DDSF_HEIGHT         = 0x00000002l;
const unsigned long DDSF_WIDTH          = 0x00000004l;
const unsigned long DDSF_PITCH          = 0x00000008l;
const unsigned long DDSF_PIXELFORMAT    = 0x00001000l;
const unsigned long DDSF_MIPMAPCOUNT    = 0x00020000l;
const unsigned long DDSF_LINEARSIZE     = 0x00080000l;
const unsigned long DDSF_DEPTH          = 0x00800000l;

// pixel format flags
const unsigned long DDSF_ALPHAPIXELS    = 0x00000001l;
const unsigned long DDSF_FOURCC         = 0x00000004l;
const unsigned long DDSF_RGB            = 0x00000040l;
const unsigned long DDSF_RGBA           = 0x00000041l;

// our extended flags
const unsigned long DDSF_ID_INDEXCOLOR	= 0x10000000l;
const unsigned long DDSF_ID_MONOCHROME	= 0x20000000l;

// dwCaps1 flags
const unsigned long DDSF_COMPLEX         = 0x00000008l;
const unsigned long DDSF_TEXTURE         = 0x00001000l;
const unsigned long DDSF_MIPMAP          = 0x00400000l;

#define DDS_MAKEFOURCC( a, b, c, d ) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef struct {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwFourCC;
    unsigned long dwRGBBitCount;
    unsigned long dwRBitMask;
    unsigned long dwGBitMask;
    unsigned long dwBBitMask;
    unsigned long dwABitMask;
} ddsFilePixelFormat_t;

typedef struct
{
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwHeight;
    unsigned long dwWidth;
    unsigned long dwPitchOrLinearSize;
    unsigned long dwDepth;
    unsigned long dwMipMapCount;
    unsigned long dwReserved1[11];
    ddsFilePixelFormat_t ddspf;
    unsigned long dwCaps1;
    unsigned long dwCaps2;
    unsigned long dwReserved2[3];
} ddsFileHeader_t;

struct mipmapState_t {
	enum colorType_e {
		MT_NONE,
		MT_DEFAULT,
		MT_WATER,
		MT_COLORLEVELS
	};

	bool operator == ( const mipmapState_t &a ) {
		return !memcmp( this, &a, sizeof( mipmapState_t ) );
	}

	bool operator != ( const mipmapState_t &a ) {
		return !operator ==( a );
	}

	float		color[4];	// Color to blend to
	float		blend[4];	// Blend factor for every channel
	colorType_e	colorType;
};

static const mipmapState_t defaultMipmapState = { {0,0,0,0}, {0,0,0,0}, mipmapState_t::MT_NONE };

#define	MAX_IMAGE_NAME	256

class idImageGeneratorFunctorBase {
public:
	virtual ~idImageGeneratorFunctorBase( void ) { }
	virtual void	operator()( class idImage *image ) const = 0;
};

template <class T> class idImageGeneratorFunctor : public idImageGeneratorFunctorBase {
public:
	typedef void( T::*func_t )( class idImage *image );

	void			Init( T* generatorClass, func_t imageGenerator ) {
						this->generatorClass = generatorClass;
						this->imageGenerator = imageGenerator;
					}

	virtual void	operator()( class idImage *image ) const {
						(*generatorClass.*imageGenerator)( image );
					}

private:
	T *				generatorClass;
	func_t			imageGenerator;
};

class idImageGeneratorFunctorGlobal : public idImageGeneratorFunctorBase {
public:
	typedef void( *func_t )( class idImage *image );

					idImageGeneratorFunctorGlobal( func_t imageGenerator ) {
						this->imageGenerator = imageGenerator;
					}

	virtual void	operator()( class idImage *image ) const {
						imageGenerator( image );
					}

protected:
	func_t			imageGenerator;
};

class idImage {
public:
						idImage();

	// Makes this image active on the current GL texture unit.
	// automatically enables or disables cube mapping or texture3D
	virtual void		Bind();

	// for use with fragment programs, doesn't change any enable2D/3D/cube states
	virtual void		BindFragment();
	virtual void		BindFragment( const int imageUnit );

	// deletes the texture object, but leaves the structure so it can be reloaded
	virtual void		Purge();

	virtual bool		IsLoaded() const;

	virtual void		SetMipmapLevel( byte *pixels, int width, int height, int level, mipmapState_t &state );

	// used by callback functions to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	virtual void		GenerateImage( const byte *pic, int width, int height,
											textureFilter_t filter, bool allowDownSize,
											textureRepeat_t repeat, textureDepth_t depth, mipmapState_t mipmapState = defaultMipmapState );

	virtual void		GenerateImageEx( const byte *pic, int width, int height,
											textureFilter_t filter, bool allowDownSize,
											textureRepeat_t repeat, textureDepth_t depth,
											int internalFormatParm = 0, int numMipLevels = -1 );

	virtual void		Generate3DImage( const byte *pic, int width, int height, int depth,
											textureFilter_t filter, bool allowDownSize,
											textureRepeat_t repeat, textureDepth_t minDepth );

	virtual void		GenerateCubeImage( const byte *pic[6], int size,
											textureFilter_t filter, bool allowDownSize,
											textureDepth_t depth );

	virtual void		GenerateMipmaps();	// uses hardware manual mipmap generation

	void				CopyFramebuffer( int x, int y, int width, int height, bool useOversizedBuffer );

	void				CopyFramebufferCube( int x, int y, int imageWidth, int imageHeight, int faceNum );

	void				CopyDepthbuffer( int x, int y, int width, int height );

	void				UploadScratch( const byte *pic, int width, int height );

	// Copy data from one image over to the other
	void				CopyFromImage( idImage *img );
	void				CopyFromImageCube( idImage *img );

	void				Download( byte **pixels, int *width, int *height );

	// just for resource tracking
	void				SetClassification( int tag );

	// estimates size of the GL image based on dimensions and storage type
	int					StorageSize() const;

	// print a one line summary of the image
	void				Print( bool csv ) const;

	// check for changed timestamp on disk and reload if necessary
	virtual void		Reload( bool checkPrecompressed, bool force );

//==========================================================

	void				GetDownsize( int &scaled_width, int &scaled_height ) const;
	virtual void		MakeDefault();	// fill with a grid pattern
#if defined( _XENON )
	void				FromParameters( int width, int height, int internalFormat, textureType_t type, textureFilter_t filter, textureRepeat_t repeat, bool linearTexture = true );
#else
	void				FromParameters( int width, int height, int internalFormat, textureType_t type, textureFilter_t filter, textureRepeat_t repeat );
#endif
	void				SetImageFilterAndRepeat() const;
	bool				CanBePartialLoaded();
	void				WritePrecompressedImage();
	bool				CheckPrecompressedImage( bool fullLoad );
	void				UploadPrecompressedImage( byte *data, int len );
	void				ActuallyLoadImage( bool checkForPrecompressed );
	bool				StartBackgroundImageLoad();
	int					BitsForInternalFormat( int internalFormat ) const;
	void				UploadCompressedNormalMap( int width, int height, const byte *rgba, int mipLevel );
	GLenum				SelectInternalFormat( const byte **dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth ) const;
	void				ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const;

	static int			NumLevelsForImageSize( int width, int height );
	static int			NumLevelsForImageSize( int width, int height, int internalFormat );
	static int			DataSizeForImageSize( int width, int height, int internalFormat );

	void				SetLodDistance( float distance );

private:
	void				VerifyInternalFormat( GLenum target ) const;

public:
	// data commonly accessed is grouped here
	static const int	TEXTURE_NOT_LOADED = 0;
	GLuint				texnum;					// gl texture binding, will be TEXTURE_NOT_LOADED if not loaded
	textureType_t		type;
	int					frameUsed;				// for texture usage in frame statistics
	int					bindCount;				// incremented each bind

	// background loading information
	idImage*				partialImage;				// shrunken, space-saving version
	bool					isPartialImage;				// true if this is pointed to by another image
	bool					backgroundLoadInProgress;	// true if another thread is reading the complete d3t file
	backgroundDownload_t	bgl;
	idImage*				bglNext;					// linked from tr.backgroundImageLoads

	//lodding information
	bool				distanceLod;			// we are to far away, a lower res version can be used just fine
	float				smallestDistanceSeen;	// The smallest distance seen so far for the lod parameter
	int					frameOfDistance;		// The frame the lod was last set

	// parameters that define this image
	idStr				imgName;				// game path, including extension (except for cube maps), may be an image program
	const idImageGeneratorFunctorBase *	generatorFunction;	// NULL for files
	bool				fromParams;
	bool				allowDownSize;			// this also doubles as a don't-partially-load flag
	mutable bool		isMonochrome;
	int					picMipOfs;
	int					picMipMin;
	float				anisotropy;
	textureFilter_t		filter;
	textureRepeat_t		repeat;
	textureDepth_t		depth;
	cubeFiles_t			cubeFiles;				// determines the naming and flipping conventions for the six images
	mipmapState_t		mipmapState;			// Mipmap level coloring
	int					numMipLevels;
	float				minLod;
	float				maxLod;

	bool				referencedOutsideLevelLoad;
	bool				levelLoadReferenced;	// for determining if it needs to be purged
	bool				precompressedFile;		// true when it was loaded from a .d3t file
	bool				defaulted;				// true if the default image was generated because a file couldn't be loaded
	unsigned			timestamp;				// the most recent of all images used in creation, for reloadImages command

	int					imageHash;				// for identical-image checking

	int					classification;			// just for resource profiling

	int					sourceWidth, sourceHeight;				// after power of two, before downsample

	// data for listImages
	int					uploadWidth, uploadHeight, uploadDepth;	// after power of two, downsample, and MAX_TEXTURE_SIZE
	int					internalFormat;

//	idImage 			*cacheUsagePrev, *cacheUsageNext;	// for dynamic cache purging of old images

	idImage *			hashNext;				// for hash chains to speed lookup
};

ID_INLINE idImage::idImage() {
	texnum = TEXTURE_NOT_LOADED;
	partialImage = NULL;
	type = TT_DISABLED;
	isPartialImage = false;
	frameUsed = 0;
	classification = 0;
	backgroundLoadInProgress = false;
	bgl.opcode = DLTYPE_FILE;
	bgl.fh = NULL;
	bglNext = NULL;
	imgName[0] = '\0';
	generatorFunction = NULL;
	fromParams = false;
	allowDownSize = false;
	picMipOfs = 0;
	picMipMin = -10;
	anisotropy = -1.f;
	filter = TF_DEFAULT;
	repeat = TR_REPEAT;
	depth = TD_DEFAULT;
	cubeFiles = CF_2D;
	mipmapState = defaultMipmapState;
	numMipLevels = 0;
	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	precompressedFile = false;
	defaulted = false;
	timestamp = 0;
	bindCount = 0;
	sourceWidth = sourceHeight = 0;
	uploadWidth = uploadHeight = uploadDepth = 0;
	internalFormat = 0;
	hashNext = NULL;
	distanceLod = false;
	frameOfDistance = 0;
	smallestDistanceSeen = 0.0f;
	isMonochrome = false;
	minLod = 0.f;
	maxLod = 1000.f;
}


/*
==================
idImage::NumLevelsForImageSize

	FIXME: the switch statement is far from complete
==================
*/
ID_INLINE int idImage::NumLevelsForImageSize( int width, int height, int internalFormat ) {
	int	numLevels = 1;
	int minSize;

	switch( internalFormat ) {
		case GL_RGBA8:
			minSize = 1;
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			minSize = 4;
			break;
	}

	while ( width > minSize || height > minSize ) {
		numLevels++;
		width >>= 1;
		height >>= 1;
	}

	return numLevels;
}

/*
==================
idImage::DataSizeForImageSize

	FIXME: the switch statement is far from complete
==================
*/
ID_INLINE int idImage::DataSizeForImageSize( int width, int height, int internalFormat ) {
	int numLevels = NumLevelsForImageSize( width, height, internalFormat );

	int dataSize = 0;

	for ( int i = 0; i < numLevels; i++ ) {
		switch( internalFormat ) {
			case GL_RGBA8:
				dataSize += width * height * 4;
				break;
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				assert( width == height );
				dataSize += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * 8;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				assert( width == height );
				dataSize += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * 16;
				break;
			default:
				dataSize = -1;
		}

		width >>= 1;
		height >>= 1;
	}

	return dataSize;
}

class idImageCopyFunctorGlobal : public idImageGeneratorFunctorGlobal {
public:
	idImageCopyFunctorGlobal( func_t defaultGenerator ) :
	  idImageGeneratorFunctorGlobal( defaultGenerator ),
	  srcImage( NULL ),
	  dstImage( NULL ) { ; }

	virtual void operator()( idImage *image ) const {
		dstImage = image;
		if ( !srcImage ) {
			imageGenerator( dstImage );
		} else {
			// Set source to NULL, when we do a reloadimages this causes
			// the (!source) case to be executed recusively first, so
			// a new image is created and infinite recursion is avoided.
			idImage *tempSource = srcImage;
			srcImage = NULL;
			dstImage->CopyFromImage( tempSource );
			//tempSource->Purge();
			srcImage = tempSource;
		}
	}

	virtual void SetSource( idImage *image ) {
		srcImage = image;
		if ( dstImage ) {
			dstImage->CopyFromImage( srcImage );
			//Do a purge since the source image is probably not going to be used directly ever
			//source->Purge();
		}
	}

protected:
	mutable idImage *	srcImage;
	mutable idImage *	dstImage;
};

struct imageParams_t {
	textureFilter_t	tf;
	textureRepeat_t	trp;
	textureDepth_t	td;
	cubeFiles_t		cubeMap;
	mipmapState_t	mipState;
	bool			allowPicmip;
	int				picmipofs;
	int				picMipMin;
	float			anisotropy;
	bool			partialLoad;
	float			minLod;
	float			maxLod;

	imageParams_t::imageParams_t() {
		Clear();
	}
	imageParams_t::imageParams_t( textureFilter_t _filter, bool _allowDownSize, int _picmipofs, float _anisotropy,
		textureRepeat_t _repeat, textureDepth_t _depth, cubeFiles_t _cubeMap = CF_2D, mipmapState_t _mipmapState = defaultMipmapState ) {
		Clear();

		tf = _filter;
		allowPicmip = _allowDownSize;
		picmipofs = _picmipofs;
		anisotropy = _anisotropy;
		trp = _repeat;
		td = _depth;
		cubeMap = _cubeMap;
		mipState = _mipmapState;
	}

	void Clear() {
		tf = TF_DEFAULT;
		trp = TR_REPEAT;
		td = TD_DEFAULT;
		cubeMap = CF_2D;
		mipState = defaultMipmapState;
		allowPicmip = true;
		picmipofs = 0;
		partialLoad = false;
		anisotropy = -1.f;
		minLod = 0.f;
		maxLod = 1000.f;
		picMipMin = -10;
	}
};

static const imageParams_t defaultImageParms;

class idImageManager {
public:
	void				Init();
	void				PreSys3DShutdown();
	void				Shutdown();

	bool				IsInitialized() const { return ( images.Num() != 0 ); }

	static idImage*		ParseImage( idParser& src, const imageParams_t& defaultParms = defaultImageParms );

	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default
	// grid pattern.
	// Will automatically resample non-power-of-two images and execute image programs if needed.
	virtual idImage *			ImageFromFile( const char *name, imageParams_t params );
												//textureFilter_t filter, bool allowDownSize, int picmipofs, float anisotropy,
												//textureRepeat_t repeat, textureDepth_t depth, cubeFiles_t cubeMap = CF_2D, mipmapState_t mipmapState = defaultMipmapState,
												//bool partialLoad = false );

	// look for a loaded image, whatever the parameters
	idImage*					GetImage( const char* name ) const;

	// The callback will be issued immediately, and later if images are reloaded or vid_restart
	// The callback function should call one of the idImage::Generate* functions to fill in the data
	virtual idImage *			ImageFromFunction( const char *name, const idImageGeneratorFunctorBase & generatorFunction );

	// called once a frame to allow any background loads that have been completed
	// to turn into textures.
	void						CompleteBackgroundImageLoads();

#if defined( _XENON )
	virtual idImage*			ImageFromParameters( const char* name, int width, int height, int internalFormat, textureType_t type, textureFilter_t filter, textureRepeat_t repeat, bool linearTexture = true );
#else
	virtual idImage*			ImageFromParameters( const char* name, int width, int height, int internalFormat, textureType_t type, textureFilter_t filter, textureRepeat_t repeat );
#endif

	virtual idMegaTexture*		MegaTextureFromFile( const char *name );

	// returns the number of bytes of image data bound in the previous frame
	int					SumOfUsedImages();

	// called each frame to allow some cvars to automatically force changes
	void				CheckCvars();

	void				PurgeAllMegaTextures();

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadAllImages();

	// disable the active texture unit
	virtual void		BindNull();

	// Mark all file based images as currently unused,
	// but don't free anything.  Calls to ImageFromFile() will
	// either mark the image as used, or create a new image without
	// loading the actual data.
	// Called only by renderSystem::BeginLevelLoad
	void				BeginLevelLoad();

	// Free all images marked as unused, and load all images that are necessary.
	// This architecture prevents us from having the union of two level's
	// worth of data present at one time.
	// Called only by renderSystem::EndLevelLoad
	void				EndLevelLoad();

	void				SetInsideLevelLoad( bool value ) { insideLevelLoad = value; }

	void				LevelStart();

	// During level load, there is the requirement to load textures at two points. Once
	// after the level loading GUI has been loaded, and once in EndLevelLoad.
	int					LoadPendingImages( bool updatePacifier = true );

	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild();
	void				AddDDSCommand( const char* sourceFile, const char* destFile, const char* codec, const char* params );

	virtual void		LoadImage( const char* fileName, byte **pic, int *width, int *height, unsigned *timestamp, bool makePowerOf2 );

	virtual void		FreeImageBuffer( byte*& buffer );

	// data is RGB or RGBA
	virtual void		WriteTGA( const char* fileName, const byte* data, int width, int height, int depth = 4, bool swapBGR = true, bool flipVertical = false );
	virtual int			WriteTGABuffer( byte*& outBuffer, const byte* data, int width, int height, int depth = 4, bool swapBGR = true, bool flipVertical = false );

	// data is RGB or RGBA
	virtual void		WriteBMP( const char* fileName, const byte* data, int width, int height, int depth = 4 );
	virtual int			WriteBMPBuffer( byte*& outBuffer, const byte* data, int width, int height, int depth = 4 );


	// data is an 8 bit index into palette, which is RGB (no A)
	virtual void		WritePalTGA( const char *filename, const byte *data, const byte *palette, int width, int height, bool flipVertical = false );

	void				GetPureServerChecksum( unsigned int& checksum );

	// cvars
	static idCVar		image_roundDown;			// round bad sizes down to nearest power of two
	static idCVar		image_colorMipLevels;		// development aid to see texture mip usage
	static idCVar		image_useCompression;		// 0 = force everything to high quality
	static idCVar		image_filter;				// changes texture filtering on mipmapped images
	static idCVar		image_anisotropy;			// set the maximum texture anisotropy if available
	static idCVar		image_lodbias;				// change lod bias on mipmapped images
	static idCVar		image_useAllFormats;		// allow alpha/intensity/luminance/luminance+alpha
#ifdef ID_ALLOW_TOOLS
	static idCVar		image_usePrecompressedTextures;	// use .dds files if present
	static idCVar		image_writePrecompressedTextures; // write .dds files if necessary
#endif
	static idCVar		image_writeNormalTGA;		// debug tool to write out .tgas of the final normal maps
	static idCVar		image_writeNormalTGAPalletized;		// debug tool to write out palletized versions of the final normal maps
	static idCVar		image_writeTGA;				// debug tool to write out .tgas of the non normal maps
	static idCVar		image_useNormalCompression;	// 1 = use 256 color compression for normal maps if available, 2 = use rxgb compression
	static idCVar		image_useOffLineCompression; // will write a batch file with commands for the offline compression
	static idCVar		image_skipUpload;			// used during the build process, will skip uploads
	static idCVar		image_useBackgroundLoads;	// 1 = enable background loading of images
	static idCVar		image_showBackgroundLoads;	// 1 = print number of outstanding background loads
	static idCVar		image_ignoreHighQuality;	// ignore high quality on materials
	static idCVar		image_detailPower;			// controls how fast the detail textures fade out
	static idCVar		image_picMipEnable;				// Controls texture miplevel downscale
	static idCVar		image_picMip;				// Controls texture miplevel downscale
	static idCVar		image_editorPicMip;			// Controls texture miplevel downscale
#if !defined( SD_PUBLIC_BUILD )
	static idCVar		image_globalPicMip;			// Controls texture miplevel downscale
#endif // !SD_PUBLIC_BUILD
	static idCVar		image_bumpPicMip;			// Controls texture miplevel downscale
	static idCVar		image_diffusePicMip;		// Controls texture miplevel downscale
	static idCVar		image_specularPicMip;		// Controls texture miplevel downscale

	// built-in images
	idImage *			defaultImage;
	idImage *			defaultMaterialImage;
	idImage *			flatNormalMap;				// 128 128 255 in all pixels
	idImage *			rampImage;					// 0-255 in RGBA in S
	idImage *			alphaRampImage;				// 0-255 in alpha, 255 in RGB
	idImage *			alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	idImage *			whiteImage;					// full of 0xff
	idImage *			grayImage;					// full of 0x77
	idImage *			blackImage;					// full of 0x00
	idImage *			normalCubeMapImage;			// cube map to normalize STR into RGB
	idImage *			blackCubeMapImage;			// just all black
	idImage *			noFalloffImage;				// all 255, but zero clamped
	idImage *			fogImage;					// increasing alpha is denser fog
	idImage *			fogEnterImage;				// adjust fogImage alpha based on terminator plane
	idImage *			cinematicImage;
	idImage *			cinematicYImage;
	idImage *			cinematicUImage;
	idImage *			cinematicVImage;
	idImage *			scratchImage;
	idImage *			currentRenderImage;			// for SS_POST_PROCESS shaders
	idImage *			currentDepthImage;
	idImage *			postProcessBuffer[2];
#if 0
	GLuint				postProcessBufferFBO[2];
#endif
	idImage *			scratchCubeMapImage;
	idImage *			scratchImage2;				// PENTA: TEST
	idImage *			noise;					// 32x32x32x8bit noise image
	idImage *			specularTableImage;			// 1D intensity texture with our specular function
	idImage *			specular2DTableImage;		// 2D intensity texture with our specular function with variable specularity
	idImage *			borderClampImage;			// white inside, black outside
	idImage *			dither[16];
	idImage *			defaultDetailMaskImage;
	idImage *			diffusionMask;

	//--------------------------------------------------------

	virtual idImage *	AllocImage( const char *name );
	void				SetNormalPalette();
	void				ChangeTextureFilter();

	idList<idImage*>	images;
	idStrList			ddsSourceFileList;
	idStrList			ddsDestFileList;
	idStrList			ddsCodecList;
	idStrList			ddsParamList;
	idHashIndex			ddsHash;

	idList<idMegaTexture*>	megaTextures;

	bool				insideLevelLoad;			// don't actually load images now

	byte				originalToCompressed[256];	// maps normal maps to 8 bit textures
	byte				compressedPalette[768];		// the palette that normal maps use

	// default filter modes for images
	GLenum				textureMinFilter;
	GLenum				textureMaxFilter;
	float				textureAnisotropy;
	float				textureLODBias;

	idImage *			imageHashTable[FILE_HASH_SIZE];

	idImage *			backgroundImageLoads;		// chain of images that have background file loads active

	int					numActiveBackgroundImageLoads;
	static const int	MAX_BACKGROUND_IMAGE_LOADS = 8;
};

extern idImageManager	*globalImages;		// pointer to global list for the rest of the system

/*
====================================================================

IMAGEPROCESS

FIXME: make an "imageBlock" type to hold byte*,width,height?
====================================================================
*/

byte *R_Dropsample( const byte *in, int inwidth, int inheight,
							int outwidth, int outheight );
byte *R_ResampleTexture( const byte *in, int inwidth, int inheight,
							int outwidth, int outheight );
byte *R_MipMapWithAlphaSpecularity( const byte *in, int width, int height );
byte *R_MipMap( const byte *in, int width, int height, bool preserveBorder );
byte *R_MipMap3D( const byte *in, int width, int height, int depth, bool preserveBorder );

// these operate in-place on the provided pixels
void R_SetBorderTexels( byte *inBase, int width, int height, const byte border[4] );
void R_SetBorderTexels3D( byte *inBase, int width, int height, int depth, const byte border[4] );
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4], byte amount );
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4], byte amount[4] );
void R_HorizontalFlip( byte *data, int width, int height );
void R_VerticalFlip( byte *data, int width, int height );
void R_RotatePic( byte *data, int width );
void R_SetAlphaChannel( byte *data, int pixelCount, byte alpha );
void R_MakeVirtualTexture( const char *out, int tileSize, int pad, byte *data, int w, int h, bool preserveBorder );

/*
====================================================================

IMAGEFILES

====================================================================
*/

void LoadBMP( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, bool markPaksReferenced = true );
void LoadTGA( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, bool markPaksReferenced = true );

void R_LoadImage( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, bool makePowerOf2 );

// pic is in top to bottom raster format
bool R_LoadCubeImages( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, unsigned *timestamp );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, textureDepth_t *depth = NULL );
const char *R_ParsePastImageProgram( idParser &src );

typedef struct {
	float percent;
	idStr name;
	idVec4 color;
} imageClassUsage_t;

float R_GetImageUsageData( idList< imageClassUsage_t > &target );

/*
==============
sdImageSequence
==============
*/
class sdImageSequence {

	idList<idImage*> images;
	float rate;

public:

	sdImageSequence();
	~sdImageSequence();

	void Cleanup( void );

	void SetRate( float r );
	void AddImage( idImage *img );

	void UpdateBindings( void );
};

#endif /* !__IMAGE_H__ */
