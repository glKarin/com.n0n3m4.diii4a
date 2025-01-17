#include "quakedef.h"
#include "r_shadow.h"
#include "cl_collision.h"
#include "portals.h"
#include "image.h"

static void R_Shadow_EditLights_Init(void);

typedef enum r_shadow_rendermode_e
{
	R_SHADOW_RENDERMODE_NONE,
	R_SHADOW_RENDERMODE_ZPASS_STENCIL,
	R_SHADOW_RENDERMODE_ZPASS_SEPARATESTENCIL,
	R_SHADOW_RENDERMODE_ZPASS_STENCILTWOSIDE,
	R_SHADOW_RENDERMODE_ZFAIL_STENCIL,
	R_SHADOW_RENDERMODE_ZFAIL_SEPARATESTENCIL,
	R_SHADOW_RENDERMODE_ZFAIL_STENCILTWOSIDE,
	R_SHADOW_RENDERMODE_LIGHT_GLSL,
	R_SHADOW_RENDERMODE_VISIBLEVOLUMES,
	R_SHADOW_RENDERMODE_VISIBLELIGHTING,
	R_SHADOW_RENDERMODE_SHADOWMAP2D
}
r_shadow_rendermode_t;

typedef enum r_shadow_shadowmode_e
{
	R_SHADOW_SHADOWMODE_DISABLED,
	R_SHADOW_SHADOWMODE_SHADOWMAP2D
}
r_shadow_shadowmode_t;

r_shadow_rendermode_t r_shadow_rendermode = R_SHADOW_RENDERMODE_NONE;
r_shadow_rendermode_t r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_NONE;
int r_shadow_scenemaxlights;
int r_shadow_scenenumlights;
rtlight_t **r_shadow_scenelightlist; // includes both static lights and dlights, as filtered by appropriate flags
qbool r_shadow_usingshadowmap2d;
qbool r_shadow_usingshadowmaportho;
int r_shadow_shadowmapside;
float r_shadow_lightshadowmap_texturescale[4]; // xy = scale, zw = offset
float r_shadow_lightshadowmap_parameters[4]; // x = frustum width in pixels (excludes border), y = z scale, z = size of viewport, w = z center
float r_shadow_modelshadowmap_texturescale[4]; // xy = scale, zw = offset
float r_shadow_modelshadowmap_parameters[4]; // xyz = scale, w = shadow brightness
#if 0
int r_shadow_drawbuffer;
int r_shadow_readbuffer;
#endif
int r_shadow_cullface_front, r_shadow_cullface_back;
GLuint r_shadow_fbo2d;
int r_shadow_shadowmode_shadowmapping; // cached value of r_shadow_shadowmapping cvar
int r_shadow_shadowmode_deferred; // cached value of r_shadow_deferred cvar
r_shadow_shadowmode_t r_shadow_shadowmode;
int r_shadow_shadowmapfilterquality;
int r_shadow_shadowmapdepthbits;
int r_shadow_shadowmapmaxsize;
int r_shadow_shadowmaptexturesize;
qbool r_shadow_shadowmapvsdct;
qbool r_shadow_shadowmapsampler;
qbool r_shadow_shadowmapshadowsampler;
int r_shadow_shadowmappcf;
int r_shadow_shadowmapborder;
matrix4x4_t r_shadow_shadowmapmatrix;
int r_shadow_lightscissor[4];
qbool r_shadow_usingdeferredprepass;
qbool r_shadow_shadowmapdepthtexture;
mod_alloclightmap_state_t r_shadow_shadowmapatlas_state;
int r_shadow_shadowmapatlas_modelshadows_x;
int r_shadow_shadowmapatlas_modelshadows_y;
int r_shadow_shadowmapatlas_modelshadows_size;
int maxshadowtriangles;
int *shadowelements;

int maxshadowvertices;
float *shadowvertex3f;

int maxshadowmark;
int numshadowmark;
int *shadowmark;
int *shadowmarklist;
int shadowmarkcount;

int maxshadowsides;
int numshadowsides;
unsigned char *shadowsides;
int *shadowsideslist;

int maxvertexupdate;
int *vertexupdate;
int *vertexremap;
int vertexupdatenum;

int r_shadow_buffer_numleafpvsbytes;
unsigned char *r_shadow_buffer_visitingleafpvs;
unsigned char *r_shadow_buffer_leafpvs;
int *r_shadow_buffer_leaflist;

int r_shadow_buffer_numsurfacepvsbytes;
unsigned char *r_shadow_buffer_surfacepvs;
int *r_shadow_buffer_surfacelist;
unsigned char *r_shadow_buffer_surfacesides;

int r_shadow_buffer_numshadowtrispvsbytes;
unsigned char *r_shadow_buffer_shadowtrispvs;
int r_shadow_buffer_numlighttrispvsbytes;
unsigned char *r_shadow_buffer_lighttrispvs;

rtexturepool_t *r_shadow_texturepool;
rtexture_t *r_shadow_attenuationgradienttexture;
skinframe_t *r_shadow_lightcorona;
rtexture_t *r_shadow_shadowmap2ddepthbuffer;
rtexture_t *r_shadow_shadowmap2ddepthtexture;
rtexture_t *r_shadow_shadowmapvsdcttexture;

GLuint r_shadow_prepassgeometryfbo;
GLuint r_shadow_prepasslightingdiffusespecularfbo;
GLuint r_shadow_prepasslightingdiffusefbo;
int r_shadow_prepass_width;
int r_shadow_prepass_height;
rtexture_t *r_shadow_prepassgeometrydepthbuffer;
rtexture_t *r_shadow_prepassgeometrynormalmaptexture;
rtexture_t *r_shadow_prepasslightingdiffusetexture;
rtexture_t *r_shadow_prepasslightingspeculartexture;

int r_shadow_viewfbo;
rtexture_t *r_shadow_viewdepthtexture;
rtexture_t *r_shadow_viewcolortexture;
int r_shadow_viewx;
int r_shadow_viewy;
int r_shadow_viewwidth;
int r_shadow_viewheight;

// lights are reloaded when this changes
char r_shadow_mapname[MAX_QPATH];

// buffer for doing corona fading
unsigned int r_shadow_occlusion_buf = 0;

// used only for light filters (cubemaps)
rtexturepool_t *r_shadow_filters_texturepool;

cvar_t r_shadow_bumpscale_basetexture = {CF_CLIENT, "r_shadow_bumpscale_basetexture", "0", "generate fake bumpmaps from diffuse textures at this bumpyness, try 4 to match tenebrae, higher values increase depth, requires r_restart to take effect"};
cvar_t r_shadow_bumpscale_bumpmap = {CF_CLIENT, "r_shadow_bumpscale_bumpmap", "4", "what magnitude to interpret _bump.tga textures as, higher values increase depth, requires r_restart to take effect"};
cvar_t r_shadow_debuglight = {CF_CLIENT, "r_shadow_debuglight", "-1", "renders only one light, for level design purposes or debugging"};
cvar_t r_shadow_deferred = {CF_CLIENT | CF_ARCHIVE, "r_shadow_deferred", "0", "uses image-based lighting instead of geometry-based lighting, the method used renders a depth image and a normalmap image, renders lights into separate diffuse and specular images, and then combines this into the normal rendering, requires r_shadow_shadowmapping"};
cvar_t r_shadow_usebihculling = {CF_CLIENT, "r_shadow_usebihculling", "1", "use BIH (Bounding Interval Hierarchy) for culling lit surfaces instead of BSP (Binary Space Partitioning)"};
cvar_t r_shadow_usenormalmap = {CF_CLIENT | CF_ARCHIVE, "r_shadow_usenormalmap", "1", "enables use of directional shading on lights"};
cvar_t r_shadow_gloss = {CF_CLIENT | CF_ARCHIVE, "r_shadow_gloss", "1", "0 disables gloss (specularity) rendering, 1 uses gloss if textures are found, 2 forces a flat metallic specular effect on everything without textures (similar to tenebrae)"};
cvar_t r_shadow_gloss2intensity = {CF_CLIENT, "r_shadow_gloss2intensity", "0.125", "how bright the forced flat gloss should look if r_shadow_gloss is 2"};
cvar_t r_shadow_glossintensity = {CF_CLIENT, "r_shadow_glossintensity", "1", "how bright textured glossmaps should look if r_shadow_gloss is 1 or 2"};
cvar_t r_shadow_glossexponent = {CF_CLIENT, "r_shadow_glossexponent", "32", "how 'sharp' the gloss should appear (specular power)"};
cvar_t r_shadow_gloss2exponent = {CF_CLIENT, "r_shadow_gloss2exponent", "32", "same as r_shadow_glossexponent but for forced gloss (gloss 2) surfaces"};
cvar_t r_shadow_glossexact = {CF_CLIENT, "r_shadow_glossexact", "0", "use exact reflection math for gloss (slightly slower, but should look a tad better)"};
cvar_t r_shadow_lightattenuationdividebias = {CF_CLIENT, "r_shadow_lightattenuationdividebias", "1", "changes attenuation texture generation"};
cvar_t r_shadow_lightattenuationlinearscale = {CF_CLIENT, "r_shadow_lightattenuationlinearscale", "2", "changes attenuation texture generation"};
cvar_t r_shadow_lightintensityscale = {CF_CLIENT, "r_shadow_lightintensityscale", "1", "renders all world lights brighter or darker"};
cvar_t r_shadow_lightradiusscale = {CF_CLIENT, "r_shadow_lightradiusscale", "1", "renders all world lights larger or smaller"};
cvar_t r_shadow_projectdistance = {CF_CLIENT, "r_shadow_projectdistance", "0", "how far to cast shadows"};
cvar_t r_shadow_frontsidecasting = {CF_CLIENT, "r_shadow_frontsidecasting", "1", "whether to cast shadows from illuminated triangles (front side of model) or unlit triangles (back side of model)"};
cvar_t r_shadow_realtime_dlight = {CF_CLIENT | CF_ARCHIVE, "r_shadow_realtime_dlight", "1", "enables rendering of dynamic lights such as explosions and rocket light"};
cvar_t r_shadow_realtime_dlight_shadows = {CF_CLIENT | CF_ARCHIVE, "r_shadow_realtime_dlight_shadows", "1", "enables rendering of shadows from dynamic lights"};
cvar_t r_shadow_realtime_dlight_svbspculling = {CF_CLIENT, "r_shadow_realtime_dlight_svbspculling", "0", "enables svbsp optimization on dynamic lights (very slow!)"};
cvar_t r_shadow_realtime_dlight_portalculling = {CF_CLIENT, "r_shadow_realtime_dlight_portalculling", "0", "enables portal optimization on dynamic lights (slow!)"};
cvar_t r_shadow_realtime_world = {CF_CLIENT | CF_ARCHIVE, "r_shadow_realtime_world", "0", "enables rendering of full world lighting (whether loaded from the map, or a .rtlights file, or a .ent file, or a .lights file produced by hlight)"};
cvar_t r_shadow_realtime_world_importlightentitiesfrommap = {CF_CLIENT, "r_shadow_realtime_world_importlightentitiesfrommap", "1", "load lights from .ent file or map entities at startup if no .rtlights or .lights file is present (if set to 2, always use the .ent or map entities)"};
cvar_t r_shadow_realtime_world_lightmaps = {CF_CLIENT | CF_ARCHIVE, "r_shadow_realtime_world_lightmaps", "0", "brightness to render lightmaps when using full world lighting, try 0.5 for a tenebrae-like appearance"};
cvar_t r_shadow_realtime_world_shadows = {CF_CLIENT | CF_ARCHIVE, "r_shadow_realtime_world_shadows", "1", "enables rendering of shadows from world lights"};
cvar_t r_shadow_realtime_world_compile = {CF_CLIENT, "r_shadow_realtime_world_compile", "1", "enables compilation of world lights for higher performance rendering"};
cvar_t r_shadow_realtime_world_compileshadow = {CF_CLIENT, "r_shadow_realtime_world_compileshadow", "1", "enables compilation of shadows from world lights for higher performance rendering"};
cvar_t r_shadow_realtime_world_compilesvbsp = {CF_CLIENT, "r_shadow_realtime_world_compilesvbsp", "1", "enables svbsp optimization during compilation (slower than compileportalculling but more exact)"};
cvar_t r_shadow_realtime_world_compileportalculling = {CF_CLIENT, "r_shadow_realtime_world_compileportalculling", "1", "enables portal-based culling optimization during compilation (overrides compilesvbsp)"};
cvar_t r_shadow_scissor = {CF_CLIENT, "r_shadow_scissor", "1", "use scissor optimization of light rendering (restricts rendering to the portion of the screen affected by the light)"};
cvar_t r_shadow_shadowmapping = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping", "1", "enables use of shadowmapping (shadow rendering by depth texture sampling)"};
cvar_t r_shadow_shadowmapping_filterquality = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_filterquality", "-1", "shadowmap filter modes: -1 = auto-select, 0 = no filtering, 1 = bilinear, 2 = bilinear 2x2 blur (fast), 3 = 3x3 blur (moderate), 4 = 4x4 blur (slow)"};
cvar_t r_shadow_shadowmapping_useshadowsampler = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_useshadowsampler", "1", "whether to use sampler2DShadow if available"};
cvar_t r_shadow_shadowmapping_depthbits = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_depthbits", "24", "requested minimum shadowmap texture depth bits"};
cvar_t r_shadow_shadowmapping_vsdct = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_vsdct", "1", "enables use of virtual shadow depth cube texture"};
cvar_t r_shadow_shadowmapping_minsize = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_minsize", "32", "limit of shadowmap side size - must be at least r_shadow_shadowmapping_bordersize+2"};
cvar_t r_shadow_shadowmapping_maxsize = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_maxsize", "512", "limit of shadowmap side size - can not be more than 1/8th of atlassize because lights store 6 sides (2x3 grid) and sometimes 12 sides (4x3 grid for shadows from EF_NOSELFSHADOW entities) and there are multiple lights..."};
cvar_t r_shadow_shadowmapping_texturesize = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_texturesize", "8192", "size of shadowmap atlas texture - all shadowmaps are packed into this texture at frame start"};
cvar_t r_shadow_shadowmapping_precision = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_precision", "1", "makes shadowmaps have a maximum resolution of this number of pixels per light source radius unit such that, for example, at precision 0.5 a light with radius 200 will have a maximum resolution of 100 pixels"};
//cvar_t r_shadow_shadowmapping_lod_bias = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_lod_bias", "16", "shadowmap size bias"};
//cvar_t r_shadow_shadowmapping_lod_scale = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_lod_scale", "128", "shadowmap size scaling parameter"};
cvar_t r_shadow_shadowmapping_bordersize = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_bordersize", "5", "shadowmap size bias for filtering"};
cvar_t r_shadow_shadowmapping_nearclip = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_nearclip", "1", "shadowmap nearclip in world units"};
cvar_t r_shadow_shadowmapping_bias = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_bias", "0.03", "shadowmap bias parameter (this is multiplied by nearclip * 1024 / lodsize)"};
cvar_t r_shadow_shadowmapping_polygonfactor = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_polygonfactor", "2", "slope-dependent shadowmapping bias"};
cvar_t r_shadow_shadowmapping_polygonoffset = {CF_CLIENT | CF_ARCHIVE, "r_shadow_shadowmapping_polygonoffset", "0", "constant shadowmapping bias"};
cvar_t r_shadow_sortsurfaces = {CF_CLIENT, "r_shadow_sortsurfaces", "1", "improve performance by sorting illuminated surfaces by texture"};
cvar_t r_shadow_culllights_pvs = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_pvs", "1", "check if light overlaps any visible bsp leafs when determining if the light is visible"};
cvar_t r_shadow_culllights_trace = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace", "1", "use raytraces from the eye to random places within light bounds to determine if the light is visible"};
cvar_t r_shadow_culllights_trace_eyejitter = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_eyejitter", "16", "offset eye location randomly by this much"};
cvar_t r_shadow_culllights_trace_enlarge = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_enlarge", "0", "make light bounds bigger by *(1.0+enlarge)"};
cvar_t r_shadow_culllights_trace_expand = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_expand", "8", "make light bounds bigger by this many units"};
cvar_t r_shadow_culllights_trace_pad = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_pad", "8", "accept traces that hit within this many units of the light bounds"};
cvar_t r_shadow_culllights_trace_samples = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_samples", "16", "use this many traces to random positions (in addition to center trace)"};
cvar_t r_shadow_culllights_trace_tempsamples = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_tempsamples", "16", "use this many traces if the light was created by csqc (no inter-frame caching), -1 disables the check (to avoid flicker entirely)"};
cvar_t r_shadow_culllights_trace_delay = {CF_CLIENT | CF_ARCHIVE, "r_shadow_culllights_trace_delay", "1", "light will be considered visible for this many seconds after any trace connects"};
cvar_t r_shadow_bouncegrid = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid", "0", "perform particle tracing for indirect lighting (Global Illumination / radiosity) using a 3D texture covering the scene, only active on levels with realtime lights active (r_shadow_realtime_world is usually required for these)"};
cvar_t r_shadow_bouncegrid_blur = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_blur", "0", "apply a 1-radius blur on bouncegrid to denoise it and deal with boundary issues with surfaces"};
cvar_t r_shadow_bouncegrid_dynamic_bounceminimumintensity = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_bounceminimumintensity", "0.05", "stop bouncing once intensity drops below this fraction of the original particle color"};
cvar_t r_shadow_bouncegrid_dynamic_culllightpaths = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_culllightpaths", "0", "skip accumulating light in the bouncegrid texture where the light paths are out of view (dynamic mode only)"};
cvar_t r_shadow_bouncegrid_dynamic_directionalshading = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_directionalshading", "1", "use diffuse shading rather than ambient, 3D texture becomes 8x as many pixels to hold the additional data"};
cvar_t r_shadow_bouncegrid_dynamic_dlightparticlemultiplier = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_dlightparticlemultiplier", "1", "if set to a high value like 16 this can make dlights look great, but 0 is recommended for performance reasons"};
cvar_t r_shadow_bouncegrid_dynamic_hitmodels = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_hitmodels", "0", "enables hitting character model geometry (SLOW)"};
cvar_t r_shadow_bouncegrid_dynamic_lightradiusscale = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_lightradiusscale", "5", "particles stop at this fraction of light radius (can be more than 1)"};
cvar_t r_shadow_bouncegrid_dynamic_maxbounce = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_maxbounce", "5", "maximum number of bounces for a particle (minimum is 0)"};
cvar_t r_shadow_bouncegrid_dynamic_maxphotons = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_maxphotons", "25000", "upper bound on photons to shoot per update, divided proportionately between lights - normally the number of photons is calculated by energyperphoton"};
cvar_t r_shadow_bouncegrid_dynamic_quality = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_quality", "1", "amount of photons that should be fired (this is multiplied by spacing ^ 2 to make it adaptive with spacing changes)"};
cvar_t r_shadow_bouncegrid_dynamic_spacing = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_spacing", "64", "unit size of bouncegrid pixel"};
cvar_t r_shadow_bouncegrid_dynamic_updateinterval = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_updateinterval", "0", "update bouncegrid texture once per this many seconds, useful values are 0, 0.05, or 1000000"};
cvar_t r_shadow_bouncegrid_dynamic_x = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_x", "64", "maximum texture size of bouncegrid on X axis"};
cvar_t r_shadow_bouncegrid_dynamic_y = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_y", "64", "maximum texture size of bouncegrid on Y axis"};
cvar_t r_shadow_bouncegrid_dynamic_z = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_dynamic_z", "32", "maximum texture size of bouncegrid on Z axis"};
cvar_t r_shadow_bouncegrid_floatcolors = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_floatcolors", "1", "upload texture as RGBA16F (or RGBA32F when set to 2) rather than RGBA8 format - this gives more dynamic range and accuracy"};
cvar_t r_shadow_bouncegrid_includedirectlighting = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_includedirectlighting", "0", "allows direct lighting to be recorded, not just indirect (gives an effect somewhat like r_shadow_realtime_world_lightmaps)"};
cvar_t r_shadow_bouncegrid_intensity = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_intensity", "4", "overall brightness of bouncegrid texture"};
cvar_t r_shadow_bouncegrid_lightpathsize = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_lightpathsize", "64", "radius (in game units) of the light path for accumulation of light in the bouncegrid texture"};
cvar_t r_shadow_bouncegrid_normalizevectors = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_normalizevectors", "1", "normalize random vectors (otherwise their length can vary, which dims the lighting further from the light)"};
cvar_t r_shadow_bouncegrid_particlebounceintensity = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_particlebounceintensity", "4", "amount of energy carried over after each bounce, this is a multiplier of texture color and the result is clamped to 1 or less, to prevent adding energy on each bounce"};
cvar_t r_shadow_bouncegrid_particleintensity = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_particleintensity", "1", "brightness of particles contributing to bouncegrid texture"};
cvar_t r_shadow_bouncegrid_rng_seed = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_rng_seed", "0", "0+ = use this number as RNG seed, -1 = use time instead for disco-like craziness in dynamic mode"};
cvar_t r_shadow_bouncegrid_rng_type = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_rng_type", "0", "0 = Lehmer 128bit RNG (slow but high quality), 1 = lhcheeserand 32bit RNG (quick)"};
cvar_t r_shadow_bouncegrid_static = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static", "1", "use static radiosity solution (high quality) rather than dynamic (splotchy)"};
cvar_t r_shadow_bouncegrid_static_bounceminimumintensity = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_bounceminimumintensity", "0.01", "stop bouncing once intensity drops below this fraction of the original particle color"};
cvar_t r_shadow_bouncegrid_static_directionalshading = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_directionalshading", "1", "whether to use directionalshading when in static mode"};
cvar_t r_shadow_bouncegrid_static_lightradiusscale = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_lightradiusscale", "5", "particles stop at this fraction of light radius (can be more than 1) when in static mode"};
cvar_t r_shadow_bouncegrid_static_maxbounce = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_maxbounce", "5", "maximum number of bounces for a particle (minimum is 0) in static mode"};
cvar_t r_shadow_bouncegrid_static_maxphotons = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_maxphotons", "250000", "upper bound on photons in static mode"};
cvar_t r_shadow_bouncegrid_static_quality = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_quality", "16", "amount of photons that should be fired (this is multiplied by spacing ^ 2 to make it adaptive with spacing changes)"};
cvar_t r_shadow_bouncegrid_static_spacing = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_static_spacing", "64", "unit size of bouncegrid pixel when in static mode"};
cvar_t r_shadow_bouncegrid_subsamples = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_subsamples", "1", "when generating the texture, sample this many points along each dimension (multisampling uses more compute but not more memory bandwidth)"};
cvar_t r_shadow_bouncegrid_threaded = {CF_CLIENT | CF_ARCHIVE, "r_shadow_bouncegrid_threaded", "1", "enables use of taskqueue_maxthreads to perform the traces and slice rendering of bouncegrid"};
cvar_t r_coronas = {CF_CLIENT | CF_ARCHIVE, "r_coronas", "0", "brightness of corona flare effects around certain lights, 0 disables corona effects"};
cvar_t r_coronas_occlusionsizescale = {CF_CLIENT | CF_ARCHIVE, "r_coronas_occlusionsizescale", "0.1", "size of light source for corona occlusion checksum the proportion of hidden pixels controls corona intensity"};
cvar_t r_coronas_occlusionquery = {CF_CLIENT | CF_ARCHIVE, "r_coronas_occlusionquery", "0", "fades coronas according to visibility, requires OpenGL 4.4"};
cvar_t gl_flashblend = {CF_CLIENT | CF_ARCHIVE, "gl_flashblend", "0", "render bright coronas for dynamic lights instead of actual lighting, fast but ugly"};
cvar_t r_editlights = {CF_CLIENT, "r_editlights", "0", "enables .rtlights file editing mode"};
cvar_t r_editlights_cursordistance = {CF_CLIENT, "r_editlights_cursordistance", "1024", "maximum distance of cursor from eye"};
cvar_t r_editlights_cursorpushback = {CF_CLIENT, "r_editlights_cursorpushback", "0", "how far to pull the cursor back toward the eye"};
cvar_t r_editlights_cursorpushoff = {CF_CLIENT, "r_editlights_cursorpushoff", "4", "how far to push the cursor off the impacted surface"};
cvar_t r_editlights_cursorgrid = {CF_CLIENT, "r_editlights_cursorgrid", "4", "snaps cursor to this grid size"};
cvar_t r_editlights_quakelightsizescale = {CF_CLIENT | CF_ARCHIVE, "r_editlights_quakelightsizescale", "1", "changes size of light entities loaded from a map"};
cvar_t r_editlights_drawproperties = {CF_CLIENT, "r_editlights_drawproperties", "1", "draw properties of currently selected light"};
cvar_t r_editlights_current_origin = {CF_CLIENT, "r_editlights_current_origin", "0 0 0", "origin of selected light"};
cvar_t r_editlights_current_angles = {CF_CLIENT, "r_editlights_current_angles", "0 0 0", "angles of selected light"};
cvar_t r_editlights_current_color = {CF_CLIENT, "r_editlights_current_color", "1 1 1", "color of selected light"};
cvar_t r_editlights_current_radius = {CF_CLIENT, "r_editlights_current_radius", "0", "radius of selected light"};
cvar_t r_editlights_current_corona = {CF_CLIENT, "r_editlights_current_corona", "0", "corona intensity of selected light"};
cvar_t r_editlights_current_coronasize = {CF_CLIENT, "r_editlights_current_coronasize", "0", "corona size of selected light"};
cvar_t r_editlights_current_style = {CF_CLIENT, "r_editlights_current_style", "0", "style of selected light"};
cvar_t r_editlights_current_shadows = {CF_CLIENT, "r_editlights_current_shadows", "0", "shadows flag of selected light"};
cvar_t r_editlights_current_cubemap = {CF_CLIENT, "r_editlights_current_cubemap", "0", "cubemap of selected light"};
cvar_t r_editlights_current_ambient = {CF_CLIENT, "r_editlights_current_ambient", "0", "ambient intensity of selected light"};
cvar_t r_editlights_current_diffuse = {CF_CLIENT, "r_editlights_current_diffuse", "1", "diffuse intensity of selected light"};
cvar_t r_editlights_current_specular = {CF_CLIENT, "r_editlights_current_specular", "1", "specular intensity of selected light"};
cvar_t r_editlights_current_normalmode = {CF_CLIENT, "r_editlights_current_normalmode", "0", "normalmode flag of selected light"};
cvar_t r_editlights_current_realtimemode = {CF_CLIENT, "r_editlights_current_realtimemode", "0", "realtimemode flag of selected light"};

r_shadow_bouncegrid_state_t r_shadow_bouncegrid_state;

// note the table actually includes one more value, just to avoid the need to clamp the distance index due to minor math error
#define ATTENTABLESIZE 256
// 1D gradient, 2D circle and 3D sphere attenuation textures
#define ATTEN1DSIZE 32
#define ATTEN2DSIZE 64
#define ATTEN3DSIZE 32

static float r_shadow_attendividebias; // r_shadow_lightattenuationdividebias
static float r_shadow_attenlinearscale; // r_shadow_lightattenuationlinearscale
static float r_shadow_attentable[ATTENTABLESIZE+1];

rtlight_t *r_shadow_compilingrtlight;
static memexpandablearray_t r_shadow_worldlightsarray;
dlight_t *r_shadow_selectedlight;
dlight_t r_shadow_bufferlight;
vec3_t r_editlights_cursorlocation;
qbool r_editlights_lockcursor;

extern int con_vislines;

void R_Shadow_UncompileWorldLights(void);
void R_Shadow_ClearWorldLights(void);
void R_Shadow_SaveWorldLights(void);
void R_Shadow_LoadWorldLights(void);
void R_Shadow_LoadLightsFile(void);
void R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite(void);
void R_Shadow_EditLights_Reload_f(cmd_state_t *cmd);
static void R_Shadow_MakeTextures(void);

#define EDLIGHTSPRSIZE			8
skinframe_t *r_editlights_sprcursor;
skinframe_t *r_editlights_sprlight;
skinframe_t *r_editlights_sprnoshadowlight;
skinframe_t *r_editlights_sprcubemaplight;
skinframe_t *r_editlights_sprcubemapnoshadowlight;
skinframe_t *r_editlights_sprselection;

static void R_Shadow_DrawModelShadowMaps(void);
static void R_Shadow_MakeShadowMap(int texturesize);
static void R_Shadow_MakeVSDCT(void);
static void R_Shadow_SetShadowMode(void)
{
	r_shadow_shadowmode_shadowmapping = r_shadow_shadowmapping.integer;
	r_shadow_shadowmode_deferred = r_shadow_deferred.integer;
	r_shadow_shadowmapborder = bound(1, r_shadow_shadowmapping_bordersize.integer, 16);
	r_shadow_shadowmaptexturesize = bound(256, r_shadow_shadowmapping_texturesize.integer, (int)vid.maxtexturesize_2d);
	r_shadow_shadowmapmaxsize = bound(r_shadow_shadowmapborder+2, r_shadow_shadowmapping_maxsize.integer, r_shadow_shadowmaptexturesize / 8);
	r_shadow_shadowmapvsdct = r_shadow_shadowmapping_vsdct.integer != 0 && vid.renderpath == RENDERPATH_GL32;
	r_shadow_shadowmapfilterquality = r_shadow_shadowmapping_filterquality.integer;
	r_shadow_shadowmapshadowsampler = r_shadow_shadowmapping_useshadowsampler.integer != 0;
	r_shadow_shadowmapdepthbits = r_shadow_shadowmapping_depthbits.integer;
	r_shadow_shadowmapsampler = false;
	r_shadow_shadowmappcf = 0;
	r_shadow_shadowmapdepthtexture = r_fb.usedepthtextures;
	r_shadow_shadowmode = R_SHADOW_SHADOWMODE_DISABLED;
	if (r_shadow_shadowmode_shadowmapping || r_shadow_shadowmode_deferred)
	{
		switch (vid.renderpath)
		{
		case RENDERPATH_GL32:
			if (r_shadow_shadowmapfilterquality < 0)
			{
				if (!r_fb.usedepthtextures)
					r_shadow_shadowmappcf = 1;
				else if ((strstr(gl_vendor, "NVIDIA") || strstr(gl_renderer, "Radeon HD")) && r_shadow_shadowmapshadowsampler)
				{
					r_shadow_shadowmapsampler = true;
					r_shadow_shadowmappcf = 1;
				}
				else if (vid.support.amd_texture_texture4 || vid.support.arb_texture_gather)
					r_shadow_shadowmappcf = 1;
				else if ((strstr(gl_vendor, "ATI") || strstr(gl_vendor, "Advanced Micro Devices")) && !strstr(gl_renderer, "Mesa") && !strstr(gl_version, "Mesa"))
					r_shadow_shadowmappcf = 1;
				else
					r_shadow_shadowmapsampler = r_shadow_shadowmapshadowsampler;
			}
			else
			{
				r_shadow_shadowmapsampler = r_shadow_shadowmapshadowsampler;
				switch (r_shadow_shadowmapfilterquality)
				{
				case 1:
					break;
				case 2:
					r_shadow_shadowmappcf = 1;
					break;
				case 3:
					r_shadow_shadowmappcf = 1;
					break;
				case 4:
					r_shadow_shadowmappcf = 2;
					break;
				}
			}
			if (!r_fb.usedepthtextures)
				r_shadow_shadowmapsampler = false;
			r_shadow_shadowmode = R_SHADOW_SHADOWMODE_SHADOWMAP2D;
			break;
		case RENDERPATH_GLES2:
			r_shadow_shadowmode = R_SHADOW_SHADOWMODE_SHADOWMAP2D;
			break;
		}
	}

	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		Mod_AllocLightmap_Init(&r_shadow_shadowmapatlas_state, r_main_mempool, r_shadow_shadowmaptexturesize, r_shadow_shadowmaptexturesize);
		break;
	case R_SHADOW_SHADOWMODE_DISABLED:
		break;
	}

	if(R_CompileShader_CheckStaticParms())
		R_GLSL_Restart_f(cmd_local);
}

qbool R_Shadow_ShadowMappingEnabled(void)
{
	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		return true;
	case R_SHADOW_SHADOWMODE_DISABLED:
		return false;
	}
	return false;
}

static void R_Shadow_FreeShadowMaps(void)
{
	R_Shadow_UncompileWorldLights();

	Mod_AllocLightmap_Free(&r_shadow_shadowmapatlas_state);

	R_Shadow_SetShadowMode();

	R_Mesh_DestroyFramebufferObject(r_shadow_fbo2d);

	r_shadow_fbo2d = 0;

	if (r_shadow_shadowmap2ddepthtexture)
		R_FreeTexture(r_shadow_shadowmap2ddepthtexture);
	r_shadow_shadowmap2ddepthtexture = NULL;

	if (r_shadow_shadowmap2ddepthbuffer)
		R_FreeTexture(r_shadow_shadowmap2ddepthbuffer);
	r_shadow_shadowmap2ddepthbuffer = NULL;

	if (r_shadow_shadowmapvsdcttexture)
		R_FreeTexture(r_shadow_shadowmapvsdcttexture);
	r_shadow_shadowmapvsdcttexture = NULL;

}

static void r_shadow_start(void)
{
	// allocate vertex processing arrays
	memset(&r_shadow_bouncegrid_state, 0, sizeof(r_shadow_bouncegrid_state));
	r_shadow_attenuationgradienttexture = NULL;
	r_shadow_shadowmode = R_SHADOW_SHADOWMODE_DISABLED;
	r_shadow_shadowmap2ddepthtexture = NULL;
	r_shadow_shadowmap2ddepthbuffer = NULL;
	r_shadow_shadowmapvsdcttexture = NULL;
	r_shadow_shadowmapmaxsize = 0;
	r_shadow_shadowmaptexturesize = 0;
	r_shadow_shadowmapfilterquality = -1;
	r_shadow_shadowmapdepthbits = 0;
	r_shadow_shadowmapvsdct = false;
	r_shadow_shadowmapsampler = false;
	r_shadow_shadowmappcf = 0;
	r_shadow_fbo2d = 0;

	R_Shadow_FreeShadowMaps();

	r_shadow_texturepool = NULL;
	r_shadow_filters_texturepool = NULL;
	R_Shadow_MakeTextures();
	r_shadow_scenemaxlights = 0;
	r_shadow_scenenumlights = 0;
	r_shadow_scenelightlist = NULL;
	maxshadowtriangles = 0;
	shadowelements = NULL;
	maxshadowvertices = 0;
	shadowvertex3f = NULL;
	maxvertexupdate = 0;
	vertexupdate = NULL;
	vertexremap = NULL;
	vertexupdatenum = 0;
	maxshadowmark = 0;
	numshadowmark = 0;
	shadowmark = NULL;
	shadowmarklist = NULL;
	shadowmarkcount = 0;
	maxshadowsides = 0;
	numshadowsides = 0;
	shadowsides = NULL;
	shadowsideslist = NULL;
	r_shadow_buffer_numleafpvsbytes = 0;
	r_shadow_buffer_visitingleafpvs = NULL;
	r_shadow_buffer_leafpvs = NULL;
	r_shadow_buffer_leaflist = NULL;
	r_shadow_buffer_numsurfacepvsbytes = 0;
	r_shadow_buffer_surfacepvs = NULL;
	r_shadow_buffer_surfacelist = NULL;
	r_shadow_buffer_surfacesides = NULL;
	r_shadow_buffer_numshadowtrispvsbytes = 0;
	r_shadow_buffer_shadowtrispvs = NULL;
	r_shadow_buffer_numlighttrispvsbytes = 0;
	r_shadow_buffer_lighttrispvs = NULL;

	r_shadow_usingdeferredprepass = false;
	r_shadow_prepass_width = r_shadow_prepass_height = 0;

	// determine renderpath specific capabilities, we don't need to figure
	// these out per frame...
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
		r_shadow_bouncegrid_state.allowdirectionalshading = true;
		r_shadow_bouncegrid_state.capable = true;
		break;
	case RENDERPATH_GLES2:
		// for performance reasons, do not use directional shading on GLES devices
		r_shadow_bouncegrid_state.capable = true;
		break;
	}
}

static void R_Shadow_BounceGrid_FreeHighPixels(void)
{
	r_shadow_bouncegrid_state.highpixels = NULL;
	if (r_shadow_bouncegrid_state.blurpixels[0]) { Mem_Free(r_shadow_bouncegrid_state.blurpixels[0]); r_shadow_bouncegrid_state.blurpixels[0] = NULL; }
	if (r_shadow_bouncegrid_state.blurpixels[1]) { Mem_Free(r_shadow_bouncegrid_state.blurpixels[1]); r_shadow_bouncegrid_state.blurpixels[1] = NULL; }
	if (r_shadow_bouncegrid_state.u8pixels)      { Mem_Free(r_shadow_bouncegrid_state.u8pixels);      r_shadow_bouncegrid_state.u8pixels      = NULL; }
	if (r_shadow_bouncegrid_state.fp16pixels)    { Mem_Free(r_shadow_bouncegrid_state.fp16pixels);    r_shadow_bouncegrid_state.fp16pixels    = NULL; }
	if (r_shadow_bouncegrid_state.photons)       { Mem_Free(r_shadow_bouncegrid_state.photons);       r_shadow_bouncegrid_state.photons       = NULL; }
	if (r_shadow_bouncegrid_state.photons_tasks) { Mem_Free(r_shadow_bouncegrid_state.photons_tasks); r_shadow_bouncegrid_state.photons_tasks = NULL; }
	if (r_shadow_bouncegrid_state.slices_tasks)  { Mem_Free(r_shadow_bouncegrid_state.slices_tasks);  r_shadow_bouncegrid_state.slices_tasks  = NULL; }
}

static void R_Shadow_FreeDeferred(void);
static void r_shadow_shutdown(void)
{
	CHECKGLERROR

	R_Shadow_FreeShadowMaps();

	r_shadow_usingdeferredprepass = false;
	if (r_shadow_prepass_width)
		R_Shadow_FreeDeferred();
	r_shadow_prepass_width = r_shadow_prepass_height = 0;

	CHECKGLERROR
	r_shadow_scenemaxlights = 0;
	r_shadow_scenenumlights = 0;
	if (r_shadow_scenelightlist)
		Mem_Free(r_shadow_scenelightlist);
	r_shadow_scenelightlist = NULL;
	R_Shadow_BounceGrid_FreeHighPixels();
	memset(&r_shadow_bouncegrid_state, 0, sizeof(r_shadow_bouncegrid_state));
	r_shadow_attenuationgradienttexture = NULL;
	R_FreeTexturePool(&r_shadow_texturepool);
	R_FreeTexturePool(&r_shadow_filters_texturepool);
	maxshadowtriangles = 0;
	if (shadowelements)
		Mem_Free(shadowelements);
	shadowelements = NULL;
	if (shadowvertex3f)
		Mem_Free(shadowvertex3f);
	shadowvertex3f = NULL;
	maxvertexupdate = 0;
	if (vertexupdate)
		Mem_Free(vertexupdate);
	vertexupdate = NULL;
	if (vertexremap)
		Mem_Free(vertexremap);
	vertexremap = NULL;
	vertexupdatenum = 0;
	maxshadowmark = 0;
	numshadowmark = 0;
	if (shadowmark)
		Mem_Free(shadowmark);
	shadowmark = NULL;
	if (shadowmarklist)
		Mem_Free(shadowmarklist);
	shadowmarklist = NULL;
	shadowmarkcount = 0;
	maxshadowsides = 0;
	numshadowsides = 0;
	if (shadowsides)
		Mem_Free(shadowsides);
	shadowsides = NULL;
	if (shadowsideslist)
		Mem_Free(shadowsideslist);
	shadowsideslist = NULL;
	r_shadow_buffer_numleafpvsbytes = 0;
	if (r_shadow_buffer_visitingleafpvs)
		Mem_Free(r_shadow_buffer_visitingleafpvs);
	r_shadow_buffer_visitingleafpvs = NULL;
	if (r_shadow_buffer_leafpvs)
		Mem_Free(r_shadow_buffer_leafpvs);
	r_shadow_buffer_leafpvs = NULL;
	if (r_shadow_buffer_leaflist)
		Mem_Free(r_shadow_buffer_leaflist);
	r_shadow_buffer_leaflist = NULL;
	r_shadow_buffer_numsurfacepvsbytes = 0;
	if (r_shadow_buffer_surfacepvs)
		Mem_Free(r_shadow_buffer_surfacepvs);
	r_shadow_buffer_surfacepvs = NULL;
	if (r_shadow_buffer_surfacelist)
		Mem_Free(r_shadow_buffer_surfacelist);
	r_shadow_buffer_surfacelist = NULL;
	if (r_shadow_buffer_surfacesides)
		Mem_Free(r_shadow_buffer_surfacesides);
	r_shadow_buffer_surfacesides = NULL;
	r_shadow_buffer_numshadowtrispvsbytes = 0;
	if (r_shadow_buffer_shadowtrispvs)
		Mem_Free(r_shadow_buffer_shadowtrispvs);
	r_shadow_buffer_numlighttrispvsbytes = 0;
	if (r_shadow_buffer_lighttrispvs)
		Mem_Free(r_shadow_buffer_lighttrispvs);
}

static void r_shadow_newmap(void)
{
	R_Shadow_BounceGrid_FreeHighPixels();

	if (r_shadow_bouncegrid_state.texture)    { R_FreeTexture(r_shadow_bouncegrid_state.texture);r_shadow_bouncegrid_state.texture = NULL; }
	if (r_shadow_lightcorona)                 { R_SkinFrame_MarkUsed(r_shadow_lightcorona); }
	if (r_editlights_sprcursor)               { R_SkinFrame_MarkUsed(r_editlights_sprcursor); }
	if (r_editlights_sprlight)                { R_SkinFrame_MarkUsed(r_editlights_sprlight); }
	if (r_editlights_sprnoshadowlight)        { R_SkinFrame_MarkUsed(r_editlights_sprnoshadowlight); }
	if (r_editlights_sprcubemaplight)         { R_SkinFrame_MarkUsed(r_editlights_sprcubemaplight); }
	if (r_editlights_sprcubemapnoshadowlight) { R_SkinFrame_MarkUsed(r_editlights_sprcubemapnoshadowlight); }
	if (r_editlights_sprselection)            { R_SkinFrame_MarkUsed(r_editlights_sprselection); }
	if (strncmp(cl.worldname, r_shadow_mapname, sizeof(r_shadow_mapname)))
		R_Shadow_EditLights_Reload_f(cmd_local);
}

void R_Shadow_Init(void)
{
	Cvar_RegisterVariable(&r_shadow_bumpscale_basetexture);
	Cvar_RegisterVariable(&r_shadow_bumpscale_bumpmap);
	Cvar_RegisterVariable(&r_shadow_usebihculling);
	Cvar_RegisterVariable(&r_shadow_usenormalmap);
	Cvar_RegisterVariable(&r_shadow_debuglight);
	Cvar_RegisterVariable(&r_shadow_deferred);
	Cvar_RegisterVariable(&r_shadow_gloss);
	Cvar_RegisterVariable(&r_shadow_gloss2intensity);
	Cvar_RegisterVariable(&r_shadow_glossintensity);
	Cvar_RegisterVariable(&r_shadow_glossexponent);
	Cvar_RegisterVariable(&r_shadow_gloss2exponent);
	Cvar_RegisterVariable(&r_shadow_glossexact);
	Cvar_RegisterVariable(&r_shadow_lightattenuationdividebias);
	Cvar_RegisterVariable(&r_shadow_lightattenuationlinearscale);
	Cvar_RegisterVariable(&r_shadow_lightintensityscale);
	Cvar_RegisterVariable(&r_shadow_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_projectdistance);
	Cvar_RegisterVariable(&r_shadow_frontsidecasting);
	Cvar_RegisterVariable(&r_shadow_realtime_world_importlightentitiesfrommap);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_shadows);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_svbspculling);
	Cvar_RegisterVariable(&r_shadow_realtime_dlight_portalculling);
	Cvar_RegisterVariable(&r_shadow_realtime_world);
	Cvar_RegisterVariable(&r_shadow_realtime_world_lightmaps);
	Cvar_RegisterVariable(&r_shadow_realtime_world_shadows);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compile);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compileshadow);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compilesvbsp);
	Cvar_RegisterVariable(&r_shadow_realtime_world_compileportalculling);
	Cvar_RegisterVariable(&r_shadow_scissor);
	Cvar_RegisterVariable(&r_shadow_shadowmapping);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_vsdct);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_filterquality);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_useshadowsampler);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_depthbits);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_precision);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_maxsize);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_minsize);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_texturesize);
//	Cvar_RegisterVariable(&r_shadow_shadowmapping_lod_bias);
//	Cvar_RegisterVariable(&r_shadow_shadowmapping_lod_scale);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_bordersize);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_nearclip);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_bias);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_polygonfactor);
	Cvar_RegisterVariable(&r_shadow_shadowmapping_polygonoffset);
	Cvar_RegisterVariable(&r_shadow_sortsurfaces);
	Cvar_RegisterVariable(&r_shadow_culllights_pvs);
	Cvar_RegisterVariable(&r_shadow_culllights_trace);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_eyejitter);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_enlarge);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_expand);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_pad);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_samples);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_tempsamples);
	Cvar_RegisterVariable(&r_shadow_culllights_trace_delay);
	Cvar_RegisterVariable(&r_shadow_bouncegrid);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_blur);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_bounceminimumintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_culllightpaths);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_directionalshading);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_dlightparticlemultiplier);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_hitmodels);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_maxbounce);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_maxphotons);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_quality);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_spacing);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_updateinterval);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_x);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_y);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_dynamic_z);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_floatcolors);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_includedirectlighting);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_intensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_lightpathsize);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_normalizevectors);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_particlebounceintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_particleintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_rng_seed);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_rng_type);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_bounceminimumintensity);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_directionalshading);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_lightradiusscale);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_maxbounce);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_maxphotons);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_quality);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_static_spacing);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_subsamples);
	Cvar_RegisterVariable(&r_shadow_bouncegrid_threaded);
	Cvar_RegisterVariable(&r_coronas);
	Cvar_RegisterVariable(&r_coronas_occlusionsizescale);
	Cvar_RegisterVariable(&r_coronas_occlusionquery);
	Cvar_RegisterVariable(&gl_flashblend);
	R_Shadow_EditLights_Init();
	Mem_ExpandableArray_NewArray(&r_shadow_worldlightsarray, r_main_mempool, sizeof(dlight_t), 128);
	r_shadow_scenemaxlights = 0;
	r_shadow_scenenumlights = 0;
	r_shadow_scenelightlist = NULL;
	maxshadowtriangles = 0;
	shadowelements = NULL;
	maxshadowvertices = 0;
	shadowvertex3f = NULL;
	maxvertexupdate = 0;
	vertexupdate = NULL;
	vertexremap = NULL;
	vertexupdatenum = 0;
	maxshadowmark = 0;
	numshadowmark = 0;
	shadowmark = NULL;
	shadowmarklist = NULL;
	shadowmarkcount = 0;
	maxshadowsides = 0;
	numshadowsides = 0;
	shadowsides = NULL;
	shadowsideslist = NULL;
	r_shadow_buffer_numleafpvsbytes = 0;
	r_shadow_buffer_visitingleafpvs = NULL;
	r_shadow_buffer_leafpvs = NULL;
	r_shadow_buffer_leaflist = NULL;
	r_shadow_buffer_numsurfacepvsbytes = 0;
	r_shadow_buffer_surfacepvs = NULL;
	r_shadow_buffer_surfacelist = NULL;
	r_shadow_buffer_surfacesides = NULL;
	r_shadow_buffer_shadowtrispvs = NULL;
	r_shadow_buffer_lighttrispvs = NULL;
	R_RegisterModule("R_Shadow", r_shadow_start, r_shadow_shutdown, r_shadow_newmap, NULL, NULL);
}

matrix4x4_t matrix_attenuationxyz =
{
	{
		{0.5, 0.0, 0.0, 0.5},
		{0.0, 0.5, 0.0, 0.5},
		{0.0, 0.0, 0.5, 0.5},
		{0.0, 0.0, 0.0, 1.0}
	}
};

matrix4x4_t matrix_attenuationz =
{
	{
		{0.0, 0.0, 0.5, 0.5},
		{0.0, 0.0, 0.0, 0.5},
		{0.0, 0.0, 0.0, 0.5},
		{0.0, 0.0, 0.0, 1.0}
	}
};

static void R_Shadow_ResizeShadowArrays(int numvertices, int numtriangles, int vertscale, int triscale)
{
	numvertices = ((numvertices + 255) & ~255) * vertscale;
	numtriangles = ((numtriangles + 255) & ~255) * triscale;
	// make sure shadowelements is big enough for this volume
	if (maxshadowtriangles < numtriangles)
	{
		maxshadowtriangles = numtriangles;
		if (shadowelements)
			Mem_Free(shadowelements);
		shadowelements = (int *)Mem_Alloc(r_main_mempool, maxshadowtriangles * sizeof(int[3]));
	}
	// make sure shadowvertex3f is big enough for this volume
	if (maxshadowvertices < numvertices)
	{
		maxshadowvertices = numvertices;
		if (shadowvertex3f)
			Mem_Free(shadowvertex3f);
		shadowvertex3f = (float *)Mem_Alloc(r_main_mempool, maxshadowvertices * sizeof(float[3]));
	}
}

static void R_Shadow_EnlargeLeafSurfaceTrisBuffer(int numleafs, int numsurfaces, int numshadowtriangles, int numlighttriangles)
{
	int numleafpvsbytes = (((numleafs + 7) >> 3) + 255) & ~255;
	int numsurfacepvsbytes = (((numsurfaces + 7) >> 3) + 255) & ~255;
	int numshadowtrispvsbytes = (((numshadowtriangles + 7) >> 3) + 255) & ~255;
	int numlighttrispvsbytes = (((numlighttriangles + 7) >> 3) + 255) & ~255;
	if (r_shadow_buffer_numleafpvsbytes < numleafpvsbytes)
	{
		if (r_shadow_buffer_visitingleafpvs)
			Mem_Free(r_shadow_buffer_visitingleafpvs);
		if (r_shadow_buffer_leafpvs)
			Mem_Free(r_shadow_buffer_leafpvs);
		if (r_shadow_buffer_leaflist)
			Mem_Free(r_shadow_buffer_leaflist);
		r_shadow_buffer_numleafpvsbytes = numleafpvsbytes;
		r_shadow_buffer_visitingleafpvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		r_shadow_buffer_leafpvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes);
		r_shadow_buffer_leaflist = (int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numleafpvsbytes * 8 * sizeof(*r_shadow_buffer_leaflist));
	}
	if (r_shadow_buffer_numsurfacepvsbytes < numsurfacepvsbytes)
	{
		if (r_shadow_buffer_surfacepvs)
			Mem_Free(r_shadow_buffer_surfacepvs);
		if (r_shadow_buffer_surfacelist)
			Mem_Free(r_shadow_buffer_surfacelist);
		if (r_shadow_buffer_surfacesides)
			Mem_Free(r_shadow_buffer_surfacesides);
		r_shadow_buffer_numsurfacepvsbytes = numsurfacepvsbytes;
		r_shadow_buffer_surfacepvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes);
		r_shadow_buffer_surfacelist = (int *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
		r_shadow_buffer_surfacesides = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numsurfacepvsbytes * 8 * sizeof(*r_shadow_buffer_surfacelist));
	}
	if (r_shadow_buffer_numshadowtrispvsbytes < numshadowtrispvsbytes)
	{
		if (r_shadow_buffer_shadowtrispvs)
			Mem_Free(r_shadow_buffer_shadowtrispvs);
		r_shadow_buffer_numshadowtrispvsbytes = numshadowtrispvsbytes;
		r_shadow_buffer_shadowtrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numshadowtrispvsbytes);
	}
	if (r_shadow_buffer_numlighttrispvsbytes < numlighttrispvsbytes)
	{
		if (r_shadow_buffer_lighttrispvs)
			Mem_Free(r_shadow_buffer_lighttrispvs);
		r_shadow_buffer_numlighttrispvsbytes = numlighttrispvsbytes;
		r_shadow_buffer_lighttrispvs = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_buffer_numlighttrispvsbytes);
	}
}

void R_Shadow_PrepareShadowMark(int numtris)
{
	// make sure shadowmark is big enough for this volume
	if (maxshadowmark < numtris)
	{
		maxshadowmark = numtris;
		if (shadowmark)
			Mem_Free(shadowmark);
		if (shadowmarklist)
			Mem_Free(shadowmarklist);
		shadowmark = (int *)Mem_Alloc(r_main_mempool, maxshadowmark * sizeof(*shadowmark));
		shadowmarklist = (int *)Mem_Alloc(r_main_mempool, maxshadowmark * sizeof(*shadowmarklist));
		shadowmarkcount = 0;
	}
	shadowmarkcount++;
	// if shadowmarkcount wrapped we clear the array and adjust accordingly
	if (shadowmarkcount == 0)
	{
		shadowmarkcount = 1;
		memset(shadowmark, 0, maxshadowmark * sizeof(*shadowmark));
	}
	numshadowmark = 0;
}

void R_Shadow_PrepareShadowSides(int numtris)
{
	if (maxshadowsides < numtris)
	{
		maxshadowsides = numtris;
		if (shadowsides)
			Mem_Free(shadowsides);
		if (shadowsideslist)
			Mem_Free(shadowsideslist);
		shadowsides = (unsigned char *)Mem_Alloc(r_main_mempool, maxshadowsides * sizeof(*shadowsides));
		shadowsideslist = (int *)Mem_Alloc(r_main_mempool, maxshadowsides * sizeof(*shadowsideslist));
	}
	numshadowsides = 0;
}

int R_Shadow_CalcTriangleSideMask(const vec3_t p1, const vec3_t p2, const vec3_t p3, float bias)
{
	// p1, p2, p3 are in the cubemap's local coordinate system
	// bias = border/(size - border)
	int mask = 0x3F;

	float dp1 = p1[0] + p1[1], dn1 = p1[0] - p1[1], ap1 = fabs(dp1), an1 = fabs(dn1),
		  dp2 = p2[0] + p2[1], dn2 = p2[0] - p2[1], ap2 = fabs(dp2), an2 = fabs(dn2),
		  dp3 = p3[0] + p3[1], dn3 = p3[0] - p3[1], ap3 = fabs(dp3), an3 = fabs(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<4)
			| (dp1 >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp2 >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp3 >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<4)
			| (dn1 >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2))
			| (dn2 >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2))			
			| (dn3 >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2));

	dp1 = p1[1] + p1[2], dn1 = p1[1] - p1[2], ap1 = fabs(dp1), an1 = fabs(dn1),
	dp2 = p2[1] + p2[2], dn2 = p2[1] - p2[2], ap2 = fabs(dp2), an2 = fabs(dn2),
	dp3 = p3[1] + p3[2], dn3 = p3[1] - p3[2], ap3 = fabs(dp3), an3 = fabs(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<0)
			| (dp1 >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4))
			| (dp2 >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4))			
			| (dp3 >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<0)
			| (dn1 >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn2 >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn3 >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4));

	dp1 = p1[2] + p1[0], dn1 = p1[2] - p1[0], ap1 = fabs(dp1), an1 = fabs(dn1),
	dp2 = p2[2] + p2[0], dn2 = p2[2] - p2[0], ap2 = fabs(dp2), an2 = fabs(dn2),
	dp3 = p3[2] + p3[0], dn3 = p3[2] - p3[0], ap3 = fabs(dp3), an3 = fabs(dn3);
	if(ap1 > bias*an1 && ap2 > bias*an2 && ap3 > bias*an3)
		mask &= (3<<2)
			| (dp1 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp2 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp3 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	if(an1 > bias*ap1 && an2 > bias*ap2 && an3 > bias*ap3)
		mask &= (3<<2)
			| (dn1 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn2 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn3 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0));

	return mask;
}

static int R_Shadow_CalcBBoxSideMask(const vec3_t mins, const vec3_t maxs, const matrix4x4_t *worldtolight, const matrix4x4_t *radiustolight, float bias)
{
	vec3_t center, radius, lightcenter, lightradius, pmin, pmax;
	float dp1, dn1, ap1, an1, dp2, dn2, ap2, an2;
	int mask = 0x3F;

	VectorSubtract(maxs, mins, radius);
	VectorScale(radius, 0.5f, radius);
	VectorAdd(mins, radius, center);
	Matrix4x4_Transform(worldtolight, center, lightcenter);
	Matrix4x4_Transform3x3(radiustolight, radius, lightradius);
	VectorSubtract(lightcenter, lightradius, pmin);
	VectorAdd(lightcenter, lightradius, pmax);

	dp1 = pmax[0] + pmax[1], dn1 = pmax[0] - pmin[1], ap1 = fabs(dp1), an1 = fabs(dn1),
	dp2 = pmin[0] + pmin[1], dn2 = pmin[0] - pmax[1], ap2 = fabs(dp2), an2 = fabs(dn2);
	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<4)
			| (dp1 >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2))
			| (dp2 >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<4)
			| (dn1 >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2))
			| (dn2 >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2));

	dp1 = pmax[1] + pmax[2], dn1 = pmax[1] - pmin[2], ap1 = fabs(dp1), an1 = fabs(dn1),
	dp2 = pmin[1] + pmin[2], dn2 = pmin[1] - pmax[2], ap2 = fabs(dp2), an2 = fabs(dn2);
	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<0)
			| (dp1 >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4))
			| (dp2 >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<0)
			| (dn1 >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4))
			| (dn2 >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4));

	dp1 = pmax[2] + pmax[0], dn1 = pmax[2] - pmin[0], ap1 = fabs(dp1), an1 = fabs(dn1),
	dp2 = pmin[2] + pmin[0], dn2 = pmin[2] - pmax[0], ap2 = fabs(dp2), an2 = fabs(dn2);
	if(ap1 > bias*an1 && ap2 > bias*an2)
		mask &= (3<<2)
			| (dp1 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0))
			| (dp2 >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	if(an1 > bias*ap1 && an2 > bias*ap2)
		mask &= (3<<2)
			| (dn1 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0))
			| (dn2 >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0));

	return mask;
}

#define R_Shadow_CalcEntitySideMask(ent, worldtolight, radiustolight, bias) R_Shadow_CalcBBoxSideMask((ent)->mins, (ent)->maxs, worldtolight, radiustolight, bias)

int R_Shadow_CalcSphereSideMask(const vec3_t p, float radius, float bias)
{
	// p is in the cubemap's local coordinate system
	// bias = border/(size - border)
	float dxyp = p[0] + p[1], dxyn = p[0] - p[1], axyp = fabs(dxyp), axyn = fabs(dxyn);
	float dyzp = p[1] + p[2], dyzn = p[1] - p[2], ayzp = fabs(dyzp), ayzn = fabs(dyzn);
	float dzxp = p[2] + p[0], dzxn = p[2] - p[0], azxp = fabs(dzxp), azxn = fabs(dzxn);
	int mask = 0x3F;
	if(axyp > bias*axyn + radius) mask &= dxyp < 0 ? ~((1<<0)|(1<<2)) : ~((2<<0)|(2<<2));
	if(axyn > bias*axyp + radius) mask &= dxyn < 0 ? ~((1<<0)|(2<<2)) : ~((2<<0)|(1<<2));
	if(ayzp > bias*ayzn + radius) mask &= dyzp < 0 ? ~((1<<2)|(1<<4)) : ~((2<<2)|(2<<4));
	if(ayzn > bias*ayzp + radius) mask &= dyzn < 0 ? ~((1<<2)|(2<<4)) : ~((2<<2)|(1<<4));
	if(azxp > bias*azxn + radius) mask &= dzxp < 0 ? ~((1<<4)|(1<<0)) : ~((2<<4)|(2<<0));
	if(azxn > bias*azxp + radius) mask &= dzxn < 0 ? ~((1<<4)|(2<<0)) : ~((2<<4)|(1<<0));
	return mask;
}

static int R_Shadow_CullFrustumSides(rtlight_t *rtlight, float size, float border)
{
	int i;
	vec3_t o, p, n;
	int sides = 0x3F, masks[6] = { 3<<4, 3<<4, 3<<0, 3<<0, 3<<2, 3<<2 };
	float scale = (size - 2*border)/size, len;
	float bias = border / (float)(size - border), dp, dn, ap, an;
	// check if cone enclosing side would cross frustum plane
	scale = 2 / (scale*scale + 2);
	Matrix4x4_OriginFromMatrix(&rtlight->matrix_lighttoworld, o);
	for (i = 0;i < 5;i++)
	{
		if (PlaneDiff(o, &r_refdef.view.frustum[i]) > -0.03125)
			continue;
		Matrix4x4_Transform3x3(&rtlight->matrix_worldtolight, r_refdef.view.frustum[i].normal, n);
		len = scale*VectorLength2(n);
		if(n[0]*n[0] > len) sides &= n[0] < 0 ? ~(1<<0) : ~(2 << 0);
		if(n[1]*n[1] > len) sides &= n[1] < 0 ? ~(1<<2) : ~(2 << 2);
		if(n[2]*n[2] > len) sides &= n[2] < 0 ? ~(1<<4) : ~(2 << 4);
	}
	if (PlaneDiff(o, &r_refdef.view.frustum[4]) >= r_refdef.farclip - r_refdef.nearclip + 0.03125)
	{
		Matrix4x4_Transform3x3(&rtlight->matrix_worldtolight, r_refdef.view.frustum[4].normal, n);
		len = scale*VectorLength2(n);
		if(n[0]*n[0] > len) sides &= n[0] >= 0 ? ~(1<<0) : ~(2 << 0);
		if(n[1]*n[1] > len) sides &= n[1] >= 0 ? ~(1<<2) : ~(2 << 2);
		if(n[2]*n[2] > len) sides &= n[2] >= 0 ? ~(1<<4) : ~(2 << 4);
	}
	// this next test usually clips off more sides than the former, but occasionally clips fewer/different ones, so do both and combine results
	// check if frustum corners/origin cross plane sides
#if 1
	// infinite version, assumes frustum corners merely give direction and extend to infinite distance
	Matrix4x4_Transform(&rtlight->matrix_worldtolight, r_refdef.view.origin, p);
	dp = p[0] + p[1], dn = p[0] - p[1], ap = fabs(dp), an = fabs(dn);
	masks[0] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
	masks[1] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2));
	dp = p[1] + p[2], dn = p[1] - p[2], ap = fabs(dp), an = fabs(dn);
	masks[2] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
	masks[3] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4));
	dp = p[2] + p[0], dn = p[2] - p[0], ap = fabs(dp), an = fabs(dn);
	masks[4] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
	masks[5] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0));
	for (i = 0;i < 4;i++)
	{
		Matrix4x4_Transform(&rtlight->matrix_worldtolight, r_refdef.view.frustumcorner[i], n);
		VectorSubtract(n, p, n);
		dp = n[0] + n[1], dn = n[0] - n[1], ap = fabs(dp), an = fabs(dn);
		if(ap > 0) masks[0] |= dp >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2);
		if(an > 0) masks[1] |= dn >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2);
		dp = n[1] + n[2], dn = n[1] - n[2], ap = fabs(dp), an = fabs(dn);
		if(ap > 0) masks[2] |= dp >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4);
		if(an > 0) masks[3] |= dn >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4);
		dp = n[2] + n[0], dn = n[2] - n[0], ap = fabs(dp), an = fabs(dn);
		if(ap > 0) masks[4] |= dp >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0);
		if(an > 0) masks[5] |= dn >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0);
	}
#else
	// finite version, assumes corners are a finite distance from origin dependent on far plane
	for (i = 0;i < 5;i++)
	{
		Matrix4x4_Transform(&rtlight->matrix_worldtolight, !i ? r_refdef.view.origin : r_refdef.view.frustumcorner[i-1], p);
		dp = p[0] + p[1], dn = p[0] - p[1], ap = fabs(dp), an = fabs(dn);
		masks[0] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<0)|(1<<2) : (2<<0)|(2<<2));
		masks[1] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<0)|(2<<2) : (2<<0)|(1<<2));
		dp = p[1] + p[2], dn = p[1] - p[2], ap = fabs(dp), an = fabs(dn);
		masks[2] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<2)|(1<<4) : (2<<2)|(2<<4));
		masks[3] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<2)|(2<<4) : (2<<2)|(1<<4));
		dp = p[2] + p[0], dn = p[2] - p[0], ap = fabs(dp), an = fabs(dn);
		masks[4] |= ap <= bias*an ? 0x3F : (dp >= 0 ? (1<<4)|(1<<0) : (2<<4)|(2<<0));
		masks[5] |= an <= bias*ap ? 0x3F : (dn >= 0 ? (1<<4)|(2<<0) : (2<<4)|(1<<0));
	}
#endif
	return sides & masks[0] & masks[1] & masks[2] & masks[3] & masks[4] & masks[5];
}

int R_Shadow_ChooseSidesFromBox(int firsttriangle, int numtris, const float *invertex3f, const int *elements, const matrix4x4_t *worldtolight, const vec3_t projectorigin, const vec3_t projectdirection, const vec3_t lightmins, const vec3_t lightmaxs, const vec3_t surfacemins, const vec3_t surfacemaxs, int *totals)
{
	int t, tend;
	const int *e;
	const float *v[3];
	float normal[3];
	vec3_t p[3];
	float bias;
	int mask, surfacemask = 0;
	if (!BoxesOverlap(lightmins, lightmaxs, surfacemins, surfacemaxs))
		return 0;
	bias = r_shadow_shadowmapborder / (float)(r_shadow_shadowmapmaxsize - r_shadow_shadowmapborder);
	tend = firsttriangle + numtris;
	if (BoxInsideBox(surfacemins, surfacemaxs, lightmins, lightmaxs))
	{
		// surface box entirely inside light box, no box cull
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3, v[2] = invertex3f + e[2] * 3;
				TriangleNormal(v[0], v[1], v[2], normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < 0))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3,	v[2] = invertex3f + e[2] * 3;
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, v[0], v[1], v[2]))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
	}
	else
	{
		// surface box not entirely inside light box, cull each triangle
		if (projectdirection)
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3,	v[2] = invertex3f + e[2] * 3;
				TriangleNormal(v[0], v[1], v[2], normal);
				if (r_shadow_frontsidecasting.integer == (DotProduct(normal, projectdirection) < 0)
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
		else
		{
			for (t = firsttriangle, e = elements + t * 3;t < tend;t++, e += 3)
			{
				v[0] = invertex3f + e[0] * 3, v[1] = invertex3f + e[1] * 3, v[2] = invertex3f + e[2] * 3;
				if (r_shadow_frontsidecasting.integer == PointInfrontOfTriangle(projectorigin, v[0], v[1], v[2])
				 && TriangleBBoxOverlapsBox(v[0], v[1], v[2], lightmins, lightmaxs))
				{
					Matrix4x4_Transform(worldtolight, v[0], p[0]), Matrix4x4_Transform(worldtolight, v[1], p[1]), Matrix4x4_Transform(worldtolight, v[2], p[2]);
					mask = R_Shadow_CalcTriangleSideMask(p[0], p[1], p[2], bias);
					surfacemask |= mask;
					if(totals)
					{
						totals[0] += mask&1, totals[1] += (mask>>1)&1, totals[2] += (mask>>2)&1, totals[3] += (mask>>3)&1, totals[4] += (mask>>4)&1, totals[5] += mask>>5;
						shadowsides[numshadowsides] = mask;
						shadowsideslist[numshadowsides++] = t;
					}
				}
			}
		}
	}
	return surfacemask;
}

void R_Shadow_ShadowMapFromList(int numverts, int numtris, const float *vertex3f, const int *elements, int numsidetris, const int *sidetotals, const unsigned char *sides, const int *sidetris)
{
	int i, j, outtriangles = 0;
	int *outelement3i[6];
	if (!numverts || !numsidetris || !r_shadow_compilingrtlight)
		return;
	outtriangles = sidetotals[0] + sidetotals[1] + sidetotals[2] + sidetotals[3] + sidetotals[4] + sidetotals[5];
	// make sure shadowelements is big enough for this mesh
	if (maxshadowtriangles < outtriangles)
		R_Shadow_ResizeShadowArrays(0, outtriangles, 0, 1);

	// compute the offset and size of the separate index lists for each cubemap side
	outtriangles = 0;
	for (i = 0;i < 6;i++)
	{
		outelement3i[i] = shadowelements + outtriangles * 3;
		r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap->sideoffsets[i] = outtriangles;
		r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap->sidetotals[i] = sidetotals[i];
		outtriangles += sidetotals[i];
	}

	// gather up the (sparse) triangles into separate index lists for each cubemap side
	for (i = 0;i < numsidetris;i++)
	{
		const int *element = elements + sidetris[i] * 3;
		for (j = 0;j < 6;j++)
		{
			if (sides[i] & (1 << j))
			{
				outelement3i[j][0] = element[0];
				outelement3i[j][1] = element[1];
				outelement3i[j][2] = element[2];
				outelement3i[j] += 3;
			}
		}
	}

	Mod_ShadowMesh_AddMesh(r_shadow_compilingrtlight->static_meshchain_shadow_shadowmap, vertex3f, outtriangles, shadowelements);
}

static void R_Shadow_MakeTextures_MakeCorona(void)
{
	float dx, dy;
	int x, y, a;
	unsigned char pixels[32][32][4];
	for (y = 0;y < 32;y++)
	{
		dy = (y - 15.5f) * (1.0f / 16.0f);
		for (x = 0;x < 32;x++)
		{
			dx = (x - 15.5f) * (1.0f / 16.0f);
			a = (int)(((1.0f / (dx * dx + dy * dy + 0.2f)) - (1.0f / (1.0f + 0.2))) * 32.0f / (1.0f / (1.0f + 0.2)));
			a = bound(0, a, 255);
			pixels[y][x][0] = a;
			pixels[y][x][1] = a;
			pixels[y][x][2] = a;
			pixels[y][x][3] = 255;
		}
	}
	r_shadow_lightcorona = R_SkinFrame_LoadInternalBGRA("lightcorona", TEXF_FORCELINEAR, &pixels[0][0][0], 32, 32, 0, 0, 0, false);
}

static unsigned int R_Shadow_MakeTextures_SamplePoint(float x, float y, float z)
{
	float dist = sqrt(x*x+y*y+z*z);
	float intensity = dist < 1 ? ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) : 0;
	// note this code could suffer byte order issues except that it is multiplying by an integer that reads the same both ways
	return bound(0, (unsigned int)(intensity * 256.0f), 255) * 0x01010101U;
}

static void R_Shadow_MakeTextures(void)
{
	int x;
	float intensity, dist;
	unsigned int *data;
	R_Shadow_FreeShadowMaps();
	R_FreeTexturePool(&r_shadow_texturepool);
	r_shadow_texturepool = R_AllocTexturePool();
	r_shadow_attenlinearscale = r_shadow_lightattenuationlinearscale.value;
	r_shadow_attendividebias = r_shadow_lightattenuationdividebias.value;
	data = (unsigned int *)Mem_Alloc(tempmempool, max(max(ATTEN3DSIZE*ATTEN3DSIZE*ATTEN3DSIZE, ATTEN2DSIZE*ATTEN2DSIZE), ATTEN1DSIZE) * 4);
	// the table includes one additional value to avoid the need to clamp indexing due to minor math errors
	for (x = 0;x <= ATTENTABLESIZE;x++)
	{
		dist = (x + 0.5f) * (1.0f / ATTENTABLESIZE) * (1.0f / 0.9375);
		intensity = dist < 1 ? ((1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) : 0;
		r_shadow_attentable[x] = bound(0, intensity, 1);
	}
	// 1D gradient texture
	for (x = 0;x < ATTEN1DSIZE;x++)
		data[x] = R_Shadow_MakeTextures_SamplePoint((x + 0.5f) * (1.0f / ATTEN1DSIZE) * (1.0f / 0.9375), 0, 0);
	r_shadow_attenuationgradienttexture = R_LoadTexture2D(r_shadow_texturepool, "attenuation1d", ATTEN1DSIZE, 1, (unsigned char *)data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, -1, NULL);
	Mem_Free(data);

	R_Shadow_MakeTextures_MakeCorona();

	// Editor light sprites
	r_editlights_sprcursor = R_SkinFrame_LoadInternal8bit("gfx/editlights/cursor", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	".3............3."
	"..5...2332...5.."
	"...7.3....3.7..."
	"....7......7...."
	"...3.7....7.3..."
	"..2...7..7...2.."
	"..3..........3.."
	"..3..........3.."
	"..2...7..7...2.."
	"...3.7....7.3..."
	"....7......7...."
	"...7.3....3.7..."
	"..5...2332...5.."
	".3............3."
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/light", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......1111......"
	"....11233211...."
	"...1234554321..."
	"...1356776531..."
	"..124677776421.."
	"..135777777531.."
	"..135777777531.."
	"..124677776421.."
	"...1356776531..."
	"...1234554321..."
	"....11233211...."
	"......1111......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprnoshadowlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/noshadow", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......1111......"
	"....11233211...."
	"...1234554321..."
	"...1356226531..."
	"..12462..26421.."
	"..1352....2531.."
	"..1352....2531.."
	"..12462..26421.."
	"...1356226531..."
	"...1234554321..."
	"....11233211...."
	"......1111......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprcubemaplight = R_SkinFrame_LoadInternal8bit("gfx/editlights/cubemaplight", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......2772......"
	"....27755772...."
	"..277533335772.."
	"..753333333357.."
	"..777533335777.."
	"..735775577537.."
	"..733357753337.."
	"..733337733337.."
	"..753337733357.."
	"..277537735772.."
	"....27777772...."
	"......2772......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprcubemapnoshadowlight = R_SkinFrame_LoadInternal8bit("gfx/editlights/cubemapnoshadowlight", TEXF_ALPHA | TEXF_CLAMP, (const unsigned char *)
	"................"
	"................"
	"......2772......"
	"....27722772...."
	"..2772....2772.."
	"..72........27.."
	"..7772....2777.."
	"..7.27722772.7.."
	"..7...2772...7.."
	"..7....77....7.."
	"..72...77...27.."
	"..2772.77.2772.."
	"....27777772...."
	"......2772......"
	"................"
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
	r_editlights_sprselection = R_SkinFrame_LoadInternal8bit("gfx/editlights/selection", TEXF_ALPHA | TEXF_CLAMP, (unsigned char *)
	"................"
	".777752..257777."
	".742........247."
	".72..........27."
	".7............7."
	".5............5."
	".2............2."
	"................"
	"................"
	".2............2."
	".5............5."
	".7............7."
	".72..........27."
	".742........247."
	".777752..257777."
	"................"
	, 16, 16, palette_bgra_embeddedpic, palette_bgra_embeddedpic);
}

void R_Shadow_RenderMode_Begin(void)
{
#if 0
	GLint drawbuffer;
	GLint readbuffer;
#endif

	if (r_shadow_lightattenuationdividebias.value != r_shadow_attendividebias
	 || r_shadow_lightattenuationlinearscale.value != r_shadow_attenlinearscale)
		R_Shadow_MakeTextures();

	CHECKGLERROR
	R_Mesh_ResetTextureState();
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	GL_DepthTest(true);
	GL_DepthMask(false);
	GL_Color(0, 0, 0, 1);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	
	r_shadow_rendermode = R_SHADOW_RENDERMODE_NONE;
	r_shadow_lightingrendermode = R_SHADOW_RENDERMODE_LIGHT_GLSL;

	CHECKGLERROR
#if 0
	qglGetIntegerv(GL_DRAW_BUFFER, &drawbuffer);CHECKGLERROR
	qglGetIntegerv(GL_READ_BUFFER, &readbuffer);CHECKGLERROR
	r_shadow_drawbuffer = drawbuffer;
	r_shadow_readbuffer = readbuffer;
#endif
	r_shadow_cullface_front = r_refdef.view.cullface_front;
	r_shadow_cullface_back = r_refdef.view.cullface_back;
}

void R_Shadow_RenderMode_ActiveLight(const rtlight_t *rtlight)
{
	rsurface.rtlight = rtlight;
}

void R_Shadow_RenderMode_Reset(void)
{
	R_Mesh_ResetTextureState();
	R_Mesh_SetRenderTargets(r_shadow_viewfbo);
	R_SetViewport(&r_refdef.view.viewport);
	GL_Scissor(r_shadow_lightscissor[0], r_shadow_lightscissor[1], r_shadow_lightscissor[2], r_shadow_lightscissor[3]);
	GL_DepthRange(0, 1);
	GL_DepthTest(true);
	GL_DepthMask(false);
	GL_DepthFunc(GL_LEQUAL);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);CHECKGLERROR
	r_refdef.view.cullface_front = r_shadow_cullface_front;
	r_refdef.view.cullface_back = r_shadow_cullface_back;
	GL_CullFace(r_refdef.view.cullface_back);
	GL_Color(1, 1, 1, 1);
	GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	R_SetupShader_Generic_NoTexture(false, false);
	r_shadow_usingshadowmap2d = false;
}

void R_Shadow_ClearStencil(void)
{
	GL_Clear(GL_STENCIL_BUFFER_BIT, NULL, 1.0f, 0);
	r_refdef.stats[r_stat_lights_clears]++;
}

static void R_Shadow_MakeVSDCT(void)
{
	// maps to a 2x3 texture rectangle with normalized coordinates
	// +-
	// XX
	// YY
	// ZZ
	// stores abs(dir.xy), offset.xy/2.5
	unsigned char data[4*6] =
	{
		255, 0, 0x33, 0x33, // +X: <1, 0>, <0.5, 0.5>
		255, 0, 0x99, 0x33, // -X: <1, 0>, <1.5, 0.5>
		0, 255, 0x33, 0x99, // +Y: <0, 1>, <0.5, 1.5>
		0, 255, 0x99, 0x99, // -Y: <0, 1>, <1.5, 1.5>
		0,   0, 0x33, 0xFF, // +Z: <0, 0>, <0.5, 2.5>
		0,   0, 0x99, 0xFF, // -Z: <0, 0>, <1.5, 2.5>
	};
	r_shadow_shadowmapvsdcttexture = R_LoadTextureCubeMap(r_shadow_texturepool, "shadowmapvsdct", 1, data, TEXTYPE_RGBA, TEXF_FORCENEAREST | TEXF_CLAMP | TEXF_ALPHA, -1, NULL);
}

static void R_Shadow_MakeShadowMap(int texturesize)
{
	switch (r_shadow_shadowmode)
	{
	case R_SHADOW_SHADOWMODE_SHADOWMAP2D:
		if (r_shadow_shadowmap2ddepthtexture) return;
		if (r_fb.usedepthtextures)
		{
			r_shadow_shadowmap2ddepthtexture = R_LoadTextureShadowMap2D(r_shadow_texturepool, "shadowmap", texturesize, texturesize, r_shadow_shadowmapdepthbits >= 24 ? (r_shadow_shadowmapsampler ? TEXTYPE_SHADOWMAP24_COMP : TEXTYPE_SHADOWMAP24_RAW) : (r_shadow_shadowmapsampler ? TEXTYPE_SHADOWMAP16_COMP : TEXTYPE_SHADOWMAP16_RAW), r_shadow_shadowmapsampler);
			r_shadow_shadowmap2ddepthbuffer = NULL;
			r_shadow_fbo2d = R_Mesh_CreateFramebufferObject(r_shadow_shadowmap2ddepthtexture, NULL, NULL, NULL, NULL);
		}
		else
		{
			r_shadow_shadowmap2ddepthtexture = R_LoadTexture2D(r_shadow_texturepool, "shadowmaprendertarget", texturesize, texturesize, NULL, TEXTYPE_COLORBUFFER, TEXF_RENDERTARGET | TEXF_FORCENEAREST | TEXF_CLAMP | TEXF_ALPHA, -1, NULL);
			r_shadow_shadowmap2ddepthbuffer = R_LoadTextureRenderBuffer(r_shadow_texturepool, "shadowmap", texturesize, texturesize, r_shadow_shadowmapdepthbits >= 24 ? TEXTYPE_DEPTHBUFFER24 : TEXTYPE_DEPTHBUFFER16);
			r_shadow_fbo2d = R_Mesh_CreateFramebufferObject(r_shadow_shadowmap2ddepthbuffer, r_shadow_shadowmap2ddepthtexture, NULL, NULL, NULL);
		}
		break;
	case R_SHADOW_SHADOWMODE_DISABLED:
		break;
	}
}

void R_Shadow_ClearShadowMapTexture(void)
{
	r_viewport_t viewport;
	float clearcolor[4];

	// if they don't exist, create our textures now
	if (!r_shadow_shadowmap2ddepthtexture)
		R_Shadow_MakeShadowMap(r_shadow_shadowmaptexturesize);
	if (r_shadow_shadowmapvsdct && !r_shadow_shadowmapvsdcttexture)
		R_Shadow_MakeVSDCT();

	// we're setting up to render shadowmaps, so change rendermode
	r_shadow_rendermode = R_SHADOW_RENDERMODE_SHADOWMAP2D;

	R_Mesh_ResetTextureState();
	R_Shadow_RenderMode_Reset();
	if (r_shadow_shadowmap2ddepthbuffer)
		R_Mesh_SetRenderTargets(r_shadow_fbo2d);
	else
		R_Mesh_SetRenderTargets(r_shadow_fbo2d);
	R_SetupShader_DepthOrShadow(true, r_shadow_shadowmap2ddepthbuffer != NULL, false); // FIXME test if we have a skeletal model?
	GL_PolygonOffset(r_shadow_shadowmapping_polygonfactor.value, r_shadow_shadowmapping_polygonoffset.value);
	GL_DepthMask(true);
	GL_DepthTest(true);

	// we have to set a viewport to clear anything in some renderpaths (D3D)
	R_Viewport_InitOrtho(&viewport, &identitymatrix, 0, 0, r_shadow_shadowmaptexturesize, r_shadow_shadowmaptexturesize, 0, 0, 1.0, 1.0, 0.001f, 1.0f, NULL);
	R_SetViewport(&viewport);
	GL_Scissor(viewport.x, viewport.y, viewport.width, viewport.height);
	if (r_shadow_shadowmap2ddepthbuffer)
		GL_ColorMask(1, 1, 1, 1);
	else
		GL_ColorMask(0, 0, 0, 0);
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		GL_CullFace(r_refdef.view.cullface_back);
		break;
	}
	Vector4Set(clearcolor, 1, 1, 1, 1);
	if (r_shadow_shadowmap2ddepthbuffer)
		GL_Clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT, clearcolor, 1.0f, 0);
	else
		GL_Clear(GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
}

static void R_Shadow_SetShadowmapParametersForLight(qbool noselfshadowpass)
{
	int size = rsurface.rtlight->shadowmapatlassidesize;
	float nearclip = r_shadow_shadowmapping_nearclip.value / rsurface.rtlight->radius;
	float farclip = 1.0f;
	float bias = r_shadow_shadowmapping_bias.value * nearclip * (1024.0f / size);// * rsurface.rtlight->radius;
	r_shadow_lightshadowmap_texturescale[0] = 1.0f / R_TextureWidth(r_shadow_shadowmap2ddepthtexture);
	r_shadow_lightshadowmap_texturescale[1] = 1.0f / R_TextureHeight(r_shadow_shadowmap2ddepthtexture);
	r_shadow_lightshadowmap_texturescale[2] = rsurface.rtlight->shadowmapatlasposition[0] + (noselfshadowpass ? size * 2 : 0);
	r_shadow_lightshadowmap_texturescale[3] = rsurface.rtlight->shadowmapatlasposition[1];
	r_shadow_lightshadowmap_parameters[0] = 0.5f * (size - r_shadow_shadowmapborder);
	r_shadow_lightshadowmap_parameters[1] = -nearclip * farclip / (farclip - nearclip) - 0.5f * bias;
	r_shadow_lightshadowmap_parameters[2] = r_shadow_shadowmapvsdct ? 2.5f*size : size;
	r_shadow_lightshadowmap_parameters[3] = 0.5f + 0.5f * (farclip + nearclip) / (farclip - nearclip);
	if (r_shadow_shadowmap2ddepthbuffer)
	{
		// completely different meaning than in depthtexture approach
		r_shadow_lightshadowmap_parameters[1] = 0;
		r_shadow_lightshadowmap_parameters[3] = -bias;
	}
}

static void R_Shadow_RenderMode_ShadowMap(int side, int size, int x, int y)
{
	float nearclip, farclip;
	r_viewport_t viewport;
	int flipped;

	if (r_shadow_rendermode != R_SHADOW_RENDERMODE_SHADOWMAP2D)
	{
		r_shadow_rendermode = R_SHADOW_RENDERMODE_SHADOWMAP2D;

		R_Mesh_ResetTextureState();
		R_Shadow_RenderMode_Reset();
		if (r_shadow_shadowmap2ddepthbuffer)
			R_Mesh_SetRenderTargets(r_shadow_fbo2d);
		else
			R_Mesh_SetRenderTargets(r_shadow_fbo2d);
		R_SetupShader_DepthOrShadow(true, r_shadow_shadowmap2ddepthbuffer != NULL, false); // FIXME test if we have a skeletal model?
		GL_PolygonOffset(r_shadow_shadowmapping_polygonfactor.value, r_shadow_shadowmapping_polygonoffset.value);
		GL_DepthMask(true);
		GL_DepthTest(true);
	}

	nearclip = r_shadow_shadowmapping_nearclip.value / rsurface.rtlight->radius;
	farclip = 1.0f;

	R_Viewport_InitRectSideView(&viewport, &rsurface.rtlight->matrix_lighttoworld, side, size, r_shadow_shadowmapborder, nearclip, farclip, NULL, x, y);
	R_SetViewport(&viewport);
	GL_Scissor(viewport.x, viewport.y, viewport.width, viewport.height);
	flipped = (side & 1) ^ (side >> 2);
	r_refdef.view.cullface_front = flipped ? r_shadow_cullface_back : r_shadow_cullface_front;
	r_refdef.view.cullface_back = flipped ? r_shadow_cullface_front : r_shadow_cullface_back;

	if (r_shadow_shadowmap2ddepthbuffer)
		GL_ColorMask(1,1,1,1);
	else
		GL_ColorMask(0,0,0,0);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		GL_CullFace(r_refdef.view.cullface_back);
		break;
	}

	// used in R_Q1BSP_DrawShadowMap code to check surfacesides[]
	r_shadow_shadowmapside = side;
}

void R_Shadow_RenderMode_Lighting(qbool transparent, qbool shadowmapping, qbool noselfshadowpass)
{
	R_Mesh_ResetTextureState();
	if (transparent)
	{
		r_shadow_lightscissor[0] = r_refdef.view.viewport.x;
		r_shadow_lightscissor[1] = r_refdef.view.viewport.y;
		r_shadow_lightscissor[2] = r_refdef.view.viewport.width;
		r_shadow_lightscissor[3] = r_refdef.view.viewport.height;
	}
	if (shadowmapping)
		R_Shadow_SetShadowmapParametersForLight(noselfshadowpass);
	R_Shadow_RenderMode_Reset();
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	if (!transparent)
		GL_DepthFunc(GL_EQUAL);
	// do global setup needed for the chosen lighting mode
	if (r_shadow_rendermode == R_SHADOW_RENDERMODE_LIGHT_GLSL)
		GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 0);
	r_shadow_usingshadowmap2d = shadowmapping;
	r_shadow_rendermode = r_shadow_lightingrendermode;
}

static const unsigned short bboxelements[36] =
{
	5, 1, 3, 5, 3, 7,
	6, 2, 0, 6, 0, 4,
	7, 3, 2, 7, 2, 6,
	4, 0, 1, 4, 1, 5,
	4, 5, 7, 4, 7, 6,
	1, 0, 2, 1, 2, 3,
};

static const float bboxpoints[8][3] =
{
	{-1,-1,-1},
	{ 1,-1,-1},
	{-1, 1,-1},
	{ 1, 1,-1},
	{-1,-1, 1},
	{ 1,-1, 1},
	{-1, 1, 1},
	{ 1, 1, 1},
};

void R_Shadow_RenderMode_DrawDeferredLight(qbool shadowmapping)
{
	int i;
	float vertex3f[8*3];
	const matrix4x4_t *matrix = &rsurface.rtlight->matrix_lighttoworld;
// do global setup needed for the chosen lighting mode
	R_Shadow_RenderMode_Reset();
	r_shadow_rendermode = r_shadow_lightingrendermode;
	R_EntityMatrix(&identitymatrix);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
	if (rsurface.rtlight->specularscale > 0 && r_shadow_gloss.integer > 0)
		R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo);
	else
		R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusefbo);

	r_shadow_usingshadowmap2d = shadowmapping;

	// render the lighting
	R_SetupShader_DeferredLight(rsurface.rtlight);
	for (i = 0;i < 8;i++)
		Matrix4x4_Transform(matrix, bboxpoints[i], vertex3f + i*3);
	GL_ColorMask(1,1,1,1);
	GL_DepthMask(false);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(0, 0);
	GL_DepthTest(true);
	GL_DepthFunc(GL_GREATER);
	GL_CullFace(r_refdef.view.cullface_back);
	R_Mesh_PrepareVertices_Vertex3f(8, vertex3f, NULL, 0);
	R_Mesh_Draw(0, 8, 0, 12, NULL, NULL, 0, bboxelements, NULL, 0);
}

static qbool R_Shadow_BounceGrid_CheckEnable(int flag)
{
	qbool enable = r_shadow_bouncegrid_state.capable && r_shadow_bouncegrid.integer != 0 && r_refdef.scene.worldmodel;
	int lightindex;
	int range;
	dlight_t *light;
	rtlight_t *rtlight;
	vec3_t lightcolor;

	// see if there are really any lights to render...
	if (enable && r_shadow_bouncegrid_static.integer)
	{
		enable = false;
		range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
		for (lightindex = 0;lightindex < range;lightindex++)
		{
			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light || !(light->flags & flag))
				continue;
			rtlight = &light->rtlight;
			// when static, we skip styled lights because they tend to change...
			if (rtlight->style > 0)
				continue;
			VectorScale(rtlight->color, (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale), lightcolor);
			if (!VectorLength2(lightcolor))
				continue;
			enable = true;
			break;
		}
	}

	return enable;
}

static void R_Shadow_BounceGrid_GenerateSettings(r_shadow_bouncegrid_settings_t *settings)
{
	qbool s = r_shadow_bouncegrid_static.integer != 0;
	float spacing = bound(1.0f, s ? r_shadow_bouncegrid_static_spacing.value : r_shadow_bouncegrid_dynamic_spacing.value, 1024.0f);
	float quality = bound(0.0001f, (s ? r_shadow_bouncegrid_static_quality.value : r_shadow_bouncegrid_dynamic_quality.value), 1024.0f);
	float bounceminimumintensity = s ? r_shadow_bouncegrid_static_bounceminimumintensity.value : r_shadow_bouncegrid_dynamic_bounceminimumintensity.value;

	// prevent any garbage in alignment padded areas as we'll be using memcmp
	memset(settings, 0, sizeof(*settings));

	// build up a complete collection of the desired settings, so that memcmp can be used to compare parameters
	settings->staticmode                    = s;
	settings->blur                          = r_shadow_bouncegrid_blur.integer != 0;
	settings->floatcolors                   = bound(0, r_shadow_bouncegrid_floatcolors.integer, 2);
	settings->lightpathsize                 = bound(0.0f, r_shadow_bouncegrid_lightpathsize.value, 1024.0f);
	settings->directionalshading            = (s ? r_shadow_bouncegrid_static_directionalshading.integer != 0 : r_shadow_bouncegrid_dynamic_directionalshading.integer != 0) && r_shadow_bouncegrid_state.allowdirectionalshading;
	settings->dlightparticlemultiplier      = s ? 0 : r_shadow_bouncegrid_dynamic_dlightparticlemultiplier.value;
	settings->hitmodels                     = s ? false : r_shadow_bouncegrid_dynamic_hitmodels.integer != 0;
	settings->includedirectlighting         = r_shadow_bouncegrid_includedirectlighting.integer != 0 || r_shadow_bouncegrid.integer == 2;
	settings->lightradiusscale              = (s ? r_shadow_bouncegrid_static_lightradiusscale.value : r_shadow_bouncegrid_dynamic_lightradiusscale.value);
	settings->maxbounce                     = (s ? r_shadow_bouncegrid_static_maxbounce.integer : r_shadow_bouncegrid_dynamic_maxbounce.integer);
	settings->particlebounceintensity       = r_shadow_bouncegrid_particlebounceintensity.value;
	settings->particleintensity             = r_shadow_bouncegrid_particleintensity.value * (settings->directionalshading ? 4.0f : 1.0f) / 65536.0f;
	settings->maxphotons                    = s ? r_shadow_bouncegrid_static_maxphotons.integer : r_shadow_bouncegrid_dynamic_maxphotons.integer;
	settings->energyperphoton               = 4096.0f / quality;
	settings->spacing[0]                    = spacing;
	settings->spacing[1]                    = spacing;
	settings->spacing[2]                    = spacing;
	settings->rng_type                      = r_shadow_bouncegrid_rng_type.integer;
	settings->rng_seed                      = r_shadow_bouncegrid_rng_seed.integer;
	settings->bounceminimumintensity2       = bounceminimumintensity * bounceminimumintensity;
	settings->normalizevectors              = r_shadow_bouncegrid_normalizevectors.integer != 0;
	settings->subsamples                    = bound(1, r_shadow_bouncegrid_subsamples.integer, 4);

	// bound the values for sanity
	settings->maxphotons = bound(1, settings->maxphotons, 25000000);
	settings->lightradiusscale = bound(0.0001f, settings->lightradiusscale, 1024.0f);
	settings->maxbounce = bound(0, settings->maxbounce, 16);
	settings->spacing[0] = bound(1, settings->spacing[0], 512);
	settings->spacing[1] = bound(1, settings->spacing[1], 512);
	settings->spacing[2] = bound(1, settings->spacing[2], 512);
}

static void R_Shadow_BounceGrid_UpdateSpacing(void)
{
	float m[16];
	int c[4];
	int resolution[3];
	int numpixels;
	vec3_t ispacing;
	vec3_t maxs = {0,0,0};
	vec3_t mins = {0,0,0};
	vec3_t size;
	vec3_t spacing;
	r_shadow_bouncegrid_settings_t *settings = &r_shadow_bouncegrid_state.settings;

	// get the spacing values
	spacing[0] = settings->spacing[0];
	spacing[1] = settings->spacing[1];
	spacing[2] = settings->spacing[2];
	ispacing[0] = 1.0f / spacing[0];
	ispacing[1] = 1.0f / spacing[1];
	ispacing[2] = 1.0f / spacing[2];

	// calculate texture size enclosing entire world bounds at the spacing
	if (r_refdef.scene.worldmodel)
	{
		int lightindex;
		int range;
		qbool bounds_set = false;
		dlight_t *light;
		rtlight_t *rtlight;

		// calculate bounds enclosing world lights as they should be noticably tighter 
		// than the world bounds on maps with unlit monster containers (see e1m7 etc)
		range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
		for (lightindex = 0;lightindex < range;lightindex++)
		{
			const vec_t *rtlmins, *rtlmaxs;

			light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;

			rtlight = &light->rtlight;
			rtlmins = rtlight->cullmins;
			rtlmaxs = rtlight->cullmaxs;

			if (!bounds_set)
			{
				VectorCopy(rtlmins, mins);
				VectorCopy(rtlmaxs, maxs);
				bounds_set = true;
			}
			else
			{
				mins[0] = min(mins[0], rtlmins[0]);
				mins[1] = min(mins[1], rtlmins[1]);
				mins[2] = min(mins[2], rtlmins[2]);
				maxs[0] = max(maxs[0], rtlmaxs[0]);
				maxs[1] = max(maxs[1], rtlmaxs[1]);
				maxs[2] = max(maxs[2], rtlmaxs[2]);
			}
		}

		// limit to no larger than the world bounds
		mins[0] = max(mins[0], r_refdef.scene.worldmodel->normalmins[0]);
		mins[1] = max(mins[1], r_refdef.scene.worldmodel->normalmins[1]);
		mins[2] = max(mins[2], r_refdef.scene.worldmodel->normalmins[2]);
		maxs[0] = min(maxs[0], r_refdef.scene.worldmodel->normalmaxs[0]);
		maxs[1] = min(maxs[1], r_refdef.scene.worldmodel->normalmaxs[1]);
		maxs[2] = min(maxs[2], r_refdef.scene.worldmodel->normalmaxs[2]);

		VectorMA(mins, -2.0f, spacing, mins);
		VectorMA(maxs, 2.0f, spacing, maxs);
	}
	else
	{
		VectorSet(mins, -1048576.0f, -1048576.0f, -1048576.0f);
		VectorSet(maxs,  1048576.0f,  1048576.0f,  1048576.0f);
	}
	VectorSubtract(maxs, mins, size);
	// now we can calculate the resolution we want
	c[0] = (int)floor(size[0] / spacing[0] + 0.5f);
	c[1] = (int)floor(size[1] / spacing[1] + 0.5f);
	c[2] = (int)floor(size[2] / spacing[2] + 0.5f);
	// figure out the exact texture size (honoring power of 2 if required)
	resolution[0] = bound(4, c[0], (int)vid.maxtexturesize_3d);
	resolution[1] = bound(4, c[1], (int)vid.maxtexturesize_3d);
	resolution[2] = bound(4, c[2], (int)vid.maxtexturesize_3d);
	size[0] = spacing[0] * resolution[0];
	size[1] = spacing[1] * resolution[1];
	size[2] = spacing[2] * resolution[2];

	// if dynamic we may or may not want to use the world bounds
	// if the dynamic size is smaller than the world bounds, use it instead
	if (!settings->staticmode && (r_shadow_bouncegrid_dynamic_x.integer * r_shadow_bouncegrid_dynamic_y.integer * r_shadow_bouncegrid_dynamic_z.integer < resolution[0] * resolution[1] * resolution[2]))
	{
		// we know the resolution we want
		c[0] = r_shadow_bouncegrid_dynamic_x.integer;
		c[1] = r_shadow_bouncegrid_dynamic_y.integer;
		c[2] = r_shadow_bouncegrid_dynamic_z.integer;
		// now we can calculate the texture size
		resolution[0] = bound(4, c[0], (int)vid.maxtexturesize_3d);
		resolution[1] = bound(4, c[1], (int)vid.maxtexturesize_3d);
		resolution[2] = bound(4, c[2], (int)vid.maxtexturesize_3d);
		size[0] = spacing[0] * resolution[0];
		size[1] = spacing[1] * resolution[1];
		size[2] = spacing[2] * resolution[2];
		// center the rendering on the view
		mins[0] = floor(r_refdef.view.origin[0] * ispacing[0] + 0.5f) * spacing[0] - 0.5f * size[0];
		mins[1] = floor(r_refdef.view.origin[1] * ispacing[1] + 0.5f) * spacing[1] - 0.5f * size[1];
		mins[2] = floor(r_refdef.view.origin[2] * ispacing[2] + 0.5f) * spacing[2] - 0.5f * size[2];
	}

	// recalculate the maxs in case the resolution was not satisfactory
	VectorAdd(mins, size, maxs);

	// check if this changed the texture size
	r_shadow_bouncegrid_state.createtexture = !(r_shadow_bouncegrid_state.texture && r_shadow_bouncegrid_state.resolution[0] == resolution[0] && r_shadow_bouncegrid_state.resolution[1] == resolution[1] && r_shadow_bouncegrid_state.resolution[2] == resolution[2] && r_shadow_bouncegrid_state.directional == r_shadow_bouncegrid_state.settings.directionalshading);
	r_shadow_bouncegrid_state.directional = r_shadow_bouncegrid_state.settings.directionalshading;
	VectorCopy(mins, r_shadow_bouncegrid_state.mins);
	VectorCopy(maxs, r_shadow_bouncegrid_state.maxs);
	VectorCopy(size, r_shadow_bouncegrid_state.size);
	VectorCopy(spacing, r_shadow_bouncegrid_state.spacing);
	VectorCopy(ispacing, r_shadow_bouncegrid_state.ispacing);
	VectorCopy(resolution, r_shadow_bouncegrid_state.resolution);

	// reallocate pixels for this update if needed...
	r_shadow_bouncegrid_state.pixelbands = settings->directionalshading ? 8 : 1;
	r_shadow_bouncegrid_state.pixelsperband = resolution[0]*resolution[1]*resolution[2];
	r_shadow_bouncegrid_state.bytesperband = r_shadow_bouncegrid_state.pixelsperband*4;
	numpixels = r_shadow_bouncegrid_state.pixelsperband*r_shadow_bouncegrid_state.pixelbands;
	if (r_shadow_bouncegrid_state.numpixels != numpixels)
	{
		if (r_shadow_bouncegrid_state.texture) { R_FreeTexture(r_shadow_bouncegrid_state.texture);r_shadow_bouncegrid_state.texture = NULL; }

		R_Shadow_BounceGrid_FreeHighPixels();

		r_shadow_bouncegrid_state.numpixels = numpixels;
	}

	// update the bouncegrid matrix to put it in the world properly
	memset(m, 0, sizeof(m));
	m[0] = 1.0f / r_shadow_bouncegrid_state.size[0];
	m[3] = -r_shadow_bouncegrid_state.mins[0] * m[0];
	m[5] = 1.0f / r_shadow_bouncegrid_state.size[1];
	m[7] = -r_shadow_bouncegrid_state.mins[1] * m[5];
	m[10] = 1.0f / r_shadow_bouncegrid_state.size[2];
	m[11] = -r_shadow_bouncegrid_state.mins[2] * m[10];
	m[15] = 1.0f;
	Matrix4x4_FromArrayFloatD3D(&r_shadow_bouncegrid_state.matrix, m);
}

static float R_Shadow_BounceGrid_RefractiveIndexAtPoint(vec3_t point)
{
	// check material at shadoworigin to see what the initial refractive index should be
	int hitsupercontentsmask = SUPERCONTENTS_SOLID | SUPERCONTENTS_LIQUIDSMASK;
	int skipsupercontentsmask = 0;
	int skipmaterialflagsmask = MATERIALFLAG_CUSTOMBLEND;
	trace_t trace = CL_TracePoint(point, r_shadow_bouncegrid_state.settings.staticmode ? MOVE_WORLDONLY : (r_shadow_bouncegrid_state.settings.hitmodels ? MOVE_HITMODEL : MOVE_NOMONSTERS), NULL, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, true, false, NULL, true);
	if (trace.starttexture && (trace.starttexture->currentmaterialflags & (MATERIALFLAG_REFRACTION | MATERIALFLAG_WATERSHADER)))
		return trace.starttexture->refractive_index;
	else if (trace.startsupercontents & SUPERCONTENTS_LIQUIDSMASK)
		return 1.333f; // water
	else
		return 1.0003f; // air
}

// enumerate world rtlights and sum the overall amount of light in the world,
// from that we can calculate a scaling factor to fairly distribute photons
// to all the lights
//
// this modifies rtlight->photoncolor and rtlight->photons
static void R_Shadow_BounceGrid_AssignPhotons_Task(taskqueue_task_t *t)
{
	// get the range of light numbers we'll be looping over:
	// range = static lights
	// range1 = dynamic lights (optional)
	// range2 = range + range1
	unsigned int range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	unsigned int range1 = r_shadow_bouncegrid_state.settings.staticmode ? 0 : r_refdef.scene.numlights;
	unsigned int range2 = range + range1;
	int flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;

	float normalphotonscaling;
	float photonscaling;
	float photonintensity;
	float photoncount = 0.0f;
	float lightintensity;
	float radius;
	float s;
	float w;
	vec3_t cullmins;
	vec3_t cullmaxs;
	unsigned int lightindex;
	dlight_t *light;
	rtlight_t *rtlight;
	int shootparticles;
	int shotparticles;
	float bounceminimumintensity2;
	float startrefractiveindex;
	unsigned int seed;
	randomseed_t randomseed;
	vec3_t baseshotcolor;

	normalphotonscaling = 1.0f / max(0.0000001f, r_shadow_bouncegrid_state.settings.energyperphoton);
	for (lightindex = 0;lightindex < range2;lightindex++)
	{
		if (lightindex < range)
		{
			light = (dlight_t *)Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;
			rtlight = &light->rtlight;
			VectorClear(rtlight->bouncegrid_photoncolor);
			rtlight->bouncegrid_photons = 0;
			rtlight->bouncegrid_hits = 0;
			rtlight->bouncegrid_traces = 0;
			rtlight->bouncegrid_effectiveradius = 0;
			if (!(light->flags & flag))
				continue;
			if (r_shadow_bouncegrid_state.settings.staticmode)
			{
				// when static, we skip styled lights because they tend to change...
				if (rtlight->style > 0 && r_shadow_bouncegrid.integer != 2)
					continue;
			}
			else if (r_shadow_debuglight.integer >= 0 && (int)lightindex != r_shadow_debuglight.integer)
				continue;
		}
		else
		{
			rtlight = r_refdef.scene.lights[lightindex - range];
			VectorClear(rtlight->bouncegrid_photoncolor);
			rtlight->bouncegrid_photons = 0;
			rtlight->bouncegrid_hits = 0;
			rtlight->bouncegrid_traces = 0;
			rtlight->bouncegrid_effectiveradius = 0;
		}
		// draw only visible lights (major speedup)
		radius = rtlight->radius * r_shadow_bouncegrid_state.settings.lightradiusscale;
		cullmins[0] = rtlight->shadoworigin[0] - radius;
		cullmins[1] = rtlight->shadoworigin[1] - radius;
		cullmins[2] = rtlight->shadoworigin[2] - radius;
		cullmaxs[0] = rtlight->shadoworigin[0] + radius;
		cullmaxs[1] = rtlight->shadoworigin[1] + radius;
		cullmaxs[2] = rtlight->shadoworigin[2] + radius;
		w = r_shadow_lightintensityscale.value * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale);
		if (!r_shadow_bouncegrid_state.settings.staticmode)
		{
			// skip if the expanded light box does not touch any visible leafs
			if (r_refdef.scene.worldmodel
				&& r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs
				&& !r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs(r_refdef.scene.worldmodel, r_refdef.viewcache.world_leafvisible, cullmins, cullmaxs))
				continue;
			// skip if the expanded light box is not visible to traceline
			// note that PrepareLight already did this check but for a smaller box, so we
			// end up casting more traces per frame per light when using bouncegrid, which
			// is probably fine (and they use the same timer)
			if (r_shadow_culllights_trace.integer)
			{
				if (rtlight->trace_timer != host.realtime && R_CanSeeBox(rtlight->trace_timer == 0 ? r_shadow_culllights_trace_tempsamples.integer : r_shadow_culllights_trace_samples.integer, r_shadow_culllights_trace_eyejitter.value, r_shadow_culllights_trace_enlarge.value, r_shadow_culllights_trace_expand.value, r_shadow_culllights_trace_pad.value, r_refdef.view.origin, rtlight->cullmins, rtlight->cullmaxs))
					rtlight->trace_timer = host.realtime;
				if (host.realtime - rtlight->trace_timer > r_shadow_culllights_trace_delay.value)
					continue;
			}
			// skip if expanded light box is offscreen
			if (R_CullFrustum(cullmins, cullmaxs))
				continue;
			// skip if overall light intensity is zero
			if (w * VectorLength2(rtlight->color) == 0.0f)
				continue;
		}
		// a light that does not emit any light before style is applied, can be
		// skipped entirely (it may just be a corona)
		if (rtlight->radius == 0.0f || VectorLength2(rtlight->color) == 0.0f)
			continue;
		w *= ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.rtlightstylevalue[rtlight->style] : 1);
		VectorScale(rtlight->color, w, rtlight->bouncegrid_photoncolor);
		// skip lights that will emit no photons
		if (!VectorLength2(rtlight->bouncegrid_photoncolor))
			continue;
		// shoot particles from this light
		// use a calculation for the number of particles that will not
		// vary with lightstyle, otherwise we get randomized particle
		// distribution, the seeded random is only consistent for a
		// consistent number of particles on this light...
		s = rtlight->radius;
		lightintensity = VectorLength(rtlight->color) * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale);
		if (lightindex >= range)
			lightintensity *= r_shadow_bouncegrid_state.settings.dlightparticlemultiplier;
		rtlight->bouncegrid_photons = lightintensity * s * s * normalphotonscaling;
		photoncount += rtlight->bouncegrid_photons;
		VectorScale(rtlight->bouncegrid_photoncolor, r_shadow_bouncegrid_state.settings.particleintensity * r_shadow_bouncegrid_state.settings.energyperphoton, rtlight->bouncegrid_photoncolor);
		// if the lightstyle happens to be off right now, we can skip actually
		// firing the photons, but we did have to count them in the total.
		//if (VectorLength2(rtlight->photoncolor) == 0.0f)
		//	rtlight->bouncegrid_photons = 0;
	}
	// the user provided an energyperphoton value which we try to use
	// if that results in too many photons to shoot this frame, then we cap it
	// which causes photons to appear/disappear from frame to frame, so we don't
	// like doing that in the typical case
	photonscaling = 1.0f;
	photonintensity = 1.0f;
	if (photoncount > r_shadow_bouncegrid_state.settings.maxphotons)
	{
		photonscaling = r_shadow_bouncegrid_state.settings.maxphotons / photoncount;
		photonintensity = 1.0f / photonscaling;
	}

	// modify the lights to reflect our computed scaling
	for (lightindex = 0; lightindex < range2; lightindex++)
	{
		if (lightindex < range)
		{
			light = (dlight_t *)Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;
			rtlight = &light->rtlight;
		}
		else
			rtlight = r_refdef.scene.lights[lightindex - range];
		rtlight->bouncegrid_photons *= photonscaling;
		VectorScale(rtlight->bouncegrid_photoncolor, photonintensity, rtlight->bouncegrid_photoncolor);
	}

	// compute a seed for the unstable random modes
	Math_RandomSeed_FromInts(&randomseed, 0, 0, 0, host.realtime * 1000.0);
	seed = host.realtime * 1000.0;

	for (lightindex = 0; lightindex < range2; lightindex++)
	{
		if (lightindex < range)
		{
			light = (dlight_t *)Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
			if (!light)
				continue;
			rtlight = &light->rtlight;
		}
		else
			rtlight = r_refdef.scene.lights[lightindex - range];
		// note that this code used to keep track of residual photons and
		// distribute them evenly to achieve exactly a desired photon count,
		// but that caused unwanted flickering in dynamic mode
		shootparticles = (int)floor(rtlight->bouncegrid_photons);
		// skip if we won't be shooting any photons
		if (!shootparticles)
			continue;
		radius = rtlight->radius * r_shadow_bouncegrid_state.settings.lightradiusscale;
		//s = settings.particleintensity / shootparticles;
		//VectorScale(rtlight->bouncegrid_photoncolor, s, baseshotcolor);
		VectorCopy(rtlight->bouncegrid_photoncolor, baseshotcolor);
		if (VectorLength2(baseshotcolor) <= 0.0f)
			continue;
		r_refdef.stats[r_stat_bouncegrid_lights]++;
		r_refdef.stats[r_stat_bouncegrid_particles] += shootparticles;
		// we stop caring about bounces once the brightness goes below this fraction of the original intensity
		bounceminimumintensity2 = VectorLength(baseshotcolor) * r_shadow_bouncegrid_state.settings.bounceminimumintensity2;

		// check material at shadoworigin to see what the initial refractive index should be
		startrefractiveindex = R_Shadow_BounceGrid_RefractiveIndexAtPoint(rtlight->shadoworigin);

		// for seeded random we start the RNG with the position of the light
		if (r_shadow_bouncegrid_state.settings.rng_seed >= 0)
		{
			union
			{
				unsigned int i[4];
				float f[4];
			}
			u;
			u.f[0] = rtlight->shadoworigin[0];
			u.f[1] = rtlight->shadoworigin[1];
			u.f[2] = rtlight->shadoworigin[2];
			u.f[3] = 1;
			switch (r_shadow_bouncegrid_state.settings.rng_type)
			{
			default:
			case 0:
				// we have to shift the seed provided by the user because the result must be odd
				Math_RandomSeed_FromInts(&randomseed, u.i[0], u.i[1], u.i[2], u.i[3] ^ (r_shadow_bouncegrid_state.settings.rng_seed << 1));
				break;
			case 1:
				seed = u.i[0] ^ u.i[1] ^ u.i[2] ^ u.i[3] ^ r_shadow_bouncegrid_state.settings.rng_seed;
				break;
			}
		}

		for (shotparticles = 0; shotparticles < shootparticles && r_shadow_bouncegrid_state.numphotons < r_shadow_bouncegrid_state.settings.maxphotons; shotparticles++)
		{
			r_shadow_bouncegrid_photon_t *p = r_shadow_bouncegrid_state.photons + r_shadow_bouncegrid_state.numphotons++;
			VectorCopy(baseshotcolor, p->color);
			VectorCopy(rtlight->shadoworigin, p->start);
			switch (r_shadow_bouncegrid_state.settings.rng_type)
			{
			default:
			case 0:
				// figure out a random direction for the initial photon to go
				VectorLehmerRandom(&randomseed, p->end);
				break;
			case 1:
				// figure out a random direction for the initial photon to go
				VectorCheeseRandom(seed, p->end);
				break;
			}

			// we want a uniform distribution spherically, not merely within the sphere
			if (r_shadow_bouncegrid_state.settings.normalizevectors)
				VectorNormalize(p->end);

			VectorMA(p->start, radius, p->end, p->end);
			p->bounceminimumintensity2 = bounceminimumintensity2;
			p->startrefractiveindex = startrefractiveindex;
			p->numpaths = 0;
		}
	}

	t->done = 1;
}

static void R_Shadow_BounceGrid_Slice(int zi)
{
	float *highpixels = r_shadow_bouncegrid_state.highpixels;
	int xi, yi; // pixel increments
	float color[32] = { 0 };
	float radius = r_shadow_bouncegrid_state.settings.lightpathsize;
	float iradius = 1.0f / radius;
	int slicemins[3], slicemaxs[3];
	int resolution[3];
	int pixelsperband = r_shadow_bouncegrid_state.pixelsperband;
	int pixelbands = r_shadow_bouncegrid_state.pixelbands;
	int photonindex;
	int samples = r_shadow_bouncegrid_state.settings.subsamples;
	float isamples = 1.0f / samples;
	float samplescolorscale = isamples * isamples * isamples;

	// we use these a lot, so get a local copy
	VectorCopy(r_shadow_bouncegrid_state.resolution, resolution);

	for (photonindex = 0; photonindex < r_shadow_bouncegrid_state.numphotons; photonindex++)
	{
		r_shadow_bouncegrid_photon_t *photon = r_shadow_bouncegrid_state.photons + photonindex;
		int pathindex;
		for (pathindex = 0; pathindex < photon->numpaths; pathindex++)
		{
			r_shadow_bouncegrid_photon_path_t *path = photon->paths + pathindex;
			float pathstart[3], pathend[3], pathmins[3], pathmaxs[3], pathdelta[3], pathdir[3], pathlength2, pathilength;

			VectorSubtract(path->start, r_shadow_bouncegrid_state.mins, pathstart);
			VectorSubtract(path->end, r_shadow_bouncegrid_state.mins, pathend);

			pathmins[2] = min(pathstart[2], pathend[2]);
			slicemins[2] = (int)floor((pathmins[2] - radius) * r_shadow_bouncegrid_state.ispacing[2]);
			pathmaxs[2] = max(pathstart[2], pathend[2]);
			slicemaxs[2] = (int)floor((pathmaxs[2] + radius) * r_shadow_bouncegrid_state.ispacing[2] + 1);

			// skip if the path doesn't touch this slice
			if (zi < slicemins[2] || zi >= slicemaxs[2])
				continue;

			pathmins[0] = min(pathstart[0], pathend[0]);
			slicemins[0] = (int)floor((pathmins[0] - radius) * r_shadow_bouncegrid_state.ispacing[0]);
			slicemins[0] = max(slicemins[0], 1);
			pathmaxs[0] = max(pathstart[0], pathend[0]);
			slicemaxs[0] = (int)floor((pathmaxs[0] + radius) * r_shadow_bouncegrid_state.ispacing[0]);
			slicemaxs[0] = min(slicemaxs[0], resolution[0] - 1);

			pathmins[1] = min(pathstart[1], pathend[1]);
			slicemins[1] = (int)floor((pathmins[1] - radius) * r_shadow_bouncegrid_state.ispacing[1] + 1);
			slicemins[1] = max(slicemins[1], 1);
			pathmaxs[1] = max(pathstart[1], pathend[1]);
			slicemaxs[1] = (int)floor((pathmaxs[1] + radius) * r_shadow_bouncegrid_state.ispacing[1] + 1);
			slicemaxs[1] = min(slicemaxs[1], resolution[1] - 1);

			// skip if the path is out of bounds on X or Y
			if (slicemins[0] >= slicemaxs[0] || slicemins[1] >= slicemaxs[1])
				continue;

			// calculate second order spherical harmonics values (average, slopeX, slopeY, slopeZ)
			// accumulate average shotcolor
			VectorSubtract(pathend, pathstart, pathdelta);
			pathlength2 = VectorLength2(pathdelta);
			pathilength = pathlength2 > 0.0f ? 1.0f / sqrt(pathlength2) : 0.0f;
			VectorScale(pathdelta, pathilength, pathdir);
			// the color is scaled by the number of subsamples
			color[0] = path->color[0] * samplescolorscale;
			color[1] = path->color[1] * samplescolorscale;
			color[2] = path->color[2] * samplescolorscale;
			color[3] = 0.0f;
			if (pixelbands > 1)
			{
				// store bentnormal in case the shader has a use for it,
				// bentnormal is an intensity-weighted average of the directions,
				// and will be normalized on conversion to texture pixels.
				float intensity = VectorLength(color);
				color[4] = pathdir[0] * intensity;
				color[5] = pathdir[1] * intensity;
				color[6] = pathdir[2] * intensity;
				color[7] = intensity;
				// for each color component (R, G, B) calculate the amount that a
				// direction contributes
				color[8] = color[0] * max(0.0f, pathdir[0]);
				color[9] = color[0] * max(0.0f, pathdir[1]);
				color[10] = color[0] * max(0.0f, pathdir[2]);
				color[11] = 0.0f;
				color[12] = color[1] * max(0.0f, pathdir[0]);
				color[13] = color[1] * max(0.0f, pathdir[1]);
				color[14] = color[1] * max(0.0f, pathdir[2]);
				color[15] = 0.0f;
				color[16] = color[2] * max(0.0f, pathdir[0]);
				color[17] = color[2] * max(0.0f, pathdir[1]);
				color[18] = color[2] * max(0.0f, pathdir[2]);
				color[19] = 0.0f;
				// and do the same for negative directions
				color[20] = color[0] * max(0.0f, -pathdir[0]);
				color[21] = color[0] * max(0.0f, -pathdir[1]);
				color[22] = color[0] * max(0.0f, -pathdir[2]);
				color[23] = 0.0f;
				color[24] = color[1] * max(0.0f, -pathdir[0]);
				color[25] = color[1] * max(0.0f, -pathdir[1]);
				color[26] = color[1] * max(0.0f, -pathdir[2]);
				color[27] = 0.0f;
				color[28] = color[2] * max(0.0f, -pathdir[0]);
				color[29] = color[2] * max(0.0f, -pathdir[1]);
				color[30] = color[2] * max(0.0f, -pathdir[2]);
				color[31] = 0.0f;
			}

			for (yi = slicemins[1]; yi < slicemaxs[1]; yi++)
			{
				for (xi = slicemins[0]; xi < slicemaxs[0]; xi++)
				{
					float sample[3], diff[3], nearest[3], along, distance2;
					float *p = highpixels + 4 * ((zi * resolution[1] + yi) * resolution[0] + xi);
					int xs, ys, zs;
					// loop over the subsamples
					for (zs = 0; zs < samples; zs++)
					{
						sample[2] = (zi + (zs + 0.5f) * isamples) * r_shadow_bouncegrid_state.spacing[2];
						for (ys = 0; ys < samples; ys++)
						{
							sample[1] = (yi + (ys + 0.5f) * isamples) * r_shadow_bouncegrid_state.spacing[1];
							for (xs = 0; xs < samples; xs++)
							{
								sample[0] = (xi + (xs + 0.5f) * isamples) * r_shadow_bouncegrid_state.spacing[0];

								// measure distance from subsample to line segment and see if it is within radius
								along = DotProduct(sample, pathdir) * pathilength;
								if (along <= 0)
									VectorCopy(pathstart, nearest);
								else if (along >= 1)
									VectorCopy(pathend, nearest);
								else
									VectorLerp(pathstart, along, pathend, nearest);
								VectorSubtract(sample, nearest, diff);
								VectorScale(diff, iradius, diff);
								distance2 = VectorLength2(diff);
								if (distance2 < 1.0f)
								{
									// contribute some color to this pixel, across all bands
									float w = 1.0f - sqrt(distance2);
									int band;
									w *= w;
									if (pixelbands > 1)
									{
										// small optimization for alpha - only color[7] is non-zero, so skip the rest of the alpha elements.
										p[pixelsperband * 4 + 3] += color[7] * w;
									}
									for (band = 0; band < pixelbands; band++)
									{
										// add to the pixel color (RGB only - see above)
										p[band * pixelsperband * 4 + 0] += color[band * 4 + 0] * w;
										p[band * pixelsperband * 4 + 1] += color[band * 4 + 1] * w;
										p[band * pixelsperband * 4 + 2] += color[band * 4 + 2] * w;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

static void R_Shadow_BounceGrid_Slice_Task(taskqueue_task_t *t)
{
	R_Shadow_BounceGrid_Slice((int)t->i[0]);
	t->done = 1;
}

static void R_Shadow_BounceGrid_EnqueueSlices_Task(taskqueue_task_t *t)
{
	int i, slices;
	// we need to wait for the texture clear to finish before we start adding light to it
	if (r_shadow_bouncegrid_state.cleartex_task.done == 0)
	{
		TaskQueue_Yield(t);
		return;
	}
	slices = r_shadow_bouncegrid_state.resolution[2] - 2;
	for (i = 0; i < slices; i++)
		TaskQueue_Setup(r_shadow_bouncegrid_state.slices_tasks + i, NULL, R_Shadow_BounceGrid_Slice_Task, i + 1, 0, NULL, NULL);
	TaskQueue_Enqueue(slices, r_shadow_bouncegrid_state.slices_tasks);
	TaskQueue_Setup(&r_shadow_bouncegrid_state.slices_done_task, NULL, TaskQueue_Task_CheckTasksDone, slices, 0, r_shadow_bouncegrid_state.slices_tasks, 0);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.slices_done_task);
	t->done = 1;
}

static void R_Shadow_BounceGrid_BlurPixelsInDirection(const float *inpixels, float *outpixels, int off)
{
	const float *inpixel;
	float *outpixel;
	int pixelbands = r_shadow_bouncegrid_state.pixelbands;
	int pixelband;
	unsigned int index;
	unsigned int x, y, z;
	unsigned int resolution[3];
	VectorCopy(r_shadow_bouncegrid_state.resolution, resolution);
	for (pixelband = 0;pixelband < pixelbands;pixelband++)
	{
		for (z = 1;z < resolution[2]-1;z++)
		{
			for (y = 1;y < resolution[1]-1;y++)
			{
				x = 1;
				index = ((pixelband*resolution[2]+z)*resolution[1]+y)*resolution[0]+x;
				inpixel = inpixels + 4*index;
				outpixel = outpixels + 4*index;
				for (;x < resolution[0]-1;x++, inpixel += 4, outpixel += 4)
				{
					outpixel[0] = (inpixel[0] + inpixel[  off] + inpixel[0-off]) * (1.0f / 3.0);
					outpixel[1] = (inpixel[1] + inpixel[1+off] + inpixel[1-off]) * (1.0f / 3.0);
					outpixel[2] = (inpixel[2] + inpixel[2+off] + inpixel[2-off]) * (1.0f / 3.0);
					outpixel[3] = (inpixel[3] + inpixel[3+off] + inpixel[3-off]) * (1.0f / 3.0);
				}
			}
		}
	}
}

static void R_Shadow_BounceGrid_BlurPixels_Task(taskqueue_task_t *t)
{
	float *pixels[4];
	unsigned int resolution[3];
	if (r_shadow_bouncegrid_state.settings.blur)
	{
		VectorCopy(r_shadow_bouncegrid_state.resolution, resolution);

		pixels[0] = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index];
		pixels[1] = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index ^ 1];
		pixels[2] = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index];
		pixels[3] = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index ^ 1];

		// blur on X
		R_Shadow_BounceGrid_BlurPixelsInDirection(pixels[0], pixels[1], 4);
		// blur on Y
		R_Shadow_BounceGrid_BlurPixelsInDirection(pixels[1], pixels[2], resolution[0] * 4);
		// blur on Z
		R_Shadow_BounceGrid_BlurPixelsInDirection(pixels[2], pixels[3], resolution[0] * resolution[1] * 4);

		// toggle the state, highpixels now points to pixels[3] result
		r_shadow_bouncegrid_state.highpixels_index ^= 1;
		r_shadow_bouncegrid_state.highpixels = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index];
	}
	t->done = 1;
}

static void R_Shadow_BounceGrid_ConvertPixelsAndUpload(void)
{
	int floatcolors = r_shadow_bouncegrid_state.settings.floatcolors;
	unsigned char *pixelsbgra8 = NULL;
	unsigned char *pixelbgra8;
	unsigned short *pixelsrgba16f = NULL;
	unsigned short *pixelrgba16f;
	float *pixelsrgba32f = NULL;
	float *highpixels = r_shadow_bouncegrid_state.highpixels;
	float *highpixel;
	float *bandpixel;
	unsigned int pixelsperband = r_shadow_bouncegrid_state.pixelsperband;
	unsigned int pixelbands = r_shadow_bouncegrid_state.pixelbands;
	unsigned int pixelband;
	unsigned int x, y, z;
	unsigned int index, bandindex;
	unsigned int resolution[3];
	int c[4];
	VectorCopy(r_shadow_bouncegrid_state.resolution, resolution);

	if (r_shadow_bouncegrid_state.createtexture && r_shadow_bouncegrid_state.texture)
	{
		R_FreeTexture(r_shadow_bouncegrid_state.texture);
		r_shadow_bouncegrid_state.texture = NULL;
	}

	// if bentnormals exist, we need to normalize and bias them for the shader
	if (pixelbands > 1)
	{
		pixelband = 1;
		for (z = 0;z < resolution[2]-1;z++)
		{
			for (y = 0;y < resolution[1]-1;y++)
			{
				x = 1;
				index = ((pixelband*resolution[2]+z)*resolution[1]+y)*resolution[0]+x;
				highpixel = highpixels + 4*index;
				for (;x < resolution[0]-1;x++, index++, highpixel += 4)
				{
					// only convert pixels that were hit by photons
					if (highpixel[3] != 0.0f)
						VectorNormalize(highpixel);
					VectorSet(highpixel, highpixel[0] * 0.5f + 0.5f, highpixel[1] * 0.5f + 0.5f, highpixel[2] * 0.5f + 0.5f);
					highpixel[pixelsperband * 4 + 3] = 1.0f;
				}
			}
		}
	}

	// start by clearing the pixels array - we won't be writing to all of it
	//
	// then process only the pixels that have at least some color, skipping
	// the higher bands for speed on pixels that are black
	switch (floatcolors)
	{
	case 0:
		if (r_shadow_bouncegrid_state.u8pixels == NULL)
			r_shadow_bouncegrid_state.u8pixels = (unsigned char *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.numpixels * sizeof(unsigned char[4]));
		pixelsbgra8 = r_shadow_bouncegrid_state.u8pixels;
		for (pixelband = 0;pixelband < pixelbands;pixelband++)
		{
			if (pixelband == 1)
				memset(pixelsbgra8 + pixelband * r_shadow_bouncegrid_state.bytesperband, 128, r_shadow_bouncegrid_state.bytesperband);
			else
				memset(pixelsbgra8 + pixelband * r_shadow_bouncegrid_state.bytesperband, 0, r_shadow_bouncegrid_state.bytesperband);
		}
		for (z = 1;z < resolution[2]-1;z++)
		{
			for (y = 1;y < resolution[1]-1;y++)
			{
				x = 1;
				pixelband = 0;
				index = ((pixelband*resolution[2]+z)*resolution[1]+y)*resolution[0]+x;
				highpixel = highpixels + 4*index;
				for (;x < resolution[0]-1;x++, index++, highpixel += 4)
				{
					// only convert pixels that were hit by photons
					if (VectorLength2(highpixel))
					{
						// normalize the bentnormal now
						if (pixelbands > 1)
						{
							VectorNormalize(highpixel + pixelsperband * 4);
							highpixel[pixelsperband * 4 + 3] = 1.0f;
						}
						// process all of the pixelbands for this pixel
						for (pixelband = 0, bandindex = index;pixelband < pixelbands;pixelband++, bandindex += pixelsperband)
						{
							pixelbgra8 = pixelsbgra8 + 4*bandindex;
							bandpixel = highpixels + 4*bandindex;
							c[0] = (int)(bandpixel[0]*256.0f);
							c[1] = (int)(bandpixel[1]*256.0f);
							c[2] = (int)(bandpixel[2]*256.0f);
							c[3] = (int)(bandpixel[3]*256.0f);
							pixelbgra8[2] = (unsigned char)bound(0, c[0], 255);
							pixelbgra8[1] = (unsigned char)bound(0, c[1], 255);
							pixelbgra8[0] = (unsigned char)bound(0, c[2], 255);
							pixelbgra8[3] = (unsigned char)bound(0, c[3], 255);
						}
					}
				}
			}
		}

		if (!r_shadow_bouncegrid_state.createtexture)
			R_UpdateTexture(r_shadow_bouncegrid_state.texture, pixelsbgra8, 0, 0, 0, resolution[0], resolution[1], resolution[2]*pixelbands, 0);
		else
			r_shadow_bouncegrid_state.texture = R_LoadTexture3D(r_shadow_texturepool, "bouncegrid", resolution[0], resolution[1], resolution[2]*pixelbands, pixelsbgra8, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, 0, NULL);
		break;
	case 1:
		if (r_shadow_bouncegrid_state.fp16pixels == NULL)
			r_shadow_bouncegrid_state.fp16pixels = (unsigned short *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.numpixels * sizeof(unsigned short[4]));
		pixelsrgba16f = r_shadow_bouncegrid_state.fp16pixels;
		memset(pixelsrgba16f, 0, r_shadow_bouncegrid_state.numpixels * sizeof(unsigned short[4]));
		for (z = 1;z < resolution[2]-1;z++)
		{
			for (y = 1;y < resolution[1]-1;y++)
			{
				x = 1;
				pixelband = 0;
				index = ((pixelband*resolution[2]+z)*resolution[1]+y)*resolution[0]+x;
				highpixel = highpixels + 4*index;
				for (;x < resolution[0]-1;x++, index++, highpixel += 4)
				{
					// only convert pixels that were hit by photons
					if (VectorLength2(highpixel))
					{
						// process all of the pixelbands for this pixel
						for (pixelband = 0, bandindex = index;pixelband < pixelbands;pixelband++, bandindex += pixelsperband)
						{
							// time to have fun with IEEE 754 bit hacking...
							union {
								float f[4];
								unsigned int raw[4];
							} u;
							pixelrgba16f = pixelsrgba16f + 4*bandindex;
							bandpixel = highpixels + 4*bandindex;
							VectorCopy4(bandpixel, u.f);
							VectorCopy4(u.raw, c);
							// this math supports negative numbers, snaps denormals to zero
							//pixelrgba16f[0] = (unsigned short)(((c[0] & 0x7FFFFFFF) < 0x38000000) ? 0 : (((c[0] - 0x38000000) >> 13) & 0x7FFF) | ((c[0] >> 16) & 0x8000));
							//pixelrgba16f[1] = (unsigned short)(((c[1] & 0x7FFFFFFF) < 0x38000000) ? 0 : (((c[1] - 0x38000000) >> 13) & 0x7FFF) | ((c[1] >> 16) & 0x8000));
							//pixelrgba16f[2] = (unsigned short)(((c[2] & 0x7FFFFFFF) < 0x38000000) ? 0 : (((c[2] - 0x38000000) >> 13) & 0x7FFF) | ((c[2] >> 16) & 0x8000));
							//pixelrgba16f[3] = (unsigned short)(((c[3] & 0x7FFFFFFF) < 0x38000000) ? 0 : (((c[3] - 0x38000000) >> 13) & 0x7FFF) | ((c[3] >> 16) & 0x8000));
							// this math does not support negative
							pixelrgba16f[0] = (unsigned short)((c[0] < 0x38000000) ? 0 : ((c[0] - 0x38000000) >> 13));
							pixelrgba16f[1] = (unsigned short)((c[1] < 0x38000000) ? 0 : ((c[1] - 0x38000000) >> 13));
							pixelrgba16f[2] = (unsigned short)((c[2] < 0x38000000) ? 0 : ((c[2] - 0x38000000) >> 13));
							pixelrgba16f[3] = (unsigned short)((c[3] < 0x38000000) ? 0 : ((c[3] - 0x38000000) >> 13));
						}
					}
				}
			}
		}

		if (!r_shadow_bouncegrid_state.createtexture)
			R_UpdateTexture(r_shadow_bouncegrid_state.texture, (const unsigned char *)pixelsrgba16f, 0, 0, 0, resolution[0], resolution[1], resolution[2]*pixelbands, 0);
		else
			r_shadow_bouncegrid_state.texture = R_LoadTexture3D(r_shadow_texturepool, "bouncegrid", resolution[0], resolution[1], resolution[2]*pixelbands, (const unsigned char *)pixelsrgba16f, TEXTYPE_COLORBUFFER16F, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, 0, NULL);
		break;
	case 2:
		// our native format happens to match, so this is easy.
		pixelsrgba32f = highpixels;

		if (!r_shadow_bouncegrid_state.createtexture)
			R_UpdateTexture(r_shadow_bouncegrid_state.texture, (const unsigned char *)pixelsrgba32f, 0, 0, 0, resolution[0], resolution[1], resolution[2]*pixelbands, 0);
		else
			r_shadow_bouncegrid_state.texture = R_LoadTexture3D(r_shadow_texturepool, "bouncegrid", resolution[0], resolution[1], resolution[2]*pixelbands, (const unsigned char *)pixelsrgba32f, TEXTYPE_COLORBUFFER32F, TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCELINEAR, 0, NULL);
		break;
	}

	r_shadow_bouncegrid_state.lastupdatetime = host.realtime;
}

static void R_Shadow_BounceGrid_ClearTex_Task(taskqueue_task_t *t)
{
	memset(r_shadow_bouncegrid_state.highpixels, 0, r_shadow_bouncegrid_state.numpixels * sizeof(float[4]));
	t->done = 1;
}

static void R_Shadow_BounceGrid_TracePhotons_Shot(r_shadow_bouncegrid_photon_t *p, int remainingbounces, vec3_t shotstart, vec3_t shotend, vec3_t shotcolor, float bounceminimumintensity2, float previousrefractiveindex)
{
	int hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask;
	vec3_t shothit;
	vec3_t surfacenormal;
	vec3_t reflectstart, reflectend, reflectcolor;
	vec3_t refractstart, refractend, refractcolor;
	vec_t s;
	float reflectamount = 1.0f;
	trace_t cliptrace;
	// figure out what we want to interact with
	hitsupercontentsmask = SUPERCONTENTS_SOLID | SUPERCONTENTS_LIQUIDSMASK;
	skipsupercontentsmask = 0;
	skipmaterialflagsmask = MATERIALFLAG_CUSTOMBLEND;
	//r_refdef.scene.worldmodel->TraceLineAgainstSurfaces(r_refdef.scene.worldmodel, NULL, NULL, &cliptrace, clipstart, clipend, hitsupercontentsmask);
	//r_refdef.scene.worldmodel->TraceLine(r_refdef.scene.worldmodel, NULL, NULL, &cliptrace2, clipstart, clipend, hitsupercontentsmask);
	if (r_shadow_bouncegrid_state.settings.staticmode || r_shadow_bouncegrid_state.settings.rng_seed < 0 || r_shadow_bouncegrid_threaded.integer)
	{
		// static mode fires a LOT of rays but none of them are identical, so they are not cached
		// non-stable random in dynamic mode also never reuses a direction, so there's no reason to cache it
		cliptrace = CL_TraceLine(shotstart, shotend, r_shadow_bouncegrid_state.settings.staticmode ? MOVE_WORLDONLY : (r_shadow_bouncegrid_state.settings.hitmodels ? MOVE_HITMODEL : MOVE_NOMONSTERS), NULL, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value, true, false, NULL, true, true);
	}
	else
	{
		// dynamic mode fires many rays and most will match the cache from the previous frame
		cliptrace = CL_Cache_TraceLineSurfaces(shotstart, shotend, r_shadow_bouncegrid_state.settings.staticmode ? MOVE_WORLDONLY : (r_shadow_bouncegrid_state.settings.hitmodels ? MOVE_HITMODEL : MOVE_NOMONSTERS), hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
	}
	VectorCopy(cliptrace.endpos, shothit);
	if ((remainingbounces == r_shadow_bouncegrid_state.settings.maxbounce || r_shadow_bouncegrid_state.settings.includedirectlighting) && p->numpaths < PHOTON_MAX_PATHS)
	{
		qbool notculled = true;
		// cull paths that fail R_CullFrustum in dynamic mode
		if (!r_shadow_bouncegrid_state.settings.staticmode
			&& r_shadow_bouncegrid_dynamic_culllightpaths.integer)
		{
			vec3_t cullmins, cullmaxs;
			cullmins[0] = min(shotstart[0], shothit[0]) - r_shadow_bouncegrid_state.settings.spacing[0] - r_shadow_bouncegrid_state.settings.lightpathsize;
			cullmins[1] = min(shotstart[1], shothit[1]) - r_shadow_bouncegrid_state.settings.spacing[1] - r_shadow_bouncegrid_state.settings.lightpathsize;
			cullmins[2] = min(shotstart[2], shothit[2]) - r_shadow_bouncegrid_state.settings.spacing[2] - r_shadow_bouncegrid_state.settings.lightpathsize;
			cullmaxs[0] = max(shotstart[0], shothit[0]) + r_shadow_bouncegrid_state.settings.spacing[0] + r_shadow_bouncegrid_state.settings.lightpathsize;
			cullmaxs[1] = max(shotstart[1], shothit[1]) + r_shadow_bouncegrid_state.settings.spacing[1] + r_shadow_bouncegrid_state.settings.lightpathsize;
			cullmaxs[2] = max(shotstart[2], shothit[2]) + r_shadow_bouncegrid_state.settings.spacing[2] + r_shadow_bouncegrid_state.settings.lightpathsize;
			if (R_CullFrustum(cullmins, cullmaxs))
				notculled = false;
		}
		if (notculled)
		{
			r_shadow_bouncegrid_photon_path_t *path = p->paths + p->numpaths++;
			VectorCopy(shotstart, path->start);
			VectorCopy(shothit, path->end);
			VectorCopy(shotcolor, path->color);
		}
	}
	if (cliptrace.fraction < 1.0f && remainingbounces > 0)
	{
		// scale down shot color by bounce intensity and texture color (or 50% if no texture reported)
		// also clamp the resulting color to never add energy, even if the user requests extreme values
		VectorCopy(cliptrace.plane.normal, surfacenormal);
		VectorSet(reflectcolor, 0.5f, 0.5f, 0.5f);
		VectorClear(refractcolor);
		// FIXME: we need to determine the exact triangle, vertex color and texcoords and texture color and texture normal for the impacted point
		if (cliptrace.hittexture)
		{
			if (cliptrace.hittexture->currentskinframe)
				VectorCopy(cliptrace.hittexture->currentskinframe->avgcolor, reflectcolor);
			if (cliptrace.hittexture->currentalpha < 1.0f && (cliptrace.hittexture->currentmaterialflags & (MATERIALFLAG_ALPHA | MATERIALFLAG_ALPHATEST)))
			{
				reflectamount *= cliptrace.hittexture->currentalpha;
				if (cliptrace.hittexture->currentskinframe)
					reflectamount *= cliptrace.hittexture->currentskinframe->avgcolor[3];
			}
			if (cliptrace.hittexture->currentmaterialflags & MATERIALFLAG_WATERSHADER)
			{
				float Fresnel;
				vec3_t lightdir;
				//reflectchance = pow(min(1.0f, 1.0f - cliptrace.
				VectorSubtract(shotstart, shotend, lightdir);
				VectorNormalize(lightdir);
				Fresnel = min(1.0f, 1.0f - DotProduct(lightdir, surfacenormal));
				Fresnel = Fresnel * Fresnel * (cliptrace.hittexture->reflectmax - cliptrace.hittexture->reflectmin) + cliptrace.hittexture->reflectmin;
				reflectamount *= Fresnel;
				VectorCopy(cliptrace.hittexture->refractcolor4f, refractcolor);
			}
			if (cliptrace.hittexture->currentmaterialflags & MATERIALFLAG_REFRACTION)
				VectorCopy(cliptrace.hittexture->refractcolor4f, refractcolor);
			// make sure we do not gain energy even if surface colors are out of bounds
			reflectcolor[0] = min(reflectcolor[0], 1.0f);
			reflectcolor[1] = min(reflectcolor[1], 1.0f);
			reflectcolor[2] = min(reflectcolor[2], 1.0f);
			refractcolor[0] = min(refractcolor[0], 1.0f);
			refractcolor[1] = min(refractcolor[1], 1.0f);
			refractcolor[2] = min(refractcolor[2], 1.0f);
		}
		// reflected and refracted shots
		VectorScale(reflectcolor, r_shadow_bouncegrid_state.settings.particlebounceintensity * reflectamount, reflectcolor);
		VectorScale(refractcolor, (1.0f - reflectamount), refractcolor);
		VectorMultiply(reflectcolor, shotcolor, reflectcolor);
		VectorMultiply(refractcolor, shotcolor, refractcolor);

		if (VectorLength2(reflectcolor) >= bounceminimumintensity2)
		{
			// reflect the remaining portion of the line across plane normal
			VectorSubtract(shotend, shothit, reflectend);
			VectorReflect(reflectend, 1.0, surfacenormal, reflectend);
			// calculate the new line start and end
			VectorCopy(shothit, reflectstart);
			VectorAdd(reflectstart, reflectend, reflectend);
			R_Shadow_BounceGrid_TracePhotons_Shot(p, remainingbounces - 1, reflectstart, reflectend, reflectcolor, bounceminimumintensity2, previousrefractiveindex);
		}

		if (VectorLength2(refractcolor) >= bounceminimumintensity2)
		{
			// Check what refractive index is on the other side
			float refractiveindex;
			VectorMA(shothit, 0.0625f, cliptrace.plane.normal, refractstart);
			refractiveindex = R_Shadow_BounceGrid_RefractiveIndexAtPoint(refractstart);
			// reflect the remaining portion of the line across plane normal
			VectorSubtract(shotend, shothit, refractend);
			s = refractiveindex / previousrefractiveindex;
			VectorReflect(refractend, -1.0f / s, surfacenormal, refractend);
			// we also need to reflect the start to the other side of the plane so it doesn't just hit the same surface again
			// calculate the new line start and end
			VectorMA(shothit, 0.0625f, cliptrace.plane.normal, refractstart);
			VectorAdd(refractstart, refractend, refractend);
			R_Shadow_BounceGrid_TracePhotons_Shot(p, remainingbounces - 1, refractstart, refractend, refractcolor, bounceminimumintensity2, refractiveindex);
		}
	}
}

static void R_Shadow_BounceGrid_TracePhotons_ShotTask(taskqueue_task_t *t)
{
	r_shadow_bouncegrid_photon_t *p = (r_shadow_bouncegrid_photon_t *)t->p[0];
	R_Shadow_BounceGrid_TracePhotons_Shot(p, r_shadow_bouncegrid_state.settings.maxbounce, p->start, p->end, p->color, p->bounceminimumintensity2, p->startrefractiveindex);
	t->done = 1;
}

static void R_Shadow_BounceGrid_EnqueuePhotons_Task(taskqueue_task_t *t)
{
	int i;
	for (i = 0; i < r_shadow_bouncegrid_state.numphotons; i++)
		TaskQueue_Setup(r_shadow_bouncegrid_state.photons_tasks + i, NULL, R_Shadow_BounceGrid_TracePhotons_ShotTask, 0, 0, r_shadow_bouncegrid_state.photons + i, NULL);
	TaskQueue_Setup(&r_shadow_bouncegrid_state.photons_done_task, NULL, TaskQueue_Task_CheckTasksDone, r_shadow_bouncegrid_state.numphotons, 0, r_shadow_bouncegrid_state.photons_tasks, NULL);
	if (r_shadow_bouncegrid_threaded.integer)
	{
		TaskQueue_Enqueue(r_shadow_bouncegrid_state.numphotons, r_shadow_bouncegrid_state.photons_tasks);
		TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.photons_done_task);
	}
	else
	{
		// when not threaded we still have to report task status
		for (i = 0; i < r_shadow_bouncegrid_state.numphotons; i++)
			r_shadow_bouncegrid_state.photons_tasks[i].func(r_shadow_bouncegrid_state.photons_tasks + i);
		r_shadow_bouncegrid_state.photons_done_task.done = 1;
	}
	t->done = 1;
}

void R_Shadow_UpdateBounceGridTexture(void)
{
	int flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	r_shadow_bouncegrid_settings_t settings;
	qbool enable = false;
	qbool settingschanged;

	enable = R_Shadow_BounceGrid_CheckEnable(flag);
	
	R_Shadow_BounceGrid_GenerateSettings(&settings);
	
	// changing intensity does not require an update
	r_shadow_bouncegrid_state.intensity = r_shadow_bouncegrid_intensity.value;

	settingschanged = memcmp(&r_shadow_bouncegrid_state.settings, &settings, sizeof(settings)) != 0;

	// when settings change, we free everything as it is just simpler that way.
	if (settingschanged || !enable)
	{
		// not enabled, make sure we free anything we don't need anymore.
		if (r_shadow_bouncegrid_state.texture)
		{
			R_FreeTexture(r_shadow_bouncegrid_state.texture);
			r_shadow_bouncegrid_state.texture = NULL;
		}
		R_Shadow_BounceGrid_FreeHighPixels();
		r_shadow_bouncegrid_state.numpixels = 0;
		r_shadow_bouncegrid_state.numphotons = 0;
		r_shadow_bouncegrid_state.directional = false;

		if (!enable)
			return;
	}

	// if all the settings seem identical to the previous update, return
	if (r_shadow_bouncegrid_state.texture && (settings.staticmode || host.realtime < r_shadow_bouncegrid_state.lastupdatetime + r_shadow_bouncegrid_dynamic_updateinterval.value) && !settingschanged)
		return;

	// store the new settings
	r_shadow_bouncegrid_state.settings = settings;

	R_Shadow_BounceGrid_UpdateSpacing();

	// allocate the highpixels array we'll be accumulating light into
	if (r_shadow_bouncegrid_state.blurpixels[0] == NULL)
		r_shadow_bouncegrid_state.blurpixels[0] = (float *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.numpixels * sizeof(float[4]));
	if (r_shadow_bouncegrid_state.settings.blur && r_shadow_bouncegrid_state.blurpixels[1] == NULL)
		r_shadow_bouncegrid_state.blurpixels[1] = (float *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.numpixels * sizeof(float[4]));
	r_shadow_bouncegrid_state.highpixels_index = 0;
	r_shadow_bouncegrid_state.highpixels = r_shadow_bouncegrid_state.blurpixels[r_shadow_bouncegrid_state.highpixels_index];

	// set up the tracking of photon data
	if (r_shadow_bouncegrid_state.photons == NULL)
		r_shadow_bouncegrid_state.photons = (r_shadow_bouncegrid_photon_t *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.settings.maxphotons * sizeof(r_shadow_bouncegrid_photon_t));
	if (r_shadow_bouncegrid_state.photons_tasks == NULL)
		r_shadow_bouncegrid_state.photons_tasks = (taskqueue_task_t *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.settings.maxphotons * sizeof(taskqueue_task_t));
	r_shadow_bouncegrid_state.numphotons = 0;

	// set up the tracking of slice tasks
	if (r_shadow_bouncegrid_state.slices_tasks == NULL)
		r_shadow_bouncegrid_state.slices_tasks = (taskqueue_task_t *)Mem_Alloc(r_main_mempool, r_shadow_bouncegrid_state.resolution[2] * sizeof(taskqueue_task_t));

	memset(&r_shadow_bouncegrid_state.cleartex_task, 0, sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.assignphotons_task, 0, sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.enqueuephotons_task, 0, sizeof(taskqueue_task_t));
	memset(r_shadow_bouncegrid_state.photons_tasks, 0, r_shadow_bouncegrid_state.settings.maxphotons * sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.photons_done_task, 0, sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.enqueue_slices_task, 0, sizeof(taskqueue_task_t));
	memset(r_shadow_bouncegrid_state.slices_tasks, 0, r_shadow_bouncegrid_state.resolution[2] * sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.slices_done_task, 0, sizeof(taskqueue_task_t));
	memset(&r_shadow_bouncegrid_state.blurpixels_task, 0, sizeof(taskqueue_task_t));

	// clear the texture
	TaskQueue_Setup(&r_shadow_bouncegrid_state.cleartex_task, NULL, R_Shadow_BounceGrid_ClearTex_Task, 0, 0, NULL, NULL);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.cleartex_task);

	// calculate weighting factors for distributing photons among the lights
	TaskQueue_Setup(&r_shadow_bouncegrid_state.assignphotons_task, NULL, R_Shadow_BounceGrid_AssignPhotons_Task, 0, 0, NULL, NULL);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.assignphotons_task);

	// enqueue tasks to trace the photons from lights
	TaskQueue_Setup(&r_shadow_bouncegrid_state.enqueuephotons_task, &r_shadow_bouncegrid_state.assignphotons_task, R_Shadow_BounceGrid_EnqueuePhotons_Task, 0, 0, NULL, NULL);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.enqueuephotons_task);

	// accumulate the light paths into texture
	TaskQueue_Setup(&r_shadow_bouncegrid_state.enqueue_slices_task, &r_shadow_bouncegrid_state.photons_done_task, R_Shadow_BounceGrid_EnqueueSlices_Task, 0, 0, NULL, NULL);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.enqueue_slices_task);

	// apply a mild blur filter to the texture
	TaskQueue_Setup(&r_shadow_bouncegrid_state.blurpixels_task, &r_shadow_bouncegrid_state.slices_done_task, R_Shadow_BounceGrid_BlurPixels_Task, 0, 0, NULL, NULL);
	TaskQueue_Enqueue(1, &r_shadow_bouncegrid_state.blurpixels_task);

	TaskQueue_WaitForTaskDone(&r_shadow_bouncegrid_state.blurpixels_task);
	R_TimeReport("bouncegrid_gen");

	// convert the pixels to lower precision and upload the texture
	// this unfortunately has to run on the main thread for OpenGL calls, so we have to block on the previous task...
	R_Shadow_BounceGrid_ConvertPixelsAndUpload();
	R_TimeReport("bouncegrid_tex");

	// after we compute the static lighting we don't need to keep the highpixels array around
	if (settings.staticmode)
		R_Shadow_BounceGrid_FreeHighPixels();
}

void R_Shadow_RenderMode_VisibleLighting(qbool transparent)
{
	R_Shadow_RenderMode_Reset();
	GL_BlendFunc(GL_ONE, GL_ONE);
	GL_DepthRange(0, 1);
	GL_DepthTest(r_showlighting.integer < 2);
	GL_Color(0.1 * r_refdef.view.colorscale, 0.0125 * r_refdef.view.colorscale, 0, 1);
	if (!transparent)
		GL_DepthFunc(GL_EQUAL);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_VISIBLELIGHTING;
}

void R_Shadow_RenderMode_End(void)
{
	R_Shadow_RenderMode_Reset();
	R_Shadow_RenderMode_ActiveLight(NULL);
	GL_DepthMask(true);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	r_shadow_rendermode = R_SHADOW_RENDERMODE_NONE;
}

int bboxedges[12][2] =
{
	// top
	{0, 1}, // +X
	{0, 2}, // +Y
	{1, 3}, // Y, +X
	{2, 3}, // X, +Y
	// bottom
	{4, 5}, // +X
	{4, 6}, // +Y
	{5, 7}, // Y, +X
	{6, 7}, // X, +Y
	// verticals
	{0, 4}, // +Z
	{1, 5}, // X, +Z
	{2, 6}, // Y, +Z
	{3, 7}, // XY, +Z
};

qbool R_Shadow_ScissorForBBox(const float *mins, const float *maxs)
{
	if (!r_shadow_scissor.integer || r_shadow_usingdeferredprepass || r_trippy.integer)
	{
		r_shadow_lightscissor[0] = r_refdef.view.viewport.x;
		r_shadow_lightscissor[1] = r_refdef.view.viewport.y;
		r_shadow_lightscissor[2] = r_refdef.view.viewport.width;
		r_shadow_lightscissor[3] = r_refdef.view.viewport.height;
		return false;
	}
	if(R_ScissorForBBox(mins, maxs, r_shadow_lightscissor))
		return true; // invisible
	if(r_shadow_lightscissor[0] != r_refdef.view.viewport.x
	|| r_shadow_lightscissor[1] != r_refdef.view.viewport.y
	|| r_shadow_lightscissor[2] != r_refdef.view.viewport.width
	|| r_shadow_lightscissor[3] != r_refdef.view.viewport.height)
		r_refdef.stats[r_stat_lights_scissored]++;
	return false;
}

static void R_Shadow_RenderLighting_VisibleLighting(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	// used to display how many times a surface is lit for level design purposes
	RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_NOGAPS, texturenumsurfaces, texturesurfacelist);
	R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, NULL, NULL);
	RSurf_DrawBatch();
}

static void R_Shadow_RenderLighting_Light_GLSL(int texturenumsurfaces, const msurface_t **texturesurfacelist, const float ambientcolor[3], const float diffusecolor[3], const float specularcolor[3])
{
	// ARB2 GLSL shader path (GFFX5200, Radeon 9500)
	R_SetupShader_Surface(ambientcolor, diffusecolor, specularcolor, RSURFPASS_RTLIGHT, texturenumsurfaces, texturesurfacelist, NULL, false, false);
	RSurf_DrawBatch();
}

extern cvar_t gl_lightmaps;
void R_Shadow_RenderLighting(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	qbool negated;
	float ambientcolor[3], diffusecolor[3], specularcolor[3];
	VectorM(rsurface.rtlight->ambientscale + rsurface.texture->rtlightambient, rsurface.texture->render_rtlight_diffuse, ambientcolor);
	VectorM(rsurface.rtlight->diffusescale * max(0, 1.0 - rsurface.texture->rtlightambient), rsurface.texture->render_rtlight_diffuse, diffusecolor);
	VectorM(rsurface.rtlight->specularscale, rsurface.texture->render_rtlight_specular, specularcolor);
	if (!r_shadow_usenormalmap.integer)
	{
		VectorMAM(1.0f, ambientcolor, 1.0f, diffusecolor, ambientcolor);
		VectorClear(diffusecolor);
		VectorClear(specularcolor);
	}
	VectorMultiply(ambientcolor, rsurface.rtlight->currentcolor, ambientcolor);
	VectorMultiply(diffusecolor, rsurface.rtlight->currentcolor, diffusecolor);
	VectorMultiply(specularcolor, rsurface.rtlight->currentcolor, specularcolor);
	if (VectorLength2(ambientcolor) + VectorLength2(diffusecolor) + VectorLength2(specularcolor) < (1.0f / 1048576.0f))
		return;
	negated = (rsurface.rtlight->currentcolor[0] + rsurface.rtlight->currentcolor[1] + rsurface.rtlight->currentcolor[2] < 0);
	if(negated)
	{
		VectorNegate(ambientcolor, ambientcolor);
		VectorNegate(diffusecolor, diffusecolor);
		VectorNegate(specularcolor, specularcolor);
		GL_BlendEquationSubtract(true);
	}
	RSurf_SetupDepthAndCulling(false);
	switch (r_shadow_rendermode)
	{
	case R_SHADOW_RENDERMODE_VISIBLELIGHTING:
		GL_DepthTest(!(rsurface.texture->currentmaterialflags & MATERIALFLAG_NODEPTHTEST) && !r_showdisabledepthtest.integer);
		R_Shadow_RenderLighting_VisibleLighting(texturenumsurfaces, texturesurfacelist);
		break;
	case R_SHADOW_RENDERMODE_LIGHT_GLSL:
		R_Shadow_RenderLighting_Light_GLSL(texturenumsurfaces, texturesurfacelist, ambientcolor, diffusecolor, specularcolor);
		break;
	default:
		Con_Printf("R_Shadow_RenderLighting: unknown r_shadow_rendermode %i\n", r_shadow_rendermode);
		break;
	}
	if(negated)
		GL_BlendEquationSubtract(false);
}

void R_RTLight_Update(rtlight_t *rtlight, int isstatic, matrix4x4_t *matrix, vec3_t color, int style, const char *cubemapname, int shadow, vec_t corona, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags)
{
	matrix4x4_t tempmatrix = *matrix;
	Matrix4x4_Scale(&tempmatrix, r_shadow_lightradiusscale.value, 1);

	// if this light has been compiled before, free the associated data
	R_RTLight_Uncompile(rtlight);

	// clear it completely to avoid any lingering data
	memset(rtlight, 0, sizeof(*rtlight));

	// copy the properties
	rtlight->matrix_lighttoworld = tempmatrix;
	Matrix4x4_Invert_Simple(&rtlight->matrix_worldtolight, &tempmatrix);
	Matrix4x4_OriginFromMatrix(&tempmatrix, rtlight->shadoworigin);
	rtlight->radius = Matrix4x4_ScaleFromMatrix(&tempmatrix);
	VectorCopy(color, rtlight->color);
	rtlight->cubemapname[0] = 0;
	if (cubemapname && cubemapname[0])
		dp_strlcpy(rtlight->cubemapname, cubemapname, sizeof(rtlight->cubemapname));
	rtlight->shadow = shadow;
	rtlight->corona = corona;
	rtlight->style = style;
	rtlight->isstatic = isstatic;
	rtlight->coronasizescale = coronasizescale;
	rtlight->ambientscale = ambientscale;
	rtlight->diffusescale = diffusescale;
	rtlight->specularscale = specularscale;
	rtlight->flags = flags;

	// compute derived data
	//rtlight->cullradius = rtlight->radius;
	//rtlight->cullradius2 = rtlight->radius * rtlight->radius;
	rtlight->cullmins[0] = rtlight->shadoworigin[0] - rtlight->radius;
	rtlight->cullmins[1] = rtlight->shadoworigin[1] - rtlight->radius;
	rtlight->cullmins[2] = rtlight->shadoworigin[2] - rtlight->radius;
	rtlight->cullmaxs[0] = rtlight->shadoworigin[0] + rtlight->radius;
	rtlight->cullmaxs[1] = rtlight->shadoworigin[1] + rtlight->radius;
	rtlight->cullmaxs[2] = rtlight->shadoworigin[2] + rtlight->radius;
}

// compiles rtlight geometry
// (undone by R_FreeCompiledRTLight, which R_UpdateLight calls)
void R_RTLight_Compile(rtlight_t *rtlight)
{
	int i;
	int numsurfaces, numleafs, numleafpvsbytes, numshadowtrispvsbytes, numlighttrispvsbytes;
	int lighttris, shadowtris;
	entity_render_t *ent = r_refdef.scene.worldentity;
	model_t *model = r_refdef.scene.worldmodel;
	unsigned char *data;

	// compile the light
	rtlight->compiled = true;
	rtlight->static_numleafs = 0;
	rtlight->static_numleafpvsbytes = 0;
	rtlight->static_leaflist = NULL;
	rtlight->static_leafpvs = NULL;
	rtlight->static_numsurfaces = 0;
	rtlight->static_surfacelist = NULL;
	rtlight->static_shadowmap_receivers = 0x3F;
	rtlight->static_shadowmap_casters = 0x3F;
	rtlight->cullmins[0] = rtlight->shadoworigin[0] - rtlight->radius;
	rtlight->cullmins[1] = rtlight->shadoworigin[1] - rtlight->radius;
	rtlight->cullmins[2] = rtlight->shadoworigin[2] - rtlight->radius;
	rtlight->cullmaxs[0] = rtlight->shadoworigin[0] + rtlight->radius;
	rtlight->cullmaxs[1] = rtlight->shadoworigin[1] + rtlight->radius;
	rtlight->cullmaxs[2] = rtlight->shadoworigin[2] + rtlight->radius;

	if (model && model->GetLightInfo)
	{
		// this variable must be set for the CompileShadowMap code
		r_shadow_compilingrtlight = rtlight;
		R_FrameData_SetMark();
		model->GetLightInfo(ent, rtlight->shadoworigin, rtlight->radius, rtlight->cullmins, rtlight->cullmaxs, r_shadow_buffer_leaflist, r_shadow_buffer_leafpvs, &numleafs, r_shadow_buffer_surfacelist, r_shadow_buffer_surfacepvs, &numsurfaces, r_shadow_buffer_shadowtrispvs, r_shadow_buffer_lighttrispvs, r_shadow_buffer_visitingleafpvs, 0, NULL, rtlight->shadow == 0);
		R_FrameData_ReturnToMark();
		numleafpvsbytes = (model->brush.num_leafs + 7) >> 3;
		numshadowtrispvsbytes = (model->surfmesh.num_triangles + 7) >> 3;
		numlighttrispvsbytes = (model->surfmesh.num_triangles + 7) >> 3;
		data = (unsigned char *)Mem_Alloc(r_main_mempool, sizeof(int) * numsurfaces + sizeof(int) * numleafs + numleafpvsbytes + numshadowtrispvsbytes + numlighttrispvsbytes);
		rtlight->static_numsurfaces = numsurfaces;
		rtlight->static_surfacelist = (int *)data;data += sizeof(int) * numsurfaces;
		rtlight->static_numleafs = numleafs;
		rtlight->static_leaflist = (int *)data;data += sizeof(int) * numleafs;
		rtlight->static_numleafpvsbytes = numleafpvsbytes;
		rtlight->static_leafpvs = (unsigned char *)data;data += numleafpvsbytes;
		rtlight->static_numshadowtrispvsbytes = numshadowtrispvsbytes;
		rtlight->static_shadowtrispvs = (unsigned char *)data;data += numshadowtrispvsbytes;
		rtlight->static_numlighttrispvsbytes = numlighttrispvsbytes;
		rtlight->static_lighttrispvs = (unsigned char *)data;data += numlighttrispvsbytes;
		if (rtlight->static_numsurfaces)
			memcpy(rtlight->static_surfacelist, r_shadow_buffer_surfacelist, rtlight->static_numsurfaces * sizeof(*rtlight->static_surfacelist));
		if (rtlight->static_numleafs)
			memcpy(rtlight->static_leaflist, r_shadow_buffer_leaflist, rtlight->static_numleafs * sizeof(*rtlight->static_leaflist));
		if (rtlight->static_numleafpvsbytes)
			memcpy(rtlight->static_leafpvs, r_shadow_buffer_leafpvs, rtlight->static_numleafpvsbytes);
		if (rtlight->static_numshadowtrispvsbytes)
			memcpy(rtlight->static_shadowtrispvs, r_shadow_buffer_shadowtrispvs, rtlight->static_numshadowtrispvsbytes);
		if (rtlight->static_numlighttrispvsbytes)
			memcpy(rtlight->static_lighttrispvs, r_shadow_buffer_lighttrispvs, rtlight->static_numlighttrispvsbytes);
		R_FrameData_SetMark();
		if (model->CompileShadowMap && rtlight->shadow)
			model->CompileShadowMap(ent, rtlight->shadoworigin, NULL, rtlight->radius, numsurfaces, r_shadow_buffer_surfacelist);
		R_FrameData_ReturnToMark();
		// now we're done compiling the rtlight
		r_shadow_compilingrtlight = NULL;
	}


	// use smallest available cullradius - box radius or light radius
	//rtlight->cullradius = RadiusFromBoundsAndOrigin(rtlight->cullmins, rtlight->cullmaxs, rtlight->shadoworigin);
	//rtlight->cullradius = min(rtlight->cullradius, rtlight->radius);

	lighttris = 0;
	if (rtlight->static_numlighttrispvsbytes)
		for (i = 0;i < rtlight->static_numlighttrispvsbytes*8;i++)
			if (CHECKPVSBIT(rtlight->static_lighttrispvs, i))
				lighttris++;

	shadowtris = 0;
	if (rtlight->static_numshadowtrispvsbytes)
		for (i = 0;i < rtlight->static_numshadowtrispvsbytes*8;i++)
			if (CHECKPVSBIT(rtlight->static_shadowtrispvs, i))
				shadowtris++;

	if (developer_extra.integer)
		Con_DPrintf("static light built: %f %f %f : %f %f %f box, %i light triangles, %i shadow triangles\n", rtlight->cullmins[0], rtlight->cullmins[1], rtlight->cullmins[2], rtlight->cullmaxs[0], rtlight->cullmaxs[1], rtlight->cullmaxs[2], lighttris, shadowtris);
}

void R_RTLight_Uncompile(rtlight_t *rtlight)
{
	if (rtlight->compiled)
	{
		if (rtlight->static_meshchain_shadow_shadowmap)
			Mod_ShadowMesh_Free(rtlight->static_meshchain_shadow_shadowmap);
		rtlight->static_meshchain_shadow_shadowmap = NULL;
		// these allocations are grouped
		if (rtlight->static_surfacelist)
			Mem_Free(rtlight->static_surfacelist);
		rtlight->static_numleafs = 0;
		rtlight->static_numleafpvsbytes = 0;
		rtlight->static_leaflist = NULL;
		rtlight->static_leafpvs = NULL;
		rtlight->static_numsurfaces = 0;
		rtlight->static_surfacelist = NULL;
		rtlight->static_numshadowtrispvsbytes = 0;
		rtlight->static_shadowtrispvs = NULL;
		rtlight->static_numlighttrispvsbytes = 0;
		rtlight->static_lighttrispvs = NULL;
		rtlight->compiled = false;
	}
}

void R_Shadow_UncompileWorldLights(void)
{
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		R_RTLight_Uncompile(&light->rtlight);
	}
}

static void R_Shadow_ComputeShadowCasterCullingPlanes(rtlight_t *rtlight)
{
	int i, j;
	mplane_t plane;
	// reset the count of frustum planes
	// see rtlight->cached_frustumplanes definition for how much this array
	// can hold
	rtlight->cached_numfrustumplanes = 0;

	if (r_trippy.integer)
		return;

	// haven't implemented a culling path for ortho rendering
	if (!r_refdef.view.useperspective)
	{
		// check if the light is on screen and copy the 4 planes if it is
		for (i = 0;i < 4;i++)
			if (PlaneDiff(rtlight->shadoworigin, &r_refdef.view.frustum[i]) < -0.03125)
				break;
		if (i == 4)
			for (i = 0;i < 4;i++)
				rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = r_refdef.view.frustum[i];
		return;
	}

#if 1
	// generate a deformed frustum that includes the light origin, this is
	// used to cull shadow casting surfaces that can not possibly cast a
	// shadow onto the visible light-receiving surfaces, which can be a
	// performance gain
	//
	// if the light origin is onscreen the result will be 4 planes exactly
	// if the light origin is offscreen on only one axis the result will
	// be exactly 5 planes (split-side case)
	// if the light origin is offscreen on two axes the result will be
	// exactly 4 planes (stretched corner case)
	for (i = 0;i < 4;i++)
	{
		// quickly reject standard frustum planes that put the light
		// origin outside the frustum
		if (PlaneDiff(rtlight->shadoworigin, &r_refdef.view.frustum[i]) < -0.03125)
			continue;
		// copy the plane
		rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = r_refdef.view.frustum[i];
	}
	// if all the standard frustum planes were accepted, the light is onscreen
	// otherwise we need to generate some more planes below...
	if (rtlight->cached_numfrustumplanes < 4)
	{
		// at least one of the stock frustum planes failed, so we need to
		// create one or two custom planes to enclose the light origin
		for (i = 0;i < 4;i++)
		{
			// create a plane using the view origin and light origin, and a
			// single point from the frustum corner set
			TriangleNormal(r_refdef.view.origin, r_refdef.view.frustumcorner[i], rtlight->shadoworigin, plane.normal);
			VectorNormalize(plane.normal);
			plane.dist = DotProduct(r_refdef.view.origin, plane.normal);
			// see if this plane is backwards and flip it if so
			for (j = 0;j < 4;j++)
				if (j != i && DotProduct(r_refdef.view.frustumcorner[j], plane.normal) - plane.dist < -0.03125)
					break;
			if (j < 4)
			{
				VectorNegate(plane.normal, plane.normal);
				plane.dist *= -1;
				// flipped plane, test again to see if it is now valid
				for (j = 0;j < 4;j++)
					if (j != i && DotProduct(r_refdef.view.frustumcorner[j], plane.normal) - plane.dist < -0.03125)
						break;
				// if the plane is still not valid, then it is dividing the
				// frustum and has to be rejected
				if (j < 4)
					continue;
			}
			// we have created a valid plane, compute extra info
			PlaneClassify(&plane);
			// copy the plane
			rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = plane;
#if 1
			// if we've found 5 frustum planes then we have constructed a
			// proper split-side case and do not need to keep searching for
			// planes to enclose the light origin
			if (rtlight->cached_numfrustumplanes == 5)
				break;
#endif
		}
	}
#endif

#if 0
	for (i = 0;i < rtlight->cached_numfrustumplanes;i++)
	{
		plane = rtlight->cached_frustumplanes[i];
		Con_Printf("light %p plane #%i %f %f %f : %f (%f %f %f %f %f)\n", rtlight, i, plane.normal[0], plane.normal[1], plane.normal[2], plane.dist, PlaneDiff(r_refdef.view.frustumcorner[0], &plane), PlaneDiff(r_refdef.view.frustumcorner[1], &plane), PlaneDiff(r_refdef.view.frustumcorner[2], &plane), PlaneDiff(r_refdef.view.frustumcorner[3], &plane), PlaneDiff(rtlight->shadoworigin, &plane));
	}
#endif

#if 0
	// now add the light-space box planes if the light box is rotated, as any
	// caster outside the oriented light box is irrelevant (even if it passed
	// the worldspace light box, which is axial)
	if (rtlight->matrix_lighttoworld.m[0][0] != 1 || rtlight->matrix_lighttoworld.m[1][1] != 1 || rtlight->matrix_lighttoworld.m[2][2] != 1)
	{
		for (i = 0;i < 6;i++)
		{
			vec3_t v;
			VectorClear(v);
			v[i >> 1] = (i & 1) ? -1 : 1;
			Matrix4x4_Transform(&rtlight->matrix_lighttoworld, v, plane.normal);
			VectorSubtract(plane.normal, rtlight->shadoworigin, plane.normal);
			plane.dist = VectorNormalizeLength(plane.normal);
			plane.dist += DotProduct(plane.normal, rtlight->shadoworigin);
			rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = plane;
		}
	}
#endif

#if 0
	// add the world-space reduced box planes
	for (i = 0;i < 6;i++)
	{
		VectorClear(plane.normal);
		plane.normal[i >> 1] = (i & 1) ? -1 : 1;
		plane.dist = (i & 1) ? -rtlight->cached_cullmaxs[i >> 1] : rtlight->cached_cullmins[i >> 1];
		rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = plane;
	}
#endif

#if 0
	{
	int j, oldnum;
	vec3_t points[8];
	vec_t bestdist;
	// reduce all plane distances to tightly fit the rtlight cull box, which
	// is in worldspace
	VectorSet(points[0], rtlight->cached_cullmins[0], rtlight->cached_cullmins[1], rtlight->cached_cullmins[2]);
	VectorSet(points[1], rtlight->cached_cullmaxs[0], rtlight->cached_cullmins[1], rtlight->cached_cullmins[2]);
	VectorSet(points[2], rtlight->cached_cullmins[0], rtlight->cached_cullmaxs[1], rtlight->cached_cullmins[2]);
	VectorSet(points[3], rtlight->cached_cullmaxs[0], rtlight->cached_cullmaxs[1], rtlight->cached_cullmins[2]);
	VectorSet(points[4], rtlight->cached_cullmins[0], rtlight->cached_cullmins[1], rtlight->cached_cullmaxs[2]);
	VectorSet(points[5], rtlight->cached_cullmaxs[0], rtlight->cached_cullmins[1], rtlight->cached_cullmaxs[2]);
	VectorSet(points[6], rtlight->cached_cullmins[0], rtlight->cached_cullmaxs[1], rtlight->cached_cullmaxs[2]);
	VectorSet(points[7], rtlight->cached_cullmaxs[0], rtlight->cached_cullmaxs[1], rtlight->cached_cullmaxs[2]);
	oldnum = rtlight->cached_numfrustumplanes;
	rtlight->cached_numfrustumplanes = 0;
	for (j = 0;j < oldnum;j++)
	{
		// find the nearest point on the box to this plane
		bestdist = DotProduct(rtlight->cached_frustumplanes[j].normal, points[0]);
		for (i = 1;i < 8;i++)
		{
			dist = DotProduct(rtlight->cached_frustumplanes[j].normal, points[i]);
			if (bestdist > dist)
				bestdist = dist;
		}
		Con_Printf("light %p %splane #%i %f %f %f : %f < %f\n", rtlight, rtlight->cached_frustumplanes[j].dist < bestdist + 0.03125 ? "^2" : "^1", j, rtlight->cached_frustumplanes[j].normal[0], rtlight->cached_frustumplanes[j].normal[1], rtlight->cached_frustumplanes[j].normal[2], rtlight->cached_frustumplanes[j].dist, bestdist);
		// if the nearest point is near or behind the plane, we want this
		// plane, otherwise the plane is useless as it won't cull anything
		if (rtlight->cached_frustumplanes[j].dist < bestdist + 0.03125)
		{
			PlaneClassify(&rtlight->cached_frustumplanes[j]);
			rtlight->cached_frustumplanes[rtlight->cached_numfrustumplanes++] = rtlight->cached_frustumplanes[j];
		}
	}
	}
#endif
}

static void R_Shadow_DrawWorldShadow_ShadowMap(int numsurfaces, int *surfacelist, const unsigned char *trispvs, const unsigned char *surfacesides)
{
	RSurf_ActiveModelEntity(r_refdef.scene.worldentity, false, false, false);

	if (rsurface.rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
	{
		shadowmesh_t *mesh = rsurface.rtlight->static_meshchain_shadow_shadowmap;
		if (mesh->sidetotals[r_shadow_shadowmapside])
		{
			CHECKGLERROR
			GL_CullFace(GL_NONE);
			r_refdef.stats[r_stat_lights_shadowtriangles] += mesh->sidetotals[r_shadow_shadowmapside];
			R_Mesh_PrepareVertices_Vertex3f(mesh->numverts, mesh->vertex3f, mesh->vbo_vertexbuffer, mesh->vbooffset_vertex3f);
			R_Mesh_Draw(0, mesh->numverts, mesh->sideoffsets[r_shadow_shadowmapside], mesh->sidetotals[r_shadow_shadowmapside], mesh->element3i, mesh->element3i_indexbuffer, mesh->element3i_bufferoffset, mesh->element3s, mesh->element3s_indexbuffer, mesh->element3s_bufferoffset);
			CHECKGLERROR
		}
	}
	else if (r_refdef.scene.worldentity->model)
		r_refdef.scene.worldmodel->DrawShadowMap(r_shadow_shadowmapside, r_refdef.scene.worldentity, rsurface.rtlight->shadoworigin, NULL, rsurface.rtlight->radius, numsurfaces, surfacelist, surfacesides, rsurface.rtlight->cached_cullmins, rsurface.rtlight->cached_cullmaxs);

	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

static void R_Shadow_DrawEntityShadow(entity_render_t *ent)
{
	vec3_t relativeshadoworigin, relativeshadowmins, relativeshadowmaxs;
	vec_t relativeshadowradius;
	RSurf_ActiveModelEntity(ent, false, false, false);
	Matrix4x4_Transform(&ent->inversematrix, rsurface.rtlight->shadoworigin, relativeshadoworigin);
	// we need to re-init the shader for each entity because the matrix changed
	relativeshadowradius = rsurface.rtlight->radius / ent->scale;
	relativeshadowmins[0] = relativeshadoworigin[0] - relativeshadowradius;
	relativeshadowmins[1] = relativeshadoworigin[1] - relativeshadowradius;
	relativeshadowmins[2] = relativeshadoworigin[2] - relativeshadowradius;
	relativeshadowmaxs[0] = relativeshadoworigin[0] + relativeshadowradius;
	relativeshadowmaxs[1] = relativeshadoworigin[1] + relativeshadowradius;
	relativeshadowmaxs[2] = relativeshadoworigin[2] + relativeshadowradius;
	ent->model->DrawShadowMap(r_shadow_shadowmapside, ent, relativeshadoworigin, NULL, relativeshadowradius, ent->model->submodelsurfaces_end - ent->model->submodelsurfaces_start, ent->model->modelsurfaces_sorted + ent->model->submodelsurfaces_start, NULL, relativeshadowmins, relativeshadowmaxs);
	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

void R_Shadow_SetupEntityLight(const entity_render_t *ent)
{
	// set up properties for rendering light onto this entity
	RSurf_ActiveModelEntity(ent, true, true, false);
	Matrix4x4_Concat(&rsurface.entitytolight, &rsurface.rtlight->matrix_worldtolight, &ent->matrix);
	Matrix4x4_Concat(&rsurface.entitytoattenuationxyz, &matrix_attenuationxyz, &rsurface.entitytolight);
	Matrix4x4_Concat(&rsurface.entitytoattenuationz, &matrix_attenuationz, &rsurface.entitytolight);
	Matrix4x4_Transform(&ent->inversematrix, rsurface.rtlight->shadoworigin, rsurface.entitylightorigin);
}

static void R_Shadow_DrawWorldLight(int numsurfaces, int *surfacelist, const unsigned char *lighttrispvs)
{
	if (!r_refdef.scene.worldmodel->DrawLight)
		return;

	// set up properties for rendering light onto this entity
	RSurf_ActiveModelEntity(r_refdef.scene.worldentity, false, false, false);
	rsurface.entitytolight = rsurface.rtlight->matrix_worldtolight;
	Matrix4x4_Concat(&rsurface.entitytoattenuationxyz, &matrix_attenuationxyz, &rsurface.entitytolight);
	Matrix4x4_Concat(&rsurface.entitytoattenuationz, &matrix_attenuationz, &rsurface.entitytolight);
	VectorCopy(rsurface.rtlight->shadoworigin, rsurface.entitylightorigin);

	r_refdef.scene.worldmodel->DrawLight(r_refdef.scene.worldentity, numsurfaces, surfacelist, lighttrispvs);

	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

static void R_Shadow_DrawEntityLight(entity_render_t *ent)
{
	model_t *model = ent->model;
	if (!model->DrawLight)
		return;

	R_Shadow_SetupEntityLight(ent);

	model->DrawLight(ent, model->submodelsurfaces_end - model->submodelsurfaces_start, model->modelsurfaces_sorted + model->submodelsurfaces_start, NULL);

	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

static void R_Shadow_PrepareLight(rtlight_t *rtlight)
{
	int i;
	float f;
	int numleafs, numsurfaces;
	int *leaflist, *surfacelist;
	unsigned char *leafpvs;
	unsigned char *shadowtrispvs;
	unsigned char *lighttrispvs;
	//unsigned char *surfacesides;
	int numlightentities;
	int numlightentities_noselfshadow;
	int numshadowentities;
	int numshadowentities_noselfshadow;
	// FIXME: bounds check lightentities and shadowentities, etc.
	static entity_render_t *lightentities[MAX_EDICTS];
	static entity_render_t *lightentities_noselfshadow[MAX_EDICTS];
	static entity_render_t *shadowentities[MAX_EDICTS];
	static entity_render_t *shadowentities_noselfshadow[MAX_EDICTS];
	qbool nolight;
	qbool castshadows;

	rtlight->draw = false;
	rtlight->cached_numlightentities = 0;
	rtlight->cached_numlightentities_noselfshadow = 0;
	rtlight->cached_numshadowentities = 0;
	rtlight->cached_numshadowentities_noselfshadow = 0;
	rtlight->cached_numsurfaces = 0;
	rtlight->cached_lightentities = NULL;
	rtlight->cached_lightentities_noselfshadow = NULL;
	rtlight->cached_shadowentities = NULL;
	rtlight->cached_shadowentities_noselfshadow = NULL;
	rtlight->cached_shadowtrispvs = NULL;
	rtlight->cached_lighttrispvs = NULL;
	rtlight->cached_surfacelist = NULL;
	rtlight->shadowmapsidesize = 0;

	// skip lights that don't light because of ambientscale+diffusescale+specularscale being 0 (corona only lights)
	// skip lights that are basically invisible (color 0 0 0)
	nolight = VectorLength2(rtlight->color) * (rtlight->ambientscale + rtlight->diffusescale + rtlight->specularscale) < (1.0f / 1048576.0f);

	// loading is done before visibility checks because loading should happen
	// all at once at the start of a level, not when it stalls gameplay.
	// (especially important to benchmarks)
	// compile light
	if (rtlight->isstatic && !nolight && !rtlight->compiled && r_shadow_realtime_world_compile.integer)
		R_RTLight_Compile(rtlight);

	// load cubemap
	rtlight->currentcubemap = rtlight->cubemapname[0] ? R_GetCubemap(rtlight->cubemapname) : r_texture_whitecube;

	// look up the light style value at this time
	f = ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.rtlightstylevalue[rtlight->style] : 1) * r_shadow_lightintensityscale.value;
	VectorScale(rtlight->color, f, rtlight->currentcolor);
	/*
	if (rtlight->selected)
	{
		f = 2 + sin(host.realtime * M_PI * 4.0);
		VectorScale(rtlight->currentcolor, f, rtlight->currentcolor);
	}
	*/

	// skip if lightstyle is currently off
	if (VectorLength2(rtlight->currentcolor) < (1.0f / 1048576.0f))
		return;

	// skip processing on corona-only lights
	if (nolight)
		return;

	// skip if the light box is not touching any visible leafs
	if (r_shadow_culllights_pvs.integer
		&& r_refdef.scene.worldmodel
		&& r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs
		&& !r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs(r_refdef.scene.worldmodel, r_refdef.viewcache.world_leafvisible, rtlight->cullmins, rtlight->cullmaxs))
		return;

	// skip if the light box is not visible to traceline
	if (r_shadow_culllights_trace.integer)
	{
		if (rtlight->trace_timer != host.realtime && R_CanSeeBox(rtlight->trace_timer == 0 ? r_shadow_culllights_trace_tempsamples.integer : r_shadow_culllights_trace_samples.integer, r_shadow_culllights_trace_eyejitter.value, r_shadow_culllights_trace_enlarge.value, r_shadow_culllights_trace_expand.value, r_shadow_culllights_trace_pad.value, r_refdef.view.origin, rtlight->cullmins, rtlight->cullmaxs))
			rtlight->trace_timer = host.realtime;
		if (host.realtime - rtlight->trace_timer > r_shadow_culllights_trace_delay.value)
			return;
	}

	// skip if the light box is off screen
	if (R_CullFrustum(rtlight->cullmins, rtlight->cullmaxs))
		return;

	// in the typical case this will be quickly replaced by GetLightInfo
	VectorCopy(rtlight->cullmins, rtlight->cached_cullmins);
	VectorCopy(rtlight->cullmaxs, rtlight->cached_cullmaxs);

	R_Shadow_ComputeShadowCasterCullingPlanes(rtlight);

	// don't allow lights to be drawn if using r_shadow_bouncegrid 2, except if we're using static bouncegrid where dynamic lights still need to draw
	if (r_shadow_bouncegrid.integer == 2 && (rtlight->isstatic || !r_shadow_bouncegrid_static.integer))
		return;

	if (rtlight->compiled && r_shadow_realtime_world_compile.integer)
	{
		// compiled light, world available and can receive realtime lighting
		// retrieve leaf information
		numleafs = rtlight->static_numleafs;
		leaflist = rtlight->static_leaflist;
		leafpvs = rtlight->static_leafpvs;
		numsurfaces = rtlight->static_numsurfaces;
		surfacelist = rtlight->static_surfacelist;
		//surfacesides = NULL;
		shadowtrispvs = rtlight->static_shadowtrispvs;
		lighttrispvs = rtlight->static_lighttrispvs;
	}
	else if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->GetLightInfo)
	{
		// dynamic light, world available and can receive realtime lighting
		// calculate lit surfaces and leafs
		r_refdef.scene.worldmodel->GetLightInfo(r_refdef.scene.worldentity, rtlight->shadoworigin, rtlight->radius, rtlight->cached_cullmins, rtlight->cached_cullmaxs, r_shadow_buffer_leaflist, r_shadow_buffer_leafpvs, &numleafs, r_shadow_buffer_surfacelist, r_shadow_buffer_surfacepvs, &numsurfaces, r_shadow_buffer_shadowtrispvs, r_shadow_buffer_lighttrispvs, r_shadow_buffer_visitingleafpvs, rtlight->cached_numfrustumplanes, rtlight->cached_frustumplanes, rtlight->shadow == 0);
		R_Shadow_ComputeShadowCasterCullingPlanes(rtlight);
		leaflist = r_shadow_buffer_leaflist;
		leafpvs = r_shadow_buffer_leafpvs;
		surfacelist = r_shadow_buffer_surfacelist;
		//surfacesides = r_shadow_buffer_surfacesides;
		shadowtrispvs = r_shadow_buffer_shadowtrispvs;
		lighttrispvs = r_shadow_buffer_lighttrispvs;
		// if the reduced leaf bounds are offscreen, skip it
		if (R_CullFrustum(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
			return;
	}
	else
	{
		// no world
		numleafs = 0;
		leaflist = NULL;
		leafpvs = NULL;
		numsurfaces = 0;
		surfacelist = NULL;
		//surfacesides = NULL;
		shadowtrispvs = NULL;
		lighttrispvs = NULL;
	}
	// check if light is illuminating any visible leafs
	if (numleafs)
	{
		for (i = 0; i < numleafs; i++)
			if (r_refdef.viewcache.world_leafvisible[leaflist[i]])
				break;
		if (i == numleafs)
			return;
	}

	// make a list of lit entities and shadow casting entities
	numlightentities = 0;
	numlightentities_noselfshadow = 0;
	numshadowentities = 0;
	numshadowentities_noselfshadow = 0;

	// add dynamic entities that are lit by the light
	for (i = 0; i < r_refdef.scene.numentities; i++)
	{
		model_t *model;
		entity_render_t *ent = r_refdef.scene.entities[i];
		vec3_t org;
		if (!BoxesOverlap(ent->mins, ent->maxs, rtlight->cached_cullmins, rtlight->cached_cullmaxs))
			continue;
		// skip the object entirely if it is not within the valid
		// shadow-casting region (which includes the lit region)
		if (R_CullBox(ent->mins, ent->maxs, rtlight->cached_numfrustumplanes, rtlight->cached_frustumplanes))
			continue;
		if (!(model = ent->model))
			continue;
		if (r_refdef.viewcache.entityvisible[i] && model->DrawLight && (ent->flags & RENDER_LIGHT))
		{
			// this entity wants to receive light, is visible, and is
			// inside the light box
			// TODO: check if the surfaces in the model can receive light
			// so now check if it's in a leaf seen by the light
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS && !r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS(r_refdef.scene.worldmodel, leafpvs, ent->mins, ent->maxs))
				continue;
			if (ent->flags & RENDER_NOSELFSHADOW)
				lightentities_noselfshadow[numlightentities_noselfshadow++] = ent;
			else
				lightentities[numlightentities++] = ent;
			// since it is lit, it probably also casts a shadow...
			// about the VectorDistance2 - light emitting entities should not cast their own shadow
			Matrix4x4_OriginFromMatrix(&ent->matrix, org);
			if ((ent->flags & RENDER_SHADOW) && model->DrawShadowMap && VectorDistance2(org, rtlight->shadoworigin) > 0.1)
			{
				// note: exterior models without the RENDER_NOSELFSHADOW
				// flag still create a RENDER_NOSELFSHADOW shadow but
				// are lit normally, this means that they are
				// self-shadowing but do not shadow other
				// RENDER_NOSELFSHADOW entities such as the gun
				// (very weird, but keeps the player shadow off the gun)
				if (ent->flags & (RENDER_NOSELFSHADOW | RENDER_EXTERIORMODEL))
					shadowentities_noselfshadow[numshadowentities_noselfshadow++] = ent;
				else
					shadowentities[numshadowentities++] = ent;
			}
		}
		else if (ent->flags & RENDER_SHADOW)
		{
			// this entity is not receiving light, but may still need to
			// cast a shadow...
			// TODO: check if the surfaces in the model can cast shadow
			// now check if it is in a leaf seen by the light
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS && !r_refdef.scene.worldmodel->brush.BoxTouchingLeafPVS(r_refdef.scene.worldmodel, leafpvs, ent->mins, ent->maxs))
				continue;
			// about the VectorDistance2 - light emitting entities should not cast their own shadow
			Matrix4x4_OriginFromMatrix(&ent->matrix, org);
			if ((ent->flags & RENDER_SHADOW) && model->DrawShadowMap && VectorDistance2(org, rtlight->shadoworigin) > 0.1)
			{
				if (ent->flags & (RENDER_NOSELFSHADOW | RENDER_EXTERIORMODEL))
					shadowentities_noselfshadow[numshadowentities_noselfshadow++] = ent;
				else
					shadowentities[numshadowentities++] = ent;
			}
		}
	}

	// return if there's nothing at all to light
	if (numsurfaces + numlightentities + numlightentities_noselfshadow == 0)
		return;

	// count this light in the r_speeds
	r_refdef.stats[r_stat_lights]++;

	// flag it as worth drawing later
	rtlight->draw = true;

	// if we have shadows disabled, don't count the shadow entities, this way we don't do the R_AnimCache_GetEntity on each one
	castshadows = numsurfaces + numshadowentities + numshadowentities_noselfshadow > 0 && rtlight->shadow && (rtlight->isstatic ? r_refdef.scene.rtworldshadows : r_refdef.scene.rtdlightshadows);
	if (!castshadows)
		numshadowentities = numshadowentities_noselfshadow = 0;
	rtlight->castshadows = castshadows;

	// cache all the animated entities that cast a shadow but are not visible
	for (i = 0; i < numshadowentities; i++)
		R_AnimCache_GetEntity(shadowentities[i], false, false);
	for (i = 0; i < numshadowentities_noselfshadow; i++)
		R_AnimCache_GetEntity(shadowentities_noselfshadow[i], false, false);

	// we can convert noselfshadow to regular if there are no receivers of that type (or we're using r_shadow_deferred which doesn't support noselfshadow anyway)
	if (numshadowentities_noselfshadow > 0 && (numlightentities_noselfshadow == 0 || r_shadow_usingdeferredprepass))
	{
		for (i = 0; i < numshadowentities_noselfshadow; i++)
			shadowentities[numshadowentities++] = shadowentities_noselfshadow[i];
		numshadowentities_noselfshadow = 0;
	}

	// we can convert noselfshadow to regular if there are no casters of that type
	if (numlightentities_noselfshadow > 0 && numshadowentities_noselfshadow == 0)
	{
		for (i = 0; i < numlightentities_noselfshadow; i++)
			lightentities[numlightentities++] = lightentities_noselfshadow[i];
		numlightentities_noselfshadow = 0;
	}

	// allocate some temporary memory for rendering this light later in the frame
	// reusable buffers need to be copied, static data can be used as-is
	rtlight->cached_numlightentities               = numlightentities;
	rtlight->cached_numlightentities_noselfshadow  = numlightentities_noselfshadow;
	rtlight->cached_numshadowentities              = numshadowentities;
	rtlight->cached_numshadowentities_noselfshadow = numshadowentities_noselfshadow;
	rtlight->cached_numsurfaces                    = numsurfaces;
	rtlight->cached_lightentities                  = (entity_render_t**)R_FrameData_Store(numlightentities*sizeof(entity_render_t*), (void*)lightentities);
	rtlight->cached_lightentities_noselfshadow     = (entity_render_t**)R_FrameData_Store(numlightentities_noselfshadow*sizeof(entity_render_t*), (void*)lightentities_noselfshadow);
	rtlight->cached_shadowentities                 = (entity_render_t**)R_FrameData_Store(numshadowentities*sizeof(entity_render_t*), (void*)shadowentities);
	rtlight->cached_shadowentities_noselfshadow    = (entity_render_t**)R_FrameData_Store(numshadowentities_noselfshadow*sizeof(entity_render_t *), (void*)shadowentities_noselfshadow);
	if (shadowtrispvs == r_shadow_buffer_shadowtrispvs)
	{
		int numshadowtrispvsbytes = ((r_refdef.scene.worldmodel->surfmesh.num_triangles + 7) >> 3);
		int numlighttrispvsbytes = ((r_refdef.scene.worldmodel->surfmesh.num_triangles + 7) >> 3);
		rtlight->cached_shadowtrispvs                  =   (unsigned char *)R_FrameData_Store(numshadowtrispvsbytes, shadowtrispvs);
		rtlight->cached_lighttrispvs                   =   (unsigned char *)R_FrameData_Store(numlighttrispvsbytes, lighttrispvs);
		rtlight->cached_surfacelist                    =              (int*)R_FrameData_Store(numsurfaces*sizeof(int), (void*)surfacelist);
	}
	else
	{
		// compiled light data
		rtlight->cached_shadowtrispvs = shadowtrispvs;
		rtlight->cached_lighttrispvs = lighttrispvs;
		rtlight->cached_surfacelist = surfacelist;
	}

	if (R_Shadow_ShadowMappingEnabled())
	{
		// figure out the shadowmapping parameters for this light
		vec3_t nearestpoint;
		vec_t distance;
		int lodlinear;
		nearestpoint[0] = bound(rtlight->cullmins[0], r_refdef.view.origin[0], rtlight->cullmaxs[0]);
		nearestpoint[1] = bound(rtlight->cullmins[1], r_refdef.view.origin[1], rtlight->cullmaxs[1]);
		nearestpoint[2] = bound(rtlight->cullmins[2], r_refdef.view.origin[2], rtlight->cullmaxs[2]);
		distance = VectorDistance(nearestpoint, r_refdef.view.origin);
		lodlinear = (rtlight->radius * r_shadow_shadowmapping_precision.value) / sqrt(max(1.0f, distance / rtlight->radius));
		//lodlinear = (int)(r_shadow_shadowmapping_lod_bias.value + r_shadow_shadowmapping_lod_scale.value * rtlight->radius / max(1.0f, distance));
		lodlinear = bound(r_shadow_shadowmapping_minsize.integer, lodlinear, r_shadow_shadowmapmaxsize);
		rtlight->shadowmapsidesize = bound(r_shadow_shadowmapborder, lodlinear, r_shadow_shadowmapmaxsize);
		// shadowmapatlas* variables will be set by R_Shadow_PrepareLights()
	}
}

static void R_Shadow_DrawLightShadowMaps(rtlight_t *rtlight)
{
	int i;
	int numsurfaces;
	unsigned char *shadowtrispvs, *surfacesides;
	int numlightentities;
	int numlightentities_noselfshadow;
	int numshadowentities;
	int numshadowentities_noselfshadow;
	entity_render_t **lightentities;
	entity_render_t **lightentities_noselfshadow;
	entity_render_t **shadowentities;
	entity_render_t **shadowentities_noselfshadow;
	int *surfacelist;
	static unsigned char entitysides[MAX_EDICTS];
	static unsigned char entitysides_noselfshadow[MAX_EDICTS];
	float borderbias;
	int side;
	int size;
	int castermask;
	int receivermask;
	matrix4x4_t radiustolight;

	// check if we cached this light this frame (meaning it is worth drawing)
	if (!rtlight->draw || !rtlight->castshadows)
		return;

	// if PrepareLights could not find any space for this shadowmap, we may as well mark it as not casting shadows...
	if (rtlight->shadowmapatlassidesize == 0)
	{
		rtlight->castshadows = false;
		return;
	}

	// set up a scissor rectangle for this light
	if (R_Shadow_ScissorForBBox(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
		return;

	numlightentities = rtlight->cached_numlightentities;
	numlightentities_noselfshadow = rtlight->cached_numlightentities_noselfshadow;
	numshadowentities = rtlight->cached_numshadowentities;
	numshadowentities_noselfshadow = rtlight->cached_numshadowentities_noselfshadow;
	numsurfaces = rtlight->cached_numsurfaces;
	lightentities = rtlight->cached_lightentities;
	lightentities_noselfshadow = rtlight->cached_lightentities_noselfshadow;
	shadowentities = rtlight->cached_shadowentities;
	shadowentities_noselfshadow = rtlight->cached_shadowentities_noselfshadow;
	shadowtrispvs = rtlight->cached_shadowtrispvs;
	surfacelist = rtlight->cached_surfacelist;

	// make this the active rtlight for rendering purposes
	R_Shadow_RenderMode_ActiveLight(rtlight);

	radiustolight = rtlight->matrix_worldtolight;
	Matrix4x4_Abs(&radiustolight);

	size = rtlight->shadowmapatlassidesize;
	borderbias = r_shadow_shadowmapborder / (float)(size - r_shadow_shadowmapborder);

	surfacesides = NULL;
	castermask = 0;
	receivermask = 0;
	if (numsurfaces)
	{
		if (rtlight->compiled && r_shadow_realtime_world_compile.integer && r_shadow_realtime_world_compileshadow.integer)
		{
			castermask = rtlight->static_shadowmap_casters;
			receivermask = rtlight->static_shadowmap_receivers;
		}
		else
		{
			surfacesides = r_shadow_buffer_surfacesides;
			for (i = 0; i < numsurfaces; i++)
			{
				msurface_t *surface = r_refdef.scene.worldmodel->data_surfaces + surfacelist[i];
				surfacesides[i] = R_Shadow_CalcBBoxSideMask(surface->mins, surface->maxs, &rtlight->matrix_worldtolight, &radiustolight, borderbias);
				castermask |= surfacesides[i];
				receivermask |= surfacesides[i];
			}
		}
	}

	for (i = 0; i < numlightentities && receivermask < 0x3F; i++)
		receivermask |= R_Shadow_CalcEntitySideMask(lightentities[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias);
	for (i = 0; i < numlightentities_noselfshadow && receivermask < 0x3F; i++)
		receivermask |= R_Shadow_CalcEntitySideMask(lightentities_noselfshadow[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias);

	receivermask &= R_Shadow_CullFrustumSides(rtlight, size, r_shadow_shadowmapborder);

	if (receivermask)
	{
		for (i = 0; i < numshadowentities; i++)
			castermask |= (entitysides[i] = R_Shadow_CalcEntitySideMask(shadowentities[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias));
		for (i = 0; i < numshadowentities_noselfshadow; i++)
			castermask |= (entitysides_noselfshadow[i] = R_Shadow_CalcEntitySideMask(shadowentities_noselfshadow[i], &rtlight->matrix_worldtolight, &radiustolight, borderbias));
	}

	// there is no need to render shadows for sides that have no receivers...
	castermask &= receivermask;

	//Con_Printf("distance %f lodlinear %i size %i\n", distance, lodlinear, size);

	// render shadow casters into shadowmaps for this light
	for (side = 0; side < 6; side++)
	{
		int bit = 1 << side;
		if (castermask & bit)
		{
			R_Shadow_RenderMode_ShadowMap(side, size, rtlight->shadowmapatlasposition[0], rtlight->shadowmapatlasposition[1]);
			if (numsurfaces)
				R_Shadow_DrawWorldShadow_ShadowMap(numsurfaces, surfacelist, shadowtrispvs, surfacesides);
			for (i = 0; i < numshadowentities; i++)
				if (entitysides[i] & bit)
					R_Shadow_DrawEntityShadow(shadowentities[i]);
			for (i = 0; i < numshadowentities_noselfshadow; i++)
				if (entitysides_noselfshadow[i] & bit)
					R_Shadow_DrawEntityShadow(shadowentities_noselfshadow[i]);
		}
	}
	// additionally if there are any noselfshadow casters we have to render a second set of shadowmaps without those :(
	if (numshadowentities_noselfshadow)
	{
		for (side = 0; side < 6; side++)
		{
			int bit = 1 << side;
			if (castermask & bit)
			{
				R_Shadow_RenderMode_ShadowMap(side, size, rtlight->shadowmapatlasposition[0] + size * 2, rtlight->shadowmapatlasposition[1]);
				if (numsurfaces)
					R_Shadow_DrawWorldShadow_ShadowMap(numsurfaces, surfacelist, shadowtrispvs, surfacesides);
				for (i = 0; i < numshadowentities; i++)
					if (entitysides[i] & bit)
						R_Shadow_DrawEntityShadow(shadowentities[i]);
			}
		}
	}
}

static void R_Shadow_DrawLight(rtlight_t *rtlight)
{
	int i;
	int numsurfaces;
	unsigned char *lighttrispvs;
	int numlightentities;
	int numlightentities_noselfshadow;
	entity_render_t **lightentities;
	entity_render_t **lightentities_noselfshadow;
	int *surfacelist;
	qbool castshadows;

	// check if we cached this light this frame (meaning it is worth drawing)
	if (!rtlight->draw)
		return;

	// set up a scissor rectangle for this light
	if (R_Shadow_ScissorForBBox(rtlight->cached_cullmins, rtlight->cached_cullmaxs))
		return;

	numlightentities = rtlight->cached_numlightentities;
	numlightentities_noselfshadow = rtlight->cached_numlightentities_noselfshadow;
	numsurfaces = rtlight->cached_numsurfaces;
	lightentities = rtlight->cached_lightentities;
	lightentities_noselfshadow = rtlight->cached_lightentities_noselfshadow;
	lighttrispvs = rtlight->cached_lighttrispvs;
	surfacelist = rtlight->cached_surfacelist;
	castshadows = rtlight->castshadows;

	// make this the active rtlight for rendering purposes
	R_Shadow_RenderMode_ActiveLight(rtlight);

	if (r_showlighting.integer && r_refdef.view.showdebug && numsurfaces + numlightentities + numlightentities_noselfshadow)
	{
		// optionally draw the illuminated areas
		// for performance analysis by level designers
		R_Shadow_RenderMode_VisibleLighting(false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
		for (i = 0;i < numlightentities_noselfshadow;i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
	}

	if (castshadows && r_shadow_shadowmode == R_SHADOW_SHADOWMODE_SHADOWMAP2D)
	{
		matrix4x4_t radiustolight = rtlight->matrix_worldtolight;
		Matrix4x4_Abs(&radiustolight);

		//Con_Printf("distance %f lodlinear %i size %i\n", distance, lodlinear, size);

		// render lighting using the depth texture as shadowmap
		// draw lighting in the unmasked areas
		if (numsurfaces + numlightentities)
		{
			R_Shadow_RenderMode_Lighting(false, true, false);
			// draw lighting in the unmasked areas
			if (numsurfaces)
				R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
			for (i = 0; i < numlightentities; i++)
				R_Shadow_DrawEntityLight(lightentities[i]);
		}
		// offset to the noselfshadow part of the atlas and draw those too
		if (numlightentities_noselfshadow)
		{
			R_Shadow_RenderMode_Lighting(false, true, true);
			for (i = 0; i < numlightentities_noselfshadow; i++)
				R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);
		}

		// rasterize the box when rendering deferred lighting - the regular surface lighting only applies to transparent surfaces
		if (r_shadow_usingdeferredprepass)
			R_Shadow_RenderMode_DrawDeferredLight(true);
	}
	else
	{
		// draw lighting in the unmasked areas
		R_Shadow_RenderMode_Lighting(false, false, false);
		if (numsurfaces)
			R_Shadow_DrawWorldLight(numsurfaces, surfacelist, lighttrispvs);
		for (i = 0;i < numlightentities;i++)
			R_Shadow_DrawEntityLight(lightentities[i]);
		for (i = 0;i < numlightentities_noselfshadow;i++)
			R_Shadow_DrawEntityLight(lightentities_noselfshadow[i]);

		// rasterize the box when rendering deferred lighting - the regular surface lighting only applies to transparent surfaces
		if (r_shadow_usingdeferredprepass)
			R_Shadow_RenderMode_DrawDeferredLight(false);
	}
}

static void R_Shadow_FreeDeferred(void)
{
	R_Mesh_DestroyFramebufferObject(r_shadow_prepassgeometryfbo);
	r_shadow_prepassgeometryfbo = 0;

	R_Mesh_DestroyFramebufferObject(r_shadow_prepasslightingdiffusespecularfbo);
	r_shadow_prepasslightingdiffusespecularfbo = 0;

	R_Mesh_DestroyFramebufferObject(r_shadow_prepasslightingdiffusefbo);
	r_shadow_prepasslightingdiffusefbo = 0;

	if (r_shadow_prepassgeometrydepthbuffer)
		R_FreeTexture(r_shadow_prepassgeometrydepthbuffer);
	r_shadow_prepassgeometrydepthbuffer = NULL;

	if (r_shadow_prepassgeometrynormalmaptexture)
		R_FreeTexture(r_shadow_prepassgeometrynormalmaptexture);
	r_shadow_prepassgeometrynormalmaptexture = NULL;

	if (r_shadow_prepasslightingdiffusetexture)
		R_FreeTexture(r_shadow_prepasslightingdiffusetexture);
	r_shadow_prepasslightingdiffusetexture = NULL;

	if (r_shadow_prepasslightingspeculartexture)
		R_FreeTexture(r_shadow_prepasslightingspeculartexture);
	r_shadow_prepasslightingspeculartexture = NULL;
}

void R_Shadow_DrawPrepass(void)
{
	int i;
	int lnum;
	entity_render_t *ent;
	float clearcolor[4];

	R_Mesh_ResetTextureState();
	GL_DepthMask(true);
	GL_ColorMask(1,1,1,1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_Color(1,1,1,1);
	GL_DepthTest(true);
	R_Mesh_SetRenderTargets(r_shadow_prepassgeometryfbo);
	Vector4Set(clearcolor, 0.5f,0.5f,0.5f,1.0f);
	GL_Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, clearcolor, 1.0f, 0);
	if (r_timereport_active)
		R_TimeReport("prepasscleargeom");

	if (cl.csqc_vidvars.drawworld && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawPrepass)
		r_refdef.scene.worldmodel->DrawPrepass(r_refdef.scene.worldentity);
	if (r_timereport_active)
		R_TimeReport("prepassworld");

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (ent->model && ent->model->DrawPrepass != NULL)
			ent->model->DrawPrepass(ent);
	}

	if (r_timereport_active)
		R_TimeReport("prepassmodels");

	GL_DepthMask(false);
	GL_ColorMask(1,1,1,1);
	GL_Color(1,1,1,1);
	GL_DepthTest(true);
	R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo);
	Vector4Set(clearcolor, 0, 0, 0, 0);
	GL_Clear(GL_COLOR_BUFFER_BIT, clearcolor, 1.0f, 0);
	if (r_timereport_active)
		R_TimeReport("prepassclearlit");

	R_Shadow_RenderMode_Begin();

	for (lnum = 0; lnum < r_shadow_scenenumlights; lnum++)
		R_Shadow_DrawLight(r_shadow_scenelightlist[lnum]);

	R_Shadow_RenderMode_End();

	if (r_timereport_active)
		R_TimeReport("prepasslights");
}

#define MAX_SCENELIGHTS 65536
static qbool R_Shadow_PrepareLights_AddSceneLight(rtlight_t *rtlight)
{
	if (r_shadow_scenemaxlights <= r_shadow_scenenumlights)
	{
		if (r_shadow_scenenumlights >= MAX_SCENELIGHTS)
			return false;
		r_shadow_scenemaxlights *= 2;
		r_shadow_scenemaxlights = bound(1024, r_shadow_scenemaxlights, MAX_SCENELIGHTS);
		r_shadow_scenelightlist = (rtlight_t **)Mem_Realloc(r_main_mempool, r_shadow_scenelightlist, r_shadow_scenemaxlights * sizeof(rtlight_t *));
	}
	r_shadow_scenelightlist[r_shadow_scenenumlights++] = rtlight;
	return true;
}

void R_Shadow_DrawLightSprites(void);
void R_Shadow_PrepareLights(void)
{
	int flag;
	int lnum;
	size_t lightindex;
	dlight_t *light;
	size_t range;
	float f;

	int shadowmapborder = bound(1, r_shadow_shadowmapping_bordersize.integer, 16);
	int shadowmaptexturesize = bound(256, r_shadow_shadowmapping_texturesize.integer, (int)vid.maxtexturesize_2d);
	int shadowmapmaxsize = bound(shadowmapborder+2, r_shadow_shadowmapping_maxsize.integer, shadowmaptexturesize / 8);

	if (r_shadow_shadowmode_shadowmapping != r_shadow_shadowmapping.integer ||
		r_shadow_shadowmode_deferred != r_shadow_deferred.integer ||
	    r_shadow_shadowmaptexturesize != shadowmaptexturesize ||
		r_shadow_shadowmapvsdct != (r_shadow_shadowmapping_vsdct.integer != 0 && vid.renderpath == RENDERPATH_GL32) ||
		r_shadow_shadowmapfilterquality != r_shadow_shadowmapping_filterquality.integer ||
		r_shadow_shadowmapshadowsampler != r_shadow_shadowmapping_useshadowsampler.integer ||
		r_shadow_shadowmapdepthbits != r_shadow_shadowmapping_depthbits.integer ||
		r_shadow_shadowmapborder != shadowmapborder ||
		r_shadow_shadowmapmaxsize != shadowmapmaxsize ||
		r_shadow_shadowmapdepthtexture != r_fb.usedepthtextures)
		R_Shadow_FreeShadowMaps();

	r_shadow_usingshadowmaportho = false;

	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
#ifndef USE_GLES2
		if (!r_shadow_deferred.integer || vid.maxdrawbuffers < 2)
		{
			r_shadow_usingdeferredprepass = false;
			if (r_shadow_prepass_width)
				R_Shadow_FreeDeferred();
			r_shadow_prepass_width = r_shadow_prepass_height = 0;
			break;
		}

		if (r_shadow_prepass_width != r_fb.screentexturewidth || r_shadow_prepass_height != r_fb.screentextureheight)
		{
			R_Shadow_FreeDeferred();

			r_shadow_usingdeferredprepass = true;
			r_shadow_prepass_width = r_fb.screentexturewidth;
			r_shadow_prepass_height = r_fb.screentextureheight;
			r_shadow_prepassgeometrydepthbuffer = R_LoadTextureRenderBuffer(r_shadow_texturepool, "prepassgeometrydepthbuffer", r_fb.screentexturewidth, r_fb.screentextureheight, TEXTYPE_DEPTHBUFFER24);
			r_shadow_prepassgeometrynormalmaptexture = R_LoadTexture2D(r_shadow_texturepool, "prepassgeometrynormalmap", r_fb.screentexturewidth, r_fb.screentextureheight, NULL, TEXTYPE_COLORBUFFER32F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, NULL);
			r_shadow_prepasslightingdiffusetexture = R_LoadTexture2D(r_shadow_texturepool, "prepasslightingdiffuse", r_fb.screentexturewidth, r_fb.screentextureheight, NULL, TEXTYPE_COLORBUFFER16F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, NULL);
			r_shadow_prepasslightingspeculartexture = R_LoadTexture2D(r_shadow_texturepool, "prepasslightingspecular", r_fb.screentexturewidth, r_fb.screentextureheight, NULL, TEXTYPE_COLORBUFFER16F, TEXF_RENDERTARGET | TEXF_CLAMP | TEXF_ALPHA | TEXF_FORCENEAREST, -1, NULL);

			// set up the geometry pass fbo (depth + normalmap)
			r_shadow_prepassgeometryfbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepassgeometrynormalmaptexture, NULL, NULL, NULL);
			R_Mesh_SetRenderTargets(r_shadow_prepassgeometryfbo);
			// render depth into a renderbuffer and other important properties into the normalmap texture

			// set up the lighting pass fbo (diffuse + specular)
			r_shadow_prepasslightingdiffusespecularfbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, r_shadow_prepasslightingspeculartexture, NULL, NULL);
			R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusespecularfbo);
			// render diffuse into one texture and specular into another,
			// with depth and normalmap bound as textures,
			// with depth bound as attachment as well

			// set up the lighting pass fbo (diffuse)
			r_shadow_prepasslightingdiffusefbo = R_Mesh_CreateFramebufferObject(r_shadow_prepassgeometrydepthbuffer, r_shadow_prepasslightingdiffusetexture, NULL, NULL, NULL);
			R_Mesh_SetRenderTargets(r_shadow_prepasslightingdiffusefbo);
			// render diffuse into one texture,
			// with depth and normalmap bound as textures,
			// with depth bound as attachment as well
		}
#endif
		break;
	case RENDERPATH_GLES2:
		r_shadow_usingdeferredprepass = false;
		break;
	}

	R_Shadow_EnlargeLeafSurfaceTrisBuffer(r_refdef.scene.worldmodel->brush.num_leafs, r_refdef.scene.worldmodel->num_surfaces, r_refdef.scene.worldmodel->surfmesh.num_triangles, r_refdef.scene.worldmodel->surfmesh.num_triangles);

	r_shadow_scenenumlights = 0;
	flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0; lightindex < range; lightindex++)
	{
		light = (dlight_t *)Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light && (light->flags & flag))
		{
			R_Shadow_PrepareLight(&light->rtlight);
			R_Shadow_PrepareLights_AddSceneLight(&light->rtlight);
		}
	}
	if (r_refdef.scene.rtdlight)
	{
		for (lnum = 0; lnum < r_refdef.scene.numlights; lnum++)
		{
			R_Shadow_PrepareLight(r_refdef.scene.lights[lnum]);
			R_Shadow_PrepareLights_AddSceneLight(r_refdef.scene.lights[lnum]);
		}
	}
	else if (gl_flashblend.integer)
	{
		for (lnum = 0; lnum < r_refdef.scene.numlights; lnum++)
		{
			rtlight_t *rtlight = r_refdef.scene.lights[lnum];
			f = ((rtlight->style >= 0 && rtlight->style < MAX_LIGHTSTYLES) ? r_refdef.scene.lightstylevalue[rtlight->style] : 1) * r_shadow_lightintensityscale.value;
			VectorScale(rtlight->color, f, rtlight->currentcolor);
		}
	}

	// when debugging a single light, we still want to run the prepare, so we only replace the light list afterward...
	if (r_shadow_debuglight.integer >= 0)
	{
		r_shadow_scenenumlights = 0;
		lightindex = r_shadow_debuglight.integer;
		light = (dlight_t *)Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
		{
			R_Shadow_PrepareLight(&light->rtlight);
			R_Shadow_PrepareLights_AddSceneLight(&light->rtlight);
		}
	}

	// if we're doing shadowmaps we need to prepare the atlas layout now
	if (R_Shadow_ShadowMappingEnabled())
	{
		int lod;

		// allocate shadowmaps in the atlas now
		// we may have to make multiple attempts to fit the shadowmaps in the limited space of the atlas, this will appear as lod popping of all shadowmaps whenever it changes, but at least we can still cast shadows from all lights...
		for (lod = 0; lod < 16; lod++)
		{
			int packing_success = 0;
			int packing_failure = 0;
			Mod_AllocLightmap_Reset(&r_shadow_shadowmapatlas_state);
			// we actually have to reserve space for the R_DrawModelShadowMaps if that feature is active, it uses 0,0 so this is easy.
			if (r_shadow_shadowmapatlas_modelshadows_size)
				Mod_AllocLightmap_Block(&r_shadow_shadowmapatlas_state, r_shadow_shadowmapatlas_modelshadows_size, r_shadow_shadowmapatlas_modelshadows_size, &r_shadow_shadowmapatlas_modelshadows_x, &r_shadow_shadowmapatlas_modelshadows_y);
			for (lnum = 0; lnum < r_shadow_scenenumlights; lnum++)
			{
				rtlight_t *rtlight = r_shadow_scenelightlist[lnum];
				int size = rtlight->shadowmapsidesize >> lod;
				int width, height;
				if (!rtlight->castshadows)
					continue;
				size = bound(r_shadow_shadowmapborder, size, r_shadow_shadowmaptexturesize);
				width = size * 2;
				height = size * 3;
				// when there are noselfshadow entities in the light bounds, we have to render two separate sets of shadowmaps :(
				if (rtlight->cached_numshadowentities_noselfshadow)
					width *= 2;
				if (Mod_AllocLightmap_Block(&r_shadow_shadowmapatlas_state, width, height, &rtlight->shadowmapatlasposition[0], &rtlight->shadowmapatlasposition[1]))
				{
					rtlight->shadowmapatlassidesize = size;
					packing_success++;
				}
				else
				{
					// note down that we failed to pack this one, it will have to disable shadows
					rtlight->shadowmapatlassidesize = 0;
					packing_failure++;
				}
			}
			// generally everything fits and we stop here on the first iteration
			if (packing_failure == 0)
				break;
		}
	}

	if (r_editlights.integer)
		R_Shadow_DrawLightSprites();
}

void R_Shadow_DrawShadowMaps(void)
{
	R_Shadow_RenderMode_Begin();
	R_Shadow_RenderMode_ActiveLight(NULL);

	// now that we have a layout of shadowmaps in the atlas, we can render the shadowmaps
	R_Shadow_ClearShadowMapTexture();

	// render model shadowmaps (r_shadows 2) if desired which will be sampled in the forward pass
	if (r_shadow_shadowmapatlas_modelshadows_size)
		R_Shadow_DrawModelShadowMaps();

	if (R_Shadow_ShadowMappingEnabled())
	{
		int lnum;
		for (lnum = 0; lnum < r_shadow_scenenumlights; lnum++)
			R_Shadow_DrawLightShadowMaps(r_shadow_scenelightlist[lnum]);
	}

	R_Shadow_RenderMode_End();
}

void R_Shadow_DrawLights(void)
{
	int lnum;

	R_Shadow_RenderMode_Begin();

	for (lnum = 0; lnum < r_shadow_scenenumlights; lnum++)
		R_Shadow_DrawLight(r_shadow_scenelightlist[lnum]);

	R_Shadow_RenderMode_End();
}

#define MAX_MODELSHADOWS 1024
static int r_shadow_nummodelshadows;
static entity_render_t *r_shadow_modelshadows[MAX_MODELSHADOWS];

void R_Shadow_PrepareModelShadows(void)
{
	int i;
	float scale, size, radius, dot1, dot2;
	prvm_vec3_t prvmshadowdir, prvmshadowfocus;
	vec3_t shadowdir, shadowforward, shadowright, shadoworigin, shadowfocus, shadowmins, shadowmaxs;
	entity_render_t *ent;

	r_shadow_nummodelshadows = 0;
	r_shadow_shadowmapatlas_modelshadows_size = 0;

	if (!r_refdef.scene.numentities || r_refdef.scene.lightmapintensity <= 0.0f || r_shadows.integer <= 0)
		return;

	size = r_shadow_shadowmaptexturesize / 4;
	scale = r_shadow_shadowmapping_precision.value * r_shadows_shadowmapscale.value;
	radius = 0.5f * size / scale;

	Math_atov(r_shadows_throwdirection.string, prvmshadowdir);
	VectorCopy(prvmshadowdir, shadowdir);
	VectorNormalize(shadowdir);
	dot1 = DotProduct(r_refdef.view.forward, shadowdir);
	dot2 = DotProduct(r_refdef.view.up, shadowdir);
	if (fabs(dot1) <= fabs(dot2))
		VectorMA(r_refdef.view.forward, -dot1, shadowdir, shadowforward);
	else
		VectorMA(r_refdef.view.up, -dot2, shadowdir, shadowforward);
	VectorNormalize(shadowforward);
	CrossProduct(shadowdir, shadowforward, shadowright);
	Math_atov(r_shadows_focus.string, prvmshadowfocus);
	VectorCopy(prvmshadowfocus, shadowfocus);
	VectorM(shadowfocus[0], r_refdef.view.right, shadoworigin);
	VectorMA(shadoworigin, shadowfocus[1], r_refdef.view.up, shadoworigin);
	VectorMA(shadoworigin, -shadowfocus[2], r_refdef.view.forward, shadoworigin);
	VectorAdd(shadoworigin, r_refdef.view.origin, shadoworigin);
	if (shadowfocus[0] || shadowfocus[1] || shadowfocus[2])
		dot1 = 1;
	VectorMA(shadoworigin, (1.0f - fabs(dot1)) * radius, shadowforward, shadoworigin);

	shadowmins[0] = shadoworigin[0] - r_shadows_throwdistance.value * fabs(shadowdir[0]) - radius * (fabs(shadowforward[0]) + fabs(shadowright[0]));
	shadowmins[1] = shadoworigin[1] - r_shadows_throwdistance.value * fabs(shadowdir[1]) - radius * (fabs(shadowforward[1]) + fabs(shadowright[1]));
	shadowmins[2] = shadoworigin[2] - r_shadows_throwdistance.value * fabs(shadowdir[2]) - radius * (fabs(shadowforward[2]) + fabs(shadowright[2]));
	shadowmaxs[0] = shadoworigin[0] + r_shadows_throwdistance.value * fabs(shadowdir[0]) + radius * (fabs(shadowforward[0]) + fabs(shadowright[0]));
	shadowmaxs[1] = shadoworigin[1] + r_shadows_throwdistance.value * fabs(shadowdir[1]) + radius * (fabs(shadowforward[1]) + fabs(shadowright[1]));
	shadowmaxs[2] = shadoworigin[2] + r_shadows_throwdistance.value * fabs(shadowdir[2]) + radius * (fabs(shadowforward[2]) + fabs(shadowright[2]));

	for (i = 0; i < r_refdef.scene.numentities; i++)
	{
		ent = r_refdef.scene.entities[i];
		if (!BoxesOverlap(ent->mins, ent->maxs, shadowmins, shadowmaxs))
			continue;
		// cast shadows from anything of the map (submodels are optional)
		if (ent->model && ent->model->DrawShadowMap != NULL && (!ent->model->brush.submodel || r_shadows_castfrombmodels.integer) && (ent->flags & RENDER_SHADOW))
		{
			if (r_shadow_nummodelshadows >= MAX_MODELSHADOWS)
				break;
			r_shadow_modelshadows[r_shadow_nummodelshadows++] = ent;
			R_AnimCache_GetEntity(ent, false, false);
		}
	}

	if (r_shadow_nummodelshadows)
	{
		r_shadow_shadowmapatlas_modelshadows_x = 0;
		r_shadow_shadowmapatlas_modelshadows_y = 0;
		r_shadow_shadowmapatlas_modelshadows_size = size;
	}
}

static void R_Shadow_DrawModelShadowMaps(void)
{
	int i;
	float relativethrowdistance, scale, size, radius, nearclip, farclip, bias, dot1, dot2;
	entity_render_t *ent;
	vec3_t relativelightorigin;
	vec3_t relativelightdirection, relativeforward, relativeright;
	vec3_t relativeshadowmins, relativeshadowmaxs;
	vec3_t shadowdir, shadowforward, shadowright, shadoworigin, shadowfocus;
	prvm_vec3_t prvmshadowdir, prvmshadowfocus;
	float m[12];
	matrix4x4_t shadowmatrix, cameramatrix, mvpmatrix, invmvpmatrix, scalematrix, texmatrix;
	r_viewport_t viewport;

	size = r_shadow_shadowmapatlas_modelshadows_size;
	scale = (r_shadow_shadowmapping_precision.value * r_shadows_shadowmapscale.value) / size;
	radius = 0.5f / scale;
	nearclip = -r_shadows_throwdistance.value;
	farclip = r_shadows_throwdistance.value;
	bias = (r_shadows_shadowmapbias.value < 0) ? r_shadow_shadowmapping_bias.value : r_shadows_shadowmapbias.value * r_shadow_shadowmapping_nearclip.value / (2 * r_shadows_throwdistance.value) * (1024.0f / size);

	// set the parameters that will be used on the regular model renders using these shadows we're about to produce
	r_shadow_modelshadowmap_parameters[0] = size;
	r_shadow_modelshadowmap_parameters[1] = size;
	r_shadow_modelshadowmap_parameters[2] = 1.0;
	r_shadow_modelshadowmap_parameters[3] = bound(0.0f, 1.0f - r_shadows_darken.value, 1.0f);
	r_shadow_modelshadowmap_texturescale[0] = 1.0f / r_shadow_shadowmaptexturesize;
	r_shadow_modelshadowmap_texturescale[1] = 1.0f / r_shadow_shadowmaptexturesize;
	r_shadow_modelshadowmap_texturescale[2] = r_shadow_shadowmapatlas_modelshadows_x;
	r_shadow_modelshadowmap_texturescale[3] = r_shadow_shadowmapatlas_modelshadows_y;
	r_shadow_usingshadowmaportho = true;

	Math_atov(r_shadows_throwdirection.string, prvmshadowdir);
	VectorCopy(prvmshadowdir, shadowdir);
	VectorNormalize(shadowdir);
	Math_atov(r_shadows_focus.string, prvmshadowfocus);
	VectorCopy(prvmshadowfocus, shadowfocus);
	VectorM(shadowfocus[0], r_refdef.view.right, shadoworigin);
	VectorMA(shadoworigin, shadowfocus[1], r_refdef.view.up, shadoworigin);
	VectorMA(shadoworigin, -shadowfocus[2], r_refdef.view.forward, shadoworigin);
	VectorAdd(shadoworigin, r_refdef.view.origin, shadoworigin);
	dot1 = DotProduct(r_refdef.view.forward, shadowdir);
	dot2 = DotProduct(r_refdef.view.up, shadowdir);
	if (fabs(dot1) <= fabs(dot2))
		VectorMA(r_refdef.view.forward, -dot1, shadowdir, shadowforward);
	else
		VectorMA(r_refdef.view.up, -dot2, shadowdir, shadowforward);
	VectorNormalize(shadowforward);
	VectorM(scale, shadowforward, &m[0]);
	if (shadowfocus[0] || shadowfocus[1] || shadowfocus[2])
		dot1 = 1;
	m[3] = fabs(dot1) * 0.5f - DotProduct(shadoworigin, &m[0]);
	CrossProduct(shadowdir, shadowforward, shadowright);
	VectorM(scale, shadowright, &m[4]);
	m[7] = 0.5f - DotProduct(shadoworigin, &m[4]);
	VectorM(1.0f / (farclip - nearclip), shadowdir, &m[8]);
	m[11] = 0.5f - DotProduct(shadoworigin, &m[8]);
	Matrix4x4_FromArray12FloatD3D(&shadowmatrix, m);
	Matrix4x4_Invert_Full(&cameramatrix, &shadowmatrix);
	R_Viewport_InitOrtho(&viewport, &cameramatrix, r_shadow_shadowmapatlas_modelshadows_x, r_shadow_shadowmapatlas_modelshadows_y, r_shadow_shadowmapatlas_modelshadows_size, r_shadow_shadowmapatlas_modelshadows_size, 0, 0, 1, 1, 0, -1, NULL);
	R_SetViewport(&viewport);

	VectorMA(shadoworigin, (1.0f - fabs(dot1)) * radius, shadowforward, shadoworigin);

	// render into a slightly restricted region so that the borders of the
	// shadowmap area fade away, rather than streaking across everything
	// outside the usable area
	GL_Scissor(viewport.x + r_shadow_shadowmapborder, viewport.y + r_shadow_shadowmapborder, viewport.width - 2*r_shadow_shadowmapborder, viewport.height - 2*r_shadow_shadowmapborder);

	for (i = 0;i < r_shadow_nummodelshadows;i++)
	{
		ent = r_shadow_modelshadows[i];
		relativethrowdistance = r_shadows_throwdistance.value * Matrix4x4_ScaleFromMatrix(&ent->inversematrix);
		Matrix4x4_Transform(&ent->inversematrix, shadoworigin, relativelightorigin);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowdir, relativelightdirection);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowforward, relativeforward);
		Matrix4x4_Transform3x3(&ent->inversematrix, shadowright, relativeright);
		relativeshadowmins[0] = relativelightorigin[0] - r_shadows_throwdistance.value * fabs(relativelightdirection[0]) - radius * (fabs(relativeforward[0]) + fabs(relativeright[0]));
		relativeshadowmins[1] = relativelightorigin[1] - r_shadows_throwdistance.value * fabs(relativelightdirection[1]) - radius * (fabs(relativeforward[1]) + fabs(relativeright[1]));
		relativeshadowmins[2] = relativelightorigin[2] - r_shadows_throwdistance.value * fabs(relativelightdirection[2]) - radius * (fabs(relativeforward[2]) + fabs(relativeright[2]));
		relativeshadowmaxs[0] = relativelightorigin[0] + r_shadows_throwdistance.value * fabs(relativelightdirection[0]) + radius * (fabs(relativeforward[0]) + fabs(relativeright[0]));
		relativeshadowmaxs[1] = relativelightorigin[1] + r_shadows_throwdistance.value * fabs(relativelightdirection[1]) + radius * (fabs(relativeforward[1]) + fabs(relativeright[1]));
		relativeshadowmaxs[2] = relativelightorigin[2] + r_shadows_throwdistance.value * fabs(relativelightdirection[2]) + radius * (fabs(relativeforward[2]) + fabs(relativeright[2]));
		RSurf_ActiveModelEntity(ent, false, false, false);
		ent->model->DrawShadowMap(0, ent, relativelightorigin, relativelightdirection, relativethrowdistance, ent->model->submodelsurfaces_end - ent->model->submodelsurfaces_start, ent->model->modelsurfaces_sorted + ent->model->submodelsurfaces_start, NULL, relativeshadowmins, relativeshadowmaxs);
		rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
	}

#if 0
	if (r_test.integer)
	{
		unsigned char *rawpixels = Z_Malloc(viewport.width*viewport.height*4);
		CHECKGLERROR
		qglReadPixels(viewport.x, viewport.y, viewport.width, viewport.height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, rawpixels);
		CHECKGLERROR
		Image_WriteTGABGRA("r_shadows_2.tga", viewport.width, viewport.height, rawpixels);
		Cvar_SetValueQuick(&r_test, 0);
		Z_Free(rawpixels);
	}
#endif

	Matrix4x4_Concat(&mvpmatrix, &r_refdef.view.viewport.projectmatrix, &r_refdef.view.viewport.viewmatrix);
	Matrix4x4_Invert_Full(&invmvpmatrix, &mvpmatrix);
	Matrix4x4_CreateScale3(&scalematrix, size, -size, 1);
	Matrix4x4_AdjustOrigin(&scalematrix, 0, size, -0.5f * bias);
	Matrix4x4_Concat(&texmatrix, &scalematrix, &shadowmatrix);
	Matrix4x4_Concat(&r_shadow_shadowmapmatrix, &texmatrix, &invmvpmatrix);
}

static void R_BeginCoronaQuery(rtlight_t *rtlight, float scale, qbool usequery)
{
	float zdist;
	vec3_t centerorigin;
#ifndef USE_GLES2
	float vertex3f[12];
#endif
	// if it's too close, skip it
	if (VectorLength(rtlight->currentcolor) < (1.0f / 256.0f))
		return;
	zdist = (DotProduct(rtlight->shadoworigin, r_refdef.view.forward) - DotProduct(r_refdef.view.origin, r_refdef.view.forward));
	if (zdist < 32)
 		return;
	if (usequery && r_numqueries + 2 <= r_maxqueries)
	{
		rtlight->corona_queryindex_allpixels = r_queries[r_numqueries++];
		rtlight->corona_queryindex_visiblepixels = r_queries[r_numqueries++];
		// we count potential samples in the middle of the screen, we count actual samples at the light location, this allows counting potential samples of off-screen lights
		VectorMA(r_refdef.view.origin, zdist, r_refdef.view.forward, centerorigin);

		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
#ifndef USE_GLES2
			CHECKGLERROR
			// NOTE: GL_DEPTH_TEST must be enabled or ATI won't count samples, so use GL_DepthFunc instead
			qglBeginQuery(GL_SAMPLES_PASSED, rtlight->corona_queryindex_allpixels);
			GL_DepthFunc(GL_ALWAYS);
			R_CalcSprite_Vertex3f(vertex3f, centerorigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
			R_Mesh_PrepareVertices_Vertex3f(4, vertex3f, NULL, 0);
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
			qglEndQuery(GL_SAMPLES_PASSED);
			GL_DepthFunc(GL_LEQUAL);
			qglBeginQuery(GL_SAMPLES_PASSED, rtlight->corona_queryindex_visiblepixels);
			R_CalcSprite_Vertex3f(vertex3f, rtlight->shadoworigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
			R_Mesh_PrepareVertices_Vertex3f(4, vertex3f, NULL, 0);
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
			qglEndQuery(GL_SAMPLES_PASSED);
			CHECKGLERROR
#endif
			break;
		}
	}
	rtlight->corona_visibility = bound(0, (zdist - 32) / 32, 1);
}

static float spritetexcoord2f[4*2] = {0, 1, 0, 0, 1, 0, 1, 1};

static void R_DrawCorona(rtlight_t *rtlight, float cscale, float scale)
{
	vec3_t color;
	unsigned int occlude = 0;

	// now we have to check the query result
	if (rtlight->corona_queryindex_visiblepixels)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
#ifndef USE_GLES2
			// store the pixel counts into a uniform buffer for the shader to
			// use - we'll never know the results on the cpu without
			// synchronizing and we don't want that
#define BUFFER_OFFSET(i)    ((GLint *)((unsigned char*)NULL + (i)))
			if (!r_shadow_occlusion_buf) {
				qglGenBuffers(1, &r_shadow_occlusion_buf);
				qglBindBuffer(GL_QUERY_BUFFER, r_shadow_occlusion_buf);
				qglBufferData(GL_QUERY_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
			} else {
				qglBindBuffer(GL_QUERY_BUFFER, r_shadow_occlusion_buf);
			}
			qglGetQueryObjectiv(rtlight->corona_queryindex_visiblepixels, GL_QUERY_RESULT, BUFFER_OFFSET(0));
			qglGetQueryObjectiv(rtlight->corona_queryindex_allpixels, GL_QUERY_RESULT, BUFFER_OFFSET(4));
			qglBindBufferBase(GL_UNIFORM_BUFFER, 0, r_shadow_occlusion_buf);
			occlude = MATERIALFLAG_OCCLUDE;
			cscale *= rtlight->corona_visibility;
			CHECKGLERROR
			break;
#else
			return;
#endif
		}
	}
	else
	{
		if (CL_Cache_TraceLineSurfaces(r_refdef.view.origin, rtlight->shadoworigin, MOVE_NORMAL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT).fraction < 1)
			return;
	}
	VectorScale(rtlight->currentcolor, cscale, color);
	if (VectorLength(color) > (1.0f / 256.0f))
	{
		float vertex3f[12];
		qbool negated = (color[0] + color[1] + color[2] < 0);
		if(negated)
		{
			VectorNegate(color, color);
			GL_BlendEquationSubtract(true);
		}
		R_CalcSprite_Vertex3f(vertex3f, rtlight->shadoworigin, r_refdef.view.right, r_refdef.view.up, scale, -scale, -scale, scale);
		RSurf_ActiveCustomEntity(&identitymatrix, &identitymatrix, RENDER_NODEPTHTEST, 0, color[0], color[1], color[2], 1, 4, vertex3f, spritetexcoord2f, NULL, NULL, NULL, NULL, 2, polygonelement3i, polygonelement3s, false, false);
		R_DrawCustomSurface(r_shadow_lightcorona, &identitymatrix, MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE | MATERIALFLAG_NODEPTHTEST | occlude, 0, 4, 0, 2, false, false, false);
		if(negated)
			GL_BlendEquationSubtract(false);
	}
}

void R_Shadow_DrawCoronas(void)
{
	int i, flag;
	qbool usequery = false;
	size_t lightindex;
	dlight_t *light;
	rtlight_t *rtlight;
	size_t range;
	if (r_coronas.value < (1.0f / 256.0f) && !gl_flashblend.integer)
		return;
	if (r_fb.water.renderingscene)
		return;
	flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
	R_EntityMatrix(&identitymatrix);

	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked

	// check occlusion of coronas, using occlusion queries or raytraces
	r_numqueries = 0;
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		// buffer binding target GL_QUERY_BUFFER: Core since version 4.4
		usequery = r_coronas_occlusionquery.integer && vid.support.glversion >= 44;
#ifndef USE_GLES2
		if (usequery)
		{
			GL_ColorMask(0,0,0,0);
			if (r_maxqueries < ((unsigned int)range + r_refdef.scene.numlights) * 2)
			if (r_maxqueries < MAX_OCCLUSION_QUERIES)
			{
				i = r_maxqueries;
				r_maxqueries = ((unsigned int)range + r_refdef.scene.numlights) * 4;
				r_maxqueries = min(r_maxqueries, MAX_OCCLUSION_QUERIES);
				CHECKGLERROR
				qglGenQueries(r_maxqueries - i, r_queries + i);
				CHECKGLERROR
			}
			RSurf_ActiveModelEntity(r_refdef.scene.worldentity, false, false, false);
			GL_BlendFunc(GL_ONE, GL_ZERO);
			GL_CullFace(GL_NONE);
			GL_DepthMask(false);
			GL_DepthRange(0, 1);
			GL_PolygonOffset(0, 0);
			GL_DepthTest(true);
			R_Mesh_ResetTextureState();
			R_SetupShader_Generic_NoTexture(false, false);
		}
#endif
		break;
	}
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		rtlight = &light->rtlight;
		rtlight->corona_visibility = 0;
		rtlight->corona_queryindex_visiblepixels = 0;
		rtlight->corona_queryindex_allpixels = 0;
		if (!(rtlight->flags & flag))
			continue;
		if (rtlight->corona <= 0)
			continue;
		if (r_shadow_debuglight.integer >= 0 && r_shadow_debuglight.integer != (int)lightindex)
			continue;
		R_BeginCoronaQuery(rtlight, rtlight->radius * rtlight->coronasizescale * r_coronas_occlusionsizescale.value, usequery);
	}
	for (i = 0;i < r_refdef.scene.numlights;i++)
	{
		rtlight = r_refdef.scene.lights[i];
		rtlight->corona_visibility = 0;
		rtlight->corona_queryindex_visiblepixels = 0;
		rtlight->corona_queryindex_allpixels = 0;
		if (!(rtlight->flags & flag))
			continue;
		if (rtlight->corona <= 0)
			continue;
		R_BeginCoronaQuery(rtlight, rtlight->radius * rtlight->coronasizescale * r_coronas_occlusionsizescale.value, usequery);
	}
	if (usequery)
		GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);

	// now draw the coronas using the query data for intensity info
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		rtlight = &light->rtlight;
		if (rtlight->corona_visibility <= 0)
			continue;
		R_DrawCorona(rtlight, rtlight->corona * r_coronas.value * 0.25f, rtlight->radius * rtlight->coronasizescale);
	}
	for (i = 0;i < r_refdef.scene.numlights;i++)
	{
		rtlight = r_refdef.scene.lights[i];
		if (rtlight->corona_visibility <= 0)
			continue;
		R_DrawCorona(rtlight, rtlight->corona * r_coronas.value * 0.25f, rtlight->radius * rtlight->coronasizescale);
	}
}



static dlight_t *R_Shadow_NewWorldLight(void)
{
	return (dlight_t *)Mem_ExpandableArray_AllocRecord(&r_shadow_worldlightsarray);
}

static void R_Shadow_UpdateWorldLight(dlight_t *light, vec3_t origin, vec3_t angles, vec3_t color, vec_t radius, vec_t corona, int style, int shadowenable, const char *cubemapname, vec_t coronasizescale, vec_t ambientscale, vec_t diffusescale, vec_t specularscale, int flags)
{
	matrix4x4_t matrix;

	// note that style is no longer validated here, -1 is used for unstyled lights and >= MAX_LIGHTSTYLES is accepted for sake of editing rtlights files that might be out of bounds but perfectly formatted

	// validate parameters
	if (!cubemapname)
		cubemapname = "";

	// copy to light properties
	VectorCopy(origin, light->origin);
	light->angles[0] = angles[0] - 360 * floor(angles[0] / 360);
	light->angles[1] = angles[1] - 360 * floor(angles[1] / 360);
	light->angles[2] = angles[2] - 360 * floor(angles[2] / 360);
	/*
	light->color[0] = max(color[0], 0);
	light->color[1] = max(color[1], 0);
	light->color[2] = max(color[2], 0);
	*/
	light->color[0] = color[0];
	light->color[1] = color[1];
	light->color[2] = color[2];
	light->radius = max(radius, 0);
	light->style = style;
	light->shadow = shadowenable;
	light->corona = corona;
	dp_strlcpy(light->cubemapname, cubemapname, sizeof(light->cubemapname));
	light->coronasizescale = coronasizescale;
	light->ambientscale = ambientscale;
	light->diffusescale = diffusescale;
	light->specularscale = specularscale;
	light->flags = flags;

	// update renderable light data
	Matrix4x4_CreateFromQuakeEntity(&matrix, light->origin[0], light->origin[1], light->origin[2], light->angles[0], light->angles[1], light->angles[2], light->radius);
	R_RTLight_Update(&light->rtlight, true, &matrix, light->color, light->style, light->cubemapname[0] ? light->cubemapname : NULL, light->shadow, light->corona, light->coronasizescale, light->ambientscale, light->diffusescale, light->specularscale, light->flags);
}

static void R_Shadow_FreeWorldLight(dlight_t *light)
{
	if (r_shadow_selectedlight == light)
		r_shadow_selectedlight = NULL;
	R_RTLight_Uncompile(&light->rtlight);
	Mem_ExpandableArray_FreeRecord(&r_shadow_worldlightsarray, light);
}

void R_Shadow_ClearWorldLights(void)
{
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
			R_Shadow_FreeWorldLight(light);
	}
	r_shadow_selectedlight = NULL;
}

static void R_Shadow_SelectLight(dlight_t *light)
{
	if (r_shadow_selectedlight)
		r_shadow_selectedlight->selected = false;
	r_shadow_selectedlight = light;
	if (r_shadow_selectedlight)
		r_shadow_selectedlight->selected = true;
}

static void R_Shadow_DrawCursor_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	// this is never batched (there can be only one)
	float vertex3f[12];
	R_CalcSprite_Vertex3f(vertex3f, r_editlights_cursorlocation, r_refdef.view.right, r_refdef.view.up, EDLIGHTSPRSIZE, -EDLIGHTSPRSIZE, -EDLIGHTSPRSIZE, EDLIGHTSPRSIZE);
	RSurf_ActiveCustomEntity(&identitymatrix, &identitymatrix, 0, 0, 1, 1, 1, 1, 4, vertex3f, spritetexcoord2f, NULL, NULL, NULL, NULL, 2, polygonelement3i, polygonelement3s, false, false);
	R_DrawCustomSurface(r_editlights_sprcursor, &identitymatrix, MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false, false);
}

static void R_Shadow_DrawLightSprite_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	float intensity;
	float s;
	vec3_t spritecolor;
	skinframe_t *skinframe;
	float vertex3f[12];

	// this is never batched (due to the ent parameter changing every time)
	// so numsurfaces == 1 and surfacelist[0] == lightnumber
	const dlight_t *light = (dlight_t *)ent;
	s = EDLIGHTSPRSIZE;

	R_CalcSprite_Vertex3f(vertex3f, light->origin, r_refdef.view.right, r_refdef.view.up, s, -s, -s, s);

	intensity = 0.5f;
	VectorScale(light->color, intensity, spritecolor);
	if (VectorLength(spritecolor) < 0.1732f)
		VectorSet(spritecolor, 0.1f, 0.1f, 0.1f);
	if (VectorLength(spritecolor) > 1.0f)
		VectorNormalize(spritecolor);

	// draw light sprite
	if (light->cubemapname[0] && !light->shadow)
		skinframe = r_editlights_sprcubemapnoshadowlight;
	else if (light->cubemapname[0])
		skinframe = r_editlights_sprcubemaplight;
	else if (!light->shadow)
		skinframe = r_editlights_sprnoshadowlight;
	else
		skinframe = r_editlights_sprlight;

	RSurf_ActiveCustomEntity(&identitymatrix, &identitymatrix, 0, 0, spritecolor[0], spritecolor[1], spritecolor[2], 1, 4, vertex3f, spritetexcoord2f, NULL, NULL, NULL, NULL, 2, polygonelement3i, polygonelement3s, false, false);
	R_DrawCustomSurface(skinframe, &identitymatrix, MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false, false);

	// draw selection sprite if light is selected
	if (light->selected)
	{
		RSurf_ActiveCustomEntity(&identitymatrix, &identitymatrix, 0, 0, 1, 1, 1, 1, 4, vertex3f, spritetexcoord2f, NULL, NULL, NULL, NULL, 2, polygonelement3i, polygonelement3s, false, false);
		R_DrawCustomSurface(r_editlights_sprselection, &identitymatrix, MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_FULLBRIGHT | MATERIALFLAG_NOCULLFACE, 0, 4, 0, 2, false, false, false);
		// VorteX todo: add normalmode/realtime mode light overlay sprites?
	}
}

void R_Shadow_DrawLightSprites(void)
{
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (light)
			R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, light->origin, R_Shadow_DrawLightSprite_TransparentCallback, (entity_render_t *)light, 5, &light->rtlight);
	}
	if (!r_editlights_lockcursor)
		R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, r_editlights_cursorlocation, R_Shadow_DrawCursor_TransparentCallback, NULL, 0, NULL);
}

int R_Shadow_GetRTLightInfo(unsigned int lightindex, float *origin, float *radius, float *color)
{
	unsigned int range;
	dlight_t *light;
	rtlight_t *rtlight;
	range = (unsigned int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray);
	if (lightindex >= range)
		return -1;
	light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
	if (!light)
		return 0;
	rtlight = &light->rtlight;
	//if (!(rtlight->flags & flag))
	//	return 0;
	VectorCopy(rtlight->shadoworigin, origin);
	*radius = rtlight->radius;
	VectorCopy(rtlight->color, color);
	return 1;
}

static void R_Shadow_SelectLightInView(void)
{
	float bestrating, rating, temp[3];
	dlight_t *best;
	size_t lightindex;
	dlight_t *light;
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	best = NULL;
	bestrating = 0;

	if (r_editlights_lockcursor)
		return;
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		VectorSubtract(light->origin, r_refdef.view.origin, temp);
		rating = (DotProduct(temp, r_refdef.view.forward) / sqrt(DotProduct(temp, temp)));
		if (rating >= 0.95)
		{
			rating /= (1 + 0.0625f * sqrt(DotProduct(temp, temp)));
			if (bestrating < rating && CL_TraceLine(light->origin, r_refdef.view.origin, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true).fraction == 1.0f)
			{
				bestrating = rating;
				best = light;
			}
		}
	}
	R_Shadow_SelectLight(best);
}

void R_Shadow_LoadWorldLights(void)
{
	int n, a, style, shadow, flags;
	char tempchar, *lightsstring, *s, *t, name[MAX_QPATH], cubemapname[MAX_QPATH];
	float origin[3], radius, color[3], angles[3], corona, coronasizescale, ambientscale, diffusescale, specularscale;
	if (cl.worldmodel == NULL)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.rtlights", cl.worldnamenoextension);
	lightsstring = (char *)FS_LoadFile(name, tempmempool, false, NULL);
	if (lightsstring)
	{
		s = lightsstring;
		n = 0;
		while (*s)
		{
			/*
			t = s;
			shadow = true;
			for (;COM_Parse(t, true) && strcmp(
			if (COM_Parse(t, true))
			{
				if (com_token[0] == '!')
				{
					shadow = false;
					origin[0] = atof(com_token+1);
				}
				else
					origin[0] = atof(com_token);
				if (Com_Parse(t
			}
			*/
			t = s;
			while (*s && *s != '\n' && *s != '\r')
				s++;
			if (!*s)
				break;
			tempchar = *s;
			shadow = true;
			// check for modifier flags
			if (*t == '!')
			{
				shadow = false;
				t++;
			}
			*s = 0;
#if _MSC_VER >= 1400
#define sscanf sscanf_s
#endif
			cubemapname[sizeof(cubemapname)-1] = 0;
#if MAX_QPATH != 128
#error update this code if MAX_QPATH changes
#endif
			a = sscanf(t, "%f %f %f %f %f %f %f %d %127s %f %f %f %f %f %f %f %f %i", &origin[0], &origin[1], &origin[2], &radius, &color[0], &color[1], &color[2], &style, cubemapname
#if _MSC_VER >= 1400
, (unsigned int)sizeof(cubemapname)
#endif
, &corona, &angles[0], &angles[1], &angles[2], &coronasizescale, &ambientscale, &diffusescale, &specularscale, &flags);
			*s = tempchar;
			if (a < 18)
				flags = LIGHTFLAG_REALTIMEMODE;
			if (a < 17)
				specularscale = 1;
			if (a < 16)
				diffusescale = 1;
			if (a < 15)
				ambientscale = 0;
			if (a < 14)
				coronasizescale = 0.25f;
			if (a < 13)
				VectorClear(angles);
			if (a < 10)
				corona = 0;
			if (a < 9 || !strcmp(cubemapname, "\"\""))
				cubemapname[0] = 0;
			// remove quotes on cubemapname
			if (cubemapname[0] == '"' && cubemapname[strlen(cubemapname) - 1] == '"')
			{
				size_t namelen;
				namelen = strlen(cubemapname) - 2;
				memmove(cubemapname, cubemapname + 1, namelen);
				cubemapname[namelen] = '\0';
			}
			if (a < 8)
			{
				Con_Printf("found %d parameters on line %i, should be 8 or more parameters (origin[0] origin[1] origin[2] radius color[0] color[1] color[2] style \"cubemapname\" corona angles[0] angles[1] angles[2] coronasizescale ambientscale diffusescale specularscale flags)\n", a, n + 1);
				break;
			}
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, angles, color, radius, corona, style, shadow, cubemapname, coronasizescale, ambientscale, diffusescale, specularscale, flags);
			if (*s == '\r')
				s++;
			if (*s == '\n')
				s++;
			n++;
		}
		if (*s)
			Con_Printf("invalid rtlights file \"%s\"\n", name);
		Mem_Free(lightsstring);
	}
}

void R_Shadow_SaveWorldLights(void)
{
	size_t lightindex;
	dlight_t *light;
	size_t bufchars, bufmaxchars;
	char *buf, *oldbuf;
	char name[MAX_QPATH];
	char line[MAX_INPUTLINE];
	size_t range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked, assuming the dpsnprintf mess doesn't screw it up...
	// I hate lines which are 3 times my screen size :( --blub
	if (!range)
		return;
	if (cl.worldmodel == NULL)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.rtlights", cl.worldnamenoextension);
	bufchars = bufmaxchars = 0;
	buf = NULL;
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		if (light->coronasizescale != 0.25f || light->ambientscale != 0 || light->diffusescale != 1 || light->specularscale != 1 || light->flags != LIGHTFLAG_REALTIMEMODE)
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d \"%s\" %f %f %f %f %f %f %f %f %i\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style, light->cubemapname, light->corona, light->angles[0], light->angles[1], light->angles[2], light->coronasizescale, light->ambientscale, light->diffusescale, light->specularscale, light->flags);
		else if (light->cubemapname[0] || light->corona || light->angles[0] || light->angles[1] || light->angles[2])
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d \"%s\" %f %f %f %f\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style, light->cubemapname, light->corona, light->angles[0], light->angles[1], light->angles[2]);
		else
			dpsnprintf(line, sizeof(line), "%s%f %f %f %f %f %f %f %d\n", light->shadow ? "" : "!", light->origin[0], light->origin[1], light->origin[2], light->radius, light->color[0], light->color[1], light->color[2], light->style);
		if (bufchars + strlen(line) > bufmaxchars)
		{
			bufmaxchars = bufchars + strlen(line) + 2048;
			oldbuf = buf;
			buf = (char *)Mem_Alloc(tempmempool, bufmaxchars);
			if (oldbuf)
			{
				if (bufchars)
					memcpy(buf, oldbuf, bufchars);
				Mem_Free(oldbuf);
			}
		}
		if (strlen(line))
		{
			memcpy(buf + bufchars, line, strlen(line));
			bufchars += strlen(line);
		}
	}
	if (bufchars)
		FS_WriteFile(name, buf, (fs_offset_t)bufchars);
	if (buf)
		Mem_Free(buf);
}

void R_Shadow_LoadLightsFile(void)
{
	int n, a, style;
	char tempchar, *lightsstring, *s, *t, name[MAX_QPATH];
	float origin[3], radius, color[3], subtract, spotdir[3], spotcone, falloff, distbias;
	if (cl.worldmodel == NULL)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	dpsnprintf(name, sizeof(name), "%s.lights", cl.worldnamenoextension);
	lightsstring = (char *)FS_LoadFile(name, tempmempool, false, NULL);
	if (lightsstring)
	{
		s = lightsstring;
		n = 0;
		while (*s)
		{
			t = s;
			while (*s && *s != '\n' && *s != '\r')
				s++;
			if (!*s)
				break;
			tempchar = *s;
			*s = 0;
			a = sscanf(t, "%f %f %f %f %f %f %f %f %f %f %f %f %f %d", &origin[0], &origin[1], &origin[2], &falloff, &color[0], &color[1], &color[2], &subtract, &spotdir[0], &spotdir[1], &spotdir[2], &spotcone, &distbias, &style);
			*s = tempchar;
			if (a < 14)
			{
				Con_Printf("invalid lights file, found %d parameters on line %i, should be 14 parameters (origin[0] origin[1] origin[2] falloff light[0] light[1] light[2] subtract spotdir[0] spotdir[1] spotdir[2] spotcone distancebias style)\n", a, n + 1);
				break;
			}
			radius = sqrt(DotProduct(color, color) / (falloff * falloff * 8192.0f * 8192.0f));
			radius = bound(15, radius, 4096);
			VectorScale(color, (2.0f / (8388608.0f)), color);
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, vec3_origin, color, radius, 0, style, true, NULL, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
			if (*s == '\r')
				s++;
			if (*s == '\n')
				s++;
			n++;
		}
		if (*s)
			Con_Printf("invalid lights file \"%s\"\n", name);
		Mem_Free(lightsstring);
	}
}

// tyrlite/hmap2 light types in the delay field
typedef enum lighttype_e {LIGHTTYPE_MINUSX, LIGHTTYPE_RECIPX, LIGHTTYPE_RECIPXX, LIGHTTYPE_NONE, LIGHTTYPE_SUN, LIGHTTYPE_MINUSXX} lighttype_t;

void R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite(void)
{
	int entnum;
	int style;
	int islight;
	int skin;
	int pflags;
	//int effects;
	int type;
	int n;
	char *entfiledata;
	const char *data;
	float origin[3], angles[3], radius, color[3], light[4], fadescale, lightscale, originhack[3], overridecolor[3], vec[4];
	char key[256], value[MAX_INPUTLINE];
	char vabuf[1024];

	if (cl.worldmodel == NULL)
	{
		Con_Print("No map loaded.\n");
		return;
	}
	// try to load a .ent file first
	dpsnprintf(key, sizeof(key), "%s.ent", cl.worldnamenoextension);
	data = entfiledata = (char *)FS_LoadFile(key, tempmempool, true, NULL);
	// and if that is not found, fall back to the bsp file entity string
	if (!data)
		data = cl.worldmodel->brush.entities;
	if (!data)
		return;
	for (entnum = 0;COM_ParseToken_Simple(&data, false, false, true) && com_token[0] == '{';entnum++)
	{
		type = LIGHTTYPE_MINUSX;
		origin[0] = origin[1] = origin[2] = 0;
		originhack[0] = originhack[1] = originhack[2] = 0;
		angles[0] = angles[1] = angles[2] = 0;
		color[0] = color[1] = color[2] = 1;
		light[0] = light[1] = light[2] = 1;light[3] = 300;
		overridecolor[0] = overridecolor[1] = overridecolor[2] = 1;
		fadescale = 1;
		lightscale = 1;
		style = 0;
		skin = 0;
		pflags = 0;
		//effects = 0;
		islight = false;
		while (1)
		{
			if (!COM_ParseToken_Simple(&data, false, false, true))
				break; // error
			if (com_token[0] == '}')
				break; // end of entity
			if (com_token[0] == '_')
				dp_strlcpy(key, com_token + 1, sizeof(key));
			else
				dp_strlcpy(key, com_token, sizeof(key));
			while (key[strlen(key)-1] == ' ') // remove trailing spaces
				key[strlen(key)-1] = 0;
			if (!COM_ParseToken_Simple(&data, false, false, true))
				break; // error
			dp_strlcpy(value, com_token, sizeof(value));

			// now that we have the key pair worked out...
			if (!strcmp("light", key))
			{
				n = sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
				if (n == 1)
				{
					// quake
					light[0] = vec[0] * (1.0f / 256.0f);
					light[1] = vec[0] * (1.0f / 256.0f);
					light[2] = vec[0] * (1.0f / 256.0f);
					light[3] = vec[0];
				}
				else if (n == 4)
				{
					// halflife
					light[0] = vec[0] * (1.0f / 255.0f);
					light[1] = vec[1] * (1.0f / 255.0f);
					light[2] = vec[2] * (1.0f / 255.0f);
					light[3] = vec[3];
				}
			}
			else if (!strcmp("delay", key))
				type = atoi(value);
			else if (!strcmp("origin", key))
				sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
			else if (!strcmp("angle", key))
				angles[0] = 0, angles[1] = atof(value), angles[2] = 0;
			else if (!strcmp("angles", key))
				sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
			else if (!strcmp("color", key))
				sscanf(value, "%f %f %f", &color[0], &color[1], &color[2]);
			else if (!strcmp("wait", key))
				fadescale = atof(value);
			else if (!strcmp("classname", key))
			{
				if (!strncmp(value, "light", 5))
				{
					islight = true;
					if (!strcmp(value, "light_fluoro"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 1;
						overridecolor[2] = 1;
					}
					if (!strcmp(value, "light_fluorospark"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 1;
						overridecolor[2] = 1;
					}
					if (!strcmp(value, "light_globe"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.8;
						overridecolor[2] = 0.4;
					}
					if (!strcmp(value, "light_flame_large_yellow"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_flame_small_yellow"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_torch_small_white"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
					if (!strcmp(value, "light_torch_small_walltorch"))
					{
						originhack[0] = 0;
						originhack[1] = 0;
						originhack[2] = 0;
						overridecolor[0] = 1;
						overridecolor[1] = 0.5;
						overridecolor[2] = 0.1;
					}
				}
			}
			else if (!strcmp("style", key))
				style = atoi(value);
			else if (!strcmp("skin", key))
				skin = (int)atof(value);
			else if (!strcmp("pflags", key))
				pflags = (int)atof(value);
			//else if (!strcmp("effects", key))
			//	effects = (int)atof(value);
			else if (cl.worldmodel->type == mod_brushq3)
			{
				if (!strcmp("scale", key))
					lightscale = atof(value);
				if (!strcmp("fade", key))
					fadescale = atof(value);
			}
		}
		if (!islight)
			continue;
		if (lightscale <= 0)
			lightscale = 1;
		if (fadescale <= 0)
			fadescale = 1;
		if (color[0] == color[1] && color[0] == color[2])
		{
			color[0] *= overridecolor[0];
			color[1] *= overridecolor[1];
			color[2] *= overridecolor[2];
		}
		radius = light[3] * r_editlights_quakelightsizescale.value * lightscale / fadescale;
		color[0] = color[0] * light[0];
		color[1] = color[1] * light[1];
		color[2] = color[2] * light[2];
		switch (type)
		{
		case LIGHTTYPE_MINUSX:
			break;
		case LIGHTTYPE_RECIPX:
			radius *= 2;
			VectorScale(color, (1.0f / 16.0f), color);
			break;
		case LIGHTTYPE_RECIPXX:
			radius *= 2;
			VectorScale(color, (1.0f / 16.0f), color);
			break;
		default:
		case LIGHTTYPE_NONE:
			break;
		case LIGHTTYPE_SUN:
			break;
		case LIGHTTYPE_MINUSXX:
			break;
		}
		VectorAdd(origin, originhack, origin);
		if (radius >= 1)
			R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), origin, angles, color, radius, (pflags & PFLAGS_CORONA) != 0, style, (pflags & PFLAGS_NOSHADOW) == 0, skin >= 16 ? va(vabuf, sizeof(vabuf), "cubemaps/%i", skin) : NULL, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
	}
	if (entfiledata)
		Mem_Free(entfiledata);
}


static void R_Shadow_SetCursorLocationForView(void)
{
	vec_t dist, push;
	vec3_t dest, endpos;
	trace_t trace;
	VectorMA(r_refdef.view.origin, r_editlights_cursordistance.value, r_refdef.view.forward, dest);
	trace = CL_TraceLine(r_refdef.view.origin, dest, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true);
	if (trace.fraction < 1)
	{
		dist = trace.fraction * r_editlights_cursordistance.value;
		push = r_editlights_cursorpushback.value;
		if (push > dist)
			push = dist;
		push = -push;
		VectorMA(trace.endpos, push, r_refdef.view.forward, endpos);
		VectorMA(endpos, r_editlights_cursorpushoff.value, trace.plane.normal, endpos);
	}
	else
	{
		VectorClear( endpos );
	}
	r_editlights_cursorlocation[0] = floor(endpos[0] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
	r_editlights_cursorlocation[1] = floor(endpos[1] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
	r_editlights_cursorlocation[2] = floor(endpos[2] / r_editlights_cursorgrid.value + 0.5f) * r_editlights_cursorgrid.value;
}

void R_Shadow_UpdateWorldLightSelection(void)
{
	if (r_editlights.integer)
	{
		R_Shadow_SetCursorLocationForView();
		R_Shadow_SelectLightInView();
	}
	else
		R_Shadow_SelectLight(NULL);
}

static void R_Shadow_EditLights_Clear_f(cmd_state_t *cmd)
{
	R_Shadow_ClearWorldLights();
}

void R_Shadow_EditLights_Reload_f(cmd_state_t *cmd)
{
	if (!cl.worldmodel)
		return;
	dp_strlcpy(r_shadow_mapname, cl.worldname, sizeof(r_shadow_mapname));
	R_Shadow_ClearWorldLights();
	if (r_shadow_realtime_world_importlightentitiesfrommap.integer <= 1)
	{
		R_Shadow_LoadWorldLights();
		if (!Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray))
			R_Shadow_LoadLightsFile();
	}
	if (r_shadow_realtime_world_importlightentitiesfrommap.integer >= 1)
	{
		if (!Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray))
			R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite();
	}
}

static void R_Shadow_EditLights_Save_f(cmd_state_t *cmd)
{
	if (!cl.worldmodel)
		return;
	R_Shadow_SaveWorldLights();
}

static void R_Shadow_EditLights_ImportLightEntitiesFromMap_f(cmd_state_t *cmd)
{
	R_Shadow_ClearWorldLights();
	R_Shadow_LoadWorldLightsFromMap_LightArghliteTyrlite();
}

static void R_Shadow_EditLights_ImportLightsFile_f(cmd_state_t *cmd)
{
	R_Shadow_ClearWorldLights();
	R_Shadow_LoadLightsFile();
}

static void R_Shadow_EditLights_Spawn_f(cmd_state_t *cmd)
{
	vec3_t color;
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (Cmd_Argc(cmd) != 1)
	{
		Con_Print("r_editlights_spawn does not take parameters\n");
		return;
	}
	color[0] = color[1] = color[2] = 1;
	R_Shadow_UpdateWorldLight(R_Shadow_NewWorldLight(), r_editlights_cursorlocation, vec3_origin, color, 200, 0, 0, true, NULL, 0.25, 0, 1, 1, LIGHTFLAG_REALTIMEMODE);
}

static void R_Shadow_EditLights_Edit_f(cmd_state_t *cmd)
{
	vec3_t origin, angles, color;
	vec_t radius, corona, coronasizescale, ambientscale, diffusescale, specularscale;
	int style, shadows, flags, normalmode, realtimemode;
	char cubemapname[MAX_INPUTLINE];
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	VectorCopy(r_shadow_selectedlight->origin, origin);
	VectorCopy(r_shadow_selectedlight->angles, angles);
	VectorCopy(r_shadow_selectedlight->color, color);
	radius = r_shadow_selectedlight->radius;
	style = r_shadow_selectedlight->style;
	if (*r_shadow_selectedlight->cubemapname)
		dp_strlcpy(cubemapname, r_shadow_selectedlight->cubemapname, sizeof(cubemapname));
	else
		cubemapname[0] = 0;
	shadows = r_shadow_selectedlight->shadow;
	corona = r_shadow_selectedlight->corona;
	coronasizescale = r_shadow_selectedlight->coronasizescale;
	ambientscale = r_shadow_selectedlight->ambientscale;
	diffusescale = r_shadow_selectedlight->diffusescale;
	specularscale = r_shadow_selectedlight->specularscale;
	flags = r_shadow_selectedlight->flags;
	normalmode = (flags & LIGHTFLAG_NORMALMODE) != 0;
	realtimemode = (flags & LIGHTFLAG_REALTIMEMODE) != 0;
	if (!strcmp(Cmd_Argv(cmd, 1), "origin"))
	{
		if (Cmd_Argc(cmd) != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[0] = atof(Cmd_Argv(cmd, 2));
		origin[1] = atof(Cmd_Argv(cmd, 3));
		origin[2] = atof(Cmd_Argv(cmd, 4));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "originscale"))
	{
		if (Cmd_Argc(cmd) != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[0] *= atof(Cmd_Argv(cmd, 2));
		origin[1] *= atof(Cmd_Argv(cmd, 3));
		origin[2] *= atof(Cmd_Argv(cmd, 4));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "originx"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[0] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "originy"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[1] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "originz"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[2] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "move"))
	{
		if (Cmd_Argc(cmd) != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[0] += atof(Cmd_Argv(cmd, 2));
		origin[1] += atof(Cmd_Argv(cmd, 3));
		origin[2] += atof(Cmd_Argv(cmd, 4));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "movex"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[0] += atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "movey"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[1] += atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "movez"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		origin[2] += atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "angles"))
	{
		if (Cmd_Argc(cmd) != 5)
		{
			Con_Printf("usage: r_editlights_edit %s x y z\n", Cmd_Argv(cmd, 1));
			return;
		}
		angles[0] = atof(Cmd_Argv(cmd, 2));
		angles[1] = atof(Cmd_Argv(cmd, 3));
		angles[2] = atof(Cmd_Argv(cmd, 4));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "anglesx"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		angles[0] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "anglesy"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		angles[1] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "anglesz"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		angles[2] = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "color"))
	{
		if (Cmd_Argc(cmd) != 5)
		{
			Con_Printf("usage: r_editlights_edit %s red green blue\n", Cmd_Argv(cmd, 1));
			return;
		}
		color[0] = atof(Cmd_Argv(cmd, 2));
		color[1] = atof(Cmd_Argv(cmd, 3));
		color[2] = atof(Cmd_Argv(cmd, 4));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "radius"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		radius = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "colorscale"))
	{
		if (Cmd_Argc(cmd) == 3)
		{
			double scale = atof(Cmd_Argv(cmd, 2));
			color[0] *= scale;
			color[1] *= scale;
			color[2] *= scale;
		}
		else
		{
			if (Cmd_Argc(cmd) != 5)
			{
				Con_Printf("usage: r_editlights_edit %s red green blue  (OR grey instead of red green blue)\n", Cmd_Argv(cmd, 1));
				return;
			}
			color[0] *= atof(Cmd_Argv(cmd, 2));
			color[1] *= atof(Cmd_Argv(cmd, 3));
			color[2] *= atof(Cmd_Argv(cmd, 4));
		}
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "radiusscale") || !strcmp(Cmd_Argv(cmd, 1), "sizescale"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		radius *= atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "style"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		style = atoi(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "cubemap"))
	{
		if (Cmd_Argc(cmd) > 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		if (Cmd_Argc(cmd) == 3)
			dp_strlcpy(cubemapname, Cmd_Argv(cmd, 2), sizeof(cubemapname));
		else
			cubemapname[0] = 0;
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "shadows"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		shadows = Cmd_Argv(cmd, 2)[0] == 'y' || Cmd_Argv(cmd, 2)[0] == 'Y' || Cmd_Argv(cmd, 2)[0] == 't' || atoi(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "corona"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		corona = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "coronasize"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		coronasizescale = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "ambient"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		ambientscale = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "diffuse"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		diffusescale = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "specular"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		specularscale = atof(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "normalmode"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		normalmode = Cmd_Argv(cmd, 2)[0] == 'y' || Cmd_Argv(cmd, 2)[0] == 'Y' || Cmd_Argv(cmd, 2)[0] == 't' || atoi(Cmd_Argv(cmd, 2));
	}
	else if (!strcmp(Cmd_Argv(cmd, 1), "realtimemode"))
	{
		if (Cmd_Argc(cmd) != 3)
		{
			Con_Printf("usage: r_editlights_edit %s value\n", Cmd_Argv(cmd, 1));
			return;
		}
		realtimemode = Cmd_Argv(cmd, 2)[0] == 'y' || Cmd_Argv(cmd, 2)[0] == 'Y' || Cmd_Argv(cmd, 2)[0] == 't' || atoi(Cmd_Argv(cmd, 2));
	}
	else
	{
		Con_Print("usage: r_editlights_edit [property] [value]\n");
		Con_Print("Selected light's properties:\n");
		Con_Printf("Origin       : %f %f %f\n", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);
		Con_Printf("Angles       : %f %f %f\n", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);
		Con_Printf("Color        : %f %f %f\n", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);
		Con_Printf("Radius       : %f\n", r_shadow_selectedlight->radius);
		Con_Printf("Corona       : %f\n", r_shadow_selectedlight->corona);
		Con_Printf("Style        : %i\n", r_shadow_selectedlight->style);
		Con_Printf("Shadows      : %s\n", r_shadow_selectedlight->shadow ? "yes" : "no");
		Con_Printf("Cubemap      : %s\n", r_shadow_selectedlight->cubemapname);
		Con_Printf("CoronaSize   : %f\n", r_shadow_selectedlight->coronasizescale);
		Con_Printf("Ambient      : %f\n", r_shadow_selectedlight->ambientscale);
		Con_Printf("Diffuse      : %f\n", r_shadow_selectedlight->diffusescale);
		Con_Printf("Specular     : %f\n", r_shadow_selectedlight->specularscale);
		Con_Printf("NormalMode   : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? "yes" : "no");
		Con_Printf("RealTimeMode : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? "yes" : "no");
		return;
	}
	flags = (normalmode ? LIGHTFLAG_NORMALMODE : 0) | (realtimemode ? LIGHTFLAG_REALTIMEMODE : 0);
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, origin, angles, color, radius, corona, style, shadows, cubemapname, coronasizescale, ambientscale, diffusescale, specularscale, flags);
}

static void R_Shadow_EditLights_EditAll_f(cmd_state_t *cmd)
{
	size_t lightindex;
	dlight_t *light, *oldselected;
	size_t range;

	if (!r_editlights.integer)
	{
		Con_Print("Cannot edit lights when not in editing mode. Set r_editlights to 1.\n");
		return;
	}

	oldselected = r_shadow_selectedlight;
	// EditLights doesn't seem to have a "remove" command or something so:
	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		R_Shadow_SelectLight(light);
		R_Shadow_EditLights_Edit_f(cmd_local);
	}
	// return to old selected (to not mess editing once selection is locked)
	R_Shadow_SelectLight(oldselected);
}

void R_Shadow_EditLights_DrawSelectedLightProperties(void)
{
	int lightnumber, lightcount;
	size_t lightindex, range;
	dlight_t *light;
	char temp[256];
	float x, y;

	if (!r_editlights.integer)
		return;

	// update cvars so QC can query them
	if (r_shadow_selectedlight)
	{
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);
		Cvar_SetQuick(&r_editlights_current_origin, temp);
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);
		Cvar_SetQuick(&r_editlights_current_angles, temp);
		dpsnprintf(temp, sizeof(temp), "%f %f %f", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);
		Cvar_SetQuick(&r_editlights_current_color, temp);
		Cvar_SetValueQuick(&r_editlights_current_radius, r_shadow_selectedlight->radius);
		Cvar_SetValueQuick(&r_editlights_current_corona, r_shadow_selectedlight->corona);
		Cvar_SetValueQuick(&r_editlights_current_coronasize, r_shadow_selectedlight->coronasizescale);
		Cvar_SetValueQuick(&r_editlights_current_style, r_shadow_selectedlight->style);
		Cvar_SetValueQuick(&r_editlights_current_shadows, r_shadow_selectedlight->shadow);
		Cvar_SetQuick(&r_editlights_current_cubemap, r_shadow_selectedlight->cubemapname);
		Cvar_SetValueQuick(&r_editlights_current_ambient, r_shadow_selectedlight->ambientscale);
		Cvar_SetValueQuick(&r_editlights_current_diffuse, r_shadow_selectedlight->diffusescale);
		Cvar_SetValueQuick(&r_editlights_current_specular, r_shadow_selectedlight->specularscale);
		Cvar_SetValueQuick(&r_editlights_current_normalmode, (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? 1 : 0);
		Cvar_SetValueQuick(&r_editlights_current_realtimemode, (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? 1 : 0);
	}

	// draw properties on screen
	if (!r_editlights_drawproperties.integer)
		return;
	x = vid_conwidth.value - 320;
	y = 5;
	DrawQ_Pic(x-5, y-5, NULL, 250, 243, 0, 0, 0, 0.75, 0);
	lightnumber = -1;
	lightcount = 0;
	range = Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray); // checked
	for (lightindex = 0;lightindex < range;lightindex++)
	{
		light = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, lightindex);
		if (!light)
			continue;
		if (light == r_shadow_selectedlight)
			lightnumber = (int)lightindex;
		lightcount++;
	}
	dpsnprintf(temp, sizeof(temp), "Cursor origin: %.0f %.0f %.0f", r_editlights_cursorlocation[0], r_editlights_cursorlocation[1], r_editlights_cursorlocation[2]); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, false, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Total lights : %i active (%i total)", lightcount, (int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray)); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, false, FONT_DEFAULT);y += 8;
	y += 8;
	if (r_shadow_selectedlight == NULL)
		return;
	dpsnprintf(temp, sizeof(temp), "Light #%i properties:", lightnumber);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Origin       : %.0f %.0f %.0f\n", r_shadow_selectedlight->origin[0], r_shadow_selectedlight->origin[1], r_shadow_selectedlight->origin[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Angles       : %.0f %.0f %.0f\n", r_shadow_selectedlight->angles[0], r_shadow_selectedlight->angles[1], r_shadow_selectedlight->angles[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Color        : %.2f %.2f %.2f\n", r_shadow_selectedlight->color[0], r_shadow_selectedlight->color[1], r_shadow_selectedlight->color[2]);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Radius       : %.0f\n", r_shadow_selectedlight->radius);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Corona       : %.0f\n", r_shadow_selectedlight->corona);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Style        : %i\n", r_shadow_selectedlight->style);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Shadows      : %s\n", r_shadow_selectedlight->shadow ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Cubemap      : %s\n", r_shadow_selectedlight->cubemapname);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "CoronaSize   : %.2f\n", r_shadow_selectedlight->coronasizescale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Ambient      : %.2f\n", r_shadow_selectedlight->ambientscale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Diffuse      : %.2f\n", r_shadow_selectedlight->diffusescale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "Specular     : %.2f\n", r_shadow_selectedlight->specularscale);DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "NormalMode   : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_NORMALMODE) ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	dpsnprintf(temp, sizeof(temp), "RealTimeMode : %s\n", (r_shadow_selectedlight->flags & LIGHTFLAG_REALTIMEMODE) ? "yes" : "no");DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT);y += 8;
	y += 8;
	dpsnprintf(temp, sizeof(temp), "Render stats\n"); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "Current color: %.3f %.3f %.3f\n", r_shadow_selectedlight->rtlight.currentcolor[0], r_shadow_selectedlight->rtlight.currentcolor[1], r_shadow_selectedlight->rtlight.currentcolor[2]); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "Shadow size  : %ix%ix6\n", r_shadow_selectedlight->rtlight.shadowmapatlassidesize, r_shadow_selectedlight->rtlight.shadowmapatlassidesize); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "World surfs  : %i\n", r_shadow_selectedlight->rtlight.cached_numsurfaces); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "Shadow ents  : %i + %i noself\n", r_shadow_selectedlight->rtlight.cached_numshadowentities, r_shadow_selectedlight->rtlight.cached_numshadowentities_noselfshadow); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "Lit ents     : %i + %i noself\n", r_shadow_selectedlight->rtlight.cached_numlightentities, r_shadow_selectedlight->rtlight.cached_numlightentities_noselfshadow); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "BG photons   : %.3f\n", r_shadow_selectedlight->rtlight.bouncegrid_photons); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "BG radius    : %.0f\n", r_shadow_selectedlight->rtlight.bouncegrid_effectiveradius); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "BG color     : %.3f %.3f %.3f\n", r_shadow_selectedlight->rtlight.bouncegrid_photoncolor[0], r_shadow_selectedlight->rtlight.bouncegrid_photoncolor[1], r_shadow_selectedlight->rtlight.bouncegrid_photoncolor[2]); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
	dpsnprintf(temp, sizeof(temp), "BG stats     : %i traces %i hits\n", r_shadow_selectedlight->rtlight.bouncegrid_traces, r_shadow_selectedlight->rtlight.bouncegrid_hits); DrawQ_String(x, y, temp, 0, 8, 8, 1, 1, 1, 1, 0, NULL, true, FONT_DEFAULT); y += 8;
}

static void R_Shadow_EditLights_ToggleShadow_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_selectedlight->angles, r_shadow_selectedlight->color, r_shadow_selectedlight->radius, r_shadow_selectedlight->corona, r_shadow_selectedlight->style, !r_shadow_selectedlight->shadow, r_shadow_selectedlight->cubemapname, r_shadow_selectedlight->coronasizescale, r_shadow_selectedlight->ambientscale, r_shadow_selectedlight->diffusescale, r_shadow_selectedlight->specularscale, r_shadow_selectedlight->flags);
}

static void R_Shadow_EditLights_ToggleCorona_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot spawn light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_selectedlight->angles, r_shadow_selectedlight->color, r_shadow_selectedlight->radius, !r_shadow_selectedlight->corona, r_shadow_selectedlight->style, r_shadow_selectedlight->shadow, r_shadow_selectedlight->cubemapname, r_shadow_selectedlight->coronasizescale, r_shadow_selectedlight->ambientscale, r_shadow_selectedlight->diffusescale, r_shadow_selectedlight->specularscale, r_shadow_selectedlight->flags);
}

static void R_Shadow_EditLights_Remove_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot remove light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_FreeWorldLight(r_shadow_selectedlight);
	r_shadow_selectedlight = NULL;
}

static void R_Shadow_EditLights_Help_f(cmd_state_t *cmd)
{
	Con_Print(
"Documentation on r_editlights system:\n"
"Settings:\n"
"r_editlights : enable/disable editing mode\n"
"r_editlights_cursordistance : maximum distance of cursor from eye\n"
"r_editlights_cursorpushback : push back cursor this far from surface\n"
"r_editlights_cursorpushoff : push cursor off surface this far\n"
"r_editlights_cursorgrid : snap cursor to grid of this size\n"
"r_editlights_quakelightsizescale : imported quake light entity size scaling\n"
"Commands:\n"
"r_editlights_help : this help\n"
"r_editlights_clear : remove all lights\n"
"r_editlights_reload : reload .rtlights, .lights file, or entities\n"
"r_editlights_lock : lock selection to current light, if already locked - unlock\n"
"r_editlights_save : save to .rtlights file\n"
"r_editlights_spawn : create a light with default settings\n"
"r_editlights_edit command : edit selected light - more documentation below\n"
"r_editlights_remove : remove selected light\n"
"r_editlights_toggleshadow : toggles on/off selected light's shadow property\n"
"r_editlights_importlightentitiesfrommap : reload light entities\n"
"r_editlights_importlightsfile : reload .light file (produced by hlight)\n"
"Edit commands:\n"
"origin x y z : set light location\n"
"originx x: set x component of light location\n"
"originy y: set y component of light location\n"
"originz z: set z component of light location\n"
"move x y z : adjust light location\n"
"movex x: adjust x component of light location\n"
"movey y: adjust y component of light location\n"
"movez z: adjust z component of light location\n"
"angles x y z : set light angles\n"
"anglesx x: set x component of light angles\n"
"anglesy y: set y component of light angles\n"
"anglesz z: set z component of light angles\n"
"color r g b : set color of light (can be brighter than 1 1 1)\n"
"radius radius : set radius (size) of light\n"
"colorscale grey : multiply color of light (1 does nothing)\n"
"colorscale r g b : multiply color of light (1 1 1 does nothing)\n"
"radiusscale scale : multiply radius (size) of light (1 does nothing)\n"
"sizescale scale : multiply radius (size) of light (1 does nothing)\n"
"originscale x y z : multiply origin of light (1 1 1 does nothing)\n"
"style style : set lightstyle of light (flickering patterns, switches, etc)\n"
"cubemap basename : set filter cubemap of light\n"
"shadows 1/0 : turn on/off shadows\n"
"corona n : set corona intensity\n"
"coronasize n : set corona size (0-1)\n"
"ambient n : set ambient intensity (0-1)\n"
"diffuse n : set diffuse intensity (0-1)\n"
"specular n : set specular intensity (0-1)\n"
"normalmode 1/0 : turn on/off rendering of this light in rtworld 0 mode\n"
"realtimemode 1/0 : turn on/off rendering of this light in rtworld 1 mode\n"
"<nothing> : print light properties to console\n"
	);
}

static void R_Shadow_EditLights_CopyInfo_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot copy light info when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	VectorCopy(r_shadow_selectedlight->angles, r_shadow_bufferlight.angles);
	VectorCopy(r_shadow_selectedlight->color, r_shadow_bufferlight.color);
	r_shadow_bufferlight.radius = r_shadow_selectedlight->radius;
	r_shadow_bufferlight.style = r_shadow_selectedlight->style;
	if (*r_shadow_selectedlight->cubemapname)
		dp_strlcpy(r_shadow_bufferlight.cubemapname, r_shadow_selectedlight->cubemapname, sizeof(r_shadow_bufferlight.cubemapname));
	else
		r_shadow_bufferlight.cubemapname[0] = 0;
	r_shadow_bufferlight.shadow = r_shadow_selectedlight->shadow;
	r_shadow_bufferlight.corona = r_shadow_selectedlight->corona;
	r_shadow_bufferlight.coronasizescale = r_shadow_selectedlight->coronasizescale;
	r_shadow_bufferlight.ambientscale = r_shadow_selectedlight->ambientscale;
	r_shadow_bufferlight.diffusescale = r_shadow_selectedlight->diffusescale;
	r_shadow_bufferlight.specularscale = r_shadow_selectedlight->specularscale;
	r_shadow_bufferlight.flags = r_shadow_selectedlight->flags;
}

static void R_Shadow_EditLights_PasteInfo_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot paste light info when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light.\n");
		return;
	}
	R_Shadow_UpdateWorldLight(r_shadow_selectedlight, r_shadow_selectedlight->origin, r_shadow_bufferlight.angles, r_shadow_bufferlight.color, r_shadow_bufferlight.radius, r_shadow_bufferlight.corona, r_shadow_bufferlight.style, r_shadow_bufferlight.shadow, r_shadow_bufferlight.cubemapname, r_shadow_bufferlight.coronasizescale, r_shadow_bufferlight.ambientscale, r_shadow_bufferlight.diffusescale, r_shadow_bufferlight.specularscale, r_shadow_bufferlight.flags);
}

static void R_Shadow_EditLights_Lock_f(cmd_state_t *cmd)
{
	if (!r_editlights.integer)
	{
		Con_Print("Cannot lock on light when not in editing mode.  Set r_editlights to 1.\n");
		return;
	}
	if (r_editlights_lockcursor)
	{
		r_editlights_lockcursor = false;
		return;
	}
	if (!r_shadow_selectedlight)
	{
		Con_Print("No selected light to lock on.\n");
		return;
	}
	r_editlights_lockcursor = true;
}

static void R_Shadow_EditLights_Init(void)
{
	Cvar_RegisterVariable(&r_editlights);
	Cvar_RegisterVariable(&r_editlights_cursordistance);
	Cvar_RegisterVariable(&r_editlights_cursorpushback);
	Cvar_RegisterVariable(&r_editlights_cursorpushoff);
	Cvar_RegisterVariable(&r_editlights_cursorgrid);
	Cvar_RegisterVariable(&r_editlights_quakelightsizescale);
	Cvar_RegisterVariable(&r_editlights_drawproperties);
	Cvar_RegisterVariable(&r_editlights_current_origin);
	Cvar_RegisterVariable(&r_editlights_current_angles);
	Cvar_RegisterVariable(&r_editlights_current_color);
	Cvar_RegisterVariable(&r_editlights_current_radius);
	Cvar_RegisterVariable(&r_editlights_current_corona);
	Cvar_RegisterVariable(&r_editlights_current_coronasize);
	Cvar_RegisterVariable(&r_editlights_current_style);
	Cvar_RegisterVariable(&r_editlights_current_shadows);
	Cvar_RegisterVariable(&r_editlights_current_cubemap);
	Cvar_RegisterVariable(&r_editlights_current_ambient);
	Cvar_RegisterVariable(&r_editlights_current_diffuse);
	Cvar_RegisterVariable(&r_editlights_current_specular);
	Cvar_RegisterVariable(&r_editlights_current_normalmode);
	Cvar_RegisterVariable(&r_editlights_current_realtimemode);
	Cmd_AddCommand(CF_CLIENT, "r_editlights_help", R_Shadow_EditLights_Help_f, "prints documentation on console commands and variables in rtlight editing system");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_clear", R_Shadow_EditLights_Clear_f, "removes all world lights (let there be darkness!)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_reload", R_Shadow_EditLights_Reload_f, "reloads rtlights file (or imports from .lights file or .ent file or the map itself)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_save", R_Shadow_EditLights_Save_f, "save .rtlights file for current level");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_spawn", R_Shadow_EditLights_Spawn_f, "creates a light with default properties (let there be light!)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_edit", R_Shadow_EditLights_Edit_f, "changes a property on the selected light");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_editall", R_Shadow_EditLights_EditAll_f, "changes a property on ALL lights at once (tip: use radiusscale and colorscale to alter these properties)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_remove", R_Shadow_EditLights_Remove_f, "remove selected light");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_toggleshadow", R_Shadow_EditLights_ToggleShadow_f, "toggle on/off the shadow option on the selected light");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_togglecorona", R_Shadow_EditLights_ToggleCorona_f, "toggle on/off the corona option on the selected light");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_importlightentitiesfrommap", R_Shadow_EditLights_ImportLightEntitiesFromMap_f, "load lights from .ent file or map entities (ignoring .rtlights or .lights file)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_importlightsfile", R_Shadow_EditLights_ImportLightsFile_f, "load lights from .lights file (ignoring .rtlights or .ent files and map entities)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_copyinfo", R_Shadow_EditLights_CopyInfo_f, "store a copy of all properties (except origin) of the selected light");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_pasteinfo", R_Shadow_EditLights_PasteInfo_f, "apply the stored properties onto the selected light (making it exactly identical except for origin)");
	Cmd_AddCommand(CF_CLIENT, "r_editlights_lock", R_Shadow_EditLights_Lock_f, "lock selection to current light, if already locked - unlock");
}



/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

void R_CompleteLightPoint(float *ambient, float *diffuse, float *lightdir, const vec3_t p, const int flags, float lightmapintensity, float ambientintensity)
{
	int i, numlights, flag, q;
	rtlight_t *light;
	dlight_t *dlight;
	float relativepoint[3];
	float color[3];
	float dist;
	float dist2;
	float intensity;
	float sa[3], sx[3], sy[3], sz[3], sd[3];
	float lightradius2;

	// use first order spherical harmonics to combine directional lights
	for (q = 0; q < 3; q++)
		sa[q] = sx[q] = sy[q] = sz[q] = sd[q] = 0;

	if (flags & LP_LIGHTMAP)
	{
		if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->lit && r_refdef.scene.worldmodel->brush.LightPoint)
		{
			float tempambient[3];
			for (q = 0; q < 3; q++)
				tempambient[q] = color[q] = relativepoint[q] = 0;
			r_refdef.scene.worldmodel->brush.LightPoint(r_refdef.scene.worldmodel, p, tempambient, color, relativepoint);
			// calculate a weighted average light direction as well
			intensity = VectorLength(color);
			for (q = 0; q < 3; q++)
			{
				sa[q] += (0.5f * color[q] + tempambient[q]) * lightmapintensity;
				sx[q] += (relativepoint[0] * color[q]) * lightmapintensity;
				sy[q] += (relativepoint[1] * color[q]) * lightmapintensity;
				sz[q] += (relativepoint[2] * color[q]) * lightmapintensity;
				sd[q] += (intensity * relativepoint[q]) * lightmapintensity;
			}
		}
		else
		{
			// unlit map - fullbright but scaled by lightmapintensity
			for (q = 0; q < 3; q++)
				sa[q] += lightmapintensity;
		}
	}

	if (flags & LP_RTWORLD)
	{
		flag = r_refdef.scene.rtworld ? LIGHTFLAG_REALTIMEMODE : LIGHTFLAG_NORMALMODE;
		numlights = (int)Mem_ExpandableArray_IndexRange(&r_shadow_worldlightsarray);
		for (i = 0; i < numlights; i++)
		{
			dlight = (dlight_t *) Mem_ExpandableArray_RecordAtIndex(&r_shadow_worldlightsarray, i);
			if (!dlight)
				continue;
			light = &dlight->rtlight;
			if (!(light->flags & flag))
				continue;
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = sqrt(dist2) / light->radius;
			intensity = min(1.0f, (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist)) * r_shadow_lightintensityscale.value;
			if (intensity <= 0.0f)
				continue;
			if (light->shadow && CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true).fraction < 1)
				continue;
			for (q = 0; q < 3; q++)
				color[q] = light->currentcolor[q] * intensity;
			intensity = VectorLength(color);
			VectorNormalize(relativepoint);
			for (q = 0; q < 3; q++)
			{
				sa[q] += 0.5f * color[q];
				sx[q] += relativepoint[0] * color[q];
				sy[q] += relativepoint[1] * color[q];
				sz[q] += relativepoint[2] * color[q];
				sd[q] += intensity * relativepoint[q];
			}
		}
		// FIXME: sample bouncegrid too!
	}

	if (flags & LP_DYNLIGHT)
	{
		// sample dlights
		for (i = 0;i < r_refdef.scene.numlights;i++)
		{
			light = r_refdef.scene.lights[i];
			// sample
			lightradius2 = light->radius * light->radius;
			VectorSubtract(light->shadoworigin, p, relativepoint);
			dist2 = VectorLength2(relativepoint);
			if (dist2 >= lightradius2)
				continue;
			dist = sqrt(dist2) / light->radius;
			intensity = (1.0f - dist) * r_shadow_lightattenuationlinearscale.value / (r_shadow_lightattenuationdividebias.value + dist*dist) * r_shadow_lightintensityscale.value;
			if (intensity <= 0.0f)
				continue;
			if (light->shadow && CL_TraceLine(p, light->shadoworigin, MOVE_NOMONSTERS, NULL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT, collision_extendmovelength.value, true, false, NULL, false, true).fraction < 1)
				continue;
			for (q = 0; q < 3; q++)
				color[q] = light->currentcolor[q] * intensity;
			intensity = VectorLength(color);
			VectorNormalize(relativepoint);
			for (q = 0; q < 3; q++)
			{
				sa[q] += 0.5f * color[q];
				sx[q] += relativepoint[0] * color[q];
				sy[q] += relativepoint[1] * color[q];
				sz[q] += relativepoint[2] * color[q];
				sd[q] += intensity * relativepoint[q];
			}
		}
	}

	// calculate the weighted-average light direction (bentnormal)
	for (q = 0; q < 3; q++)
		lightdir[q] = sd[q];
	VectorNormalize(lightdir);
	for (q = 0; q < 3; q++)
	{
		// extract the diffuse color along the chosen direction and scale it
		diffuse[q] = (lightdir[0] * sx[q] + lightdir[1] * sy[q] + lightdir[2] * sz[q]);
		// subtract some of diffuse from ambient
		ambient[q] = sa[q] + -0.333f * diffuse[q] + ambientintensity;
	}
}
