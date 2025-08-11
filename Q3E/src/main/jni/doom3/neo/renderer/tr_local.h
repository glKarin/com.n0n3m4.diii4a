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

#include "qgl.h"

#include "matrix/esUtil.h"
#if defined(_SHADOW_MAPPING) || defined(_D3BFG_CULLING)
#include "matrix/RenderMatrix.h"
#endif

#ifdef GL_ES_VERSION_3_0 // GLES3.1
#define GL_BLIT_FRAMEBUFFER_AVAILABLE() ( GLIMP_PROCISVALID(qglBlitFramebuffer) )
#define GL_DRAW_BUFFERS_AVAILABLE() ( GLIMP_PROCISVALID(qglDrawBuffers) )
#define GL_DEBUG_MESSAGE_AVAILABLE() ( GLIMP_PROCISVALID(qglDebugMessageControl) && GLIMP_PROCISVALID(qglDebugMessageCallback) && GLIMP_PROCISVALID(qglGetDebugMessageLog))
#endif


extern bool USING_GLES3;
#if !defined(__ANDROID__)
extern bool USING_GL;
#endif
#ifdef _OPENGLES3
extern int GLES3_VERSION;
#define USING_GLES30 (GLES3_VERSION > -1)
#define USING_GLES31 (GLES3_VERSION > 0)
#define USING_GLES32 (GLES3_VERSION > 1)
#endif

#define COLOR_MODULATE_IS_NORMALIZED 1

extern const float zero[];
extern const float one[];
extern const float negOne[];
#ifdef COLOR_MODULATE_IS_NORMALIZED
extern const float oneModulate[];
extern const float negOneModulate[];
#else
#define oneModulate one
#define negOneModulate negOne
#endif

#ifdef _SHADOW_MAPPING

//#define SHADOW_MAPPING_DEBUG

#define MAX_SHADOWMAP_RESOLUTIONS 5

// RB: added multiple subfrustums for cascaded shadow mapping
enum frustumPlanes_t
{
	FRUSTUM_PLANE_LEFT,
	FRUSTUM_PLANE_RIGHT,
	FRUSTUM_PLANE_BOTTOM,
	FRUSTUM_PLANE_TOP,
	FRUSTUM_PLANE_NEAR,
	FRUSTUM_PLANE_FAR,
	FRUSTUM_PLANES = 6,
	FRUSTUM_CLIPALL = 1 | 2 | 4 | 8 | 16 | 32
};

enum
{
	FRUSTUM_PRIMARY,
	FRUSTUM_CASCADE1,
	FRUSTUM_CASCADE2,
	FRUSTUM_CASCADE3,
	FRUSTUM_CASCADE4,
	FRUSTUM_CASCADE5,
	MAX_FRUSTUMS,
};

typedef idPlane frustum_t[FRUSTUM_PLANES];
#endif

#include "rb/Framebuffer.h"
#include "rb/OfflineScreenRenderer.h"
#include "rb/StencilTexture.h"
#include "Image.h"

#include "MegaTexture.h"

struct shaderProgram_s;

class idRenderWorldLocal;

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
class idScreenRect
{
	public:
		short		x1, y1, x2, y2;							// inclusive pixel bounds inside viewport
		float       zmin, zmax;								// for depth bounds test

		void		Clear();								// clear to backwards values
		void		AddPoint(float x, float y);			// adds a point
		void		Expand();								// expand by one pixel each way to fix roundoffs
		void		Intersect(const idScreenRect &rect);
		void		Union(const idScreenRect &rect);
		bool		Equals(const idScreenRect &rect) const;
		bool		IsEmpty() const;

#ifdef _D3BFG_CULLING
        //anon begin
        int			GetArea() const {
            return (x2 - x1 + 1) * (y2 - y1 + 1);
        }
        //anon end
		bool		IsEmptyWithZ() const {
			return IsEmpty() || zmin > zmax;
		}
		void		IntersectWithZ( const idScreenRect &rect ) {
			Intersect( rect );
			zmin = zmin > rect.zmin ? zmin : rect.zmin;
			zmax = zmax < rect.zmax ? zmax : rect.zmax;
		}
#endif
};

idScreenRect R_ScreenRectFromViewFrustumBounds(const idBounds &bounds);
void R_ShowColoredScreenRect(const idScreenRect &rect, int colorIndex);

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

#include "ModelDecal.h"
#include "ModelOverlay.h"
#include "Interaction.h"


// drawSurf_t structures command the back end to render surfaces
// a given srfTriangles_t may be used with multiple viewEntity_t,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique srfTriangles_t

// drawSurf_t are always allocated and freed every frame, they are never cached
static const int	DSF_VIEW_INSIDE_SHADOW	= 1;

typedef struct drawSurf_s {
	const srfTriangles_t	*geo;
	const struct viewEntity_s *space;
	const idMaterial		*material;	// may be NULL for shadow volumes
	float					sort;		// material->sort, modified by gui / entity sort offsets
	const float				*shaderRegisters;	// evaluated and adjusted for referenceShaders
	const struct drawSurf_s	*nextOnLight;	// viewLight chains
	idScreenRect			scissorRect;	// for scissor clipping, local inside renderView viewport
	int						dsFlags;			// DSF_VIEW_INSIDE_SHADOW, etc
	struct vertCache_s		*dynamicTexCoords;	// float * in vertex cache memory
	// specular directions for non vertex program cards, skybox texcoords, etc
#ifdef _MULTITHREAD //k: only for frontend in multithread, like memory address ==
	const srfTriangles_t 	*geoOrig;
#endif
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
	idRenderEntityLocal 	*entity;					// only one of entity / light will be non-NULL
	idRenderLightLocal 	*light;					// only one of entity / light will be non-NULL
	struct portalArea_s		*area;					// so owners can find all the areas they are in
} areaReference_t;


// idRenderLight should become the new public interface replacing the qhandle_t to light defs in the idRenderWorld interface
class idRenderLight
{
	public:
		virtual					~idRenderLight() {}

		virtual void			FreeRenderLight() = 0;
		virtual void			UpdateRenderLight(const renderLight_t *re, bool forceUpdate = false) = 0;
		virtual void			GetRenderLight(renderLight_t *re) = 0;
		virtual void			ForceUpdate() = 0;
		virtual int				GetIndex() = 0;
};


// idRenderEntity should become the new public interface replacing the qhandle_t to entity defs in the idRenderWorld interface
class idRenderEntity
{
	public:
		virtual					~idRenderEntity() {}

		virtual void			FreeRenderEntity() = 0;
		virtual void			UpdateRenderEntity(const renderEntity_t *re, bool forceUpdate = false) = 0;
		virtual void			GetRenderEntity(renderEntity_t *re) = 0;
		virtual void			ForceUpdate() = 0;
		virtual int				GetIndex() = 0;

		// overlays are extra polygons that deform with animating models for blood and damage marks
		virtual void			ProjectOverlay(const idPlane localTextureAxis[2], const idMaterial *material) = 0;
		virtual void			RemoveDecals() = 0;
};


class idRenderLightLocal : public idRenderLight
{
	public:
		idRenderLightLocal();

		virtual void			FreeRenderLight();
		virtual void			UpdateRenderLight(const renderLight_t *re, bool forceUpdate = false);
		virtual void			GetRenderLight(renderLight_t *re);
		virtual void			ForceUpdate();
		virtual int				GetIndex();

		renderLight_t			parms;					// specification

		bool					lightHasMoved;			// the light has changed its position since it was
		// first added, so the prelight model is not valid

		float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

		idRenderWorldLocal 	*world;
		int						index;					// in world lightdefs

		int						areaNum;				// if not -1, we may be able to cull all the light's
		// interactions if !viewDef->connectedAreas[areaNum]

		int						lastModifiedFrameNum;	// to determine if it is constantly changing,
		// and should go in the dynamic frame memory, or kept
		// in the cached memory
		bool					archived;				// for demo writing


		// derived information
		idPlane					lightProject[4];

		const idMaterial 		*lightShader;			// guaranteed to be valid, even if parms.shader isn't
		idImage 				*falloffImage;

		idVec3					globalLightOrigin;		// accounting for lightCenter and parallel


		idPlane					frustum[6];				// in global space, positive side facing out, last two are front/back
		idWinding 				*frustumWindings[6];		// used for culling
		srfTriangles_t 		*frustumTris;			// triangulated frustumWindings[]

		int						numShadowFrustums;		// one for projected lights, usually six for point lights
		shadowFrustum_t			shadowFrustums[6];

		int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
		struct viewLight_s 	*viewLight;

		areaReference_t 		*references;				// each area the light is present in will have a lightRef
		idInteraction 			*firstInteraction;		// doubly linked list
		idInteraction 			*lastInteraction;

		struct doublePortal_s 	*foggedPortals;
#if defined(_SHADOW_MAPPING) || defined(_D3BFG_CULLING)
        RenderMatrix			baseLightProject;		// global xyz1 to projected light strq
        RenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space
#endif
#ifdef _D3BFG_CULLING
        //anon begin
        idBounds				globalLightBounds;
        //anon end
#endif
};


class idRenderEntityLocal : public idRenderEntity
{
	public:
		idRenderEntityLocal();

		virtual void			FreeRenderEntity();
		virtual void			UpdateRenderEntity(const renderEntity_t *re, bool forceUpdate = false);
		virtual void			GetRenderEntity(renderEntity_t *re);
		virtual void			ForceUpdate();
		virtual int				GetIndex();

		// overlays are extra polygons that deform with animating models for blood and damage marks
		virtual void			ProjectOverlay(const idPlane localTextureAxis[2], const idMaterial *material);
		virtual void			RemoveDecals();

		renderEntity_t			parms;

		float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

		idRenderWorldLocal 	*world;
		int						index;					// in world entityDefs

		int						lastModifiedFrameNum;	// to determine if it is constantly changing,
		// and should go in the dynamic frame memory, or kept
		// in the cached memory
		bool					archived;				// for demo writing

		idRenderModel 			*dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
		int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
		// dynamicModel if this doesn't == tr.viewCount
		idRenderModel 			*cachedDynamicModel;

		idBounds				referenceBounds;		// the local bounds used to place entityRefs, either from parms or a model

		// a viewEntity_t is created whenever a idRenderEntityLocal is considered for inclusion
		// in a given view, even if it turns out to not be visible
		int						viewCount;				// if tr.viewCount == viewCount, viewEntity is valid,
		// but the entity may still be off screen
		struct viewEntity_s 	*viewEntity;				// in frame temporary memory

		int						visibleCount;
		// if tr.viewCount == visibleCount, at least one ambient
		// surface has actually been added by R_AddAmbientDrawsurfs
		// note that an entity could still be in the view frustum and not be visible due
		// to portal passing

		idRenderModelDecal 	*decals;					// chain of decals that have been projected on this model
		idRenderModelOverlay 	*overlay;				// blood overlays on animated models

		areaReference_t 		*entityRefs;				// chain of all references
		idInteraction 			*firstInteraction;		// doubly linked list
		idInteraction 			*lastInteraction;

		bool					needsPortalSky;

#ifdef _D3BFG_CULLING
        RenderMatrix			modelRenderMatrix;
        // axis aligned bounding box in world space, derived from refernceBounds and
        // modelMatrix in R_CreateEntityRefs()
        idBounds		globalReferenceBounds;
        RenderMatrix			inverseBaseModelProject;	// transforms the unit cube to exactly cover the model in world space
#endif
};


// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
typedef struct viewLight_s {
	struct viewLight_s 	*next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idRenderLightLocal 	*lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// if the view isn't inside the light, we can use the non-reversed
	// shadow drawing, avoiding the draws of the front and rear caps
	bool					viewInsideLight;

	// true if globalLightOrigin is inside the view frustum, even if it may
	// be obscured by geometry.  This allows us to skip shadows from non-visible objects
	bool					viewSeesGlobalLightOrigin;

	// if !viewInsideLight, the corresponding bit for each of the shadowFrustum
	// projection planes that the view is on the negative side of will be set,
	// allowing us to skip drawing the projected caps of shadows if we can't see the face
	int						viewSeesShadowPlaneBits;

	idVec3					globalLightOrigin;			// global light origin used by backend
	idPlane					lightProject[4];			// light project used by backend
	idPlane					fogPlane;					// fog plane for backend fog volume rendering
	const srfTriangles_t 	*frustumTris;				// light frustum for backend fog volume rendering
	const idMaterial 		*lightShader;				// light shader used by backend
	const float				*shaderRegisters;			// shader registers used by backend
	idImage 				*falloffImage;				// falloff image used by backend

	const struct drawSurf_s	*globalShadows;				// shadow everything
	const struct drawSurf_s	*localInteractions;			// don't get local shadows
	const struct drawSurf_s	*localShadows;				// don't shadow local Surfaces
	const struct drawSurf_s	*globalInteractions;		// get shadows from everything
	const struct drawSurf_s	*translucentInteractions;	// get shadows from everything

#ifdef _SHADOW_MAPPING
    bool					pointLight;					// otherwise a projection light (should probably invert the sense of this, because points are way more common)
    bool					parallel;					// lightCenter gives the direction to the light at infinity
    idVec3					lightCenter;				// offset the lighting direction for shading and
    int						shadowLOD;					// level of detail for shadowmap selection
    RenderMatrix			baseLightProject;			// global xyz1 to projected light strq
	RenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space
    idVec3					lightRadius;		// xyz radius for point lights
	const struct drawSurf_s	*perforatedShadows;	//karin: perforated surface for shadow mapping
#endif
} viewLight_t;


// a viewEntity is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewEntity are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// A single entityDef can generate multiple viewEntity_t in a single frame, as when seen in a mirror
typedef struct viewEntity_s {
	struct viewEntity_s	*next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idRenderEntityLocal	*entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect		scissorRect;

	bool				weaponDepthHack;
	float				modelDepthHack;

	float				modelMatrix[16];		// local coords to global coords
	float				modelViewMatrix[16];	// local coords to eye coords
#ifdef _SHADOW_MAPPING
	RenderMatrix		mvp;
#endif
} viewEntity_t;

const int	MAX_CLIP_PLANES	= 1;				// we may expand this to six for some subview issues

// viewDefs are allocated on the frame temporary stack memory
typedef struct viewDef_s {
	// specified in the call to DrawScene()
	renderView_t		renderView;

	float				projectionMatrix[16];
	viewEntity_t		worldSpace;

	idRenderWorldLocal *renderWorld;

	float				floatTime;

	idVec3				initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool				isSubview;				// true if this view is not the main view
	bool				isMirror;				// the portal is a mirror, invert the face culling
	bool				isXraySubview;

	bool				isEditor;

	int					numClipPlanes;			// mirrors will often use a single clip plane
	idPlane				clipPlanes[MAX_CLIP_PLANES];		// in world space, the positive side
	// of the plane is the visible side
	idScreenRect		viewport;				// in real pixels and proper Y flip

	idScreenRect		scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	struct viewDef_s 	*superView;				// never go into an infinite subview loop
	struct drawSurf_s 	*subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t 		**drawSurfs;				// we don't use an idList for this, because
	int					numDrawSurfs;			// it is allocated in frame temporary memory
	int					maxDrawSurfs;			// may be resized

	struct viewLight_s	*viewLights;			// chain of all viewLights effecting view
	struct viewEntity_s	*viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	idPlane				frustum[5];				// positive sides face outward, [4] is the front clip plane
	idFrustum			viewFrustum;

	int					areaNum;				// -1 = not in a valid area

	bool 				*connectedAreas;
	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.

#ifdef _SHADOW_MAPPING
	// RB parallel light split frustums
    frustum_t			frustums[MAX_FRUSTUMS];					// positive sides face outward, [4] is the front clip plane
    float				frustumSplitDistances[MAX_FRUSTUMS];
    idRenderMatrix		frustumMVPs[MAX_FRUSTUMS];
	// RB
#endif
} viewDef_t;


// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
typedef struct {
	const drawSurf_t 	*surf;

	idImage 			*lightImage;
	idImage 			*lightFalloffImage;
	idImage 			*bumpImage;
	idImage 			*diffuseImage;
	idImage 			*specularImage;

	idVec4				diffuseColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4				specularColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	stageVertexColor_t	vertexColor;	// applies to both diffuse and specular

	int					ambientLight;	// use tr.ambientNormalMap instead of normalization cube map
	// (not a bool just to avoid an uninitialized memory check of the pad region by valgrind)

	// these are loaded into the vertex program
	idVec4				localLightOrigin;
	idVec4				localViewOrigin;
	idVec4				lightProjection[4];	// in local coordinates, possibly with a texture matrix baked in
	idVec4				bumpMatrix[2];
	idVec4				diffuseMatrix[2];
	idVec4				specularMatrix[2];
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
	viewDef_t	*viewDef;
} drawSurfsCommand_t;

typedef struct {
	renderCommand_t		commandId, *next;
	int		x, y, imageWidth, imageHeight;
	idImage	*image;
	int		cubeFace;					// when copying to a cubeMap
} copyRenderCommand_t;

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

	srfTriangles_t 	*firstDeferredFreeTriSurf;
	srfTriangles_t 	*lastDeferredFreeTriSurf;

	int					memoryHighwater;	// max used on any frame

	// the currently building command list
	// commands can be inserted at the front if needed, as for required
	// dynamically generated textures
	emptyCommand_t	*cmdHead, *cmdTail;		// may be of other command type based on commandId
} frameData_t;

extern	frameData_t	*frameData;

//=======================================================================

void R_LockSurfaceScene(viewDef_t *parms);
void R_ClearCommandChain(void);
void R_AddDrawViewCmd(viewDef_t *parms);

void R_ReloadGuis_f(const idCmdArgs &args);
void R_ListGuis_f(const idCmdArgs &args);

void *R_GetCommandBuffer(int bytes);

// this allows a global override of all materials
bool R_GlobalShaderOverride(const idMaterial **shader);

// this does various checks before calling the idDeclSkin
const idMaterial *R_RemapShaderBySkin(const idMaterial *shader, const idDeclSkin *customSkin, const idMaterial *customShader);


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
#ifdef GL_ES_VERSION_3_0
	int		current2DArrayMap;
#endif
} tmu_t;

const int MAX_MULTITEXTURE_UNITS =	8;
typedef struct {
	tmu_t		tmu[MAX_MULTITEXTURE_UNITS];
	int			currenttmu;

	int			faceCulling;
	int			glStateBits;
	bool		forceGlState;		// the next GL_State will ignore glStateBits and set everything

	shaderProgram_s	*currentProgram;
//#ifdef _SHADOW_MAPPING
	idFramebuffer*		currentFramebuffer;
//#endif
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

// all state modified by the back end is separated
// from the front end state
typedef struct {
	int					frameCount;		// used to track all images used in a frame
	const viewDef_t		*viewDef;
	backEndCounters_t	pc;

	const viewEntity_t *currentSpace;		// for detecting when a matrix must change
	idScreenRect		currentScissor;
	// for scissor clipping, local inside renderView viewport

	viewLight_t 		*vLight;
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

	// our OpenGL state deltas
	glstate_t			glState;

	int					c_copyFrameBuffer;
#ifdef _SHADOW_MAPPING
    RenderMatrix		shadowV[6];				// shadow depth view matrix
    RenderMatrix		shadowP[6];				// shadow depth projection matrix
#endif
} backEndState_t;


const int MAX_GUI_SURFACES	= 1024;		// default size of the drawSurfs list for guis, will
// be automatically expanded as needed

typedef enum {
	BE_GLSL,
	BE_BAD
} backEndName_t;

typedef struct {
	int		x, y, width, height;	// these are in physical, OpenGL Y-at-bottom pixels
} renderCrop_t;
static const int	MAX_RENDER_CROPS = 8;

#ifdef _MULTITHREAD
#include "rb/RenderThread.h"
void RB_SetupRenderTools(void);
void R_ShowSurfaceInfo(void); // run on frontend instead of RB_ShowSurfaceInfo
extern idCVar harm_r_renderToolsMultithread;
#endif

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
class idRenderSystemLocal : public idRenderSystem
{
	public:
		// external functions
		virtual void			Init(void);
		virtual void			Shutdown(void);
		virtual void			InitOpenGL(void);
		virtual void			ShutdownOpenGL(void);
		virtual bool			IsOpenGLRunning(void) const;
		virtual bool			IsFullScreen(void) const;
		virtual int				GetScreenWidth(void) const;
		virtual int				GetScreenHeight(void) const;
		virtual idRenderWorld 	*AllocRenderWorld(void);
		virtual void			FreeRenderWorld(idRenderWorld *rw);
		virtual void			BeginLevelLoad(void);
		virtual void			EndLevelLoad(void);
		virtual bool			RegisterFont(const char *fontName, fontInfoEx_t &font);
		virtual void			SetColor(const idVec4 &rgba);
		virtual void			SetColor4(float r, float g, float b, float a);
		virtual void			DrawStretchPic(const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
		                bool clip = true, float x = 0.0f, float y = 0.0f, float w = 640.0f, float h = 0.0f);
		virtual void			DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material);

		virtual void			DrawStretchTri(idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material);
		virtual void			GlobalToNormalizedDeviceCoordinates(const idVec3 &global, idVec3 &ndc);
		virtual void			GetGLSettings(int &width, int &height);
		virtual void			PrintMemInfo(MemInfo_t *mi);

		virtual void			DrawSmallChar(int x, int y, int ch, const idMaterial *material);
		virtual void			DrawSmallStringExt(int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material);
		virtual void			DrawBigChar(int x, int y, int ch, const idMaterial *material);
		virtual void			DrawBigStringExt(int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material);
		virtual void			WriteDemoPics();
		virtual void			DrawDemoPics();
		virtual void			BeginFrame(int windowWidth, int windowHeight);
		virtual void			EndFrame(int *frontEndMsec, int *backEndMsec);
		virtual void			TakeScreenshot(int width, int height, const char *fileName, int downSample, renderView_t *ref);
		virtual void			CropRenderSize(int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false);
		virtual void			CaptureRenderToImage(const char *imageName);
		virtual void			CaptureRenderToFile(const char *fileName, bool fixAlpha);
		virtual void			UnCrop();
		virtual bool			UploadImage(const char *imageName, const byte *data, int width, int height);
#ifdef _HUMANHEAD
        virtual void			SetEntireSceneMaterial(idMaterial* material) { (void)material; }; // HUMANHEAD CJR
        virtual bool			IsScopeView() {
            return scopeView;
        };// HUMANHEAD CJR
        virtual void			SetScopeView(bool view) {
            scopeView = view;
        } // HUMANHEAD CJR
        virtual bool			IsShuttleView() {
            return shuttleView;
        };// HUMANHEAD CJR
        virtual void			SetShuttleView(bool view) {
            shuttleView = view;
        }
        virtual bool			SupportsFragmentPrograms(void) {
            return false;
        };// HUMANHEAD CJR
        virtual int				VideoCardNumber(void) {
            return 0;
        }
#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	    virtual void			LogViewRender(const struct renderView_s *view) { (void)view; }
#endif //HUMANHEAD END

        bool scopeView;
        bool shuttleView;
        int lastRenderSkybox;
        ID_INLINE bool SkyboxRenderedInFrame() const {
            return frameCount == lastRenderSkybox;
        }
        ID_INLINE void RenderSkyboxInFrame() {
            lastRenderSkybox = frameCount;
        }
#endif
#ifdef _MULTITHREAD
	virtual void EndFrame(byte *data, int *frontEndMsec, int *backEndMsec);
#endif

	public:
		// internal functions
		idRenderSystemLocal(void);
		~idRenderSystemLocal(void);

		void					Clear(void);
		void					SetBackEndRenderer();			// sets tr.backEndRenderer based on cvars
		void					RenderViewToViewport(const renderView_t *renderView, idScreenRect *viewport);
#ifdef _RAVEN
#ifndef _CONSOLE
	    virtual void			TrackTextureUsage( TextureTrackCommand command, int frametime = 0, const char *name=NULL ) { (void)command; (void)frametime; (void)name; }
#endif
        // RAVEN END
        virtual void			SetSpecialEffect( ESpecialEffectType Which, bool Enabled ) { (void)Which; (void)Enabled; }
        virtual void			SetSpecialEffectParm( ESpecialEffectType Which, int Parm, float Value ) { (void)Which; (void)Parm; (void)Value; }
        virtual void			ShutdownSpecialEffects( void ) { }
            virtual void			DrawStretchCopy( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) {
                DrawStretchPic(x, y, w, h, s1, t1, s2, t2, material);
            }
        virtual void			DebugGraph( float cur, float min, float max, const idVec4 &color ) { (void)cur, (void)min; (void)max; (void)color; }
#endif

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

		// determines which back end to use
		backEndName_t			backEndRenderer;
		float					backEndRendererMaxLight;	// 1.0 for standard, unlimited for floats
		// determines how much overbrighting needs
		// to be done post-process

		idVec4					ambientLightVector;	// used for "ambient bump mapping"

		float					sortOffset;				// for determinist sorting of equal sort materials

		idList<idRenderWorldLocal *>worlds;

		idRenderWorldLocal 	*primaryWorld;
		renderView_t			primaryRenderView;
		viewDef_t 				*primaryView;
		// many console commands need to know which world they should operate on

		const idMaterial 		*defaultMaterial;
		idImage 				*testImage;
		idCinematic 			*testVideo;
		float					testVideoStartTime;

		idImage 				*ambientCubeImage;	// hack for testing dependent ambient lighting

		viewDef_t 				*viewDef;

		performanceCounters_t	pc;					// performance counters

		drawSurfsCommand_t		lockSurfacesCmd;	// use this when r_lockSurfaces = 1

		viewEntity_t			identitySpace;		// can use if we don't know viewDef->worldSpace is valid
		FILE 					*logFile;			// for logging GL calls and frame breaks

		int						stencilIncr, stencilDecr;	// GL_INCR / INCR_WRAP_EXT, GL_DECR / GL_DECR_EXT

		renderCrop_t			renderCrops[MAX_RENDER_CROPS];
		int						currentRenderCrop;

		// GUI drawing variables for surface creation
		int						guiRecursionLevel;		// to prevent infinite overruns
		class idGuiModel 		*guiModel;
		class idGuiModel 		*demoGuiModel;

		unsigned short			gammaTable[256];	// brightness / gamma modify this
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
#ifdef _NO_LIGHT
extern idCVar r_noLight;				// no lighting
#endif
extern idCVar r_useETC1;				// ETC1 compression
extern idCVar r_useETC1Cache;				// use ETC1 cache
extern idCVar r_useDXT;					// DXT compression
#ifdef _OPENGLES3
extern idCVar r_useETC2;				// ETC2 compression
#endif

extern idCVar r_finish;					// force a call to glFinish() every frame
extern idCVar r_frontBuffer;			// draw to front buffer for debugging
extern idCVar r_swapInterval;			// changes swap interval
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

extern idCVar r_renderer;

extern idCVar r_checkBounds;			// compare all surface bounds with precalculated ones

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
extern idCVar r_useShadowProjectedCull;	// 1 = discard triangles outside light volume before shadowing
extern idCVar r_useDeferredTangents;	// 1 = don't always calc tangents after deform
extern idCVar r_useCachedDynamicModels;	// 1 = cache snapshots of dynamic models
extern idCVar r_useInfiniteFarZ;		// 1 = use the no-far-clip-plane trick
extern idCVar r_useScissor;				// 1 = scissor clip as portals and lights are processed
extern idCVar r_usePortals;				// 1 = use portals to perform area culling, otherwise draw everything
extern idCVar r_useStateCaching;		// avoid redundant state changes in GL_*() calls
extern idCVar r_useCombinerDisplayLists;// if 1, put all nvidia register combiner programming in display lists
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

extern idCVar harm_r_maxFps;
extern idCVar harm_r_shadowCarmackInverse;

extern idCVar r_screenshotFormat;
extern idCVar r_screenshotJpgQuality;
extern idCVar r_screenshotPngCompression;

#ifdef _D3BFG_CULLING
extern idCVar harm_r_occlusionCulling;
extern idCVar r_useLightPortalCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 MVP to plane culling
extern idCVar r_useLightAreaCulling;		// 0 = off, 1 = on
extern idCVar r_useEntityPortalCulling;		// 0 = none, 1 = box
#endif

#ifdef _RAVEN
extern idCVar r_skipSky;
#endif

/*
====================================================================

GL wrapper/helper functions

====================================================================
*/

void	GL_SelectTexture(int unit);
void	GL_UseProgram(shaderProgram_s *program);
void	GL_Uniform1fv(GLint location, const GLfloat *value);
void	GL_Uniform2fv(GLint location, const GLfloat *value);
void	GL_Uniform3fv(GLint location, const GLfloat *value);
void	GL_Uniform4fv(GLint location, const GLfloat *value);
void 	GL_Uniform1i(GLint location, GLint w);
void	GL_Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void	GL_UniformMatrix4fv(GLint location, const GLfloat *value);
void 	GL_UniformMatrix4fv(GLint location, const GLsizei n, const GLfloat *value);
void	GL_Uniform1f(GLint location, GLfloat value);
void	GL_EnableVertexAttribArray(GLuint index);
void	GL_DisableVertexAttribArray(GLuint index);
void	GL_VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
void	GL_CheckErrors(void);
void	GL_ClearStateDelta(void);
void	GL_State(int stateVector);
void	GL_TexEnv(int env);
void	GL_Cull(int cullType);
bool    GL_CheckErrors(const char *name);
void	GL_SelectTextureForce(int unit);

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

#ifdef _RAVEN //k: quake4 blend
const int GLS_SRCBLEND_SRC_COLOR				= 0x00000002;
#endif

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

void R_Init(void);
void R_InitOpenGL(void);

void R_DoneFreeType(void);

void R_SetColorMappings(void);

void R_ScreenShot_f(const idCmdArgs &args);
void R_StencilShot(void);


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

bool		GLimp_Init(glimpParms_t parms);
// If the desired mode can't be set satisfactorily, false will be returned.
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.

bool		GLimp_SetScreenParms(glimpParms_t parms);
// will set up gl up with the new parms

void		GLimp_Shutdown(void);
// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.

void		GLimp_SwapBuffers(void);
// Calls the system specific swapbuffers routine, and may also perform
// other system specific cvar checks that happen every frame.
// This will not be called if 'r_drawBuffer GL_FRONT'

void		GLimp_SetGamma(unsigned short red[256],
                               unsigned short green[256],
                               unsigned short blue[256]);
// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision


bool		GLimp_SpawnRenderThread(void (*function)(void));
// Returns false if the system only has a single processor

void 		*GLimp_BackEndSleep(void);
void		GLimp_FrontEndSleep(void);
void		GLimp_WakeBackEnd(void *data);
// these functions implement the dual processor syncronization

void		GLimp_ActivateContext(void);
void		GLimp_DeactivateContext(void);
// These are used for managing SMP handoffs of the OpenGL context
// between threads, and as a performance tunining aid.  Setting
// 'r_skipRenderContext 1' will call GLimp_DeactivateContext() before
// the 3D rendering code, and GLimp_ActivateContext() afterwards.  On
// most OpenGL implementations, this will result in all OpenGL calls
// being immediate returns, which lets us guage how much time is
// being spent inside OpenGL.

void		GLimp_EnableLogging(bool enable);
#ifdef _IMGUI
#include "imgui/r_imgui.h"
#endif


/*
====================================================================

MAIN

====================================================================
*/

void R_RenderView(viewDef_t *parms);

// performs radius cull first, then corner cull
bool R_CullLocalBox(const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes);
bool R_RadiusCullLocalBox(const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes);
bool R_CornerCullLocalBox(const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes);

void R_AxisToModelMatrix(const idMat3 &axis, const idVec3 &origin, float modelMatrix[16]);

// note that many of these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);
void R_GlobalVectorToLocal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);
void R_GlobalPlaneToLocal(const float modelMatrix[16], const idPlane &in, idPlane &out);
void R_PointTimesMatrix(const float modelMatrix[16], const idVec4 &in, idVec4 &out);
void R_LocalPointToGlobal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);
void R_LocalVectorToGlobal(const float modelMatrix[16], const idVec3 &in, idVec3 &out);
void R_LocalPlaneToGlobal(const float modelMatrix[16], const idPlane &in, idPlane &out);
void R_TransformEyeZToWin(float src_z, const float *projectionMatrix, float &dst_z);

void R_GlobalToNormalizedDeviceCoordinates(const idVec3 &global, idVec3 &ndc);

void R_TransformModelToClip(const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst);

void R_TransformClipToDevice(const idPlane &clip, const viewDef_t *view, idVec3 &normalized);

void R_TransposeGLMatrix(const float in[16], float out[16]);
void R_TransposeGLMatrix(float out[16]);

void R_SetViewMatrix(viewDef_t *viewDef);

void myGlMultMatrix(const float *a, const float *b, float *out);
void RB_ComputeMVP( const drawSurf_t * const surf, float mvp[16] );

/*
============================================================

LIGHT

============================================================
*/

void R_ListRenderLightDefs_f(const idCmdArgs &args);
void R_ListRenderEntityDefs_f(const idCmdArgs &args);

bool R_IssueEntityDefCallback(idRenderEntityLocal *def);
idRenderModel *R_EntityDefDynamicModel(idRenderEntityLocal *def);

viewEntity_t *R_SetEntityDefViewEntity(idRenderEntityLocal *def);
viewLight_t *R_SetLightDefViewLight(idRenderLightLocal *def);

void R_AddDrawSurf(const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity,
                   const idMaterial *shader, const idScreenRect &scissor);

void R_LinkLightSurf(const drawSurf_t **link, const srfTriangles_t *tri, const viewEntity_t *space,
                     const idRenderLightLocal *light, const idMaterial *shader, const idScreenRect &scissor, bool viewInsideShadow);

bool R_CreateAmbientCache(srfTriangles_t *tri, bool needsLighting);
void R_CreatePrivateShadowCache(srfTriangles_t *tri);
void R_CreateVertexProgramShadowCache(srfTriangles_t *tri);

/*
============================================================

LIGHTRUN

============================================================
*/

void R_RegenerateWorld_f(const idCmdArgs &args);

void R_ModulateLights_f(const idCmdArgs &args);

void R_SetLightProject(idPlane lightProject[4], const idVec3 origin, const idVec3 targetPoint,
                       const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop);

#ifdef _RAVEN // particle
void R_AddEffectSurfaces(void);
#endif

void R_AddLightSurfaces(void);
void R_AddModelSurfaces(void);
void R_RemoveUnecessaryViewLights(void);

void R_FreeDerivedData(void);
void R_ReCreateWorldReferences(void);

void R_CreateEntityRefs(idRenderEntityLocal *def);
void R_CreateLightRefs(idRenderLightLocal *light);

void R_DeriveLightData(idRenderLightLocal *light);
void R_FreeLightDefDerivedData(idRenderLightLocal *light);
void R_CheckForEntityDefsUsingModel(idRenderModel *model);

void R_ClearEntityDefDynamicModel(idRenderEntityLocal *def);
void R_FreeEntityDefDerivedData(idRenderEntityLocal *def, bool keepDecals, bool keepCachedDynamicModel);
void R_FreeEntityDefCachedDynamicModel(idRenderEntityLocal *def);
void R_FreeEntityDefDecals(idRenderEntityLocal *def);
void R_FreeEntityDefOverlay(idRenderEntityLocal *def);
void R_FreeEntityDefFadedDecals(idRenderEntityLocal *def, int time);

void R_CreateLightDefFogPortals(idRenderLightLocal *ldef);
#ifdef _D3BFG_CULLING
void R_DeriveEntityData( idRenderEntityLocal *def );
#endif

/*
============================================================

POLYTOPE

============================================================
*/

srfTriangles_t *R_PolytopeSurface(int numPlanes, const idPlane *planes, idWinding **windings);

/*
============================================================

RENDER

============================================================
*/

void RB_EnterWeaponDepthHack(const drawSurf_t *surf);
void RB_EnterModelDepthHack(const drawSurf_t *surf);
void RB_LeaveDepthHack(const drawSurf_t *surf);
void RB_DrawElementsImmediate(const srfTriangles_t *tri);
void RB_RenderTriangleSurface(const srfTriangles_t *tri);
void RB_T_RenderTriangleSurface(const drawSurf_t *surf);
void RB_RenderDrawSurfListWithFunction(drawSurf_t **drawSurfs, int numDrawSurfs,
                                       void (*triFunc_)(const drawSurf_t *));
void RB_RenderDrawSurfChainWithFunction(const drawSurf_t *drawSurfs,
                                        void (*triFunc_)(const drawSurf_t *));
void RB_DrawShaderPasses(drawSurf_t **drawSurfs, int numDrawSurfs);
void RB_LoadShaderTextureMatrix(const float *shaderRegisters, const textureStage_t *texture);
void RB_LoadShaderTextureMatrix(const float *shaderRegisters, const textureStage_t *texture, bool transpose);
void RB_GetShaderTextureMatrix(const float *shaderRegisters, const textureStage_t *texture, float matrix[16]);
void RB_CreateSingleDrawInteractions(const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *));

const shaderStage_t *RB_SetLightTexture(const idRenderLightLocal *light);

void RB_DrawView(const void *data);

void RB_DetermineLightScale(void);
void RB_STD_LightScale(void);
void RB_BeginDrawingView(void);

/*
============================================================

DRAW_STANDARD

============================================================
*/

void RB_DrawElementsWithCounters(const srfTriangles_t *tri);
void RB_DrawShadowElementsWithCounters(const srfTriangles_t *tri, int numIndexes);
void RB_STD_FillDepthBuffer(drawSurf_t **drawSurfs, int numDrawSurfs);
void RB_BindVariableStageImage(const textureStage_t *texture, const float *shaderRegisters);
void RB_FinishStageTexture(const textureStage_t *texture, const drawSurf_t *surf);
void RB_StencilShadowPass(const drawSurf_t *drawSurfs);
void RB_STD_DrawView(void);
void RB_STD_FogAllLights(void);
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float textureMatrix[16] );
void RB_PrepareStageTexturing(const shaderStage_t *pStage,  const drawSurf_t *surf, idDrawVert *ac);
void RB_FinishStageTexturing(const shaderStage_t *pStage, const drawSurf_t *surf, idDrawVert *ac);

/*
============================================================

DRAW_*

============================================================
*/

#ifndef GL_ES_VERSION_2_0
void	R_ReloadARBPrograms_f(const idCmdArgs &args);
int		R_FindARBProgram(GLenum target, const char *program);
#endif

typedef enum {
	PROG_INVALID,
	VPROG_INTERACTION,
	VPROG_ENVIRONMENT,
	VPROG_BUMPY_ENVIRONMENT,
	VPROG_STENCIL_SHADOW,
	VPROG_TEST,
	FPROG_INTERACTION,
	FPROG_ENVIRONMENT,
	FPROG_BUMPY_ENVIRONMENT,
	FPROG_TEST,
	VPROG_AMBIENT,
	FPROG_AMBIENT,
	VPROG_GLASSWARP,
	FPROG_GLASSWARP,
	PROG_USER
} program_t;

typedef enum {
	// base
	SHADER_INTERACTION = 0,
	SHADER_SHADOW,
	SHADER_DEFAULT,
	SHADER_ZFILL,
	SHADER_ZFILLCLIP,
	SHADER_CUBEMAP,
	SHADER_ENVIRONMENT,
    SHADER_BUMPY_ENVIRONMENT,
	SHADER_FOG,
	SHADER_BLENDLIGHT,
	SHADER_INTERACTION_PBR,
	SHADER_INTERACTION_BLINNPHONG,
    SHADER_AMBIENT_LIGHTING,
#ifdef _GLOBAL_ILLUMINATION
    SHADER_GLOBAL_ILLUMINATION,
#endif
	SHADER_DIFFUSECUBEMAP,
	// SHADER_GLASSWARP,
	SHADER_TEXGEN,
	// new stage
	SHADER_HEATHAZE,
	SHADER_HEATHAZE_WITH_MASK,
	SHADER_HEATHAZE_WITH_MASK_AND_VERTEX,
	SHADER_COLORPROCESS,
	// D3XP
    SHADER_ENVIROSUIT,
#ifdef _HUMANHEAD
	SHADER_SCREENEFFECT, // spiritview
	SHADER_RADIALBLUR, // deathview
	SHADER_LIQUID, // liquid
    SHADER_MEMBRANE, // membrane
	SHADER_SCREENPROCESS, // unused
#endif
    SHADER_MEGATEXTURE,
	// shadow mapping
#ifdef _SHADOW_MAPPING
	SHADER_DEPTH,
    SHADER_DEPTH_PERFORATED,
	SHADER_DEPTH_COLOR, // OpenGLES2.0 only
	SHADER_DEPTH_PERFORATED_COLOR, // OpenGLES2.0 only
	SHADER_INTERACTION_POINT_LIGHT,
	SHADER_INTERACTION_PBR_POINT_LIGHT,
	SHADER_INTERACTION_BLINNPHONG_POINT_LIGHT,
    SHADER_AMBIENT_LIGHTING_POINT_LIGHT,
	SHADER_INTERACTION_PARALLEL_LIGHT,
	SHADER_INTERACTION_PBR_PARALLEL_LIGHT,
	SHADER_INTERACTION_BLINNPHONG_PARALLEL_LIGHT,
    SHADER_AMBIENT_LIGHTING_PARALLEL_LIGHT,
	SHADER_INTERACTION_SPOT_LIGHT,
	SHADER_INTERACTION_PBR_SPOT_LIGHT,
	SHADER_INTERACTION_BLINNPHONG_SPOT_LIGHT,
    SHADER_AMBIENT_LIGHTING_SPOT_LIGHT,
#endif
	// translucent stencil shadow
#ifdef _STENCIL_SHADOW_IMPROVE
	SHADER_INTERACTION_TRANSLUCENT,
	SHADER_INTERACTION_PBR_TRANSLUCENT,
	SHADER_INTERACTION_BLINNPHONG_TRANSLUCENT,
    SHADER_AMBIENT_LIGHTING_TRANSLUCENT,
#ifdef _SOFT_STENCIL_SHADOW
	SHADER_INTERACTION_SOFT,
	SHADER_INTERACTION_PBR_SOFT,
	SHADER_INTERACTION_BLINNPHONG_SOFT,
    SHADER_AMBIENT_LIGHTING_SOFT,
#endif
#endif
    // post process
#ifdef _POSTPROCESS
    SHADER_RETRO_2BIT,
    SHADER_RETRO_C64,
    SHADER_RETRO_CPC,
    SHADER_RETRO_GENESIS,
    SHADER_RETRO_PSX,
#endif
    // costum
	SHADER_CUSTOM,
} glsl_program_t;

#define SHADER_BASE_BEGIN SHADER_INTERACTION
#define SHADER_BASE_END SHADER_TEXGEN

#define SHADER_NEW_STAGE_BEGIN SHADER_HEATHAZE
#define SHADER_NEW_STAGE_END SHADER_MEGATEXTURE

#ifdef _SHADOW_MAPPING
#define SHADER_SHADOW_MAPPING_BEGIN SHADER_DEPTH
#define SHADER_SHADOW_MAPPING_END SHADER_AMBIENT_LIGHTING_SPOT_LIGHT
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
#ifdef _SOFT_STENCIL_SHADOW
#define SHADER_STENCIL_SHADOW_BEGIN SHADER_INTERACTION_TRANSLUCENT
#define SHADER_STENCIL_SHADOW_END SHADER_AMBIENT_LIGHTING_SOFT
#else
#define SHADER_STENCIL_SHADOW_BEGIN SHADER_INTERACTION_TRANSLUCENT
#define SHADER_STENCIL_SHADOW_END SHADER_INTERACTION_BLINNPHONG_TRANSLUCENT
#endif
#endif

#ifdef _POSTPROCESS
#define SHADER_POSTPROCESS_BEGIN SHADER_RETRO_2BIT
#define SHADER_POSTPROCESS_END SHADER_RETRO_PSX
#endif

/*

  All vertex programs use the same constant register layout:

c[4]	localLightOrigin
c[5]	localViewOrigin
c[6]	lightProjection S
c[7]	lightProjection T
c[8]	lightProjection Q
c[9]	lightFalloff	S
c[10]	bumpMatrix S
c[11]	bumpMatrix T
c[12]	diffuseMatrix S
c[13]	diffuseMatrix T
c[14]	specularMatrix S
c[15]	specularMatrix T


c[20]	light falloff tq constant

// texture 0 was cube map
// texture 1 will be the per-surface bump map
// texture 2 will be the light falloff texture
// texture 3 will be the light projection texture
// texture 4 is the per-surface diffuse map
// texture 5 is the per-surface specular map
// texture 6 is the specular half angle cube map

*/

typedef enum {
	PP_LIGHT_ORIGIN = 4,
	PP_VIEW_ORIGIN,
	PP_LIGHT_PROJECT_S,
	PP_LIGHT_PROJECT_T,
	PP_LIGHT_PROJECT_Q,
	PP_LIGHT_FALLOFF_S,
	PP_BUMP_MATRIX_S,
	PP_BUMP_MATRIX_T,
	PP_DIFFUSE_MATRIX_S,
	PP_DIFFUSE_MATRIX_T,
	PP_SPECULAR_MATRIX_S,
	PP_SPECULAR_MATRIX_T,
	PP_COLOR_MODULATE,
	PP_COLOR_ADD,

	PP_LIGHT_FALLOFF_TQ = 20	// only for NV programs
} programParameter_t;


/*
============================================================

DRAW_GLSL

============================================================
*/

#define MAX_UNIFORM_PARMS 8
#define MAX_MEGATEXTURE_PARMS 9
#define HARM_SHADER_NAME_LENGTH 64
//#define _HARM_SHADER_NAME
typedef struct shaderProgram_s {
	GLuint		program;

	GLuint		vertexShader;
	GLuint		fragmentShader;

	GLint		glColor;
	GLint		alphaTest;
	GLint		specularExponent;

	GLint		modelViewProjectionMatrix;
	GLint		modelMatrix;
	GLint		textureMatrix;
	GLint		modelViewMatrix;
	GLint		projectionMatrix;
	GLint		clipPlane;
	GLint		fogMatrix;
	GLint		fogColor;

	GLint		windowCoords;
	GLint		eyeOrigin;
	GLint		localEyeOrigin;
	GLint		localLightOrigin;
	GLint		localViewOrigin;

	GLint		lightProjectionS;
	GLint		lightProjectionT;
	GLint		lightProjectionQ;
	GLint		lightFalloff;

	GLint		bumpMatrixS;
	GLint		bumpMatrixT;
	GLint		diffuseMatrixS;
	GLint		diffuseMatrixT;
	GLint		specularMatrixS;
	GLint		specularMatrixT;

	GLint		colorModulate;
	GLint		colorAdd;

	GLint		diffuseColor;
	GLint		specularColor;

	/* gl_... */
	GLint		attr_TexCoord;
	GLint		attr_Tangent;
	GLint		attr_Bitangent;
	GLint		attr_Normal;
	GLint		attr_Vertex;
	GLint		attr_Color;

	GLint		nonPowerOfTwo;

	GLint		u_fragmentMap[MAX_FRAGMENT_IMAGES];
	//k: cubemap texture units
	GLint		u_fragmentCubeMap[MAX_FRAGMENT_IMAGES];
	GLint		u_vertexParm[MAX_VERTEX_PARMS];
#if defined(_GLSL_PROGRAM) || defined(_RAVEN) || defined(_HUMANHEAD) //karin: fragment shader parms
	GLint		u_fragmentParm[MAX_FRAGMENT_PARMS];
#endif
	GLint		texgenS;
	GLint		texgenT;
	GLint		texgenQ;
	GLint		u_uniformParm[MAX_UNIFORM_PARMS];

    GLint       u_megaTextureLevel[MAX_MEGATEXTURE_PARMS];

#ifdef _SHADOW_MAPPING
	GLint		shadowMVPMatrix;
	GLint		globalLightOrigin;
#endif

	char 		name[HARM_SHADER_NAME_LENGTH];
    int         type; // glsl_program_t
} shaderProgram_t;
#define SHADER_PARM_ADDR(prop) offsetof(shaderProgram_t, prop)
#define SHADER_PARM_LOCATION(location) (*(GLint *)((char *)backEnd.glState.currentProgram + location))
#define SHADER_PARMS_ADDR(prop, i) ((GLint)(offsetof(shaderProgram_t, prop)) + i * (GLint)sizeof(GLuint))

struct GLSLShaderProp
{
	idStr name;
	shaderProgram_t *program;
	idStr default_vertex_shader_source;
	idStr default_fragment_shader_source;
	idStr macros;
	idStr vertex_shader_source_file;
	idStr fragment_shader_source_file;
    int type; // glsl_program_t

	GLSLShaderProp()
			: program(NULL),
            type(SHADER_CUSTOM)
	{}

	GLSLShaderProp(const char *name)
			: name(name),
			  program(NULL),
              type(SHADER_CUSTOM)
	{
		vertex_shader_source_file = name;
		vertex_shader_source_file += ".vert";
		fragment_shader_source_file = name;
		fragment_shader_source_file += ".frag";
	}

	GLSLShaderProp(const char *name, int type, shaderProgram_t *program, const idStr &vs, const idStr &fs, const idStr &macros)
			: name(name),
              type(type),
			  program(program),
			  default_vertex_shader_source(vs),
			  default_fragment_shader_source(fs),
			  macros(macros)
	{
		vertex_shader_source_file = name;
		vertex_shader_source_file += ".vert";
		fragment_shader_source_file = name;
		fragment_shader_source_file += ".frag";
	}
};

typedef int shaderHandle_t; // > 0 is internal shader(index = handle - 1), < 0 is custom shader(index = -handle - 1), = 0 is invalid
#define SHADER_HANDLE_IS_VALID(x) ( (x) != idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_INVALID(x) ( (x) == idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_INVALID (idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_BUILTIN(x) ( (x) > idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_CUSTOM(x) ( (x) < idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_MAX_CUSTOM 32
class idGLSLShaderManager
{
public:
	~idGLSLShaderManager();
	int Add(shaderProgram_t *shader); // return added shader's index
    void Shutdown(void);
	const shaderProgram_t * Find(const char *name) const;
	const shaderProgram_t * Find(GLuint handle) const; // handle is OpenGL shader program's handle
	shaderHandle_t Load(const GLSLShaderProp &prop); // frontend: if in multi-threading, only add on queue, because current thread has not OpenGL context; else if not in multi-threading, actual load directly. however always return a shader program handle, if has loaded, return OpenGL program handle(> 0), else return -(customShaders::index + 1), error return 0.
	void ActuallyLoad(void); // backend: if in multi-threading, load actually from queue with OpenGL context
	const shaderProgram_t * Get(shaderHandle_t handle) const;
	shaderHandle_t GetHandle(const char *name) const;
	void ReloadShaders(void);

	static idGLSLShaderManager _shaderManager;
	static const shaderHandle_t INVALID_SHADER_HANDLE;

private:
	int FindIndex(const char *name) const; // return raw index
	int FindIndex(GLuint handle) const; // return raw index
    int FindCustomIndex(const char *name) const; // return raw index
    GLSLShaderProp * FindCustom(const char *name, int *index = NULL);

private:
	idList<shaderProgram_t *> shaders; // available shaders, include internal shaders and loaded custom shaders
	idList<GLSLShaderProp> customShaders; // custom shaders load list. GLSLShaderProp::program == NULL: loading not start; GLSLShaderProp::program->program > 0: load success; GLSLShaderProp::program->program == 0: load failed
	// idList<unsigned int> queue; // custom shaders load queue: index to customShaders
	unsigned int queueCurrentIndex; // current loaded index in customShaders

private:
	idGLSLShaderManager() : queueCurrentIndex(0) {}
};
extern idGLSLShaderManager *shaderManager;


/* This file was automatically generated.  Do not edit! */
void R_ReloadGLSLPrograms_f(const idCmdArgs &args);
void R_GLSL_Init(void);
void R_GLSL_Shutdown(void);
void RB_GLSL_DrawInteractions(void);
void RB_GLSL_CreateDrawInteractions(const drawSurf_t *surf);
void RB_GLSL_DrawInteraction(const drawInteraction_t *din);
bool RB_GLSL_FindGLSLShaderSource(const char *name, int type, idStr *source, idStr *realPath);

void R_CheckBackEndCvars(void);

#define GLSL_PROGRAM_PROC extern
#include "glsl/glsl_program.h"
#undef GLSL_PROGRAM_PROC

/*
============================================================

TR_STENCILSHADOWS

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1

============================================================
*/

void R_MakeShadowFrustums(idRenderLightLocal *def);

typedef enum {
	SG_DYNAMIC,		// use infinite projections
	SG_STATIC,		// clip to bounds
	SG_OFFLINE		// perform very time consuming optimizations
} shadowGen_t;

srfTriangles_t *R_CreateShadowVolume(const idRenderEntityLocal *ent,
                                     const srfTriangles_t *tri, const idRenderLightLocal *light,
                                     shadowGen_t optimize, srfCullInfo_t &cullInfo);

/*
============================================================

TR_TURBOSHADOW

Fast, non-clipped overshoot shadow volumes

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1
calling this function may modify "facing" based on culling

============================================================
*/

srfTriangles_t *R_CreateVertexProgramTurboShadowVolume(const idRenderEntityLocal *ent,
                const srfTriangles_t *tri, const idRenderLightLocal *light,
                srfCullInfo_t &cullInfo);

/*
srfTriangles_t *R_CreateTurboShadowVolume(const idRenderEntityLocal *ent,
                const srfTriangles_t *tri, const idRenderLightLocal *light,
                srfCullInfo_t &cullInfo);
*/

/*
============================================================

util/shadowopt3

dmap time optimization of shadow volumes, called from R_CreateShadowVolume

============================================================
*/


typedef struct {
	idVec3	*verts;			// includes both front and back projections, caller should free
	int		numVerts;
	glIndex_t	*indexes;	// caller should free

	// indexes must be sorted frontCap, rearCap, silPlanes so the caps can be removed
	// when the viewer is in a position that they don't need to see them
	int		numFrontCapIndexes;
	int		numRearCapIndexes;
	int		numSilPlaneIndexes;
	int		totalIndexes;
} optimizedShadow_t;

optimizedShadow_t SuperOptimizeOccluders(idVec4 *verts, glIndex_t *indexes, int numIndexes,
                idPlane projectionPlane, idVec3 projectionOrigin);

void CleanupOptimizedShadowTris(srfTriangles_t *tri);

/*
============================================================

TRISURF

============================================================
*/

#define USE_TRI_DATA_ALLOCATOR

void				R_InitTriSurfData(void);
void				R_ShutdownTriSurfData(void);
void				R_PurgeTriSurfData(frameData_t *frame);
void				R_ShowTriSurfMemory_f(const idCmdArgs &args);

srfTriangles_t 	*R_AllocStaticTriSurf(void);
srfTriangles_t 	*R_CopyStaticTriSurf(const srfTriangles_t *tri);
void				R_AllocStaticTriSurfVerts(srfTriangles_t *tri, int numVerts);
void				R_AllocStaticTriSurfIndexes(srfTriangles_t *tri, int numIndexes);
void				R_AllocStaticTriSurfShadowVerts(srfTriangles_t *tri, int numVerts);
void				R_AllocStaticTriSurfPlanes(srfTriangles_t *tri, int numIndexes);
void				R_ResizeStaticTriSurfVerts(srfTriangles_t *tri, int numVerts);
void				R_ResizeStaticTriSurfIndexes(srfTriangles_t *tri, int numIndexes);
void				R_ResizeStaticTriSurfShadowVerts(srfTriangles_t *tri, int numVerts);
void				R_ReferenceStaticTriSurfVerts(srfTriangles_t *tri, const srfTriangles_t *reference);
void				R_ReferenceStaticTriSurfIndexes(srfTriangles_t *tri, const srfTriangles_t *reference);
void				R_FreeStaticTriSurfSilIndexes(srfTriangles_t *tri);
void				R_FreeStaticTriSurf(srfTriangles_t *tri);
void				R_FreeStaticTriSurfVertexCaches(srfTriangles_t *tri);
void				R_ReallyFreeStaticTriSurf(srfTriangles_t *tri);
void				R_FreeDeferredTriSurfs(frameData_t *frame);
int					R_TriSurfMemory(const srfTriangles_t *tri);

void				R_BoundTriSurf(srfTriangles_t *tri);
void				R_RemoveDuplicatedTriangles(srfTriangles_t *tri);
void				R_CreateSilIndexes(srfTriangles_t *tri);
void				R_RemoveDegenerateTriangles(srfTriangles_t *tri);
void				R_RemoveUnusedVerts(srfTriangles_t *tri);
void				R_RangeCheckIndexes(const srfTriangles_t *tri);
void				R_CreateVertexNormals(srfTriangles_t *tri);	// also called by dmap
void				R_DeriveFacePlanes(srfTriangles_t *tri);		// also called by renderbump
void				R_CleanupTriangles(srfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);
void				R_ReverseTriangles(srfTriangles_t *tri);

// Only deals with vertexes and indexes, not silhouettes, planes, etc.
// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
srfTriangles_t 	*R_MergeSurfaceList(const srfTriangles_t **surfaces, int numSurfaces);
srfTriangles_t 	*R_MergeTriangles(const srfTriangles_t *tri1, const srfTriangles_t *tri2);

// if the deformed verts have significant enough texture coordinate changes to reverse the texture
// polarity of a triangle, the tangents will be incorrect
void				R_DeriveTangents(srfTriangles_t *tri, bool allocFacePlanes = true);

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct deformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;

	int				numMirroredVerts;
	int 			*mirroredVerts;

	int				numIndexes;
	glIndex_t 		*indexes;

	glIndex_t 		*silIndexes;

	int				numDupVerts;
	int 			*dupVerts;

	int				numSilEdges;
	silEdge_t 		*silEdges;

	dominantTri_t 	*dominantTris;
} deformInfo_t;


deformInfo_t 		*R_BuildDeformInfo(int numVerts, const idDrawVert *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents);
void				R_FreeDeformInfo(deformInfo_t *deformInfo);
int					R_DeformInfoMemoryUsed(deformInfo_t *deformInfo);

/*
============================================================

SUBVIEW

============================================================
*/

bool	R_PreciseCullSurface(const drawSurf_t *drawSurf, idBounds &ndcBounds);
bool	R_GenerateSubViews(void);

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitFrameData(void);
void R_ShutdownFrameData(void);
int R_CountFrameData(void);
void R_ToggleSmpFrame(void);
void *R_FrameAlloc(int bytes);
void *R_ClearedFrameAlloc(int bytes);
void R_FrameFree(void *data);

void *R_StaticAlloc(int bytes);		// just malloc with error checking
void *R_ClearedStaticAlloc(int bytes);	// with memset
void R_StaticFree(void *data);


/*
=============================================================

RENDERER DEBUG TOOLS

=============================================================
*/

float RB_DrawTextLength(const char *text, float scale, int len);
void RB_AddDebugText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align, const int lifetime, const bool depthTest);
void RB_ClearDebugText(int time);
void RB_AddDebugLine(const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifeTime, const bool depthTest);
void RB_ClearDebugLines(int time);
void RB_AddDebugPolygon(const idVec4 &color, const idWinding &winding, const int lifeTime, const bool depthTest);
void RB_ClearDebugPolygons(int time);
void RB_DrawBounds(const idBounds &bounds);
void RB_ShowLights(drawSurf_t **drawSurfs, int numDrawSurfs);
void RB_ShowLightCount(drawSurf_t **drawSurfs, int numDrawSurfs);
void RB_PolygonClear(void);
void RB_ScanStencilBuffer(void);
void RB_ShowDestinationAlpha(void);
void RB_ShowOverdraw(void);
void RB_RenderDebugTools(drawSurf_t **drawSurfs, int numDrawSurfs);
void RB_ShutdownDebugTools(void);

/*
=============================================================

TR_BACKEND

=============================================================
*/

void RB_SetDefaultGLState(void);
void RB_SetGL2D(void);

// write a comment to the r_logFile if it is enabled
#ifdef _RENDERER_LOG_COMMENT
void RB_LogComment(const char *comment, ...) id_attribute((format(printf,1,2)));
#else
#define RB_LogComment(...)
#endif

void RB_ShowImages(void);

void RB_ExecuteBackEndCommands(const emptyCommand_t *cmds);

void LoadJPG_stb(const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp);
void LoadPNG(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp);
void LoadDDS(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp);
void LoadBimage(const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp);

void R_WritePNG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical = false, int quality = 100, const char *basePath = NULL);
void R_WriteJPG(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical = false, int compression = 0, const char *basePath = NULL);
void R_WriteBMP(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical = false, const char *basePath = NULL);
void R_WriteDDS(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical, const char *basePath = NULL);

void R_WriteScreenshotImage(const char *filename, const byte *data, int width, int height, int comp, bool flipVertical = false, const char *basePath = NULL);

/*
=============================================================

TR_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis(const srfTriangles_t *tri, idVec3 &origin, idVec3 axis[3]);
void R_RenderGuiSurf(idUserInterface *gui, drawSurf_t *drawSurf);

/*
=============================================================

TR_ORDERINDEXES

=============================================================
*/

void R_OrderIndexes(int numIndexes, glIndex_t *indexes);

/*
=============================================================

TR_DEFORM

=============================================================
*/

void R_DeformDrawSurf(drawSurf_t *drawSurf);

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

localTrace_t R_LocalTrace(const idVec3 &start, const idVec3 &end, const float radius, const srfTriangles_t *tri);
void RB_ShowTrace(drawSurf_t **drawSurfs, int numDrawSurfs);

/*
=============================================================

TR_SHADOWBOUNDS

=============================================================
*/
idScreenRect R_CalcIntersectionScissor(const idRenderLightLocal *lightDef,
                                       const idRenderEntityLocal *entityDef,
                                       const viewDef_t *viewDef);

//=============================================

#include "RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"

#ifdef _RAVEN //k: macros for renderEffect_s::suppressSurfaceMask
#define SUPPRESS_SURFACE_MASK(x) (1 << (x))
#define SUPPRESS_SURFACE_MASK_CHECK(t, x) ((t) & SUPPRESS_SURFACE_MASK(x))
#include "../raven/renderer/NewShaderStage.h"
#endif

extern bool GLimp_CheckGLInitialized(void); // Check GL context initialized, only for Android
//extern volatile bool has_gl_context;
extern unsigned int lastRenderTime;
extern int r_maxFps;

#if defined(_SHADOW_MAPPING) || defined(_D3BFG_CULLING)
float R_ComputePointLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject );
float R_ComputeSpotLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject );
float R_ComputeParallelLightProjectionMatrix( idRenderLightLocal* light, idRenderMatrix& localProject );

void R_SetupFrontEndViewDefMVP(void);
void R_SetupFrontEndFrustums(void);
void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds );
#endif

#ifdef _SHADOW_MAPPING

#define ALLOW_SHADOW_MAPPING_ON_ALPHATEST_MTR(x) ( ( (shader->GetContentFlags() & CONTENTS_SOLID) != 0 || ( (shader->GetCullType() == CT_TWO_SIDED) || shader->ShouldCreateBackSides() ) ) && (shader->GetSort() != SS_DECAL) )

#include "matrix/GLMatrix.h"

extern idCVar r_useShadowMapping;			// use shadow mapping instead of stencil shadows
extern idCVar r_useHalfLambertLighting;		// use Half-Lambert lighting instead of classic Lambert

extern idCVar r_showShadowMaps;
extern idCVar r_showShadowMapLODs;

extern idCVar r_shadowMapFrustumFOV;
extern idCVar r_shadowMapSingleSide;
extern idCVar r_shadowMapImageSize;
extern idCVar r_shadowMapJitterScale;
extern idCVar r_shadowMapBiasScale;
extern idCVar r_shadowMapRandomizeJitter;
extern idCVar r_shadowMapSamples;
extern idCVar r_shadowMapSplits;
extern idCVar r_shadowMapSplitWeight;
extern idCVar r_shadowMapLodScale;
extern idCVar r_shadowMapLodBias;
extern idCVar r_shadowMapPolygonFactor;
extern idCVar r_shadowMapPolygonOffset;
extern idCVar r_shadowMapOccluderFacing;
extern idCVar r_forceShadowMapsOnAlphaTestedSurfaces;

extern idCVar harm_r_shadowMapLod;
extern idCVar harm_r_shadowMapBias;
extern idCVar harm_r_shadowMapAlpha;
extern idCVar harm_r_shadowMapCombine;
#if 0
extern idCVar harm_r_shadowMapSampleFactor;
extern idCVar harm_r_shadowMapFrustumNear;
extern idCVar harm_r_shadowMapFrustumFar;
#endif
extern idCVar harm_r_useLightScissors;
extern idCVar harm_r_shadowMapDepthBuffer;
extern idCVar harm_r_shadowMapNonParallelLightUltra;

extern idBounds bounds_zeroOneCube;
extern idBounds bounds_unitCube;

void R_SetupShadowMappingLOD(const idRenderLightLocal *light, viewLight_t *vLight);
void R_SetupShadowMappingProjectionMatrix(idRenderLightLocal *light);
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
extern idCVar harm_r_stencilShadowTranslucent;
extern idCVar harm_r_stencilShadowAlpha;

#ifdef _SOFT_STENCIL_SHADOW
extern idCVar harm_r_stencilShadowSoft;
extern idCVar harm_r_stencilShadowSoftBias;
extern idCVar harm_r_stencilShadowSoftCopyStencilBuffer;
#endif
#endif

#ifdef _NO_GAMMA //karin: r_brightness when unsupport gamma
extern float RB_overbright;
#endif

#ifdef _K_DEV
#define HARM_CHECK_SHADER(x) \
	if (!backEnd.glState.currentProgram) { \
		common->Printf(x ": no current program object\n"); \
        Sys_Trap(); \
		return; \
	}

#ifdef _HARM_SHADER_NAME
#define HARM_CHECK_SHADER_ATTR(x, index) \
	if ((*(GLint *)((char *)backEnd.glState.currentProgram + index)) == -1) { \
		common->Printf(x ": unbound attribute index\n"); \
		RB_LogComment("Current shader program: %s, index: %d\n", backEnd.glState.currentProgram->name, index); \
        Sys_Trap(); \
		return; \
	}
#else
#define HARM_CHECK_SHADER_ATTR(x, index) \
	if ((*(GLint *)((char *)backEnd.glState.currentProgram + index)) == -1) { \
		common->Printf(x ": unbound attribute index\n"); \
        Sys_Trap(); \
		return; \
	}
#endif

#define HARM_CHECK_SHADER_ERROR(x) GL_CheckErrors(x);

#else
#define HARM_CHECK_SHADER(x)
#define HARM_CHECK_SHADER_ATTR(x, index)
#define HARM_CHECK_SHADER_ERROR(x)
#endif

#ifdef _EXTRAS_TOOLS
void ModelTest_RenderFrame(int time);
void ModelLight_RenderFrame(int time);
#endif

extern idCVar harm_r_lightingModel;
#define r_interactionLightingModel harm_r_lightingModel.GetInteger()
#define HARM_INTERACTION_SHADER_NOLIGHTING 0
#define HARM_INTERACTION_SHADER_PHONG 1
#define HARM_INTERACTION_SHADER_BLINNPHONG 2
#define HARM_INTERACTION_SHADER_PBR 3
#define HARM_INTERACTION_SHADER_AMBIENT 4

extern idCVar harm_r_useHighPrecision;

#define GL_VERSION_GL_ES2 0x00020000
#define GL_VERSION_GL_ES3 0x00030000
#define GL_VERSION_GL_ES31 0x00030001
#define GL_VERSION_GL_ES32 0x00030002
#if !defined(__ANDROID__)
#define GL_VERSION_GL_CORE 0x10000000
#define GL_VERSION_GL_COMPATIBILITY 0x20000000
#endif

#define GL_VERSION_NAME_GL_ES2 "GLES2"
#define GL_VERSION_NAME_GL_ES3 "GLES3.0"
#if !defined(__ANDROID__)
#define GL_VERSION_NAME_GL_CORE "OpenGL_core"
#define GL_VERSION_NAME_GL_COMPATIBILITY "OpenGL_compatibility"
#endif

#ifdef _GLOBAL_ILLUMINATION
extern idCVar harm_r_globalIllumination;
extern idCVar harm_r_globalIlluminationBrightness;
#define HARM_RENDER_GLOBAL_ILLUMINATION() (harm_r_globalIllumination.GetBool() && harm_r_globalIlluminationBrightness.GetFloat() > 0.0f)

void RB_DrawGlobalIlluminations( drawSurf_t **drawSurfs, int numDrawSurfs );
#endif

#ifdef _POSTPROCESS
extern idCVar r_renderMode;
void RB_PP_Render(void);
#endif

#ifdef _OPENGLES3
#if !defined(__ANDROID__)
#define DEFAULT_GLES_VERSION GL_VERSION_GL_CORE
#else
#define DEFAULT_GLES_VERSION GL_VERSION_GL_ES3
#endif
#else
#define DEFAULT_GLES_VERSION GL_VERSION_GL_ES2
#endif

extern idCVar harm_r_openglVersion;
extern int gl_version;

extern idCVar harm_r_debugOpenGL;
void RB_DebugOpenGL(void);

#endif /* !__TR_LOCAL_H__ */
