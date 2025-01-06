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

#ifndef __TR_LOCAL_H__
#define __TR_LOCAL_H__

#include "BufferObject.h"
#include "GLMatrix.h"
#include "RenderMatrix.h"
#include "Image.h"
#include "DeclRenderProg.h"
#include "RenderTexture.h"
#include "RenderShadowSystem.h"

class idRenderWorldLocal;
class idRenderEntityLocal;
class idRenderLightLocal;
class idRenderModelCommitted;

// maximum texture units
const int MAX_PROG_TEXTURE_PARMS = 16;

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
const int SMP_FRAMES = 1;

const int FALLOFF_TEXTURE_SIZE =	64;

const float	DEFAULT_FOG_DISTANCE = 500.0f;

const int FOG_ENTER_SIZE = 64;
const float FOG_ENTER = (FOG_ENTER_SIZE+1.0f)/(FOG_ENTER_SIZE*2);
// picky to get the bilerp correct at terminator


// idScreenRect gets carried around with each drawSurf, so it makes sense
// to keep it compact, instead of just using the idBounds class
class idScreenRect {
public:
	short		x1, y1, x2, y2;							// inclusive pixel bounds inside viewport
    float       zmin, zmax;								// for depth bounds test

	void		Clear();								// clear to backwards values
	void		AddPoint( float x, float y );			// adds a point
	void		Expand();								// expand by one pixel each way to fix roundoffs
	void		Intersect( const idScreenRect &rect );
	void		Union( const idScreenRect &rect );
	bool		Equals( const idScreenRect &rect ) const;
	bool		IsEmpty() const;
};

idScreenRect R_ScreenRectFromViewFrustumBounds( const idBounds &bounds );

typedef enum {
	DC_BAD,
	DC_RENDERVIEW,
	DC_UPDATE_ENTITYDEF,
	DC_DELETE_ENTITYDEF,
	DC_UPDATE_LIGHTDEF,
	DC_DELETE_LIGHTDEF,
	DC_LOADMAP,
	DC_CROP_RENDER,
	DC_UNCROP_RENDER,
	DC_CAPTURE_RENDER,
	DC_END_FRAME,
	DC_DEFINE_MODEL,
	DC_SET_PORTAL_STATE,
	DC_UPDATE_SOUNDOCCLUSION,
	DC_GUI_MODEL
} demoCommand_t;

/*
==============================================================================

SURFACES

==============================================================================
*/

#include "../models/ModelDecal.h"
#include "../models/ModelOverlay.h"

// drawSurf_t structures command the back end to render surfaces
// a given srfTriangles_t may be used with multiple idRenderModelCommitted,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique srfTriangles_t

// drawSurf_t are always allocated and freed every frame, they are never cached
static const int	DSF_VIEW_INSIDE_SHADOW	= 1;

#define MAX_RENDERLIGHTS_PER_SURFACE		25

typedef struct drawSurf_s {
	const srfTriangles_t	*geo;
	const idRenderModelCommitted *space;
	const idMaterial		*material;	// may be NULL for shadow volumes
	bool					forceTwoSided;
	float					sort;		// material->sort, modified by gui / entity sort offsets
	const float				*shaderRegisters;	// evaluated and adjusted for referenceShaders
	const struct drawSurf_s	*nextOnLight;	// viewLight chains
	idScreenRect			scissorRect;	// for scissor clipping, local inside renderView viewport
	int						dsFlags;			// DSF_VIEW_INSIDE_SHADOW, etc
	struct vertCache_s		*dynamicTexCoords;	// float * in vertex cache memory

// jmarshall
	int numSurfRenderLights;
	bool hideInMainView;
	const idRenderLightCommitted* surfRenderLights[MAX_RENDERLIGHTS_PER_SURFACE];
	idVec4 forceColor;
// jmarshall end
} drawSurf_t;


typedef struct {
	int		numPlanes;		// this is always 6 for now
	idPlane	planes[6];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to
} shadowFrustum_t;


// areas have references to hold all the lights and entities in them
typedef struct areaReference_s {
	struct areaReference_s *areaNext;				// chain in the area
	struct areaReference_s *areaPrev;
	struct areaReference_s *ownerNext;				// chain on either the entityDef or lightDef
	idRenderEntityLocal *	entity;					// only one of entity / light will be non-NULL
	idRenderLightLocal *	light;					// only one of entity / light will be non-NULL
	struct portalArea_s	*	area;					// so owners can find all the areas they are in
} areaReference_t;

#include "RenderLight.h"
#include "RenderEntity.h"
#include "RenderModelCommitted.h"

const int	MAX_CLIP_PLANES	= 1;				// we may expand this to six for some subview issues

// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
typedef struct {
	const drawSurf_t *	surf;

	idImage *			bumpImage;
	idImage *			diffuseImage;
	idImage *			specularImage;

	idVec4				diffuseColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4				specularColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	stageVertexColor_t	vertexColor;	// applies to both diffuse and specular

	int					ambientLight;	// use tr.ambientNormalMap instead of normalization cube map 
	// (not a bool just to avoid an uninitialized memory check of the pad region by valgrind)

	// these are loaded into the vertex program
	idVec4				localLightOrigin;
	idVec4				localViewOrigin;
	idVec4				bumpMatrix[2];
	idVec4				diffuseMatrix[2];
	idVec4				specularMatrix[2];

	rvmProgramVariants_t programVariant;
} drawInteraction_t;


/*
=============================================================

RENDERER BACK END COMMAND QUEUE

TR_CMDS

=============================================================
*/

typedef enum {
	RC_NOP,
	RC_DRAW_VIEW,
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_RENDER_TOOLGUI,
	RC_SET_RENDERTEXTURE,
	RC_RESOLVE_MSAA,
	RC_CLEAR_RENDERTARGET,
	RC_SWAP_BUFFERS		// can't just assume swap at end of list because
						// of forced list submission before syncs
} renderCommand_t;

typedef struct {
	renderCommand_t		commandId, *next;
} emptyCommand_t;

typedef struct {
	renderCommand_t		commandId, *next;
	GLenum	buffer;
	int		frameCount;
} setBufferCommand_t;

typedef struct {
	renderCommand_t		commandId, *next;
	idRenderWorldCommitted	*viewDef;
} drawSurfsCommand_t;

typedef struct {
	renderCommand_t		commandId, * next;
	rvmToolGui* toolGui;
} drawToolGuiCommand_t;

typedef struct {
	renderCommand_t		commandId, *next;
	int		x, y, imageWidth, imageHeight;
	idImage	*image;
	int		cubeFace;					// when copying to a cubeMap
} copyRenderCommand_t;

typedef struct {
	renderCommand_t		commandId, * next;
	idRenderTexture* renderTexture;
} setRenderTargetCommand_t;

typedef struct {
	renderCommand_t		commandId, * next;
	bool clearColor;
	bool clearDepth;

	float clearDepthValue;
	idVec4 clearColorValue;
} renderClearBufferCommand_t;

typedef struct {
	renderCommand_t		commandId, * next;
	idRenderTexture* msaaRenderTexture;
	idRenderTexture* destRenderTexture;
} resolveRenderTargetCommand_t;

//=======================================================================

// this is the inital allocation for max number of drawsurfs
// in a given view, but it will automatically grow if needed
const int	INITIAL_DRAWSURFS =			0x4000;

// a request for frame memory will never fail
// (until malloc fails), but it may force the
// allocation of a new memory block that will
// be discontinuous with the existing memory
typedef struct frameMemoryBlock_s {
	struct frameMemoryBlock_s *next;
	int		size;
	int		used;
	int		poop;			// so that base is 16 byte aligned
	byte	base[4];	// dynamically allocated as [size]
} frameMemoryBlock_t;

// all of the information needed by the back end must be
// contained in a frameData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine (OBSOLETE: this capability has been removed)
typedef struct {
	// one or more blocks of memory for all frame
	// temporary allocations
	frameMemoryBlock_t	*memory;

	// alloc will point somewhere into the memory chain
	frameMemoryBlock_t	*alloc;

	srfTriangles_t *	firstDeferredFreeTriSurf;
	srfTriangles_t *	lastDeferredFreeTriSurf;

	int					memoryHighwater;	// max used on any frame

	// the currently building command list 
	// commands can be inserted at the front if needed, as for required
	// dynamically generated textures
	emptyCommand_t	*cmdHead, *cmdTail;		// may be of other command type based on commandId
} frameData_t;

extern	frameData_t	*frameData;

//=======================================================================

void R_LockSurfaceScene( idRenderWorldCommitted *parms );
void R_ClearCommandChain( void );

void R_ReloadGuis_f( const idCmdArgs &args );
void R_ListGuis_f( const idCmdArgs &args );

void *R_GetCommandBuffer( int bytes );

// this allows a global override of all materials
bool R_GlobalShaderOverride( const idMaterial **shader );

// this does various checks before calling the idDeclSkin
const idMaterial *R_RemapShaderBySkin( const idMaterial *shader, const idDeclSkin *customSkin, const idMaterial *customShader );


//====================================================


/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_in, c_sphere_cull_clip, c_sphere_cull_out;
	int		c_box_cull_in, c_box_cull_out;
	int		c_createInteractions;	// number of calls to idInteraction::CreateInteraction
	int		c_createLightTris;
	int		c_createShadowVolumes;
	int		c_generateMd5;
	int		c_entityDefCallbacks;
	int		c_alloc, c_free;	// counts for R_StaticAllc/R_StaticFree
	int		c_visibleViewEntities;
	int		c_shadowViewEntities;
	int		c_viewLights;
	int		c_numViews;			// number of total views rendered
	int		c_deformedSurfaces;	// idMD5Mesh::GenerateSurface
	int		c_deformedVerts;	// idMD5Mesh::GenerateSurface
	int		c_deformedIndexes;	// idMD5Mesh::GenerateSurface
	int		c_tangentIndexes;	// R_DeriveTangents()
	int		c_entityUpdates, c_lightUpdates, c_entityReferences, c_lightReferences;
	int		c_guiSurfs;
	int		frontEndMsec;		// sum of time in all RE_RenderScene's in a frame
} performanceCounters_t;


typedef struct {
	int		current2DMap;
	int		current3DMap;
	int		currentCubeMap;
	int		texEnv;
	textureType_t	textureType;
} tmu_t;

const int MAX_MULTITEXTURE_UNITS =	16;
typedef struct {
	tmu_t		tmu[MAX_MULTITEXTURE_UNITS];
	int			currenttmu;

	int			faceCulling;
	int			glStateBits;
	bool		forceGlState;		// the next GL_State will ignore glStateBits and set everything
} glstate_t;


typedef struct {
	int		c_surfaces;
	int		c_shaders;
	int		c_vertexes;
	int		c_indexes;		// one set per pass
	int		c_totalIndexes;	// counting all passes

	int		c_drawElements;
	int		c_drawIndexes;
	int		c_drawVertexes;
	int		c_drawRefIndexes;
	int		c_drawRefVertexes;

	int		c_shadowElements;
	int		c_shadowIndexes;
	int		c_shadowVertexes;

	int		c_vboIndexes;
	float	c_overDraw;	

	float	maxLightValue;	// for light scale
	int		msec;			// total msec for backend run
} backEndCounters_t;

#include "RenderLightCommitted.h"
#include "RenderWorldCommitted.h"

// all state modified by the back end is separated
// from the front end state
typedef struct {
	int					frameCount;		// used to track all images used in a frame
	const idRenderWorldCommitted	*	viewDef;
	backEndCounters_t	pc;

	const idRenderModelCommitted *currentSpace;		// for detecting when a matrix must change
	idScreenRect		currentScissor;
	// for scissor clipping, local inside renderView viewport

	idRenderLightCommitted *		vLight;
	int					depthFunc;			// GLS_DEPTHFUNC_EQUAL, or GLS_DEPTHFUNC_LESS for translucent
	float				lightTextureMatrix[16];	// only if lightStage->texture.hasMatrix
	float				lightColor[4];		// evaluation of current light's color stage

	float				lightScale;			// Every light color calaculation will be multiplied by this,
											// which will guarantee that the result is < tr.backEndRendererMaxLight
											// A card with high dynamic range will have this set to 1.0
	float				overBright;			// The amount that all light interactions must be multiplied by
											// with post processing to get the desired total light level.
											// A high dynamic range card will have this set to 1.0.

	bool				currentRenderCopied;	// true if any material has already referenced _currentRender

	int					currentShadowMapSlice;

	// our OpenGL state deltas
	glstate_t			glState;

	int					c_copyFrameBuffer;

	idRenderTexture	*	renderTexture;

	bool				clearColor;
	idVec4				clearColorValue;

	bool				clearDepth;
	float				clearDepthValue;
} backEndState_t;


const int MAX_GUI_SURFACES	= 1024;		// default size of the drawSurfs list for guis, will
										// be automatically expanded as needed

typedef enum {
	BE_ARB,
	BE_NV10,
	BE_NV20,
	BE_R200,
	BE_ARB2,
	BE_BAD
} backEndName_t;

typedef struct {
	int		x, y, width, height;	// these are in physical, OpenGL Y-at-bottom pixels
} renderCrop_t;
static const int	MAX_RENDER_CROPS = 8;

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
class idRenderSystemLocal : public idRenderSystem {
public:
	// external functions
	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			InitOpenGL( void );
	virtual void			ShutdownOpenGL( void );
	virtual bool			IsOpenGLRunning( void ) const;
	virtual bool			IsFullScreen( void ) const;
	virtual int				GetScreenWidth( void ) const;
	virtual int				GetScreenHeight( void ) const;
	virtual idRenderWorld *	AllocRenderWorld( void );
	virtual void			FreeRenderWorld( idRenderWorld *rw );
	virtual void			BeginLevelLoad( void );
	virtual void			EndLevelLoad( void );
	virtual idFont*			RegisterFont(const char* fontName);
	virtual idDrawVert*		AllocTris(int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material);

	virtual void			RenderLightFrustum(const renderLight_t& renderLight, idPlane lightFrustum[6]);
	virtual srfTriangles_t*	PolytopeSurface(int numPlanes, const idPlane* planes, idWinding** windings);
	virtual void			FreeStaticTriSurf(srfTriangles_t* tri);

	virtual void			DrawStretchPic ( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
											bool clip = true, float x = 0.0f, float y = 0.0f, float w = 640.0f, float h = 0.0f );
	virtual void			DrawStretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material );

	virtual void			DrawStretchTri ( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material );
	virtual void			GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc );
	virtual void			GetGLSettings( int& width, int& height );
	virtual void			PrintMemInfo( MemInfo_t *mi );

	virtual void			DrawSmallChar( int x, int y, int ch, const idMaterial *material );
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material );
	virtual void			DrawBigChar( int x, int y, int ch, const idMaterial *material );
	virtual void			DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material );
	virtual void			WriteDemoPics();
	virtual void			DrawDemoPics();
	virtual void			BeginFrame( int windowWidth, int windowHeight );
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec );
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int downSample, renderView_t *ref );
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false );
	virtual void			CaptureRenderToImage( const char *imageName );
	virtual void			CaptureRenderToFile( const char *fileName, bool fixAlpha );
	virtual void			UnCrop();
	virtual void			GetCardCaps( bool &oldCard, bool &nv10or20 );
	virtual bool			UploadImage( const char *imageName, const byte *data, int width, int height );
	virtual idRenderTexture* AllocRenderTexture(const char* name, idImage* albedoTexture, idImage* depthTexture, idImage* colorImage2, idImage* colorImage3, idImage* colorImage4);
	virtual idImage*		CreateImage(const char* name, idImageOpts* opts, textureFilter_t textureFilter);
	rvmDeclRenderProg*		FindRenderProgram(const char* name) { return (rvmDeclRenderProg*)declManager->FindType(DECL_RENDERPROGS, name); }	
	virtual void			NukeShadowMapCache(void);
	virtual void			InvalidateShadowMap(int uniqueLightID);
	virtual void			RenderToolGui(rvmToolGui* toolGui);
	virtual int				GetNumMSAASamples();	
	virtual void			ResizeImage(idImage* image, int width, int height);
	virtual void			ResizeRenderTexture(idRenderTexture* renderTexture, int width, int height);
	virtual void			BindRenderTexture(idRenderTexture* renderTexture);
	virtual void			ResolveMSAA(idRenderTexture* msaaRenderTexture, idRenderTexture* destRenderTexture);
	virtual void			GetImageSize(idImage* image, int& imageWidth, int& imageHeight);
	virtual void			ClearRenderTarget(bool clearColor, bool clearDepth, float depthValue, float red, float green, float blue);
public:
	// internal functions
							idRenderSystemLocal( void );
							~idRenderSystemLocal( void );

	void					Clear( void );
	void					SetBackEndRenderer();			// sets tr.backEndRenderer based on cvars
	void					RenderViewToViewport( const renderView_t *renderView, idScreenRect *viewport );

	void					InitRenderParms(void);
public:
	// renderer globals
	bool					registered;		// cleared at shutdown, set at InitOpenGL

	bool					takingScreenshot;

	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if subviewed)
											// and every R_MarkFragments call

	int						staticAllocCount;	// running total of bytes allocated

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	int						viewportOffset[2];	// for doing larger-than-window tiled renderings
	int						tiledViewport[2];

	// determines which back end to use, and if vertex programs are in use
	backEndName_t			backEndRenderer;
	bool					backEndRendererHasVertexPrograms;
	float					backEndRendererMaxLight;	// 1.0 for standard, unlimited for floats
														// determines how much overbrighting needs
														// to be done post-process

	idVec4					ambientLightVector;	// used for "ambient bump mapping"

	float					sortOffset;				// for determinist sorting of equal sort materials

	idList<idRenderWorldLocal*>worlds;

	idRenderWorldLocal *	primaryWorld;
	renderView_t			primaryRenderView;
	idRenderWorldCommitted *				primaryView;
	// many console commands need to know which world they should operate on

	const idMaterial *		defaultMaterial;
	idImage *				testImage;
	idCinematic *			testVideo;
	float					testVideoStartTime;

	idImage *				ambientCubeImage;	// hack for testing dependent ambient lighting

	idRenderWorldCommitted *				viewDef;

	performanceCounters_t	pc;					// performance counters

	drawSurfsCommand_t		lockSurfacesCmd;	// use this when r_lockSurfaces = 1

	idRenderModelCommitted			identitySpace;		// can use if we don't know viewDef->worldSpace is valid
	FILE *					logFile;			// for logging GL calls and frame breaks

	int						stencilIncr, stencilDecr;	// GL_INCR / INCR_WRAP_EXT, GL_DECR / GL_DECR_EXT

	renderCrop_t			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	class idGuiModel *		guiModel;
	class idGuiModel *		demoGuiModel;

	rvmDeclRenderParam*		albedoTextureParam;
	rvmDeclRenderParam*		bumpmapTextureParam;
	rvmDeclRenderParam*		specularTextureParam;
	rvmDeclRenderParam*		lightfalloffTextureParam;
	rvmDeclRenderParam*		lightProgTextureParam;
	rvmDeclRenderParam*		bumpmatrixSParam;
	rvmDeclRenderParam*		bumpmatrixTParam;
	rvmDeclRenderParam*		diffuseMatrixSParam;
	rvmDeclRenderParam*		diffuseMatrixTParam;
	rvmDeclRenderParam*		specularMatrixSParam;
	rvmDeclRenderParam*		specularMatrixTParam;

	rvmDeclRenderParam*		viewOriginParam;
	rvmDeclRenderParam*		globalLightOriginParam;
	rvmDeclRenderParam*		lightColorParam;
	rvmDeclRenderParam*		lightScaleParam;

	rvmDeclRenderParam*		modelMatrixX;
	rvmDeclRenderParam*		modelMatrixY;
	rvmDeclRenderParam*		modelMatrixZ;
	rvmDeclRenderParam*		modelMatrixW;

	rvmDeclRenderParam*		viewMatrixX;
	rvmDeclRenderParam*		viewMatrixY;
	rvmDeclRenderParam*		viewMatrixZ;
	rvmDeclRenderParam*		viewMatrixW;

	rvmDeclRenderParam*		projectionMatrixX;
	rvmDeclRenderParam*		projectionMatrixY;
	rvmDeclRenderParam*		projectionMatrixZ;
	rvmDeclRenderParam*		projectionMatrixW;

	rvmDeclRenderParam* mvpMatrixX;
	rvmDeclRenderParam* mvpMatrixY;
	rvmDeclRenderParam* mvpMatrixZ;
	rvmDeclRenderParam* mvpMatrixW;

	rvmDeclRenderParam* numLightsParam;

	rvmDeclRenderParam* globalLightExtentsParam;

	rvmDeclRenderParam* vertexScaleAddParam;
	rvmDeclRenderParam* vertexScaleModulateParam;

	rvmDeclRenderParam* shadowMapInfoParm;

	rvmDeclRenderParam* shadowMapAtlasParam;
	rvmDeclRenderParam* atlasLookupParam;
	rvmDeclRenderParam* vertexColorParm;
	rvmDeclRenderParam* screenInfoParam;
	rvmDeclRenderParam* genericShaderParam;
#ifdef __ANDROID__ //karin: extras uniforms of GLSL for GLES
	rvmDeclRenderParam* textureMatrixX;
	rvmDeclRenderParam* textureMatrixY;
	rvmDeclRenderParam* textureMatrixZ;
	rvmDeclRenderParam* textureMatrixW;

	rvmDeclRenderParam* alphaTest;
	rvmDeclRenderParam* texgenS;
	rvmDeclRenderParam* texgenT;
	rvmDeclRenderParam* texgenQ;
#endif

	idStr					globalRenderInclude;

	rvmDeclRenderProg*		guiTextureProgram;
	rvmDeclRenderProg*		guiColorProgram;
	rvmDeclRenderProg*		interactionProgram[PROG_VARIANT_NUMVARIANTS];
	rvmDeclRenderProg*		interactionEditorSelectProgram[PROG_VARIANT_NUMVARIANTS];
	rvmDeclRenderProg*		occluderProgram[PROG_VARIANT_NUMVARIANTS];
	rvmDeclRenderProg*		shadowMapProgram[PROG_VARIANT_NUMVARIANTS];
	rvmDeclRenderProg*		shadowMapAlbedoProgram[PROG_VARIANT_NUMVARIANTS];

	unsigned short			gammaTable[256];	// brightness / gamma modify this

	idList<idFont*>			fonts;
};

extern backEndState_t		backEnd;
extern idRenderSystemLocal	tr;
extern glconfig_t			glConfig;		// outside of TR since it shouldn't be cleared during ref re-init


//
// cvars
//
extern idCVar r_ext_vertex_array_range;

extern idCVar r_glDriver;				// "opengl32", etc
extern idCVar r_mode;					// video mode number
extern idCVar r_displayRefresh;			// optional display refresh rate option for vid mode
extern idCVar r_fullscreen;				// 0 = windowed, 1 = full screen
extern idCVar r_multiSamples;			// number of antialiasing samples

extern idCVar r_ignore;					// used for random debugging without defining new vars
extern idCVar r_ignore2;				// used for random debugging without defining new vars
extern idCVar r_znear;					// near Z clip plane

extern idCVar r_finish;					// force a call to qglFinish() every frame
extern idCVar r_frontBuffer;			// draw to front buffer for debugging
extern idCVar r_swapInterval;			// changes wglSwapIntarval
extern idCVar r_offsetFactor;			// polygon offset parameter
extern idCVar r_offsetUnits;			// polygon offset parameter
extern idCVar r_singleTriangle;			// only draw a single triangle per primitive
extern idCVar r_logFile;				// number of frames to emit GL logs
extern idCVar r_clear;					// force screen clear every frame
extern idCVar r_shadows;				// enable shadows
extern idCVar r_subviewOnly;			// 1 = don't render main view, allowing subviews to be debugged
extern idCVar r_lightScale;				// all light intensities are multiplied by this, which is normally 2
extern idCVar r_flareSize;				// scale the flare deforms from the material def

extern idCVar r_gamma;					// changes gamma tables
extern idCVar r_brightness;				// changes gamma tables

extern idCVar r_renderer;				// arb, nv10, nv20, r200, gl2, etc

extern idCVar r_cgVertexProfile;		// arbvp1, vp20, vp30
extern idCVar r_cgFragmentProfile;		// arbfp1, fp30

extern idCVar r_checkBounds;			// compare all surface bounds with precalculated ones

extern idCVar r_useNV20MonoLights;		// 1 = allow an interaction pass optimization
extern idCVar r_useLightPortalFlow;		// 1 = do a more precise area reference determination
extern idCVar r_useTripleTextureARB;	// 1 = cards with 3+ texture units do a two pass instead of three pass
extern idCVar r_useShadowSurfaceScissor;// 1 = scissor shadows by the scissor rect of the interaction surfaces
extern idCVar r_useConstantMaterials;	// 1 = use pre-calculated material registers if possible
extern idCVar r_useInteractionTable;	// create a full entityDefs * lightDefs table to make finding interactions faster
extern idCVar r_useNodeCommonChildren;	// stop pushing reference bounds early when possible
extern idCVar r_useSilRemap;			// 1 = consider verts with the same XYZ, but different ST the same for shadows
extern idCVar r_useCulling;				// 0 = none, 1 = sphere, 2 = sphere + box
extern idCVar r_useLightCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces
extern idCVar r_useLightScissors;		// 1 = use custom scissor rectangle for each light
extern idCVar r_useClippedLightScissors;// 0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always
extern idCVar r_useEntityCulling;		// 0 = none, 1 = box
extern idCVar r_useEntityScissors;		// 1 = use custom scissor rectangle for each entity
extern idCVar r_useInteractionCulling;	// 1 = cull interactions
extern idCVar r_useInteractionScissors;	// 1 = use a custom scissor rectangle for each interaction
extern idCVar r_useFrustumFarDistance;	// if != 0 force the view frustum far distance to this distance
extern idCVar r_useShadowCulling;		// try to cull shadows from partially visible lights
extern idCVar r_usePreciseTriangleInteractions;	// 1 = do winding clipping to determine if each ambiguous tri should be lit
extern idCVar r_useTurboShadow;			// 1 = use the infinite projection with W technique for dynamic shadows
extern idCVar r_useExternalShadows;		// 1 = skip drawing caps when outside the light volume
extern idCVar r_useOptimizedShadows;	// 1 = use the dmap generated static shadow volumes
extern idCVar r_useShadowVertexProgram;	// 1 = do the shadow projection in the vertex program on capable cards
extern idCVar r_useShadowProjectedCull;	// 1 = discard triangles outside light volume before shadowing
extern idCVar r_useDeferredTangents;	// 1 = don't always calc tangents after deform
extern idCVar r_useCachedDynamicModels;	// 1 = cache snapshots of dynamic models
extern idCVar r_useTwoSidedStencil;		// 1 = do stencil shadows in one pass with different ops on each side
extern idCVar r_useInfiniteFarZ;		// 1 = use the no-far-clip-plane trick
extern idCVar r_useScissor;				// 1 = scissor clip as portals and lights are processed
extern idCVar r_usePortals;				// 1 = use portals to perform area culling, otherwise draw everything
extern idCVar r_useStateCaching;		// avoid redundant state changes in GL_*() calls
extern idCVar r_useCombinerDisplayLists;// if 1, put all nvidia register combiner programming in display lists
extern idCVar r_useVertexBuffers;		// if 0, don't use ARB_vertex_buffer_object for vertexes
extern idCVar r_useIndexBuffers;		// if 0, don't use ARB_vertex_buffer_object for indexes
extern idCVar r_useEntityCallbacks;		// if 0, issue the callback immediately at update time, rather than defering
extern idCVar r_lightAllBackFaces;		// light all the back faces, even when they would be shadowed
extern idCVar r_useDepthBoundsTest;     // use depth bounds test to reduce shadow fill

extern idCVar r_skipPostProcess;		// skip all post-process renderings
extern idCVar r_skipSuppress;			// ignore the per-view suppressions
extern idCVar r_skipInteractions;		// skip all light/surface interaction drawing
extern idCVar r_skipFrontEnd;			// bypasses all front end work, but 2D gui rendering still draws
extern idCVar r_skipBackEnd;			// don't draw anything
extern idCVar r_skipCopyTexture;		// do all rendering, but don't actually copyTexSubImage2D
extern idCVar r_skipRender;				// skip 3D rendering, but pass 2D
extern idCVar r_skipRenderContext;		// NULL the rendering context during backend 3D rendering
extern idCVar r_skipTranslucent;		// skip the translucent interaction rendering
extern idCVar r_skipAmbient;			// bypasses all non-interaction drawing
extern idCVar r_skipNewAmbient;			// bypasses all vertex/fragment program ambients
extern idCVar r_skipBlendLights;		// skip all blend lights
extern idCVar r_skipFogLights;			// skip all fog lights
extern idCVar r_skipSubviews;			// 1 = don't render any mirrors / cameras / etc
extern idCVar r_skipGuiShaders;			// 1 = don't render any gui elements on surfaces
extern idCVar r_skipParticles;			// 1 = don't render any particles
extern idCVar r_skipUpdates;			// 1 = don't accept any entity or light updates, making everything static
extern idCVar r_skipDeforms;			// leave all deform materials in their original state
extern idCVar r_skipDynamicTextures;	// don't dynamically create textures
extern idCVar r_skipLightScale;			// don't do any post-interaction light scaling, makes things dim on low-dynamic range cards
extern idCVar r_skipBump;				// uses a flat surface instead of the bump map
extern idCVar r_skipSpecular;			// use black for specular
extern idCVar r_skipDiffuse;			// use black for diffuse
extern idCVar r_skipOverlays;			// skip overlay surfaces
extern idCVar r_skipROQ;

extern idCVar r_ignoreGLErrors;

extern idCVar r_forceLoadImages;		// draw all images to screen after registration
extern idCVar r_demonstrateBug;			// used during development to show IHV's their problems
extern idCVar r_screenFraction;			// for testing fill rate, the resolution of the entire screen can be changed

extern idCVar r_showUnsmoothedTangents;	// highlight geometry rendered with unsmoothed tangents
extern idCVar r_showSilhouette;			// highlight edges that are casting shadow planes
extern idCVar r_showVertexColor;		// draws all triangles with the solid vertex color
extern idCVar r_showUpdates;			// report entity and light updates and ref counts
extern idCVar r_showDemo;				// report reads and writes to the demo file
extern idCVar r_showDynamic;			// report stats on dynamic surface generation
extern idCVar r_showLightScale;			// report the scale factor applied to drawing for overbrights
extern idCVar r_showIntensity;			// draw the screen colors based on intensity, red = 0, green = 128, blue = 255
extern idCVar r_showDefs;				// report the number of modeDefs and lightDefs in view
extern idCVar r_showTrace;				// show the intersection of an eye trace with the world
extern idCVar r_showSmp;				// show which end (front or back) is blocking
extern idCVar r_showDepth;				// display the contents of the depth buffer and the depth range
extern idCVar r_showImages;				// draw all images to screen instead of rendering
extern idCVar r_showTris;				// enables wireframe rendering of the world
extern idCVar r_showSurfaceInfo;		// show surface material name under crosshair
extern idCVar r_showNormals;			// draws wireframe normals
extern idCVar r_showEdges;				// draw the sil edges
extern idCVar r_showViewEntitys;		// displays the bounding boxes of all view models and optionally the index
extern idCVar r_showTexturePolarity;	// shade triangles by texture area polarity
extern idCVar r_showTangentSpace;		// shade triangles by tangent space
extern idCVar r_showDominantTri;		// draw lines from vertexes to center of dominant triangles
extern idCVar r_showTextureVectors;		// draw each triangles texture (tangent) vectors
extern idCVar r_showLights;				// 1 = print light info, 2 = also draw volumes
extern idCVar r_showLightCount;			// colors surfaces based on light count
extern idCVar r_showShadows;			// visualize the stencil shadow volumes
extern idCVar r_showShadowCount;		// colors screen based on shadow volume depth complexity
extern idCVar r_showLightScissors;		// show light scissor rectangles
extern idCVar r_showEntityScissors;		// show entity scissor rectangles
extern idCVar r_showInteractionFrustums;// show a frustum for each interaction
extern idCVar r_showInteractionScissors;// show screen rectangle which contains the interaction frustum
extern idCVar r_showMemory;				// print frame memory utilization
extern idCVar r_showCull;				// report sphere and box culling stats
extern idCVar r_showInteractions;		// report interaction generation activity
extern idCVar r_showSurfaces;			// report surface/light/shadow counts
extern idCVar r_showPrimitives;			// report vertex/index/draw counts
extern idCVar r_showPortals;			// draw portal outlines in color based on passed / not passed
extern idCVar r_showAlloc;				// report alloc/free counts
extern idCVar r_showSkel;				// draw the skeleton when model animates
extern idCVar r_showOverDraw;			// show overdraw
extern idCVar r_jointNameScale;			// size of joint names when r_showskel is set to 1
extern idCVar r_jointNameOffset;		// offset of joint names when r_showskel is set to 1

extern idCVar r_testGamma;				// draw a grid pattern to test gamma levels
extern idCVar r_testStepGamma;			// draw a grid pattern to test gamma levels
extern idCVar r_testGammaBias;			// draw a grid pattern to test gamma levels

extern idCVar r_testARBProgram;			// experiment with vertex/fragment programs

extern idCVar r_singleLight;			// suppress all but one light
extern idCVar r_singleEntity;			// suppress all but one entity
extern idCVar r_singleArea;				// only draw the portal area the view is actually in
extern idCVar r_singleSurface;			// suppress all but one surface on each entity
extern idCVar r_shadowPolygonOffset;	// bias value added to depth test for stencil shadow drawing
extern idCVar r_shadowPolygonFactor;	// scale value for stencil shadow drawing

extern idCVar r_jitter;					// randomly subpixel jitter the projection matrix
extern idCVar r_lightSourceRadius;		// for soft-shadow sampling
extern idCVar r_lockSurfaces;
extern idCVar r_orderIndexes;			// perform index reorganization to optimize vertex use

extern idCVar r_debugLineDepthTest;		// perform depth test on debug lines
extern idCVar r_debugLineWidth;			// width of debug lines
extern idCVar r_debugArrowStep;			// step size of arrow cone line rotation in degrees
extern idCVar r_debugPolygonFilled;

extern idCVar r_materialOverride;		// override all materials

extern idCVar r_debugRenderToTexture;

/*
====================================================================

GL wrapper/helper functions

====================================================================
*/

void	GL_SelectTexture( int unit );
void	GL_CheckErrors( void );
void	GL_ClearStateDelta( void );
void	GL_State( int stateVector );
void	GL_TexEnv( int env );
void	GL_Cull( int cullType );

const int GLS_SRCBLEND_ZERO						= 0x00000001;
const int GLS_SRCBLEND_ONE						= 0x0;
const int GLS_SRCBLEND_DST_COLOR				= 0x00000003;
const int GLS_SRCBLEND_ONE_MINUS_DST_COLOR		= 0x00000004;
const int GLS_SRCBLEND_SRC_ALPHA				= 0x00000005;
const int GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000006;
const int GLS_SRCBLEND_DST_ALPHA				= 0x00000007;
const int GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		= 0x00000008;
const int GLS_SRCBLEND_ALPHA_SATURATE			= 0x00000009;
const int GLS_SRCBLEND_BITS						= 0x0000000f;

const int GLS_DSTBLEND_ZERO						= 0x0;
const int GLS_DSTBLEND_ONE						= 0x00000020;
const int GLS_DSTBLEND_SRC_COLOR				= 0x00000030;
const int GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		= 0x00000040;
const int GLS_DSTBLEND_SRC_ALPHA				= 0x00000050;
const int GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000060;
const int GLS_DSTBLEND_DST_ALPHA				= 0x00000070;
const int GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		= 0x00000080;
const int GLS_DSTBLEND_BITS						= 0x000000f0;


// these masks are the inverse, meaning when set the qglColorMask value will be 0,
// preventing that channel from being written
const int GLS_DEPTHMASK							= 0x00000100;
const int GLS_REDMASK							= 0x00000200;
const int GLS_GREENMASK							= 0x00000400;
const int GLS_BLUEMASK							= 0x00000800;
const int GLS_ALPHAMASK							= 0x00001000;
const int GLS_COLORMASK							= (GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK);

const int GLS_POLYMODE_LINE						= 0x00002000;

const int GLS_DEPTHFUNC_ALWAYS					= 0x00010000;
const int GLS_DEPTHFUNC_EQUAL					= 0x00020000;
const int GLS_DEPTHFUNC_LESS					= 0x0;

const int GLS_ATEST_EQ_255						= 0x10000000;
const int GLS_ATEST_LT_128						= 0x20000000;
const int GLS_ATEST_GE_128						= 0x40000000;
const int GLS_ATEST_BITS						= 0x70000000;

const int GLS_DEFAULT							= GLS_DEPTHFUNC_ALWAYS;

void R_Init( void );
void R_InitOpenGL( void );

void R_DoneFreeType( void );

void R_SetColorMappings( void );

void R_ScreenShot_f( const idCmdArgs &args );
void R_StencilShot( void );

bool R_CheckExtension( char *name );


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

typedef struct {
	int			width;
	int			height;
	bool		fullScreen;
	bool		stereo;
	int			displayHz;
	int			multiSamples;
} glimpParms_t;

bool		GLimp_Init( glimpParms_t parms );
// If the desired mode can't be set satisfactorily, false will be returned.
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.

bool		GLimp_SetScreenParms( glimpParms_t parms );
// will set up gl up with the new parms

void		GLimp_Shutdown( void );
// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.

void		GLimp_SwapBuffers( void );
// Calls the system specific swapbuffers routine, and may also perform
// other system specific cvar checks that happen every frame.
// This will not be called if 'r_drawBuffer GL_FRONT'

void		GLimp_SetGamma( unsigned short red[256], 
						    unsigned short green[256],
							unsigned short blue[256] );
// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision


bool		GLimp_SpawnRenderThread( void (*function)( void ) );
// Returns false if the system only has a single processor

void *		GLimp_BackEndSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeBackEnd( void *data );
// these functions implement the dual processor syncronization

void		GLimp_ActivateContext( void );
void		GLimp_DeactivateContext( void );
// These are used for managing SMP handoffs of the OpenGL context
// between threads, and as a performance tunining aid.  Setting
// 'r_skipRenderContext 1' will call GLimp_DeactivateContext() before
// the 3D rendering code, and GLimp_ActivateContext() afterwards.  On
// most OpenGL implementations, this will result in all OpenGL calls
// being immediate returns, which lets us guage how much time is
// being spent inside OpenGL.

void		GLimp_EnableLogging( bool enable );


/*
====================================================================

MAIN

====================================================================
*/

// performs radius cull first, then corner cull
bool R_CullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes );
bool R_RadiusCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes );
bool R_CornerCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes );

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] );

// note that many of these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out );
void R_PointTimesMatrix( const float modelMatrix[16], const idVec4 &in, idVec4 &out );
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out );
void R_TransformEyeZToWin( float src_z, const float *projectionMatrix, float &dst_z );

void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc );

void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst );

void R_TransformClipToDevice( const idPlane &clip, const idRenderWorldCommitted *view, idVec3 &normalized );

void R_TransposeGLMatrix( const float in[16], float out[16] );

void R_SetViewMatrix( idRenderWorldCommitted *viewDef );

void myGlMultMatrix( const float *a, const float *b, float *out );

/*
============================================================

LIGHT

============================================================
*/

void R_ListRenderLightDefs_f( const idCmdArgs &args );
void R_ListRenderEntityDefs_f( const idCmdArgs &args );

bool R_IssueEntityDefCallback( idRenderEntityLocal *def );
idRenderModel *R_EntityDefDynamicModel( idRenderEntityLocal *def );

drawSurf_t *R_AddDrawSurf( const srfTriangles_t *tri, const idRenderModelCommitted *space, const renderEntity_t *renderEntity,
					const idMaterial *shader, const idScreenRect &scissor, idVec4 forceColor = vec4_one );


bool R_CreateAmbientCache( srfTriangles_t *tri, bool needsLighting );
bool R_CreateLightingCache( const idRenderEntityLocal *ent, const idRenderLightLocal *light, srfTriangles_t *tri );
void R_CreatePrivateShadowCache( srfTriangles_t *tri );
void R_CreateVertexProgramShadowCache( srfTriangles_t *tri );

/*
============================================================

LIGHTRUN

============================================================
*/


void R_FreeDerivedData( void );
void R_ReCreateWorldReferences( void );
void R_CheckForEntityDefsUsingModel(idRenderModel* model);

/*
============================================================

POLYTOPE

============================================================
*/

srfTriangles_t *R_PolytopeSurface( int numPlanes, const idPlane *planes, idWinding **windings );

/*
============================================================

RENDER

============================================================
*/

void RB_EnterWeaponDepthHack();
void RB_EnterModelDepthHack( float depth );
void RB_LeaveDepthHack();
void RB_DrawElementsImmediate( const srfTriangles_t *tri );
void RB_RenderTriangleSurface( const srfTriangles_t *tri );
void RB_T_RenderTriangleSurface( const drawSurf_t *surf );
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, 
					  void (*triFunc_)( const drawSurf_t *) );
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, 
										void (*triFunc_)( const drawSurf_t *) );
void RB_DrawShaderPasses( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture );
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] );
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *), rvmProgramVariants_t programVariant);

const shaderStage_t *RB_SetLightTexture( const idRenderLightLocal *light );

void RB_DetermineLightScale( void );
void RB_BeginDrawingView (void);

/*
============================================================

DRAW_STANDARD

============================================================
*/

void RB_DrawElementsWithCounters( const srfTriangles_t *tri );
void RB_DrawShadowElementsWithCounters( const srfTriangles_t *tri, int numIndexes );
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters );
void RB_BindStageTexture( const float *shaderRegisters, const textureStage_t *texture, const drawSurf_t *surf );
void RB_FinishStageTexture( const textureStage_t *texture, const drawSurf_t *surf );

/*
============================================================

TRISURF

============================================================
*/

#define USE_TRI_DATA_ALLOCATOR

void				R_InitTriSurfData( void );
void				R_ShutdownTriSurfData( void );
void				R_PurgeTriSurfData( frameData_t *frame );
void				R_ShowTriSurfMemory_f( const idCmdArgs &args );

srfTriangles_t *	R_AllocStaticTriSurf( void );
srfTriangles_t *	R_CopyStaticTriSurf( const srfTriangles_t *tri );
void				R_AllocStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_AllocStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_AllocStaticTriSurfPlanes( srfTriangles_t *tri, int numIndexes );
void				R_ResizeStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_ResizeStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_ReferenceStaticTriSurfVerts( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_ReferenceStaticTriSurfIndexes( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_FreeStaticTriSurfSilIndexes( srfTriangles_t *tri );
void				R_FreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeStaticTriSurfVertexCaches( srfTriangles_t *tri );
void				R_ReallyFreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeDeferredTriSurfs( frameData_t *frame );
int					R_TriSurfMemory( const srfTriangles_t *tri );

void				R_BoundTriSurf( srfTriangles_t *tri );
void				R_RemoveDuplicatedTriangles( srfTriangles_t *tri );
void				R_CreateSilIndexes( srfTriangles_t *tri );
void				R_RemoveDegenerateTriangles( srfTriangles_t *tri );
void				R_RemoveUnusedVerts( srfTriangles_t *tri );
void				R_RangeCheckIndexes( const srfTriangles_t *tri );
void				R_CreateVertexNormals( srfTriangles_t *tri );	// also called by dmap
void				R_DeriveFacePlanes( srfTriangles_t *tri );		// also called by renderbump
void				R_CleanupTriangles( srfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents );
void				R_ReverseTriangles( srfTriangles_t *tri );

// Only deals with vertexes and indexes, not silhouettes, planes, etc.
// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
srfTriangles_t *	R_MergeSurfaceList( const srfTriangles_t **surfaces, int numSurfaces );
srfTriangles_t *	R_MergeTriangles( const srfTriangles_t *tri1, const srfTriangles_t *tri2 );

// if the deformed verts have significant enough texture coordinate changes to reverse the texture
// polarity of a triangle, the tangents will be incorrect
void				R_DeriveTangents( srfTriangles_t *tri, bool allocFacePlanes = true );

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct deformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;
	idDrawVert* verts;

	int				numMirroredVerts;
	int *			mirroredVerts;

	int				numIndexes;
	glIndex_t *		indexes;

	glIndex_t *		silIndexes;

	int				numDupVerts;
	int *			dupVerts;

	int				numSilEdges;
	silEdge_t *		silEdges;

	dominantTri_t *	dominantTris;
} deformInfo_t;


deformInfo_t *		R_BuildDeformInfo( int numVerts, const idDrawVert *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents );
void				R_FreeDeformInfo( deformInfo_t *deformInfo );
int					R_DeformInfoMemoryUsed( deformInfo_t *deformInfo );

/*
============================================================

SUBVIEW

============================================================
*/

bool	R_PreciseCullSurface( const drawSurf_t *drawSurf, idBounds &ndcBounds );
bool	R_GenerateSubViews( void );

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitFrameData( void );
void R_ShutdownFrameData( void );
int R_CountFrameData( void );
void R_ToggleSmpFrame( void );
void *R_FrameAlloc( int bytes );
void *R_ClearedFrameAlloc( int bytes );

void *R_StaticAlloc( int bytes );		// just malloc with error checking
void *R_ClearedStaticAlloc( int bytes );	// with memset
void R_StaticFree( void *data );


/*
=============================================================

RENDERER DEBUG TOOLS

=============================================================
*/

float RB_DrawTextLength( const char *text, float scale, int len );
void RB_AddDebugText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align, const int lifetime, const bool depthTest );
void RB_ClearDebugText( int time );
void RB_AddDebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifeTime, const bool depthTest );
void RB_ClearDebugLines( int time );
void RB_AddDebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime, const bool depthTest );
void RB_ClearDebugPolygons( int time );
void RB_DrawBounds( const idBounds &bounds );
void RB_ShowLights( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_ShowLightCount( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_PolygonClear( void );
void RB_ScanStencilBuffer( void );
void RB_ShowDestinationAlpha( void );
void RB_ShowOverdraw( void );
void RB_RenderDebugTools( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_ShutdownDebugTools( void );

/*
=============================================================

TR_BACKEND

=============================================================
*/

void RB_SetDefaultGLState( void );
void RB_SetGL2D( void );

// write a comment to the r_logFile if it is enabled
void RB_LogComment( const char *comment, ... ) id_attribute((format(printf,1,2)));

void RB_ShowImages( void );

void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds );


/*
=============================================================

TR_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis( const srfTriangles_t *tri, idVec3 &origin, idVec3 axis[3] );
void R_RenderGuiSurf( idUserInterface *gui, drawSurf_t *drawSurf );

/*
=============================================================

TR_ORDERINDEXES

=============================================================
*/

void R_OrderIndexes( int numIndexes, glIndex_t *indexes );

/*
=============================================================

TR_TRACE

=============================================================
*/

typedef struct {
	float		fraction;
	// only valid if fraction < 1.0
	idVec3		point;
	idVec3		normal;
	int			indexes[3];
} localTrace_t;

localTrace_t R_LocalTrace( const idVec3 &start, const idVec3 &end, const float radius, const srfTriangles_t *tri );
void RB_ShowTrace( drawSurf_t **drawSurfs, int numDrawSurfs );

/*
=============================================================

TR_SHADOWBOUNDS

=============================================================
*/
idScreenRect R_CalcIntersectionScissor( const idRenderLightLocal * lightDef,
									    const idRenderEntityLocal * entityDef,
									    const idRenderWorldCommitted * viewDef );

//=============================================

#include "../models/RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"
#include "jobs/render/Render.h"

void RB_SetModelMatrix(const float* modelMatrix);
void RB_SetMVP(const idRenderMatrix& mvp);
void RB_SetProjectionMatrix(const float* projectionMatrix);
void RB_SetViewMatrix(const float* projectionMatrix);
void RB_BindJointBuffer(idJointBuffer* jointBuffer, float* inverseJointPose, int numJoints, void* colorOffset, void* color2Offset);
void RB_UnBindJointBuffer(void);

// transform Z in eye coordinates to window coordinates
ID_INLINE void R_TransformEyeZToWin(float src_z, const float* projectionMatrix, float& dst_z) {
	float clip_z, clip_w;

	// projection
	clip_z = src_z * projectionMatrix[2 + 2 * 4] + projectionMatrix[2 + 3 * 4];
	clip_w = src_z * projectionMatrix[3 + 2 * 4] + projectionMatrix[3 + 3 * 4];

	if (clip_w <= 0.0f) {
		dst_z = 0.0f;					// clamp to near plane
	}
	else {
		dst_z = clip_z / clip_w;
		dst_z = dst_z * 0.5f + 0.5f;	// convert to window coords
	}
}

/*
======================
R_ScreenRectFromViewFrustumBounds
======================
*/
ID_INLINE idScreenRect R_ScreenRectFromViewFrustumBounds(const idBounds& bounds) {
	idScreenRect screenRect;

	screenRect.x1 = idMath::FtoiFast(0.5f * (1.0f - bounds[1].y) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1));
	screenRect.x2 = idMath::FtoiFast(0.5f * (1.0f - bounds[0].y) * (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1));
	screenRect.y1 = idMath::FtoiFast(0.5f * (1.0f + bounds[0].z) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1));
	screenRect.y2 = idMath::FtoiFast(0.5f * (1.0f + bounds[1].z) * (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1));

	if (r_useDepthBoundsTest.GetInteger()) {
		R_TransformEyeZToWin(-bounds[0].x, tr.viewDef->projectionMatrix, screenRect.zmin);
		R_TransformEyeZToWin(-bounds[1].x, tr.viewDef->projectionMatrix, screenRect.zmax);
	}

	return screenRect;
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
ID_INLINE void R_TransformClipToDevice(const idPlane& clip, const idRenderWorldCommitted* view, idVec3& normalized) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = clip[2] / clip[3];
}

void RB_SetMVP(const float *m);
void RB_SetTextureMatrix(const float* matrix);
void RB_SetTextureMatrix(const idRenderMatrix &matrix);
void RB_SetupMVP_DrawSurf(const drawSurf_t* drawSurf);
void RB_SetupMVP(void);
void RB_SetupMVP_CurrentViewDef(void);
void RB_EnterWeaponDepthHack(const drawSurf_t *surf);
void RB_EnterModelDepthHack(const drawSurf_t *surf );
void RB_LeaveDepthHack(const drawSurf_t *surf);
#define MAX_COLOR_BUFFER_COUNT 5

#ifdef __ANDROID__ //karin: using heap memory instead of _alloca stack memory on Android
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
			common->Printf("[Harmattan]: %p alloca on heap memory %p(%zu bytes)\n", this, data, size);
			return data;
		}

		void * Alloc16(size_t size) {
			Free();
			data = calloc(size + 15, 1);
			void *ptr = ((void *)(((intptr_t)data + 15) & ~15));
			common->Printf("[Harmattan]: %p alloca16 on heap memory %p(%zu bytes) <- %p(%zu bytes)\n", this, ptr, size, data, size + 15);
			return ptr;
		}

		bool IsAlloc(void) const {
			return data != NULL;
		}

	private:
		void *data;

		void Free(void) {
			if(data) {
				common->Printf("[Harmattan]: %p free alloca16 heap memory %p\n", this, data);
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
#define _DROID_ALLOC16_DEF(T, alloc_size, varname, x) \
	idAllocAutoHeap _allocAutoHeap##x; \
	T *varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (alloc_size) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##x.Alloc16(alloc_size) : _alloca16(alloc_size)); \
	if(_allocAutoHeap##x.IsAlloc()) \
		common->Printf("[Harmattan]: Alloca on heap memory %s %p(%lu bytes)\n", #varname, varname, (size_t)alloc_size);

#define _DROID_ALLOC16(T, alloc_size, varname, x) \
	idAllocAutoHeap _allocAutoHeap##x; \
	varname = (T *) (HARM_MAX_STACK_ALLOC_SIZE == 0 || (HARM_MAX_STACK_ALLOC_SIZE > 0 && (alloc_size) >= HARM_MAX_STACK_ALLOC_SIZE) ? _allocAutoHeap##x.Alloc16(alloc_size) : _alloca16(alloc_size)); \
	if(_allocAutoHeap##x.IsAlloc()) \
		common->Printf("[Harmattan]: Alloca on heap memory %s %p(%zu bytes)\n", #varname, varname, (size_t)alloc_size);

#include "esUtil.h"

#endif


enum rvnmOcclusionQueryState
{
    OCCLUSION_QUERY_STATE_HIDDEN,
    OCCLUSION_QUERY_STATE_VISIBLE,
    OCCLUSION_QUERY_STATE_WAITING
};

class rvmOcclusionQuery {
public:
    rvmOcclusionQuery();

    ~rvmOcclusionQuery();

    void RunQuery(idRenderLightLocal* light);

    idRenderLightLocal* GetViewLight() { return light; }

    bool IsVisible();

    int GetQueryStartTime(void);

    bool IsQueryStale(void);

private:
    int queryStartTime;
    int queryTimeOutTime;
    GLuint id;
    idRenderLightLocal* light;
    rvnmOcclusionQueryState queryState;
};

#endif /* !__TR_LOCAL_H__ */
