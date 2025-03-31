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

#ifndef __RENDERER_H__
#define __RENDERER_H__

/*
===============================================================================

	idRenderSystem is responsible for managing the screen, which can have
	multiple idRenderWorld and 2D drawing done on it.

===============================================================================
*/

enum glVendor_t {
	glvAny,
	glvAMD,
	glvIntel,
	glvNVIDIA,
};

// Contains variables specific to the OpenGL configuration being run right now.
// These are constant once the OpenGL subsystem is initialized.
typedef struct glconfig_s {
	bool				isInitialized;

	const char			*renderer_string;
	const char			*vendor_string;
	const char			*version_string;
	const char			*wgl_extensions_string;
	glVendor_t			vendor;

	// OpenGL initialization settings
	int					vidWidth, vidHeight;	// passed to R_BeginFrame
	int					displayFrequency;
	bool				isFullscreen;
	bool				srgb;

	//GL extensions which can potentially be used in-game
	bool				anisotropicAvailable;
	bool				depthBoundsTestAvailable;
	bool				geometryShaderAvailable;
	bool				bufferStorageAvailable; // persistent mapping
	bool				stencilTexturing;		// stencil SS

	// values of various GL limits
	int					maxTextureSize;
	int					maxTextures;
	int					maxTextureUnits;
	float				maxTextureAnisotropy;
	int					maxSamples;
} glconfig_t;

int GetConsoleFontSize();
#define SMALLCHAR_WIDTH		(GetConsoleFontSize())
#define SMALLCHAR_HEIGHT	(2 * GetConsoleFontSize())
#define BIGCHAR_WIDTH		16
#define BIGCHAR_HEIGHT		16

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		480

// font support 
#define GLYPH_START			0
#define GLYPH_END			255
#define GLYPH_CHARSTART		32
#define GLYPH_CHAREND		127
#define GLYPHS_PER_FONT		(GLYPH_END - GLYPH_START + 1)

// stgatilov #6283: now it is possible to customize existing fonts
// to do it, add suffix like this to font name:    @param1@param2@param3=XX
struct fontParameters_t {
	idVec2 scale = idVec2(1.0f, 1.0f);
};

typedef struct {
	float				height;			// number of scan lines
	float				top;			// top of glyph in buffer
	float				bottom;			// bottom of glyph in buffer
	float				pitch;			// width for copying
	float				xSkip;			// x adjustment
	float				imageWidth;		// width of actual image
	float				imageHeight;	// height of actual image
	float				s;				// x offset in image where glyph starts
	float				t;				// y offset in image where glyph starts
	float				s2;
	float				t2;
	const idMaterial *	glyph;			// shader with the glyph
	char				shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t			glyphs [GLYPHS_PER_FONT];
	float				glyphScale;
	char				name[64];
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
} fontInfoEx_t;

class idRenderWorld;


class idRenderSystem {
public:

	virtual					~idRenderSystem() {}

	// set up cvars and basic data structures, but don't
	// init OpenGL, so it can also be used for dedicated servers
	virtual void			Init( void ) = 0;

	// only called before quitting
	virtual void			Shutdown( void ) = 0;

	virtual void			InitOpenGL( void ) = 0;
	virtual void			ShutdownOpenGL( void ) = 0;
	virtual bool			IsOpenGLRunning( void ) const = 0;

	virtual bool			IsFullScreen( void ) const = 0;
	virtual int				GetScreenWidth( void ) const = 0;
	virtual int				GetScreenHeight( void ) const = 0;

	// allocate a renderWorld to be used for drawing
	virtual idRenderWorld *	AllocRenderWorld( void ) = 0;
	virtual	void			FreeRenderWorld( idRenderWorld * rw ) = 0;

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	virtual void			BeginLevelLoad( void ) = 0;
	virtual void			EndLevelLoad( void ) = 0;

	// font support
	virtual bool			RegisterFont( const char *fontName, const fontParameters_t &params, fontInfoEx_t &font ) = 0;

	// GUI drawing just involves shader parameter setting and axial image subsections
	virtual void			SetColor( const idVec4 &rgba ) = 0;
	virtual void			SetColor4( float r, float g, float b, float a ) = 0;

	virtual void			DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
											bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f ) = 0;
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) = 0;

	virtual void			DrawStretchTri ( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material ) = 0;
	virtual void			GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) = 0;
	virtual void			GetGLSettings( int& width, int& height ) = 0;
	virtual void			PrintMemInfo( MemInfo_t *mi ) = 0;

	virtual void			DrawSmallChar( int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;
	virtual void			DrawBigChar( int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;

	// dump all 2D drawing so far this frame to the demo file
	virtual void			WriteDemoPics() = 0;

	// draw the 2D pics that were saved out with the current demo frame
	virtual void			DrawDemoPics() = 0;

	// a frame cam consist of 2D drawing and potentially multiple 3D scenes
	// window sizes are needed to convert SCREEN_WIDTH / SCREEN_HEIGHT values
	virtual void			BeginFrame( int windowWidth, int windowHeight ) = 0;

	// if the pointers are not NULL, timing info will be returned
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec ) = 0;

	// aviDemo uses this.
	// Will automatically tile render large screen shots if necessary
	// Samples is the number of jittered frames for anti-aliasing
	// If ref == NULL, session->updateScreen will be used
    // envshot check added since we do not want to use the usual screenshot
    // renaming for the envshot function
	// This will perform swapbuffers, so it is NOT an approppriate way to
	// generate image files that happen during gameplay, as for savegame
	// markers.  Use WriteRender() instead.
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int samples, struct renderView_s *ref, bool envshot = false ) = 0;

	// the render output can be cropped down to a subset of the real screen, as
	// for save-game reviews and split-screen multiplayer.  Users of the renderer
	// will not know the actual pixel size of the area they are rendering to

	// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

	// to render to a texture, first set the crop size with makePowerOfTwo = true,
	// then perform all desired rendering, then capture to an image
	// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false ) = 0;

	/**
	 * greebo: Get the size of the current rendercrop - unless forceDimensions == true has been passed to CropRenderSize()
	 * the actual dimensions of the rendercrop might differ, so use these to query them, e.g. to allocate a buffer 
	 * for use with CaptureRenderToBuffer().
	 */
	virtual void			GetCurrentRenderCropSize(int& width, int& height) = 0;

	virtual void			CaptureRenderToImage( idImageScratch &image ) = 0;
	
	virtual void			PostProcess() = 0;

	/**
	 * greebo: Like CaptureRenderToFile, but writes the result into the given byte buffer.
	 * The buffer is managed by the calling code and needs to provide space for the current rendercrop's 
	 * size using 3 bytes per pixel (stored in order RGB). Use CropRenderSize(), then GetCurrentRenderCropSize() 
	 * to receive the necessary size.
	 * stgatilov: usePbo = true is set only for lightgem capturing! (see #4395)
	 */
	virtual void			CaptureRenderToBuffer(unsigned char* buffer, bool usePbo = false) = 0;

	virtual void			UnCrop() = 0;
};

extern idRenderSystem *			renderSystem;

//
// functions mainly intended for editor and dmap integration
//

// returns the frustum planes in world space
void R_RenderLightFrustum( const struct renderLight_s &renderLight, idPlane lightFrustum[6] );

// for use by dmap to do the carving-on-light-boundaries and for the editor for display
void R_LightProjectionMatrix( const idVec3 &origin, const idPlane &rearPlane, idVec4 mat[4] );

// used by the view shot taker
void R_ScreenshotFilename( int &lastNumber, const char *base, idStr &fileName );

#endif /* !__RENDERER_H__ */
