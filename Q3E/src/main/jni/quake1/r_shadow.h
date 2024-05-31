
#ifndef R_SHADOW_H
#define R_SHADOW_H

#include "qtypes.h"
#include "taskqueue.h"
#include "matrixlib.h"
struct rtlight_s;
struct msurface_s;
struct entity_render_s;

#define R_SHADOW_SHADOWMAP_NUMCUBEMAPS 8

extern struct cvar_s r_shadow_bumpscale_basetexture;
extern struct cvar_s r_shadow_bumpscale_bumpmap;
extern struct cvar_s r_shadow_debuglight;
extern struct cvar_s r_shadow_gloss;
extern struct cvar_s r_shadow_gloss2intensity;
extern struct cvar_s r_shadow_glossintensity;
extern struct cvar_s r_shadow_glossexponent;
extern struct cvar_s r_shadow_gloss2exponent;
extern struct cvar_s r_shadow_glossexact;
extern struct cvar_s r_shadow_lightattenuationpower;
extern struct cvar_s r_shadow_lightattenuationscale;
extern struct cvar_s r_shadow_lightintensityscale;
extern struct cvar_s r_shadow_lightradiusscale;
extern struct cvar_s r_shadow_projectdistance;
extern struct cvar_s r_shadow_frontsidecasting;
extern struct cvar_s r_shadow_realtime_dlight;
extern struct cvar_s r_shadow_realtime_dlight_shadows;
extern struct cvar_s r_shadow_realtime_dlight_svbspculling;
extern struct cvar_s r_shadow_realtime_dlight_portalculling;
extern struct cvar_s r_shadow_realtime_world;
extern struct cvar_s r_shadow_realtime_world_lightmaps;
extern struct cvar_s r_shadow_realtime_world_shadows;
extern struct cvar_s r_shadow_realtime_world_compile;
extern struct cvar_s r_shadow_realtime_world_compileshadow;
extern struct cvar_s r_shadow_realtime_world_compilesvbsp;
extern struct cvar_s r_shadow_realtime_world_compileportalculling;
extern struct cvar_s r_shadow_scissor;

// used by shader for bouncegrid feature
typedef struct r_shadow_bouncegrid_settings_s
{
	qbool staticmode;
	qbool directionalshading;
	qbool includedirectlighting;
	qbool blur;
	qbool normalizevectors;
	int floatcolors;
	float dlightparticlemultiplier;
	qbool hitmodels;
	float lightradiusscale;
	int maxbounce;
	float lightpathsize;
	float particlebounceintensity;
	float particleintensity;
	int maxphotons;
	float energyperphoton;
	float spacing[3];
	int rng_type;
	int rng_seed;
	float bounceminimumintensity2;
	int subsamples;
}
r_shadow_bouncegrid_settings_t;

#define PHOTON_MAX_PATHS 11

typedef struct r_shadow_bouncegrid_photon_path_s
{
	vec3_t start;
	vec3_t end;
	vec3_t color;
}
r_shadow_bouncegrid_photon_path_t;

typedef struct r_shadow_bouncegrid_photon_s
{
	// parameters for tracing this photon
	vec3_t start;
	vec3_t end;
	float color[3];
	float bounceminimumintensity2;
	float startrefractiveindex;

	// results
	int numpaths;
	r_shadow_bouncegrid_photon_path_t paths[PHOTON_MAX_PATHS];
}
r_shadow_bouncegrid_photon_t;

typedef struct r_shadow_bouncegrid_state_s
{
	r_shadow_bouncegrid_settings_t settings;
	qbool capable;
	qbool allowdirectionalshading;
	qbool directional; // copied from settings.directionalshading after createtexture is decided
	qbool createtexture; // set to true to recreate the texture rather than updating it - happens when size changes or directional changes
	struct rtexture_s *texture;
	matrix4x4_t matrix;
	vec_t intensity;
	double lastupdatetime;
	int resolution[3];
	int numpixels;
	int pixelbands;
	int pixelsperband;
	int bytesperband;
	float spacing[3];
	float ispacing[3];
	vec3_t mins;
	vec3_t maxs;
	vec3_t size;

	// per-frame data that is very temporary
	int highpixels_index; // which one is active - this toggles when doing blur
	float *highpixels; // equals blurpixels[highpixels_index]
	float *blurpixels[2];
	unsigned char *u8pixels; // temporary processing buffer when outputting to rgba8 format
	unsigned short *fp16pixels; // temporary processing buffer when outputting to rgba16f format
								// describe the photons we intend to shoot for threaded dispatch
	int numphotons; // number of photons to shoot this frame, always <= settings.maxphotons
	r_shadow_bouncegrid_photon_t *photons; // describes the photons being shot this frame

	// tasks
	taskqueue_task_t cleartex_task; // clears the highpixels array
	taskqueue_task_t assignphotons_task; // sets the photon counts on lights, etc
	taskqueue_task_t enqueuephotons_task; // enqueues tasks to shoot the photons
	taskqueue_task_t *photons_tasks; // [maxphotons] taskqueue entries to perform the photon shots
	taskqueue_task_t photons_done_task; // checks that all photon shots are completed
	taskqueue_task_t enqueue_slices_task; // enqueues slice tasks to render the light accumulation into the texture
	taskqueue_task_t *slices_tasks; // [resolution[1]] taskqueue entries to perform the light path accumulation into the texture
	taskqueue_task_t slices_done_task; // checks that light accumulation in the texture is done
	taskqueue_task_t blurpixels_task; // blurs the highpixels array
}
r_shadow_bouncegrid_state_t;

extern r_shadow_bouncegrid_state_t r_shadow_bouncegrid_state;

void R_Shadow_Init(void);
qbool R_Shadow_ShadowMappingEnabled(void);
void R_Shadow_ShadowMapFromList(int numverts, int numtris, const float *vertex3f, const int *elements, int numsidetris, const int *sidetotals, const unsigned char *sides, const int *sidetris);
int R_Shadow_CalcTriangleSideMask(const vec3_t p1, const vec3_t p2, const vec3_t p3, float bias);
int R_Shadow_CalcSphereSideMask(const vec3_t p1, float radius, float bias);
int R_Shadow_ChooseSidesFromBox(int firsttriangle, int numtris, const float *invertex3f, const int *elements, const matrix4x4_t *worldtolight, const vec3_t projectorigin, const vec3_t projectdirection, const vec3_t lightmins, const vec3_t lightmaxs, const vec3_t surfacemins, const vec3_t surfacemaxs, int *totals);
void R_Shadow_RenderLighting(int texturenumsurfaces, const struct msurface_s **texturesurfacelist);
void R_Shadow_RenderMode_Begin(void);
void R_Shadow_RenderMode_ActiveLight(const struct rtlight_s *rtlight);
void R_Shadow_RenderMode_Reset(void);
void R_Shadow_RenderMode_Lighting(qbool transparent, qbool shadowmapping, qbool noselfshadowpass);
void R_Shadow_RenderMode_DrawDeferredLight(qbool shadowmapping);
void R_Shadow_RenderMode_VisibleLighting(qbool transparent);
void R_Shadow_RenderMode_End(void);
void R_Shadow_ClearStencil(void);
void R_Shadow_SetupEntityLight(const struct entity_render_s *ent);

qbool R_Shadow_ScissorForBBox(const float *mins, const float *maxs);

// these never change, they are used to create attenuation matrices
extern matrix4x4_t matrix_attenuationxyz;
extern matrix4x4_t matrix_attenuationz;

void R_Shadow_UpdateWorldLightSelection(void);

extern struct rtlight_s *r_shadow_compilingrtlight;

void R_RTLight_Update(struct rtlight_s *rtlight, int isstatic, matrix4x4_t *matrix, vec3_t color, int style, const char *cubemapname, int shadow, vec_t corona, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags);
void R_RTLight_Compile(struct rtlight_s *rtlight);
void R_RTLight_Uncompile(struct rtlight_s *rtlight);

void R_Shadow_PrepareLights(void);
void R_Shadow_ClearShadowMapTexture(void);
void R_Shadow_DrawPrepass(void);
void R_Shadow_DrawLights(void);
void R_Shadow_DrawCoronas(void);

extern int maxshadowmark;
extern int numshadowmark;
extern int *shadowmark;
extern int *shadowmarklist;
extern int shadowmarkcount;
void R_Shadow_PrepareShadowMark(int numtris);

extern int maxshadowsides;
extern int numshadowsides;
extern unsigned char *shadowsides;
extern int *shadowsideslist;
void R_Shadow_PrepareShadowSides(int numtris);

void R_Shadow_PrepareModelShadows(void);

#define LP_LIGHTMAP		1
#define LP_RTWORLD		2
#define LP_DYNLIGHT		4
void R_CompleteLightPoint(float *ambient, float *diffuse, float *lightdir, const vec3_t p, const int flags, float lightmapintensity, float ambientintensity);

void R_Shadow_DrawShadowMaps(void);

#endif
