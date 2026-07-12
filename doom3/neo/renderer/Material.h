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

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
===============================================================================

	Material

===============================================================================
*/

#ifdef _SPLASHDAMAGE
using namespace sdUtility;

class sdDeclRenderBinding;
class sdDeclRenderProgram;
class sdDeclSurfaceType;
class sdDeclSurfaceTypeMap;
class sdRenderProgram;
#endif

class idImage;
class idCinematic;
class idUserInterface;
class idMegaTexture;

#ifdef _HUMANHEAD
// HUMANHEAD tmj: type of subview that this surface represents
typedef enum {
	SC_MIRROR,
	SC_PORTAL,
	SC_PORTAL_SKYBOX,
} subviewClass_t;
#endif

// moved from image.h for default parm
typedef enum {
	TF_LINEAR,
	TF_NEAREST,
	TF_DEFAULT				// use the user-specified r_textureFilter
} textureFilter_t;

typedef enum {
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_BORDER,		// this should replace TR_CLAMP_TO_ZERO and TR_CLAMP_TO_ZERO_ALPHA,
	// but I don't want to risk changing it right now
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures,
	// set AFTER image format selection
	TR_CLAMP_TO_ZERO_ALPHA	// guarantee 0 alpha edge for projected textures,
	// set AFTER image format selection
#ifdef _SPLASHDAMAGE
	,
	TR_CLAMP_X,				// only clamp x direction
	TR_CLAMP_Y,				// only clamp y direction
	TR_MIRROR,
	TR_MIRROR_X,
	TR_MIRROR_Y,
#endif
} textureRepeat_t;

#ifdef _RAVENxxx //karin: TODO decal in Quake4
typedef struct {
    int		stayTime;		// msec for no change
    float	maxAngle;		// maximum dot product to reject projection angles
} decalInfo_t;
#else
typedef struct {
	int		stayTime;		// msec for no change
	int		fadeTime;		// msec to fade vertex colors over
	float	start[4];		// vertex color at spawn (possibly out of 0.0 - 1.0 range, will clamp after calc)
	float	end[4];			// vertex color at fade-out (possibly out of 0.0 - 1.0 range, will clamp after calc)
} decalInfo_t;
#endif

typedef enum {
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
#ifdef _HUMANHEADxxx
	DFRM_BEAM,		// HUMANHEAD
	DFRM_CORONA,	// HUMANHEAD
	DFRM_JITTER,	// HUMANHEAD: Jitter the model
#endif
	DFRM_PARTICLE,
	DFRM_PARTICLE2,
	DFRM_TURB
} deform_t;

typedef enum {
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
	DI_XRAY_RENDER,
#ifdef _RAVEN
    // RAVEN BEGIN
// AReis: Used for water reflection/refraction.
    DI_REFLECTION_RENDER,
    DI_REFRACTION_RENDER,
// RAVEN END
#endif
#ifdef _HUMANHEAD
	DI_PORTAL_RENDER, // HUMANHEAD
	DI_SKYBOX_RENDER, // HUMANHEAD tmj
#endif
	DI_REMOTE_RENDER
} dynamicidImage_t;

// note: keep opNames[] in sync with changes
typedef enum {
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_TABLE,
	OP_TYPE_GT,
	OP_TYPE_GE,
	OP_TYPE_LT,
	OP_TYPE_LE,
	OP_TYPE_EQ,
	OP_TYPE_NE,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_SOUND
#ifdef _RAVEN
	// RAVEN BEGIN
// rjohnson: new shader stage system
	,
	OP_TYPE_GLSL_ENABLED,
	OP_TYPE_POT_X,
	OP_TYPE_POT_Y,
// RAVEN END
#endif
#ifdef _HUMANHEAD
	,
    OP_TYPE_FRAGMENTPROGRAMS // HUMANHEAD CJR:  Added so fragment programs support can be toggled
#endif
#ifdef _SPLASHDAMAGE
	, OP_TYPE_LOAD
#endif
} expOpType_t;

typedef enum {
	EXP_REG_TIME,

	EXP_REG_PARM0,
	EXP_REG_PARM1,
	EXP_REG_PARM2,
	EXP_REG_PARM3,
	EXP_REG_PARM4,
	EXP_REG_PARM5,
	EXP_REG_PARM6,
	EXP_REG_PARM7,
	EXP_REG_PARM8,
	EXP_REG_PARM9,
	EXP_REG_PARM10,
	EXP_REG_PARM11,

	EXP_REG_GLOBAL0,
	EXP_REG_GLOBAL1,
	EXP_REG_GLOBAL2,
	EXP_REG_GLOBAL3,
	EXP_REG_GLOBAL4,
	EXP_REG_GLOBAL5,
	EXP_REG_GLOBAL6,
	EXP_REG_GLOBAL7,

#ifdef _RAVEN
    // RAVEN BEGIN
// rjohnson: added vertex randomizing
	EXP_REG_VERTEX_RANDOMIZER,
// RAVEN END
#endif
#ifdef _HUMANHEAD
	EXP_REG_DISTANCE, // HUMANHEAD: CJR
#endif

#ifdef _SPLASHDAMAGE
	EXP_REG_NUMLIGHTS,
	EXP_REG_SUN_R,
	EXP_REG_SUN_G,
	EXP_REG_SUN_B,
	EXP_REG_SUN_AZIMUTH,
	EXP_REG_WIND_X,
	EXP_REG_WIND_Y,
	EXP_REG_RANDF,
#endif
	EXP_REG_NUM_PREDEFINED
} expRegister_t;

typedef struct {
	expOpType_t		opType;
	int				a, b, c;
} expOp_t;

typedef struct {
	int				registers[4];
} colorStage_t;

typedef enum {
	TG_EXPLICIT,
	TG_DIFFUSE_CUBE,
	TG_REFLECT_CUBE,
	TG_SKYBOX_CUBE,
	TG_WOBBLESKY_CUBE,
	TG_SCREEN,			// screen aligned, for mirrorRenders and screen space temporaries
	TG_SCREEN2,
	TG_GLASSWARP
} texgen_t;

typedef struct {
	idCinematic 		*cinematic;
	idImage 			*image;
	texgen_t			texgen;
	bool				hasMatrix;
	int					matrix[2][3];	// we only allow a subset of the full projection matrix

	// dynamic image variables
	dynamicidImage_t	dynamic;
	int					width, height;
	int					dynamicFrameCount;
} textureStage_t;

// the order BUMP / DIFFUSE / SPECULAR is necessary for interactions to draw correctly on low end cards
typedef enum {
	SL_AMBIENT,						// execute after lighting
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR
} stageLighting_t;

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
typedef enum {
	SVC_IGNORE,
	SVC_MODULATE,
#ifdef _SPLASHDAMAGE
	SVC_MODULATE_ALPHA,
#endif
	SVC_INVERSE_MODULATE
} stageVertexColor_t;

static const int	MAX_FRAGMENT_IMAGES = 8;
#ifdef _RAVEN
// RAVEN BEGIN
// AReis: Increased MAX_VERTEX_PARMS from 4 to 16 and added MAX_FRAGMENT_PARMS.
static const int        MAX_VERTEX_PARMS = 16;
static const int        MAX_FRAGMENT_PARMS = 8;
// RAVEN END
#elif defined(_HUMANHEAD)
static const int	MAX_VERTEX_PARMS = 8;
static const int	MAX_FRAGMENT_PARMS = 8;
#else
static const int	MAX_VERTEX_PARMS = 4;
static const int	MAX_FRAGMENT_PARMS = 4; // add for DOOM3
#endif

typedef struct {
	int					vertexProgram;
	int					numVertexParms;
	int					vertexParms[MAX_VERTEX_PARMS][4];	// evaluated register indexes

#if defined(_GLSL_PROGRAM) || defined(_RAVEN) || defined(_HUMANHEAD) || defined(_SPLASHDAMAGE) //karin: fragment shader parms
// RAVEN BEGIN
// AReis: New Fragment Parm stuff.
    int                 numFragmentParms;
    int                 fragmentParms[MAX_FRAGMENT_PARMS][4];        // evaluated register indexes
// RAVEN END
#endif

	int					fragmentProgram;
	int					numFragmentProgramImages;
	idImage 			*fragmentProgramImages[MAX_FRAGMENT_IMAGES];

	idMegaTexture		*megaTexture;		// handles all the binding and parameter setting
	int 				glslProgram;        // built-in shader is OpenGL shader program handle, otherwise is -(custom shader index + 1) (less than 0)
} newShaderStage_t;

#ifdef _HUMANHEAD
//HUMANHEAD bjk: specular exponent
typedef struct {
	float				exponent;
	float				brightness;
} specData_t;
//HUMANHEAD END
#endif

#ifdef _SPLASHDAMAGE
struct stageVector_t {
	int							registers[4];
	const sdDeclRenderBinding*	renderBinding;
};
const int MAX_STAGE_VECTORS = 32;

struct stageTexture_t {
	idImage*					image;
	const sdDeclRenderBinding*	renderBinding;
};
const int MAX_STAGE_TEXTURES = 16;

struct stageTextureMatrix_t {
	int							matrix[2][3];
	const sdDeclRenderBinding*	renderBinding_s;
	const sdDeclRenderBinding*	renderBinding_t;
};
const int MAX_STAGE_TEXTUREMATRICES = 3;

struct stageParseData_t {
							stageParseData_t() :
								numVectors( 0 ),
								numTextures( 0 ),
								numTextureMatrices( 0 ) {
								memset( vectors, 0, sizeof( vectors ) );
								memset( textures, 0, sizeof( textures ) );
								memset( textureMatrices, 0, sizeof( textureMatrices ) );
								declRenderProgram = NULL;
							}

	int						numVectors;
	stageVector_t			vectors[MAX_STAGE_VECTORS];
	int						numTextures;
	stageTexture_t			textures[MAX_STAGE_TEXTURES];
	int						numTextureMatrices;
	stageTextureMatrix_t	textureMatrices[MAX_STAGE_TEXTUREMATRICES];

	const sdDeclRenderProgram *declRenderProgram;
};

//karin: I see them always constants, maybe not need register
typedef struct {
	int				tint[3];
	int				distortion[4];
	int				fresnel;
	int				glare;
	int				offset[4];
	int				desat;
	int				lerp; // must register
} waterStage_t;
#endif

typedef struct {
	int					conditionRegister;	// if registers[conditionRegister] == 0, skip stage
	stageLighting_t		lighting;			// determines which passes interact with lights
	int					drawStateBits;
	colorStage_t		color;
	bool				hasAlphaTest;
	int					alphaTestRegister;
	textureStage_t		texture;
	stageVertexColor_t	vertexColor;
	bool				ignoreAlphaTest;	// this stage should act as translucent, even
	// if the surface is alpha tested
	float				privatePolygonOffset;	// a per-stage polygon offset

	newShaderStage_t	*newStage;			// vertex / fragment program based stage

#ifdef _RAVEN
	// RAVEN BEGIN
// rjohnson: new shader stage system
	class rvNewShaderStage	*newShaderStage;
#endif

#ifdef _HUMANHEAD
    bool				isGlow; // HUMANHEAD CJR:  Glow overlay
    bool				isScopeView; // HUMANHEAD CJR:  Scope view
    bool				isShuttleView;	// HUMANHEAD pdm: shuttle view
    bool				isNotScopeView; // HUMANHEAD CJR:  Does not show up in scope view
    bool				isSpiritWalk; // HUMANHEAD CJR: Spiritwalk view
    bool				isNotSpiritWalk; // HUMANHEAD CJR:  Does not show up in spirit view

    specData_t			specular;	//HUMANHEAD bjk: specular exponent
#endif

#ifdef _SPLASHDAMAGE
	float				lineWidth;

	const sdRenderProgram*		renderProgram;

	int							numVectors;
	stageVector_t*				vectors;
	int							numTextures;
	stageTexture_t*				textures;
	int							numTextureMatrices;
	stageTextureMatrix_t*		textureMatrices;
	int							destinationBuffer; //-1 for normal rendering
#endif
} shaderStage_t;
#ifdef _SPLASHDAMAGE
typedef shaderStage_t materialStage_t;
#endif

typedef enum {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum {
#ifdef _RAVEN
	SS_MIN = -10000,
// RAVEN BEGIN
	SS_SUBVIEW = -4,	// mirrors, viewscreens, etc
	SS_PREGUI = -3,		// guis
// RAVEN END
#else
	SS_SUBVIEW = -3,	// mirrors, viewscreens, etc
#endif
#ifdef _SPLASHDAMAGE
	SS_BAD = -1,
	SS_OPAQUEFIRST,		// opaque
	SS_OPAQUE,			// opaque
	SS_OPAQUENEARER,	// used for materials with expensive shaders, such as the megaTexture
	SS_OPAQUENEAREST,	// used for materials that definitely should be rendered 'last'
	SS_DECAL,			// scorch marks, etc.

	SS_GUI,		// guis

	SS_REFRACTABLE,			// Translucent, but drawn before refraction surfaces
	SS_REFRACTION,			// Stage using refraction screen copy to texture

	SS_FAR_PRE_ATMOS,		// Translucent materials drawn before the atmosphere, this is usally not what we want
	SS_MEDIUM_PRE_ATMOS,	// as translucent mats don't fill the z-buffer and thus get fogged like what's behind them
	SS_CLOSE_PRE_ATMOS,		// instead of for their own depth.

	//
	//	Anything whith the following sort orders will need to be fogged "manually" (i.e. by setting up the shader properly)
	//
	SS_ATMOSPHERE,			// Not really used; just as a new phase

	SS_PORTAL_SKY, // compat for DOOM3
#else
	SS_GUI = -2,		// guis
	SS_BAD = -1,
	SS_OPAQUE,			// opaque

	SS_PORTAL_SKY,

	SS_DECAL,			// scorch marks, etc.
#endif

	SS_FAR,
	SS_MEDIUM,			// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST,			// screen blood blobs

	SS_POST_PROCESS = 100	// after a screen copy to texture
} materialSort_t;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES			= 256;

const int MAX_TEXGEN_REGISTERS		= 4;

#ifdef _HUMANHEAD
const int MAX_ENTITY_SHADER_PARMS	= 13; // HUMANHEAD pdm: increased from 12
#else
const int MAX_ENTITY_SHADER_PARMS	= 12;
#endif

// material flags
typedef enum {
	MF_DEFAULTED				= BIT(0),
	MF_POLYGONOFFSET			= BIT(1),
	MF_NOSHADOWS				= BIT(2),
	MF_FORCESHADOWS				= BIT(3),
	MF_NOSELFSHADOW				= BIT(4),
	MF_NOPORTALFOG				= BIT(5),	// this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE			= BIT(6)	// in use (visible) per editor
#ifdef _RAVEN
// RAVEN BEGIN
// jscott: for portal skies
	,
	MF_SKY						= BIT(7),
	MF_NEED_CURRENT_RENDER		= BIT(8)	// for hud guis that need sort order preseved but need back end too
// RAVEN END
#endif
#ifdef _HUMANHEAD
	,
    MF_USESDISTANCE				= BIT(7),	// HUMANHEAD pdm: distance optimization
	MF_LIGHT_WHOLE_MESH			= BIT(8),	// HUMANHEAD bjk: dont cull tris with light bounds
    //HUMANHEAD PCF rww 05/11/06 - can be used explicitly by surfaces which use alpha coverage but do not want collision anyway
    MF_SKIPCLIP                 = BIT(9)
		//HUMANHEAD END
#endif
#ifdef _SPLASHDAMAGE
	,
    MF_NOAMBIENT					= BIT(7),	// No cubemap ambient light on this shader
    MF_NOATMOSPHERE					= BIT(7),	// No extinction/inscattering on this shader
    MF_FORCETANGENTS				= BIT(8),	// Force tangents and normal calculation
    MF_CLUSTERTRANSFORM				= BIT(9),	// the vertex shader will do the final offseting for the cluster model
    MF_FORCEATMOSPHERE				= BIT(10),	// No extinction/inscattering on this shader
    MF_FLIPBACKSIDENORMALS			= BIT(11),	// hack to make foliage light better, for twosided materials give one side flipped normals
    MF_FULLSCREENPOSTPROCESS		= BIT(12),	// Don't modify depth and always pass the depth test for post processing materials (using SS_POSTPROCESS sort)
    MF_NOHWSKINNING					= BIT(13),	// Don't use hardware skinning, use the SIMD code instead
    MF_OCCLUSION_OCCLUDE			= BIT(14),
    MF_OCCLUSION_QUERY				= BIT(15),
    MF_NOSURFACEMERGE				= BIT(16),	// Don't merge MD5 mesh surface which have this set
    MF_SHADOWMAPPED					= BIT(17),	// This light uses shadow maps
    MF_VERTEXPOSITIONONLY			= BIT(18),	// Ignore UV, color, etc data from source models when loaded
    MF_ONLYATMOSPHEREINTERACTION	= BIT(19),
    MF_NOATMOSPHEREINTERACTION		= BIT(20),
    MF_ADVERT						= BIT(21),
    MF_FORCESOURCENORMALS			= BIT(22),
    MF_BAKEDINATMOSLIGHTCOL			= BIT(23),
    MF_TRANSLUCENTINTERACTION		= BIT(24),
    MF_RECEIVESLIGHTINGONBACKSIDES	= BIT(25),
    MF_LOWRANGEUVCOMPRESS			= BIT(26),
    MF_UPDATECURRENTRENDER			= BIT(27),
    MF_SHADOWSCASTONLYFROMSTATICOBJECTS = BIT(28),
    MF_HASMEGA						= BIT(29),
    MF_NOIMPLICITSTAGES				= BIT(30),
#endif
} materialFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
typedef enum {
#ifdef _RAVEN //karin: must sync with script/defs.script | cm/CollisionModel_debug.cpp
	CONTENTS_SOLID				= BIT(0),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT(1),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT(2),	// used for water
	CONTENTS_PLAYERCLIP			= BIT(3),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT(4),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT(5),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT(6),	// solid to IK
	CONTENTS_BLOOD				= BIT(7),	// used to detect blood decals
	CONTENTS_BODY				= BIT(8),	// used for actors
	CONTENTS_PROJECTILE			= BIT(9),	// used for projectiles
	CONTENTS_CORPSE				= BIT(10),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT(11),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT(12),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT(13),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT(14),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER	= BIT(15),	// used for triggers that are activated by the flashlight
// RAVEN BEGIN
// bdube: new clip that blocks monster visibility
	CONTENTS_SIGHTCLIP			= BIT(16),	// used for blocking sight for actors and cameras
	CONTENTS_LARGESHOTCLIP		= BIT(17),	// used to block large shots (fence that allows bullets through but not rockets for example)
// cdr: AASTactical
	CONTENTS_NOTACTICALFEATURES	= BIT(18),	// don't place tactical features here
	CONTENTS_VEHICLECLIP		= BIT(19),	// solid to vehicles

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT(20),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT(21),	// don't cut this brush with CSG operations in the editor
	CONTENTS_FLYCLIP			= BIT(22),	// solid to vehicles

// mekberg: added
	CONTENTS_ITEMCLIP			= BIT(23),	// so items can collide
	CONTENTS_PROJECTILECLIP		= BIT(24),  // unlike contents_projectile, projectiles only NOT hitscans
// RAVEN END
	/* // jmarshall23's botAI, not q4 sdk
		CONTENTS_FOG				= BIT(25),
		CONTENTS_LAVA				= BIT(26),
		CONTENTS_SLIME				= BIT(27),*/
#elif defined(_HUMANHEAD) //karin: must sync with script/defs.script | cm/CollisionModel_debug.cpp
	CONTENTS_SOLID				= BIT(0),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT(1),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT(2),	// used for water
	CONTENTS_PLAYERCLIP			= BIT(3),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT(4),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT(5),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT(6),	// solid to IK
	CONTENTS_BLOOD				= BIT(7),	// used to detect blood decals
	CONTENTS_BODY				= BIT(8),	// used for actors
	CONTENTS_PROJECTILE			= BIT(9),	// used for projectiles
	CONTENTS_CORPSE				= BIT(10),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT(11),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT(12),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT(13),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT(14),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER	= BIT(15),	// used for triggers that are activated by the flashlight

	// HUMANHEAD CJR: Content flags.  Note that for simplicity of merging, id's areaportal and nocsg flags were left as is
	CONTENTS_FORCEFIELD			= BIT(16),	// forcefield matter, only passable in spirit mode
	CONTENTS_SPIRITBRIDGE		= BIT(17),	// cjr - Collidable only by spiritwalking players
	// END HUMANHEAD

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT(18),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT(19),	// don't cut this brush with CSG operations in the editor

	// HUMANHEAD CJR: Content flags.  Note that for simplicity of merging, id's areaportal and nocsg flags were left as is
	CONTENTS_BLOCK_RADIUSDAMAGE = BIT(20),	// aob - used by objects like forcefields and chaff
	CONTENTS_SHOOTABLE			= BIT(21),	// pdm - bullets collide with but not player or monsters
	CONTENTS_DEATHVOLUME		= BIT(22),	// AOB: used by death zones so the player can do a simple contents check
	CONTENTS_VEHICLECLIP		= BIT(23),	// PDM: used to clip off vehicle movement
	CONTENTS_OWNER_TO_OWNER		= BIT(24),	// bjk: used to disable owner to owner rejection for collision
	CONTENTS_GAME_PORTAL		= BIT(25),  // cjr: used for clipping against game portals (glow portals, etc)
	CONTENTS_SHOOTABLEBYARROW	= BIT(26),	// pdm: solid to spirit arrows specifically as opposed to other projectiles
	CONTENTS_HUNTERCLIP			= BIT(27),	// pdm: solid to hunters, but not hunters in vehicles
	// HUMANHEAD END
#elif defined(_SPLASHDAMAGE) //karin: must sync with script/defs.script | cm/CollisionModel_debug.cpp
	CONTENTS_SOLID				= BIT(0),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT(1),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT(2),	// used for water
	CONTENTS_PLAYERCLIP			= BIT(3),	// solid to players
	CONTENTS_WALKERCLIP			= BIT(4),	// solid to walkers
	CONTENTS_MOVEABLECLIP		= BIT(5),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT(6),	// solid to IK
	CONTENTS_SLIDEMOVER			= BIT(7),	// contains players & vehicles that move like players (with a SlideMove)
	CONTENTS_BODY				= BIT(8),	// used for actors
	CONTENTS_PROJECTILE			= BIT(9),	// used for projectiles
	CONTENTS_CORPSE				= BIT(10),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT(11),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT(12),	// used for triggers
	CONTENTS_VEHICLECLIP		= BIT(13),	// solid to vehicles
	CONTENTS_EXPLOSIONSOLID		= BIT(14),	// used for selection traces
	CONTENTS_MONSTER			= BIT(15),	// monster physics
	CONTENTS_FORCEFIELD			= BIT(16),	// these can be hit by projectiles but other things can move through them
	CONTENTS_SHADOWCOLLISION	= BIT(17),
	CONTENTS_CROSSHAIRSOLID		= BIT(18),
	CONTENTS_FLYERHIVECLIP		= BIT(19),	// flyer hive only

	// AAS
	CONTENTS_AAS_SOLID_PLAYER	= BIT(24),	//
	CONTENTS_AAS_SOLID_VEHICLE	= BIT(25),	//
	CONTENTS_AAS_CLUSTER_PORTAL	= BIT(26),	//
	CONTENTS_AAS_OBSTACLE		= BIT(27),	//

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT(28),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT(29),	// don't cut this brush with CSG operations in the editor
	CONTENTS_OCCLUDER			= BIT(30),	// occluder brushes for outdoor occlusion
#define CONTENTS_MONSTERCLIP CONTENTS_MONSTER
#define CONTENTS_AAS_SOLID (CONTENTS_AAS_SOLID_PLAYER|CONTENTS_AAS_SOLID_VEHICLE)
#else
	CONTENTS_SOLID				= BIT(0),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT(1),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT(2),	// used for water
	CONTENTS_PLAYERCLIP			= BIT(3),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT(4),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT(5),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT(6),	// solid to IK
	CONTENTS_BLOOD				= BIT(7),	// used to detect blood decals
	CONTENTS_BODY				= BIT(8),	// used for actors
	CONTENTS_PROJECTILE			= BIT(9),	// used for projectiles
	CONTENTS_CORPSE				= BIT(10),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT(11),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT(12),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT(13),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT(14),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER	= BIT(15),	// used for triggers that are activated by the flashlight

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT(20),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT(21),	// don't cut this brush with CSG operations in the editor
#endif

	CONTENTS_REMOVE_UTIL		= ~(CONTENTS_AREAPORTAL|CONTENTS_NOCSG)
} contentsFlags_t;

// surface types
const int NUM_SURFACE_BITS		= 4;
const int MAX_SURFACE_TYPES		= 1 << NUM_SURFACE_BITS;

typedef enum {
	SURFTYPE_NONE,					// default type
	SURFTYPE_METAL,
	SURFTYPE_STONE,
	SURFTYPE_FLESH,
	SURFTYPE_WOOD,
	SURFTYPE_CARDBOARD,
	SURFTYPE_LIQUID,
	SURFTYPE_GLASS,
#ifdef _HUMANHEAD
	SURFTYPE_TILE,
	SURFTYPE_WALLWALK,
	SURFTYPE_ALTMETAL,
	SURFTYPE_FORCEFIELD,
	SURFTYPE_PIPE,
	SURFTYPE_SPIRIT,
	SURFTYPE_CHAFF,
	NUM_SURFACE_TYPES
#else
	SURFTYPE_PLASTIC,
	SURFTYPE_RICOCHET,
	SURFTYPE_10,
	SURFTYPE_11,
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
#endif
} surfTypes_t;

// surface flags
typedef enum {
#ifdef _SPLASHDAMAGE //karin: must sync with script/defs.script
    SURF_NODAMAGE				= BIT(0),	// never give falling damage
    SURF_SLICK					= BIT(1),	// affects game physics
    SURF_COLLISION				= BIT(2),	// collision surface
    SURF_LADDER					= BIT(3),	// player can climb up this surface
    SURF_NOIMPACT				= BIT(4),	// don't make missile explosions
    SURF_NOSTEPS				= BIT(5),	// no footstep sounds
    SURF_DISCRETE				= BIT(6),	// not clipped or merged by utilities
    SURF_NOFRAGMENT				= BIT(7),	// dmap won't cut surface at each bsp boundary
    SURF_NULLNORMAL				= BIT(8),	// renderbump will draw this surface as 0x80 0x80 0x80, which
    // won't collect light from any angle
    SURF_NONSOLID				= BIT(9),
    SURF_NOPLANT				= BIT(10),	// can't plant landmines, HE charges etc here
    SURF_NOAREAS				= BIT(11),	// don't create AAS areas on this surface

    SURF_SHADOWCOLLISION		= BIT(12),	// shadow collision surface, used only when CONTENTS_SHADOWCOLLISION is specified
#else
	SURF_TYPE_BIT0				= BIT(0),	// encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1				= BIT(1),	// "
	SURF_TYPE_BIT2				= BIT(2),	// "
	SURF_TYPE_BIT3				= BIT(3),	// "
	SURF_TYPE_MASK				= (1 << NUM_SURFACE_BITS) - 1,

	SURF_NODAMAGE				= BIT(4),	// never give falling damage
	SURF_SLICK					= BIT(5),	// effects game physics
	SURF_COLLISION				= BIT(6),	// collision surface
	SURF_LADDER					= BIT(7),	// player can climb up this surface
	SURF_NOIMPACT				= BIT(8),	// don't make missile explosions
	SURF_NOSTEPS				= BIT(9),	// no footstep sounds
	SURF_DISCRETE				= BIT(10),	// not clipped or merged by utilities
	SURF_NOFRAGMENT				= BIT(11),	// dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL				= BIT(12)	// renderbump will draw this surface as 0x80 0x80 0x80, which
#ifdef _RAVEN //karin: must sync with script/defs.script
	// RAVEN BEGIN
// bdube: added bounce
	, SURF_BOUNCE				= BIT(13),  // projectiles should bounce off this surface

// dluetscher: added no T fix
	SURF_NO_T_FIX				= BIT(14),	// merge surfaces (like decals), but does not try to T-fix them

// RAVEN END
#endif
	// won't collect light from any angle
#endif
} surfaceFlags_t;

#ifdef _SPLASHDAMAGE
// portal flags
typedef enum {
    PORTAL_VIS,						// block visibility, splits surfaces
    PORTAL_OUTSIDE,					// defines a border between an outside and an inside area
    PORTAL_BLOCKAMBIENT,			// defines ambient light sectors
    PORTAL_AUDIO,					// defines sound sectors
    PORTAL_PLAYZONE,				// defines playzone areas
    PORTAL_OCCTEST,					// enables occlusion testing on portal
    NUM_PORTAL_FLAGS
} portalFlags_t;
#endif

class idSoundEmitter;

class idMaterial : public idDecl
{
#ifdef _RAVEN
// rjohnson: new shader stage system
	friend class rvNewShaderStage;
// RAVEN END
#endif
	public:
		idMaterial();
		virtual				~idMaterial();

		virtual size_t		Size(void) const;
		virtual bool		SetDefaultText(void);
		virtual const char *DefaultDefinition(void) const;
#ifdef _RAVEN
		virtual bool		Parse(const char *text, const int textLength, bool noCaching = false);
#else
		virtual bool		Parse(const char *text, const int textLength);
#endif
		virtual void		FreeData(void);
		virtual void		Print(void) const;

		//BSM Nerve: Added for material editor
		bool				Save(const char *fileName = NULL);

		// returns the internal image name for stage 0, which can be used
		// for the renderer CaptureRenderToImage() call
		// I'm not really sure why this needs to be virtual...
		virtual const char	*ImageName(void) const;

		void				ReloadImages(bool force) const;

		// returns number of stages this material contains
		const int			GetNumStages(void) const {
			return numStages;
		}

		// get a specific stage
		const shaderStage_t *GetStage(const int index) const {
			assert(index >= 0 && index < numStages);
			return &stages[index];
		}

		// get the first bump map stage, or NULL if not present.
		// used for bumpy-specular
		const shaderStage_t *GetBumpStage(void) const;

		// returns true if the material will draw anything at all.  Triggers, portals,
		// etc, will not have anything to draw.  A not drawn surface can still castShadow,
		// which can be used to make a simplified shadow hull for a complex object set
		// as noShadow
		bool				IsDrawn(void) const {
			return (numStages > 0 || entityGui != 0 || gui != NULL);
		}

		// returns true if the material will draw any non light interaction stages
		bool				HasAmbient(void) const {
			return (numAmbientStages > 0);
		}

		// returns true if material has a gui
		bool				HasGui(void) const {
			return (entityGui != 0 || gui != NULL);
		}

		// returns true if the material will generate another view, either as
		// a mirror or dynamic rendered image
		bool				HasSubview(void) const {
			return hasSubview;
		}

		// returns true if the material will generate shadows, not making a
		// distinction between global and no-self shadows
		bool				SurfaceCastsShadow(void) const {
			return TestMaterialFlag(MF_FORCESHADOWS) || !TestMaterialFlag(MF_NOSHADOWS);
		}

		// returns true if the material will generate interactions with fog/blend lights
		// All non-translucent surfaces receive fog unless they are explicitly noFog
		bool				ReceivesFog(void) const {
			return (IsDrawn() && !noFog && coverage != MC_TRANSLUCENT);
		}

		// returns true if the material will generate interactions with normal lights
		// Many special effect surfaces don't have any bump/diffuse/specular
		// stages, and don't interact with lights at all
		bool				ReceivesLighting(void) const {
			return numAmbientStages != numStages;
		}

		// returns true if the material should generate interactions on sides facing away
		// from light centers, as with noshadow and noselfshadow options
		bool				ReceivesLightingOnBackSides(void) const {
			return (materialFlags & (MF_NOSELFSHADOW|MF_NOSHADOWS)) != 0;
		}

		// Standard two-sided triangle rendering won't work with bump map lighting, because
		// the normal and tangent vectors won't be correct for the back sides.  When two
		// sided lighting is desired. typically for alpha tested surfaces, this is
		// addressed by having CleanupModelSurfaces() create duplicates of all the triangles
		// with apropriate order reversal.
		bool				ShouldCreateBackSides(void) const {
			return shouldCreateBackSides;
		}

		// characters and models that are created by a complete renderbump can use a faster
		// method of tangent and normal vector generation than surfaces which have a flat
		// renderbump wrapped over them.
		bool				UseUnsmoothedTangents(void) const {
			return unsmoothedTangents;
		}

		// by default, monsters can have blood overlays placed on them, but this can
		// be overrided on a per-material basis with the "noOverlays" material command.
		// This will always return false for translucent surfaces
		bool				AllowOverlays(void) const {
			return allowOverlays;
		}

		// MC_OPAQUE, MC_PERFORATED, or MC_TRANSLUCENT, for interaction list linking and
		// dmap flood filling
		// The depth buffer will not be filled for MC_TRANSLUCENT surfaces
		// FIXME: what do nodraw surfaces return?
		materialCoverage_t	Coverage(void) const {
			return coverage;
		}

		// returns true if this material takes precedence over other in coplanar cases
		bool				HasHigherDmapPriority(const idMaterial &other) const {
			return (IsDrawn() && !other.IsDrawn()) ||
			       (Coverage() < other.Coverage());
		}

		// returns a idUserInterface if it has a global gui, or NULL if no gui
		idUserInterface		*GlobalGui(void) const {
			return gui;
		}

		// a discrete surface will never be merged with other surfaces by dmap, which is
		// necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
		// special effects from being combined into a single surface
		// guis, merging sprites or other effects, mirrors and remote views are always discrete
		bool				IsDiscrete(void) const {
			return (entityGui || gui || deform != DFRM_NONE || sort == SS_SUBVIEW ||
			        (surfaceFlags & SURF_DISCRETE) != 0);
		}

		// Normally, dmap chops each surface by every BSP boundary, then reoptimizes.
		// For gigantic polygons like sky boxes, this can cause a huge number of planar
		// triangles that make the optimizer take forever to turn back into a single
		// triangle.  The "noFragment" option causes dmap to only break the polygons at
		// area boundaries, instead of every BSP boundary.  This has the negative effect
		// of not automatically fixing up interpenetrations, so when this is used, you
		// should manually make the edges of your sky box exactly meet, instead of poking
		// into each other.
		bool				NoFragment(void) const {
			return (surfaceFlags & SURF_NOFRAGMENT) != 0;
		}

		//------------------------------------------------------------------
		// light shader specific functions, only called for light entities

		// lightshader option to fill with fog from viewer instead of light from center
		bool				IsFogLight() const {
			return fogLight;
		}

		// perform simple blending of the projection, instead of interacting with bumps and textures
		bool				IsBlendLight() const {
			return blendLight;
		}

		// an ambient light has non-directional bump mapping and no specular
		bool				IsAmbientLight() const {
			return ambientLight;
		}

		// implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
		// but individual light entities can also override this value
		bool				LightCastsShadows() const {
			return TestMaterialFlag(MF_FORCESHADOWS) ||
			       (!fogLight && !ambientLight && !blendLight && !TestMaterialFlag(MF_NOSHADOWS));
		}

		// fog lights, blend lights, ambient lights, etc will all have to have interaction
		// triangles generated for sides facing away from the light as well as those
		// facing towards the light.  It is debatable if noshadow lights should effect back
		// sides, making everything "noSelfShadow", but that would make noshadow lights
		// potentially slower than normal lights, which detracts from their optimization
		// ability, so they currently do not.
		bool				LightEffectsBackSides() const {
			return fogLight || ambientLight || blendLight;
		}

		// NULL unless an image is explicitly specified in the shader with "lightFalloffShader <image>"
		idImage				*LightFalloffImage() const {
			return lightFalloffImage;
		}

		//------------------------------------------------------------------

		// returns the renderbump command line for this shader, or an empty string if not present
		const char 		*GetRenderBump() const {
			return renderBump;
		};

		// set specific material flag(s)
		void				SetMaterialFlag(const int flag) const {
			materialFlags |= flag;
		}

		// clear specific material flag(s)
		void				ClearMaterialFlag(const int flag) const {
			materialFlags &= ~flag;
		}

		// test for existance of specific material flag(s)
		bool				TestMaterialFlag(const int flag) const {
			return (materialFlags & flag) != 0;
		}

		// get content flags
		const int			GetContentFlags(void) const {
			return contentFlags;
		}

		// get surface flags
		const int			GetSurfaceFlags(void) const {
			return surfaceFlags;
		}

		// gets name for surface type (stone, metal, flesh, etc.)
#ifdef _SPLASHDAMAGE
		const class sdDeclSurfaceType*		GetSurfaceType( void ) const { return surfaceTypeDecl; }
		const class sdDeclSurfaceTypeMap*	GetSurfaceTypeMapDecl( void ) const { return surfaceTypeMapDecl; }
		//const class sdSurfaceTypeMap*		GetSurfaceTypeMap( void ) const { return surfaceTypeMap; }
		const idVec3&		GetSurfaceColor( void ) const { return surfaceColor; }
#else
		const surfTypes_t	GetSurfaceType(void) const {
			return static_cast<surfTypes_t>(surfaceFlags & SURF_TYPE_MASK);
		}
#endif

		// get material description
		const char 		*GetDescription(void) const {
			return desc;
		}

		// get sort order
		const float			GetSort(void) const {
			return sort;
		}
		// this is only used by the gui system to force sorting order
		// on images referenced from tga's instead of materials.
		// this is done this way as there are 2000 tgas the guis use
		void				SetSort(float s) const {
			sort = s;
		};

		// DFRM_NONE, DFRM_SPRITE, etc
		deform_t			Deform(void) const {
			return deform;
		}

		// flare size, expansion size, etc
		const int			GetDeformRegister(int index) const {
			return deformRegisters[index];
		}

		// particle system to emit from surface and table for turbulent
		const idDecl		*GetDeformDecl(void) const {
			return deformDecl;
		}

		// currently a surface can only have one unique texgen for all the stages
		texgen_t			Texgen() const;

		// wobble sky parms
		const int 			*GetTexGenRegisters(void) const {
			return texGenRegisters;
		}

		// get cull type
		const cullType_t	GetCullType(void) const {
			return cullType;
		}

		float				GetEditorAlpha(void) const {
			return editorAlpha;
		}

		int					GetEntityGui(void) const {
			return entityGui;
		}

		decalInfo_t			GetDecalInfo(void) const {
			return decalInfo;
		}

		// spectrums are used for "invisible writing" that can only be
		// illuminated by a light of matching spectrum
		int					Spectrum(void) const {
			return spectrum;
		}

		float				GetPolygonOffset(void) const {
			return polygonOffset;
		}

		float				GetSurfaceArea(void) const {
			return surfaceArea;
		}
		void				AddToSurfaceArea(float area) {
			surfaceArea += area;
		}

		//------------------------------------------------------------------

		// returns the length, in milliseconds, of the videoMap on this material,
		// or zero if it doesn't have one
		int					CinematicLength(void) const;

		void				CloseCinematic(void) const;

		void				ResetCinematicTime(int time) const;

		void				UpdateCinematic(int time) const;

		//------------------------------------------------------------------

		// gets an image for the editor to use
#ifdef _SPLASHDAMAGE //karin: called in game, make virtual
		virtual 
#endif
		idImage 			*GetEditorImage(void) const;
		int					GetImageWidth(void) const;
		int					GetImageHeight(void) const;

		void				SetGui(const char *_gui) const;

		// just for resource tracking
		void				SetImageClassifications(int tag) const;

		//------------------------------------------------------------------

		// returns number of registers this material contains
		const int			GetNumRegisters() const {
			return numRegisters;
		}

		// regs should point to a float array large enough to hold GetNumRegisters() floats
		void				EvaluateRegisters(float *regs, const float entityParms[MAX_ENTITY_SHADER_PARMS],
		                const struct viewDef_s *view, idSoundEmitter *soundEmitter = NULL) const;

		// if a material only uses constants (no entityParm or globalparm references), this
		// will return a pointer to an internal table, and EvaluateRegisters will not need
		// to be called.  If NULL is returned, EvaluateRegisters must be used.
		const float 		*ConstantRegisters() const;

		bool				SuppressInSubview() const				{
			return suppressInSubview;
		};
		bool				IsPortalSky() const						{
			return portalSky;
		};
		void				AddReference();
#ifdef _RAVEN // quake4 material
// RAVEN BEGIN
// dluetscher: added SURF_NO_T_FIX to merge surfaces (like decals), but skipping any T-junction fixing
            bool				NoTFix( void ) const { return ( surfaceFlags & SURF_NO_T_FIX ) != 0; }
// RAVEN END

            const rvDeclMatType* GetMaterialType(void) const { return(materialType); }
// RAVEN BEGIN
// rjohnson: added vertex randomizing
        // regs should point to a float array large enough to hold GetNumRegisters() floats
        void				EvaluateRegisters( float *regs, const float entityParms[MAX_ENTITY_SHADER_PARMS],
                                                    const struct viewDef_s *view, int soundEmitter = 0, idVec3 *randomizer = NULL ) const;
        // RAVEN END
#endif
#ifdef _HUMANHEAD
// HUMANHEAD tmj: returns how the subview should be rendered (i.e. mirror/portal/skybox)
        subviewClass_t		GetSubviewClass( void) const { return subviewClass; }
        int					GetDirectPortalDistance() const { return directPortalDistance; } // HUMANHEAD CJR:  direct render portal distance cull
#endif
#ifdef _NO_LIGHT
		bool IsNoLight(void) const { return noLight; }
#endif
#ifdef _SPLASHDAMAGE
    	static void			CacheFromDict( const idDict& dict );
#endif

	private:
		// parse the entire material
		void				CommonInit();
#ifdef _SPLASHDAMAGE
		void				ParseMaterial(idParser &src);
		bool				MatchToken(idParser &src, const char *match);
		void				ParseSort(idParser &src);
		void				ParseBlend(idParser &src, shaderStage_t *stage);
		void				ParseVertexParm(idParser &src, newShaderStage_t *newStage);
		void				ParseFragmentMap(idParser &src, newShaderStage_t *newStage);
		void				ParseStage(idParser &src, const textureRepeat_t trpDefault = TR_REPEAT);
		void				ParseDeform(idParser &src);
		void				ParseDecalInfo(idParser &src);
#else
		void				ParseMaterial(idLexer &src);
		bool				MatchToken(idLexer &src, const char *match);
		void				ParseSort(idLexer &src);
		void				ParseBlend(idLexer &src, shaderStage_t *stage);
		void				ParseVertexParm(idLexer &src, newShaderStage_t *newStage);
		void				ParseFragmentMap(idLexer &src, newShaderStage_t *newStage);
		void				ParseStage(idLexer &src, const textureRepeat_t trpDefault = TR_REPEAT);
		void				ParseDeform(idLexer &src);
		void				ParseDecalInfo(idLexer &src);
#endif
		bool				CheckSurfaceParm(idToken *token);
		int					GetExpressionConstant(float f);
		int					GetExpressionTemporary(void);
		expOp_t				*GetExpressionOp(void);
		int					EmitOp(int a, int b, expOpType_t opType);
#ifdef _SPLASHDAMAGE
		int					ParseEmitOp(idParser &src, int a, expOpType_t opType, int priority);
		int					ParseTerm(idParser &src);
		int					ParseExpressionPriority(idParser &src, int priority);
		int					ParseExpression(idParser &src);
#else
		int					ParseEmitOp(idLexer &src, int a, expOpType_t opType, int priority);
		int					ParseTerm(idLexer &src);
		int					ParseExpressionPriority(idLexer &src, int priority);
		int					ParseExpression(idLexer &src);
#endif
		void				ClearStage(shaderStage_t *ss);
		int					NameToSrcBlendMode(const idStr &name);
		int					NameToDstBlendMode(const idStr &name);
		void				MultiplyTextureMatrix(textureStage_t *ts, int registers[2][3]);	// FIXME: for some reason the const is bad for gcc and Mac
		void				SortInteractionStages();
		void				AddImplicitStages(const textureRepeat_t trpDefault = TR_REPEAT);
		void				CheckForConstantRegisters();
#if defined(_GLSL_PROGRAM) || defined(_RAVEN) || defined(_HUMANHEAD) //karin: fragment shader parms
		void				ParseFragmentParm(idLexer &src, newShaderStage_t *newStage);
#endif
#ifdef _SPLASHDAMAGE
		void				ParseFragmentParm(idParser &src, newShaderStage_t *newStage);
#endif
#ifdef _GLSL_PROGRAM
#ifdef _SPLASHDAMAGE
		void				ParseGLSLProgram(idParser &src, newShaderStage_t *newStage);
#else
        void                ParseGLSLProgram(idLexer &src, newShaderStage_t *newStage);
#endif
#endif

#ifdef _SPLASHDAMAGE
		void				ParseFillMode( idParser &src, materialStage_t *stage );
		void				ParseCullFace( idParser &src, materialStage_t *stage );

		materialStage_t *	AllocAndCopyStage(const materialStage_t *ss);
        void				CompleteStage( materialStage_t* ms, stageParseData_t& spd, const sdDeclRenderBinding** defaults, const int numDefaults );
        void				CompleteInterationStage( materialStage_t *ss, stageParseData_t& spd );
        void				FinishStage( materialStage_t* ms, stageParseData_t& spd );
        int					ParseProgramStageVector( idParser &src, stageParseData_t& spd, const sdDeclRenderBinding *binding = NULL );
        int					ParseProgramStageTexture( idParser &src, stageParseData_t& spd, const sdDeclRenderBinding *binding = NULL );
        int					ParseProgramStageMatrix( idParser &src, stageParseData_t& spd );
		bool				ParseConstantCVarExpression( idParser& src, bool& result );
#endif

	private:
		idStr				desc;				// description
		idStr				renderBump;			// renderbump command options, without the "renderbump" at the start

		idImage				*lightFalloffImage;

		int					entityGui;			// draw a gui with the idUserInterface from the renderEntity_t
		// non zero will draw gui, gui2, or gui3 from renderEnitty_t
		mutable idUserInterface	*gui;			// non-custom guis are shared by all users of a material
#ifdef _RAVEN // quake4 material
// RAVEN BEGIN
// jscott: for material types
        const rvDeclMatType* materialType;
        byte* materialTypeArray;	// an array of material type indices generated from the hit image
        idStr				materialTypeArrayName;
        int					MTAWidth;
        int					MTAHeight;

// rjohnson: started tracking image/material usage
        int					useCount;
        int					globalUseCount;

// AReis: New portal distance culling stuff.
        float				portalDistanceNear;
        float				portalDistanceFar;
        idImage* portalImage;
// RAVEN END
#endif
#ifdef _HUMANHEAD
        subviewClass_t		subviewClass;		// HUMANHEAD tmj: Type of subview this surface points to
        int					directPortalDistance; // HUMANHEAD:  Distance at which direct render portals are drawn
#endif

		bool				noFog;				// surface does not create fog interactions

		int					spectrum;			// for invisible writing, used for both lights and surfaces

		float				polygonOffset;

		int					contentFlags;		// content flags
		int					surfaceFlags;		// surface flags
		mutable int			materialFlags;		// material flags

		decalInfo_t			decalInfo;


		mutable	float		sort;				// lower numbered shaders draw before higher numbered
		deform_t			deform;
		int					deformRegisters[4];		// numeric parameter for deforms
		const idDecl		*deformDecl;			// for surface emitted particle deforms and tables

		int					texGenRegisters[MAX_TEXGEN_REGISTERS];	// for wobbleSky

		materialCoverage_t	coverage;
		cullType_t			cullType;			// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
		bool				shouldCreateBackSides;

		bool				fogLight;
		bool				blendLight;
		bool				ambientLight;
		bool				unsmoothedTangents;
		bool				hasSubview;			// mirror, remote render, etc
		bool				allowOverlays;

		int					numOps;
		expOp_t 			*ops;				// evaluate to make expressionRegisters

		int					numRegisters;																			//
		float 				*expressionRegisters;

		float 				*constantRegisters;	// NULL if ops ever reference globalParms or entityParms

		int					numStages;
		int					numAmbientStages;

		shaderStage_t 		*stages;

		struct mtrParsingData_s	*pd;			// only used during parsing

		float				surfaceArea;		// only for listSurfaceAreas

		// we defer loading of the editor image until it is asked for, so the game doesn't load up
		// all the invisible and uncompressed images.
		// If editorImage is NULL, it will atempt to load editorImageName, and set editorImage to that or defaultImage
		idStr				editorImageName;
		mutable idImage 	*editorImage;		// image used for non-shaded preview
		float				editorAlpha;

		bool				suppressInSubview;
		bool				portalSky;
		int					refCount;
#ifdef _SPLASHDAMAGE
		const sdDeclSurfaceType*	surfaceTypeDecl;
		const sdDeclSurfaceTypeMap*	surfaceTypeMapDecl;
		//const sdSurfaceTypeMap*		surfaceTypeMap;
		idVec3				surfaceColor;
		bool				writeDepth;
#endif
#ifdef _NO_LIGHT
		bool 				noLight;
#endif
};

typedef idList<const idMaterial *> idMatList;

#endif /* !__MATERIAL_H__ */
