// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __RENDERWORLD_H__
#define __RENDERWORLD_H__

/*
===============================================================================

	Render World

===============================================================================
*/

#include "Material.h"
#include "../decllib/declDecal.h"

// shader parms
const int MAX_GLOBAL_SHADER_PARMS		= 4;	// Rarely used so lowered this from 12

const int SHADERPARM_RED				= 0;
const int SHADERPARM_GREEN				= 1;
const int SHADERPARM_BLUE				= 2;
const int SHADERPARM_ALPHA				= 3;
const int SHADERPARM_TIMESCALE			= 3;
const int SHADERPARM_TIMEOFFSET			= 4;
const int SHADERPARM_DIVERSITY			= 5;	// random between 0.0 and 1.0 for some effects (muzzle flashes, etc)
const int SHADERPARM_MODE				= 7;	// for selecting which shader passes to enable
const int SHADERPARM_TIME_OF_DEATH		= 7;	// for the monster skin-burn-away effect enable and time offset

// model parms
const int SHADERPARM_MD5_SKINSCALE		= 8;	// for scaling vertex offsets on md5 models (jack skellington effect)

const int SHADERPARM_MD3_FRAME			= 8;
const int SHADERPARM_MD3_LASTFRAME		= 9;
const int SHADERPARM_MD3_BACKLERP		= 10;

const int SHADERPARM_BEAM_END_X			= 8;	// for _beam models
const int SHADERPARM_BEAM_END_Y			= 9;
const int SHADERPARM_BEAM_END_Z			= 10;
const int SHADERPARM_BEAM_WIDTH			= 11;

const int SHADERPARM_SPRITE_WIDTH		= 8;	// for _sprite models
const int SHADERPARM_SPRITE_HEIGHT		= 9;
const int SHADERPARM_SPRITE_OFFSET		= 10;

const int SHADERPARM_FADE_ORIGIN_X		= 8;
const int SHADERPARM_FADE_ORIGIN_Y		= 9;
const int SHADERPARM_FADE_ORIGIN_Z		= 10;

const int SHADERPARM_PARTICLE_STOPTIME	= 8;	// don't spawn any more particles after this time

// guis
const int MAX_RENDERENTITY_GUI			= 3;

// RAVEN BEGIN
// jscott: for effect brightness
const int SHADERPARM_BRIGHTNESS			= 6;	// for the overall brightness of effects
// RAVEN END

struct srfTriangles_t;
struct renderEntity_t;
struct renderView_s;
class idRenderModel;
class sdDeclAmbientCubeMap;
class idDemoFile;
class sdUserInterface;

typedef bool( *deferredEntityCallback_t )( renderEntity_t*, const renderView_s*, int& lastModifiedGameTime );

const int MAX_SURFACE_BITS = 64;

const unsigned short MIRROR_VIEW_ID			= 0xFFFF;
const unsigned short FAST_MIRROR_VIEW_ID	= 0xFFFE;
const		   int	 WORLD_SPAWN_ID			= 0xFFFF0000;

enum sdUpdateBits {
	UDB_NONE				= 0,		// nothing to be updated
	UDB_PUSH				= BIT(0),	// PushPolytopeIntoTree needs to be called again, i.e. the areas this object touches could have changed
	UDB_INTERACTIONS		= BIT(1),	// Recalculate interactions for this object, i.e. the lights this object is affected by could have changed
	UDB_DECALS				= BIT(2),	// Decals on this surface could have changed and need to be recalculated
	UDB_CACHEDMODEL			= BIT(3),	// The model has changed and the old cached version is not useful anymore
	UDB_SURFACEMASK			= BIT(4),	// The surface mask is dirty
	UDB_IMPOSTERS			= BIT(5),	// The imposters changed
};

struct sdInstCommon {
	byte	color[4];
	idVec3	origin;
	idMat3	axis;
	union {
		float   dist; // overloaded
		float	maxVisDist;
	};
};


struct sdInstRT {
	sdInstCommon inst;
	float dist;
};

struct sdInstInfo {
	sdInstCommon inst;
	float	maxVisDist;
	float	minVisDist;
	idVec3	fadeOrigin;
};

struct renderEntity_t {
	idRenderModel*					hModel;				// this can only be null if callback is set

	int								spawnID;//entityNum;
	int								mapId;

	// Entities that are expensive to generate, like skeletal models, can be
	// deferred until their bounds are found to be in view, in the frustum
	// of a shadowing light that is in view, or contacted by a trace / overlay test.
	// This is also used to do visual cueing on items in the view
	// The renderView may be NULL if the callback is being issued for a non-view related
	// source.
	// The callback function should clear renderEntity->callback if it doesn't
	// want to be called again next time the entity is referenced (ie, if the
	// callback has now made the entity valid until the next updateEntity)
	idBounds						bounds;					// only needs to be set for deferred models and md5s
	deferredEntityCallback_t		callback;

	void *							callbackData;			// used for whatever the callback wants

	// player bodies and possibly player shadows should be suppressed in views from
	// that player's eyes, but will show up in mirrors and other subviews
	// security cameras could suppress their model in their subviews if we add a way
	// of specifying a view number for a remoteRenderMap view
	unsigned short					suppressSurfaceInViewID;
	unsigned short					suppressShadowInViewID;

	// world models for the player and weapons will not cast shadows from view weapon
	// muzzle flashes
	unsigned short					suppressShadowInLightID;

	// if non-zero, the surface and shadow (if it casts one)
	// will only show up in the specific view, ie: player weapons
	unsigned short					allowSurfaceInViewID;

	// if non-zero, the surface will act as a noSelfShadow in the specified view
	unsigned short					noSelfShadowInViewID;

	unsigned char					drawSpec;
	unsigned char					shadowSpec;

	// positioning
	// axis rotation vectors must be unit length for many
	// R_LocalToGlobal functions to work, so don't scale models!
	// axis vectors are [0] = forward, [1] = left, [2] = up
	idVec3							origin;
	idMat3							axis;

	// texturing
	const idMaterial*				customShader;			// if non-0, all surfaces will use this
	const idMaterial*				referenceShader;		// used so flares can reference the proper light shader
	const idDeclSkin*				customSkin;				// 0 for no remappings
	class idSoundEmitter*			referenceSound;			// for shader sound tables, allowing effects to vary with sounds
	float							shaderParms[ MAX_ENTITY_SHADER_PARMS ];	// can be used in any way by shader or model generation

	struct renderView_s*			remoteRenderView;		// any remote camera surfaces will use this

	int								numJoints;
	idJointMat *					joints;					// array of joints that will modify vertices.
															// NULL if non-deformable model.  NOT freed by renderer, unless jointsAllocated is set

	float							modelDepthHack;			// squash depth range so particle effects don't clip into walls
	bool							foliageDepthHack;

	float							sortOffset;				// Override material sort form render entity

	float							coverage;				// used when flag overridencoverage is specified

	short							minGpuSpec;
	short							numInsts;
	sdInstInfo *					insts;					//

	int								numVisDummies;
	idVec3							*dummies;

	int								numAreas;
	int	*							areas;

	struct renderEntityFlags_t {
		// options to override surface shader flags (replace with material parameters?)
		bool	noSelfShadow					: 1;	// cast shadows onto other objects,but not self
		bool	noShadow						: 1;	// no shadow at all

		bool	noDynamicInteractions			: 1;	// don't create any light / shadow interactions after
														// the level load is completed.  This is a performance hack
														// for the gigantic outdoor meshes in the monorail map, so
														// all the lights in the moving monorail don't touch the meshes

		bool	noHardwareSkinning				: 1;

		bool	weaponDepthHack					: 1;	// squash depth range so view weapons don't poke into walls
												// this automatically implies noShadow
		bool	forceUpdate						: 1;	// force an update (NOTE: not a bool to keep this struct a multiple of 4 bytes)
		bool	pushIntoConnectedOutsideAreas	: 1;	// forces the entityDef to be pushed in all connected outside areas
		bool	pushIntoOutsideAreas			: 1;	// forces the entityDef to be pushed in all connected outside areas

		bool	forceImposter					: 1;	// Never render the actual entity always the imposter
		bool	useFadeOrigin					: 1;	// Use an offset stored in the shaderparms for imposter fade calculations
		bool	pushByOrigin					: 1;	// Use origin in push
		bool	pushByCenter					: 1;	// Use center of bounds in push
		bool	pushByInstances					: 1;	// Use center of bounds in push

		bool	overridenBounds					: 1;

		bool	overridenCoverage				: 1;
		bool	occlusionTest					: 1;

		bool	foliageDepthHack				: 1;

		bool	dontCastFromAtmosLight			: 1;

		bool	usePointTestForAmbientCubeMaps	: 1;

		bool	disableLODs						: 1;

		bool	jointsAllocated					: 1;
	};
	float							weaponDepthHackFOV_x;
	float							weaponDepthHackFOV_y;
	renderEntityFlags_t				flags;

	float							shadowVisDistMult;		// max distance this entity will be drawn at
	int								maxVisDist;				// max distance this entity will be drawn at
	int								minVisDist;				// min distance this entity will be drawn at
	float							visDistFalloff;			// fraction of range spent fading in

	sdBitField< MAX_SURFACE_BITS - 1 >	hideSurfaceMask;		// mask to hide surfaces within a model

	const sdDeclImposter*			imposter;				// Imposter to use for entity (screen space based unless forceImposter is on)

	const sdDeclAmbientCubeMap*		ambientCubeMap;			// Override the ambient cubemap for this model (instead of the one specified by the area)

	int ApplyChanges( const renderEntity_t &other ); // Returns sdUpdateBits mask
};

const int MAX_LIGHT_AREAS = 8;
const int MAX_PRELIGHTS = 5;

struct renderLight_t {
	idMat3					axis;				// rotation vectors, must be unit length
	idVec3					origin;

	// if non-zero, the light will not show up in the specific view,
	// which may be used if we want to have slightly different muzzle
	// flash lights for the player and other views
	unsigned short			suppressLightInViewID;

	// if non-zero, the light will only show up in the specific view
	// which can allow player gun gui lights and such to not effect everyone
	unsigned short			allowLightInViewID;

	int						mapId;

	struct renderLightFlags_t {
		bool				noShadows		: 1;	// (should we replace this with material parameters on the shader?)
		bool				noSpecular		: 1;	// (should we replace this with material parameters on the shader?)

		bool				pointLight		: 1;	// otherwise a projection light (should probably invert the sense of this, because points are way more common)
		bool				parallel		: 1;	// lightCenter gives the direction to the light at infinity

		bool				atmosphereLight	: 1;	// special case atmosphere light, used for terrain lighting
		bool				insideLight		: 1;
	};
	renderLightFlags_t		flags;

	idVec3					lightRadius;		// xyz radius for point lights
	idVec3					lightCenter;		// offset the lighting direction for shading and
												// shadows, relative to origin

	// frustum definition for projected lights, all reletive to origin
	// FIXME: we should probably have real plane equations here, and offer
	// a helper function for conversion from this format
	idVec3					target;
	idVec3					right;
	idVec3					up;
	idVec3					start;
	idVec3					end;

	// Dmap will generate an optimized shadow volumes named _prelight_<lightName>
	// for the light against all the _area* models in the map.  The renderer will
	// ignore this value if the light has been moved after initial creation
	int						numPrelightModels;
	idRenderModel *			prelightModels[ MAX_PRELIGHTS ];

	// muzzle flash lights will not cast shadows from player and weapon world models
	int						lightId;

	int						maxVisDist;

	unsigned int			minSpecShadowColor;

	const idMaterial *		material;			// NULL = either lights/defaultPointLight or lights/defaultProjectedLight
	float					shaderParms[MAX_ENTITY_SHADER_PARMS];		// can be used in any way by shader
	idSoundEmitter *		referenceSound;		// for shader sound tables, allowing effects to vary with sounds

	int						manualPriority;		// if nonzero this light may be added to the manual lights list preferring higher priority ones
												// if there are too many lights ( which means it will affect particles etc...)

	int						drawSpec;
	int						shadowSpec;

	// light polytope doesn't get pushed down the tree, instead, only add to the specified areas
	int						numAreas;
	int						areas[ MAX_LIGHT_AREAS ];

	struct atmosLightProjection_t *atmosLightProjection;

	int						ApplyChanges( const renderLight_t &other ); // Returns sdUpdateBits mask
};

// RAVEN BEGIN
// jscott: for handling of effects
typedef struct renderEffect_s {

	const rvDeclEffect*		declEffect;

	unsigned short			suppressSurfaceInViewID;
	unsigned short			allowSurfaceInViewID;
	unsigned short			suppressLightsInViewID;
	int						groupID;

	idVec3					origin;
	idMat3					axis;

	idVec3					gravity;
	idVec3					endOrigin;
	idVec3					windVector;
	idVec3					materialColor;

	float					attenuation;
	bool					hasEndOrigin;
	bool					loop;						// effect is looping
	bool					useRenderBounds;
	bool					isStatic;
//	bool					ambient;					// effect is from an entity
	unsigned short			weaponDepthHackInViewID;	// squash depth range so view weapons don't poke into walls
	float					weaponDepthHackFOV_x;
	float					weaponDepthHackFOV_y;
	float					modelDepthHack;
	bool					foliageDepthHack;
	float					distanceOffset;

	float					maxVisDist;

	// THIS MUST BE THE LAST PARAMETER
	float					shaderParms[ MAX_ENTITY_SHADER_PARMS ];	// can be used in any way by shader or model generation

	int						ApplyChanges( const struct renderEffect_s &other ); // Returns sdUpdateBits mask
} renderEffect_t;
// RAVEN END

struct occlusionTest_t {
	idMat3		axis;
	idVec3		origin;
	idBounds	bb;
	int			view;
};

struct cheapDecalParameters_t {
	idVec3				origin;
	idVec3				normal;
	int					jointIdx;
	const sdDeclDecal*	decl;
};

typedef struct renderView_s {
	// player views will set this to a non-zero integer for model suppress / allow
	// subviews (mirrors, cameras, etc) will always clear it to zero
	unsigned short			viewID;

	// sized from 0 to SCREEN_WIDTH / SCREEN_HEIGHT (640/480), not actual resolution
	int						x, y, width, height;

	float					fov_x, fov_y;				// perspective
	float					size_x, size_y;				// orthographic

	idVec3					vieworg;
	idMat3					viewaxis;					// transformation matrix, view looks down the positive X axis
	idMat3					lastViewAxis;					// transformation matrix, view looks down the positive X axis
	idVec3					lastViewOrg;

	// time in milliseconds for shader effects and other time dependent rendering issues
	int						time;
	float					shaderParms[MAX_GLOBAL_SHADER_PARMS];	// can be used in any way by shader

	const idMaterial*		globalMaterial;							// used to override everything draw
	const idDeclSkin*		globalSkin;								// apply this skin to everything

	idVec4					clearColor;
	float					foliageDepthHack;

	float					nearPlane;
	float					farPlane;
	int						forceClear;

	struct renderViewFlags_t {
		bool				cramZNear			: 1;	// for cinematics, we want to set ZNear much lower
		bool				forceUpdate			: 1;	// for an update
		bool				forceViewIDOnly		: 1;	// only render entities with a matching viewID
		bool				forceDefsVisible	: 1;
	};
	renderViewFlags_t		flags;
} renderView_t;


// exitPortal_t is returned by idRenderWorld::GetPortal()
typedef struct {
	int						areas[2];			// areas connected by this portal
	const idWinding	*		w;					// winding points have counter clockwise ordering seen from areas[0]
	idPlane					plane;
	idVec3					center;
	int						blockingBits;		// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc
	qhandle_t				portalHandle;
	int						portalFlags;
} exitPortal_t;

// modelTrace_t is for tracing vs. visual geometry
typedef struct modelTrace_s {
	float						fraction;			// fraction of trace completed
	idVec3						point;				// end point of trace in global space
	idVec3						normal;				// hit triangle normal vector in global space
	idVec2						st;					// texture coordinate
	const idMaterial *			material;			// material of hit surface
	const sdDeclSurfaceType *	surfaceType;
	const srfTriangles_t	*	geometry;
	idVec3						surfaceColor;
	const renderEntity_t *		entity;				// render entity that was hit
	const class idRenderEntity *		def;
	int							jointNumber;		// md5 joint nearest to the hit triangle

} modelTrace_t;

static const int NUM_PORTAL_ATTRIBUTES = 3;

typedef enum {
	PS_BLOCK_NONE		= 0,

	PS_BLOCK_VIEW		= 1,
	PS_BLOCK_LOCATION	= 2,	// game map location strings often stop in hallways
	PS_BLOCK_AIR		= 4,	// windows between pressurized and unpresurized areas

	PS_BLOCK_ALL = ( 1 << NUM_PORTAL_ATTRIBUTES ) - 1
} portalConnection_t;

// RAVEN BEGIN
// jscott: effect handling in the renderer
class rvRenderEffect {
public:
	virtual					~rvRenderEffect( void ) {}

	virtual void			FreeRenderEffect( void ) = 0;
	virtual void			UpdateRenderEffect( const renderEffect_t *def, bool forceUpdate = false ) = 0;
	virtual void			ForceUpdate( void ) = 0;
	virtual int				GetIndex( void ) = 0;
};
// RAVEN END

class sdOcclusionTest {
public:
	virtual					~sdOcclusionTest( void ) {}

	virtual void			FreeOcclusionTest( void ) = 0;
	virtual void			UpdateOcclusionTest( const occlusionTest_t *def ) = 0;
};

class idRenderWorld {
public:
	virtual					~idRenderWorld() {};

	// The same render world can be reinitialized as often as desired
	// a NULL or empty mapName will create an empty, single area world
	virtual bool			InitFromMap( const char *mapName ) = 0;
	virtual void			LinkCullSectorsToArea( int area ) = 0;

	//-------------- Entity and Light Defs -----------------

	// entityDefs and lightDefs are added to a given world to determine
	// what will be drawn for a rendered scene.  Most update work is defered
	// until it is determined that it is actually needed for a given view.
	virtual	qhandle_t		AddEntityDef( const renderEntity_t *re ) = 0;
	virtual	void			UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t *re ) = 0;
	virtual	void			FreeEntityDef( qhandle_t entityHandle ) = 0;
	virtual const renderEntity_t *GetRenderEntity( qhandle_t entityHandle ) const = 0;
	virtual renderEntity_t *GetRenderEntity( qhandle_t entityHandle ) = 0;

	virtual	qhandle_t		AddLightDef( const renderLight_t *rlight ) = 0;
	virtual	void			UpdateLightDef( qhandle_t lightHandle, const renderLight_t *rlight ) = 0;
	virtual	void			FreeLightDef( qhandle_t lightHandle ) = 0;
	virtual const renderLight_t *GetRenderLight( qhandle_t lightHandle ) const = 0;

// RAVEN BEGIN
// jscott: handling of effects
	virtual qhandle_t		AddEffectDef( const renderEffect_t *reffect, int time ) = 0;
	virtual bool			UpdateEffectDef( qhandle_t effectHandle, const renderEffect_t *reffect, int time ) = 0;
	virtual void			StopEffectDef( qhandle_t effectHandle ) = 0;
	virtual void			RestartEffectDef( qhandle_t effectHandle ) = 0;
	virtual void			FreeEffectDef( qhandle_t effectHandle ) = 0;
	virtual void			FreeStoppedEffectDefs( void ) = 0;
// RAVEN END

// Game side occlusion tests
	virtual qhandle_t		AddOcclusionTestDef( const occlusionTest_t *occtest ) = 0;
	virtual void			UpdateOcclusionTestDef( qhandle_t occtestHandle, const occlusionTest_t *occtest ) = 0;
	virtual void			UpdateOcclusionTestDefViewID( qhandle_t occtestHandle, int viewID ) = 0;
	virtual bool			IsVisibleOcclusionTestDef( qhandle_t occtestHandle ) = 0;
	virtual	void			FreeOcclusionTestDef( qhandle_t occtestHandle ) = 0;
	virtual int				CountVisibleOcclusionTestDef( qhandle_t occtestHandle ) = 0;


	virtual bool			IsVisibleEntity( int viewID, int occid ) = 0;


	virtual void			UpdateOcclusionTests( void ) = 0;


	// Force the generation of all light / surface interactions at the start of a level
	// If this isn't called, they will all be dynamically generated
	virtual	void			GenerateAllInteractions() = 0;

	virtual idRenderModel*	GetEntityHandleDynamicModel( qhandle_t entityHandle ) = 0;

	//-------------- Decals and Overlays  -----------------

	virtual idRenderModel*	CreateDecalModel() = 0;
	virtual void 			AddToProjectedDecal( const idFixedWinding& winding, const idVec3 &projectionOrigin, const bool parallel, const idVec4& color, idRenderModel* model, int entityNum, const idMaterial** onlyMaterials = NULL, const int numOnlyMaterials = 1 ) = 0;
	virtual void			ResetDecalModel( idRenderModel* model ) = 0;
	virtual void			FinishDecal( idRenderModel* model ) = 0;

	// Creates decals on all world surfaces that the winding projects onto.
	// The projection origin should be infront of the winding plane.
	// The decals are projected onto world geometry between the winding plane and the projection origin.
	// The decals are depth faded from the winding plane to a certain distance infront of the
	// winding plane and the same distance from the projection origin towards the winding.
	// Arnout: FIXME as soon as there is a nicer way to get the current game time in the engine, stop passing in currentTime
	virtual void			ProjectDecalOntoWorld( const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime, const int currentTime ) = 0;

	// Creates decals on static models.
	virtual void			ProjectDecal( qhandle_t entityHandle, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime, const int currentTime ) = 0;

	// Creates overlays on dynamic models.
	virtual void			ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial *material ) = 0;

	// Removes all decals and overlays from the given entity def.
	virtual void			RemoveDecals( qhandle_t entityHandle ) = 0;
	virtual void			AddCheapDecal( qhandle_t entityHandle, const cheapDecalParameters_t &params, float time ) = 0;

	virtual void			ClearDecals( void ) = 0;

	//-------------- Env Bounds -----------------

	virtual void			AddEnvBounds( idVec3 const &origin, idVec3 const &scale, const char *cubemap ) = 0;

	//-------------- Scene Rendering -----------------

	// some calls to material functions use the current renderview time when servicing cinematics.  this function
	// ensures that any parms accessed (such as time) are properly set.
	virtual void			SetRenderView( const renderView_t *renderView ) = 0;

	// rendering a scene may actually render multiple subviews for mirrors and portals, and
	// may render composite textures for gui console screens and light projections
	// It would also be acceptable to render a scene multiple times, for "rear view mirrors", etc
	virtual void			RenderScene( const renderView_t *renderView ) = 0;
	virtual void			PerformRenderScene( const renderView_t *renderView ) = 0;

	//-------------- Portal Area Information -----------------

	// returns the number of portals
	virtual int				NumPortals( void ) const = 0;

	// returns 0 if no portal contacts the bounds
	// This is used by the game to identify portals that are contained
	// inside doors, so the connection between areas can be topologically
	// terminated when the door shuts.
	virtual	qhandle_t		FindPortal( const idBounds &b ) const = 0;

	// doors explicitly close off portals when shut
	// multiple bits can be set to block multiple things, ie: ( PS_VIEW | PS_LOCATION | PS_AIR )
	virtual	void			SetPortalState( qhandle_t portal, int blockingBits ) = 0;
	virtual int				GetPortalState( qhandle_t portal ) = 0;

	virtual void			UpdatePortalOccTestView( int viewID ) = 0;

	// returns true only if a chain of portals without the given connection bits set
	// exists between the two areas (a door doesn't separate them, etc)
	virtual	bool			AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) = 0;
	virtual	bool			AreasAreConnected( int areaNum1, int areaNum2, portalFlags_t flag ) = 0;
	virtual	bool			AreasAreConnected( int areaNum1, int areaNum2 ) = 0;

	// returns the number of portal areas in a map, so game code can build information
	// tables for the different areas
	virtual	int				NumAreas( void ) const = 0;

	// Will return -1 if the point is not in an area, otherwise
	// it will return 0 <= value < NumAreas()
	virtual int				PointInArea( const idVec3 &point ) const = 0;

	// fills the *areas array with the numbers of the areas the bounds cover
	// returns the total number of areas the bounds cover
	virtual int				BoundsInAreas( const idBounds &bounds, int *areas, int maxAreas ) const = 0;

	// Used by the sound system to do area flowing
	virtual	int				NumPortalsInArea( int areaNum ) = 0;

	// returns one portal from an area
	virtual exitPortal_t	GetPortal( int areaNum, int portalNum ) = 0;

	// set portal flags on areas directly; primarely for editor reasons
	virtual void			SetAreaPortalFlags( int areaNum, int flags ) = 0;
	virtual int				GetAreaPortalFlags( int areaNum ) const = 0;

	// set the ambient lighting & atmosphere to use for this area
	virtual void			SetAreaAmbientCubeMap( int areaNum, const sdDeclAmbientCubeMap *cubeMapDecl ) = 0;

	virtual const sdDeclAmbientCubeMap *GetAreaAmbientCubeMap( int areaNum ) = 0;
	virtual void			SetCubemapSunProperties( const sdDeclAmbientCubeMap *cubeMapDecl, const idVec3 &sunDir, const idVec3 &sunColor ) = 0;

	//-------------- Tracing  -----------------

	// Traces vs the render model, possibly instantiating a dynamic version, and returns true if something was hit
	virtual bool			ModelTrace( modelTrace_t &trace, qhandle_t entityHandle, const idVec3 &start, const idVec3 &end, const float radius, int surfCollision = SURF_COLLISION ) const = 0;

	// Traces vs the whole rendered world. FIXME: we need some kind of material flags.
	virtual bool			Trace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, bool skipDynamic = true ) const = 0;

	// Traces vs the world model bsp tree.
	virtual bool			FastWorldTrace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;

	//-------------- Demo Control  -----------------

	// Writes a loadmap command to the demo, and clears archive counters.
	virtual void			StartWritingDemo( idDemoFile *demo ) = 0;
	virtual void			StopWritingDemo() = 0;

	// Returns true when demoRenderView has been filled in.
	// adds/updates/frees entityDefs and lightDefs based on the current demo file
	// and returns the renderView to be used to render this frame.
	// a demo file may need to be advanced multiple times if the framerate
	// is less than 30hz
	// demoTimeOffset will be set if a new map load command was processed before
	// the next renderScene
	virtual bool			ProcessDemoCommand( idDemoFile *readDemo, renderView_t *demoRenderView, int *demoTimeOffset ) = 0;

	// this is used to regenerate all interactions ( which is currently only done during influences ), there may be a less
	// expensive way to do it
	virtual void			RegenerateWorld() = 0;

	//-------------- Debug Visualization  -----------------

	// Line drawing for debug visualization
	virtual void			DebugClearLines( int time ) = 0;		// a time of 0 will clear all lines and text
	virtual void			DebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifetime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugArrow( const idVec4 &color, const idVec3 &start, const idVec3 &end, int size, const int lifetime = 0, bool depthTest = false ) = 0;
	virtual void			DebugWinding( const idVec4 &color, const idWinding &w, const idVec3 &origin, const idMat3 &axis, const int lifetime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugCircle( const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugSphere( const idVec4 &color, const idSphere &sphere, const int lifetime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugBounds( const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const idMat3& axes = mat3_identity, const int lifetime = 0 ) = 0;
	virtual void			DebugBox( const idVec4 &color, const idBox &box, const int lifetime = 0 ) = 0;
	virtual void			DebugFrustum( const idVec4 &color, const idFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0 ) = 0;
	virtual void			DebugCone( const idVec4 &color, const idVec3 &apex, const idVec3 &dir, float radius1, float radius2, const int lifetime = 0 ) = 0;
	virtual void			DebugAxis( const idVec3 &origin, const idMat3 &axis, int lifetime = 0 ) = 0;

	// Polygon drawing for debug visualization.
	virtual void			DebugClearPolygons( int time ) = 0;		// a time of 0 will clear all polygons
	virtual void			DebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime = 0, const bool depthTest = false, idImage* image = NULL ) = 0;

	// Text drawing for debug visualization.
	virtual void			DrawText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align = 1, const int lifetime = 0) = 0;

	// Atmosphere / Ambient Systems.
	virtual void			SetAtmosphere( const sdDeclAtmosphere* atmosphere ) = 0;

	virtual const sdDeclAtmosphere*	GetAtmosphere() const = 0;

	virtual void			SetupMatrices( const renderView_t* renderView, float* projectionMatrix, float* modelViewMatrix, const bool allowJitter ) = 0;

	virtual void			SetMegaTextureSTGrid( const idBounds& bounds, const idVec2* grid, int width, int height ) = 0;

	virtual struct atmosLightProjection_t *FindAtmosLightProjection( int lightID ) = 0;
};


/*
============
sdUserInterface
============
*/
class sdUserInterface {
public:
	virtual					~sdUserInterface( void ) {}
	virtual void			Draw( void ) = 0;
	virtual void			Activate( void ) = 0;
	virtual void			OnInputInit( void ) = 0;
	virtual void			OnInputShutdown( void ) = 0;
};

#endif /* !__RENDERWORLD_H__ */
