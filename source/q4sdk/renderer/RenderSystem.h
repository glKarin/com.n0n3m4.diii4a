// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __RENDERER_H__
#define __RENDERER_H__

/*
===============================================================================

	idRenderSystem is responsible for managing the screen, which can have
	multiple idRenderWorld and 2D drawing done on it.

===============================================================================
*/


// Contains variables specific to the OpenGL configuration being run right now.
// These are constant once the OpenGL subsystem is initialized.
typedef struct glconfig_s {
	const char			*renderer_string;
	const char			*vendor_string;
	const char			*version_string;
	const char			*extensions_string;
	const char			*wgl_extensions_string;

	float				glVersion;				// atof( version_string )


	int					maxTextureSize;			// queried from GL
	int					maxTextureUnits;
	int					maxTextureCoords;
	int					maxTextureImageUnits;
	float				maxTextureAnisotropy;

	int					colorBits, depthBits, stencilBits;
// RAVEN BEGIN
	int					alphaBits;
// RAVEN END

	bool				multitextureAvailable;
	bool				anisotropicAvailable;
	bool				textureLODBiasAvailable;
	bool				textureEnvAddAvailable;
	bool				textureEnvCombineAvailable;
// RAVEN BEGIN
// jscott: added
	bool				blendSquareAvailable;
// RAVEN END
	bool				registerCombinersAvailable;
	bool				cubeMapAvailable;
	bool				envDot3Available;
	bool				texture3DAvailable;
	bool				sharedTexturePaletteAvailable;
// RAVEN BEGIN
// dluetscher: added
	bool				drawRangeElementsAvailable;
	bool				blendMinMaxAvailable;
	bool				floatBufferAvailable;
// RAVEN END
	bool				ARBVertexBufferObjectAvailable;
	bool				ARBVertexProgramAvailable;
	bool				ARBFragmentProgramAvailable;
	bool				twoSidedStencilAvailable;
	bool				textureNonPowerOfTwoAvailable;
	bool				depthBoundsTestAvailable;

// RAVEN BEGIN
// rjohnson: new shader stage system
	bool				GLSLProgramAvailable;
// RAVEN END

// RAVEN BEGIN
// dluetscher: added check for NV_vertex_program and NV_fragment_program support
	bool				nvProgramsAvailable;
// RAVEN END

	// ati r200 extensions
	bool				atiFragmentShaderAvailable;

	// ati r300
	bool				atiTwoSidedStencilAvailable;

	int					vidWidth, vidHeight;	// passed to R_BeginFrame

	int					displayFrequency;

	bool				isFullscreen;

#ifndef _CONSOLE
	bool				preferNV20Path;			// for FX5200 cards
// RAVEN BEGIN
// dluetscher: added preferSimpleLighting flag
	bool				preferSimpleLighting;	// for the ATI 9700 cards
// RAVEN END
#endif

	bool				allowNV20Path;
	bool				allowNV10Path;
	bool				allowR200Path;
	bool				allowARB2Path;

	bool				isInitialized;
// INTEL BEGIN 
// Anu adding support to toggle SMP
#ifdef ENABLE_INTEL_SMP
	bool				isSmpAvailable;
	int					isSmpActive;
	int					rearmSmp;
#endif
// INTEL END

} glconfig_t;


// font support 
#define GLYPH_COUNT			256

typedef struct glyphInfo_s
{
	float			width;					// number of pixels wide
	float			height;					// number of scan lines
	float			horiAdvance;			// number of pixels to advance to the next char
	float			horiBearingX;			// x offset into space to render glyph
	float			horiBearingY;			// y offset 
	float			s1;						// x start tex coord
	float			t1;						// y start tex coord
	float			s2;						// x end tex coord
	float			t2;						// y end tex coord
} glyphInfo_t;

typedef struct fontInfo_s
{
	glyphInfo_t		glyphs[GLYPH_COUNT];

	float			pointSize;
	float			fontHeight;				// max height of font
	float			ascender;
	float			descender;

	idMaterial		*material;
} fontInfo_t;

typedef struct {
	fontInfo_t			fontInfoSmall;
	fontInfo_t			fontInfoMedium;
	fontInfo_t			fontInfoLarge;
	float				maxHeight;
	float				maxWidth;
	float				maxHeightSmall;
	float				maxWidthSmall;
	float				maxHeightMedium;
	float				maxWidthMedium;
	float				maxHeightLarge;
	float				maxWidthLarge;
	char				name[64];
// RAVEN BEGIN
// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	idList<idMaterial*> allMaterials;
#endif
// RAVEN END
} fontInfoEx_t;

const int TINYCHAR_WIDTH		= 4;
const int TINYCHAR_HEIGHT		= 8;
const int SMALLCHAR_WIDTH		= 8;
const int SMALLCHAR_HEIGHT		= 16;
const int BIGCHAR_WIDTH			= 16;
const int BIGCHAR_HEIGHT		= 16;

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
const int SCREEN_WIDTH			= 640;
const int SCREEN_HEIGHT			= 480;

// RAVEN BEGIN
// rjohnson: new blur special effect
typedef enum
{
	SPECIAL_EFFECT_NONE = 0,
	SPECIAL_EFFECT_BLUR		= 0x00000001,
	SPECIAL_EFFECT_AL		= 0x00000002,
	SPECIAL_EFFECT_MAX,
} ESpecialEffectType;
// RAVEN END

class idRenderWorld;


class idRenderSystem {
public:

	virtual					~idRenderSystem() {}

	// set up cvars and basic data structures, but don't
	// init OpenGL, so it can also be used for dedicated servers
// RAVEN BEGIN
	// nrausch: Init things that touch the render state seperately
	virtual void			DeferredInit( void ) { }
// RAVEN END
	virtual void			Init( void ) = 0;

	// only called before quitting
	virtual void			Shutdown( void ) = 0;

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void			FlushLevelImages( void ) = 0;
#endif
// RAVEN END

	virtual void			InitOpenGL( void ) = 0;

	virtual void			ShutdownOpenGL( void ) = 0;

	virtual bool			IsOpenGLRunning( void ) const = 0;
	virtual void			GetValidModes( idStr &Mode4x3Text, idStr &Mode4x3Values, idStr &Mode16x9Text, idStr &Mode16x9Values, 
										   idStr &Mode16x10Text, idStr &Mode16x10Values ) = 0;

	virtual bool			IsFullScreen( void ) const = 0;
	virtual int				GetScreenWidth( void ) const = 0;
	virtual int				GetScreenHeight( void ) const = 0;

	// allocate a renderWorld to be used for drawing
	virtual idRenderWorld *	AllocRenderWorld( void ) = 0;
	virtual	void			FreeRenderWorld( idRenderWorld * rw ) = 0;

// RAVEN BEGIN
	virtual void			RemoveAllModelReferences( idRenderModel *model ) = 0;
// RAVEN BEGIN

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	virtual void			BeginLevelLoad( void ) = 0;
	virtual void			EndLevelLoad( void ) = 0;

// RAVEN BEGIN
// mwhitlock: changes for Xenon to enable us to use texture resources from .xpr
// bundles.
#if defined(_XENON)
	// Like BeginLevelLoad and EndLevelLoad, but only deals with textures.
	virtual void			XenonBeginLevelLoadTextures( void ) = 0;
	virtual	void			XenonEndLevelLoadTextures( void ) = 0;
#endif
// RAVEN END

#ifdef Q4SDK_MD5R

	virtual	void			ExportMD5R( bool compressed ) = 0;
	virtual void			CopyPrimBatchTriangles( idDrawVert *destDrawVerts, glIndex_t *destIndices, void *primBatchMesh, void *silTraceVerts ) = 0;

#else	// Q4SDK_MD5R

// RAVEN BEGIN
// dluetscher: added call to write out the MD5R models that have been converted at load,
//			   also added call to retrieve idDrawVert geometry from a MD5R primitive batch 
//			   from the game DLL
#if defined( _MD5R_WRITE_SUPPORT ) && defined( _MD5R_SUPPORT )
	virtual	void			ExportMD5R( bool compressed ) = 0;
#endif
#if defined( _MD5R_SUPPORT )
	virtual void			CopyPrimBatchTriangles( idDrawVert *destDrawVerts, glIndex_t *destIndices, rvMesh *primBatchMesh, const rvSilTraceVertT *silTraceVerts ) = 0;
#endif

#endif // !Q4SDK_MD5R

// jnewquist: Track texture usage during cinematics for streaming purposes
#ifndef _CONSOLE
	enum TextureTrackCommand {
		TEXTURE_TRACK_BEGIN,
		TEXTURE_TRACK_UPDATE,
		TEXTURE_TRACK_END
	};
	virtual void			TrackTextureUsage( TextureTrackCommand command, int frametime = 0, const char *name=NULL ) = 0;
#endif
// RAVEN END

	// font support
	virtual bool			RegisterFont( const char *fontName, fontInfoEx_t &font ) = 0;
	// GUI drawing just involves shader parameter setting and axial image subsections
	virtual void			SetColor( const idVec4 &rgba ) = 0;
	virtual void			SetColor4( float r, float g, float b, float a ) = 0;

	virtual void			DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material, bool clip = true ) = 0;
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) = 0;
// RAVEN BEGIN
// jnewquist: Deal with flipped back-buffer copies on Xenon
	virtual void			DrawStretchCopy( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) = 0;
// RAVEN END

	virtual void			DrawStretchTri ( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material ) = 0;
	virtual void			GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) = 0;
	virtual void			GetGLSettings( int& width, int& height ) = 0;
	virtual void			PrintMemInfo( MemInfo *mi ) = 0;

	virtual void			DrawTinyChar( int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawTinyStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;
	virtual void			DrawSmallChar( int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;
	virtual void			DrawBigChar( int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;

	// dump all 2D drawing so far this frame to the demo file
	virtual void			WriteDemoPics() = 0;

	// draw the 2D pics that were saved out with the current demo frame
	virtual void			DrawDemoPics() = 0;

	// FIXME: add an interface for arbitrary point/texcoord drawing


	// a frame cam consist of 2D drawing and potentially multiple 3D scenes
	// window sizes are needed to convert SCREEN_WIDTH / SCREEN_HEIGHT values
	virtual void			BeginFrame( int windowWidth, int windowHeight ) = 0;
// RAVEN BEGIN
	virtual void			BeginFrame( struct viewDef_s *viewDef, int windowWidth, int windowHeight ) = 0;
	virtual	void			RenderLightFrustum( const struct renderLight_s &renderLight, idPlane lightFrustum[6] ) = 0;
	virtual	void			LightProjectionMatrix( const idVec3 &origin, const idPlane &rearPlane, idVec4 mat[4] ) = 0;
	virtual void			ToggleSmpFrame( void ) = 0;
// RAVEN END

// RAVEN BEGIN
// rjohnson: new blur special effect
	virtual void			SetSpecialEffect( ESpecialEffectType Which, bool Enabled ) = 0;
	virtual void			SetSpecialEffectParm( ESpecialEffectType Which, int Parm, float Value ) = 0;
	virtual void			ShutdownSpecialEffects( void ) = 0;
// RAVEN END

	// if the pointers are not NULL, timing info will be returned
	virtual void			EndFrame( int *frontEndMsec = NULL, int *backEndMsec = NULL, int *numVerts = NULL, int *numIndexes = NULL ) = 0;

	// aviDemo uses this.
	// Will automatically tile render large screen shots if necessary
	// Samples is the number of jittered frames for anti-aliasing
	// If ref == NULL, session->updateScreen will be used
	// This will perform swapbuffers, so it is NOT an approppriate way to
	// generate image files that happen during gameplay, as for savegame
	// markers.  Use WriteRender() instead.
// RAVEN BEGIN
// rjohnson: added basePath
	virtual	void			TakeJPGScreenshot( int width, int height, const char *fileName, int blends, struct renderView_s *ref, const char *basePath = "fs_savepath" ) = 0;
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int blends, struct renderView_s *ref, const char *basePath = "fs_savepath" ) = 0;
// RAVEN END

	// the render output can be cropped down to a subset of the real screen, as
	// for save-game reviews and split-screen multiplayer.  Users of the renderer
	// will not know the actual pixel size of the area they are rendering to

	// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

	// to render to a texture, first set the crop size with makePowerOfTwo = true,
	// then perform all desired rendering, then capture to an image
	// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false ) = 0;
	virtual void			CaptureRenderToImage( const char *imageName ) = 0;
	virtual void			CaptureRenderToMemory( void *buffer ) = 0;
	// fixAlpha will set all the alpha channel values to 0xff, which allows screen captures
	// to use the default tga loading code without having dimmed down areas in many places
	virtual void			CaptureRenderToFile( const char *fileName, bool fixAlpha = false ) = 0;
	virtual void			UnCrop() = 0;
	virtual void			GetCardCaps( bool &oldCard, bool &nv10or20 ) = 0;
	virtual bool			UploadImage( const char *imageName, const byte *data, int width, int height ) = 0;

	virtual void			DebugGraph( float cur, float min, float max, const idVec4 &color ) = 0;
	virtual void			ShowDebugGraph( void ) = 0;
};

extern idRenderSystem *		renderSystem;

#endif /* !__RENDERER_H__ */
