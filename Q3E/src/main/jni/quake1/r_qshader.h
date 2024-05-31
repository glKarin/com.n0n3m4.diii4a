#ifndef QSHADER_H
#define QSHADER_H

#include "qtypes.h"

// various flags from shaders, used for special effects not otherwise classified
// TODO: support these features more directly
#define Q3TEXTUREFLAG_TWOSIDED 1
#define Q3TEXTUREFLAG_NOPICMIP 16
#define Q3TEXTUREFLAG_POLYGONOFFSET 32
#define Q3TEXTUREFLAG_REFRACTION 256
#define Q3TEXTUREFLAG_REFLECTION 512
#define Q3TEXTUREFLAG_WATERSHADER 1024
#define Q3TEXTUREFLAG_CAMERA 2048
#define Q3TEXTUREFLAG_TRANSPARENTSORT 4096

#define Q3PATHLENGTH 64
#define TEXTURE_MAXFRAMES 64
#define Q3WAVEPARMS 4
#define Q3DEFORM_MAXPARMS 3
#define Q3SHADER_MAXLAYERS 8
#define Q3RGBGEN_MAXPARMS 3
#define Q3ALPHAGEN_MAXPARMS 1
#define Q3TCGEN_MAXPARMS 6
#define Q3TCMOD_MAXPARMS 6
#define Q3MAXTCMODS 8
#define Q3MAXDEFORMS 4

typedef enum q3wavefunc_e
{
	Q3WAVEFUNC_NONE,
	Q3WAVEFUNC_INVERSESAWTOOTH,
	Q3WAVEFUNC_NOISE,
	Q3WAVEFUNC_SAWTOOTH,
	Q3WAVEFUNC_SIN,
	Q3WAVEFUNC_SQUARE,
	Q3WAVEFUNC_TRIANGLE,
	Q3WAVEFUNC_COUNT
}
q3wavefunc_e;
typedef int q3wavefunc_t;
#define Q3WAVEFUNC_USER_COUNT 4
#define Q3WAVEFUNC_USER_SHIFT 8 // use 8 bits for wave func type

typedef enum q3deform_e
{
	Q3DEFORM_NONE,
	Q3DEFORM_PROJECTIONSHADOW,
	Q3DEFORM_AUTOSPRITE,
	Q3DEFORM_AUTOSPRITE2,
	Q3DEFORM_TEXT0,
	Q3DEFORM_TEXT1,
	Q3DEFORM_TEXT2,
	Q3DEFORM_TEXT3,
	Q3DEFORM_TEXT4,
	Q3DEFORM_TEXT5,
	Q3DEFORM_TEXT6,
	Q3DEFORM_TEXT7,
	Q3DEFORM_BULGE,
	Q3DEFORM_WAVE,
	Q3DEFORM_NORMAL,
	Q3DEFORM_MOVE,
	Q3DEFORM_COUNT
}
q3deform_t;

typedef enum q3rgbgen_e
{
	Q3RGBGEN_IDENTITY,
	Q3RGBGEN_CONST,
	Q3RGBGEN_ENTITY,
	Q3RGBGEN_EXACTVERTEX,
	Q3RGBGEN_IDENTITYLIGHTING,
	Q3RGBGEN_LIGHTINGDIFFUSE,
	Q3RGBGEN_ONEMINUSENTITY,
	Q3RGBGEN_ONEMINUSVERTEX,
	Q3RGBGEN_VERTEX,
	Q3RGBGEN_WAVE,
	Q3RGBGEN_COUNT
}
q3rgbgen_t;

typedef enum q3alphagen_e
{
	Q3ALPHAGEN_IDENTITY,
	Q3ALPHAGEN_CONST,
	Q3ALPHAGEN_ENTITY,
	Q3ALPHAGEN_LIGHTINGSPECULAR,
	Q3ALPHAGEN_ONEMINUSENTITY,
	Q3ALPHAGEN_ONEMINUSVERTEX,
	Q3ALPHAGEN_PORTAL,
	Q3ALPHAGEN_VERTEX,
	Q3ALPHAGEN_WAVE,
	Q3ALPHAGEN_COUNT
}
q3alphagen_t;

typedef enum q3tcgen_e
{
	Q3TCGEN_NONE,
	Q3TCGEN_TEXTURE, // very common
	Q3TCGEN_ENVIRONMENT, // common
	Q3TCGEN_LIGHTMAP,
	Q3TCGEN_VECTOR,
	Q3TCGEN_COUNT
}
q3tcgen_t;

typedef enum q3tcmod_e
{
	Q3TCMOD_NONE,
	Q3TCMOD_ENTITYTRANSLATE,
	Q3TCMOD_ROTATE,
	Q3TCMOD_SCALE,
	Q3TCMOD_SCROLL,
	Q3TCMOD_STRETCH,
	Q3TCMOD_TRANSFORM,
	Q3TCMOD_TURBULENT,
	Q3TCMOD_PAGE,
	Q3TCMOD_COUNT
}
q3tcmod_t;

typedef struct q3shaderinfo_layer_rgbgen_s
{
	q3rgbgen_t rgbgen;
	float parms[Q3RGBGEN_MAXPARMS];
	q3wavefunc_t wavefunc;
	float waveparms[Q3WAVEPARMS];
}
q3shaderinfo_layer_rgbgen_t;

typedef struct q3shaderinfo_layer_alphagen_s
{
	q3alphagen_t alphagen;
	float parms[Q3ALPHAGEN_MAXPARMS];
	q3wavefunc_t wavefunc;
	float waveparms[Q3WAVEPARMS];
}
q3shaderinfo_layer_alphagen_t;

typedef struct q3shaderinfo_layer_tcgen_s
{
	q3tcgen_t tcgen;
	float parms[Q3TCGEN_MAXPARMS];
}
q3shaderinfo_layer_tcgen_t;

typedef struct q3shaderinfo_layer_tcmod_s
{
	q3tcmod_t tcmod;
	float parms[Q3TCMOD_MAXPARMS];
	q3wavefunc_t wavefunc;
	float waveparms[Q3WAVEPARMS];
}
q3shaderinfo_layer_tcmod_t;

typedef struct q3shaderinfo_layer_s
{
	int alphatest;
	int clampmap;
	float framerate;
	int numframes;
	int dptexflags;
	char** texturename;
	int blendfunc[2];
	q3shaderinfo_layer_rgbgen_t rgbgen;
	q3shaderinfo_layer_alphagen_t alphagen;
	q3shaderinfo_layer_tcgen_t tcgen;
	q3shaderinfo_layer_tcmod_t tcmods[Q3MAXTCMODS];
}
q3shaderinfo_layer_t;

typedef struct q3shaderinfo_deform_s
{
	q3deform_t deform;
	float parms[Q3DEFORM_MAXPARMS];
	q3wavefunc_t wavefunc;
	float waveparms[Q3WAVEPARMS];
}
q3shaderinfo_deform_t;

typedef enum dpoffsetmapping_technique_s
{
	OFFSETMAPPING_OFF,			// none
	OFFSETMAPPING_DEFAULT,		// cvar-set
	OFFSETMAPPING_LINEAR,		// linear
	OFFSETMAPPING_RELIEF		// relief
}dpoffsetmapping_technique_t;

typedef enum dptransparentsort_category_e
{
	TRANSPARENTSORT_SKY,
	TRANSPARENTSORT_DISTANCE,
	TRANSPARENTSORT_HUD,
}dptransparentsortcategory_t;

typedef struct shader_s
{
	char name[Q3PATHLENGTH];
#define Q3SHADERINFO_COMPARE_START surfaceparms
	int surfaceparms;
	int surfaceflags;
	int textureflags;
	int numlayers;
	qbool lighting;
	qbool vertexalpha;
	qbool textureblendalpha;
	q3shaderinfo_layer_t layers[Q3SHADER_MAXLAYERS];
	char skyboxname[Q3PATHLENGTH];
	q3shaderinfo_deform_t deforms[Q3MAXDEFORMS];

	// dp-specific additions:

	// shadow control
	qbool dpnortlight;
	qbool dpshadow;
	qbool dpnoshadow;

	// add collisions to all triangles of the surface
	qbool dpmeshcollisions;

	// kill shader based on cvar checks
	qbool dpshaderkill;

	// fake reflection
	char dpreflectcube[Q3PATHLENGTH];

	// reflection
	float reflectmin; // when refraction is used, minimum amount of reflection (when looking straight down)
	float reflectmax; // when refraction is used, maximum amount of reflection (when looking parallel to water)
	float refractfactor; // amount of refraction distort (1.0 = like the cvar specifies)
	vec4_t refractcolor4f; // color tint of refraction (including alpha factor)
	float reflectfactor; // amount of reflection distort (1.0 = like the cvar specifies)
	vec4_t reflectcolor4f; // color tint of reflection (including alpha factor)
	float r_water_wateralpha; // additional wateralpha to apply when r_water is active
	float r_water_waterscroll[2]; // water normalmapscrollblend - scale and speed

	// offsetmapping
	dpoffsetmapping_technique_t offsetmapping;
	float offsetscale;
	float offsetbias; // 0 is normal, 1 leads to alpha 0 being neutral and alpha 1 pushing "out"

	// polygonoffset (only used if Q3TEXTUREFLAG_POLYGONOFFSET)
	float biaspolygonoffset, biaspolygonfactor;

	// transparent sort category
	dptransparentsortcategory_t transparentsort;

	// gloss
	float specularscalemod;
	float specularpowermod;

	// rtlighting ambient addition
	float rtlightambient;
#define Q3SHADERINFO_COMPARE_END rtlightambient
}
shader_t;

typedef struct texture_shaderpass_s
{
	qbool alphatest; // FIXME: handle alphafunc properly
	float framerate;
	int numframes;
	struct skinframe_s *skinframes[TEXTURE_MAXFRAMES];
	int blendfunc[2];
	q3shaderinfo_layer_rgbgen_t rgbgen;
	q3shaderinfo_layer_alphagen_t alphagen;
	q3shaderinfo_layer_tcgen_t tcgen;
	q3shaderinfo_layer_tcmod_t tcmods[Q3MAXTCMODS];
}
texture_shaderpass_t;

#endif
