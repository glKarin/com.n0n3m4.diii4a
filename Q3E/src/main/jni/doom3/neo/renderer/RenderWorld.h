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

#ifndef __RENDERWORLD_H__
#define __RENDERWORLD_H__

/*
===============================================================================

	Render World

===============================================================================
*/

#define PROC_FILE_EXT				"proc"

#ifdef _RAVEN // quake4 proc file
#define PROC_FILE_ID					"PROC"
#define PROC_FILEVERSION				"4" // jmarshall: changed to string. 
#else
#define	PROC_FILE_ID				"mapProcFile003"
#endif

// shader parms
#ifdef _HUMANHEAD
const int MAX_GLOBAL_SHADER_PARMS	= 13;	// HUMANHEAD pdm: increased from 12 (see also: MAX_ENTITY_SHADER_PARMS)
#else
const int MAX_GLOBAL_SHADER_PARMS	= 12;
#endif

const int SHADERPARM_RED			= 0;
const int SHADERPARM_GREEN			= 1;
const int SHADERPARM_BLUE			= 2;
const int SHADERPARM_ALPHA			= 3;
const int SHADERPARM_TIMESCALE		= 3;
const int SHADERPARM_TIMEOFFSET		= 4;
const int SHADERPARM_DIVERSITY		= 5;	// random between 0.0 and 1.0 for some effects (muzzle flashes, etc)
const int SHADERPARM_MODE			= 7;	// for selecting which shader passes to enable
const int SHADERPARM_TIME_OF_DEATH	= 7;	// for the monster skin-burn-away effect enable and time offset

// model parms
const int SHADERPARM_MD5_SKINSCALE	= 8;	// for scaling vertex offsets on md5 models (jack skellington effect)

const int SHADERPARM_MD3_FRAME		= 8;
const int SHADERPARM_MD3_LASTFRAME	= 9;
const int SHADERPARM_MD3_BACKLERP	= 10;

const int SHADERPARM_BEAM_END_X		= 8;	// for _beam models
const int SHADERPARM_BEAM_END_Y		= 9;
const int SHADERPARM_BEAM_END_Z		= 10;
const int SHADERPARM_BEAM_WIDTH		= 11;

const int SHADERPARM_SPRITE_WIDTH		= 8;
const int SHADERPARM_SPRITE_HEIGHT		= 9;

const int SHADERPARM_PARTICLE_STOPTIME = 8;	// don't spawn any more particles after this time

// guis
const int MAX_RENDERENTITY_GUI		= 3;

#ifdef _HUMANHEAD
// HUMANHEAD pdm
const int SHADERPARM_MISC = 6; // aob
const int SHADERPARM_ANY_DEFORM = 9; // Model deformation
const int SHADERPARM_ANY_DEFORM_PARM1 = 10; // Model deformation parameter1
const int SHADERPARM_ANY_DEFORM_PARM2 = 11; // Model deformation parameter2
const int SHADERPARM_DISTANCE = 12; // CJR -- distance shader parm index
// Always add to the end of the list so the old numbers don't change
enum {
	DEFORMTYPE_NONE = 0,
	DEFORMTYPE_SCALE,
	DEFORMTYPE_VERTEXCOLOR,
	DEFORMTYPE_SPHERE,
	DEFORMTYPE_RIPPLE,
	DEFORMTYPE_PLANTSWAYX,
	DEFORMTYPE_PLANTSWAYY,
	DEFORMTYPE_FLATTEN,
	DEFORMTYPE_VIBRATE,
	DEFORMTYPE_SQUISH,
	DEFORMTYPE_TURBULENT,	// 10
	DEFORMTYPE_RAYS,
	DEFORMTYPE_ALPHAGLOW,
	DEFORMTYPE_FATTEN,
	DEFORMTYPE_RIPPLECENTER,
	DEFORMTYPE_MELT,
	DEFORMTYPE_POD,
	DEFORMTYPE_DEATHEFFECT,
	DEFORMTYPE_WINDBLAST,
	DEFORMTYPE_PINCHPOINT,
	DEFORMTYPE_PORTAL
	//NOTE: Any added here should also be added to prey_defs.script
};
// HUMANHEAD END

// HUMANHEAD CJR:  Beam node information
#define MAX_BEAM_NODES				32
typedef struct hhBeamNodes_s {
	idVec3		nodes[MAX_BEAM_NODES];
} hhBeamNodes_t;
// END HUMANHEAD
#endif

#ifdef _RAVEN
// RAVEN BEGIN
// jscott: for effect brightness
const int SHADERPARM_BRIGHTNESS		= 6;	// for the overall brightness of effects
// RAVEN END

// RAVEN BEGIN
// dluetscher: added a default value for light detail levels
#define DEFAULT_LIGHT_DETAIL_LEVEL	10.f
// RAVEN END 

// RAVEN BEGIN
// AReis: Render flags.
enum
{
	RF_NORMAL					= 0,
	// Let the renderer know its in a editor of sorts.
	RF_IS_EDITOR				= BIT( 0 ),
	// Viewdef is fullscreen and is 2d (mostly for main menu)
	RF_IS_FULLSCREEN_2D			= BIT( 1 ),
	// Don't draw the GUI, just the world.
	RF_NO_GUI					= BIT( 2 ),
	// Only draw the GUI, not the world.
	RF_GUI_ONLY					= BIT( 3 ),
// RAVEN BEGIN
// dluetscher: added render flag to denote that penumbra map rendering is desired 
	RF_PENUMBRA_MAP				= BIT( 4 ),
// dluetscher: added render flag that defers the command buffer submission of render 
//			   commands until the first non-deferred render command (with the exception of 
//			   certain render commands like RC_DRAW_PENUMBRA_MAPS - which ignores deferred
//			   render commands and lets them get submitted past)
	RF_DEFER_COMMAND_SUBMIT		= BIT( 5 ),
	// this is a portal sky view
	RF_PORTAL_SKY				= BIT( 6 ),
	// this the primary view - when this is rendered, we then know we can capture the screen buffer
	RF_PRIMARY_VIEW				= BIT( 7 ),
// RAVEN END
};

// RAVEN END

#endif

typedef bool(*deferredEntityCallback_t)(renderEntity_s *, const renderView_s *);

#ifdef _RAVEN // particle
// RAVEN BEGIN
// jscott: for handling of effects
typedef struct renderEffect_s {

	const idDecl			*declEffect;

	float					startTime;
	int						suppressSurfaceInViewID;
	int						allowSurfaceInViewID;
	int						groupID;

	idVec3					origin;
	idMat3					axis;

	idVec3					gravity;
	idVec3					endOrigin;

	float					attenuation;
	bool					hasEndOrigin;
	bool					loop;						// effect is looping
	bool					ambient;					// effect is from an entity
	bool					inConnectedArea;
	int						weaponDepthHackInViewID;	// squash depth range so view weapons don't poke into walls
	float					modelDepthHack;	

	int						referenceSoundHandle;		// for shader sound tables, allowing effects to vary with sounds

	float					shaderParms[ MAX_ENTITY_SHADER_PARMS ];	// can be used in any way by shader or model generation
} renderEffect_t;
// RAVEN END

// RAVEN BEGIN
// jscott: effect handling in the renderer
class rvRenderEffect {
public:
	virtual					~rvRenderEffect(void) {}
};
// RAVEN END
#endif

typedef struct renderEntity_s {
	idRenderModel 			*hModel;				// this can only be null if callback is set

	int						entityNum;
	int						bodyId;

	// Entities that are expensive to generate, like skeletal models, can be
	// deferred until their bounds are found to be in view, in the frustum
	// of a shadowing light that is in view, or contacted by a trace / overlay test.
	// This is also used to do visual cueing on items in the view
	// The renderView may be NULL if the callback is being issued for a non-view related
	// source.
	// The callback function should clear renderEntity->callback if it doesn't
	// want to be called again next time the entity is referenced (ie, if the
	// callback has now made the entity valid until the next updateEntity)
	idBounds				bounds;					// only needs to be set for deferred models and md5s
	deferredEntityCallback_t	callback;

	void 					*callbackData;			// used for whatever the callback wants

	// player bodies and possibly player shadows should be suppressed in views from
	// that player's eyes, but will show up in mirrors and other subviews
	// security cameras could suppress their model in their subviews if we add a way
	// of specifying a view number for a remoteRenderMap view
	int						suppressSurfaceInViewID;
	int						suppressShadowInViewID;

	// world models for the player and weapons will not cast shadows from view weapon
	// muzzle flashes
	int						suppressShadowInLightID;

	// if non-zero, the surface and shadow (if it casts one)
	// will only show up in the specific view, ie: player weapons
	int						allowSurfaceInViewID;

	// positioning
	// axis rotation vectors must be unit length for many
	// R_LocalToGlobal functions to work, so don't scale models!
	// axis vectors are [0] = forward, [1] = left, [2] = up
	idVec3					origin;
	idMat3					axis;

	// texturing
	const idMaterial 		*customShader;			// if non-0, all surfaces will use this
	const idMaterial 		*referenceShader;		// used so flares can reference the proper light shader
	const idDeclSkin 		*customSkin;				// 0 for no remappings
#ifdef _RAVEN
// RAVEN BEGIN
	int						referenceSoundHandle;	// for shader sound tables, allowing effects to vary with sounds
// RAVEN END	
// RAVEN BEGIN
// bdube: hiding surfaces
	// Mask of surfaces which should be suppresesed when rendering the entity
	int						suppressSurfaceMask;
// RAVEN END	
// RAVEN BEGIN
// ddynerman: Wolf LOD code
	float					shadowLODDistance;
	int						suppressLOD;
// RAVEN END
// RAVEN BEGIN
// bdube: overlay shaders
	const idMaterial *		overlayShader;			// overlays the model on top of itself with this shader (originally for powerups)

// bdube: weapon depth hack only in a given view id
	int						weaponDepthHackInViewID;// squash depth range so view weapons don't poke into walls
// RAVEN END
#else
	class idSoundEmitter 	*referenceSound;			// for shader sound tables, allowing effects to vary with sounds
#endif
	float					shaderParms[ MAX_ENTITY_SHADER_PARMS ];	// can be used in any way by shader or model generation

	// networking: see WriteGUIToSnapshot / ReadGUIFromSnapshot
	class idUserInterface *gui[ MAX_RENDERENTITY_GUI ];

	struct renderView_s		*remoteRenderView;		// any remote camera surfaces will use this

	int						numJoints;
	idJointMat 			*joints;					// array of joints that will modify vertices.
	// NULL if non-deformable model.  NOT freed by renderer

	float					modelDepthHack;			// squash depth range so particle effects don't clip into walls

	// options to override surface shader flags (replace with material parameters?)
	bool					noSelfShadow;			// cast shadows onto other objects,but not self
	bool					noShadow;				// no shadow at all

	bool					noDynamicInteractions;	// don't create any light / shadow interactions after
	// the level load is completed.  This is a performance hack
	// for the gigantic outdoor meshes in the monorail map, so
	// all the lights in the moving monorail don't touch the meshes

	bool					weaponDepthHack;		// squash depth range so view weapons don't poke into walls
	// this automatically implies noShadow
	int						forceUpdate;			// force an update (NOTE: not a bool to keep this struct a multiple of 4 bytes)
#ifdef _HUMANHEAD
	const hhDeclBeam*		declBeam;			// HUMANHEAD beam information
	hhBeamNodes_t*			beamNodes;			// HUMANHEAD beam node array (sized to the number of beams in the system)
												
	//HUMANHEAD rww - moved other bools up here
//#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	bool					notInRenderDemos;		//if this is true, we will not record this renderentity
//#endif //HUMANHEAD END
	   
	// HUMANHEAD:
	bool				    onlyVisibleInSpirit;	// True if this entity is only visible when spiritwalking or deathwalking
	bool				    onlyInvisibleInSpirit;	// True if this entity is only invisible when spiritwalking or deathwalking -tmj
	bool				    lowSkippable;			// bjk: True if skippable in low quality
	float				    eyeDistance;			// HUMANHEAD pdm: precalculated distance to eye
	// HUMANHEAD END
#endif

	int						timeGroup;
	int						xrayIndex;
} renderEntity_t;


typedef struct renderLight_s {
	idMat3					axis;				// rotation vectors, must be unit length
	idVec3					origin;

	// if non-zero, the light will not show up in the specific view,
	// which may be used if we want to have slightly different muzzle
	// flash lights for the player and other views
	int						suppressLightInViewID;

	// if non-zero, the light will only show up in the specific view
	// which can allow player gun gui lights and such to not effect everyone
	int						allowLightInViewID;

	// I am sticking the four bools together so there are no unused gaps in
	// the padded structure, which could confuse the memcmp that checks for redundant
	// updates
	bool					noShadows;			// (should we replace this with material parameters on the shader?)
	bool					noSpecular;			// (should we replace this with material parameters on the shader?)

#ifdef _HUMANHEAD
	bool					lowSkippable;		// HUMANHEAD bjk: True if skippable in low quality
#endif

	bool					pointLight;			// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	bool					parallel;			// lightCenter gives the direction to the light at infinity
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

	// Dmap will generate an optimized shadow volume named _prelight_<lightName>
	// for the light against all the _area* models in the map.  The renderer will
	// ignore this value if the light has been moved after initial creation
	idRenderModel 			*prelightModel;

	// muzzle flash lights will not cast shadows from player and weapon world models
	int						lightId;


	const idMaterial 		*shader;				// NULL = either lights/defaultPointLight or lights/defaultProjectedLight
	float					shaderParms[MAX_ENTITY_SHADER_PARMS];		// can be used in any way by shader
#ifdef _RAVEN
// RAVEN BEGIN
// dluetscher: added a min light detail level setting that describes when this light is visible
    float					detailLevel;
// ddynerman: no dynamic shadows - allows dmap to create optimized static shadows, but prevents dynamic shadows
    bool					noDynamicShadows;
// RAVEN END
    bool					globalLight;		// Whether this is a global light or not.

    int						referenceSoundHandle;		// for shader sound tables, allowing effects to vary with sounds
#else
	idSoundEmitter 		    *referenceSound;		// for shader sound tables, allowing effects to vary with sounds
#endif
} renderLight_t;


typedef struct renderView_s {
	// player views will set this to a non-zero integer for model suppress / allow
	// subviews (mirrors, cameras, etc) will always clear it to zero
	int						viewID;

	// sized from 0 to SCREEN_WIDTH / SCREEN_HEIGHT (640/480), not actual resolution
	int						x, y, width, height;

	float					fov_x, fov_y;
	idVec3					vieworg;
	idMat3					viewaxis;			// transformation matrix, view looks down the positive X axis

	bool					cramZNear;			// for cinematics, we want to set ZNear much lower
	bool					forceUpdate;		// for an update

#ifdef _HUMANHEAD
	bool			        viewSpiritEntities; // HUMANHEAD cjr: this renderView can see all onlyVisibleInSpirit entities
									   // tmj: this renderView cannot see onlyInvisibleInSpirit entities

	float				    frac;			// fraction of trace before hit
#endif

	// time in milliseconds for shader effects and other time dependent rendering issues
	int						time;
	float					shaderParms[MAX_GLOBAL_SHADER_PARMS];		// can be used in any way by shader
	const idMaterial		*globalMaterial;							// used to override everything draw
} renderView_t;


// exitPortal_t is returned by idRenderWorld::GetPortal()
typedef struct {
	int					areas[2];		// areas connected by this portal
	const idWinding		*w;				// winding points have counter clockwise ordering seen from areas[0]
	int					blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc
	qhandle_t			portalHandle;
} exitPortal_t;


// guiPoint_t is returned by idRenderWorld::GuiTrace()
typedef struct {
	float				x, y;			// 0.0 to 1.0 range if trace hit a gui, otherwise -1
	int					guiId;			// id of gui ( 0, 1, or 2 ) that the trace happened against
#ifdef _HUMANHEAD
	float				frac;
#endif
} guiPoint_t;


// modelTrace_t is for tracing vs. visual geometry
typedef struct modelTrace_s {
	float					fraction;			// fraction of trace completed
	idVec3					point;				// end point of trace in global space
	idVec3					normal;				// hit triangle normal vector in global space
	const idMaterial 		*material;			// material of hit surface
	const renderEntity_t 	*entity;				// render entity that was hit
	int						jointNumber;		// md5 joint nearest to the hit triangle
#ifdef _RAVEN // quake4 trace
// RAVEN BEGIN
// jscott: added block
	const rvDeclMatType *	materialType;
// RAVEN END
#endif
} modelTrace_t;


#ifdef _RAVEN
// RAVEN BEGIN
// abahr: changed to 4 to include gravity
static const int NUM_PORTAL_ATTRIBUTES = 4;
// RAVEN END
#elif defined(_HUMANHEAD)
static const int NUM_PORTAL_ATTRIBUTES = 4;		// HUMANHEAD pdm: Bumped from 3
#else
static const int NUM_PORTAL_ATTRIBUTES = 3;
#endif

typedef enum {
	PS_BLOCK_NONE = 0,

	PS_BLOCK_VIEW = 1,
	PS_BLOCK_LOCATION = 2,		// game map location strings often stop in hallways
	PS_BLOCK_AIR = 4,			// windows between pressurized and unpresurized areas

#ifdef _RAVEN //k CONFLICT
// abahr
	PS_BLOCK_GRAVITY = 8,		// PS_BLOCK_ALL does not block gravity.  Must use info_gravityseperator

	PS_BLOCK_ALL = PS_BLOCK_VIEW|PS_BLOCK_LOCATION|PS_BLOCK_AIR
#elif defined(_HUMANHEAD)
// HUMANHEAD pdm
    PS_BLOCK_SOUND = 8,			// blocks sound completely, used on game portals
// HUMANHEAD END
	PS_BLOCK_ALL = (1<<NUM_PORTAL_ATTRIBUTES)-1
#else
	PS_BLOCK_ALL = (1<<NUM_PORTAL_ATTRIBUTES)-1
#endif
} portalConnection_t;


class idRenderWorld
{
	public:
		virtual					~idRenderWorld() {};

		// The same render world can be reinitialized as often as desired
		// a NULL or empty mapName will create an empty, single area world
		virtual bool			InitFromMap(const char *mapName) = 0;

		//-------------- Entity and Light Defs -----------------

		// entityDefs and lightDefs are added to a given world to determine
		// what will be drawn for a rendered scene.  Most update work is defered
		// until it is determined that it is actually needed for a given view.
		virtual	qhandle_t		AddEntityDef(const renderEntity_t *re) = 0;
		virtual	void			UpdateEntityDef(qhandle_t entityHandle, const renderEntity_t *re) = 0;
		virtual	void			FreeEntityDef(qhandle_t entityHandle) = 0;
		virtual const renderEntity_t *GetRenderEntity(qhandle_t entityHandle) const = 0;

		virtual	qhandle_t		AddLightDef(const renderLight_t *rlight) = 0;
		virtual	void			UpdateLightDef(qhandle_t lightHandle, const renderLight_t *rlight) = 0;
		virtual	void			FreeLightDef(qhandle_t lightHandle) = 0;
		virtual const renderLight_t *GetRenderLight(qhandle_t lightHandle) const = 0;

		// Force the generation of all light / surface interactions at the start of a level
		// If this isn't called, they will all be dynamically generated
		virtual	void			GenerateAllInteractions() = 0;

		// returns true if this area model needs portal sky to draw
		virtual bool			CheckAreaForPortalSky(int areaNum) = 0;

		//-------------- Decals and Overlays  -----------------

		// Creates decals on all world surfaces that the winding projects onto.
		// The projection origin should be infront of the winding plane.
		// The decals are projected onto world geometry between the winding plane and the projection origin.
		// The decals are depth faded from the winding plane to a certain distance infront of the
		// winding plane and the same distance from the projection origin towards the winding.
		virtual void			ProjectDecalOntoWorld(const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime) = 0;

		// Creates decals on static models.
		virtual void			ProjectDecal(qhandle_t entityHandle, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime) = 0;

		// Creates overlays on dynamic models.
		virtual void			ProjectOverlay(qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial *material) = 0;

		// Removes all decals and overlays from the given entity def.
		virtual void			RemoveDecals(qhandle_t entityHandle) = 0;

		//-------------- Scene Rendering -----------------

		// some calls to material functions use the current renderview time when servicing cinematics.  this function
		// ensures that any parms accessed (such as time) are properly set.
		virtual void			SetRenderView(const renderView_t *renderView) = 0;

#ifdef _RAVEN
// jscott: for portal skies
	virtual bool			HasSkybox( int areaNum ) = 0;
	virtual void			FindVisibleAreas( idVec3 origin, int areaNum, bool *visibleAreas ) = 0;

// jscott: handling of effects
	virtual qhandle_t		AddEffectDef( const renderEffect_t *reffect, int time ) = 0;
	virtual bool			UpdateEffectDef( qhandle_t effectHandle, const renderEffect_t *reffect, int time ) = 0;
	virtual void			StopEffectDef( qhandle_t effectHandle ) = 0;
	virtual const class rvRenderEffectLocal* GetEffectDef( qhandle_t effectHandle ) const = 0;
	virtual void			FreeEffectDef( qhandle_t effectHandle ) = 0;
	virtual bool			EffectDefHasSound( const renderEffect_s *reffect ) = 0;

	virtual void			DebugClear(int time) = 0;		// a time of 0 will clear all lines and text
// jscott: want to be able to specify depth test
	virtual void			DebugBounds(const idVec4& color, const idBounds& bounds, const idVec3& org, const int lifetime, bool depthTest) = 0;
	virtual void			DebugFOV(const idVec4& color, const idVec3& origin, const idVec3& dir, float farDot, float farDist, float nearDot = 1.0f, float nearDist = 0.0f, float alpha = 0.3f, int lifetime = 0) = 0;
		virtual void			RenderScene(const renderView_t *renderView, int renderFlags/* = RF_NORMAL */) = 0;
// AReis: Modified RenderScene() signature to include renderFlags variable.
#endif
		// rendering a scene may actually render multiple subviews for mirrors and portals, and
		// may render composite textures for gui console screens and light projections
		// It would also be acceptable to render a scene multiple times, for "rear view mirrors", etc
		virtual void			RenderScene(const renderView_t *renderView) = 0;

		//-------------- Portal Area Information -----------------

		// returns the number of portals
		virtual int				NumPortals(void) const = 0;

		// returns 0 if no portal contacts the bounds
		// This is used by the game to identify portals that are contained
		// inside doors, so the connection between areas can be topologically
		// terminated when the door shuts.
		virtual	qhandle_t		FindPortal(const idBounds &b) const = 0;

		// doors explicitly close off portals when shut
		// multiple bits can be set to block multiple things, ie: ( PS_VIEW | PS_LOCATION | PS_AIR )
		virtual	void			SetPortalState(qhandle_t portal, int blockingBits) = 0;
		virtual int				GetPortalState(qhandle_t portal) = 0;

		// returns true only if a chain of portals without the given connection bits set
		// exists between the two areas (a door doesn't separate them, etc)
		virtual	bool			AreasAreConnected(int areaNum1, int areaNum2, portalConnection_t connection) = 0;

		// returns the number of portal areas in a map, so game code can build information
		// tables for the different areas
		virtual	int				NumAreas(void) const = 0;

		// Will return -1 if the point is not in an area, otherwise
		// it will return 0 <= value < NumAreas()
		virtual int				PointInArea(const idVec3 &point) const = 0;

		// fills the *areas array with the numbers of the areas the bounds cover
		// returns the total number of areas the bounds cover
		virtual int				BoundsInAreas(const idBounds &bounds, int *areas, int maxAreas) const = 0;

		// Used by the sound system to do area flowing
		virtual	int				NumPortalsInArea(int areaNum) = 0;

		// returns one portal from an area
		virtual exitPortal_t	GetPortal(int areaNum, int portalNum) = 0;

		//-------------- Tracing  -----------------

		// Checks a ray trace against any gui surfaces in an entity, returning the
		// fraction location of the trace on the gui surface, or -1,-1 if no hit.
		// This doesn't do any occlusion testing, simply ignoring non-gui surfaces.
		// start / end are in global world coordinates.
#ifdef _HUMANHEAD
		virtual guiPoint_t		GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end, int interactiveMask ) const = 0;	// HUMANHEAD pdm: added interactiveMask
#endif
		virtual guiPoint_t		GuiTrace(qhandle_t entityHandle, const idVec3 start, const idVec3 end) const = 0;

		// Traces vs the render model, possibly instantiating a dynamic version, and returns true if something was hit
		virtual bool			ModelTrace(modelTrace_t &trace, qhandle_t entityHandle, const idVec3 &start, const idVec3 &end, const float radius) const = 0;

		// Traces vs the whole rendered world. FIXME: we need some kind of material flags.
		virtual bool			Trace(modelTrace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false) const = 0;

		// Traces vs the world model bsp tree.
		virtual bool			FastWorldTrace(modelTrace_t &trace, const idVec3 &start, const idVec3 &end) const = 0;

		//-------------- Demo Control  -----------------

		// Writes a loadmap command to the demo, and clears archive counters.
		virtual void			StartWritingDemo(idDemoFile *demo) = 0;
		virtual void			StopWritingDemo() = 0;

		// Returns true when demoRenderView has been filled in.
		// adds/updates/frees entityDefs and lightDefs based on the current demo file
		// and returns the renderView to be used to render this frame.
		// a demo file may need to be advanced multiple times if the framerate
		// is less than 30hz
		// demoTimeOffset will be set if a new map load command was processed before
		// the next renderScene
		virtual bool			ProcessDemoCommand(idDemoFile *readDemo, renderView_t *demoRenderView, int *demoTimeOffset) = 0;

		// this is used to regenerate all interactions ( which is currently only done during influences ), there may be a less
		// expensive way to do it
		virtual void			RegenerateWorld() = 0;

		//-------------- Debug Visualization  -----------------

		// Line drawing for debug visualization
		virtual void			DebugClearLines(int time) = 0;		// a time of 0 will clear all lines and text
		virtual void			DebugLine(const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifetime = 0, const bool depthTest = false) = 0;
		virtual void			DebugArrow(const idVec4 &color, const idVec3 &start, const idVec3 &end, int size, const int lifetime = 0) = 0;
		virtual void			DebugWinding(const idVec4 &color, const idWinding &w, const idVec3 &origin, const idMat3 &axis, const int lifetime = 0, const bool depthTest = false) = 0;
		virtual void			DebugCircle(const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false) = 0;
		virtual void			DebugSphere(const idVec4 &color, const idSphere &sphere, const int lifetime = 0, bool depthTest = false) = 0;
		virtual void			DebugBounds(const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const int lifetime = 0) = 0;
		virtual void			DebugBox(const idVec4 &color, const idBox &box, const int lifetime = 0) = 0;
		virtual void			DebugFrustum(const idVec4 &color, const idFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0) = 0;
		virtual void			DebugCone(const idVec4 &color, const idVec3 &apex, const idVec3 &dir, float radius1, float radius2, const int lifetime = 0) = 0;
		virtual void			DebugAxis(const idVec3 &origin, const idMat3 &axis) = 0;

		// Polygon drawing for debug visualization.
		virtual void			DebugClearPolygons(int time) = 0;		// a time of 0 will clear all polygons
		virtual void			DebugPolygon(const idVec4 &color, const idWinding &winding, const int lifeTime = 0, const bool depthTest = false) = 0;

		// Text drawing for debug visualization.
		virtual void			DrawText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false) = 0;

#ifdef _HUMANHEAD
	    //HUMANHEAD rww
#if _HH_RENDERDEMO_HACKS
	    virtual void			DemoSmokeEvent(const idDeclParticle *smoke, const int systemTimeOffset, const float diversity, const idVec3 &origin, const idMat3 &axis) = 0;
#endif
	    //HUMANHEAD END
	    // HUMANHEAD pdm: game portal support
#if GAMEPORTAL_PVS
        virtual qhandle_t		FindGamePortal(const char *name) = 0;
        virtual void			RegisterGamePortals(idMapFile *mapFile) = 0;
        virtual void			DrawGamePortals(int mode, const idMat3 &viewAxis) = 0;
        virtual bool			IsGamePortal( qhandle_t handle ) = 0;
        virtual idVec3			GetGamePortalSrc( qhandle_t handle ) = 0;
        virtual idVec3			GetGamePortalDst( qhandle_t handle ) = 0;
#endif
#if GAMEPORTAL_SOUND
        virtual int				NumSoundPortalsInArea( int areaNum ) = 0;
        virtual int				NumGamePortalsInArea( int areaNum ) = 0;
        virtual exitPortal_t	GetSoundPortal( int areaNum, int portalNum ) = 0;
        virtual void			LevelInitSoundAreas() = 0;
        virtual void			LevelShutdownSoundAreas() = 0;
        virtual void			PrecalculateValidSoundAreas(const idVec3 listenerPosition, const int listenerArea) = 0;
        virtual bool			ValidSoundArea(const int area) = 0;
        virtual float			DistanceToSoundArea(const int area) = 0;
        virtual float			MaxSoundAreaExtents(const int area) = 0;

        virtual void			DrawValidSoundAreas(const idVec3 &source, const idMat3 &viewAxis) = 0;
#endif
        // HUMANHEAD END
#endif
};
#endif /* !__RENDERWORLD_H__ */
