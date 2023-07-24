// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __RENDERER_H__
#define __RENDERER_H__

/*
===============================================================================

	idRenderSystem is responsible for managing the screen, which can have
	multiple idRenderWorld and 2D drawing done on it.

===============================================================================
*/

class idMaterial;
class idImage;
class idVec4;
class idDrawVert;
class idPlane;
class idVec3;
class idStr;

#include "../libs/qglLib/qgl.h"

#include "Model_public.h"
#include "RendererEnums.h"

// Contains variables specific to the OpenGL configuration being run right now.
// These are constant once the OpenGL subsystem is initialized.
struct glconfig_t {
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
	int					maxVertexAttribs;
	int					maxProgramLocalParameters;
	int					maxProgramEnvParameters;

	int					colorBits, depthBits, stencilBits;
	int					samples; // 0 ARB_multisample is not available

	bool				multitextureAvailable;
	bool				textureCompressionAvailable;
	bool				anisotropicAvailable;
	bool				textureLODBiasAvailable;
	bool				cubeMapAvailable;
	bool				texture3DAvailable;
	bool				rectangleTextureAvailable;
	bool				sharedTexturePaletteAvailable;
	bool				ARBVertexBufferObjectAvailable;
	bool				ARBVertexProgramAvailable;
	bool				ARBFragmentProgramAvailable;
	bool				twoSidedStencilAvailable;
	bool				textureNonPowerOfTwoAvailable;
	bool				depthBoundsTestAvailable;
	bool				pointSpriteAvailable;
	bool				occlusionQueryAvailable;
	bool				framebufferObjectAvailable;
	bool				EXTPackedDepthStencilAvailable;
	bool				blendEquationAvailable;
	bool				shadowMappingHardwareAvailable; // We have all extensions needed to do percentage closer filtering with R-compare in the texture samplers
	bool				multiSampleAvailable;
	bool				csaaAvailable;
	bool				ARBShaderObjectsAvailable;
	bool				ARBVertexShaderAvailable;
	bool				ARBFragmentShaderAvailable;
	bool				EXTGpuProgramParametersAvailable;

	// ati r300
	bool				atiTwoSidedStencilAvailable;

	bool				nvFloatBufferAvailable;
	bool				atiPixelFormatFloatAvailable;
	bool				ARBPixelFormatFloatAvailable;
	bool				timerQueryAvailable;
	bool				stringMarkerAvailable;

	// ATI
	bool				textureCompression3DCAvailable;

	// NVIDIA G80
	bool				textureCompressionLATCAvailable;

	bool				backendInitialized;
	bool				allowCgPath;

	bool				isInitialized;
};

const int SMALLCHAR_WIDTH		= 8;
const int SMALLCHAR_HEIGHT		= 16;
const int BIGCHAR_WIDTH			= 16;
const int BIGCHAR_HEIGHT		= 16;

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
const int SCREEN_WIDTH		= 640;
const int SCREEN_HEIGHT		= 480;

class idRenderWorld;
class sdFrameBuffer;

class idRenderSystem {
public:

	virtual					~idRenderSystem() {}

	// set up cvars and basic data structures, but don't
	// init OpenGL, so it can also be used for dedicated servers
	virtual void			Init( void ) = 0;

	// only called before quitting
	virtual void			Shutdown( void ) = 0;

	virtual void			ShutdownOpenGL( void ) = 0;

	virtual bool			IsOpenGLRunning( void ) const = 0;

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
	virtual void			LevelStart( void ) = 0;

	virtual void			DrawChar( int charWidth, int charHeight, int x, int y, int ch, const idMaterial *material ) = 0;
	virtual void			DrawStringExt( int charWidth, int charHeight, int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material ) = 0;
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

	virtual void			EndFrame( bool swapBuffers = true ) = 0;

	virtual void			SetCaptureBuffer( sdFrameBuffer* frameBuffer ) = 0;
	virtual sdFrameBuffer*	GetCaptureBuffer() = 0;

	// aviDemo uses this.
	// Will automatically tile render large screen shots if necessary
	// Samples is the number of jittered frames for anti-aliasing
	// If ref == NULL, session->updateScreen will be used
	// This will perform swapbuffers, so it is NOT an approppriate way to
	// generate image files that happen during gameplay, as for savegame
	// markers.  Use WriteRender() instead.
	virtual bool			TakeScreenshot( int width, int height, const char *fileName, int samples, struct renderView_s *ref, bool useOffscreenContext = false, bool flip = false ) = 0;

	// the render output can be cropped down to a subset of the real screen, as
	// for save-game reviews and split-screen multiplayer.  Users of the renderer
	// will not know the actual pixel size of the area they are rendering to

	// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

	// to render to a texture, first set the crop size with makePowerOfTwo = true,
	// then perform all desired rendering, then capture to an image
	// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false ) = 0;
	virtual void			CaptureRenderToImage( const char *imageName, int faceNum = -1, copyBuffer_t buffer = CB_COLOR ) = 0;
	virtual void			SetFrameBuffer( class sdFrameBuffer *frameBuffer ) = 0;
	virtual void			UnCrop() = 0;

	virtual void			GetCardCaps( bool &oldCard ) = 0;

	// the image has to be already loaded ( most straightforward way would be through a FindMaterial )
	// texture filter / mipmapping / repeat won't be modified by the upload
	// returns false if the image wasn't found
	virtual bool			UploadImage( const char* imageName, const byte* data, int width, int height, bool generateMipMaps = false, bool copy = true ) = 0;

	virtual	void			BindImage( textureType_t target, GLuint image ) = 0;
	virtual void			SetGLState( int stateVector ) = 0;
	virtual void			SetGLTexEnv( int env ) = 0;
	virtual	void			SelectTextureUnit( int unit ) = 0;
	virtual void			SetDefaultGLState( void ) = 0;
	virtual void			SetGL2D( void ) = 0;
	virtual void			SetCull( int cullType ) = 0;

	virtual FILE*			GetLogFileHandle() = 0;
	virtual void			SetLogFileHandle( FILE* file ) = 0;

	virtual void						LoadImage( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, bool makePowerOf2 ) = 0;

	virtual void						FlushGLErrors( bool forcePrint = false ) = 0;
	virtual int							CheckGLForErrors( bool forcePrint = false ) = 0;

	virtual class idRenderModel*		InstantiateDynamicModel( class idRenderModel* model, struct renderEntity_t* ent ) = 0;

	virtual const glconfig_t&			GLConfig() const = 0;

	virtual void			SyncRenderSystem( void )  = 0;

	virtual bool			BeginRenderSync( void ) = 0;
	virtual void			EndRenderSync( void ) = 0;

	virtual idImage			*LoadImageFromFile( const char *filename, struct imageParams_t &ip ) = 0;

	virtual bool			IsDisplayModeAvailable( int width, int height ) const = 0;

	virtual int				GetNumMSAAModes( void ) const = 0;
	virtual const char *	GetMSAAMode( int idx, int &val ) const = 0;
	virtual bool			IsMSAACountAvailable( int msaa ) const = 0;

	virtual void			LockThreads( void ) = 0;
	virtual void			UnlockThreads( void ) = 0;
	virtual int				GetDoubleBufferIndex( void ) = 0;
	virtual int				GetSyncNum( void ) = 0;
	virtual bool			IsSMPEnabled( void ) = 0;

	virtual void			FreeOcclussionQueries( void ) = 0;

	virtual int				RegisterPtr( void *ptr ) = 0;
	virtual void			UnregisterPtr( int uid ) = 0;
	virtual void*			PtrForUID( int uid ) = 0;

};

extern idRenderSystem* renderSystem;

//
// functions mainly intended for editor and dmap integration
//

// returns the frustum planes in world space
void R_RenderLightFrustum( const struct renderLight_t &renderLight, idPlane lightFrustum[6] );

// used by the view shot taker
void R_ScreenshotFilename( int &lastNumber, const char *base, idStr &fileName );

class renderSystemSync {

	bool synced;
public:
	renderSystemSync( ) : synced(true) {
		synced = renderSystem->BeginRenderSync();
	}

	~renderSystemSync() {
		if ( !synced ) {
			renderSystem->EndRenderSync();
		}
	}
};


#endif /* !__RENDERER_H__ */
