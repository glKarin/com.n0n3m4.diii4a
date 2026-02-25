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
    const char			*shading_language_version_string;

	float				glVersion;				// atof( version_string )


	int					maxTextureSize;			// queried from GL
	int					maxTextureUnits;
	int					maxTextureCoords;
	int					maxTextureImageUnits;
	float				maxTextureAnisotropy;

	int					colorBits, depthBits, stencilBits;

	bool				multitextureAvailable;
	bool				textureCompressionAvailable;
	bool				anisotropicAvailable;
	bool				textureLODBiasAvailable;
	bool				textureEnvAddAvailable;
	bool				textureEnvCombineAvailable;
	bool				cubeMapAvailable;
	bool				envDot3Available;
	bool				texture3DAvailable;
	bool				sharedTexturePaletteAvailable;
	bool				ARBVertexBufferObjectAvailable;
	bool				ARBVertexProgramAvailable;
	bool				ARBFragmentProgramAvailable;
	bool				textureNonPowerOfTwoAvailable;
	bool				depthBoundsTestAvailable;
	bool				GLSLAvailable;

	int					vidWidth, vidHeight;	// passed to R_BeginFrame

	int					displayFrequency;

	bool				isFullscreen;

	bool				allowARB2Path;
	bool				allowGLSLPath;

	bool				isInitialized;

	bool                framebufferObjectAvailable;
	int                 maxRenderbufferSize;
    int                 maxColorAttachments;

	bool                depthTextureAvailable;
	bool                depthTextureCubeMapAvailable;
	bool                depth24Available;
	bool                gl_FragDepthAvailable;
	int                 multiSamples;
	bool				debugOutput;
    bool				syncAvailable;
#ifdef _QC
    int					winWidth, winHeight;
#endif
} glconfig_t;


// font support
const int GLYPH_START			= 0;
const int GLYPH_END				= 255;
const int GLYPH_CHARSTART		= 32;
const int GLYPH_CHAREND			= 127;
const int GLYPHS_PER_FONT		= GLYPH_END - GLYPH_START + 1;

typedef struct {
	int					height;			// number of scan lines
	int					top;			// top of glyph in buffer
	int					bottom;			// bottom of glyph in buffer
	int					pitch;			// width for copying
	int					xSkip;			// x adjustment
	int					imageWidth;		// width of actual image
	int					imageHeight;	// height of actual image
	float				s;				// x offset in image where glyph starts
	float				t;				// y offset in image where glyph starts
	float				s2;
	float				t2;
	const idMaterial 	*glyph;			// shader with the glyph
	char				shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t			glyphs [GLYPHS_PER_FONT];
	float				glyphScale;
	char				name[64];

#ifdef _WCHAR_LANG
    int                 numIndexes;
    int					*indexes;
    int                 numGlyphs;
    glyphInfo_t			*glyphsTable;
#endif
} fontInfo_t;

typedef struct {
	fontInfo_t			fontInfoSmall;
	fontInfo_t			fontInfoMedium;
	fontInfo_t			fontInfoLarge;
	int					maxHeight;
	int					maxWidth;
	int					maxHeightSmall;
	int					maxWidthSmall;
	int					maxHeightMedium;
	int					maxWidthMedium;
	int					maxHeightLarge;
	int					maxWidthLarge;
	char				name[64];
} fontInfoEx_t;

#ifdef _WCHAR_LANG
#define HARM_NEW_FONT_MAGIC ((unsigned)((unsigned)'i' << (unsigned)24 | (unsigned)'d' << (unsigned)16 | (unsigned)'t' << (unsigned)8 | (unsigned)'f'))
#define HARM_NEW_FONT_VERSION 0x00010001

const glyphInfo_t * R_Font_GetGlyphInfo(const fontInfo_t *info, uint32_t charIndex);
float R_Font_GetCharWidth(const fontInfo_t *info, uint32_t charCode, float scale = 1.0f);
float R_Font_GetCharHeight(const fontInfo_t *info, uint32_t charCode, float scale = 1.0f);

void R_Font_FreeFontInfo(fontInfo_t *info);
void R_Font_FreeFontInfoEx(fontInfoEx_t *ex);
#endif

const int SMALLCHAR_WIDTH		= 8;
const int SMALLCHAR_HEIGHT		= 16;
const int BIGCHAR_WIDTH			= 16;
const int BIGCHAR_HEIGHT		= 16;

// all drawing is done to a 640 x 480 virtual screen size
// and will be automatically scaled to the real resolution
const int SCREEN_WIDTH			= 640;
const int SCREEN_HEIGHT			= 480;

class idRenderWorld;

#ifdef _RAVEN
// RAVEN BEGIN
// rjohnson: new blur special effect
typedef enum
{
	SPECIAL_EFFECT_NONE = 0,
	SPECIAL_EFFECT_BLUR		= 0x00000001,
	SPECIAL_EFFECT_AL		= 0x00000002,
	SPECIAL_EFFECT_MAX,
} ESpecialEffectType;
#endif

class idRenderSystem
{
	public:

		virtual					~idRenderSystem() {}

		// set up cvars and basic data structures, but don't
		// init OpenGL, so it can also be used for dedicated servers
		virtual void			Init(void) = 0;

		// only called before quitting
		virtual void			Shutdown(void) = 0;

		virtual void			InitOpenGL(void) = 0;

		virtual void			ShutdownOpenGL(void) = 0;

		virtual bool			IsOpenGLRunning(void) const = 0;

		virtual bool			IsFullScreen(void) const = 0;
		virtual int				GetScreenWidth(void) const = 0;
		virtual int				GetScreenHeight(void) const = 0;

		// allocate a renderWorld to be used for drawing
		virtual idRenderWorld 	*AllocRenderWorld(void) = 0;
		virtual	void			FreeRenderWorld(idRenderWorld *rw) = 0;

		// All data that will be used in a level should be
		// registered before rendering any frames to prevent disk hits,
		// but they can still be registered at a later time
		// if necessary.
		virtual void			BeginLevelLoad(void) = 0;
		virtual void			EndLevelLoad(void) = 0;

		// font support
		virtual bool			RegisterFont(const char *fontName, fontInfoEx_t &font) = 0;

		// GUI drawing just involves shader parameter setting and axial image subsections
		virtual void			SetColor(const idVec4 &rgba) = 0;
		virtual void			SetColor4(float r, float g, float b, float a) = 0;

		virtual void			DrawStretchPic(const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
		                bool clip = true, float min_x = 0.0f, float min_y = 0.0f, float max_x = 640.0f, float max_y = 480.0f) = 0;
		virtual void			DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material) = 0;

		virtual void			DrawStretchTri(idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material) = 0;
		virtual void			GlobalToNormalizedDeviceCoordinates(const idVec3 &global, idVec3 &ndc) = 0;
		virtual void			GetGLSettings(int &width, int &height) = 0;
		virtual void			PrintMemInfo(MemInfo_t *mi) = 0;

		virtual void			DrawSmallChar(int x, int y, int ch, const idMaterial *material) = 0;
		virtual void			DrawSmallStringExt(int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material) = 0;
		virtual void			DrawBigChar(int x, int y, int ch, const idMaterial *material) = 0;
		virtual void			DrawBigStringExt(int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material) = 0;

#ifdef _HUMANHEAD
        virtual void			SetEntireSceneMaterial(idMaterial* material) = 0; // HUMANHEAD CJR
        virtual bool			IsScopeView() = 0;// HUMANHEAD CJR
        virtual void			SetScopeView(bool view) = 0; // HUMANHEAD CJR
        virtual bool			IsShuttleView() = 0;// HUMANHEAD pdm
        virtual void			SetShuttleView(bool view) = 0;// HUMANHEAD pdm
        virtual bool			SupportsFragmentPrograms(void) = 0; // HUMANHEAD CJR
        virtual int				VideoCardNumber(void) = 0; // HUMANHEAD CJR
												
#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	    virtual void			LogViewRender(const struct renderView_s *view) = 0;
#endif //HUMANHEAD END
#endif

		// dump all 2D drawing so far this frame to the demo file
		virtual void			WriteDemoPics() = 0;

		// draw the 2D pics that were saved out with the current demo frame
		virtual void			DrawDemoPics() = 0;

		// FIXME: add an interface for arbitrary point/texcoord drawing


		// a frame cam consist of 2D drawing and potentially multiple 3D scenes
		// window sizes are needed to convert SCREEN_WIDTH / SCREEN_HEIGHT values
		virtual void			BeginFrame(int windowWidth, int windowHeight) = 0;

		// if the pointers are not NULL, timing info will be returned
		virtual void			EndFrame(int *frontEndMsec, int *backEndMsec) = 0;

		// aviDemo uses this.
		// Will automatically tile render large screen shots if necessary
		// Samples is the number of jittered frames for anti-aliasing
		// If ref == NULL, session->updateScreen will be used
		// This will perform swapbuffers, so it is NOT an approppriate way to
		// generate image files that happen during gameplay, as for savegame
		// markers.  Use WriteRender() instead.
		virtual void			TakeScreenshot(int width, int height, const char *fileName, int samples, struct renderView_s *ref) = 0;

		// the render output can be cropped down to a subset of the real screen, as
		// for save-game reviews and split-screen multiplayer.  Users of the renderer
		// will not know the actual pixel size of the area they are rendering to

		// the x,y,width,height values are in virtual SCREEN_WIDTH / SCREEN_HEIGHT coordinates

		// to render to a texture, first set the crop size with makePowerOfTwo = true,
		// then perform all desired rendering, then capture to an image
		// if the specified physical dimensions are larger than the current cropped region, they will be cut down to fit
		virtual void			CropRenderSize(int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false) = 0;
		virtual void			CaptureRenderToImage(const char *imageName) = 0;
		// fixAlpha will set all the alpha channel values to 0xff, which allows screen captures
		// to use the default tga loading code without having dimmed down areas in many places
		virtual void			CaptureRenderToFile(const char *fileName, bool fixAlpha = false) = 0;
		virtual void			UnCrop() = 0;
#ifdef _QC
        virtual void			GetCardCaps( bool &oldCard, bool &nv10or20 ) = 0;
#endif

		// the image has to be already loaded ( most straightforward way would be through a FindMaterial )
		// texture filter / mipmapping / repeat won't be modified by the upload
		// returns false if the image wasn't found
		virtual bool			UploadImage(const char *imageName, const byte *data, int width, int height) = 0;

#ifdef _RAVEN
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

// RAVEN BEGIN
// rjohnson: new blur special effect
	virtual void			SetSpecialEffect( ESpecialEffectType Which, bool Enabled ) = 0;
	virtual void			SetSpecialEffectParm( ESpecialEffectType Which, int Parm, float Value ) = 0;
	virtual void			ShutdownSpecialEffects( void ) = 0;
// RAVEN END

	// RAVEN BEGIN
	// jnewquist: Deal with flipped back-buffer copies on Xenon
		virtual void			DrawStretchCopy( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) = 0;
	// RAVEN END
	virtual void			DebugGraph( float cur, float min, float max, const idVec4 &color ) = 0;
#endif
#ifdef _MULTITHREAD
		virtual void EndFrame(byte *data, int *frontEndMsec, int *backEndMsec) = 0;
#endif
};

extern idRenderSystem 			*renderSystem;

//
// functions mainly intended for editor and dmap integration
//

// returns the frustum planes in world space
void R_RenderLightFrustum(const struct renderLight_s &renderLight, idPlane lightFrustum[6]);

// for use by dmap to do the carving-on-light-boundaries and for the editor for display
void R_LightProjectionMatrix(const idVec3 &origin, const idPlane &rearPlane, idVec4 mat[4]);

// used by the view shot taker
void R_ScreenshotFilename(int &lastNumber, const char *base, idStr &fileName);


//k for Android large stack memory allocate limit
#define _DYNAMIC_ALLOC_STACK_OR_HEAP

#if 0
#define _ALLOC_DEBUG(x) x
#else
#define _ALLOC_DEBUG(x)
#endif

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP

#ifdef __ANDROID__
#define _DYNAMIC_ALLOC_MAX_STACK "262144" // 256k
#else
#define _DYNAMIC_ALLOC_MAX_STACK "524288" // 512k
#endif

#define _DYNAMIC_ALLOC_CVAR_DECL idCVar harm_r_maxAllocStackMemory("harm_r_maxAllocStackMemory", _DYNAMIC_ALLOC_MAX_STACK, CVAR_INTEGER|CVAR_RENDERER|CVAR_ARCHIVE, "Control allocate temporary memory when load model data, default value is `" _DYNAMIC_ALLOC_MAX_STACK "` bytes(Because stack memory is limited by OS:\n 0 = Always heap;\n Negative = Always stack;\n Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory)).")
#define _DYNAMIC_ALLOC_CVAR_EXTERN extern idCVar harm_r_maxAllocStackMemory

#if 1
extern idCVar harm_r_maxAllocStackMemory;
#define HARM_MAX_STACK_ALLOC_SIZE (harm_r_maxAllocStackMemory.GetInteger())
#else
#define HARM_MAX_STACK_ALLOC_SIZE (1024 * 512)
#endif

struct idAllocAutoHeap {
public:
    idAllocAutoHeap()
            : data(NULL)
    { }

    ~idAllocAutoHeap() {
        Free();
    }

    void * Alloc(size_t size) {
        Free();
        data = calloc(size, 1);
        _ALLOC_DEBUG(common->Printf("%p alloca on heap memory %p(%zu bytes)\n", this, data, size));
        return data;
    }

    void * Alloc16(size_t size) {
        Free();
        data = calloc(size + 15, 1);
        void *ptr = ((void *)(((intptr_t)data + 15) & ~15));
        _ALLOC_DEBUG(common->Printf("%p alloca16 on heap memory %p(%zu bytes) <- %p(%zu bytes)\n", this, ptr, size, data, size + 15));
        return ptr;
    }

    bool IsAlloc(void) const {
        return data != NULL;
    }

private:
    void *data;

    void Free(void) {
        if(data) {
            _ALLOC_DEBUG(common->Printf("%p free alloca16 heap memory %p\n", this, data));
            free(data);
            data = NULL;
        }
    }
    void * operator new(size_t);
    void * operator new[](size_t);
    void operator delete(void *);
    void operator delete[](void *);
    idAllocAutoHeap(const idAllocAutoHeap &);
    idAllocAutoHeap & operator=(const idAllocAutoHeap &);
};

// alloc in heap memory
#define _alloca16_heap( x )					((void *)((((intptr_t)calloc( (x)+15 ,1 )) + 15) & ~15))

// Using heap memory. Also reset RLIMIT_STACK by call `setrlimit`.
#define _DROID_ALLOC16_DEF(T, varname, alloc_size) \
	T *varname; \
	_DROID_ALLOC16(T, varname, alloc_size)

#define _DROID_ALLOC16(T, varname, alloc_size) \
	idAllocAutoHeap _allocAutoHeap##_##varname; \
    size_t _alloc_size##_##varname = alloc_size; \
	varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (_alloc_size##_##varname) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##_##varname.Alloc16(_alloc_size##_##varname) : _alloca16(_alloc_size##_##varname)); \
	if(_allocAutoHeap##_##varname.IsAlloc()) { \
		_ALLOC_DEBUG(common->Printf("Alloca16 on heap memory %s %p(%zu bytes)\n", #varname, varname, _alloc_size##_##varname)); \
	}

#define _DROID_ALLOC_DEF(T, varname, alloc_size) \
	T *varname;                                     \
    _DROID_ALLOC(T, varname, alloc_size);

#define _DROID_ALLOC(T, varname, alloc_size) \
	idAllocAutoHeap _allocAutoHeap##_##varname; \
    size_t _alloc_size##_##varname = alloc_size; \
	varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (_alloc_size##_##varname) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##_##varname.Alloc(_alloc_size##_##varname) : _alloca(_alloc_size##_##varname)); \
	if(_allocAutoHeap##_##varname.IsAlloc()) { \
		_ALLOC_DEBUG(common->Printf("Alloca on heap memory %s %p(%zu bytes)\n", #varname, varname, _alloc_size##_##varname)); \
	}

// free memory when not call alloca()
#define _DROID_FREE(varname) \
	{ \
		_ALLOC_DEBUG(common->Printf("Free alloca heap memory %p\n", varname)); \
	}

#endif

#endif /* !__RENDERER_H__ */
