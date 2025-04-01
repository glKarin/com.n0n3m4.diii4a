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

#ifndef __R_IMAGE_H__
#define __R_IMAGE_H__

/*
====================================================================

IMAGE

idImage have a one to one correspondance with OpenGL textures.

No texture is ever used that does not have a corresponding idImage.

no code outside this unit should call any of these OpenGL functions:

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

#define	MAX_TEXTURE_LEVELS				14

// surface description flags
const unsigned int DDSF_CAPS           = 0x00000001l;
const unsigned int DDSF_HEIGHT         = 0x00000002l;
const unsigned int DDSF_WIDTH          = 0x00000004l;
const unsigned int DDSF_PITCH          = 0x00000008l;
const unsigned int DDSF_PIXELFORMAT    = 0x00001000l;
const unsigned int DDSF_MIPMAPCOUNT    = 0x00020000l;
const unsigned int DDSF_LINEARSIZE     = 0x00080000l;
const unsigned int DDSF_DEPTH          = 0x00800000l;

// pixel format flags
const unsigned int DDSF_ALPHAPIXELS    = 0x00000001l;
const unsigned int DDSF_FOURCC         = 0x00000004l;
const unsigned int DDSF_RGB            = 0x00000040l;
const unsigned int DDSF_RGBA           = 0x00000041l;

// dwCaps1 flags
const unsigned int DDSF_COMPLEX         = 0x00000008l;
const unsigned int DDSF_TEXTURE         = 0x00001000l;
const unsigned int DDSF_MIPMAP          = 0x00400000l;

#define DDS_MAKEFOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;
	unsigned int dwRGBBitCount;
	unsigned int dwRBitMask;
	unsigned int dwGBitMask;
	unsigned int dwBBitMask;
	unsigned int dwABitMask;
} ddsFilePixelFormat_t;

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	ddsFilePixelFormat_t ddspf;
	unsigned int dwCaps1;
	unsigned int dwCaps2;
	unsigned int dwReserved2[3];
} ddsFileHeader_t;

bool IsImageFormatCompressed( int internalFormat );
int SizeOfCompressedImage( int width, int height, int internalFormat );
void CompressImage( int internalFormat, byte *compressedPtr, const byte *srcPtr, int width, int height, int stride = 0 );
void DecompressImage( int internalFormat, const byte *compressedPtr, byte *dstPtr, int width, int height, int stride = 0 );

// increasing numeric values imply more information is stored
typedef enum {
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// will use compressed formats when possible
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_HIGH_QUALITY			// either 32 bit or a component format, no loss at all
} textureDepth_t;

typedef enum {
	TT_DISABLED,
	TT_2D,
	TT_CUBIC,
} textureType_t;

typedef enum {
	CF_2D,			// not a cube map
	CF_NATIVE,		// _px, _nx, _py, etc, directly sent to GL
	CF_CAMERA,		// _forward, _back, etc, rotated and flipped as needed before sending to GL
	CF_NONE,		// this value is ignored
} cubeFiles_t;

#define	MAX_IMAGE_NAME	256
#define MIN_IMAGE_NAME  4

typedef enum {
	IR_NONE = 0,			// should never happen
	IR_GRAPHICS = 0x1,
	IR_CPU = 0x2,
	IR_BOTH = IR_GRAPHICS | IR_CPU,
} imageResidency_t;

// stgatilov: represents uncompressed image data on CPU side in RGBA8 format
typedef struct imageBlock_s {
	byte *pic[6];
	int width;
	int height;
	int sides;			//six for cubemaps, one for the others

	ID_FORCE_INLINE byte *GetPic(int side = 0) const {
		assert(dword(side) < dword(sides));
		return pic[side];
	}
	ID_FORCE_INLINE bool IsValid() const { return pic[0] != nullptr; }
	ID_FORCE_INLINE bool IsCubemap() const { return sides == 6; }
	ID_FORCE_INLINE int GetSizeInBytes() const { return width * height * 4; }
	int GetTotalSizeInBytes() const { return sides * GetSizeInBytes(); }
	void Purge();
	static imageBlock_s Alloc2D(int w, int h);
	static imageBlock_s AllocCube(int size);

	ID_FORCE_INLINE byte* FetchPtr(int s, int t, int side = 0) const {
		assert(dword(side) < dword(sides) && dword(s) < dword(width) && dword(t) < dword(height));
		return &pic[side][4 * (t * width + s)];
	}
	ID_FORCE_INLINE idVec4 Fetch(int s, int t, int side = 0) const {
		const byte *p = FetchPtr(s, t, side);
		return idVec4(p[0], p[1], p[2], p[3]) * (1.0f / 255.0f);
	}
	idVec4 Sample(float s, float t, textureFilter_t filter, textureRepeat_t repeat, int side = 0) const;
	idVec4 SampleCube(const idVec3 &dir, textureFilter_t filter) const;
} imageBlock_t;

// stgatilov: cubeMapAxes[f] --- coordinate system of f-th face of cubemap
// [2]-th axis is major axis, [0] and [1] are directions of U and V texcoords growth
// perfectly matches OpenGL specs (CF_NATIVE layout) 
extern const idVec3 cubeMapAxes[6][3];


// stgatilov: represents compressed texture as contents of DDS file
typedef struct imageCompressedData_s {
	int fileSize;				//size of tail starting from "magic"
	int _;						//(padding)

	//----- data below is stored in DDS file -----
	dword magic;				//always must be "DDS "in little-endian
	ddsFileHeader_t header;		//DDS file header (124 bytes)
	byte contents[1];			//the rest of file (variable size)

	byte *GetFileData() const { return (byte*)&magic; }
	static int FileSizeFromContentSize(int contentSize) { return contentSize + 4 + sizeof(ddsFileHeader_t); }
	static int TotalSizeFromFileSize(int fileSize) { return fileSize + 8; }
	static int TotalSizeFromContentSize(int contentSize) { return TotalSizeFromFileSize(FileSizeFromContentSize(contentSize)); }
	int GetTotalSize() const { return TotalSizeFromFileSize(fileSize); }

	int GetWidth() const { return header.dwWidth; }
	int GetHeight() const { return header.dwHeight; }
	byte *ComputeUncompressedData() const;
} imageCompressedData_t;
static_assert(offsetof(imageCompressedData_s, contents) - offsetof(imageCompressedData_s, magic) == 128, "Wrong imageCompressedData_t layout");

enum ImageType {
	IT_UNKNOWN = 0,
	IT_ASSET,
	IT_SCRATCH,
};
class idImageAsset;
class idImageScratch;

// defines where to take image data from
struct ImageAssetSource {
	// case 1: load from file(s)
	idStr filename;
	cubeFiles_t cubeFiles = CF_NONE; // can only specify for files
	// case 2: load from function
	imageBlock_t (*generatorFunction)() = nullptr;
};

class LoadStack;

class idImage {
public:
	idImage();
	virtual ~idImage() = default;

	static const ImageType Type = IT_UNKNOWN;
	virtual ImageType GetType() const { return Type; }
	virtual idImageAsset *AsAsset() { return nullptr; }
	virtual idImageScratch *AsScratch() { return nullptr; }

	// Makes this image active on the current GL texture unit.
	// automatically enables or disables cube mapping or texture3D
	// May perform file loading if the image was not preloaded.
	// May start a background image read.
	void		Bind();

	// checks if the texture is currently bound to the specified texture unit
	bool		IsBound( int textureUnit ) const;

	// deletes the texture object, but leaves the structure so it can be reloaded
	virtual void PurgeImage();

	// estimates size of the GL image based on dimensions and storage type
	int			StorageSize() const;

	// print a one line summary of the image
	virtual void Print() const;

	void		AddReference()				{ refCount++; };

	//==========================================================

	void		SetImageFilterAndRepeat() const;
	static int	BitsForInternalFormat( int internalFormat, bool gpu = false );
	static int	NumLevelsForImageSize( int width, int height );

	// data commonly accessed is grouped here
	static const int TEXTURE_NOT_LOADED = -1;
	GLuint				texnum;					// gl texture binding, will be TEXTURE_NOT_LOADED if not loaded
	textureType_t		type;
	int					frameUsed;				// for texture usage in frame statistics
	int					bindCount;				// incremented each bind

	// parameters that define this image
	idStr				imgName;				// game path, including extension (except for cube maps), may be an image program
	textureFilter_t		filter;
	textureRepeat_t		repeat;
	textureDepth_t		depth;

	// data for listImages
	int					uploadWidth, uploadHeight;	// after power of two, downsample, and MAX_TEXTURE_SIZE
	int					internalFormat;
	const GLint *		swizzleMask;			// replacement for deprecated intensity/luminance formats

	idImage *			hashNext;				// for hash chains to speed lookup

	int					refCount;				// overall ref count
};

// read-only texture loaded from some source (file or generator)
// this texture can be shared across many users
class idImageAsset : public idImage {
public:
	idImageAsset();

	static const ImageType Type = IT_ASSET;
	virtual ImageType GetType() const override { return Type; }
	virtual idImageAsset *AsAsset() override { return this; }

	// used internally to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	void GenerateImage( const byte *pic, int width, int height,
		textureFilter_t filter, bool allowDownSize,
		textureRepeat_t repeat, textureDepth_t depth,
		imageResidency_t residency = IR_GRAPHICS );
	void GenerateCubeImage( const byte *pic[6], int size,
		textureFilter_t filter, bool allowDownSize,
		textureDepth_t depth );

	// check for changed timestamp on disk and reload if necessary
	void Reload( bool checkPrecompressed, bool force );

	virtual void PurgeImage() override;

	void ActuallyLoadImage( void );

	void MakeDefault();	// fill with a grid pattern

	// just for resource tracking
	void SetClassification( int tag );

	virtual void Print() const override;

	idVec4 Sample(float s, float t) const;	//  2D texture only
	idVec4 Sample(float x, float y, float z) const;	// cubemap only

	void GetDownsize( int &scaled_width, int &scaled_height ) const;
	void WritePrecompressedImage();
	bool CheckPrecompressedImage( bool fullLoad );
	void UploadPrecompressedImage( void );
	static GLenum SelectInternalFormat( byte const* const* dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth, GLint const* *swizzleMask = nullptr );
	void ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const;

	// total number of bytes allocated by this image on CPU right now
	int SizeOfCpuData() const;

public:
	int					classification;			// just for resource profiling

	bool				referencedOutsideLevelLoad;
	bool				levelLoadReferenced;	// for determining if it needs to be purged
	bool				precompressedFile;		// true when it was loaded from a .d3t file
	bool				defaulted;				// true if the default image was generated because a file couldn't be loaded
	ID_TIME_T			timestamp;				// the most recent of all images used in creation, for reloadImages command

	bool				allowDownSize;			// this also doubles as a don't-partially-load flag

	// stgatilov: where asset data is loaded from
	ImageAssetSource	source;

	// stgatilov: storing image data on CPU side
	imageBlock_t		cpuData;				// CPU-side usable image data (usually absent)
	imageResidency_t	residency;				// determines whether cpuData and/or texnum should be valid
	imageCompressedData_t *compressedData;		// CPU-side compressed texture contents (aka DDS file)

	//stgatilov: information about why and how this image was loaded (may be missing)
	LoadStack *			loadStack;
};

// texture with volatile contents
// it is usually generated as attachment for framebuffer object
// some textures are generated by copying contents of other textures ("_scratch")
class idImageScratch : public idImage {
public:
	idImageScratch();

	static const ImageType Type = IT_SCRATCH;
	virtual ImageType GetType() const override { return Type; }
	virtual idImageScratch *AsScratch() override { return this; }

	virtual void Print() const override;

	void GenerateAttachment( int width, int height, GLenum format,
		GLenum filter = GL_LINEAR, GLenum wrapMode = GL_CLAMP_TO_EDGE,
		int lodLevel = 0 );

	void UploadScratch( const byte *pic, int width, int height );

public:
	// #6434: if such an image is referenced in material stage,
	// it is replaced with subview-generated image (mirror, remote, X-ray, etc.)
	bool isDynamicImagePlaceholder;
};

class idImageManager {
public:
	void				Init();
	void				Shutdown();

	// Generic way to create idImageAsset from any source
	idImageAsset *		ImageFromSource(
		const idStr &name, ImageAssetSource source,
		textureFilter_t filter, bool allowDownSize,
		textureRepeat_t repeat, textureDepth_t depth,
		imageResidency_t residency = IR_GRAPHICS
	);
	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default grid pattern.
	// Will automatically resample non-power-of-two images and execute image programs if needed.
	// Note: you can query an image that was previously created with ImageFromFunction by name.
	idImageAsset *		ImageFromFile( const char *name,
		textureFilter_t filter, bool allowDownSize,
		textureRepeat_t repeat, textureDepth_t depth, cubeFiles_t cubeMap = CF_2D,
		imageResidency_t residency = IR_GRAPHICS
	) {
		idStr strname = name;
		// strip any .tga file extensions from anywhere in the _name, including image program parameters
		strname.Remove( ".tga" );
		strname.BackSlashesToSlashes();
		return ImageFromSource( strname, ImageAssetSource{ strname, cubeMap, nullptr }, filter, allowDownSize, repeat, depth, residency );
	}
	// The callback will be issued when image is generated, and later if images are reloaded or vid_restart
	// The callback function returns CPU-side uncompressed data for the image, with ownership over memory passed to the caller
	idImageAsset *		ImageFromFunction( const char *name, imageBlock_t ( *generatorFunction )(),
		textureFilter_t filter, bool allowDownSize,
		textureRepeat_t repeat, textureDepth_t depth,
		imageResidency_t residency = IR_GRAPHICS
	) {
		return ImageFromSource( name, ImageAssetSource{ "", CF_NONE, generatorFunction }, filter, allowDownSize, repeat, depth, residency );
	}

	// look for a loaded image, whatever the parameters
	idImage *			GetImage( const char *name ) const;

	idImageScratch *	ImageScratch( const char *name );

	// returns the number of bytes of image data bound in the previous frame
	int					SumOfUsedImages( int *numberOfUsed = nullptr );

	// called each frame to allow some cvars to automatically force changes
	void				CheckCvars();

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadAllImages();

	// add CPU residency bit and make sure cpuData is valid
	void				EnsureImageCpuResident( idImageAsset *image );

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

	// execute image-related function in the nearest future when modifications are thread-safe
	// it is usually deferred until the next in-between frames moment, when only one thread is active
	void				ExecuteWhenSingleThreaded( std::function<void(void)> callback );

	// Called regularly from backend thread when frontend thread is idle.
	// This allows to do execute delayed functions, which could otherwise cause race condition
	void				UpdateSingleThreaded();


	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild( bool removeDups = false );
	void				AddDDSCommand( const char *cmd );

	void				PrintMemInfo( MemInfo_t *mi );

	// cvars
	static idCVar		image_colorMipLevels;		// development aid to see texture mip usage
	static idCVar		image_downSize;				// controls texture downsampling
	static idCVar		image_downSizeAll;			// force all image types (diffuse, bump, spec) to use the same downsize value
	static idCVar		image_useCompression;		// 0 = force everything to high quality
	static idCVar		image_filter;				// changes texture filtering on mipmapped images
	static idCVar		image_anisotropy;			// set the maximum texture anisotropy if available
	static idCVar		image_lodbias;				// change lod bias on mipmapped images
	static idCVar		image_usePrecompressedTextures;	// use .dds files if present
	static idCVar		image_forceRecompress;		// stgatilov #6300: uncompress all read DDS, so that they are recompressed by our code
	static idCVar		image_writePrecompressedTextures; // write .dds files if necessary
	static idCVar		image_writeNormalTGA;		// debug tool to write out .tgas of the final normal maps
	static idCVar		image_writeTGA;				// debug tool to write out .tgas of the non normal maps
	static idCVar		image_useNormalCompression;	// use RGTC2 compression
	static idCVar		image_useOffLineCompression; // will write a batch file with commands for the offline compression
	static idCVar		image_preload;				// if 0, dynamically load all images
	static idCVar		image_forceDownSize;		// allows the ability to force a downsize
	static idCVar		image_downSizeSpecular;		// downsize specular
	static idCVar		image_downSizeSpecularLimit;// downsize specular limit
	static idCVar		image_downSizeBump;			// downsize bump maps
	static idCVar		image_downSizeBumpLimit;	// downsize bump limit
	static idCVar		image_ignoreHighQuality;	// ignore high quality on materials
	static idCVar		image_downSizeLimit;		// downsize diffuse limit

	// built-in readable images
	idImageAsset *		defaultImage;
	idImageAsset *		flatNormalMap;				// 128 128 255 in all pixels
	idImageAsset *		ambientNormalMap;			// tr.ambientLightVector encoded in all pixels
	idImageAsset *		alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	idImageAsset *		whiteImage;					// full of 0xff
	idImageAsset *		blackImage;					// full of 0x00
	idImageAsset *		whiteCubeMapImage;			// full of 0xff
	idImageAsset *		blackCubeMapImage;			// full of 0x00
	idImageAsset *		normalCubeMapImage;			// cube map to normalize STR into RGB
	idImageAsset *		noFalloffImage;				// all 255, but zero clamped
	idImageAsset *		fogImage;					// increasing alpha is denser fog
	idImageAsset *		fogEnterImage;				// adjust fogImage alpha based on terminator plane
	idImageAsset *		blueNoise1024rgbaImage;		// blue noise precomputed image for dithering
	// built-in stream-written textures
	idImageScratch *	cinematicImage;
	idImageScratch *	scratchImage;
	idImageScratch *	currentRenderImage;			// for SS_POST_PROCESS shaders
	idImageScratch *	guiRenderImage;
	idImageScratch *	currentDepthImage;			// #3877. Allow shaders to access scene depth
	idImageScratch *	shadowDepthFbo;
	idImageScratch *	shadowAtlas;
	idImageScratch *	currentStencilFbo; // these two are only used on Intel since no one else support separate stencil

	//--------------------------------------------------------

	idImageAsset *		AllocImageAsset( const char *name );
	idImageScratch *	AllocImageScratch( const char *name );
	void				SetNormalPalette();
	void				ChangeTextureFilter();

	idList<idImage*>	images;
	idStrList			ddsList;
	idHashIndex			ddsHash;

	bool				insideLevelLoad;			// don't actually load images now

	byte				originalToCompressed[256];	// maps normal maps to 8 bit textures
	byte				compressedPalette[768];		// the palette that normal maps use

	// default filter modes for images
	GLenum				textureMinFilter;
	GLenum				textureMaxFilter;
	float				textureAnisotropy;
	float				textureLODBias;

	idImage *			imageHashTable[FILE_HASH_SIZE];

	// ExecuteWhenSingleThreaded adds callbacks to this queue in thread-safe way
	idSysMutex			delayedFunctionsMutex;
	idList<std::function<void(void)>>	delayedFunctionsQueue;
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
byte *R_MipMap( const byte *in, int width, int height );

// these operate in-place on the provided pixels
void R_SetBorderTexels( byte *inBase, int width, int height, const byte border[4] );
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4] );
void R_HorizontalFlip( byte *data, int width, int height );
void R_VerticalFlip( byte *data, int width, int height );
void R_RotatePic( byte *data, int width );

/*
====================================================================

IMAGEFILES

====================================================================
*/

extern const char *cubemapFaceNamesCamera[6];
extern const char *cubemapFaceNamesNative[6];

void R_LoadImage( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );
void R_LoadCompressedImage( const char *name, imageCompressedData_t **pic, ID_TIME_T *timestamp );
// pic is in top to bottom raster format
bool R_LoadCubeImages( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, ID_TIME_T *timestamp );
void R_BakeAmbient( byte *pics[6], int *size, float multiplier, bool specular, const char *name );
void R_LoadImageData( idImageAsset &image );
void R_UploadImageData( idImageAsset& image );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp, textureDepth_t *depth = NULL );
const char *R_ParsePastImageProgram( idLexer &src );
void R_LoadImageProgramCubeMap( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, ID_TIME_T *timestamps );
const char *R_ParsePastImageProgramCubeMap( idLexer &src );

/*
====================================================================

IMAGE READER/WRITER

====================================================================
*/

// data is RGBA
void R_WriteTGA( const char* filename, const byte* data, int width, int height, bool flipVertical = false );

class idImageWriter {
public:
	//setting common settings
	inline idImageWriter &Source(const byte *data, int width, int height, int bpp = 4) {
		srcData = data;
		srcWidth = width;
		srcHeight = height;
		srcBpp = bpp;
		return *this;
	}
	inline idImageWriter &Dest(idFile *file, bool close = true) {
		dstFile = file;
		dstClose = close;
		return *this;
	}
	inline idImageWriter &Flip(bool doFlip = true) {
		flip = doFlip;
		return *this;
	}
	//perform save (final call)
	bool WriteTGA();
	bool WriteJPG(int quality = 85);
	bool WritePNG(int level = -1);
	bool WriteExtension(const char *extension);

private:
	bool Preamble();
	void Postamble();

	const byte *srcData = nullptr;
	int srcWidth = -1, srcHeight = -1, srcBpp = -1;
	idFile *dstFile = nullptr;
	bool dstClose = true;
	bool flip = false;
};

class idImageReader {
public:
	//setting common settings
	inline idImageReader &Source(idFile *file, bool close = true) {
		srcFile = file;
		srcClose = close;
		return *this;
	}
	inline idImageReader &Dest(byte* &data, int &width, int &height) {
		dstData = &data;
		dstWidth = &width;
		dstHeight = &height;
		return *this;
	}
	//perform load (final call)
	void LoadTGA();
	void LoadJPG();
	void LoadPNG();
	void LoadExtension(const char *extension = nullptr);

private:
	bool Preamble();
	void Postamble();
	void LoadBMP();

	idFile *srcFile = nullptr;
	bool srcClose = true;
	byte* *dstData = nullptr;
	int *dstWidth = nullptr, *dstHeight = nullptr;
	byte *srcBuffer = nullptr;
	int srcLength = 0;
};

#endif /* !__R_IMAGE_H__ */
