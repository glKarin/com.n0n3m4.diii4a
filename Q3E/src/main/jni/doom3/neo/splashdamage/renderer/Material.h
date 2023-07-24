// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
===============================================================================

	Material

===============================================================================
*/

#include "RendererEnums.h"
#include "../framework/declManager.h"

using namespace sdUtility;

class idImage;
class sdUserInterface;
class idMegaTexture;
class sdDeclRenderBinding;
class sdDeclRenderProgram;
class idSoundEmitter;

typedef struct decalInfo_s {
	int		stayTime;		// msec for no change
	int		fadeTime;		// msec to fade vertex colors over
	idVec4	start;		// vertex color at spawn (possibly out of 0.0 - 1.0 range, will clamp after calc)
	idVec4	end;		// vertex color at fade-out (possibly out of 0.0 - 1.0 range, will clamp after calc)
} decalInfo_t;

typedef enum {
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_FLARE_VCOL,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
	DFRM_PARTICLE,
	DFRM_PARTICLE2,
	DFRM_TURB,
	DFRM_GLOW,
} deform_t;

typedef enum {
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
	DI_REMOTE_RENDER,
	DI_REFRACT_RENDER
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
	OP_TYPE_SOUND,
	OP_TYPE_LOAD
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
	EXP_REG_NUMLIGHTS,
	EXP_REG_NUM_PREDEFINED
} expRegister_t;

typedef struct {
	expOpType_t		opType;
	int				a, b, c;
} expOp_t;

typedef struct {
	int				registers[4];
} colorStage_t;

// deprecated
typedef struct {
	idImage *			image;
	//texgen_t			texgen;
	//effectsVertexProgram_t	program;
	bool				hasMatrix;
	int					matrix[2][3];	// we only allow a subset of the full projection matrix

	// dynamic image variables
	dynamicidImage_t	dynamic;
	int					width, height;
	int					dynamicFrameCount;
} textureStage_t;

struct arbProgramBinding_t {};
// end deprecated


// Subview allows you to set some parameters when the shader has mirrored views rendered
// these mainly limit the amount of objects rendered in the reflection
typedef struct {
	float		boxExpand;			// Expand the surace's bounding box by this amount, only render objects in this box in the subview (-1 for everything)
	float		farPlane;			// Add a far culling plane to the subview camera at the given distance (-1 for none)
	idImage*	backgroundImage;	// Render this cubemap in the background (because so much stuff was culled to avoid H.O.M.)
} subviewInfo_t;

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
typedef enum {
	SVC_IGNORE,
	SVC_MODULATE,
	SVC_MODULATE_ALPHA,
	SVC_INVERSE_MODULATE
} stageVertexColor_t;

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
							}

	int						numVectors;
	stageVector_t			vectors[MAX_STAGE_VECTORS];
	int						numTextures;
	stageTexture_t			textures[MAX_STAGE_TEXTURES];
	int						numTextureMatrices;
	stageTextureMatrix_t	textureMatrices[MAX_STAGE_TEXTUREMATRICES];
};

struct materialStage_t {
	int					conditionRegister;	// if registers[conditionRegister] == 0, skip stage
//	stageLighting_t		lighting;			// determines which passes interact with lights
	int					drawStateBits;
//	colorStage_t		color;
	bool				hasAlphaTest;
	int					alphaTestRegister;
	int					specularPowerRegister;
//	textureStage_t		texture;
	stageVertexColor_t	vertexColor;
//	bool				ignoreAlphaTest;	// this stage should act as translucent, even
											// if the surface is alpha tested
	float				privatePolygonOffset;	// a per-stage polygon offset

//	bool				forceDepthFuncEqual;

	cullType_t			cullType;
	float				lineWidth;
	bool				breakpoint;			// There is a breakpoint on the drawing of this stage
//	bool				forceDepth;			// Force z-writing on certain transparent stages
	bool				hasExplicitDepthFunc; // don't autodetect depthfuncs 
	bool				hasExplicitDepthMask; // don't autodetect depthfuncs 

	const sdDeclRenderProgram*	renderProgram;

	int							numVectors;
	stageVector_t*				vectors;
	int							numTextures;
	stageTexture_t*				textures;
	int							numTextureMatrices;
	stageTextureMatrix_t*		textureMatrices;

	// look these up post-parse, the backend needs to know about these in certain cases
	stageVector_t*				colorVector;
	stageTextureMatrix_t*		diffuseTextureMatrix;

	class sdImageSequence*		imgSequence;
	idMegaTexture*				megaTexture;

	bool						updateCurrentRender;
	bool						depthStage;
	int							destinationBuffer; //-1 for normal rendering
};

typedef enum {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum {
	SS_SUBVIEW = -3,	// mirrors, viewscreens, etc
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
	SS_FAR,
	SS_MEDIUM,				// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,		// gun smoke puffs

	SS_NEAREST,				// screen blood blobs

	SS_POST_PROCESS = 100,	// after a screen copy to texture
	SS_LAST					// needs to be last
} materialSort_t;

typedef enum {
	LS_AMBIENTOCCLUSION = -3,	// Drawn just after ambient light
	LS_REFRACTABLE = -2,		// Drawn before refraction and non-refracting non-light dependent stages 
	LS_BAD = -1,				// For compatibility with normal sorts
	LS_NORMAL,					// Normal interaction stages
	LS_EFFECT,					// Drawn after interaction lights
	LS_LAST
} lightSort_t;

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES			= 256;
const int MAX_ENTITY_SHADER_PARMS	= 12;

// material flags
typedef enum {
	MF_DEFAULTED					= BIT(0),
	MF_POLYGONOFFSET				= BIT(1),
	MF_NOSHADOWS					= BIT(2),
	MF_FORCESHADOWS					= BIT(3),
	MF_NOSELFSHADOW					= BIT(4),
	MF_NOPORTALFOG					= BIT(5),	// this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE				= BIT(6),	// in use (visible) per editor
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
} materialFlags_t;

typedef enum {
	BP_DEPTHFILL				= BIT(0),
	BP_AMBIENT					= BIT(1),
	BP_ATMOSPHERE				= BIT(2),
	BP_INTERACTION				= BIT(3),
	BP_SHADOWBUFFER				= BIT(4),
} breakpointFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
typedef enum {
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
} contentsFlags_t;

// surface flags
typedef enum {
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
} surfaceFlags_t;

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

typedef struct {
	idStr		name;						// String can contain any ascii text (more like a description or whatever)
	enum { T_FLOAT, T_COLOR } parmType;
	union {
		struct {
			float		defaultVal;			// Default value
			float		min;				// Minimum value
			float		max;				// Maximum value
		} floatParm;
		struct {
			float		defaultVal[3];
		} colorParm;
	};
} sdParameterInfo;

class sdDeclSurfaceType;
class sdDeclSurfaceTypeMap;
class sdSurfaceTypeMap;

//TODO: Move this into decllib
class idMaterial : public idDecl {
public:
						idMaterial();
	virtual				~idMaterial();

	virtual size_t		Size( void ) const;
	virtual bool		SetDefaultText( void );
	virtual const char *DefaultDefinition( void ) const;
	virtual bool		Parse( const char *text, const int textLength );
	virtual void		FreeData( void );
	virtual void		Print( void ) const;

	static void			CacheFromDict( const idDict& dict );

	void				ReloadImages( bool force ) const;

						// returns number of stages this material contains
	const int			GetNumStages( void ) const { return numStages; }

						// get a specific stage
	const materialStage_t* GetStage( const int index ) const { assert(index >= 0 && index < numStages); return &stages[index]; }

						// returns true if the material will draw anything at all.  Triggers, portals,
						// etc, will not have anything to draw.  A not drawn surface can still castShadow,
						// which can be used to make a simplified shadow hull for a complex object set
						// as noShadow
	bool				IsDrawn( void ) const { return ( numStages > 0 || entityGui != 0 ); }

						// returns true if the material will draw any non light interaction stages
	bool				HasAmbient( void ) const { return ( numAmbientStages > 0 ); }

						// returns true if material has a gui
	bool				HasGui( void ) const { return ( entityGui != 0 ); }

						// returns true if the material will generate another view, either as
						// a mirror or dynamic rendered image
	bool				HasSubview( void ) const { return hasSubview; }

						// returns true if the material will generate shadows, not making a
						// distinction between global and no-self shadows
	bool				SurfaceCastsShadow( void ) const { return TestMaterialFlag( MF_FORCESHADOWS ) || !TestMaterialFlag( MF_NOSHADOWS ); }

	bool				ShadowsCastOnlyFromStaticObjects( void ) const { return TestMaterialFlag( MF_SHADOWSCASTONLYFROMSTATICOBJECTS ); }

						// returns true if the material will generate interactions with fog/blend lights
						// All non-translucent surfaces receive fog unless they are explicitly noFog
	bool				ReceivesFog( void ) const { return ( IsDrawn() && !noFog && coverage != MC_TRANSLUCENT ); }

						// returns true if the material will generate interactions with normal lights
						// Many special effect surfaces don't have any bump/diffuse/specular
						// stages, and don't interact with lights at all
	bool				ReceivesLighting( void ) const { return numAmbientStages != numStages; }

						// returns true if the material should generate interactions on sides facing away
						// from light centers, as with noshadow and noselfshadow options
	bool				ReceivesLightingOnBackSides( void ) const { return ( materialFlags & (MF_NOSELFSHADOW|MF_NOSHADOWS|MF_RECEIVESLIGHTINGONBACKSIDES) ) != 0; }

						// Standard two-sided triangle rendering won't work with bump map lighting, because
						// the normal and tangent vectors won't be correct for the back sides.  When two
						// sided lighting is desired. typically for alpha tested surfaces, this is
						// addressed by having CleanupModelSurfaces() create duplicates of all the triangles
						// with apropriate order reversal.
	bool				ShouldCreateBackSides( void ) const { return shouldCreateBackSides; }

						// This surface has a different material on the backside
	const idMaterial *	GetBackSideMaterial( void ) const { return backSideMaterial; }

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
						// characters and models that are created by a complete renderbump can use a faster
						// method of tangent and normal vector generation than surfaces which have a flat
						// renderbump wrapped over them.
	bool				UseUnsmoothedTangents( void ) const { return unsmoothedTangents; }
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS

						// by default, monsters can have blood overlays placed on them, but this can
						// be overrided on a per-material basis with the "noOverlays" material command.
						// This will always return false for translucent surfaces
	bool				AllowOverlays( void ) const { return allowOverlays; }

						// MC_OPAQUE, MC_PERFORATED, or MC_TRANSLUCENT, for interaction list linking and
						// dmap flood filling
						// The depth buffer will not be filled for MC_TRANSLUCENT surfaces
						// FIXME: what do nodraw surfaces return?
	materialCoverage_t	Coverage( void ) const { return coverage; }

						// returns true if this material takes precedence over other in coplanar cases
	bool				HasHigherDmapPriority( const idMaterial &other ) const { return ( IsDrawn() && !other.IsDrawn() ) ||
																						( Coverage() < other.Coverage() ); }

						// a discrete surface will never be merged with other surfaces by dmap, which is
						// necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
						// special effects from being combined into a single surface
						// guis, merging sprites or other effects, mirrors and remote views are always discrete
	bool				IsDiscrete( void ) const { return ( entityGui || deform != DFRM_NONE || sort == SS_SUBVIEW ||
												( surfaceFlags & SURF_DISCRETE ) != 0 ); }

						// Normally, dmap chops each surface by every BSP boundary, then reoptimizes.
						// For gigantic polygons like sky boxes, this can cause a huge number of planar
						// triangles that make the optimizer take forever to turn back into a single
						// triangle.  The "noFragment" option causes dmap to only break the polygons at
						// area boundaries, instead of every BSP boundary.  This has the negative effect
						// of not automatically fixing up interpenetrations, so when this is used, you
						// should manually make the edges of your sky box exactly meet, instead of poking
						// into each other.
	bool				NoFragment( void ) const { return ( surfaceFlags & SURF_NOFRAGMENT ) != 0; }

						// Some materials don't receive lighting, but still require full normals (and sometimes tangents)
						// to be calculated for per vertex/fragment effects. This flag makes sure that the normals and
						// tangents are generated at all times.
	bool				ForceTangentsCalculation( void ) const { return ( materialFlags & (MF_FORCETANGENTS) ) != 0; }

	//------------------------------------------------------------------
	// light shader specific functions, only called for light entities

						// lightMaterial option to fill with fog from viewer instead of light from center
	bool				IsFogLight() const { return fogLight; }

						// perform simple blending of the projection, instead of interacting with bumps and textures
	bool				IsBlendLight() const { return blendLight; }

						// implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
						// but individual light entities can also override this value
	bool				LightCastsShadows() const { return TestMaterialFlag( MF_FORCESHADOWS ) ||
								( !fogLight && !blendLight && !TestMaterialFlag( MF_NOSHADOWS ) ); }

						// fog lights, blend lights, ambient lights, etc will all have to have interaction
						// triangles generated for sides facing away from the light as well as those
						// facing towards the light.  It is debatable if noshadow lights should effect back
						// sides, making everything "noSelfShadow", but that would make noshadow lights
						// potentially slower than normal lights, which detracts from their optimization
						// ability, so they currently do not.
	bool				LightEffectsBackSides() const { return fogLight || blendLight; }

						// NULL unless an image is explicitly specified in the shader with "lightFalloffShader <image>"
	idImage	*			LightFalloffImage() const { return lightFalloffImage; }

	// returns true if the material should use static occlusion (i.e. it occludes other objects and is never occlusion culled itself)
	bool				IsOccluder( void ) const { return TestMaterialFlag( MF_OCCLUSION_OCCLUDE ); }
	bool				UseOcclusionQuery( void ) const { return TestMaterialFlag( MF_OCCLUSION_QUERY ); }

	//------------------------------------------------------------------

						// returns the renderbump command line for this shader, or an empty string if not present
	const char *		GetRenderBump() const { return renderBump; };

						// set specific material flag(s)
	void				SetMaterialFlag( const int flag ) const { materialFlags |= flag; }

						// clear specific material flag(s)
	void				ClearMaterialFlag( const int flag ) const { materialFlags &= ~flag; }

						// test for existance of specific material flag(s)
	bool				TestMaterialFlag( const int flag ) const { return ( materialFlags & flag ) != 0; }

						// get content flags
	const int			GetContentFlags( void ) const { return contentFlags; }

						// get surface flags
	const int			GetSurfaceFlags( void ) const { return surfaceFlags; }

						// get material description
	const char *		GetDescription( void ) const { return desc; }

						// get sort order
	const float			GetSort( void ) const { return sort; }

						// DFRM_NONE, DFRM_SPRITE, etc
	deform_t			Deform( void ) const { return deform; }

						// flare size, expansion size, etc
	const int			GetDeformRegister( int index ) const { return deformRegisters[index]; }

						// particle system to emit from surface and table for turbulent
	const idDecl*		GetDeformDecl( void ) const { return deformDecl; }

						// get cull type
	const cullType_t	GetCullType( void ) const { return cullType; }

	float				GetEditorAlpha( void ) const { return editorAlpha; }

	int					GetEntityGui( void ) const { return entityGui; }

						// get portal flags
	const int			GetPortalFlags() const { return portalFlags; }

	decalInfo_t			GetDecalInfo( void ) const { return decalInfo; }

						// spectrums are used for "invisible writing" that can only be
						// illuminated by a light of matching spectrum
	int					Spectrum( void ) const { return spectrum; }

	float				GetPolygonOffset( void ) const { return polygonOffset; }

	void				SetPolygonOffset( float po ) { polygonOffset = po; }

	float				GetSurfaceArea( void ) const { return surfaceArea; }
	void				AddToSurfaceArea( float area ) { surfaceArea += area; }

	const class sdDeclSurfaceType*		GetSurfaceType( void ) const { return surfaceTypeDecl; }
	const class sdDeclSurfaceTypeMap*	GetSurfaceTypeMapDecl( void ) const { return surfaceTypeMapDecl; }
	const class sdSurfaceTypeMap*		GetSurfaceTypeMap( void ) const { return surfaceTypeMap; }
	const idVec3&		GetSurfaceColor( void ) const { return surfaceColor; }

	ID_INLINE int		GetGpuSpec( void ) const { return gpuSpec; }

	//------------------------------------------------------------------

						// gets an image for the editor to use
	virtual idImage *	GetEditorImage() const;
	int					GetImageWidth() const;
	int					GetImageHeight() const;

	//------------------------------------------------------------------

						// returns number of registers this material contains
	const int			GetNumRegisters() const { return numRegisters; }

						// regs should point to a float array large enough to hold GetNumRegisters() floats
	void				EvaluateRegisters( float *regs, const float entityParms[MAX_ENTITY_SHADER_PARMS],
											const struct viewDef_s *view, idSoundEmitter *soundEmitter = NULL, int numManualLights = 0 ) const;

						// if a material only uses constants (no entityParm or globalparm references), this
						// will return a pointer to an internal table, and EvaluateRegisters will not need
						// to be called.  If NULL is returned, EvaluateRegisters must be used.
	const float *			ConstantRegisters( const float shaderParms[MAX_ENTITY_SHADER_PARMS], const struct viewDef_s *view ) const;
	void					SetLodDistance( float distance ) const;
	const subviewInfo_t&	GetSubviewInfo() const { return subviewInfo; }
	const sdParameterInfo&	GetParameterInfo( int index ) const { return parameterInfo[index]; } //For visual shader tweaking tools	
	unsigned int			GetBreakpoints() const { return breakpointFlags; }

	const float			GetSlopTexCoordMod() const { return slopTexCoordMod; }

						// evaluates and sets render bindings
	static void			SetRenderBindings( const materialStage_t* stage, const float* materialRegisters, float texCoordScale );

	//------------------------------------------------------------------

						// Purges any partial loadable images
	void				PurgePartialLoadableImages( void );

						// Schedules loading of any partial loadable images
	void				LoadPartialLoadableImages( bool blocking = false );

	bool				IsFinishedPartialLoading( void ) const;

private:
	// parse the entire material
	void				CommonInit();
	void				ParseMaterial( idParser &src );
	bool				MatchToken( idParser &src, const char *match );
	void				ParseSort( idParser &src );
	void				ParseBlend( idParser &src, materialStage_t *stage );
	void				ParseFillMode( idParser &src, materialStage_t *stage );
	void				ParseCullFace( idParser &src, materialStage_t *stage );
	void				ParseStage( idParser &src );
	void				ParseDeform( idParser &src );
	void				ParseDecalInfo( idParser &src );
	bool				CheckSurfaceParm( idToken *token );
	bool				CheckPortalParm( idToken *token );
	int					GetExpressionConstant( float f );
	int					GetExpressionTemporary( void );
	expOp_t	*			GetExpressionOp( void );
	int					EmitOp( int a, int b, expOpType_t opType );
	int					ParseEmitOp( idParser &src, int a, expOpType_t opType, int priority );
	int					ParseTerm( idParser &src );
	int					ParseExpressionPriority( idParser &src, int priority );
	int					ParseExpression( idParser &src );
	bool				ParseConstantCVarExpression( idParser& src, bool& result );
	void				ClearStage( materialStage_t* ms );
	int					NameToSrcBlendMode( const idStr &name );
	int					NameToDstBlendMode( const idStr &name );
	void				MultiplyTextureMatrix( stageTextureMatrix_t* stm, int registers[2][3], const sdDeclRenderBinding* matrix_s, const sdDeclRenderBinding* matrix_t );	// FIXME: for some reason the const is bad for gcc and Mac
	bool				ParseTextureMatrixKey( idToken& key, idParser& src, stageTextureMatrix_t& stm, const sdDeclRenderBinding* matrix_s, const sdDeclRenderBinding* matrix_t );
	bool				AddImplicitStages();
	void				CheckForConstantRegisters();

	void				CompleteStage( materialStage_t* ms, stageParseData_t& spd, const sdDeclRenderBinding** defaults, const int numDefaults );
	void				CompleteInterationStage( materialStage_t* ms, stageParseData_t& spd );
	void				FinishStage( materialStage_t* ms, stageParseData_t& spd );

	static void			SetTextureMatrix( const stageTextureMatrix_t* textureMatrix, const float* materialRegisters, idVec4 matrix[2] );

private:
	idStr				desc;				// description
	idStr				renderBump;			// renderbump command options, without the "renderbump" at the start

	idImage	*			lightFalloffImage;

	int					entityGui;			// draw a gui with the idUserInterface from the renderEntity_t
											// non zero will draw gui, gui2, or gui3 from renderEnitty_t

	bool				noFog;				// surface does not create fog interactions

	int					spectrum;			// for invisible writing, used for both lights and surfaces

	float				polygonOffset;

	int					contentFlags;		// content flags
	int					surfaceFlags;		// surface flags
	int					portalFlags;		// portal flags
	mutable int			materialFlags;		// material flags

	const sdDeclSurfaceType*	surfaceTypeDecl;
	const sdDeclSurfaceTypeMap*	surfaceTypeMapDecl;
	const sdSurfaceTypeMap*		surfaceTypeMap;
	idVec3				surfaceColor;

	decalInfo_t			decalInfo;


	int					gpuSpec;
	float				sort;				// lower numbered shaders draw before higher numbered
	deform_t			deform;
	int					deformRegisters[4];	// numeric parameter for deforms
	const idDecl*		deformDecl;			// for surface emitted particle deforms and tables

	materialCoverage_t	coverage;
	cullType_t			cullType;			// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	bool				shouldCreateBackSides;

	bool				fogLight;
	bool				blendLight;
#if SD_SUPPORT_UNSMOOTHEDTANGENTS
	bool				unsmoothedTangents;
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS
	bool				hasSubview;			// mirror, remote render, etc
	bool				allowOverlays;

	int					numOps;
	expOp_t *			ops;				// evaluate to make expressionRegisters

	int					numRegisters;
	float *				expressionRegisters;

	float *				constantRegisters;	// NULL if ops ever reference globalParms or entityParms
	mutable float		lastFloatTime;
	mutable int			atmosphereFrame;
	bool				timeBasedRegisters;

	int					numStages;
	int					numAmbientStages;

	materialStage_t *	stages;

	float				surfaceArea;		// only for listSurfaceAreas

	// we defer loading of the editor image until it is asked for, so the game doesn't load up
	// all the invisible and uncompressed images.
	// If editorImage is NULL, it will atempt to load editorImageName, and set editorImage to that or defaultImage
	idStr				editorImageName;
	mutable idImage *	editorImage;		// image used for non-shaded preview
	float				editorAlpha;
	
	bool				doLodDistance;		// images will be distance lodded on this shader			
	subviewInfo_t		subviewInfo;
	sdParameterInfo		parameterInfo[MAX_ENTITY_SHADER_PARMS];		// Names of the shader params for visual tweaking tools
	unsigned int		breakpointFlags;

	float				slopTexCoordMod;

	const idMaterial	*backSideMaterial;

	struct mtrParsingData_s	*pd;			// only used during parsing

public:
	static int			currentAtmosphereFrame;

};

typedef idList<const idMaterial *> idMatList;

#endif /* !__MATERIAL_H__ */
