// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
===============================================================================

	Material

===============================================================================
*/

class idImage;
class idCinematic;
class idUserInterface;
class idMegaTexture;
// RAVEN BEGIN
// rjohnson: new shader stage system
class rvNewShaderStage;
class rvGLSLShaderStage;
// RAVEN END

// moved from image.h for default parm
typedef enum {
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_BORDER,		// this should replace TR_CLAMP_TO_ZERO and TR_CLAMP_TO_ZERO_ALPHA,
							// but I don't want to risk changing it right now
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures,
	// set AFTER image format selection
	TR_CLAMP_TO_ZERO_ALPHA,	// guarantee 0 alpha edge for projected textures,
	// set AFTER image format selection
	TR_MIRRORED_REPEAT,
} textureRepeat_t;

typedef struct {
	int		stayTime;		// msec for no change
	float	maxAngle;		// maximum dot product to reject projection angles
} decalInfo_t;

typedef enum {
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
// ddynerman: rectangular sprites
	DFRM_RECTSPRITE,
// RAVEN END
	DFRM_TURB
} deform_t;

typedef enum {
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
// RAVEN BEGIN
// AReis: Used for water reflection/refraction.
	DI_REFLECTION_RENDER,
	DI_REFRACTION_RENDER,
// RAVEN END
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
// RAVEN BEGIN
// rjohnson: new shader stage system
	,
	OP_TYPE_GLSL_ENABLED,
	OP_TYPE_POT_X,
	OP_TYPE_POT_Y,
// RAVEN END
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

// RAVEN BEGIN
// rjohnson: added vertex randomizing
	EXP_REG_VERTEX_RANDOMIZER,
// RAVEN END

	EXP_REG_NUM_PREDEFINED
} expRegister_t;

// RAVEN BEGIN
// rjohnson: added new decal support

// decal registers
#define EXP_REG_DECAL_LIFE		EXP_REG_PARM4
#define	REG_DECAL_LIFE			4
#define EXP_REG_DECAL_SPAWN		EXP_REG_PARM5
#define	REG_DECAL_SPAWN			5
// RAVEN END

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
	TG_SCREEN			// screen aligned, for mirrorRenders and screen space temporaries
} texgen_t;

typedef struct {
	idCinematic *		cinematic;
	idImage *			image;
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
	SVC_INVERSE_MODULATE
} stageVertexColor_t;

static const int	MAX_FRAGMENT_IMAGES = 8;
// RAVEN BEGIN
// AReis: Increased MAX_VERTEX_PARMS from 4 to 16 and added MAX_FRAGMENT_PARMS.
static const int	MAX_VERTEX_PARMS = 16;
static const int	MAX_FRAGMENT_PARMS = 8;
// RAVEN END

typedef struct {
	int					vertexProgram;

// RAVEN BEGIN
// dluetscher: added support for specifying MD5R specfic vertex programs
// Q4SDK: maintain compatible structure padding
#if defined( _MD5R_SUPPORT ) || defined( Q4SDK_MD5R )
	int					md5rVertexProgram;
#endif
// RAVEN END

	int					numVertexParms;
	int					vertexParms[MAX_VERTEX_PARMS][4];	// evaluated register indexes

// RAVEN BEGIN
// AReis: New Fragment Parm stuff.
	int					numFragmentParms;
	int					fragmentParms[MAX_FRAGMENT_PARMS][4];	// evaluated register indexes
// RAVEN END

	int					fragmentProgram;
	int					numFragmentProgramImages;
	idImage *			fragmentProgramImages[MAX_FRAGMENT_IMAGES];

// RAVEN BEGIN
// AReis: Custom Bindings. These override existing parm values.
	bool				vertexParmsBindings[MAX_VERTEX_PARMS];
	bool				fragmentParmsBindings[MAX_FRAGMENT_PARMS];

// AReis: So fragment images can be bound to program specified binding, this
// list keeps track of which image to use (if any, 0 if use specified image).
	int					fragmentProgramBindings[MAX_FRAGMENT_IMAGES];
// RAVEN END

	idMegaTexture		*megaTexture;		// handles all the binding and parameter setting 
} newShaderStage_t;

typedef struct {
	int					conditionRegister;	// if registers[conditionRegister] == 0, skip stage
	stageLighting_t		lighting;			// determines which passes interact with lights
	int					drawStateBits;
	colorStage_t		color;
	bool				hasAlphaTest;
// RAVEN BEGIN
	bool				hasAlphaFunc;
	int					alphaTestMode;
// RAVEN END
	int					alphaTestRegister;
	textureStage_t		texture;
	stageVertexColor_t	vertexColor;
	bool				ignoreAlphaTest;	// this stage should act as translucent, even
											// if the surface is alpha tested
	float				privatePolygonOffset;	// a per-stage polygon offset

	newShaderStage_t	*newStage;			// vertex / fragment program based stage

// RAVEN BEGIN
// rjohnson: new shader stage system
	rvNewShaderStage	*newShaderStage;

// rjohnson: added new decal support
	int					mStageRegisterStart;
	int					mNumStageRegisters;
	int					mStageOpsStart;
	int					mNumStageOps;
// RAVEN END
} shaderStage_t;

typedef enum {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
} materialCoverage_t;

typedef enum {
	SS_MIN = -10000,
// RAVEN BEGIN
	SS_SUBVIEW = -4,	// mirrors, viewscreens, etc
	SS_PREGUI = -3,		// guis
// RAVEN END
	SS_GUI = -2,		// guis
	SS_BAD = -1,
	SS_OPAQUE,			// opaque

	SS_PORTAL_SKY,

	SS_DECAL,			// scorch marks, etc.

	SS_FAR,
	SS_MEDIUM,			// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST,			// screen blood blobs

	SS_POST_PROCESS = 100,	// after a screen copy to texture
	SS_MAX = 10000
} materialSort_t;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES			= 256;

const int MAX_TEXGEN_REGISTERS		= 4;

const int MAX_ENTITY_SHADER_PARMS	= 12;

// material flags
typedef enum {
	MF_DEFAULTED				= BIT(0),
	MF_POLYGONOFFSET			= BIT(1),
	MF_NOSHADOWS				= BIT(2),
	MF_FORCESHADOWS				= BIT(3),
	MF_NOSELFSHADOW				= BIT(4),
	MF_NOPORTALFOG				= BIT(5),	// this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE			= BIT(6)	// in use (visible) per editor
// RAVEN BEGIN
// jscott: for portal skies
	, 
	MF_SKY						= BIT(7),
	MF_NEED_CURRENT_RENDER		= BIT(8)	// for hud guis that need sort order preseved but need back end too
// RAVEN END
} materialFlags_t;

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
typedef enum {
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
	SURFTYPE_PLASTIC,
	SURFTYPE_RICOCHET,
	SURFTYPE_10,
	SURFTYPE_11,
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
} surfTypes_t;

// surface flags
typedef enum {
	SURF_TYPE_BIT0				= BIT(0),	// encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1				= BIT(1),	// "
	SURF_TYPE_BIT2				= BIT(2),	// "
	SURF_TYPE_BIT3				= BIT(3),	// "
	SURF_TYPE_MASK				= ( 1 << NUM_SURFACE_BITS ) - 1,

	SURF_NODAMAGE				= BIT(4),	// never give falling damage
	SURF_SLICK					= BIT(5),	// effects game physics
	SURF_COLLISION				= BIT(6),	// collision surface
	SURF_LADDER					= BIT(7),	// player can climb up this surface
	SURF_NOIMPACT				= BIT(8),	// don't make missile explosions
	SURF_NOSTEPS				= BIT(9),	// no footstep sounds
	SURF_DISCRETE				= BIT(10),	// not clipped or merged by utilities
	SURF_NOFRAGMENT				= BIT(11),	// dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL				= BIT(12),	// renderbump will draw this surface as 0x80 0x80 0x80, which
											// won't collect light from any angle
// RAVEN BEGIN
// bdube: added bounce
	SURF_BOUNCE					= BIT(13),	// projectiles should bounce off this surface

// dluetscher: added no T fix
	SURF_NO_T_FIX				= BIT(14),	// merge surfaces (like decals), but does not try to T-fix them

// RAVEN END
} surfaceFlags_t;

class idSoundEmitter;

// RAVEN BEGIN
// jsinger: added to allow support for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idMaterial : public idDecl, public Serializable<'IMAT'> {
public:
// jsinger:
	virtual void		Write( SerialOutputStream &stream ) const;
	virtual void		AddReferences() const;
						idMaterial( SerialInputStream &stream );
#else
class idMaterial : public idDecl {
#endif
// rjohnson: new shader stage system
	friend class rvNewShaderStage;
	friend class rvGLSLShaderStage;
// RAVEN END

public:
						idMaterial();
	virtual				~idMaterial();

	virtual size_t		Size( void ) const;
	virtual bool		SetDefaultText( void );
	virtual const char *DefaultDefinition( void ) const;
	virtual bool		Parse( const char *text, const int textLength, bool noCaching );
	virtual void		FreeData( void );
	virtual void		Print( void ) const;

						// returns the internal image name for stage 0, which can be used
						// for the renderer CaptureRenderToImage() call
						// I'm not really sure why this needs to be virtual...
	virtual const char	*ImageName( void ) const;

	void				ReloadImages( bool force ) const;
// RAVEN BEGIN
// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	int					streamCount;
	int					streamTimeStamp;
	static int			masterStreamTimeStamp;
	bool				StreamImages( bool inBackground );
	void				UnstreamImages( void );
	void				UpdateImage( int stage, const byte *data, int width, int height );
#endif
// RAVEN END
						// returns number of stages this material contains
	const int			GetNumStages( void ) const { return numStages; }

						// get a specific stage
	const shaderStage_t *GetStage( const int index ) const { assert(index >= 0 && index < numStages); return &stages[index]; }

						// get the first bump map stage, or NULL if not present.
						// used for bumpy-specular
	const shaderStage_t *GetBumpStage( void ) const;

						// returns true if the material will draw anything at all.  Triggers, portals,
						// etc, will not have anything to draw.  A not drawn surface can still castShadow,
						// which can be used to make a simplified shadow hull for a complex object set
						// as noShadow
	bool				IsDrawn( void ) const { return ( numStages > 0 || entityGui != 0 || gui != NULL ); }

						// returns true if the material will draw any non light interaction stages
	bool				HasAmbient( void ) const { return ( numAmbientStages > 0 ); }

						// returns true if material has a gui
	bool				HasGui( void ) const { return ( entityGui != 0 || gui != NULL ); }

						// returns true if the material will generate another view, either as
						// a mirror or dynamic rendered image
	bool				HasSubview( void ) const { return hasSubview; }

						// returns true if the material will generate shadows, not making a
						// distinction between global and no-self shadows
	bool				SurfaceCastsShadow( void ) const { return TestMaterialFlag( MF_FORCESHADOWS ) || !TestMaterialFlag( MF_NOSHADOWS ); }

						// returns true if the material will generate interactions with fog/blend lights
						// All non-translucent surfaces receive fog unless they are explicitly noFog
	bool				ReceivesFog( void ) const { return ( IsDrawn() && !noFog && coverage != MC_TRANSLUCENT ); }

						// returns true if the material will generate interactions with normal lights
						// Many special effect surfaces don't have any bump/diffuse/specular
						// stages, and don't interact with lights at all
	bool				ReceivesLighting( void ) const { return numAmbientStages != numStages; }

						// returns true if the material should generate interactions on sides facing away
						// from light centers, as with noshadow and noselfshadow options
	bool				ReceivesLightingOnBackSides( void ) const { return ( materialFlags & (MF_NOSELFSHADOW|MF_NOSHADOWS) ) != 0; }

						// Standard two-sided triangle rendering won't work with bump map lighting, because
						// the normal and tangent vectors won't be correct for the back sides.  When two
						// sided lighting is desired. typically for alpha tested surfaces, this is
						// addressed by having CleanupModelSurfaces() create duplicates of all the triangles
						// with apropriate order reversal.
	bool				ShouldCreateBackSides( void ) const { return shouldCreateBackSides; }

						// characters and models that are created by a complete renderbump can use a faster
						// method of tangent and normal vector generation than surfaces which have a flat
						// renderbump wrapped over them.
	bool				UseUnsmoothedTangents( void ) const { return unsmoothedTangents; }

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

						// returns a idUserInterface if it has a global gui, or NULL if no gui
	idUserInterface	*	GlobalGui( void ) const { return gui; }

						// a discrete surface will never be merged with other surfaces by dmap, which is
						// necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
						// special effects from being combined into a single surface
						// guis, merging sprites or other effects, mirrors and remote views are always discrete
	bool				IsDiscrete( void ) const { return ( entityGui || gui || deform != DFRM_NONE || sort == SS_SUBVIEW ||
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

// RAVEN BEGIN
// dluetscher: added SURF_NO_T_FIX to merge surfaces (like decals), but skipping any T-junction fixing
	bool				NoTFix( void ) const { return ( surfaceFlags & SURF_NO_T_FIX ) != 0; }
// RAVEN END

	//------------------------------------------------------------------

// RAVEN BEGIN
// jscott: added accessor
	const rvDeclMatType *		GetMaterialType( void ) const { return( materialType ); }
	const rvDeclMatType *		GetMaterialType( idVec2 &tc ) const;
	byte *						GetMaterialTypeArray( void ) const { return( materialTypeArray ); }
	const char *				GetMaterialTypeArrayName( void ) const { return( materialTypeArrayName.c_str() ); }

// jscott: for profiling
	int							GetTexelCount( void ) const;

// jscott: for error checking
	bool						HasDefaultedImage( void ) const;

// jscott: for Radiant
	idImage *					GetDiffuseImage( void ) const;

// AReis: New portal distance culling stuff.
	float						GetPortalNear( void ) const { return( portalDistanceNear ); }
	float						GetPortalFar( void ) const { return( portalDistanceFar ); }
	const idImage *				GetPortalImage( void ) const { return( portalImage ); }

// jscott: to prevent a recursive crash
	virtual	bool				RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual bool				Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

	//==================================================================
	// light shader specific functions, only called for light entities

						// lightshader option to fill with fog from viewer instead of light from center
	bool				IsFogLight() const { return fogLight; }

						// perform simple blending of the projection, instead of interacting with bumps and textures
	bool				IsBlendLight() const { return blendLight; }

						// an ambient light has non-directional bump mapping and no specular
	bool				IsAmbientLight() const { return ambientLight; }

						// implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
						// but individual light entities can also override this value
	bool				LightCastsShadows() const { return TestMaterialFlag( MF_FORCESHADOWS ) ||
								( !fogLight && !ambientLight && !blendLight && !TestMaterialFlag( MF_NOSHADOWS ) ); }

						// fog lights, blend lights, ambient lights, etc will all have to have interaction
						// triangles generated for sides facing away from the light as well as those
						// facing towards the light.  It is debatable if noshadow lights should effect back
						// sides, making everything "noSelfShadow", but that would make noshadow lights
						// potentially slower than normal lights, which detracts from their optimization
						// ability, so they currently do not.
	bool				LightEffectsBackSides() const { return fogLight || ambientLight || blendLight; }

						// NULL unless an image is explicitly specified in the shader with "lightFalloffShader <image>"
	idImage	*			LightFalloffImage() const { return lightFalloffImage; }

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

						// gets name for surface type (stone, metal, flesh, etc.)
	const surfTypes_t	GetSurfaceType( void ) const { return static_cast<surfTypes_t>( surfaceFlags & SURF_TYPE_MASK ); }

						// get material description
	const char *		GetDescription( void ) const { return desc; }

						// get sort order
	const float			GetSort( void ) const { return sort; }
						// this is only used by the gui system to force sorting order
						// on images referenced from tga's instead of materials. 
						// this is done this way as there are 2000 tgas the guis use
	void				SetSort( float s ) const { sort = s; };

						// DFRM_NONE, DFRM_SPRITE, etc
	deform_t			Deform( void ) const { return deform; }

						// flare size, expansion size, etc
	const int			GetDeformRegister( int index ) const { return deformRegisters[index]; }

						// particle system to emit from surface and table for turbulent
	const idDecl		*GetDeformDecl( void ) const { return deformDecl; }

						// currently a surface can only have one unique texgen for all the stages
	texgen_t			Texgen() const;

						// wobble sky parms
	const int *			GetTexGenRegisters( void ) const { return texGenRegisters; }

						// get cull type
	const cullType_t	GetCullType( void ) const { return cullType; }

	float				GetEditorAlpha( void ) const { return editorAlpha; }

	int					GetEntityGui( void ) const { return entityGui; }

	decalInfo_t			GetDecalInfo( void ) const { return decalInfo; }

						// spectrums are used for "invisible writing" that can only be
						// illuminated by a light of matching spectrum
	int					Spectrum( void ) const { return spectrum; }

	float				GetPolygonOffset( void ) const { return polygonOffset; }

	float				GetSurfaceArea( void ) const { return surfaceArea; }
	void				AddToSurfaceArea( float area ) { surfaceArea += area; }

	//------------------------------------------------------------------

						// returns the length, in milliseconds, of the videoMap on this material,
						// or zero if it doesn't have one
	int					CinematicLength( void ) const;

	void				CloseCinematic( void ) const;

	void				ResetCinematicTime( int time ) const;

	void				UpdateCinematic( int time ) const;

	//------------------------------------------------------------------

						// gets an image for the editor to use
	idImage *			GetEditorImage( void ) const;
	int					GetImageWidth( void ) const;
	int					GetImageHeight( void ) const;

	void				SetGui( const char *_gui ) const;

						// just for resource tracking
	void				SetImageClassifications( int tag ) const;

	//------------------------------------------------------------------

						// returns number of registers this material contains
	const int			GetNumRegisters() const { return numRegisters; }

// RAVEN BEGIN
// rjohnson: added vertex randomizing
						// regs should point to a float array large enough to hold GetNumRegisters() floats
	void				EvaluateRegisters( float *regs, const float entityParms[MAX_ENTITY_SHADER_PARMS], 
											const struct viewDef_s *view, int soundEmitter = 0, idVec3 *randomizer = NULL ) const;
// RAVEN END

// RAVEN BEGIN
// rjohnson: added new decal support
	void				EvaluateStageRegisters( int StageIndex, float *registers, const float shaderParms[MAX_ENTITY_SHADER_PARMS], float FloatTime) const;

// rjohnson: started tracking image/material usage
	void				ClearUseCount( void ) { useCount = 0; }
	void				IncreaseUseCount( void ) { useCount++; globalUseCount++; }
	int					GetUseCount( void ) { return useCount; }
	int					GetGlobalUseCount( void ) const { return globalUseCount; }
	void				ResolveUse( void );
// RAVEN END

						// if a material only uses constants (no entityParm or globalparm references), this
						// will return a pointer to an internal table, and EvaluateRegisters will not need
						// to be called.  If NULL is returned, EvaluateRegisters must be used.
	const float *		ConstantRegisters() const;

	bool				SuppressInSubview() const				{ return suppressInSubview; };
	bool				IsPortalSky() const						{ return portalSky; };
	void				AddReference();

private:
	// parse the entire material
	void				CommonInit();
// RAVEN BEGIN
// scork: now returns false (WHEN VALIDATING) under circumstances which would cause a common->FatalError normally, caller should bail ASAP on return.
	bool				ParseMaterial( idLexer &src );
// RAVEN END
	bool				MatchToken( idLexer &src, const char *match );
	void				ParseSort( idLexer &src );
	void				ParseBlend( idLexer &src, shaderStage_t *stage );
	void				ParseVertexParm( idLexer &src, newShaderStage_t *newStage );
// RAVEN BEGIN
// AReis: New fragment parm stuff.
	void				ParseFragmentParm( idLexer &src, newShaderStage_t *newStage );
// RAVEN END
	void				ParseFragmentMap( idLexer &src, newShaderStage_t *newStage );

// RAVEN BEGIN
// scork: now returns false (WHEN VALIDATING) under circumstances which would cause a common->FatalError normally, caller should bail ASAP on return.
	bool				ParseStage( idLexer &src, const textureRepeat_t trpDefault = TR_REPEAT );
// RAVEN END
	void				ParseDeform( idLexer &src );
	void				ParseDecalInfo( idLexer &src );
	bool				CheckSurfaceParm( idToken *token );
	int					GetExpressionConstant( float f );
	int					GetExpressionTemporary( void );
	expOp_t	*			GetExpressionOp( void );
	int					EmitOp( int a, int b, expOpType_t opType );
	int					ParseEmitOp( idLexer &src, int a, expOpType_t opType, int priority );
	int					ParseTerm( idLexer &src );
	int					ParseExpressionPriority( idLexer &src, int priority );
	int					ParseExpression( idLexer &src );
	void				ClearStage( shaderStage_t *ss );
	int					NameToSrcBlendMode( const idStr &name );
	int					NameToDstBlendMode( const idStr &name );
	void				MultiplyTextureMatrix( textureStage_t *ts, int registers[2][3] );	// FIXME: for some reason the const is bad for gcc and Mac
	void				SortInteractionStages();
// RAVEN BEGIN
// scork: now returns false (WHEN VALIDATING) under circumstances which would cause a common->FatalError normally, caller should bail ASAP on return.
	bool				AddImplicitStages( const textureRepeat_t trpDefault = TR_REPEAT );
// RAVEN END
	void				CheckForConstantRegisters();

private:
	idStr				desc;				// description
	idStr				renderBump;			// renderbump command options, without the "renderbump" at the start

	idImage	*			lightFalloffImage;

	int					entityGui;			// draw a gui with the idUserInterface from the renderEntity_t
											// non zero will draw gui, gui2, or gui3 from renderEnitty_t
	mutable idUserInterface	*gui;			// non-custom guis are shared by all users of a material

// RAVEN BEGIN
// jscott: for material types
	const rvDeclMatType *materialType;
	byte				*materialTypeArray;	// an array of material type indices generated from the hit image
	idStr				materialTypeArrayName;
	int					MTAWidth;
	int					MTAHeight;

// rjohnson: started tracking image/material usage
	int					useCount;
	int					globalUseCount;

// AReis: New portal distance culling stuff.
	float				portalDistanceNear;
	float				portalDistanceFar;
	idImage *			portalImage;
// RAVEN END

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
	expOp_t *			ops;				// evaluate to make expressionRegisters
																										
	int					numRegisters;																			//
	float *				expressionRegisters;

	float *				constantRegisters;	// NULL if ops ever reference globalParms or entityParms

	int					numStages;
	int					numAmbientStages;
																										
	shaderStage_t *		stages;

	struct mtrParsingData_s	*pd;			// only used during parsing

	float				surfaceArea;		// only for listSurfaceAreas

	// we defer loading of the editor image until it is asked for, so the game doesn't load up
	// all the invisible and uncompressed images.
	// If editorImage is NULL, it will atempt to load editorImageName, and set editorImage to that or defaultImage
	idStr				editorImageName;
	mutable idImage *	editorImage;		// image used for non-shaded preview
	float				editorAlpha;

	bool				suppressInSubview;
	bool				portalSky;
	int					refCount;
};

typedef idList<const idMaterial *> idMatList;

// RAVEN BEGIN
class rvMaterialEdit
{
public:
	virtual ~rvMaterialEdit() {}
	virtual	void		SetGui( idMaterial *edit, const char *name ) = 0;
	virtual int			GetImageWidth( const idMaterial *edit ) const = 0;
	virtual int			GetImageHeight( const idMaterial *edit ) const = 0;
};

extern rvMaterialEdit	*materialEdit;
// RAVEN END

#endif /* !__MATERIAL_H__ */
