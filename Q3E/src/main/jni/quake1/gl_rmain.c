/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_main.c

#include "quakedef.h"
#include "r_shadow.h"
#include "polygon.h"
#include "image.h"
#include "ft2.h"
#include "csprogs.h"
#include "cl_video.h"
#include "cl_collision.h"

#ifdef WIN32
// Enable NVIDIA High Performance Graphics while using Integrated Graphics.
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
#ifdef __cplusplus
}
#endif
#endif

mempool_t *r_main_mempool;
rtexturepool_t *r_main_texturepool;

int r_textureframe = 0; ///< used only by R_GetCurrentTexture, incremented per view and per UI render

static qbool r_loadnormalmap;
static qbool r_loadgloss;
qbool r_loadfog;
static qbool r_loaddds;
static qbool r_savedds;
static qbool r_gpuskeletal;

//
// screen size info
//
r_refdef_t r_refdef;

cvar_t r_motionblur = {CF_CLIENT | CF_ARCHIVE, "r_motionblur", "0", "screen motionblur - value represents intensity, somewhere around 0.5 recommended - NOTE: bad performance on multi-gpu!"};
cvar_t r_damageblur = {CF_CLIENT | CF_ARCHIVE, "r_damageblur", "0", "screen motionblur based on damage - value represents intensity, somewhere around 0.5 recommended - NOTE: bad performance on multi-gpu!"};
cvar_t r_motionblur_averaging = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_averaging", "0.1", "sliding average reaction time for velocity (higher = slower adaption to change)"};
cvar_t r_motionblur_randomize = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_randomize", "0.1", "randomizing coefficient to workaround ghosting"};
cvar_t r_motionblur_minblur = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_minblur", "0.5", "factor of blur to apply at all times (always have this amount of blur no matter what the other factors are)"};
cvar_t r_motionblur_maxblur = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_maxblur", "0.9", "maxmimum amount of blur"};
cvar_t r_motionblur_velocityfactor = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_velocityfactor", "1", "factoring in of player velocity to the blur equation - the faster the player moves around the map, the more blur they get"};
cvar_t r_motionblur_velocityfactor_minspeed = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_velocityfactor_minspeed", "400", "lower value of velocity when it starts to factor into blur equation"};
cvar_t r_motionblur_velocityfactor_maxspeed = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_velocityfactor_maxspeed", "800", "upper value of velocity when it reaches the peak factor into blur equation"};
cvar_t r_motionblur_mousefactor = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_mousefactor", "2", "factoring in of mouse acceleration to the blur equation - the faster the player turns their mouse, the more blur they get"};
cvar_t r_motionblur_mousefactor_minspeed = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_mousefactor_minspeed", "0", "lower value of mouse acceleration when it starts to factor into blur equation"};
cvar_t r_motionblur_mousefactor_maxspeed = {CF_CLIENT | CF_ARCHIVE, "r_motionblur_mousefactor_maxspeed", "50", "upper value of mouse acceleration when it reaches the peak factor into blur equation"};

cvar_t r_depthfirst = {CF_CLIENT | CF_ARCHIVE, "r_depthfirst", "0", "renders a depth-only version of the scene before normal rendering begins to eliminate overdraw, values: 0 = off, 1 = world depth, 2 = world and model depth"};
cvar_t r_useinfinitefarclip = {CF_CLIENT | CF_ARCHIVE, "r_useinfinitefarclip", "1", "enables use of a special kind of projection matrix that has an extremely large farclip"};
cvar_t r_farclip_base = {CF_CLIENT, "r_farclip_base", "65536", "farclip (furthest visible distance) for rendering when r_useinfinitefarclip is 0"};
cvar_t r_farclip_world = {CF_CLIENT, "r_farclip_world", "2", "adds map size to farclip multiplied by this value"};
cvar_t r_nearclip = {CF_CLIENT, "r_nearclip", "1", "distance from camera of nearclip plane" };
cvar_t r_deformvertexes = {CF_CLIENT, "r_deformvertexes", "1", "allows use of deformvertexes in shader files (can be turned off to check performance impact)"};
cvar_t r_transparent = {CF_CLIENT, "r_transparent", "1", "allows use of transparent surfaces (can be turned off to check performance impact)"};
cvar_t r_transparent_alphatocoverage = {CF_CLIENT, "r_transparent_alphatocoverage", "1", "enables GL_ALPHA_TO_COVERAGE antialiasing technique on alphablend and alphatest surfaces when using vid_samples 2 or higher"};
cvar_t r_transparent_sortsurfacesbynearest = {CF_CLIENT, "r_transparent_sortsurfacesbynearest", "1", "sort entity and world surfaces by nearest point on bounding box instead of using the center of the bounding box, usually reduces sorting artifacts"};
cvar_t r_transparent_useplanardistance = {CF_CLIENT, "r_transparent_useplanardistance", "0", "sort transparent meshes by distance from view plane rather than spherical distance to the chosen point"};
cvar_t r_showoverdraw = {CF_CLIENT, "r_showoverdraw", "0", "shows overlapping geometry"};
cvar_t r_showbboxes = {CF_CLIENT, "r_showbboxes", "0", "shows bounding boxes of server entities, value controls opacity scaling (1 = 10%,  10 = 100%)"};
cvar_t r_showbboxes_client = {CF_CLIENT, "r_showbboxes_client", "0", "shows bounding boxes of clientside qc entities, value controls opacity scaling (1 = 10%,  10 = 100%)"};
cvar_t r_showsurfaces = {CF_CLIENT, "r_showsurfaces", "0", "1 shows surfaces as different colors, or a value of 3 shows an approximation to vertex or object color (for a very approximate view of the game)"};
cvar_t r_showtris = {CF_CLIENT, "r_showtris", "0", "shows triangle outlines, value controls brightness (can be above 1)"};
cvar_t r_shownormals = {CF_CLIENT, "r_shownormals", "0", "shows per-vertex surface normals and tangent vectors for bumpmapped lighting"};
cvar_t r_showlighting = {CF_CLIENT, "r_showlighting", "0", "shows areas lit by lights, useful for finding out why some areas of a map render slowly (bright orange = lots of passes = slow), a value of 2 disables depth testing which can be interesting but not very useful"};
cvar_t r_showcollisionbrushes = {CF_CLIENT, "r_showcollisionbrushes", "0", "draws collision brushes in quake3 maps (mode 1), mode 2 disables rendering of world (trippy!)"};
cvar_t r_showcollisionbrushes_polygonfactor = {CF_CLIENT, "r_showcollisionbrushes_polygonfactor", "-1", "expands outward the brush polygons a little bit, used to make collision brushes appear infront of walls"};
cvar_t r_showcollisionbrushes_polygonoffset = {CF_CLIENT, "r_showcollisionbrushes_polygonoffset", "0", "nudges brush polygon depth in hardware depth units, used to make collision brushes appear infront of walls"};
cvar_t r_showdisabledepthtest = {CF_CLIENT, "r_showdisabledepthtest", "0", "disables depth testing on r_show* cvars, allowing you to see what hidden geometry the graphics card is processing"};
cvar_t r_showspriteedges = {CF_CLIENT, "r_showspriteedges", "0", "renders a debug outline to show the polygon shape of each sprite frame rendered (may be 2 or more in case of interpolated animations), for debugging rendering bugs with specific view types"};
cvar_t r_showparticleedges = {CF_CLIENT, "r_showparticleedges", "0", "renders a debug outline to show the polygon shape of each particle, for debugging rendering bugs with specific view types"};
cvar_t r_drawportals = {CF_CLIENT, "r_drawportals", "0", "shows portals (separating polygons) in world interior in quake1 maps"};
cvar_t r_drawentities = {CF_CLIENT, "r_drawentities","1", "draw entities (doors, players, projectiles, etc)"};
cvar_t r_draw2d = {CF_CLIENT, "r_draw2d","1", "draw 2D stuff (dangerous to turn off)"};
cvar_t r_drawworld = {CF_CLIENT, "r_drawworld","1", "draw world (most static stuff)"};
cvar_t r_drawviewmodel = {CF_CLIENT, "r_drawviewmodel","1", "draw your weapon model"};
cvar_t r_drawexteriormodel = {CF_CLIENT, "r_drawexteriormodel","1", "draw your player model (e.g. in chase cam, reflections)"};
cvar_t r_cullentities_trace = {CF_CLIENT, "r_cullentities_trace", "1", "probabistically cull invisible entities"};
cvar_t r_cullentities_trace_entityocclusion = {CF_CLIENT, "r_cullentities_trace_entityocclusion", "1", "check for occluding entities such as doors, not just world hull"};
cvar_t r_cullentities_trace_samples = {CF_CLIENT, "r_cullentities_trace_samples", "2", "number of samples to test for entity culling (in addition to center sample)"};
cvar_t r_cullentities_trace_tempentitysamples = {CF_CLIENT, "r_cullentities_trace_tempentitysamples", "-1", "number of samples to test for entity culling of temp entities (including all CSQC entities), -1 disables trace culling on these entities to prevent flicker (pvs still applies)"};
cvar_t r_cullentities_trace_enlarge = {CF_CLIENT, "r_cullentities_trace_enlarge", "0", "box enlargement for entity culling"};
cvar_t r_cullentities_trace_expand = {CF_CLIENT, "r_cullentities_trace_expand", "0", "box expanded by this many units for entity culling"};
cvar_t r_cullentities_trace_pad = {CF_CLIENT, "r_cullentities_trace_pad", "8", "accept traces that hit within this many units of the box"};
cvar_t r_cullentities_trace_delay = {CF_CLIENT, "r_cullentities_trace_delay", "1", "number of seconds until the entity gets actually culled"};
cvar_t r_cullentities_trace_eyejitter = {CF_CLIENT, "r_cullentities_trace_eyejitter", "16", "randomly offset rays from the eye by this much to reduce the odds of flickering"};
cvar_t r_sortentities = {CF_CLIENT, "r_sortentities", "0", "sort entities before drawing (might be faster)"};
cvar_t r_speeds = {CF_CLIENT, "r_speeds","0", "displays rendering statistics and per-subsystem timings"};
cvar_t r_fullbright = {CF_CLIENT, "r_fullbright","0", "makes map very bright and renders faster"};

cvar_t r_fullbright_directed = {CF_CLIENT, "r_fullbright_directed", "0", "render fullbright things (unlit worldmodel and EF_FULLBRIGHT entities, but not fullbright shaders) using a constant light direction instead to add more depth while keeping uniform brightness"};
cvar_t r_fullbright_directed_ambient = {CF_CLIENT, "r_fullbright_directed_ambient", "0.5", "ambient light multiplier for directed fullbright"};
cvar_t r_fullbright_directed_diffuse = {CF_CLIENT, "r_fullbright_directed_diffuse", "0.75", "diffuse light multiplier for directed fullbright"};
cvar_t r_fullbright_directed_pitch = {CF_CLIENT, "r_fullbright_directed_pitch", "20", "constant pitch direction ('height') of the fake light source to use for fullbright"};
cvar_t r_fullbright_directed_pitch_relative = {CF_CLIENT, "r_fullbright_directed_pitch_relative", "0", "whether r_fullbright_directed_pitch is interpreted as absolute (0) or relative (1) pitch"};

cvar_t r_wateralpha = {CF_CLIENT | CF_ARCHIVE, "r_wateralpha","1", "opacity of water polygons"};
cvar_t r_dynamic = {CF_CLIENT | CF_ARCHIVE, "r_dynamic","1", "enables dynamic lights (rocket glow and such)"};
cvar_t r_fullbrights = {CF_CLIENT | CF_ARCHIVE, "r_fullbrights", "1", "enables glowing pixels in quake textures (changes need r_restart to take effect)"};
cvar_t r_shadows = {CF_CLIENT | CF_ARCHIVE, "r_shadows", "0", "casts fake stencil shadows from models onto the world (rtlights are unaffected by this); when set to 2, always cast the shadows in the direction set by r_shadows_throwdirection, otherwise use the model lighting."};
cvar_t r_shadows_darken = {CF_CLIENT | CF_ARCHIVE, "r_shadows_darken", "0.5", "how much shadowed areas will be darkened"};
cvar_t r_shadows_throwdistance = {CF_CLIENT | CF_ARCHIVE, "r_shadows_throwdistance", "500", "how far to cast shadows from models"};
cvar_t r_shadows_throwdirection = {CF_CLIENT | CF_ARCHIVE, "r_shadows_throwdirection", "0 0 -1", "override throwing direction for r_shadows 2"};
cvar_t r_shadows_drawafterrtlighting = {CF_CLIENT | CF_ARCHIVE, "r_shadows_drawafterrtlighting", "0", "draw fake shadows AFTER realtime lightning is drawn. May be useful for simulating fast sunlight on large outdoor maps with only one noshadow rtlight. The price is less realistic appearance of dynamic light shadows."};
cvar_t r_shadows_castfrombmodels = {CF_CLIENT | CF_ARCHIVE, "r_shadows_castfrombmodels", "0", "do cast shadows from bmodels"};
cvar_t r_shadows_focus = {CF_CLIENT | CF_ARCHIVE, "r_shadows_focus", "0 0 0", "offset the shadowed area focus"};
cvar_t r_shadows_shadowmapscale = {CF_CLIENT | CF_ARCHIVE, "r_shadows_shadowmapscale", "0.25", "higher values increase shadowmap quality at a cost of area covered (multiply global shadowmap precision) for fake shadows. Needs shadowmapping ON."};
cvar_t r_shadows_shadowmapbias = {CF_CLIENT | CF_ARCHIVE, "r_shadows_shadowmapbias", "-1", "sets shadowmap bias for fake shadows. -1 sets the value of r_shadow_shadowmapping_bias. Needs shadowmapping ON."};
cvar_t r_q1bsp_skymasking = {CF_CLIENT, "r_q1bsp_skymasking", "1", "allows sky polygons in quake1 maps to obscure other geometry"};
cvar_t r_polygonoffset_submodel_factor = {CF_CLIENT, "r_polygonoffset_submodel_factor", "0", "biases depth values of world submodels such as doors, to prevent z-fighting artifacts in Quake maps"};
cvar_t r_polygonoffset_submodel_offset = {CF_CLIENT, "r_polygonoffset_submodel_offset", "14", "biases depth values of world submodels such as doors, to prevent z-fighting artifacts in Quake maps"};
cvar_t r_polygonoffset_decals_factor = {CF_CLIENT, "r_polygonoffset_decals_factor", "0", "biases depth values of decals to prevent z-fighting artifacts"};
cvar_t r_polygonoffset_decals_offset = {CF_CLIENT, "r_polygonoffset_decals_offset", "-14", "biases depth values of decals to prevent z-fighting artifacts"};
cvar_t r_fog_exp2 = {CF_CLIENT, "r_fog_exp2", "0", "uses GL_EXP2 fog (as in Nehahra) rather than realistic GL_EXP fog"};
cvar_t r_fog_clear = {CF_CLIENT, "r_fog_clear", "1", "clears renderbuffer with fog color before render starts"};
cvar_t r_drawfog = {CF_CLIENT | CF_ARCHIVE, "r_drawfog", "1", "allows one to disable fog rendering"};
cvar_t r_transparentdepthmasking = {CF_CLIENT | CF_ARCHIVE, "r_transparentdepthmasking", "0", "enables depth writes on transparent meshes whose materially is normally opaque, this prevents seeing the inside of a transparent mesh"};
cvar_t r_transparent_sortmindist = {CF_CLIENT | CF_ARCHIVE, "r_transparent_sortmindist", "0", "lower distance limit for transparent sorting"};
cvar_t r_transparent_sortmaxdist = {CF_CLIENT | CF_ARCHIVE, "r_transparent_sortmaxdist", "32768", "upper distance limit for transparent sorting"};
cvar_t r_transparent_sortarraysize = {CF_CLIENT | CF_ARCHIVE, "r_transparent_sortarraysize", "4096", "number of distance-sorting layers"};
cvar_t r_celshading = {CF_CLIENT | CF_ARCHIVE, "r_celshading", "0", "cartoon-style light shading (OpenGL 2.x only)"}; // FIXME remove OpenGL 2.x only once implemented for DX9
cvar_t r_celoutlines = {CF_CLIENT | CF_ARCHIVE, "r_celoutlines", "0", "cartoon-style outlines (requires r_shadow_deferred)"};

cvar_t gl_fogenable = {CF_CLIENT, "gl_fogenable", "0", "nehahra fog enable (for Nehahra compatibility only)"};
cvar_t gl_fogdensity = {CF_CLIENT, "gl_fogdensity", "0.25", "nehahra fog density (recommend values below 0.1) (for Nehahra compatibility only)"};
cvar_t gl_fogred = {CF_CLIENT, "gl_fogred","0.3", "nehahra fog color red value (for Nehahra compatibility only)"};
cvar_t gl_foggreen = {CF_CLIENT, "gl_foggreen","0.3", "nehahra fog color green value (for Nehahra compatibility only)"};
cvar_t gl_fogblue = {CF_CLIENT, "gl_fogblue","0.3", "nehahra fog color blue value (for Nehahra compatibility only)"};
cvar_t gl_fogstart = {CF_CLIENT, "gl_fogstart", "0", "nehahra fog start distance (for Nehahra compatibility only)"};
cvar_t gl_fogend = {CF_CLIENT, "gl_fogend","0", "nehahra fog end distance (for Nehahra compatibility only)"};
cvar_t gl_skyclip = {CF_CLIENT, "gl_skyclip", "4608", "nehahra farclip distance - the real fog end (for Nehahra compatibility only)"};

cvar_t r_texture_dds_load = {CF_CLIENT | CF_ARCHIVE, "r_texture_dds_load", "0", "load compressed dds/filename.dds texture instead of filename.tga, if the file exists (requires driver support)"};
cvar_t r_texture_dds_save = {CF_CLIENT | CF_ARCHIVE, "r_texture_dds_save", "0", "save compressed dds/filename.dds texture when filename.tga is loaded, so that it can be loaded instead next time"};

cvar_t r_usedepthtextures = {CF_CLIENT | CF_ARCHIVE, "r_usedepthtextures", "1", "use depth texture instead of depth renderbuffer where possible, uses less video memory but may render slower (or faster) depending on hardware"};
cvar_t r_viewfbo = {CF_CLIENT | CF_ARCHIVE, "r_viewfbo", "0", "enables use of an 8bit (1) or 16bit (2) or 32bit (3) per component float framebuffer render, which may be at a different resolution than the video mode; the default setting of 0 uses a framebuffer render when required, and renders directly to the screen otherwise"};
cvar_t r_rendertarget_debug = {CF_CLIENT, "r_rendertarget_debug", "-1", "replaces the view with the contents of the specified render target (by number - note that these can fluctuate depending on scene)"};
cvar_t r_viewscale = {CF_CLIENT | CF_ARCHIVE, "r_viewscale", "1", "scaling factor for resolution of the fbo rendering method, must be > 0, can be above 1 for a costly antialiasing behavior, typical values are 0.5 for 1/4th as many pixels rendered, or 1 for normal rendering"};
cvar_t r_viewscale_fpsscaling = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling", "0", "change resolution based on framerate"};
cvar_t r_viewscale_fpsscaling_min = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling_min", "0.0625", "worst acceptable quality"};
cvar_t r_viewscale_fpsscaling_multiply = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling_multiply", "5", "adjust quality up or down by the frametime difference from 1.0/target, multiplied by this factor"};
cvar_t r_viewscale_fpsscaling_stepsize = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling_stepsize", "0.01", "smallest adjustment to hit the target framerate (this value prevents minute oscillations)"};
cvar_t r_viewscale_fpsscaling_stepmax = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling_stepmax", "1.00", "largest adjustment to hit the target framerate (this value prevents wild overshooting of the estimate)"};
cvar_t r_viewscale_fpsscaling_target = {CF_CLIENT | CF_ARCHIVE, "r_viewscale_fpsscaling_target", "70", "desired framerate"};

cvar_t r_glsl_skeletal = {CF_CLIENT | CF_ARCHIVE, "r_glsl_skeletal", "1", "render skeletal models faster using a gpu-skinning technique"};
cvar_t r_glsl_deluxemapping = {CF_CLIENT | CF_ARCHIVE, "r_glsl_deluxemapping", "1", "use per pixel lighting on deluxemap-compiled q3bsp maps (or a value of 2 forces deluxemap shading even without deluxemaps)"};
cvar_t r_glsl_offsetmapping = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping", "0", "offset mapping effect (also known as parallax mapping or virtual displacement mapping)"};
cvar_t r_glsl_offsetmapping_steps = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_steps", "2", "offset mapping steps (note: too high values may be not supported by your GPU)"};
cvar_t r_glsl_offsetmapping_reliefmapping = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_reliefmapping", "0", "relief mapping effect (higher quality)"};
cvar_t r_glsl_offsetmapping_reliefmapping_steps = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_reliefmapping_steps", "10", "relief mapping steps (note: too high values may be not supported by your GPU)"};
cvar_t r_glsl_offsetmapping_reliefmapping_refinesteps = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_reliefmapping_refinesteps", "5", "relief mapping refine steps (these are a binary search executed as the last step as given by r_glsl_offsetmapping_reliefmapping_steps)"};
cvar_t r_glsl_offsetmapping_scale = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_scale", "0.04", "how deep the offset mapping effect is"};
cvar_t r_glsl_offsetmapping_lod = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_lod", "0", "apply distance-based level-of-detail correction to number of offsetmappig steps, effectively making it render faster on large open-area maps"};
cvar_t r_glsl_offsetmapping_lod_distance = {CF_CLIENT | CF_ARCHIVE, "r_glsl_offsetmapping_lod_distance", "32", "first LOD level distance, second level (-50% steps) is 2x of this, third (33%) - 3x etc."};
cvar_t r_glsl_postprocess = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess", "0", "use a GLSL postprocessing shader"};
cvar_t r_glsl_postprocess_uservec1 = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec1", "0 0 0 0", "a 4-component vector to pass as uservec1 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec2 = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec2", "0 0 0 0", "a 4-component vector to pass as uservec2 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec3 = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec3", "0 0 0 0", "a 4-component vector to pass as uservec3 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec4 = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec4", "0 0 0 0", "a 4-component vector to pass as uservec4 to the postprocessing shader (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec1_enable = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec1_enable", "1", "enables postprocessing uservec1 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec2_enable = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec2_enable", "1", "enables postprocessing uservec2 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec3_enable = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec3_enable", "1", "enables postprocessing uservec3 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_glsl_postprocess_uservec4_enable = {CF_CLIENT | CF_ARCHIVE, "r_glsl_postprocess_uservec4_enable", "1", "enables postprocessing uservec4 usage, creates USERVEC1 define (only useful if default.glsl has been customized)"};
cvar_t r_colorfringe = {CF_CLIENT | CF_ARCHIVE, "r_colorfringe", "0", "Chromatic aberration. Values higher than 0.025 will noticeably distort the image"};
cvar_t r_fxaa = {CF_CLIENT | CF_ARCHIVE, "r_fxaa", "0", "fast approximate anti aliasing"};

cvar_t r_water = {CF_CLIENT | CF_ARCHIVE, "r_water", "0", "whether to use reflections and refraction on water surfaces (note: r_wateralpha must be set below 1)"};
cvar_t r_water_cameraentitiesonly = {CF_CLIENT | CF_ARCHIVE, "r_water_cameraentitiesonly", "0", "whether to only show QC-defined reflections/refractions (typically used for camera- or portal-like effects)"};
cvar_t r_water_clippingplanebias = {CF_CLIENT | CF_ARCHIVE, "r_water_clippingplanebias", "1", "a rather technical setting which avoids black pixels around water edges"};
cvar_t r_water_resolutionmultiplier = {CF_CLIENT | CF_ARCHIVE, "r_water_resolutionmultiplier", "0.5", "multiplier for screen resolution when rendering refracted/reflected scenes, 1 is full quality, lower values are faster"};
cvar_t r_water_refractdistort = {CF_CLIENT | CF_ARCHIVE, "r_water_refractdistort", "0.01", "how much water refractions shimmer"};
cvar_t r_water_reflectdistort = {CF_CLIENT | CF_ARCHIVE, "r_water_reflectdistort", "0.01", "how much water reflections shimmer"};
cvar_t r_water_scissormode = {CF_CLIENT, "r_water_scissormode", "3", "scissor (1) or cull (2) or both (3) water renders"};
cvar_t r_water_lowquality = {CF_CLIENT, "r_water_lowquality", "0", "special option to accelerate water rendering: 1 disables all dynamic lights, 2 disables particles too"};
cvar_t r_water_hideplayer = {CF_CLIENT | CF_ARCHIVE, "r_water_hideplayer", "0", "if set to 1 then player will be hidden in refraction views, if set to 2 then player will also be hidden in reflection views, player is always visible in camera views"};

cvar_t r_lerpsprites = {CF_CLIENT | CF_ARCHIVE, "r_lerpsprites", "0", "enables animation smoothing on sprites"};
cvar_t r_lerpmodels = {CF_CLIENT | CF_ARCHIVE, "r_lerpmodels", "1", "enables animation smoothing on models"};
cvar_t r_nolerp_list = {CF_CLIENT | CF_ARCHIVE, "r_nolerp_list", "progs/v_nail.mdl,progs/v_nail2.mdl,progs/flame.mdl,progs/flame2.mdl,progs/braztall.mdl,progs/brazshrt.mdl,progs/longtrch.mdl,progs/flame_pyre.mdl,progs/v_saw.mdl,progs/v_xfist.mdl,progs/h2stuff/newfire.mdl", "comma separated list of models that will not have their animations smoothed"};
cvar_t r_lerplightstyles = {CF_CLIENT | CF_ARCHIVE, "r_lerplightstyles", "0", "enable animation smoothing on flickering lights"};
cvar_t r_waterscroll = {CF_CLIENT | CF_ARCHIVE, "r_waterscroll", "1", "makes water scroll around, value controls how much"};

cvar_t r_bloom = {CF_CLIENT | CF_ARCHIVE, "r_bloom", "0", "enables bloom effect (makes bright pixels affect neighboring pixels)"};
cvar_t r_bloom_colorscale = {CF_CLIENT | CF_ARCHIVE, "r_bloom_colorscale", "1", "how bright the glow is"};

cvar_t r_bloom_brighten = {CF_CLIENT | CF_ARCHIVE, "r_bloom_brighten", "1", "how bright the glow is, after subtract/power"};
cvar_t r_bloom_blur = {CF_CLIENT | CF_ARCHIVE, "r_bloom_blur", "4", "how large the glow is"};
cvar_t r_bloom_resolution = {CF_CLIENT | CF_ARCHIVE, "r_bloom_resolution", "320", "what resolution to perform the bloom effect at (independent of screen resolution)"};
cvar_t r_bloom_colorexponent = {CF_CLIENT | CF_ARCHIVE, "r_bloom_colorexponent", "1", "how exaggerated the glow is"};
cvar_t r_bloom_colorsubtract = {CF_CLIENT | CF_ARCHIVE, "r_bloom_colorsubtract", "0.1", "reduces bloom colors by a certain amount"};
cvar_t r_bloom_scenebrightness = {CF_CLIENT | CF_ARCHIVE, "r_bloom_scenebrightness", "1", "global rendering brightness when bloom is enabled"};

cvar_t r_hdr_scenebrightness = {CF_CLIENT | CF_ARCHIVE, "r_hdr_scenebrightness", "1", "global rendering brightness"};
cvar_t r_hdr_glowintensity = {CF_CLIENT | CF_ARCHIVE, "r_hdr_glowintensity", "1", "how bright light emitting textures should appear"};
cvar_t r_hdr_irisadaptation = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation", "0", "adjust scene brightness according to light intensity at player location"};
cvar_t r_hdr_irisadaptation_multiplier = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_multiplier", "2", "brightness at which value will be 1.0"};
cvar_t r_hdr_irisadaptation_minvalue = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_minvalue", "0.5", "minimum value that can result from multiplier / brightness"};
cvar_t r_hdr_irisadaptation_maxvalue = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_maxvalue", "4", "maximum value that can result from multiplier / brightness"};
cvar_t r_hdr_irisadaptation_value = {CF_CLIENT, "r_hdr_irisadaptation_value", "1", "current value as scenebrightness multiplier, changes continuously when irisadaptation is active"};
cvar_t r_hdr_irisadaptation_fade_up = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_fade_up", "0.1", "fade rate at which value adjusts to darkness"};
cvar_t r_hdr_irisadaptation_fade_down = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_fade_down", "0.5", "fade rate at which value adjusts to brightness"};
cvar_t r_hdr_irisadaptation_radius = {CF_CLIENT | CF_ARCHIVE, "r_hdr_irisadaptation_radius", "15", "lighting within this many units of the eye is averaged"};

cvar_t r_smoothnormals_areaweighting = {CF_CLIENT, "r_smoothnormals_areaweighting", "1", "uses significantly faster (and supposedly higher quality) area-weighted vertex normals and tangent vectors rather than summing normalized triangle normals and tangents"};

cvar_t developer_texturelogging = {CF_CLIENT, "developer_texturelogging", "0", "produces a textures.log file containing names of skins and map textures the engine tried to load"};

cvar_t gl_lightmaps = {CF_CLIENT, "gl_lightmaps", "0", "draws only lightmaps, no texture (for level designers), a value of 2 keeps normalmap shading"};

cvar_t r_test = {CF_CLIENT, "r_test", "0", "internal development use only, leave it alone (usually does nothing anyway)"};

cvar_t r_batch_multidraw = {CF_CLIENT | CF_ARCHIVE, "r_batch_multidraw", "1", "issue multiple glDrawElements calls when rendering a batch of surfaces with the same texture (otherwise the index data is copied to make it one draw)"};
cvar_t r_batch_multidraw_mintriangles = {CF_CLIENT | CF_ARCHIVE, "r_batch_multidraw_mintriangles", "0", "minimum number of triangles to activate multidraw path (copying small groups of triangles may be faster)"};
cvar_t r_batch_debugdynamicvertexpath = {CF_CLIENT | CF_ARCHIVE, "r_batch_debugdynamicvertexpath", "0", "force the dynamic batching code path for debugging purposes"};
cvar_t r_batch_dynamicbuffer = {CF_CLIENT | CF_ARCHIVE, "r_batch_dynamicbuffer", "0", "use vertex/index buffers for drawing dynamic and copytriangles batches"};

cvar_t r_glsl_saturation = {CF_CLIENT | CF_ARCHIVE, "r_glsl_saturation", "1", "saturation multiplier (only working in glsl!)"};
cvar_t r_glsl_saturation_redcompensate = {CF_CLIENT | CF_ARCHIVE, "r_glsl_saturation_redcompensate", "0", "a 'vampire sight' addition to desaturation effect, does compensation for red color, r_glsl_restart is required"};

cvar_t r_glsl_vertextextureblend_usebothalphas = {CF_CLIENT | CF_ARCHIVE, "r_glsl_vertextextureblend_usebothalphas", "0", "use both alpha layers on vertex blended surfaces, each alpha layer sets amount of 'blend leak' on another layer, requires mod_q3shader_force_terrain_alphaflag on."};

// FIXME: This cvar would grow to a ridiculous size after several launches and clean exits when used during surface sorting.
cvar_t r_framedatasize = {CF_CLIENT | CF_ARCHIVE, "r_framedatasize", "0.5", "size of renderer data cache used during one frame (for skeletal animation caching, light processing, etc)"};
cvar_t r_buffermegs[R_BUFFERDATA_COUNT] =
{
	{CF_CLIENT | CF_ARCHIVE, "r_buffermegs_vertex", "4", "vertex buffer size for one frame"},
	{CF_CLIENT | CF_ARCHIVE, "r_buffermegs_index16", "1", "index buffer size for one frame (16bit indices)"},
	{CF_CLIENT | CF_ARCHIVE, "r_buffermegs_index32", "1", "index buffer size for one frame (32bit indices)"},
	{CF_CLIENT | CF_ARCHIVE, "r_buffermegs_uniform", "0.25", "uniform buffer size for one frame"},
};

cvar_t r_q1bsp_lightmap_updates_enabled = {CF_CLIENT, "r_q1bsp_lightmap_updates_enabled", "1", "allow lightmaps to be updated on Q1BSP maps (don't turn this off except for debugging)"};
cvar_t r_q1bsp_lightmap_updates_combine = {CF_CLIENT | CF_ARCHIVE, "r_q1bsp_lightmap_updates_combine", "2", "combine lightmap texture updates to make fewer glTexSubImage2D calls, modes: 0 = immediately upload lightmaps (may be thousands of small 3x3 updates), 1 = combine to one call, 2 = combine to one full texture update (glTexImage2D) which tells the driver it does not need to lock the resource (faster on most drivers)"};
cvar_t r_q1bsp_lightmap_updates_hidden_surfaces = {CF_CLIENT | CF_ARCHIVE, "r_q1bsp_lightmap_updates_hidden_surfaces", "0", "update lightmaps on surfaces that are not visible, so that updates only occur on frames where lightstyles changed value (animation or light switches), only makes sense with combine = 2"};

extern cvar_t v_glslgamma_2d;

extern qbool v_flipped_state;

r_framebufferstate_t r_fb;

/// shadow volume bsp struct with automatically growing nodes buffer
svbsp_t r_svbsp;

int r_uniformbufferalignment = 32; // dynamically updated to match GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT

rtexture_t *r_texture_blanknormalmap;
rtexture_t *r_texture_white;
rtexture_t *r_texture_grey128;
rtexture_t *r_texture_black;
rtexture_t *r_texture_notexture;
rtexture_t *r_texture_whitecube;
rtexture_t *r_texture_normalizationcube;
rtexture_t *r_texture_fogattenuation;
rtexture_t *r_texture_fogheighttexture;
rtexture_t *r_texture_gammaramps;
unsigned int r_texture_gammaramps_serial;
//rtexture_t *r_texture_fogintensity;
rtexture_t *r_texture_reflectcube;

// TODO: hash lookups?
typedef struct cubemapinfo_s
{
	char basename[64];
	rtexture_t *texture;
}
cubemapinfo_t;

int r_texture_numcubemaps;
cubemapinfo_t *r_texture_cubemaps[MAX_CUBEMAPS];

unsigned int r_queries[MAX_OCCLUSION_QUERIES];
unsigned int r_numqueries;
unsigned int r_maxqueries;

typedef struct r_qwskincache_s
{
	char name[MAX_QPATH];
	skinframe_t *skinframe;
}
r_qwskincache_t;

static r_qwskincache_t *r_qwskincache;
static int r_qwskincache_size;

/// vertex coordinates for a quad that covers the screen exactly
extern const float r_screenvertex3f[12];
const float r_screenvertex3f[12] =
{
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0
};

void R_ModulateColors(float *in, float *out, int verts, float r, float g, float b)
{
	int i;
	for (i = 0;i < verts;i++)
	{
		out[0] = in[0] * r;
		out[1] = in[1] * g;
		out[2] = in[2] * b;
		out[3] = in[3];
		in += 4;
		out += 4;
	}
}

void R_FillColors(float *out, int verts, float r, float g, float b, float a)
{
	int i;
	for (i = 0;i < verts;i++)
	{
		out[0] = r;
		out[1] = g;
		out[2] = b;
		out[3] = a;
		out += 4;
	}
}

// FIXME: move this to client?
void FOG_clear(void)
{
	if (gamemode == GAME_NEHAHRA)
	{
		Cvar_Set(&cvars_all, "gl_fogenable", "0");
		Cvar_Set(&cvars_all, "gl_fogdensity", "0.2");
		Cvar_Set(&cvars_all, "gl_fogred", "0.3");
		Cvar_Set(&cvars_all, "gl_foggreen", "0.3");
		Cvar_Set(&cvars_all, "gl_fogblue", "0.3");
	}
	r_refdef.fog_density = 0;
	r_refdef.fog_red = 0;
	r_refdef.fog_green = 0;
	r_refdef.fog_blue = 0;
	r_refdef.fog_alpha = 1;
	r_refdef.fog_start = 0;
	r_refdef.fog_end = 16384;
	r_refdef.fog_height = 1<<30;
	r_refdef.fog_fadedepth = 128;
	memset(r_refdef.fog_height_texturename, 0, sizeof(r_refdef.fog_height_texturename));
}

static void R_BuildBlankTextures(void)
{
	unsigned char data[4];
	data[2] = 128; // normal X
	data[1] = 128; // normal Y
	data[0] = 255; // normal Z
	data[3] = 255; // height
	r_texture_blanknormalmap = R_LoadTexture2D(r_main_texturepool, "blankbump", 1, 1, data, TEXTYPE_BGRA, TEXF_PERSISTENT, -1, NULL);
	data[0] = 255;
	data[1] = 255;
	data[2] = 255;
	data[3] = 255;
	r_texture_white = R_LoadTexture2D(r_main_texturepool, "blankwhite", 1, 1, data, TEXTYPE_BGRA, TEXF_PERSISTENT, -1, NULL);
	data[0] = 128;
	data[1] = 128;
	data[2] = 128;
	data[3] = 255;
	r_texture_grey128 = R_LoadTexture2D(r_main_texturepool, "blankgrey128", 1, 1, data, TEXTYPE_BGRA, TEXF_PERSISTENT, -1, NULL);
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 255;
	r_texture_black = R_LoadTexture2D(r_main_texturepool, "blankblack", 1, 1, data, TEXTYPE_BGRA, TEXF_PERSISTENT, -1, NULL);
}

static void R_BuildNoTexture(void)
{
	r_texture_notexture = R_LoadTexture2D(r_main_texturepool, "notexture", 16, 16, Image_GenerateNoTexture(), TEXTYPE_BGRA, TEXF_MIPMAP | TEXF_PERSISTENT, -1, NULL);
}

static void R_BuildWhiteCube(void)
{
	unsigned char data[6*1*1*4];
	memset(data, 255, sizeof(data));
	r_texture_whitecube = R_LoadTextureCubeMap(r_main_texturepool, "whitecube", 1, data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_PERSISTENT, -1, NULL);
}

static void R_BuildNormalizationCube(void)
{
	int x, y, side;
	vec3_t v;
	vec_t s, t, intensity;
#define NORMSIZE 64
	unsigned char *data;
	data = (unsigned char *)Mem_Alloc(tempmempool, 6*NORMSIZE*NORMSIZE*4);
	for (side = 0;side < 6;side++)
	{
		for (y = 0;y < NORMSIZE;y++)
		{
			for (x = 0;x < NORMSIZE;x++)
			{
				s = (x + 0.5f) * (2.0f / NORMSIZE) - 1.0f;
				t = (y + 0.5f) * (2.0f / NORMSIZE) - 1.0f;
				switch(side)
				{
				default:
				case 0:
					v[0] = 1;
					v[1] = -t;
					v[2] = -s;
					break;
				case 1:
					v[0] = -1;
					v[1] = -t;
					v[2] = s;
					break;
				case 2:
					v[0] = s;
					v[1] = 1;
					v[2] = t;
					break;
				case 3:
					v[0] = s;
					v[1] = -1;
					v[2] = -t;
					break;
				case 4:
					v[0] = s;
					v[1] = -t;
					v[2] = 1;
					break;
				case 5:
					v[0] = -s;
					v[1] = -t;
					v[2] = -1;
					break;
				}
				intensity = 127.0f / sqrt(DotProduct(v, v));
				data[((side*64+y)*64+x)*4+2] = (unsigned char)(128.0f + intensity * v[0]);
				data[((side*64+y)*64+x)*4+1] = (unsigned char)(128.0f + intensity * v[1]);
				data[((side*64+y)*64+x)*4+0] = (unsigned char)(128.0f + intensity * v[2]);
				data[((side*64+y)*64+x)*4+3] = 255;
			}
		}
	}
	r_texture_normalizationcube = R_LoadTextureCubeMap(r_main_texturepool, "normalcube", NORMSIZE, data, TEXTYPE_BGRA, TEXF_CLAMP | TEXF_PERSISTENT, -1, NULL);
	Mem_Free(data);
}

static void R_BuildFogTexture(void)
{
	int x, b;
#define FOGWIDTH 256
	unsigned char data1[FOGWIDTH][4];
	//unsigned char data2[FOGWIDTH][4];
	double d, r, alpha;

	r_refdef.fogmasktable_start = r_refdef.fog_start;
	r_refdef.fogmasktable_alpha = r_refdef.fog_alpha;
	r_refdef.fogmasktable_range = r_refdef.fogrange;
	r_refdef.fogmasktable_density = r_refdef.fog_density;

	r = r_refdef.fogmasktable_range / FOGMASKTABLEWIDTH;
	for (x = 0;x < FOGMASKTABLEWIDTH;x++)
	{
		d = (x * r - r_refdef.fogmasktable_start);
		if(developer_extra.integer)
			Con_DPrintf("%f ", d);
		d = max(0, d);
		if (r_fog_exp2.integer)
			alpha = exp(-r_refdef.fogmasktable_density * r_refdef.fogmasktable_density * 0.0001 * d * d);
		else
			alpha = exp(-r_refdef.fogmasktable_density * 0.004 * d);
		if(developer_extra.integer)
			Con_DPrintf(" : %f ", alpha);
		alpha = 1 - (1 - alpha) * r_refdef.fogmasktable_alpha;
		if(developer_extra.integer)
			Con_DPrintf(" = %f\n", alpha);
		r_refdef.fogmasktable[x] = bound(0, alpha, 1);
	}

	for (x = 0;x < FOGWIDTH;x++)
	{
		b = (int)(r_refdef.fogmasktable[x * (FOGMASKTABLEWIDTH - 1) / (FOGWIDTH - 1)] * 255);
		data1[x][0] = b;
		data1[x][1] = b;
		data1[x][2] = b;
		data1[x][3] = 255;
		//data2[x][0] = 255 - b;
		//data2[x][1] = 255 - b;
		//data2[x][2] = 255 - b;
		//data2[x][3] = 255;
	}
	if (r_texture_fogattenuation)
	{
		R_UpdateTexture(r_texture_fogattenuation, &data1[0][0], 0, 0, 0, FOGWIDTH, 1, 1, 0);
		//R_UpdateTexture(r_texture_fogattenuation, &data2[0][0], 0, 0, 0, FOGWIDTH, 1, 1, 0);
	}
	else
	{
		r_texture_fogattenuation = R_LoadTexture2D(r_main_texturepool, "fogattenuation", FOGWIDTH, 1, &data1[0][0], TEXTYPE_BGRA, TEXF_FORCELINEAR | TEXF_CLAMP | TEXF_PERSISTENT, -1, NULL);
		//r_texture_fogintensity = R_LoadTexture2D(r_main_texturepool, "fogintensity", FOGWIDTH, 1, &data2[0][0], TEXTYPE_BGRA, TEXF_FORCELINEAR | TEXF_CLAMP, NULL);
	}
}

static void R_BuildFogHeightTexture(void)
{
	unsigned char *inpixels;
	int size;
	int x;
	int y;
	int j;
	float c[4];
	float f;
	inpixels = NULL;
	dp_strlcpy(r_refdef.fogheighttexturename, r_refdef.fog_height_texturename, sizeof(r_refdef.fogheighttexturename));
	if (r_refdef.fogheighttexturename[0])
		inpixels = loadimagepixelsbgra(r_refdef.fogheighttexturename, true, false, false, NULL);
	if (!inpixels)
	{
		r_refdef.fog_height_tablesize = 0;
		if (r_texture_fogheighttexture)
			R_FreeTexture(r_texture_fogheighttexture);
		r_texture_fogheighttexture = NULL;
		if (r_refdef.fog_height_table2d)
			Mem_Free(r_refdef.fog_height_table2d);
		r_refdef.fog_height_table2d = NULL;
		if (r_refdef.fog_height_table1d)
			Mem_Free(r_refdef.fog_height_table1d);
		r_refdef.fog_height_table1d = NULL;
		return;
	}
	size = image_width;
	r_refdef.fog_height_tablesize = size;
	r_refdef.fog_height_table1d = (unsigned char *)Mem_Alloc(r_main_mempool, size * 4);
	r_refdef.fog_height_table2d = (unsigned char *)Mem_Alloc(r_main_mempool, size * size * 4);
	memcpy(r_refdef.fog_height_table1d, inpixels, size * 4);
	Mem_Free(inpixels);
	// LadyHavoc: now the magic - what is that table2d for?  it is a cooked
	// average fog color table accounting for every fog layer between a point
	// and the camera.  (Note: attenuation is handled separately!)
	for (y = 0;y < size;y++)
	{
		for (x = 0;x < size;x++)
		{
			Vector4Clear(c);
			f = 0;
			if (x < y)
			{
				for (j = x;j <= y;j++)
				{
					Vector4Add(c, r_refdef.fog_height_table1d + j*4, c);
					f++;
				}
			}
			else
			{
				for (j = x;j >= y;j--)
				{
					Vector4Add(c, r_refdef.fog_height_table1d + j*4, c);
					f++;
				}
			}
			f = 1.0f / f;
			r_refdef.fog_height_table2d[(y*size+x)*4+0] = (unsigned char)(c[0] * f);
			r_refdef.fog_height_table2d[(y*size+x)*4+1] = (unsigned char)(c[1] * f);
			r_refdef.fog_height_table2d[(y*size+x)*4+2] = (unsigned char)(c[2] * f);
			r_refdef.fog_height_table2d[(y*size+x)*4+3] = (unsigned char)(c[3] * f);
		}
	}
	r_texture_fogheighttexture = R_LoadTexture2D(r_main_texturepool, "fogheighttable", size, size, r_refdef.fog_height_table2d, TEXTYPE_BGRA, TEXF_ALPHA | TEXF_CLAMP, -1, NULL);
}

//=======================================================================================================================================================

static const char *builtinshaderstrings[] =
{
#include "shader_glsl.h"
0
};

//=======================================================================================================================================================

typedef struct shaderpermutationinfo_s
{
	const char *pretext;
	const char *name;
}
shaderpermutationinfo_t;

typedef struct shadermodeinfo_s
{
	const char *sourcebasename;
	const char *extension;
	const char **builtinshaderstrings;
	const char *pretext;
	const char *name;
	char *filename;
	char *builtinstring;
	int builtincrc;
}
shadermodeinfo_t;

// NOTE: MUST MATCH ORDER OF SHADERPERMUTATION_* DEFINES!
shaderpermutationinfo_t shaderpermutationinfo[SHADERPERMUTATION_COUNT] =
{
	{"#define USEDIFFUSE\n", " diffuse"},
	{"#define USEVERTEXTEXTUREBLEND\n", " vertextextureblend"},
	{"#define USEVIEWTINT\n", " viewtint"},
	{"#define USECOLORMAPPING\n", " colormapping"},
	{"#define USESATURATION\n", " saturation"},
	{"#define USEFOGINSIDE\n", " foginside"},
	{"#define USEFOGOUTSIDE\n", " fogoutside"},
	{"#define USEFOGHEIGHTTEXTURE\n", " fogheighttexture"},
	{"#define USEFOGALPHAHACK\n", " fogalphahack"},
	{"#define USEGAMMARAMPS\n", " gammaramps"},
	{"#define USECUBEFILTER\n", " cubefilter"},
	{"#define USEGLOW\n", " glow"},
	{"#define USEBLOOM\n", " bloom"},
	{"#define USESPECULAR\n", " specular"},
	{"#define USEPOSTPROCESSING\n", " postprocessing"},
	{"#define USEREFLECTION\n", " reflection"},
	{"#define USEOFFSETMAPPING\n", " offsetmapping"},
	{"#define USEOFFSETMAPPING_RELIEFMAPPING\n", " reliefmapping"},
	{"#define USESHADOWMAP2D\n", " shadowmap2d"},
	{"#define USESHADOWMAPVSDCT\n", " shadowmapvsdct"}, // TODO make this a static parm
	{"#define USESHADOWMAPORTHO\n", " shadowmaportho"},
	{"#define USEDEFERREDLIGHTMAP\n", " deferredlightmap"},
	{"#define USEALPHAKILL\n", " alphakill"},
	{"#define USEREFLECTCUBE\n", " reflectcube"},
	{"#define USENORMALMAPSCROLLBLEND\n", " normalmapscrollblend"},
	{"#define USEBOUNCEGRID\n", " bouncegrid"},
	{"#define USEBOUNCEGRIDDIRECTIONAL\n", " bouncegriddirectional"}, // TODO make this a static parm
	{"#define USETRIPPY\n", " trippy"},
	{"#define USEDEPTHRGB\n", " depthrgb"},
	{"#define USEALPHAGENVERTEX\n", " alphagenvertex"},
	{"#define USESKELETAL\n", " skeletal"},
	{"#define USEOCCLUDE\n", " occlude"}
};

// NOTE: MUST MATCH ORDER OF SHADERMODE_* ENUMS!
shadermodeinfo_t shadermodeinfo[SHADERLANGUAGE_COUNT][SHADERMODE_COUNT] =
{
	// SHADERLANGUAGE_GLSL
	{
		{"combined", "glsl", builtinshaderstrings, "#define MODE_GENERIC\n", " generic"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_POSTPROCESS\n", " postprocess"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_DEPTH_OR_SHADOW\n", " depth/shadow"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_FLATCOLOR\n", " flatcolor"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_VERTEXCOLOR\n", " vertexcolor"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTMAP\n", " lightmap"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTDIRECTIONMAP_MODELSPACE\n", " lightdirectionmap_modelspace"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTDIRECTIONMAP_TANGENTSPACE\n", " lightdirectionmap_tangentspace"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTDIRECTIONMAP_FORCED_LIGHTMAP\n", " lightdirectionmap_forced_lightmap"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTDIRECTIONMAP_FORCED_VERTEXCOLOR\n", " lightdirectionmap_forced_vertexcolor"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTGRID\n", " lightgrid"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTDIRECTION\n", " lightdirection"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_LIGHTSOURCE\n", " lightsource"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_REFRACTION\n", " refraction"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_WATER\n", " water"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_DEFERREDGEOMETRY\n", " deferredgeometry"},
		{"combined", "glsl", builtinshaderstrings, "#define MODE_DEFERREDLIGHTSOURCE\n", " deferredlightsource"},
	},
};

struct r_glsl_permutation_s;
typedef struct r_glsl_permutation_s
{
	/// hash lookup data
	struct r_glsl_permutation_s *hashnext;
	unsigned int mode;
	uint64_t permutation;

	/// indicates if we have tried compiling this permutation already
	qbool compiled;
	/// 0 if compilation failed
	int program;
	// texture units assigned to each detected uniform
	int tex_Texture_First;
	int tex_Texture_Second;
	int tex_Texture_GammaRamps;
	int tex_Texture_Normal;
	int tex_Texture_Color;
	int tex_Texture_Gloss;
	int tex_Texture_Glow;
	int tex_Texture_SecondaryNormal;
	int tex_Texture_SecondaryColor;
	int tex_Texture_SecondaryGloss;
	int tex_Texture_SecondaryGlow;
	int tex_Texture_Pants;
	int tex_Texture_Shirt;
	int tex_Texture_FogHeightTexture;
	int tex_Texture_FogMask;
	int tex_Texture_LightGrid;
	int tex_Texture_Lightmap;
	int tex_Texture_Deluxemap;
	int tex_Texture_Attenuation;
	int tex_Texture_Cube;
	int tex_Texture_Refraction;
	int tex_Texture_Reflection;
	int tex_Texture_ShadowMap2D;
	int tex_Texture_CubeProjection;
	int tex_Texture_ScreenNormalMap;
	int tex_Texture_ScreenDiffuse;
	int tex_Texture_ScreenSpecular;
	int tex_Texture_ReflectMask;
	int tex_Texture_ReflectCube;
	int tex_Texture_BounceGrid;
	/// locations of detected uniforms in program object, or -1 if not found
	int loc_Texture_First;
	int loc_Texture_Second;
	int loc_Texture_GammaRamps;
	int loc_Texture_Normal;
	int loc_Texture_Color;
	int loc_Texture_Gloss;
	int loc_Texture_Glow;
	int loc_Texture_SecondaryNormal;
	int loc_Texture_SecondaryColor;
	int loc_Texture_SecondaryGloss;
	int loc_Texture_SecondaryGlow;
	int loc_Texture_Pants;
	int loc_Texture_Shirt;
	int loc_Texture_FogHeightTexture;
	int loc_Texture_FogMask;
	int loc_Texture_LightGrid;
	int loc_Texture_Lightmap;
	int loc_Texture_Deluxemap;
	int loc_Texture_Attenuation;
	int loc_Texture_Cube;
	int loc_Texture_Refraction;
	int loc_Texture_Reflection;
	int loc_Texture_ShadowMap2D;
	int loc_Texture_CubeProjection;
	int loc_Texture_ScreenNormalMap;
	int loc_Texture_ScreenDiffuse;
	int loc_Texture_ScreenSpecular;
	int loc_Texture_ReflectMask;
	int loc_Texture_ReflectCube;
	int loc_Texture_BounceGrid;
	int loc_Alpha;
	int loc_BloomBlur_Parameters;
	int loc_ClientTime;
	int loc_Color_Ambient;
	int loc_Color_Diffuse;
	int loc_Color_Specular;
	int loc_Color_Glow;
	int loc_Color_Pants;
	int loc_Color_Shirt;
	int loc_DeferredColor_Ambient;
	int loc_DeferredColor_Diffuse;
	int loc_DeferredColor_Specular;
	int loc_DeferredMod_Diffuse;
	int loc_DeferredMod_Specular;
	int loc_DistortScaleRefractReflect;
	int loc_EyePosition;
	int loc_FogColor;
	int loc_FogHeightFade;
	int loc_FogPlane;
	int loc_FogPlaneViewDist;
	int loc_FogRangeRecip;
	int loc_LightColor;
	int loc_LightDir;
	int loc_LightGridMatrix;
	int loc_LightGridNormalMatrix;
	int loc_LightPosition;
	int loc_OffsetMapping_ScaleSteps;
	int loc_OffsetMapping_LodDistance;
	int loc_OffsetMapping_Bias;
	int loc_PixelSize;
	int loc_ReflectColor;
	int loc_ReflectFactor;
	int loc_ReflectOffset;
	int loc_RefractColor;
	int loc_Saturation;
	int loc_ScreenCenterRefractReflect;
	int loc_ScreenScaleRefractReflect;
	int loc_ScreenToDepth;
	int loc_ShadowMap_Parameters;
	int loc_ShadowMap_TextureScale;
	int loc_SpecularPower;
	int loc_Skeletal_Transform12;
	int loc_UserVec1;
	int loc_UserVec2;
	int loc_UserVec3;
	int loc_UserVec4;
	int loc_ColorFringe;
	int loc_ViewTintColor;
	int loc_ViewToLight;
	int loc_ModelToLight;
	int loc_TexMatrix;
	int loc_BackgroundTexMatrix;
	int loc_ModelViewProjectionMatrix;
	int loc_ModelViewMatrix;
	int loc_PixelToScreenTexCoord;
	int loc_ModelToReflectCube;
	int loc_ShadowMapMatrix;
	int loc_BloomColorSubtract;
	int loc_NormalmapScrollBlend;
	int loc_BounceGridMatrix;
	int loc_BounceGridIntensity;
	/// uniform block bindings
	int ubibind_Skeletal_Transform12_UniformBlock;
	/// uniform block indices
	int ubiloc_Skeletal_Transform12_UniformBlock;
}
r_glsl_permutation_t;

#define SHADERPERMUTATION_HASHSIZE 256


// non-degradable "lightweight" shader parameters to keep the permutations simpler
// these can NOT degrade! only use for simple stuff
enum
{
	SHADERSTATICPARM_SATURATION_REDCOMPENSATE = 0, ///< red compensation filter for saturation
	SHADERSTATICPARM_EXACTSPECULARMATH = 1, ///< (lightsource or deluxemapping) use exact reflection map for specular effects, as opposed to the usual OpenGL approximation
	SHADERSTATICPARM_POSTPROCESS_USERVEC1 = 2, ///< postprocess uservec1 is enabled
	SHADERSTATICPARM_POSTPROCESS_USERVEC2 = 3, ///< postprocess uservec2 is enabled
	SHADERSTATICPARM_POSTPROCESS_USERVEC3 = 4, ///< postprocess uservec3 is enabled
	SHADERSTATICPARM_POSTPROCESS_USERVEC4 = 5,  ///< postprocess uservec4 is enabled
	SHADERSTATICPARM_VERTEXTEXTUREBLEND_USEBOTHALPHAS = 6, // use both alpha layers while blending materials, allows more advanced microblending
	SHADERSTATICPARM_OFFSETMAPPING_USELOD = 7,  ///< LOD for offsetmapping
	SHADERSTATICPARM_SHADOWMAPPCF_1 = 8, ///< PCF 1
	SHADERSTATICPARM_SHADOWMAPPCF_2 = 9, ///< PCF 2
	SHADERSTATICPARM_SHADOWSAMPLER = 10, ///< sampler
	SHADERSTATICPARM_CELSHADING = 11, ///< celshading (alternative diffuse and specular math)
	SHADERSTATICPARM_CELOUTLINES = 12, ///< celoutline (depth buffer analysis to produce outlines)
	SHADERSTATICPARM_FXAA = 13, ///< fast approximate anti aliasing
	SHADERSTATICPARM_COLORFRINGE = 14 ///< colorfringe (chromatic aberration)
};
#define SHADERSTATICPARMS_COUNT 15

static const char *shaderstaticparmstrings_list[SHADERSTATICPARMS_COUNT];
static int shaderstaticparms_count = 0;

static unsigned int r_compileshader_staticparms[(SHADERSTATICPARMS_COUNT + 0x1F) >> 5] = {0};
#define R_COMPILESHADER_STATICPARM_ENABLE(p) r_compileshader_staticparms[(p) >> 5] |= (1 << ((p) & 0x1F))

extern qbool r_shadow_shadowmapsampler;
extern int r_shadow_shadowmappcf;
qbool R_CompileShader_CheckStaticParms(void)
{
	static int r_compileshader_staticparms_save[(SHADERSTATICPARMS_COUNT + 0x1F) >> 5];
	memcpy(r_compileshader_staticparms_save, r_compileshader_staticparms, sizeof(r_compileshader_staticparms));
	memset(r_compileshader_staticparms, 0, sizeof(r_compileshader_staticparms));

	// detect all
	if (r_glsl_saturation_redcompensate.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_SATURATION_REDCOMPENSATE);
	if (r_glsl_vertextextureblend_usebothalphas.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_VERTEXTEXTUREBLEND_USEBOTHALPHAS);
	if (r_shadow_glossexact.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_EXACTSPECULARMATH);
	if (r_glsl_postprocess.integer)
	{
		if (r_glsl_postprocess_uservec1_enable.integer)
			R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_POSTPROCESS_USERVEC1);
		if (r_glsl_postprocess_uservec2_enable.integer)
			R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_POSTPROCESS_USERVEC2);
		if (r_glsl_postprocess_uservec3_enable.integer)
			R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_POSTPROCESS_USERVEC3);
		if (r_glsl_postprocess_uservec4_enable.integer)
			R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_POSTPROCESS_USERVEC4);
	}
	if (r_fxaa.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_FXAA);
	if (r_glsl_offsetmapping_lod.integer && r_glsl_offsetmapping_lod_distance.integer > 0)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_OFFSETMAPPING_USELOD);

	if (r_shadow_shadowmapsampler)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_SHADOWSAMPLER);
	if (r_shadow_shadowmappcf > 1)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_SHADOWMAPPCF_2);
	else if (r_shadow_shadowmappcf)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_SHADOWMAPPCF_1);
	if (r_celshading.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_CELSHADING);
	if (r_celoutlines.integer)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_CELOUTLINES);
	if (r_colorfringe.value)
		R_COMPILESHADER_STATICPARM_ENABLE(SHADERSTATICPARM_COLORFRINGE);

	return memcmp(r_compileshader_staticparms, r_compileshader_staticparms_save, sizeof(r_compileshader_staticparms)) != 0;
}

#define R_COMPILESHADER_STATICPARM_EMIT(p, n) \
	if(r_compileshader_staticparms[(p) >> 5] & (1 << ((p) & 0x1F))) \
		shaderstaticparmstrings_list[shaderstaticparms_count++] = "#define " n "\n"; \
	else \
		shaderstaticparmstrings_list[shaderstaticparms_count++] = "\n"
static void R_CompileShader_AddStaticParms(unsigned int mode, uint64_t permutation)
{
	shaderstaticparms_count = 0;

	// emit all
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_SATURATION_REDCOMPENSATE, "SATURATION_REDCOMPENSATE");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_EXACTSPECULARMATH, "USEEXACTSPECULARMATH");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_POSTPROCESS_USERVEC1, "USERVEC1");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_POSTPROCESS_USERVEC2, "USERVEC2");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_POSTPROCESS_USERVEC3, "USERVEC3");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_POSTPROCESS_USERVEC4, "USERVEC4");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_VERTEXTEXTUREBLEND_USEBOTHALPHAS, "USEBOTHALPHAS");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_OFFSETMAPPING_USELOD, "USEOFFSETMAPPING_LOD");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_SHADOWMAPPCF_1, "USESHADOWMAPPCF 1");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_SHADOWMAPPCF_2, "USESHADOWMAPPCF 2");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_SHADOWSAMPLER, "USESHADOWSAMPLER");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_CELSHADING, "USECELSHADING");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_CELOUTLINES, "USECELOUTLINES");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_FXAA, "USEFXAA");
	R_COMPILESHADER_STATICPARM_EMIT(SHADERSTATICPARM_COLORFRINGE, "USECOLORFRINGE");
}

/// information about each possible shader permutation
r_glsl_permutation_t *r_glsl_permutationhash[SHADERMODE_COUNT][SHADERPERMUTATION_HASHSIZE];
/// currently selected permutation
r_glsl_permutation_t *r_glsl_permutation;
/// storage for permutations linked in the hash table
memexpandablearray_t r_glsl_permutationarray;

static r_glsl_permutation_t *R_GLSL_FindPermutation(unsigned int mode, uint64_t permutation)
{
	//unsigned int hashdepth = 0;
	unsigned int hashindex = (permutation * 0x1021) & (SHADERPERMUTATION_HASHSIZE - 1);
	r_glsl_permutation_t *p;
	for (p = r_glsl_permutationhash[mode][hashindex];p;p = p->hashnext)
	{
		if (p->mode == mode && p->permutation == permutation)
		{
			//if (hashdepth > 10)
			//	Con_Printf("R_GLSL_FindPermutation: Warning: %i:%i has hashdepth %i\n", mode, permutation, hashdepth);
			return p;
		}
		//hashdepth++;
	}
	p = (r_glsl_permutation_t*)Mem_ExpandableArray_AllocRecord(&r_glsl_permutationarray);
	p->mode = mode;
	p->permutation = permutation;
	p->hashnext = r_glsl_permutationhash[mode][hashindex];
	r_glsl_permutationhash[mode][hashindex] = p;
	//if (hashdepth > 10)
	//	Con_Printf("R_GLSL_FindPermutation: Warning: %i:%i has hashdepth %i\n", mode, permutation, hashdepth);
	return p;
}

static char *R_ShaderStrCat(const char **strings)
{
	char *string, *s;
	const char **p = strings;
	const char *t;
	size_t len = 0;
	for (p = strings;(t = *p);p++)
		len += strlen(t);
	len++;
	s = string = (char *)Mem_Alloc(r_main_mempool, len);
	len = 0;
	for (p = strings;(t = *p);p++)
	{
		len = strlen(t);
		memcpy(s, t, len);
		s += len;
	}
	*s = 0;
	return string;
}

static char *R_ShaderStrCat(const char **strings);
static void R_InitShaderModeInfo(void)
{
	int i, language;
	shadermodeinfo_t *modeinfo;
	// we have a bunch of things to compute that weren't calculated at engine compile time - all filenames should have a crc of the builtin strings to prevent accidental overrides (any customization must be updated to match engine)
	for (language = 0; language < SHADERLANGUAGE_COUNT; language++)
	{
		for (i = 0; i < SHADERMODE_COUNT; i++)
		{
			char filename[MAX_QPATH];
			modeinfo = &shadermodeinfo[language][i];
			modeinfo->builtinstring = R_ShaderStrCat(modeinfo->builtinshaderstrings);
			modeinfo->builtincrc = CRC_Block((const unsigned char *)modeinfo->builtinstring, strlen(modeinfo->builtinstring));
			dpsnprintf(filename, sizeof(filename), "%s/%s_crc%i.%s", modeinfo->extension, modeinfo->sourcebasename, modeinfo->builtincrc, modeinfo->extension);
			modeinfo->filename = Mem_strdup(r_main_mempool, filename);
		}
	}
}

static char *ShaderModeInfo_GetShaderText(shadermodeinfo_t *modeinfo, qbool printfromdisknotice, qbool builtinonly)
{
	char *shaderstring;
	// if the mode has no filename we have to return the builtin string
	if (builtinonly || !modeinfo->filename)
		return Mem_strdup(r_main_mempool, modeinfo->builtinstring);
	// note that FS_LoadFile appends a 0 byte to make it a valid string
	shaderstring = (char *)FS_LoadFile(modeinfo->filename, r_main_mempool, false, NULL);
	if (shaderstring)
	{
		if (printfromdisknotice)
			Con_DPrintf("Loading shaders from file %s...\n", modeinfo->filename);
		return shaderstring;
	}
	// fall back to builtinstring
	return Mem_strdup(r_main_mempool, modeinfo->builtinstring);
}

static void R_GLSL_CompilePermutation(r_glsl_permutation_t *p, unsigned int mode, uint64_t permutation)
{
	unsigned i;
	int ubibind;
	int sampler;
	shadermodeinfo_t *modeinfo = &shadermodeinfo[SHADERLANGUAGE_GLSL][mode];
	char *sourcestring;
	char permutationname[256];
	int vertstrings_count = 0;
	int geomstrings_count = 0;
	int fragstrings_count = 0;
	const char *vertstrings_list[32+5+SHADERSTATICPARMS_COUNT+1];
	const char *geomstrings_list[32+5+SHADERSTATICPARMS_COUNT+1];
	const char *fragstrings_list[32+5+SHADERSTATICPARMS_COUNT+1];

	if (p->compiled)
		return;
	p->compiled = true;
	p->program = 0;

	permutationname[0] = 0;
	sourcestring = ShaderModeInfo_GetShaderText(modeinfo, true, false);

	dp_strlcat(permutationname, modeinfo->filename, sizeof(permutationname));

	// we need 140 for r_glsl_skeletal (GL_ARB_uniform_buffer_object)
	if(vid.support.glshaderversion >= 140)
	{
		vertstrings_list[vertstrings_count++] = "#version 140\n";
		geomstrings_list[geomstrings_count++] = "#version 140\n";
		fragstrings_list[fragstrings_count++] = "#version 140\n";
		vertstrings_list[vertstrings_count++] = "#define GLSL140\n";
		geomstrings_list[geomstrings_count++] = "#define GLSL140\n";
		fragstrings_list[fragstrings_count++] = "#define GLSL140\n";
	}
	// if we can do #version 130, we should (this improves quality of offset/reliefmapping thanks to textureGrad)
	else if(vid.support.glshaderversion >= 130)
	{
		vertstrings_list[vertstrings_count++] = "#version 130\n";
		geomstrings_list[geomstrings_count++] = "#version 130\n";
		fragstrings_list[fragstrings_count++] = "#version 130\n";
		vertstrings_list[vertstrings_count++] = "#define GLSL130\n";
		geomstrings_list[geomstrings_count++] = "#define GLSL130\n";
		fragstrings_list[fragstrings_count++] = "#define GLSL130\n";
	}
	// if we can do #version 120, we should (this adds the invariant keyword)
	else if(vid.support.glshaderversion >= 120)
	{
		vertstrings_list[vertstrings_count++] = "#version 120\n";
		geomstrings_list[geomstrings_count++] = "#version 120\n";
		fragstrings_list[fragstrings_count++] = "#version 120\n";
		vertstrings_list[vertstrings_count++] = "#define GLSL120\n";
		geomstrings_list[geomstrings_count++] = "#define GLSL120\n";
		fragstrings_list[fragstrings_count++] = "#define GLSL120\n";
	}
#ifdef _NOSDL
	else
	{
		vertstrings_list[vertstrings_count++] = "#version 100\n";
		geomstrings_list[geomstrings_count++] = "#version 100\n";
		fragstrings_list[fragstrings_count++] = "#version 100\n";
		vertstrings_list[vertstrings_count++] = "#define GLSL120\n";
		geomstrings_list[geomstrings_count++] = "#define GLSL120\n";
		fragstrings_list[fragstrings_count++] = "#define GLSL120\n";
	}
#endif
	// GLES also adds several things from GLSL120
	switch(vid.renderpath)
	{
	case RENDERPATH_GLES2:
		vertstrings_list[vertstrings_count++] = "#define GLES\n";
		geomstrings_list[geomstrings_count++] = "#define GLES\n";
		fragstrings_list[fragstrings_count++] = "#define GLES\n";
		break;
	default:
		break;
	}

	// the first pretext is which type of shader to compile as
	// (later these will all be bound together as a program object)
	vertstrings_list[vertstrings_count++] = "#define VERTEX_SHADER\n";
	geomstrings_list[geomstrings_count++] = "#define GEOMETRY_SHADER\n";
	fragstrings_list[fragstrings_count++] = "#define FRAGMENT_SHADER\n";

	// the second pretext is the mode (for example a light source)
	vertstrings_list[vertstrings_count++] = modeinfo->pretext;
	geomstrings_list[geomstrings_count++] = modeinfo->pretext;
	fragstrings_list[fragstrings_count++] = modeinfo->pretext;
	dp_strlcat(permutationname, modeinfo->name, sizeof(permutationname));

	// now add all the permutation pretexts
	for (i = 0;i < SHADERPERMUTATION_COUNT;i++)
	{
		if (permutation & (1ll<<i))
		{
			vertstrings_list[vertstrings_count++] = shaderpermutationinfo[i].pretext;
			geomstrings_list[geomstrings_count++] = shaderpermutationinfo[i].pretext;
			fragstrings_list[fragstrings_count++] = shaderpermutationinfo[i].pretext;
			dp_strlcat(permutationname, shaderpermutationinfo[i].name, sizeof(permutationname));
		}
		else
		{
			// keep line numbers correct
			vertstrings_list[vertstrings_count++] = "\n";
			geomstrings_list[geomstrings_count++] = "\n";
			fragstrings_list[fragstrings_count++] = "\n";
		}
	}

	// add static parms
	R_CompileShader_AddStaticParms(mode, permutation);
	memcpy((char *)(vertstrings_list + vertstrings_count), shaderstaticparmstrings_list, sizeof(*vertstrings_list) * shaderstaticparms_count);
	vertstrings_count += shaderstaticparms_count;
	memcpy((char *)(geomstrings_list + geomstrings_count), shaderstaticparmstrings_list, sizeof(*vertstrings_list) * shaderstaticparms_count);
	geomstrings_count += shaderstaticparms_count;
	memcpy((char *)(fragstrings_list + fragstrings_count), shaderstaticparmstrings_list, sizeof(*vertstrings_list) * shaderstaticparms_count);
	fragstrings_count += shaderstaticparms_count;

	// now append the shader text itself
	vertstrings_list[vertstrings_count++] = sourcestring;
	geomstrings_list[geomstrings_count++] = sourcestring;
	fragstrings_list[fragstrings_count++] = sourcestring;

	// we don't currently use geometry shaders for anything, so just empty the list
	geomstrings_count = 0;

	// compile the shader program
	if (vertstrings_count + geomstrings_count + fragstrings_count)
		p->program = GL_Backend_CompileProgram(vertstrings_count, vertstrings_list, geomstrings_count, geomstrings_list, fragstrings_count, fragstrings_list);
	if (p->program)
	{
		CHECKGLERROR
		qglUseProgram(p->program);CHECKGLERROR
		// look up all the uniform variable names we care about, so we don't
		// have to look them up every time we set them

#if 0
		// debugging aid
		{
			GLint activeuniformindex = 0;
			GLint numactiveuniforms = 0;
			char uniformname[128];
			GLsizei uniformnamelength = 0;
			GLint uniformsize = 0;
			GLenum uniformtype = 0;
			memset(uniformname, 0, sizeof(uniformname));
			qglGetProgramiv(p->program, GL_ACTIVE_UNIFORMS, &numactiveuniforms);
			Con_Printf("Shader has %i uniforms\n", numactiveuniforms);
			for (activeuniformindex = 0;activeuniformindex < numactiveuniforms;activeuniformindex++)
			{
				qglGetActiveUniform(p->program, activeuniformindex, sizeof(uniformname) - 1, &uniformnamelength, &uniformsize, &uniformtype, uniformname);
				Con_Printf("Uniform %i name \"%s\" size %i type %i\n", (int)activeuniformindex, uniformname, (int)uniformsize, (int)uniformtype);
			}
		}
#endif

		p->loc_Texture_First              = qglGetUniformLocation(p->program, "Texture_First");
		p->loc_Texture_Second             = qglGetUniformLocation(p->program, "Texture_Second");
		p->loc_Texture_GammaRamps         = qglGetUniformLocation(p->program, "Texture_GammaRamps");
		p->loc_Texture_Normal             = qglGetUniformLocation(p->program, "Texture_Normal");
		p->loc_Texture_Color              = qglGetUniformLocation(p->program, "Texture_Color");
		p->loc_Texture_Gloss              = qglGetUniformLocation(p->program, "Texture_Gloss");
		p->loc_Texture_Glow               = qglGetUniformLocation(p->program, "Texture_Glow");
		p->loc_Texture_SecondaryNormal    = qglGetUniformLocation(p->program, "Texture_SecondaryNormal");
		p->loc_Texture_SecondaryColor     = qglGetUniformLocation(p->program, "Texture_SecondaryColor");
		p->loc_Texture_SecondaryGloss     = qglGetUniformLocation(p->program, "Texture_SecondaryGloss");
		p->loc_Texture_SecondaryGlow      = qglGetUniformLocation(p->program, "Texture_SecondaryGlow");
		p->loc_Texture_Pants              = qglGetUniformLocation(p->program, "Texture_Pants");
		p->loc_Texture_Shirt              = qglGetUniformLocation(p->program, "Texture_Shirt");
		p->loc_Texture_FogHeightTexture   = qglGetUniformLocation(p->program, "Texture_FogHeightTexture");
		p->loc_Texture_FogMask            = qglGetUniformLocation(p->program, "Texture_FogMask");
		p->loc_Texture_LightGrid          = qglGetUniformLocation(p->program, "Texture_LightGrid");
		p->loc_Texture_Lightmap           = qglGetUniformLocation(p->program, "Texture_Lightmap");
		p->loc_Texture_Deluxemap          = qglGetUniformLocation(p->program, "Texture_Deluxemap");
		p->loc_Texture_Attenuation        = qglGetUniformLocation(p->program, "Texture_Attenuation");
		p->loc_Texture_Cube               = qglGetUniformLocation(p->program, "Texture_Cube");
		p->loc_Texture_Refraction         = qglGetUniformLocation(p->program, "Texture_Refraction");
		p->loc_Texture_Reflection         = qglGetUniformLocation(p->program, "Texture_Reflection");
		p->loc_Texture_ShadowMap2D        = qglGetUniformLocation(p->program, "Texture_ShadowMap2D");
		p->loc_Texture_CubeProjection     = qglGetUniformLocation(p->program, "Texture_CubeProjection");
		p->loc_Texture_ScreenNormalMap    = qglGetUniformLocation(p->program, "Texture_ScreenNormalMap");
		p->loc_Texture_ScreenDiffuse      = qglGetUniformLocation(p->program, "Texture_ScreenDiffuse");
		p->loc_Texture_ScreenSpecular     = qglGetUniformLocation(p->program, "Texture_ScreenSpecular");
		p->loc_Texture_ReflectMask        = qglGetUniformLocation(p->program, "Texture_ReflectMask");
		p->loc_Texture_ReflectCube        = qglGetUniformLocation(p->program, "Texture_ReflectCube");
		p->loc_Texture_BounceGrid         = qglGetUniformLocation(p->program, "Texture_BounceGrid");
		p->loc_Alpha                      = qglGetUniformLocation(p->program, "Alpha");
		p->loc_BloomBlur_Parameters       = qglGetUniformLocation(p->program, "BloomBlur_Parameters");
		p->loc_ClientTime                 = qglGetUniformLocation(p->program, "ClientTime");
		p->loc_Color_Ambient              = qglGetUniformLocation(p->program, "Color_Ambient");
		p->loc_Color_Diffuse              = qglGetUniformLocation(p->program, "Color_Diffuse");
		p->loc_Color_Specular             = qglGetUniformLocation(p->program, "Color_Specular");
		p->loc_Color_Glow                 = qglGetUniformLocation(p->program, "Color_Glow");
		p->loc_Color_Pants                = qglGetUniformLocation(p->program, "Color_Pants");
		p->loc_Color_Shirt                = qglGetUniformLocation(p->program, "Color_Shirt");
		p->loc_DeferredColor_Ambient      = qglGetUniformLocation(p->program, "DeferredColor_Ambient");
		p->loc_DeferredColor_Diffuse      = qglGetUniformLocation(p->program, "DeferredColor_Diffuse");
		p->loc_DeferredColor_Specular     = qglGetUniformLocation(p->program, "DeferredColor_Specular");
		p->loc_DeferredMod_Diffuse        = qglGetUniformLocation(p->program, "DeferredMod_Diffuse");
		p->loc_DeferredMod_Specular       = qglGetUniformLocation(p->program, "DeferredMod_Specular");
		p->loc_DistortScaleRefractReflect = qglGetUniformLocation(p->program, "DistortScaleRefractReflect");
		p->loc_EyePosition                = qglGetUniformLocation(p->program, "EyePosition");
		p->loc_FogColor                   = qglGetUniformLocation(p->program, "FogColor");
		p->loc_FogHeightFade              = qglGetUniformLocation(p->program, "FogHeightFade");
		p->loc_FogPlane                   = qglGetUniformLocation(p->program, "FogPlane");
		p->loc_FogPlaneViewDist           = qglGetUniformLocation(p->program, "FogPlaneViewDist");
		p->loc_FogRangeRecip              = qglGetUniformLocation(p->program, "FogRangeRecip");
		p->loc_LightColor                 = qglGetUniformLocation(p->program, "LightColor");
		p->loc_LightGridMatrix            = qglGetUniformLocation(p->program, "LightGridMatrix");
		p->loc_LightGridNormalMatrix      = qglGetUniformLocation(p->program, "LightGridNormalMatrix");
		p->loc_LightDir                   = qglGetUniformLocation(p->program, "LightDir");
		p->loc_LightPosition              = qglGetUniformLocation(p->program, "LightPosition");
		p->loc_OffsetMapping_ScaleSteps   = qglGetUniformLocation(p->program, "OffsetMapping_ScaleSteps");
		p->loc_OffsetMapping_LodDistance  = qglGetUniformLocation(p->program, "OffsetMapping_LodDistance");
		p->loc_OffsetMapping_Bias         = qglGetUniformLocation(p->program, "OffsetMapping_Bias");
		p->loc_PixelSize                  = qglGetUniformLocation(p->program, "PixelSize");
		p->loc_ReflectColor               = qglGetUniformLocation(p->program, "ReflectColor");
		p->loc_ReflectFactor              = qglGetUniformLocation(p->program, "ReflectFactor");
		p->loc_ReflectOffset              = qglGetUniformLocation(p->program, "ReflectOffset");
		p->loc_RefractColor               = qglGetUniformLocation(p->program, "RefractColor");
		p->loc_Saturation                 = qglGetUniformLocation(p->program, "Saturation");
		p->loc_ScreenCenterRefractReflect = qglGetUniformLocation(p->program, "ScreenCenterRefractReflect");
		p->loc_ScreenScaleRefractReflect  = qglGetUniformLocation(p->program, "ScreenScaleRefractReflect");
		p->loc_ScreenToDepth              = qglGetUniformLocation(p->program, "ScreenToDepth");
		p->loc_ShadowMap_Parameters       = qglGetUniformLocation(p->program, "ShadowMap_Parameters");
		p->loc_ShadowMap_TextureScale     = qglGetUniformLocation(p->program, "ShadowMap_TextureScale");
		p->loc_SpecularPower              = qglGetUniformLocation(p->program, "SpecularPower");
		p->loc_UserVec1                   = qglGetUniformLocation(p->program, "UserVec1");
		p->loc_UserVec2                   = qglGetUniformLocation(p->program, "UserVec2");
		p->loc_UserVec3                   = qglGetUniformLocation(p->program, "UserVec3");
		p->loc_UserVec4                   = qglGetUniformLocation(p->program, "UserVec4");
		p->loc_ColorFringe                = qglGetUniformLocation(p->program, "ColorFringe");
		p->loc_ViewTintColor              = qglGetUniformLocation(p->program, "ViewTintColor");
		p->loc_ViewToLight                = qglGetUniformLocation(p->program, "ViewToLight");
		p->loc_ModelToLight               = qglGetUniformLocation(p->program, "ModelToLight");
		p->loc_TexMatrix                  = qglGetUniformLocation(p->program, "TexMatrix");
		p->loc_BackgroundTexMatrix        = qglGetUniformLocation(p->program, "BackgroundTexMatrix");
		p->loc_ModelViewMatrix            = qglGetUniformLocation(p->program, "ModelViewMatrix");
		p->loc_ModelViewProjectionMatrix  = qglGetUniformLocation(p->program, "ModelViewProjectionMatrix");
		p->loc_PixelToScreenTexCoord      = qglGetUniformLocation(p->program, "PixelToScreenTexCoord");
		p->loc_ModelToReflectCube         = qglGetUniformLocation(p->program, "ModelToReflectCube");
		p->loc_ShadowMapMatrix            = qglGetUniformLocation(p->program, "ShadowMapMatrix");
		p->loc_BloomColorSubtract         = qglGetUniformLocation(p->program, "BloomColorSubtract");
		p->loc_NormalmapScrollBlend       = qglGetUniformLocation(p->program, "NormalmapScrollBlend");
		p->loc_BounceGridMatrix           = qglGetUniformLocation(p->program, "BounceGridMatrix");
		p->loc_BounceGridIntensity        = qglGetUniformLocation(p->program, "BounceGridIntensity");
		// initialize the samplers to refer to the texture units we use
		p->tex_Texture_First = -1;
		p->tex_Texture_Second = -1;
		p->tex_Texture_GammaRamps = -1;
		p->tex_Texture_Normal = -1;
		p->tex_Texture_Color = -1;
		p->tex_Texture_Gloss = -1;
		p->tex_Texture_Glow = -1;
		p->tex_Texture_SecondaryNormal = -1;
		p->tex_Texture_SecondaryColor = -1;
		p->tex_Texture_SecondaryGloss = -1;
		p->tex_Texture_SecondaryGlow = -1;
		p->tex_Texture_Pants = -1;
		p->tex_Texture_Shirt = -1;
		p->tex_Texture_FogHeightTexture = -1;
		p->tex_Texture_FogMask = -1;
		p->tex_Texture_LightGrid = -1;
		p->tex_Texture_Lightmap = -1;
		p->tex_Texture_Deluxemap = -1;
		p->tex_Texture_Attenuation = -1;
		p->tex_Texture_Cube = -1;
		p->tex_Texture_Refraction = -1;
		p->tex_Texture_Reflection = -1;
		p->tex_Texture_ShadowMap2D = -1;
		p->tex_Texture_CubeProjection = -1;
		p->tex_Texture_ScreenNormalMap = -1;
		p->tex_Texture_ScreenDiffuse = -1;
		p->tex_Texture_ScreenSpecular = -1;
		p->tex_Texture_ReflectMask = -1;
		p->tex_Texture_ReflectCube = -1;
		p->tex_Texture_BounceGrid = -1;
		// bind the texture samplers in use
		sampler = 0;
		if (p->loc_Texture_First           >= 0) {p->tex_Texture_First            = sampler;qglUniform1i(p->loc_Texture_First           , sampler);sampler++;}
		if (p->loc_Texture_Second          >= 0) {p->tex_Texture_Second           = sampler;qglUniform1i(p->loc_Texture_Second          , sampler);sampler++;}
		if (p->loc_Texture_GammaRamps      >= 0) {p->tex_Texture_GammaRamps       = sampler;qglUniform1i(p->loc_Texture_GammaRamps      , sampler);sampler++;}
		if (p->loc_Texture_Normal          >= 0) {p->tex_Texture_Normal           = sampler;qglUniform1i(p->loc_Texture_Normal          , sampler);sampler++;}
		if (p->loc_Texture_Color           >= 0) {p->tex_Texture_Color            = sampler;qglUniform1i(p->loc_Texture_Color           , sampler);sampler++;}
		if (p->loc_Texture_Gloss           >= 0) {p->tex_Texture_Gloss            = sampler;qglUniform1i(p->loc_Texture_Gloss           , sampler);sampler++;}
		if (p->loc_Texture_Glow            >= 0) {p->tex_Texture_Glow             = sampler;qglUniform1i(p->loc_Texture_Glow            , sampler);sampler++;}
		if (p->loc_Texture_SecondaryNormal >= 0) {p->tex_Texture_SecondaryNormal  = sampler;qglUniform1i(p->loc_Texture_SecondaryNormal , sampler);sampler++;}
		if (p->loc_Texture_SecondaryColor  >= 0) {p->tex_Texture_SecondaryColor   = sampler;qglUniform1i(p->loc_Texture_SecondaryColor  , sampler);sampler++;}
		if (p->loc_Texture_SecondaryGloss  >= 0) {p->tex_Texture_SecondaryGloss   = sampler;qglUniform1i(p->loc_Texture_SecondaryGloss  , sampler);sampler++;}
		if (p->loc_Texture_SecondaryGlow   >= 0) {p->tex_Texture_SecondaryGlow    = sampler;qglUniform1i(p->loc_Texture_SecondaryGlow   , sampler);sampler++;}
		if (p->loc_Texture_Pants           >= 0) {p->tex_Texture_Pants            = sampler;qglUniform1i(p->loc_Texture_Pants           , sampler);sampler++;}
		if (p->loc_Texture_Shirt           >= 0) {p->tex_Texture_Shirt            = sampler;qglUniform1i(p->loc_Texture_Shirt           , sampler);sampler++;}
		if (p->loc_Texture_FogHeightTexture>= 0) {p->tex_Texture_FogHeightTexture = sampler;qglUniform1i(p->loc_Texture_FogHeightTexture, sampler);sampler++;}
		if (p->loc_Texture_FogMask         >= 0) {p->tex_Texture_FogMask          = sampler;qglUniform1i(p->loc_Texture_FogMask         , sampler);sampler++;}
		if (p->loc_Texture_LightGrid       >= 0) {p->tex_Texture_LightGrid        = sampler;qglUniform1i(p->loc_Texture_LightGrid       , sampler);sampler++;}
		if (p->loc_Texture_Lightmap        >= 0) {p->tex_Texture_Lightmap         = sampler;qglUniform1i(p->loc_Texture_Lightmap        , sampler);sampler++;}
		if (p->loc_Texture_Deluxemap       >= 0) {p->tex_Texture_Deluxemap        = sampler;qglUniform1i(p->loc_Texture_Deluxemap       , sampler);sampler++;}
		if (p->loc_Texture_Attenuation     >= 0) {p->tex_Texture_Attenuation      = sampler;qglUniform1i(p->loc_Texture_Attenuation     , sampler);sampler++;}
		if (p->loc_Texture_Cube            >= 0) {p->tex_Texture_Cube             = sampler;qglUniform1i(p->loc_Texture_Cube            , sampler);sampler++;}
		if (p->loc_Texture_Refraction      >= 0) {p->tex_Texture_Refraction       = sampler;qglUniform1i(p->loc_Texture_Refraction      , sampler);sampler++;}
		if (p->loc_Texture_Reflection      >= 0) {p->tex_Texture_Reflection       = sampler;qglUniform1i(p->loc_Texture_Reflection      , sampler);sampler++;}
		if (p->loc_Texture_ShadowMap2D     >= 0) {p->tex_Texture_ShadowMap2D      = sampler;qglUniform1i(p->loc_Texture_ShadowMap2D     , sampler);sampler++;}
		if (p->loc_Texture_CubeProjection  >= 0) {p->tex_Texture_CubeProjection   = sampler;qglUniform1i(p->loc_Texture_CubeProjection  , sampler);sampler++;}
		if (p->loc_Texture_ScreenNormalMap >= 0) {p->tex_Texture_ScreenNormalMap  = sampler;qglUniform1i(p->loc_Texture_ScreenNormalMap , sampler);sampler++;}
		if (p->loc_Texture_ScreenDiffuse   >= 0) {p->tex_Texture_ScreenDiffuse    = sampler;qglUniform1i(p->loc_Texture_ScreenDiffuse   , sampler);sampler++;}
		if (p->loc_Texture_ScreenSpecular  >= 0) {p->tex_Texture_ScreenSpecular   = sampler;qglUniform1i(p->loc_Texture_ScreenSpecular  , sampler);sampler++;}
		if (p->loc_Texture_ReflectMask     >= 0) {p->tex_Texture_ReflectMask      = sampler;qglUniform1i(p->loc_Texture_ReflectMask     , sampler);sampler++;}
		if (p->loc_Texture_ReflectCube     >= 0) {p->tex_Texture_ReflectCube      = sampler;qglUniform1i(p->loc_Texture_ReflectCube     , sampler);sampler++;}
		if (p->loc_Texture_BounceGrid      >= 0) {p->tex_Texture_BounceGrid       = sampler;qglUniform1i(p->loc_Texture_BounceGrid      , sampler);sampler++;}
		// get the uniform block indices so we can bind them
		p->ubiloc_Skeletal_Transform12_UniformBlock = -1;
#ifndef USE_GLES2 /* FIXME: GLES3 only */
		p->ubiloc_Skeletal_Transform12_UniformBlock = qglGetUniformBlockIndex(p->program, "Skeletal_Transform12_UniformBlock");
#endif
		// clear the uniform block bindings
		p->ubibind_Skeletal_Transform12_UniformBlock = -1;
		// bind the uniform blocks in use
		ubibind = 0;
#ifndef USE_GLES2 /* FIXME: GLES3 only */
		if (p->ubiloc_Skeletal_Transform12_UniformBlock >= 0) {p->ubibind_Skeletal_Transform12_UniformBlock = ubibind;qglUniformBlockBinding(p->program, p->ubiloc_Skeletal_Transform12_UniformBlock, ubibind);ubibind++;}
#endif
		// we're done compiling and setting up the shader, at least until it is used
		CHECKGLERROR
		Con_DPrintf("^5GLSL shader %s compiled (%i textures).\n", permutationname, sampler);
	}
	else
		Con_Printf("^1GLSL shader %s failed!  some features may not work properly.\n", permutationname);

	// free the strings
	if (sourcestring)
		Mem_Free(sourcestring);
}

static void R_SetupShader_SetPermutationGLSL(unsigned int mode, uint64_t permutation)
{
	r_glsl_permutation_t *perm = R_GLSL_FindPermutation(mode, permutation);
	if (r_glsl_permutation != perm)
	{
		r_glsl_permutation = perm;
		if (!r_glsl_permutation->program)
		{
			if (!r_glsl_permutation->compiled)
			{
				Con_DPrintf("Compiling shader mode %u permutation %" PRIx64 "\n", mode, permutation);
				R_GLSL_CompilePermutation(perm, mode, permutation);
			}
			if (!r_glsl_permutation->program)
			{
				// remove features until we find a valid permutation
				unsigned i;
				for (i = 0;i < SHADERPERMUTATION_COUNT;i++)
				{
					// reduce i more quickly whenever it would not remove any bits
					uint64_t j = 1ll<<(SHADERPERMUTATION_COUNT-1-i);
					if (!(permutation & j))
						continue;
					permutation -= j;
					r_glsl_permutation = R_GLSL_FindPermutation(mode, permutation);
					if (!r_glsl_permutation->compiled)
						R_GLSL_CompilePermutation(perm, mode, permutation);
					if (r_glsl_permutation->program)
						break;
				}
				if (i >= SHADERPERMUTATION_COUNT)
				{
					//Con_Printf("Could not find a working OpenGL 2.0 shader for permutation %s %s\n", shadermodeinfo[mode].filename, shadermodeinfo[mode].pretext);
					r_glsl_permutation = R_GLSL_FindPermutation(mode, permutation);
					qglUseProgram(0);CHECKGLERROR
					return; // no bit left to clear, entire mode is broken
				}
			}
		}
		CHECKGLERROR
		qglUseProgram(r_glsl_permutation->program);CHECKGLERROR
	}
	if (r_glsl_permutation->loc_ModelViewProjectionMatrix >= 0) qglUniformMatrix4fv(r_glsl_permutation->loc_ModelViewProjectionMatrix, 1, false, gl_modelviewprojection16f);
	if (r_glsl_permutation->loc_ModelViewMatrix >= 0) qglUniformMatrix4fv(r_glsl_permutation->loc_ModelViewMatrix, 1, false, gl_modelview16f);
	if (r_glsl_permutation->loc_ClientTime >= 0) qglUniform1f(r_glsl_permutation->loc_ClientTime, cl.time);
	CHECKGLERROR
}

void R_GLSL_Restart_f(cmd_state_t *cmd)
{
	unsigned int i, limit;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		{
			r_glsl_permutation_t *p;
			r_glsl_permutation = NULL;
			limit = (unsigned int)Mem_ExpandableArray_IndexRange(&r_glsl_permutationarray);
			for (i = 0;i < limit;i++)
			{
				if ((p = (r_glsl_permutation_t*)Mem_ExpandableArray_RecordAtIndex(&r_glsl_permutationarray, i)))
				{
					GL_Backend_FreeProgram(p->program);
					Mem_ExpandableArray_FreeRecord(&r_glsl_permutationarray, (void*)p);
				}
			}
			memset(r_glsl_permutationhash, 0, sizeof(r_glsl_permutationhash));
		}
		break;
	}
}

static void R_GLSL_DumpShader_f(cmd_state_t *cmd)
{
	unsigned i;
	int language, mode, dupe;
	char *text;
	shadermodeinfo_t *modeinfo;
	qfile_t *file;

	for (language = 0;language < SHADERLANGUAGE_COUNT;language++)
	{
		modeinfo = shadermodeinfo[language];
		for (mode = 0;mode < SHADERMODE_COUNT;mode++)
		{
			// don't dump the same file multiple times (most or all shaders come from the same file)
			for (dupe = mode - 1;dupe >= 0;dupe--)
				if (!strcmp(modeinfo[mode].filename, modeinfo[dupe].filename))
					break;
			if (dupe >= 0)
				continue;
			text = modeinfo[mode].builtinstring;
			if (!text)
				continue;
			file = FS_OpenRealFile(modeinfo[mode].filename, "w", false);
			if (file)
			{
				FS_Print(file, "/* The engine may define the following macros:\n");
				FS_Print(file, "#define VERTEX_SHADER\n#define GEOMETRY_SHADER\n#define FRAGMENT_SHADER\n");
				for (i = 0;i < SHADERMODE_COUNT;i++)
					FS_Print(file, modeinfo[i].pretext);
				for (i = 0;i < SHADERPERMUTATION_COUNT;i++)
					FS_Print(file, shaderpermutationinfo[i].pretext);
				FS_Print(file, "*/\n");
				FS_Print(file, text);
				FS_Close(file);
				Con_Printf("%s written\n", modeinfo[mode].filename);
			}
			else
				Con_Printf(CON_ERROR "failed to write to %s\n", modeinfo[mode].filename);
		}
	}
}

void R_SetupShader_Generic(rtexture_t *t, qbool usegamma, qbool notrippy, qbool suppresstexalpha)
{
	uint64_t permutation = 0;
	if (r_trippy.integer && !notrippy)
		permutation |= SHADERPERMUTATION_TRIPPY;
	permutation |= SHADERPERMUTATION_VIEWTINT;
	if (t)
		permutation |= SHADERPERMUTATION_DIFFUSE;
	if (usegamma && v_glslgamma_2d.integer && !vid.sRGB2D && r_texture_gammaramps && !vid_gammatables_trivial)
		permutation |= SHADERPERMUTATION_GAMMARAMPS;
	if (suppresstexalpha)
		permutation |= SHADERPERMUTATION_REFLECTCUBE;
	if (vid.allowalphatocoverage)
		GL_AlphaToCoverage(false);
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		R_SetupShader_SetPermutationGLSL(SHADERMODE_GENERIC, permutation);
		if (r_glsl_permutation->tex_Texture_First >= 0)
			R_Mesh_TexBind(r_glsl_permutation->tex_Texture_First, t);
		if (r_glsl_permutation->tex_Texture_GammaRamps >= 0)
			R_Mesh_TexBind(r_glsl_permutation->tex_Texture_GammaRamps, r_texture_gammaramps);
		break;
	}
}

void R_SetupShader_Generic_NoTexture(qbool usegamma, qbool notrippy)
{
	R_SetupShader_Generic(NULL, usegamma, notrippy, false);
}

void R_SetupShader_DepthOrShadow(qbool notrippy, qbool depthrgb, qbool skeletal)
{
	uint64_t permutation = 0;
	if (r_trippy.integer && !notrippy)
		permutation |= SHADERPERMUTATION_TRIPPY;
	if (depthrgb)
		permutation |= SHADERPERMUTATION_DEPTHRGB;
	if (skeletal)
		permutation |= SHADERPERMUTATION_SKELETAL;

	if (vid.allowalphatocoverage)
		GL_AlphaToCoverage(false);
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		R_SetupShader_SetPermutationGLSL(SHADERMODE_DEPTH_OR_SHADOW, permutation);
#ifndef USE_GLES2 /* FIXME: GLES3 only */
		if (r_glsl_permutation->ubiloc_Skeletal_Transform12_UniformBlock >= 0 && rsurface.batchskeletaltransform3x4buffer) qglBindBufferRange(GL_UNIFORM_BUFFER, r_glsl_permutation->ubibind_Skeletal_Transform12_UniformBlock, rsurface.batchskeletaltransform3x4buffer->bufferobject, rsurface.batchskeletaltransform3x4offset, rsurface.batchskeletaltransform3x4size);
#endif
		break;
	}
}

#define BLENDFUNC_ALLOWS_COLORMOD      1
#define BLENDFUNC_ALLOWS_FOG           2
#define BLENDFUNC_ALLOWS_FOG_HACK0     4
#define BLENDFUNC_ALLOWS_FOG_HACKALPHA 8
#define BLENDFUNC_ALLOWS_ANYFOG        (BLENDFUNC_ALLOWS_FOG | BLENDFUNC_ALLOWS_FOG_HACK0 | BLENDFUNC_ALLOWS_FOG_HACKALPHA)
static int R_BlendFuncFlags(int src, int dst)
{
	int r = 0;

	// a blendfunc allows colormod if:
	// a) it can never keep the destination pixel invariant, or
	// b) it can keep the destination pixel invariant, and still can do so if colormodded
	// this is to prevent unintended side effects from colormod

	// a blendfunc allows fog if:
	// blend(fog(src), fog(dst)) == fog(blend(src, dst))
	// this is to prevent unintended side effects from fog

	// these checks are the output of fogeval.pl

	r |= BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_DST_ALPHA && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG_HACK0;
	if(src == GL_DST_ALPHA && dst == GL_ONE_MINUS_DST_ALPHA) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_DST_COLOR && dst == GL_ONE_MINUS_SRC_ALPHA) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_DST_COLOR && dst == GL_ONE_MINUS_SRC_COLOR) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_DST_COLOR && dst == GL_SRC_ALPHA) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_DST_COLOR && dst == GL_SRC_COLOR) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_DST_COLOR && dst == GL_ZERO) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_ONE && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG_HACK0;
	if(src == GL_ONE && dst == GL_ONE_MINUS_SRC_ALPHA) r |= BLENDFUNC_ALLOWS_FOG_HACKALPHA;
	if(src == GL_ONE && dst == GL_ZERO) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ONE_MINUS_DST_ALPHA && dst == GL_DST_ALPHA) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ONE_MINUS_DST_ALPHA && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG_HACK0;
	if(src == GL_ONE_MINUS_DST_COLOR && dst == GL_SRC_COLOR) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ONE_MINUS_SRC_ALPHA && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG_HACK0;
	if(src == GL_ONE_MINUS_SRC_ALPHA && dst == GL_SRC_ALPHA) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ONE_MINUS_SRC_ALPHA && dst == GL_SRC_COLOR) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_ONE_MINUS_SRC_COLOR && dst == GL_SRC_COLOR) r &= ~BLENDFUNC_ALLOWS_COLORMOD;
	if(src == GL_SRC_ALPHA && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG_HACK0;
	if(src == GL_SRC_ALPHA && dst == GL_ONE_MINUS_SRC_ALPHA) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ZERO && dst == GL_ONE) r |= BLENDFUNC_ALLOWS_FOG;
	if(src == GL_ZERO && dst == GL_SRC_COLOR) r &= ~BLENDFUNC_ALLOWS_COLORMOD;

	return r;
}

void R_SetupShader_Surface(const float rtlightambient[3], const float rtlightdiffuse[3], const float rtlightspecular[3], rsurfacepass_t rsurfacepass, int texturenumsurfaces, const msurface_t **texturesurfacelist, void *surfacewaterplane, qbool notrippy, qbool ui)
{
	// select a permutation of the lighting shader appropriate to this
	// combination of texture, entity, light source, and fogging, only use the
	// minimum features necessary to avoid wasting rendering time in the
	// fragment shader on features that are not being used
	uint64_t permutation = 0;
	unsigned int mode = 0;
	int blendfuncflags;
	texture_t *t = rsurface.texture;
	float m16f[16];
	matrix4x4_t tempmatrix;
	r_waterstate_waterplane_t *waterplane = (r_waterstate_waterplane_t *)surfacewaterplane;
	if (r_trippy.integer && !notrippy)
		permutation |= SHADERPERMUTATION_TRIPPY;
	if (t->currentmaterialflags & MATERIALFLAG_ALPHATEST)
		permutation |= SHADERPERMUTATION_ALPHAKILL;
	if (t->currentmaterialflags & MATERIALFLAG_OCCLUDE)
		permutation |= SHADERPERMUTATION_OCCLUDE;
	if (t->r_water_waterscroll[0] && t->r_water_waterscroll[1])
		permutation |= SHADERPERMUTATION_NORMALMAPSCROLLBLEND; // todo: make generic
	if (rsurfacepass == RSURFPASS_BACKGROUND)
	{
		// distorted background
		if (t->currentmaterialflags & MATERIALFLAG_WATERSHADER)
		{
			mode = SHADERMODE_WATER;
			if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
				permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
			if((r_wateralpha.value < 1) && (t->currentmaterialflags & MATERIALFLAG_WATERALPHA))
			{
				// this is the right thing to do for wateralpha
				GL_BlendFunc(GL_ONE, GL_ZERO);
				blendfuncflags = R_BlendFuncFlags(GL_ONE, GL_ZERO);
			}
			else
			{
				// this is the right thing to do for entity alpha
				GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				blendfuncflags = R_BlendFuncFlags(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
		else if (t->currentmaterialflags & MATERIALFLAG_REFRACTION)
		{
			mode = SHADERMODE_REFRACTION;
			if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
				permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			blendfuncflags = R_BlendFuncFlags(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			mode = SHADERMODE_GENERIC;
			permutation |= SHADERPERMUTATION_DIFFUSE | SHADERPERMUTATION_ALPHAKILL;
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			blendfuncflags = R_BlendFuncFlags(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		if (vid.allowalphatocoverage)
			GL_AlphaToCoverage(false);
	}
	else if (rsurfacepass == RSURFPASS_DEFERREDGEOMETRY)
	{
		if (r_glsl_offsetmapping.integer && ((R_TextureFlags(t->nmaptexture) & TEXF_ALPHA) || t->offsetbias != 0.0f))
		{
			switch(t->offsetmapping)
			{
			case OFFSETMAPPING_LINEAR: permutation |= SHADERPERMUTATION_OFFSETMAPPING;break;
			case OFFSETMAPPING_RELIEF: permutation |= SHADERPERMUTATION_OFFSETMAPPING | SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_DEFAULT: permutation |= SHADERPERMUTATION_OFFSETMAPPING;if (r_glsl_offsetmapping_reliefmapping.integer) permutation |= SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_OFF: break;
			}
		}
		if (t->currentmaterialflags & MATERIALFLAG_VERTEXTEXTUREBLEND)
			permutation |= SHADERPERMUTATION_VERTEXTEXTUREBLEND;
		// normalmap (deferred prepass), may use alpha test on diffuse
		mode = SHADERMODE_DEFERREDGEOMETRY;
		GL_BlendFunc(GL_ONE, GL_ZERO);
		blendfuncflags = R_BlendFuncFlags(GL_ONE, GL_ZERO);
		if (vid.allowalphatocoverage)
			GL_AlphaToCoverage(false);
	}
	else if (rsurfacepass == RSURFPASS_RTLIGHT)
	{
		if (r_glsl_offsetmapping.integer && ((R_TextureFlags(t->nmaptexture) & TEXF_ALPHA) || t->offsetbias != 0.0f))
		{
			switch(t->offsetmapping)
			{
			case OFFSETMAPPING_LINEAR: permutation |= SHADERPERMUTATION_OFFSETMAPPING;break;
			case OFFSETMAPPING_RELIEF: permutation |= SHADERPERMUTATION_OFFSETMAPPING | SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_DEFAULT: permutation |= SHADERPERMUTATION_OFFSETMAPPING;if (r_glsl_offsetmapping_reliefmapping.integer) permutation |= SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_OFF: break;
			}
		}
		if (t->currentmaterialflags & MATERIALFLAG_VERTEXTEXTUREBLEND)
			permutation |= SHADERPERMUTATION_VERTEXTEXTUREBLEND;
		if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
			permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
		// light source
		mode = SHADERMODE_LIGHTSOURCE;
		if (rsurface.rtlight->currentcubemap != r_texture_whitecube)
			permutation |= SHADERPERMUTATION_CUBEFILTER;
		if (VectorLength2(rtlightdiffuse) > 0)
			permutation |= SHADERPERMUTATION_DIFFUSE;
		if (VectorLength2(rtlightspecular) > 0)
			permutation |= SHADERPERMUTATION_SPECULAR | SHADERPERMUTATION_DIFFUSE;
		if (r_refdef.fogenabled)
			permutation |= r_texture_fogheighttexture ? SHADERPERMUTATION_FOGHEIGHTTEXTURE : (r_refdef.fogplaneviewabove ? SHADERPERMUTATION_FOGOUTSIDE : SHADERPERMUTATION_FOGINSIDE);
		if (t->colormapping)
			permutation |= SHADERPERMUTATION_COLORMAPPING;
		if (r_shadow_usingshadowmap2d)
		{
			permutation |= SHADERPERMUTATION_SHADOWMAP2D;
			if(r_shadow_shadowmapvsdct)
				permutation |= SHADERPERMUTATION_SHADOWMAPVSDCT;

			if (r_shadow_shadowmap2ddepthbuffer)
				permutation |= SHADERPERMUTATION_DEPTHRGB;
		}
		if (t->reflectmasktexture)
			permutation |= SHADERPERMUTATION_REFLECTCUBE;
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
		blendfuncflags = R_BlendFuncFlags(GL_SRC_ALPHA, GL_ONE);
		if (vid.allowalphatocoverage)
			GL_AlphaToCoverage(false);
	}
	else if (t->currentmaterialflags & MATERIALFLAG_LIGHTGRID)
	{
		if (r_glsl_offsetmapping.integer && ((R_TextureFlags(t->nmaptexture) & TEXF_ALPHA) || t->offsetbias != 0.0f))
		{
			switch(t->offsetmapping)
			{
			case OFFSETMAPPING_LINEAR: permutation |= SHADERPERMUTATION_OFFSETMAPPING;break;
			case OFFSETMAPPING_RELIEF: permutation |= SHADERPERMUTATION_OFFSETMAPPING | SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_DEFAULT: permutation |= SHADERPERMUTATION_OFFSETMAPPING;if (r_glsl_offsetmapping_reliefmapping.integer) permutation |= SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_OFF: break;
			}
		}
		if (t->currentmaterialflags & MATERIALFLAG_VERTEXTEXTUREBLEND)
			permutation |= SHADERPERMUTATION_VERTEXTEXTUREBLEND;
		if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
			permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
		// directional model lighting
		mode = SHADERMODE_LIGHTGRID;
		if ((t->glowtexture || t->backgroundglowtexture) && r_hdr_glowintensity.value > 0 && !gl_lightmaps.integer)
			permutation |= SHADERPERMUTATION_GLOW;
		permutation |= SHADERPERMUTATION_DIFFUSE;
		if (t->glosstexture || t->backgroundglosstexture)
			permutation |= SHADERPERMUTATION_SPECULAR;
		if (r_refdef.fogenabled)
			permutation |= r_texture_fogheighttexture ? SHADERPERMUTATION_FOGHEIGHTTEXTURE : (r_refdef.fogplaneviewabove ? SHADERPERMUTATION_FOGOUTSIDE : SHADERPERMUTATION_FOGINSIDE);
		if (t->colormapping)
			permutation |= SHADERPERMUTATION_COLORMAPPING;
		if (r_shadow_usingshadowmaportho && !(rsurface.ent_flags & RENDER_NOSELFSHADOW))
		{
			permutation |= SHADERPERMUTATION_SHADOWMAPORTHO;
			permutation |= SHADERPERMUTATION_SHADOWMAP2D;

			if (r_shadow_shadowmap2ddepthbuffer)
				permutation |= SHADERPERMUTATION_DEPTHRGB;
		}
		if (t->currentmaterialflags & MATERIALFLAG_REFLECTION)
			permutation |= SHADERPERMUTATION_REFLECTION;
		if (r_shadow_usingdeferredprepass && !(t->currentmaterialflags & MATERIALFLAG_BLENDED))
			permutation |= SHADERPERMUTATION_DEFERREDLIGHTMAP;
		if (t->reflectmasktexture)
			permutation |= SHADERPERMUTATION_REFLECTCUBE;
		if (r_shadow_bouncegrid_state.texture && cl.csqc_vidvars.drawworld && !notrippy)
		{
			permutation |= SHADERPERMUTATION_BOUNCEGRID;
			if (r_shadow_bouncegrid_state.directional)
				permutation |= SHADERPERMUTATION_BOUNCEGRIDDIRECTIONAL;
		}
		GL_BlendFunc(t->currentblendfunc[0], t->currentblendfunc[1]);
		blendfuncflags = R_BlendFuncFlags(t->currentblendfunc[0], t->currentblendfunc[1]);
		// when using alphatocoverage, we don't need alphakill
		if (vid.allowalphatocoverage)
		{
			if (r_transparent_alphatocoverage.integer)
			{
				GL_AlphaToCoverage((t->currentmaterialflags & MATERIALFLAG_ALPHATEST) != 0);
				permutation &= ~SHADERPERMUTATION_ALPHAKILL;
			}
			else
				GL_AlphaToCoverage(false);
		}
	}
	else if (t->currentmaterialflags & MATERIALFLAG_MODELLIGHT)
	{
		if (r_glsl_offsetmapping.integer && ((R_TextureFlags(t->nmaptexture) & TEXF_ALPHA) || t->offsetbias != 0.0f))
		{
			switch(t->offsetmapping)
			{
			case OFFSETMAPPING_LINEAR: permutation |= SHADERPERMUTATION_OFFSETMAPPING;break;
			case OFFSETMAPPING_RELIEF: permutation |= SHADERPERMUTATION_OFFSETMAPPING | SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_DEFAULT: permutation |= SHADERPERMUTATION_OFFSETMAPPING;if (r_glsl_offsetmapping_reliefmapping.integer) permutation |= SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_OFF: break;
			}
		}
		if (t->currentmaterialflags & MATERIALFLAG_VERTEXTEXTUREBLEND)
			permutation |= SHADERPERMUTATION_VERTEXTEXTUREBLEND;
		if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
			permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
		// directional model lighting
		mode = SHADERMODE_LIGHTDIRECTION;
		if ((t->glowtexture || t->backgroundglowtexture) && r_hdr_glowintensity.value > 0 && !gl_lightmaps.integer)
			permutation |= SHADERPERMUTATION_GLOW;
		if (VectorLength2(t->render_modellight_diffuse))
			permutation |= SHADERPERMUTATION_DIFFUSE;
		if (VectorLength2(t->render_modellight_specular) > 0)
			permutation |= SHADERPERMUTATION_SPECULAR;
		if (r_refdef.fogenabled)
			permutation |= r_texture_fogheighttexture ? SHADERPERMUTATION_FOGHEIGHTTEXTURE : (r_refdef.fogplaneviewabove ? SHADERPERMUTATION_FOGOUTSIDE : SHADERPERMUTATION_FOGINSIDE);
		if (t->colormapping)
			permutation |= SHADERPERMUTATION_COLORMAPPING;
		if (r_shadow_usingshadowmaportho && !(rsurface.ent_flags & RENDER_NOSELFSHADOW))
		{
			permutation |= SHADERPERMUTATION_SHADOWMAPORTHO;
			permutation |= SHADERPERMUTATION_SHADOWMAP2D;

			if (r_shadow_shadowmap2ddepthbuffer)
				permutation |= SHADERPERMUTATION_DEPTHRGB;
		}
		if (t->currentmaterialflags & MATERIALFLAG_REFLECTION)
			permutation |= SHADERPERMUTATION_REFLECTION;
		if (r_shadow_usingdeferredprepass && !(t->currentmaterialflags & MATERIALFLAG_BLENDED))
			permutation |= SHADERPERMUTATION_DEFERREDLIGHTMAP;
		if (t->reflectmasktexture)
			permutation |= SHADERPERMUTATION_REFLECTCUBE;
		if (r_shadow_bouncegrid_state.texture && cl.csqc_vidvars.drawworld && !notrippy)
		{
			permutation |= SHADERPERMUTATION_BOUNCEGRID;
			if (r_shadow_bouncegrid_state.directional)
				permutation |= SHADERPERMUTATION_BOUNCEGRIDDIRECTIONAL;
		}
		GL_BlendFunc(t->currentblendfunc[0], t->currentblendfunc[1]);
		blendfuncflags = R_BlendFuncFlags(t->currentblendfunc[0], t->currentblendfunc[1]);
		// when using alphatocoverage, we don't need alphakill
		if (vid.allowalphatocoverage)
		{
			if (r_transparent_alphatocoverage.integer)
			{
				GL_AlphaToCoverage((t->currentmaterialflags & MATERIALFLAG_ALPHATEST) != 0);
				permutation &= ~SHADERPERMUTATION_ALPHAKILL;
			}
			else
				GL_AlphaToCoverage(false);
		}
	}
	else
	{
		if (r_glsl_offsetmapping.integer && ((R_TextureFlags(t->nmaptexture) & TEXF_ALPHA) || t->offsetbias != 0.0f))
		{
			switch(t->offsetmapping)
			{
			case OFFSETMAPPING_LINEAR: permutation |= SHADERPERMUTATION_OFFSETMAPPING;break;
			case OFFSETMAPPING_RELIEF: permutation |= SHADERPERMUTATION_OFFSETMAPPING | SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_DEFAULT: permutation |= SHADERPERMUTATION_OFFSETMAPPING;if (r_glsl_offsetmapping_reliefmapping.integer) permutation |= SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING;break;
			case OFFSETMAPPING_OFF: break;
			}
		}
		if (t->currentmaterialflags & MATERIALFLAG_VERTEXTEXTUREBLEND)
			permutation |= SHADERPERMUTATION_VERTEXTEXTUREBLEND;
		if (t->currentmaterialflags & MATERIALFLAG_ALPHAGEN_VERTEX)
			permutation |= SHADERPERMUTATION_ALPHAGEN_VERTEX;
		// lightmapped wall
		if ((t->glowtexture || t->backgroundglowtexture) && r_hdr_glowintensity.value > 0 && !gl_lightmaps.integer)
			permutation |= SHADERPERMUTATION_GLOW;
		if (r_refdef.fogenabled && !ui)
			permutation |= r_texture_fogheighttexture ? SHADERPERMUTATION_FOGHEIGHTTEXTURE : (r_refdef.fogplaneviewabove ? SHADERPERMUTATION_FOGOUTSIDE : SHADERPERMUTATION_FOGINSIDE);
		if (t->colormapping)
			permutation |= SHADERPERMUTATION_COLORMAPPING;
		if (r_shadow_usingshadowmaportho && !(rsurface.ent_flags & RENDER_NOSELFSHADOW))
		{
			permutation |= SHADERPERMUTATION_SHADOWMAPORTHO;
			permutation |= SHADERPERMUTATION_SHADOWMAP2D;

			if (r_shadow_shadowmap2ddepthbuffer)
				permutation |= SHADERPERMUTATION_DEPTHRGB;
		}
		if (t->currentmaterialflags & MATERIALFLAG_REFLECTION)
			permutation |= SHADERPERMUTATION_REFLECTION;
		if (r_shadow_usingdeferredprepass && !(t->currentmaterialflags & MATERIALFLAG_BLENDED))
			permutation |= SHADERPERMUTATION_DEFERREDLIGHTMAP;
		if (t->reflectmasktexture)
			permutation |= SHADERPERMUTATION_REFLECTCUBE;
		if (r_glsl_deluxemapping.integer >= 1 && rsurface.uselightmaptexture && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brushq3.deluxemapping)
		{
			// deluxemapping (light direction texture)
			if (rsurface.uselightmaptexture && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brushq3.deluxemapping && r_refdef.scene.worldmodel->brushq3.deluxemapping_modelspace)
				mode = SHADERMODE_LIGHTDIRECTIONMAP_MODELSPACE;
			else
				mode = SHADERMODE_LIGHTDIRECTIONMAP_TANGENTSPACE;
			permutation |= SHADERPERMUTATION_DIFFUSE;
			if (VectorLength2(t->render_lightmap_specular) > 0)
				permutation |= SHADERPERMUTATION_SPECULAR | SHADERPERMUTATION_DIFFUSE;
		}
		else if (r_glsl_deluxemapping.integer >= 2)
		{
			// fake deluxemapping (uniform light direction in tangentspace)
			if (rsurface.uselightmaptexture)
				mode = SHADERMODE_LIGHTDIRECTIONMAP_FORCED_LIGHTMAP;
			else
				mode = SHADERMODE_LIGHTDIRECTIONMAP_FORCED_VERTEXCOLOR;
			permutation |= SHADERPERMUTATION_DIFFUSE;
			if (VectorLength2(t->render_lightmap_specular) > 0)
				permutation |= SHADERPERMUTATION_SPECULAR | SHADERPERMUTATION_DIFFUSE;
		}
		else if (rsurface.uselightmaptexture)
		{
			// ordinary lightmapping (q1bsp, q3bsp)
			mode = SHADERMODE_LIGHTMAP;
		}
		else
		{
			// ordinary vertex coloring (q3bsp)
			mode = SHADERMODE_VERTEXCOLOR;
		}
		if (r_shadow_bouncegrid_state.texture && cl.csqc_vidvars.drawworld && !notrippy)
		{
			permutation |= SHADERPERMUTATION_BOUNCEGRID;
			if (r_shadow_bouncegrid_state.directional)
				permutation |= SHADERPERMUTATION_BOUNCEGRIDDIRECTIONAL;
		}
		GL_BlendFunc(t->currentblendfunc[0], t->currentblendfunc[1]);
		blendfuncflags = R_BlendFuncFlags(t->currentblendfunc[0], t->currentblendfunc[1]);
		// when using alphatocoverage, we don't need alphakill
		if (vid.allowalphatocoverage)
		{
			if (r_transparent_alphatocoverage.integer)
			{
				GL_AlphaToCoverage((t->currentmaterialflags & MATERIALFLAG_ALPHATEST) != 0);
				permutation &= ~SHADERPERMUTATION_ALPHAKILL;
			}
			else
				GL_AlphaToCoverage(false);
		}
	}
	if(!(blendfuncflags & BLENDFUNC_ALLOWS_ANYFOG))
		permutation &= ~(SHADERPERMUTATION_FOGHEIGHTTEXTURE | SHADERPERMUTATION_FOGOUTSIDE | SHADERPERMUTATION_FOGINSIDE);
	if(blendfuncflags & BLENDFUNC_ALLOWS_FOG_HACKALPHA && !ui)
		permutation |= SHADERPERMUTATION_FOGALPHAHACK;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR | (rsurface.modellightmapcolor4f ? BATCHNEED_ARRAY_VERTEXCOLOR : 0) | BATCHNEED_ARRAY_TEXCOORD | (rsurface.uselightmaptexture ? BATCHNEED_ARRAY_LIGHTMAP : 0) | BATCHNEED_ALLOWMULTIDRAW, texturenumsurfaces, texturesurfacelist);
		RSurf_UploadBuffersForBatch();
		// this has to be after RSurf_PrepareVerticesForBatch
		if (rsurface.batchskeletaltransform3x4buffer)
			permutation |= SHADERPERMUTATION_SKELETAL;
		R_SetupShader_SetPermutationGLSL(mode, permutation);
#ifndef USE_GLES2 /* FIXME: GLES3 only */
		if (r_glsl_permutation->ubiloc_Skeletal_Transform12_UniformBlock >= 0 && rsurface.batchskeletaltransform3x4buffer) qglBindBufferRange(GL_UNIFORM_BUFFER, r_glsl_permutation->ubibind_Skeletal_Transform12_UniformBlock, rsurface.batchskeletaltransform3x4buffer->bufferobject, rsurface.batchskeletaltransform3x4offset, rsurface.batchskeletaltransform3x4size);
#endif
		if (r_glsl_permutation->loc_ModelToReflectCube >= 0) {Matrix4x4_ToArrayFloatGL(&rsurface.matrix, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_ModelToReflectCube, 1, false, m16f);}
		if (mode == SHADERMODE_LIGHTSOURCE)
		{
			if (r_glsl_permutation->loc_ModelToLight >= 0) {Matrix4x4_ToArrayFloatGL(&rsurface.entitytolight, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_ModelToLight, 1, false, m16f);}
			if (r_glsl_permutation->loc_LightPosition >= 0) qglUniform3f(r_glsl_permutation->loc_LightPosition, rsurface.entitylightorigin[0], rsurface.entitylightorigin[1], rsurface.entitylightorigin[2]);
			if (r_glsl_permutation->loc_LightColor >= 0) qglUniform3f(r_glsl_permutation->loc_LightColor, 1, 1, 1); // DEPRECATED
			if (r_glsl_permutation->loc_Color_Ambient >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Ambient, rtlightambient[0], rtlightambient[1], rtlightambient[2]);
			if (r_glsl_permutation->loc_Color_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Diffuse, rtlightdiffuse[0], rtlightdiffuse[1], rtlightdiffuse[2]);
			if (r_glsl_permutation->loc_Color_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Specular, rtlightspecular[0], rtlightspecular[1], rtlightspecular[2]);

			// additive passes are only darkened by fog, not tinted
			if (r_glsl_permutation->loc_FogColor >= 0)
				qglUniform3f(r_glsl_permutation->loc_FogColor, 0, 0, 0);
			if (r_glsl_permutation->loc_SpecularPower >= 0) qglUniform1f(r_glsl_permutation->loc_SpecularPower, t->specularpower * (r_shadow_glossexact.integer ? 0.25f : 1.0f) - 1.0f);
		}
		else
		{
			if (mode == SHADERMODE_FLATCOLOR)
			{
				if (r_glsl_permutation->loc_Color_Ambient >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Ambient, t->render_modellight_ambient[0], t->render_modellight_ambient[1], t->render_modellight_ambient[2]);
			}
			else if (mode == SHADERMODE_LIGHTGRID)
			{
				if (r_glsl_permutation->loc_Color_Ambient >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Ambient, t->render_lightmap_ambient[0], t->render_lightmap_ambient[1], t->render_lightmap_ambient[2]);
				if (r_glsl_permutation->loc_Color_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Diffuse, t->render_lightmap_diffuse[0], t->render_lightmap_diffuse[1], t->render_lightmap_diffuse[2]);
				if (r_glsl_permutation->loc_Color_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Specular, t->render_lightmap_specular[0], t->render_lightmap_specular[1], t->render_lightmap_specular[2]);
				// other LightGrid uniforms handled below
			}
			else if (mode == SHADERMODE_LIGHTDIRECTION)
			{
				if (r_glsl_permutation->loc_Color_Ambient >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Ambient, t->render_modellight_ambient[0], t->render_modellight_ambient[1], t->render_modellight_ambient[2]);
				if (r_glsl_permutation->loc_Color_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Diffuse, t->render_modellight_diffuse[0], t->render_modellight_diffuse[1], t->render_modellight_diffuse[2]);
				if (r_glsl_permutation->loc_Color_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Specular, t->render_modellight_specular[0], t->render_modellight_specular[1], t->render_modellight_specular[2]);
				if (r_glsl_permutation->loc_DeferredMod_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_DeferredMod_Diffuse, t->render_rtlight_diffuse[0], t->render_rtlight_diffuse[1], t->render_rtlight_diffuse[2]);
				if (r_glsl_permutation->loc_DeferredMod_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_DeferredMod_Specular, t->render_rtlight_specular[0], t->render_rtlight_specular[1], t->render_rtlight_specular[2]);
				if (r_glsl_permutation->loc_LightColor >= 0) qglUniform3f(r_glsl_permutation->loc_LightColor, 1, 1, 1); // DEPRECATED
				if (r_glsl_permutation->loc_LightDir >= 0) qglUniform3f(r_glsl_permutation->loc_LightDir, t->render_modellight_lightdir_local[0], t->render_modellight_lightdir_local[1], t->render_modellight_lightdir_local[2]);
			}
			else
			{
				if (r_glsl_permutation->loc_Color_Ambient >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Ambient, t->render_lightmap_ambient[0], t->render_lightmap_ambient[1], t->render_lightmap_ambient[2]);
				if (r_glsl_permutation->loc_Color_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Diffuse, t->render_lightmap_diffuse[0], t->render_lightmap_diffuse[1], t->render_lightmap_diffuse[2]);
				if (r_glsl_permutation->loc_Color_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Specular, t->render_lightmap_specular[0], t->render_lightmap_specular[1], t->render_lightmap_specular[2]);
				if (r_glsl_permutation->loc_DeferredMod_Diffuse >= 0) qglUniform3f(r_glsl_permutation->loc_DeferredMod_Diffuse, t->render_rtlight_diffuse[0], t->render_rtlight_diffuse[1], t->render_rtlight_diffuse[2]);
				if (r_glsl_permutation->loc_DeferredMod_Specular >= 0) qglUniform3f(r_glsl_permutation->loc_DeferredMod_Specular, t->render_rtlight_specular[0], t->render_rtlight_specular[1], t->render_rtlight_specular[2]);
			}
			// additive passes are only darkened by fog, not tinted
			if (r_glsl_permutation->loc_FogColor >= 0 && !ui)
			{
				if(blendfuncflags & BLENDFUNC_ALLOWS_FOG_HACK0)
					qglUniform3f(r_glsl_permutation->loc_FogColor, 0, 0, 0);
				else
					qglUniform3f(r_glsl_permutation->loc_FogColor, r_refdef.fogcolor[0], r_refdef.fogcolor[1], r_refdef.fogcolor[2]);
			}
			if (r_glsl_permutation->loc_DistortScaleRefractReflect >= 0) qglUniform4f(r_glsl_permutation->loc_DistortScaleRefractReflect, r_water_refractdistort.value * t->refractfactor, r_water_refractdistort.value * t->refractfactor, r_water_reflectdistort.value * t->reflectfactor, r_water_reflectdistort.value * t->reflectfactor);
			if (r_glsl_permutation->loc_ScreenScaleRefractReflect >= 0) qglUniform4f(r_glsl_permutation->loc_ScreenScaleRefractReflect, r_fb.water.screenscale[0], r_fb.water.screenscale[1], r_fb.water.screenscale[0], r_fb.water.screenscale[1]);
			if (r_glsl_permutation->loc_ScreenCenterRefractReflect >= 0) qglUniform4f(r_glsl_permutation->loc_ScreenCenterRefractReflect, r_fb.water.screencenter[0], r_fb.water.screencenter[1], r_fb.water.screencenter[0], r_fb.water.screencenter[1]);
			if (r_glsl_permutation->loc_RefractColor >= 0) qglUniform4f(r_glsl_permutation->loc_RefractColor, t->refractcolor4f[0], t->refractcolor4f[1], t->refractcolor4f[2], t->refractcolor4f[3] * t->currentalpha);
			if (r_glsl_permutation->loc_ReflectColor >= 0) qglUniform4f(r_glsl_permutation->loc_ReflectColor, t->reflectcolor4f[0], t->reflectcolor4f[1], t->reflectcolor4f[2], t->reflectcolor4f[3] * t->currentalpha);
			if (r_glsl_permutation->loc_ReflectFactor >= 0) qglUniform1f(r_glsl_permutation->loc_ReflectFactor, t->reflectmax - t->reflectmin);
			if (r_glsl_permutation->loc_ReflectOffset >= 0) qglUniform1f(r_glsl_permutation->loc_ReflectOffset, t->reflectmin);
			if (r_glsl_permutation->loc_SpecularPower >= 0) qglUniform1f(r_glsl_permutation->loc_SpecularPower, t->specularpower * (r_shadow_glossexact.integer ? 0.25f : 1.0f) - 1.0f);
			if (r_glsl_permutation->loc_NormalmapScrollBlend >= 0) qglUniform2f(r_glsl_permutation->loc_NormalmapScrollBlend, t->r_water_waterscroll[0], t->r_water_waterscroll[1]);
		}
		if (r_glsl_permutation->loc_TexMatrix >= 0) {Matrix4x4_ToArrayFloatGL(&t->currenttexmatrix, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_TexMatrix, 1, false, m16f);}
		if (r_glsl_permutation->loc_BackgroundTexMatrix >= 0) {Matrix4x4_ToArrayFloatGL(&t->currentbackgroundtexmatrix, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_BackgroundTexMatrix, 1, false, m16f);}
		if (r_glsl_permutation->loc_ShadowMapMatrix >= 0) {Matrix4x4_ToArrayFloatGL(&r_shadow_shadowmapmatrix, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_ShadowMapMatrix, 1, false, m16f);}
		if (permutation & SHADERPERMUTATION_SHADOWMAPORTHO)
		{
			if (r_glsl_permutation->loc_ShadowMap_TextureScale >= 0) qglUniform4f(r_glsl_permutation->loc_ShadowMap_TextureScale, r_shadow_modelshadowmap_texturescale[0], r_shadow_modelshadowmap_texturescale[1], r_shadow_modelshadowmap_texturescale[2], r_shadow_modelshadowmap_texturescale[3]);
			if (r_glsl_permutation->loc_ShadowMap_Parameters >= 0) qglUniform4f(r_glsl_permutation->loc_ShadowMap_Parameters, r_shadow_modelshadowmap_parameters[0], r_shadow_modelshadowmap_parameters[1], r_shadow_modelshadowmap_parameters[2], r_shadow_modelshadowmap_parameters[3]);
		}
		else
		{
			if (r_glsl_permutation->loc_ShadowMap_TextureScale >= 0) qglUniform4f(r_glsl_permutation->loc_ShadowMap_TextureScale, r_shadow_lightshadowmap_texturescale[0], r_shadow_lightshadowmap_texturescale[1], r_shadow_lightshadowmap_texturescale[2], r_shadow_lightshadowmap_texturescale[3]);
			if (r_glsl_permutation->loc_ShadowMap_Parameters >= 0) qglUniform4f(r_glsl_permutation->loc_ShadowMap_Parameters, r_shadow_lightshadowmap_parameters[0], r_shadow_lightshadowmap_parameters[1], r_shadow_lightshadowmap_parameters[2], r_shadow_lightshadowmap_parameters[3]);
		}

		if (r_glsl_permutation->loc_Color_Glow >= 0) qglUniform3f(r_glsl_permutation->loc_Color_Glow, t->render_glowmod[0], t->render_glowmod[1], t->render_glowmod[2]);
		if (r_glsl_permutation->loc_Alpha >= 0) qglUniform1f(r_glsl_permutation->loc_Alpha, t->currentalpha * ((t->basematerialflags & MATERIALFLAG_WATERSHADER && r_fb.water.enabled && !r_refdef.view.isoverlay) ? t->r_water_wateralpha : 1));
		if (r_glsl_permutation->loc_EyePosition >= 0) qglUniform3f(r_glsl_permutation->loc_EyePosition, rsurface.localvieworigin[0], rsurface.localvieworigin[1], rsurface.localvieworigin[2]);
		if (r_glsl_permutation->loc_Color_Pants >= 0)
		{
			if (t->pantstexture)
				qglUniform3f(r_glsl_permutation->loc_Color_Pants, t->render_colormap_pants[0], t->render_colormap_pants[1], t->render_colormap_pants[2]);
			else
				qglUniform3f(r_glsl_permutation->loc_Color_Pants, 0, 0, 0);
		}
		if (r_glsl_permutation->loc_Color_Shirt >= 0)
		{
			if (t->shirttexture)
				qglUniform3f(r_glsl_permutation->loc_Color_Shirt, t->render_colormap_shirt[0], t->render_colormap_shirt[1], t->render_colormap_shirt[2]);
			else
				qglUniform3f(r_glsl_permutation->loc_Color_Shirt, 0, 0, 0);
		}
		if (r_glsl_permutation->loc_FogPlane >= 0) qglUniform4f(r_glsl_permutation->loc_FogPlane, rsurface.fogplane[0], rsurface.fogplane[1], rsurface.fogplane[2], rsurface.fogplane[3]);
		if (r_glsl_permutation->loc_FogPlaneViewDist >= 0) qglUniform1f(r_glsl_permutation->loc_FogPlaneViewDist, rsurface.fogplaneviewdist);
		if (r_glsl_permutation->loc_FogRangeRecip >= 0) qglUniform1f(r_glsl_permutation->loc_FogRangeRecip, rsurface.fograngerecip);
		if (r_glsl_permutation->loc_FogHeightFade >= 0) qglUniform1f(r_glsl_permutation->loc_FogHeightFade, rsurface.fogheightfade);
		if (r_glsl_permutation->loc_OffsetMapping_ScaleSteps >= 0) qglUniform4f(r_glsl_permutation->loc_OffsetMapping_ScaleSteps,
				r_glsl_offsetmapping_scale.value*t->offsetscale,
				max(1, (permutation & SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING) ? r_glsl_offsetmapping_reliefmapping_steps.integer : r_glsl_offsetmapping_steps.integer),
				1.0 / max(1, (permutation & SHADERPERMUTATION_OFFSETMAPPING_RELIEFMAPPING) ? r_glsl_offsetmapping_reliefmapping_steps.integer : r_glsl_offsetmapping_steps.integer),
				max(1, r_glsl_offsetmapping_reliefmapping_refinesteps.integer)
			);
		if (r_glsl_permutation->loc_OffsetMapping_LodDistance >= 0) qglUniform1f(r_glsl_permutation->loc_OffsetMapping_LodDistance, r_glsl_offsetmapping_lod_distance.integer * r_refdef.view.quality);
		if (r_glsl_permutation->loc_OffsetMapping_Bias >= 0) qglUniform1f(r_glsl_permutation->loc_OffsetMapping_Bias, t->offsetbias);
		if (r_glsl_permutation->loc_ScreenToDepth >= 0) qglUniform2f(r_glsl_permutation->loc_ScreenToDepth, r_refdef.view.viewport.screentodepth[0], r_refdef.view.viewport.screentodepth[1]);
		if (r_glsl_permutation->loc_PixelToScreenTexCoord >= 0) qglUniform2f(r_glsl_permutation->loc_PixelToScreenTexCoord, 1.0f/r_fb.screentexturewidth, 1.0f/r_fb.screentextureheight);
		if (r_glsl_permutation->loc_BounceGridMatrix >= 0) {Matrix4x4_Concat(&tempmatrix, &r_shadow_bouncegrid_state.matrix, &rsurface.matrix);Matrix4x4_ToArrayFloatGL(&tempmatrix, m16f);qglUniformMatrix4fv(r_glsl_permutation->loc_BounceGridMatrix, 1, false, m16f);}
		if (r_glsl_permutation->loc_BounceGridIntensity >= 0) qglUniform1f(r_glsl_permutation->loc_BounceGridIntensity, r_shadow_bouncegrid_state.intensity*r_refdef.view.colorscale);
		if (r_glsl_permutation->loc_LightGridMatrix >= 0 && r_refdef.scene.worldmodel)
		{
			float m9f[9];
			Matrix4x4_Concat(&tempmatrix, &r_refdef.scene.worldmodel->brushq3.lightgridworldtotexturematrix, &rsurface.matrix);
			Matrix4x4_ToArrayFloatGL(&tempmatrix, m16f);
			qglUniformMatrix4fv(r_glsl_permutation->loc_LightGridMatrix, 1, false, m16f);
			Matrix4x4_Normalize3(&tempmatrix, &rsurface.matrix);
			Matrix4x4_ToArrayFloatGL(&tempmatrix, m16f);
			m9f[0] = m16f[0];m9f[1] = m16f[1];m9f[2] = m16f[2];
			m9f[3] = m16f[4];m9f[4] = m16f[5];m9f[5] = m16f[6];
			m9f[6] = m16f[8];m9f[7] = m16f[9];m9f[8] = m16f[10];
			qglUniformMatrix3fv(r_glsl_permutation->loc_LightGridNormalMatrix, 1, false, m9f);
		}

		if (r_glsl_permutation->tex_Texture_First           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_First            , r_texture_white                                     );
		if (r_glsl_permutation->tex_Texture_Second          >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Second           , r_texture_white                                     );
		if (r_glsl_permutation->tex_Texture_GammaRamps      >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_GammaRamps       , r_texture_gammaramps                                );
		if (r_glsl_permutation->tex_Texture_Normal          >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Normal           , t->nmaptexture                       );
		if (r_glsl_permutation->tex_Texture_Color           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Color            , t->basetexture                       );
		if (r_glsl_permutation->tex_Texture_Gloss           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Gloss            , t->glosstexture                      );
		if (r_glsl_permutation->tex_Texture_Glow            >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Glow             , t->glowtexture                       );
		if (r_glsl_permutation->tex_Texture_SecondaryNormal >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_SecondaryNormal  , t->backgroundnmaptexture             );
		if (r_glsl_permutation->tex_Texture_SecondaryColor  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_SecondaryColor   , t->backgroundbasetexture             );
		if (r_glsl_permutation->tex_Texture_SecondaryGloss  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_SecondaryGloss   , t->backgroundglosstexture            );
		if (r_glsl_permutation->tex_Texture_SecondaryGlow   >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_SecondaryGlow    , t->backgroundglowtexture             );
		if (r_glsl_permutation->tex_Texture_Pants           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Pants            , t->pantstexture                      );
		if (r_glsl_permutation->tex_Texture_Shirt           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Shirt            , t->shirttexture                      );
		if (r_glsl_permutation->tex_Texture_ReflectMask     >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ReflectMask      , t->reflectmasktexture                );
		if (r_glsl_permutation->tex_Texture_ReflectCube     >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ReflectCube      , t->reflectcubetexture ? t->reflectcubetexture : r_texture_whitecube);
		if (r_glsl_permutation->tex_Texture_FogHeightTexture>= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_FogHeightTexture , r_texture_fogheighttexture                          );
		if (r_glsl_permutation->tex_Texture_FogMask         >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_FogMask          , r_texture_fogattenuation                            );
		if (r_glsl_permutation->tex_Texture_Lightmap        >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Lightmap         , rsurface.lightmaptexture ? rsurface.lightmaptexture : r_texture_white);
		if (r_glsl_permutation->tex_Texture_Deluxemap       >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Deluxemap        , rsurface.deluxemaptexture ? rsurface.deluxemaptexture : r_texture_blanknormalmap);
		if (r_glsl_permutation->tex_Texture_Attenuation     >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Attenuation      , r_shadow_attenuationgradienttexture                 );
		if (rsurfacepass == RSURFPASS_BACKGROUND)
		{
			if (r_glsl_permutation->tex_Texture_Refraction  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Refraction        , waterplane->rt_refraction ? waterplane->rt_refraction->colortexture[0] : r_texture_black);
			if (r_glsl_permutation->tex_Texture_First       >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_First             , waterplane->rt_camera ? waterplane->rt_camera->colortexture[0] : r_texture_black);
			if (r_glsl_permutation->tex_Texture_Reflection  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Reflection        , waterplane->rt_reflection ? waterplane->rt_reflection->colortexture[0] : r_texture_black);
		}
		else
		{
			if (r_glsl_permutation->tex_Texture_Reflection >= 0 && waterplane) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Reflection        , waterplane->rt_reflection ? waterplane->rt_reflection->colortexture[0] : r_texture_black);
		}
		if (r_glsl_permutation->tex_Texture_ScreenNormalMap >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ScreenNormalMap   , r_shadow_prepassgeometrynormalmaptexture            );
		if (r_glsl_permutation->tex_Texture_ScreenDiffuse   >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ScreenDiffuse     , r_shadow_prepasslightingdiffusetexture              );
		if (r_glsl_permutation->tex_Texture_ScreenSpecular  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ScreenSpecular    , r_shadow_prepasslightingspeculartexture             );
		if (rsurface.rtlight || (r_shadow_usingshadowmaportho && !(rsurface.ent_flags & RENDER_NOSELFSHADOW)))
		{
			if (r_glsl_permutation->tex_Texture_ShadowMap2D     >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ShadowMap2D, r_shadow_shadowmap2ddepthtexture                           );
			if (rsurface.rtlight)
			{
				if (r_glsl_permutation->tex_Texture_Cube            >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Cube              , rsurface.rtlight->currentcubemap                    );
				if (r_glsl_permutation->tex_Texture_CubeProjection  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_CubeProjection    , r_shadow_shadowmapvsdcttexture                      );
			}
		}
		if (r_glsl_permutation->tex_Texture_BounceGrid  >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_BounceGrid, r_shadow_bouncegrid_state.texture);
		if (r_glsl_permutation->tex_Texture_LightGrid   >= 0 && r_refdef.scene.worldmodel) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_LightGrid, r_refdef.scene.worldmodel->brushq3.lightgridtexture);
		CHECKGLERROR
		break;
	}
}

void R_SetupShader_DeferredLight(const rtlight_t *rtlight)
{
	// select a permutation of the lighting shader appropriate to this
	// combination of texture, entity, light source, and fogging, only use the
	// minimum features necessary to avoid wasting rendering time in the
	// fragment shader on features that are not being used
	uint64_t permutation = 0;
	unsigned int mode = 0;
	const float *lightcolorbase = rtlight->currentcolor;
	float ambientscale = rtlight->ambientscale;
	float diffusescale = rtlight->diffusescale;
	float specularscale = rtlight->specularscale;
	// this is the location of the light in view space
	vec3_t viewlightorigin;
	// this transforms from view space (camera) to light space (cubemap)
	matrix4x4_t viewtolight;
	matrix4x4_t lighttoview;
	float viewtolight16f[16];
	// light source
	mode = SHADERMODE_DEFERREDLIGHTSOURCE;
	if (rtlight->currentcubemap != r_texture_whitecube)
		permutation |= SHADERPERMUTATION_CUBEFILTER;
	if (diffusescale > 0)
		permutation |= SHADERPERMUTATION_DIFFUSE;
	if (specularscale > 0 && r_shadow_gloss.integer > 0)
		permutation |= SHADERPERMUTATION_SPECULAR | SHADERPERMUTATION_DIFFUSE;
	if (r_shadow_usingshadowmap2d)
	{
		permutation |= SHADERPERMUTATION_SHADOWMAP2D;
		if (r_shadow_shadowmapvsdct)
			permutation |= SHADERPERMUTATION_SHADOWMAPVSDCT;

		if (r_shadow_shadowmap2ddepthbuffer)
			permutation |= SHADERPERMUTATION_DEPTHRGB;
	}
	if (vid.allowalphatocoverage)
		GL_AlphaToCoverage(false);
	Matrix4x4_Transform(&r_refdef.view.viewport.viewmatrix, rtlight->shadoworigin, viewlightorigin);
	Matrix4x4_Concat(&lighttoview, &r_refdef.view.viewport.viewmatrix, &rtlight->matrix_lighttoworld);
	Matrix4x4_Invert_Full(&viewtolight, &lighttoview);
	Matrix4x4_ToArrayFloatGL(&viewtolight, viewtolight16f);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		R_SetupShader_SetPermutationGLSL(mode, permutation);
		if (r_glsl_permutation->loc_LightPosition             >= 0) qglUniform3f(       r_glsl_permutation->loc_LightPosition            , viewlightorigin[0], viewlightorigin[1], viewlightorigin[2]);
		if (r_glsl_permutation->loc_ViewToLight               >= 0) qglUniformMatrix4fv(r_glsl_permutation->loc_ViewToLight              , 1, false, viewtolight16f);
		if (r_glsl_permutation->loc_DeferredColor_Ambient     >= 0) qglUniform3f(       r_glsl_permutation->loc_DeferredColor_Ambient    , lightcolorbase[0] * ambientscale , lightcolorbase[1] * ambientscale , lightcolorbase[2] * ambientscale );
		if (r_glsl_permutation->loc_DeferredColor_Diffuse     >= 0) qglUniform3f(       r_glsl_permutation->loc_DeferredColor_Diffuse    , lightcolorbase[0] * diffusescale , lightcolorbase[1] * diffusescale , lightcolorbase[2] * diffusescale );
		if (r_glsl_permutation->loc_DeferredColor_Specular    >= 0) qglUniform3f(       r_glsl_permutation->loc_DeferredColor_Specular   , lightcolorbase[0] * specularscale, lightcolorbase[1] * specularscale, lightcolorbase[2] * specularscale);
		if (r_glsl_permutation->loc_ShadowMap_TextureScale    >= 0) qglUniform4f(       r_glsl_permutation->loc_ShadowMap_TextureScale   , r_shadow_lightshadowmap_texturescale[0], r_shadow_lightshadowmap_texturescale[1], r_shadow_lightshadowmap_texturescale[2], r_shadow_lightshadowmap_texturescale[3]);
		if (r_glsl_permutation->loc_ShadowMap_Parameters      >= 0) qglUniform4f(       r_glsl_permutation->loc_ShadowMap_Parameters     , r_shadow_lightshadowmap_parameters[0], r_shadow_lightshadowmap_parameters[1], r_shadow_lightshadowmap_parameters[2], r_shadow_lightshadowmap_parameters[3]);
		if (r_glsl_permutation->loc_SpecularPower             >= 0) qglUniform1f(       r_glsl_permutation->loc_SpecularPower            , (r_shadow_gloss.integer == 2 ? r_shadow_gloss2exponent.value : r_shadow_glossexponent.value) * (r_shadow_glossexact.integer ? 0.25f : 1.0f) - 1.0f);
		if (r_glsl_permutation->loc_ScreenToDepth             >= 0) qglUniform2f(       r_glsl_permutation->loc_ScreenToDepth            , r_refdef.view.viewport.screentodepth[0], r_refdef.view.viewport.screentodepth[1]);
		if (r_glsl_permutation->loc_PixelToScreenTexCoord     >= 0) qglUniform2f(       r_glsl_permutation->loc_PixelToScreenTexCoord    , 1.0f/r_fb.screentexturewidth, 1.0f/r_fb.screentextureheight);

		if (r_glsl_permutation->tex_Texture_Attenuation       >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Attenuation        , r_shadow_attenuationgradienttexture                 );
		if (r_glsl_permutation->tex_Texture_ScreenNormalMap   >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ScreenNormalMap    , r_shadow_prepassgeometrynormalmaptexture            );
		if (r_glsl_permutation->tex_Texture_Cube              >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Cube               , rsurface.rtlight->currentcubemap                    );
		if (r_glsl_permutation->tex_Texture_ShadowMap2D       >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_ShadowMap2D        , r_shadow_shadowmap2ddepthtexture                    );
		if (r_glsl_permutation->tex_Texture_CubeProjection    >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_CubeProjection     , r_shadow_shadowmapvsdcttexture                      );
		break;
	}
}

#define SKINFRAME_HASH 1024

typedef struct
{
	unsigned int loadsequence; // incremented each level change
	memexpandablearray_t array;
	skinframe_t *hash[SKINFRAME_HASH];
}
r_skinframe_t;
r_skinframe_t r_skinframe;

void R_SkinFrame_PrepareForPurge(void)
{
	r_skinframe.loadsequence++;
	// wrap it without hitting zero
	if (r_skinframe.loadsequence >= 200)
		r_skinframe.loadsequence = 1;
}

void R_SkinFrame_MarkUsed(skinframe_t *skinframe)
{
	if (!skinframe)
		return;
	// mark the skinframe as used for the purging code
	skinframe->loadsequence = r_skinframe.loadsequence;
}

void R_SkinFrame_PurgeSkinFrame(skinframe_t *s)
{
	if (s == NULL)
		return;
	if (s->merged == s->base)
		s->merged = NULL;
	R_PurgeTexture(s->stain); s->stain = NULL;
	R_PurgeTexture(s->merged); s->merged = NULL;
	R_PurgeTexture(s->base); s->base = NULL;
	R_PurgeTexture(s->pants); s->pants = NULL;
	R_PurgeTexture(s->shirt); s->shirt = NULL;
	R_PurgeTexture(s->nmap); s->nmap = NULL;
	R_PurgeTexture(s->gloss); s->gloss = NULL;
	R_PurgeTexture(s->glow); s->glow = NULL;
	R_PurgeTexture(s->fog); s->fog = NULL;
	R_PurgeTexture(s->reflect); s->reflect = NULL;
	s->loadsequence = 0;
}

void R_SkinFrame_Purge(void)
{
	int i;
	skinframe_t *s;
	for (i = 0;i < SKINFRAME_HASH;i++)
	{
		for (s = r_skinframe.hash[i];s;s = s->next)
		{
			if (s->loadsequence && s->loadsequence != r_skinframe.loadsequence)
				R_SkinFrame_PurgeSkinFrame(s);
		}
	}
}

skinframe_t *R_SkinFrame_FindNextByName( skinframe_t *last, const char *name ) {
	skinframe_t *item;
	char basename[MAX_QPATH];

	Image_StripImageExtension(name, basename, sizeof(basename));

	if( last == NULL ) {
		int hashindex;
		hashindex = CRC_Block((unsigned char *)basename, strlen(basename)) & (SKINFRAME_HASH - 1);
		item = r_skinframe.hash[hashindex];
	} else {
		item = last->next;
	}

	// linearly search through the hash bucket
	for( ; item ; item = item->next ) {
		if( !strcmp( item->basename, basename ) ) {
			return item;
		}
	}
	return NULL;
}

skinframe_t *R_SkinFrame_Find(const char *name, int textureflags, int comparewidth, int compareheight, int comparecrc, qbool add)
{
	skinframe_t *item;
	int compareflags = textureflags & TEXF_IMPORTANTBITS;
	int hashindex;
	char basename[MAX_QPATH];

	Image_StripImageExtension(name, basename, sizeof(basename));

	hashindex = CRC_Block((unsigned char *)basename, strlen(basename)) & (SKINFRAME_HASH - 1);
	for (item = r_skinframe.hash[hashindex];item;item = item->next)
		if (!strcmp(item->basename, basename) &&
			item->textureflags == compareflags &&
			item->comparewidth == comparewidth &&
			item->compareheight == compareheight &&
			item->comparecrc == comparecrc)
			break;

	if (!item)
	{
		if (!add)
			return NULL;
		item = (skinframe_t *)Mem_ExpandableArray_AllocRecord(&r_skinframe.array);
		memset(item, 0, sizeof(*item));
		dp_strlcpy(item->basename, basename, sizeof(item->basename));
		item->textureflags = compareflags;
		item->comparewidth = comparewidth;
		item->compareheight = compareheight;
		item->comparecrc = comparecrc;
		item->next = r_skinframe.hash[hashindex];
		r_skinframe.hash[hashindex] = item;
	}
	else if (textureflags & TEXF_FORCE_RELOAD)
		R_SkinFrame_PurgeSkinFrame(item);

	R_SkinFrame_MarkUsed(item);
	return item;
}

#define R_SKINFRAME_LOAD_AVERAGE_COLORS(cnt, getpixel) \
	{ \
		unsigned long long avgcolor[5], wsum; \
		int pix, comp, w; \
		avgcolor[0] = 0; \
		avgcolor[1] = 0; \
		avgcolor[2] = 0; \
		avgcolor[3] = 0; \
		avgcolor[4] = 0; \
		wsum = 0; \
		for(pix = 0; pix < cnt; ++pix) \
		{ \
			w = 0; \
			for(comp = 0; comp < 3; ++comp) \
				w += getpixel; \
			if(w) /* ignore perfectly black pixels because that is better for model skins */ \
			{ \
				++wsum; \
				/* comp = 3; -- not needed, comp is always 3 when we get here */ \
				w = getpixel; \
				for(comp = 0; comp < 3; ++comp) \
					avgcolor[comp] += getpixel * w; \
				avgcolor[3] += w; \
			} \
			/* comp = 3; -- not needed, comp is always 3 when we get here */ \
			avgcolor[4] += getpixel; \
		} \
		if(avgcolor[3] == 0) /* no pixels seen? even worse */ \
			avgcolor[3] = 1; \
		skinframe->avgcolor[0] = avgcolor[2] / (255.0 * avgcolor[3]); \
		skinframe->avgcolor[1] = avgcolor[1] / (255.0 * avgcolor[3]); \
		skinframe->avgcolor[2] = avgcolor[0] / (255.0 * avgcolor[3]); \
		skinframe->avgcolor[3] = avgcolor[4] / (255.0 * cnt); \
	}

skinframe_t *R_SkinFrame_LoadExternal(const char *name, int textureflags, qbool complain, qbool fallbacknotexture)
{
	skinframe_t *skinframe;

	if (cls.state == ca_dedicated)
		return NULL;

	// return an existing skinframe if already loaded
	skinframe = R_SkinFrame_Find(name, textureflags, 0, 0, 0, false);
	if (skinframe && skinframe->base)
		return skinframe;

	// if the skinframe doesn't exist this will create it
	return R_SkinFrame_LoadExternal_SkinFrame(skinframe, name, textureflags, complain, fallbacknotexture);
}

extern cvar_t gl_picmip;
skinframe_t *R_SkinFrame_LoadExternal_SkinFrame(skinframe_t *skinframe, const char *name, int textureflags, qbool complain, qbool fallbacknotexture)
{
	int j;
	unsigned char *pixels;
	unsigned char *bumppixels;
	unsigned char *basepixels = NULL;
	int basepixels_width = 0;
	int basepixels_height = 0;
	rtexture_t *ddsbase = NULL;
	qbool ddshasalpha = false;
	float ddsavgcolor[4];
	char basename[MAX_QPATH];
	int miplevel = R_PicmipForFlags(textureflags);
	int savemiplevel = miplevel;
	int mymiplevel;
	char vabuf[1024];

	if (cls.state == ca_dedicated)
		return NULL;

	Image_StripImageExtension(name, basename, sizeof(basename));

	// check for DDS texture file first
	if (!r_loaddds || !(ddsbase = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s.dds", basename), vid.sRGB3D, textureflags, &ddshasalpha, ddsavgcolor, miplevel, false)))
	{
		basepixels = loadimagepixelsbgra(name, complain, true, false, &miplevel);
		if (basepixels == NULL && fallbacknotexture)
			basepixels = Image_GenerateNoTexture();
		if (basepixels == NULL)
			return NULL;
	}

	// FIXME handle miplevel

	if (developer_loading.integer)
		Con_Printf("loading skin \"%s\"\n", name);

	// we've got some pixels to store, so really allocate this new texture now
	if (!skinframe)
		skinframe = R_SkinFrame_Find(name, textureflags, 0, 0, 0, true);
	textureflags &= ~TEXF_FORCE_RELOAD;
	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = false;
	// we could store the q2animname here too

	if (ddsbase)
	{
		skinframe->base = ddsbase;
		skinframe->hasalpha = ddshasalpha;
		VectorCopy(ddsavgcolor, skinframe->avgcolor);
		if (r_loadfog && skinframe->hasalpha)
			skinframe->fog = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_mask.dds", skinframe->basename), false, textureflags | TEXF_ALPHA, NULL, NULL, miplevel, true);
		//Con_Printf("Texture %s has average colors %f %f %f alpha %f\n", name, skinframe->avgcolor[0], skinframe->avgcolor[1], skinframe->avgcolor[2], skinframe->avgcolor[3]);
	}
	else
	{
		basepixels_width = image_width;
		basepixels_height = image_height;
		skinframe->base = R_LoadTexture2D (r_main_texturepool, skinframe->basename, basepixels_width, basepixels_height, basepixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags & (gl_texturecompression_color.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), miplevel, NULL);
		if (textureflags & TEXF_ALPHA)
		{
			for (j = 3;j < basepixels_width * basepixels_height * 4;j += 4)
			{
				if (basepixels[j] < 255)
				{
					skinframe->hasalpha = true;
					break;
				}
			}
			if (r_loadfog && skinframe->hasalpha)
			{
				// has transparent pixels
				pixels = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
				for (j = 0;j < image_width * image_height * 4;j += 4)
				{
					pixels[j+0] = 255;
					pixels[j+1] = 255;
					pixels[j+2] = 255;
					pixels[j+3] = basepixels[j+3];
				}
				skinframe->fog = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_mask", skinframe->basename), image_width, image_height, pixels, TEXTYPE_BGRA, textureflags & (gl_texturecompression_color.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), miplevel, NULL);
				Mem_Free(pixels);
			}
		}
		R_SKINFRAME_LOAD_AVERAGE_COLORS(basepixels_width * basepixels_height, basepixels[4 * pix + comp]);
#ifndef USE_GLES2
		//Con_Printf("Texture %s has average colors %f %f %f alpha %f\n", name, skinframe->avgcolor[0], skinframe->avgcolor[1], skinframe->avgcolor[2], skinframe->avgcolor[3]);
		if (r_savedds && skinframe->base)
			R_SaveTextureDDSFile(skinframe->base, va(vabuf, sizeof(vabuf), "dds/%s.dds", skinframe->basename), r_texture_dds_save.integer < 2, skinframe->hasalpha);
		if (r_savedds && skinframe->fog)
			R_SaveTextureDDSFile(skinframe->fog, va(vabuf, sizeof(vabuf), "dds/%s_mask.dds", skinframe->basename), r_texture_dds_save.integer < 2, true);
#endif
	}

	if (r_loaddds)
	{
		mymiplevel = savemiplevel;
		if (r_loadnormalmap)
			skinframe->nmap = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_norm.dds", skinframe->basename), false, (TEXF_ALPHA | textureflags) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP), NULL, NULL, mymiplevel, true);
		skinframe->glow = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_glow.dds", skinframe->basename), vid.sRGB3D, textureflags, NULL, NULL, mymiplevel, true);
		if (r_loadgloss)
			skinframe->gloss = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_gloss.dds", skinframe->basename), vid.sRGB3D, textureflags, NULL, NULL, mymiplevel, true);
		skinframe->pants = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_pants.dds", skinframe->basename), vid.sRGB3D, textureflags, NULL, NULL, mymiplevel, true);
		skinframe->shirt = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_shirt.dds", skinframe->basename), vid.sRGB3D, textureflags, NULL, NULL, mymiplevel, true);
		skinframe->reflect = R_LoadTextureDDSFile(r_main_texturepool, va(vabuf, sizeof(vabuf), "dds/%s_reflect.dds", skinframe->basename), vid.sRGB3D, textureflags, NULL, NULL, mymiplevel, true);
	}

	// _norm is the name used by tenebrae and has been adopted as standard
	if (r_loadnormalmap && skinframe->nmap == NULL)
	{
		mymiplevel = savemiplevel;
		if ((pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_norm", skinframe->basename), false, false, false, &mymiplevel)) != NULL)
		{
			skinframe->nmap = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nmap", skinframe->basename), image_width, image_height, pixels, TEXTYPE_BGRA, (TEXF_ALPHA | textureflags) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP) & (gl_texturecompression_normal.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
			Mem_Free(pixels);
			pixels = NULL;
		}
		else if (r_shadow_bumpscale_bumpmap.value > 0 && (bumppixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_bump", skinframe->basename), false, false, false, &mymiplevel)) != NULL)
		{
			pixels = (unsigned char *)Mem_Alloc(tempmempool, image_width * image_height * 4);
			Image_HeightmapToNormalmap_BGRA(bumppixels, pixels, image_width, image_height, false, r_shadow_bumpscale_bumpmap.value);
			skinframe->nmap = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nmap", skinframe->basename), image_width, image_height, pixels, TEXTYPE_BGRA, (TEXF_ALPHA | textureflags) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP) & (gl_texturecompression_normal.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
			Mem_Free(pixels);
			Mem_Free(bumppixels);
		}
		else if (r_shadow_bumpscale_basetexture.value > 0)
		{
			pixels = (unsigned char *)Mem_Alloc(tempmempool, basepixels_width * basepixels_height * 4);
			Image_HeightmapToNormalmap_BGRA(basepixels, pixels, basepixels_width, basepixels_height, false, r_shadow_bumpscale_basetexture.value);
			skinframe->nmap = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nmap", skinframe->basename), basepixels_width, basepixels_height, pixels, TEXTYPE_BGRA, (TEXF_ALPHA | textureflags) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP) & (gl_texturecompression_normal.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
			Mem_Free(pixels);
		}
#ifndef USE_GLES2
		if (r_savedds && skinframe->nmap)
			R_SaveTextureDDSFile(skinframe->nmap, va(vabuf, sizeof(vabuf), "dds/%s_norm.dds", skinframe->basename), r_texture_dds_save.integer < 2, true);
#endif
	}

	// _luma is supported only for tenebrae compatibility
	// _blend and .blend are supported only for Q3 & QL compatibility, this hack can be removed if better Q3 shader support is implemented
	// _glow is the preferred name
	mymiplevel = savemiplevel;
	if (skinframe->glow == NULL && ((pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_glow", skinframe->basename), false, false, false, &mymiplevel)) || (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s.blend", skinframe->basename), false, false, false, &mymiplevel)) || (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_blend", skinframe->basename), false, false, false, &mymiplevel)) || (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_luma", skinframe->basename), false, false, false, &mymiplevel))))
	{
		skinframe->glow = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_glow", skinframe->basename), image_width, image_height, pixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags & (gl_texturecompression_glow.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
#ifndef USE_GLES2
		if (r_savedds && skinframe->glow)
			R_SaveTextureDDSFile(skinframe->glow, va(vabuf, sizeof(vabuf), "dds/%s_glow.dds", skinframe->basename), r_texture_dds_save.integer < 2, true);
#endif
		Mem_Free(pixels);pixels = NULL;
	}

	mymiplevel = savemiplevel;
	if (skinframe->gloss == NULL && r_loadgloss && (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_gloss", skinframe->basename), false, false, false, &mymiplevel)))
	{
		skinframe->gloss = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_gloss", skinframe->basename), image_width, image_height, pixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, (TEXF_ALPHA | textureflags) & (gl_texturecompression_gloss.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
#ifndef USE_GLES2
		if (r_savedds && skinframe->gloss)
			R_SaveTextureDDSFile(skinframe->gloss, va(vabuf, sizeof(vabuf), "dds/%s_gloss.dds", skinframe->basename), r_texture_dds_save.integer < 2, true);
#endif
		Mem_Free(pixels);
		pixels = NULL;
	}

	mymiplevel = savemiplevel;
	if (skinframe->pants == NULL && (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_pants", skinframe->basename), false, false, false, &mymiplevel)))
	{
		skinframe->pants = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_pants", skinframe->basename), image_width, image_height, pixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags & (gl_texturecompression_color.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
#ifndef USE_GLES2
		if (r_savedds && skinframe->pants)
			R_SaveTextureDDSFile(skinframe->pants, va(vabuf, sizeof(vabuf), "dds/%s_pants.dds", skinframe->basename), r_texture_dds_save.integer < 2, false);
#endif
		Mem_Free(pixels);
		pixels = NULL;
	}

	mymiplevel = savemiplevel;
	if (skinframe->shirt == NULL && (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_shirt", skinframe->basename), false, false, false, &mymiplevel)))
	{
		skinframe->shirt = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_shirt", skinframe->basename), image_width, image_height, pixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags & (gl_texturecompression_color.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
#ifndef USE_GLES2
		if (r_savedds && skinframe->shirt)
			R_SaveTextureDDSFile(skinframe->shirt, va(vabuf, sizeof(vabuf), "dds/%s_shirt.dds", skinframe->basename), r_texture_dds_save.integer < 2, false);
#endif
		Mem_Free(pixels);
		pixels = NULL;
	}

	mymiplevel = savemiplevel;
	if (skinframe->reflect == NULL && (pixels = loadimagepixelsbgra(va(vabuf, sizeof(vabuf), "%s_reflect", skinframe->basename), false, false, false, &mymiplevel)))
	{
		skinframe->reflect = R_LoadTexture2D (r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_reflect", skinframe->basename), image_width, image_height, pixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags & (gl_texturecompression_reflectmask.integer && gl_texturecompression.integer ? ~0 : ~TEXF_COMPRESS), mymiplevel, NULL);
#ifndef USE_GLES2
		if (r_savedds && skinframe->reflect)
			R_SaveTextureDDSFile(skinframe->reflect, va(vabuf, sizeof(vabuf), "dds/%s_reflect.dds", skinframe->basename), r_texture_dds_save.integer < 2, true);
#endif
		Mem_Free(pixels);
		pixels = NULL;
	}

	if (basepixels)
		Mem_Free(basepixels);

	return skinframe;
}

skinframe_t *R_SkinFrame_LoadInternalBGRA(const char *name, int textureflags, const unsigned char *skindata, int width, int height, int comparewidth, int compareheight, int comparecrc, qbool sRGB)
{
	int i;
	skinframe_t *skinframe;
	char vabuf[1024];

	if (cls.state == ca_dedicated)
		return NULL;

	// if already loaded just return it, otherwise make a new skinframe
	skinframe = R_SkinFrame_Find(name, textureflags, comparewidth, compareheight, comparecrc, true);
	if (skinframe->base)
		return skinframe;
	textureflags &= ~TEXF_FORCE_RELOAD;

	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = false;

	// if no data was provided, then clearly the caller wanted to get a blank skinframe
	if (!skindata)
		return NULL;

	if (developer_loading.integer)
		Con_Printf("loading 32bit skin \"%s\"\n", name);

	if (r_loadnormalmap && r_shadow_bumpscale_basetexture.value > 0)
	{
		unsigned char *a = (unsigned char *)Mem_Alloc(tempmempool, width * height * 8);
		unsigned char *b = a + width * height * 4;
		Image_HeightmapToNormalmap_BGRA(skindata, b, width, height, false, r_shadow_bumpscale_basetexture.value);
		skinframe->nmap = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nmap", skinframe->basename), width, height, b, TEXTYPE_BGRA, (textureflags | TEXF_ALPHA) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP), -1, NULL);
		Mem_Free(a);
	}
	skinframe->base = skinframe->merged = R_LoadTexture2D(r_main_texturepool, skinframe->basename, width, height, skindata, sRGB ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, textureflags, -1, NULL);
	if (textureflags & TEXF_ALPHA)
	{
		for (i = 3;i < width * height * 4;i += 4)
		{
			if (skindata[i] < 255)
			{
				skinframe->hasalpha = true;
				break;
			}
		}
		if (r_loadfog && skinframe->hasalpha)
		{
			unsigned char *fogpixels = (unsigned char *)Mem_Alloc(tempmempool, width * height * 4);
			memcpy(fogpixels, skindata, width * height * 4);
			for (i = 0;i < width * height * 4;i += 4)
				fogpixels[i] = fogpixels[i+1] = fogpixels[i+2] = 255;
			skinframe->fog = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_fog", skinframe->basename), width, height, fogpixels, TEXTYPE_BGRA, textureflags, -1, NULL);
			Mem_Free(fogpixels);
		}
	}

	R_SKINFRAME_LOAD_AVERAGE_COLORS(width * height, skindata[4 * pix + comp]);
	//Con_Printf("Texture %s has average colors %f %f %f alpha %f\n", name, skinframe->avgcolor[0], skinframe->avgcolor[1], skinframe->avgcolor[2], skinframe->avgcolor[3]);

	return skinframe;
}

skinframe_t *R_SkinFrame_LoadInternalQuake(const char *name, int textureflags, int loadpantsandshirt, int loadglowtexture, const unsigned char *skindata, int width, int height)
{
	int i;
	int featuresmask;
	skinframe_t *skinframe;

	if (cls.state == ca_dedicated)
		return NULL;

	// if already loaded just return it, otherwise make a new skinframe
	skinframe = R_SkinFrame_Find(name, textureflags, width, height, skindata ? CRC_Block(skindata, width*height) : 0, true);
	if (skinframe->base)
		return skinframe;
	//textureflags &= ~TEXF_FORCE_RELOAD;

	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = false;

	// if no data was provided, then clearly the caller wanted to get a blank skinframe
	if (!skindata)
		return NULL;

	if (developer_loading.integer)
		Con_Printf("loading quake skin \"%s\"\n", name);

	// we actually don't upload anything until the first use, because mdl skins frequently go unused, and are almost never used in both modes (colormapped and non-colormapped)
	skinframe->qpixels = (unsigned char *)Mem_Alloc(r_main_mempool, width*height); // FIXME LEAK
	memcpy(skinframe->qpixels, skindata, width*height);
	skinframe->qwidth = width;
	skinframe->qheight = height;

	featuresmask = 0;
	for (i = 0;i < width * height;i++)
		featuresmask |= palette_featureflags[skindata[i]];

	skinframe->hasalpha = false;
	// fence textures
	if (name[0] == '{')
		skinframe->hasalpha = true;
	skinframe->qhascolormapping = loadpantsandshirt && (featuresmask & (PALETTEFEATURE_PANTS | PALETTEFEATURE_SHIRT));
	skinframe->qgeneratenmap = r_shadow_bumpscale_basetexture.value > 0;
	skinframe->qgeneratemerged = true;
	skinframe->qgeneratebase = skinframe->qhascolormapping;
	skinframe->qgenerateglow = loadglowtexture && (featuresmask & PALETTEFEATURE_GLOW);

	R_SKINFRAME_LOAD_AVERAGE_COLORS(width * height, ((unsigned char *)palette_bgra_complete)[skindata[pix]*4 + comp]);
	//Con_Printf("Texture %s has average colors %f %f %f alpha %f\n", name, skinframe->avgcolor[0], skinframe->avgcolor[1], skinframe->avgcolor[2], skinframe->avgcolor[3]);

	return skinframe;
}

static void R_SkinFrame_GenerateTexturesFromQPixels(skinframe_t *skinframe, qbool colormapped)
{
	int width;
	int height;
	unsigned char *skindata;
	char vabuf[1024];

	if (!skinframe->qpixels)
		return;

	if (!skinframe->qhascolormapping)
		colormapped = false;

	if (colormapped)
	{
		if (!skinframe->qgeneratebase)
			return;
	}
	else
	{
		if (!skinframe->qgeneratemerged)
			return;
	}

	width = skinframe->qwidth;
	height = skinframe->qheight;
	skindata = skinframe->qpixels;

	if (skinframe->qgeneratenmap)
	{
		unsigned char *a, *b;
		skinframe->qgeneratenmap = false;
		a = (unsigned char *)Mem_Alloc(tempmempool, width * height * 8);
		b = a + width * height * 4;
		// use either a custom palette or the quake palette
		Image_Copy8bitBGRA(skindata, a, width * height, palette_bgra_complete);
		Image_HeightmapToNormalmap_BGRA(a, b, width, height, false, r_shadow_bumpscale_basetexture.value);
		skinframe->nmap = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nmap", skinframe->basename), width, height, b, TEXTYPE_BGRA, (skinframe->textureflags | TEXF_ALPHA) & (r_mipnormalmaps.integer ? ~0 : ~TEXF_MIPMAP), -1, NULL);
		Mem_Free(a);
	}

	if (skinframe->qgenerateglow)
	{
		skinframe->qgenerateglow = false;
		if (skinframe->hasalpha) // fence textures
			skinframe->glow = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_glow", skinframe->basename), width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags | TEXF_ALPHA, -1, palette_bgra_onlyfullbrights_transparent); // glow
		else
			skinframe->glow = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_glow", skinframe->basename), width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags, -1, palette_bgra_onlyfullbrights); // glow
	}

	if (colormapped)
	{
		skinframe->qgeneratebase = false;
		skinframe->base  = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_nospecial", skinframe->basename), width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags, -1, skinframe->glow ? palette_bgra_nocolormapnofullbrights : palette_bgra_nocolormap);
		skinframe->pants = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_pants", skinframe->basename), width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags, -1, palette_bgra_pantsaswhite);
		skinframe->shirt = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_shirt", skinframe->basename), width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags, -1, palette_bgra_shirtaswhite);
	}
	else
	{
		skinframe->qgeneratemerged = false;
		if (skinframe->hasalpha) // fence textures
			skinframe->merged = R_LoadTexture2D(r_main_texturepool, skinframe->basename, width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags | TEXF_ALPHA, -1, skinframe->glow ? palette_bgra_nofullbrights_transparent : palette_bgra_transparent);
		else
			skinframe->merged = R_LoadTexture2D(r_main_texturepool, skinframe->basename, width, height, skindata, vid.sRGB3D ? TEXTYPE_SRGB_PALETTE : TEXTYPE_PALETTE, skinframe->textureflags, -1, skinframe->glow ? palette_bgra_nofullbrights : palette_bgra_complete);
	}

	if (!skinframe->qgeneratemerged && !skinframe->qgeneratebase)
	{
		Mem_Free(skinframe->qpixels);
		skinframe->qpixels = NULL;
	}
}

skinframe_t *R_SkinFrame_LoadInternal8bit(const char *name, int textureflags, const unsigned char *skindata, int width, int height, const unsigned int *palette, const unsigned int *alphapalette)
{
	int i;
	skinframe_t *skinframe;
	char vabuf[1024];

	if (cls.state == ca_dedicated)
		return NULL;

	// if already loaded just return it, otherwise make a new skinframe
	skinframe = R_SkinFrame_Find(name, textureflags, width, height, skindata ? CRC_Block(skindata, width*height) : 0, true);
	if (skinframe->base)
		return skinframe;
	textureflags &= ~TEXF_FORCE_RELOAD;

	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = false;

	// if no data was provided, then clearly the caller wanted to get a blank skinframe
	if (!skindata)
		return NULL;

	if (developer_loading.integer)
		Con_Printf("loading embedded 8bit image \"%s\"\n", name);

	skinframe->base = skinframe->merged = R_LoadTexture2D(r_main_texturepool, skinframe->basename, width, height, skindata, TEXTYPE_PALETTE, textureflags, -1, palette);
	if ((textureflags & TEXF_ALPHA) && alphapalette)
	{
		for (i = 0;i < width * height;i++)
		{
			if (((unsigned char *)palette)[skindata[i]*4+3] < 255)
			{
				skinframe->hasalpha = true;
				break;
			}
		}
		if (r_loadfog && skinframe->hasalpha)
			skinframe->fog = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "%s_fog", skinframe->basename), width, height, skindata, TEXTYPE_PALETTE, textureflags, -1, alphapalette);
	}

	R_SKINFRAME_LOAD_AVERAGE_COLORS(width * height, ((unsigned char *)palette)[skindata[pix]*4 + comp]);
	//Con_Printf("Texture %s has average colors %f %f %f alpha %f\n", name, skinframe->avgcolor[0], skinframe->avgcolor[1], skinframe->avgcolor[2], skinframe->avgcolor[3]);

	return skinframe;
}

skinframe_t *R_SkinFrame_LoadMissing(void)
{
	skinframe_t *skinframe;

	if (cls.state == ca_dedicated)
		return NULL;

	skinframe = R_SkinFrame_Find("missing", TEXF_FORCENEAREST, 0, 0, 0, true);
	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = false;

	skinframe->avgcolor[0] = rand() / RAND_MAX;
	skinframe->avgcolor[1] = rand() / RAND_MAX;
	skinframe->avgcolor[2] = rand() / RAND_MAX;
	skinframe->avgcolor[3] = 1;

	return skinframe;
}

skinframe_t *R_SkinFrame_LoadNoTexture(void)
{
	if (cls.state == ca_dedicated)
		return NULL;

	return R_SkinFrame_LoadInternalBGRA("notexture", TEXF_FORCENEAREST, Image_GenerateNoTexture(), 16, 16, 0, 0, 0, false);
}

skinframe_t *R_SkinFrame_LoadInternalUsingTexture(const char *name, int textureflags, rtexture_t *tex, int width, int height, qbool sRGB)
{
	skinframe_t *skinframe;
	if (cls.state == ca_dedicated)
		return NULL;
	// if already loaded just return it, otherwise make a new skinframe
	skinframe = R_SkinFrame_Find(name, textureflags, width, height, 0, true);
	if (skinframe->base)
		return skinframe;
	textureflags &= ~TEXF_FORCE_RELOAD;
	skinframe->stain = NULL;
	skinframe->merged = NULL;
	skinframe->base = NULL;
	skinframe->pants = NULL;
	skinframe->shirt = NULL;
	skinframe->nmap = NULL;
	skinframe->gloss = NULL;
	skinframe->glow = NULL;
	skinframe->fog = NULL;
	skinframe->reflect = NULL;
	skinframe->hasalpha = (textureflags & TEXF_ALPHA) != 0;
	// if no data was provided, then clearly the caller wanted to get a blank skinframe
	if (!tex)
		return NULL;
	if (developer_loading.integer)
		Con_Printf("loading 32bit skin \"%s\"\n", name);
	skinframe->base = skinframe->merged = tex;
	Vector4Set(skinframe->avgcolor, 1, 1, 1, 1); // bogus placeholder
	return skinframe;
}

//static char *suffix[6] = {"ft", "bk", "rt", "lf", "up", "dn"};
typedef struct suffixinfo_s
{
	const char *suffix;
	qbool flipx, flipy, flipdiagonal;
}
suffixinfo_t;
static suffixinfo_t suffix[3][6] =
{
	{
		{"px",   false, false, false},
		{"nx",   false, false, false},
		{"py",   false, false, false},
		{"ny",   false, false, false},
		{"pz",   false, false, false},
		{"nz",   false, false, false}
	},
	{
		{"posx", false, false, false},
		{"negx", false, false, false},
		{"posy", false, false, false},
		{"negy", false, false, false},
		{"posz", false, false, false},
		{"negz", false, false, false}
	},
	{
		{"rt",    true, false,  true},
		{"lf",   false,  true,  true},
		{"ft",    true,  true, false},
		{"bk",   false, false, false},
		{"up",    true, false,  true},
		{"dn",    true, false,  true}
	}
};

static int componentorder[4] = {0, 1, 2, 3};

static rtexture_t *R_LoadCubemap(const char *basename)
{
	int i, j, cubemapsize, forcefilter;
	unsigned char *cubemappixels, *image_buffer;
	rtexture_t *cubemaptexture;
	char name[256];

	// HACK: if the cubemap name starts with a !, the cubemap is nearest-filtered
	forcefilter = TEXF_FORCELINEAR;
	if (basename && basename[0] == '!')
	{
		basename++;
		forcefilter = TEXF_FORCENEAREST;
	}
	// must start 0 so the first loadimagepixels has no requested width/height
	cubemapsize = 0;
	cubemappixels = NULL;
	cubemaptexture = NULL;
	// keep trying different suffix groups (posx, px, rt) until one loads
	for (j = 0;j < 3 && !cubemappixels;j++)
	{
		// load the 6 images in the suffix group
		for (i = 0;i < 6;i++)
		{
			// generate an image name based on the base and and suffix
			dpsnprintf(name, sizeof(name), "%s%s", basename, suffix[j][i].suffix);
			// load it
			if ((image_buffer = loadimagepixelsbgra(name, false, false, false, NULL)))
			{
				// an image loaded, make sure width and height are equal
				if (image_width == image_height && (!cubemappixels || image_width == cubemapsize))
				{
					// if this is the first image to load successfully, allocate the cubemap memory
					if (!cubemappixels && image_width >= 1)
					{
						cubemapsize = image_width;
						// note this clears to black, so unavailable sides are black
						cubemappixels = (unsigned char *)Mem_Alloc(tempmempool, 6*cubemapsize*cubemapsize*4);
					}
					// copy the image with any flipping needed by the suffix (px and posx types don't need flipping)
					if (cubemappixels)
						Image_CopyMux(cubemappixels+i*cubemapsize*cubemapsize*4, image_buffer, cubemapsize, cubemapsize, suffix[j][i].flipx, suffix[j][i].flipy, suffix[j][i].flipdiagonal, 4, 4, componentorder);
				}
				else
					Con_Printf("Cubemap image \"%s\" (%ix%i) is not square, OpenGL requires square cubemaps.\n", name, image_width, image_height);
				// free the image
				Mem_Free(image_buffer);
			}
		}
	}
	// if a cubemap loaded, upload it
	if (cubemappixels)
	{
		if (developer_loading.integer)
			Con_Printf("loading cubemap \"%s\"\n", basename);

		cubemaptexture = R_LoadTextureCubeMap(r_main_texturepool, basename, cubemapsize, cubemappixels, vid.sRGB3D ? TEXTYPE_SRGB_BGRA : TEXTYPE_BGRA, (gl_texturecompression_lightcubemaps.integer && gl_texturecompression.integer ? TEXF_COMPRESS : 0) | forcefilter | TEXF_CLAMP, -1, NULL);
		Mem_Free(cubemappixels);
	}
	else
	{
		Con_DPrintf("failed to load cubemap \"%s\"\n", basename);
		if (developer_loading.integer)
		{
			Con_Printf("(tried tried images ");
			for (j = 0;j < 3;j++)
				for (i = 0;i < 6;i++)
					Con_Printf("%s\"%s%s.tga\"", j + i > 0 ? ", " : "", basename, suffix[j][i].suffix);
			Con_Print(" and was unable to find any of them).\n");
		}
	}
	return cubemaptexture;
}

rtexture_t *R_GetCubemap(const char *basename)
{
	int i;
	for (i = 0;i < r_texture_numcubemaps;i++)
		if (r_texture_cubemaps[i] != NULL)
			if (!strcasecmp(r_texture_cubemaps[i]->basename, basename))
				return r_texture_cubemaps[i]->texture ? r_texture_cubemaps[i]->texture : r_texture_whitecube;
	if (i >= MAX_CUBEMAPS || !r_main_mempool)
		return r_texture_whitecube;
	r_texture_numcubemaps++;
	r_texture_cubemaps[i] = (cubemapinfo_t *)Mem_Alloc(r_main_mempool, sizeof(cubemapinfo_t));
	dp_strlcpy(r_texture_cubemaps[i]->basename, basename, sizeof(r_texture_cubemaps[i]->basename));
	r_texture_cubemaps[i]->texture = R_LoadCubemap(r_texture_cubemaps[i]->basename);
	return r_texture_cubemaps[i]->texture;
}

static void R_Main_FreeViewCache(void)
{
	if (r_refdef.viewcache.entityvisible)
		Mem_Free(r_refdef.viewcache.entityvisible);
	if (r_refdef.viewcache.world_pvsbits)
		Mem_Free(r_refdef.viewcache.world_pvsbits);
	if (r_refdef.viewcache.world_leafvisible)
		Mem_Free(r_refdef.viewcache.world_leafvisible);
	if (r_refdef.viewcache.world_surfacevisible)
		Mem_Free(r_refdef.viewcache.world_surfacevisible);
	memset(&r_refdef.viewcache, 0, sizeof(r_refdef.viewcache));
}

static void R_Main_ResizeViewCache(void)
{
	int numentities = r_refdef.scene.numentities;
	int numclusters = r_refdef.scene.worldmodel ? r_refdef.scene.worldmodel->brush.num_pvsclusters : 1;
	int numclusterbytes = r_refdef.scene.worldmodel ? r_refdef.scene.worldmodel->brush.num_pvsclusterbytes : 1;
	int numleafs = r_refdef.scene.worldmodel ? r_refdef.scene.worldmodel->brush.num_leafs : 1;
	int numsurfaces = r_refdef.scene.worldmodel ? r_refdef.scene.worldmodel->num_surfaces : 1;
	if (r_refdef.viewcache.maxentities < numentities)
	{
		r_refdef.viewcache.maxentities = numentities;
		if (r_refdef.viewcache.entityvisible)
			Mem_Free(r_refdef.viewcache.entityvisible);
		r_refdef.viewcache.entityvisible = (unsigned char *)Mem_Alloc(r_main_mempool, r_refdef.viewcache.maxentities);
	}
	if (r_refdef.viewcache.world_numclusters != numclusters)
	{
		r_refdef.viewcache.world_numclusters = numclusters;
		r_refdef.viewcache.world_numclusterbytes = numclusterbytes;
		if (r_refdef.viewcache.world_pvsbits)
			Mem_Free(r_refdef.viewcache.world_pvsbits);
		r_refdef.viewcache.world_pvsbits = (unsigned char *)Mem_Alloc(r_main_mempool, r_refdef.viewcache.world_numclusterbytes);
	}
	if (r_refdef.viewcache.world_numleafs != numleafs)
	{
		r_refdef.viewcache.world_numleafs = numleafs;
		if (r_refdef.viewcache.world_leafvisible)
			Mem_Free(r_refdef.viewcache.world_leafvisible);
		r_refdef.viewcache.world_leafvisible = (unsigned char *)Mem_Alloc(r_main_mempool, r_refdef.viewcache.world_numleafs);
	}
	if (r_refdef.viewcache.world_numsurfaces != numsurfaces)
	{
		r_refdef.viewcache.world_numsurfaces = numsurfaces;
		if (r_refdef.viewcache.world_surfacevisible)
			Mem_Free(r_refdef.viewcache.world_surfacevisible);
		r_refdef.viewcache.world_surfacevisible = (unsigned char *)Mem_Alloc(r_main_mempool, r_refdef.viewcache.world_numsurfaces);
	}
}

extern rtexture_t *loadingscreentexture;
static void gl_main_start(void)
{
	loadingscreentexture = NULL;
	r_texture_blanknormalmap = NULL;
	r_texture_white = NULL;
	r_texture_grey128 = NULL;
	r_texture_black = NULL;
	r_texture_whitecube = NULL;
	r_texture_normalizationcube = NULL;
	r_texture_fogattenuation = NULL;
	r_texture_fogheighttexture = NULL;
	r_texture_gammaramps = NULL;
	r_texture_numcubemaps = 0;
	r_uniformbufferalignment = 32;

	r_loaddds = r_texture_dds_load.integer != 0;
	r_savedds = vid.support.ext_texture_compression_s3tc && r_texture_dds_save.integer;

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		r_loadnormalmap = true;
		r_loadgloss = true;
		r_loadfog = false;
#ifdef GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
		qglGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &r_uniformbufferalignment);
#endif
		break;
	}

	R_AnimCache_Free();
	R_FrameData_Reset();
	R_BufferData_Reset();

	r_numqueries = 0;
	r_maxqueries = 0;
	memset(r_queries, 0, sizeof(r_queries));

	r_qwskincache = NULL;
	r_qwskincache_size = 0;

	// due to caching of texture_t references, the collision cache must be reset
	Collision_Cache_Reset(true);

	// set up r_skinframe loading system for textures
	memset(&r_skinframe, 0, sizeof(r_skinframe));
	r_skinframe.loadsequence = 1;
	Mem_ExpandableArray_NewArray(&r_skinframe.array, r_main_mempool, sizeof(skinframe_t), 256);

	r_main_texturepool = R_AllocTexturePool();
	R_BuildBlankTextures();
	R_BuildNoTexture();
	R_BuildWhiteCube();
#ifndef USE_GLES2
	R_BuildNormalizationCube();
#endif //USE_GLES2
	r_texture_fogattenuation = NULL;
	r_texture_fogheighttexture = NULL;
	r_texture_gammaramps = NULL;
	//r_texture_fogintensity = NULL;
	memset(&r_fb, 0, sizeof(r_fb));
	Mem_ExpandableArray_NewArray(&r_fb.rendertargets, r_main_mempool, sizeof(r_rendertarget_t), 128);
	r_glsl_permutation = NULL;
	memset(r_glsl_permutationhash, 0, sizeof(r_glsl_permutationhash));
	Mem_ExpandableArray_NewArray(&r_glsl_permutationarray, r_main_mempool, sizeof(r_glsl_permutation_t), 256);
	memset(&r_svbsp, 0, sizeof (r_svbsp));

	memset(r_texture_cubemaps, 0, sizeof(r_texture_cubemaps));
	r_texture_numcubemaps = 0;

	r_refdef.fogmasktable_density = 0;

#ifdef __ANDROID__
	// For Steelstorm Android
	// FIXME CACHE the program and reload
	// FIXME see possible combinations for SS:BR android
	Con_DPrintf("Compiling most used shaders for SS:BR android... START\n");
	R_SetupShader_SetPermutationGLSL(0, 12);
	R_SetupShader_SetPermutationGLSL(0, 13);
	R_SetupShader_SetPermutationGLSL(0, 8388621);
	R_SetupShader_SetPermutationGLSL(3, 0);
	R_SetupShader_SetPermutationGLSL(3, 2048);
	R_SetupShader_SetPermutationGLSL(5, 0);
	R_SetupShader_SetPermutationGLSL(5, 2);
	R_SetupShader_SetPermutationGLSL(5, 2048);
	R_SetupShader_SetPermutationGLSL(5, 8388608);
	R_SetupShader_SetPermutationGLSL(11, 1);
	R_SetupShader_SetPermutationGLSL(11, 2049);
	R_SetupShader_SetPermutationGLSL(11, 8193);
	R_SetupShader_SetPermutationGLSL(11, 10241);
	Con_DPrintf("Compiling most used shaders for SS:BR android... END\n");
#endif
}

extern unsigned int r_shadow_occlusion_buf;

static void gl_main_shutdown(void)
{
	R_RenderTarget_FreeUnused(true);
	Mem_ExpandableArray_FreeArray(&r_fb.rendertargets);
	R_AnimCache_Free();
	R_FrameData_Reset();
	R_BufferData_Reset();

	R_Main_FreeViewCache();

	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
#if defined(GL_SAMPLES_PASSED) && !defined(USE_GLES2)
		if (r_maxqueries)
			qglDeleteQueries(r_maxqueries, r_queries);
#endif
		break;
	}
	r_shadow_occlusion_buf = 0;
	r_numqueries = 0;
	r_maxqueries = 0;
	memset(r_queries, 0, sizeof(r_queries));

	r_qwskincache = NULL;
	r_qwskincache_size = 0;

	// clear out the r_skinframe state
	Mem_ExpandableArray_FreeArray(&r_skinframe.array);
	memset(&r_skinframe, 0, sizeof(r_skinframe));

	if (r_svbsp.nodes)
		Mem_Free(r_svbsp.nodes);
	memset(&r_svbsp, 0, sizeof (r_svbsp));
	R_FreeTexturePool(&r_main_texturepool);
	loadingscreentexture = NULL;
	r_texture_blanknormalmap = NULL;
	r_texture_white = NULL;
	r_texture_grey128 = NULL;
	r_texture_black = NULL;
	r_texture_whitecube = NULL;
	r_texture_normalizationcube = NULL;
	r_texture_fogattenuation = NULL;
	r_texture_fogheighttexture = NULL;
	r_texture_gammaramps = NULL;
	r_texture_numcubemaps = 0;
	//r_texture_fogintensity = NULL;
	memset(&r_fb, 0, sizeof(r_fb));
	R_GLSL_Restart_f(cmd_local);

	r_glsl_permutation = NULL;
	memset(r_glsl_permutationhash, 0, sizeof(r_glsl_permutationhash));
	Mem_ExpandableArray_FreeArray(&r_glsl_permutationarray);
}

static void gl_main_newmap(void)
{
	// FIXME: move this code to client
	char *entities, entname[MAX_QPATH];
	if (r_qwskincache)
		Mem_Free(r_qwskincache);
	r_qwskincache = NULL;
	r_qwskincache_size = 0;
	if (cl.worldmodel)
	{
		dpsnprintf(entname, sizeof(entname), "%s.ent", cl.worldnamenoextension);
		if ((entities = (char *)FS_LoadFile(entname, tempmempool, true, NULL)))
		{
			CL_ParseEntityLump(entities);
			Mem_Free(entities);
			return;
		}
		if (cl.worldmodel->brush.entities)
			CL_ParseEntityLump(cl.worldmodel->brush.entities);
	}
	R_Main_FreeViewCache();

	R_FrameData_Reset();
	R_BufferData_Reset();
}

void GL_Main_Init(void)
{
	int i;
	r_main_mempool = Mem_AllocPool("Renderer", 0, NULL);
	R_InitShaderModeInfo();

	Cmd_AddCommand(CF_CLIENT, "r_glsl_restart", R_GLSL_Restart_f, "unloads GLSL shaders, they will then be reloaded as needed");
	Cmd_AddCommand(CF_CLIENT, "r_glsl_dumpshader", R_GLSL_DumpShader_f, "dumps the engine internal default.glsl shader into glsl/default.glsl");
	// FIXME: the client should set up r_refdef.fog stuff including the fogmasktable
	if (gamemode == GAME_NEHAHRA)
	{
		Cvar_RegisterVariable (&gl_fogenable);
		Cvar_RegisterVariable (&gl_fogdensity);
		Cvar_RegisterVariable (&gl_fogred);
		Cvar_RegisterVariable (&gl_foggreen);
		Cvar_RegisterVariable (&gl_fogblue);
		Cvar_RegisterVariable (&gl_fogstart);
		Cvar_RegisterVariable (&gl_fogend);
		Cvar_RegisterVariable (&gl_skyclip);
	}
	Cvar_RegisterVariable(&r_motionblur);
	Cvar_RegisterVariable(&r_damageblur);
	Cvar_RegisterVariable(&r_motionblur_averaging);
	Cvar_RegisterVariable(&r_motionblur_randomize);
	Cvar_RegisterVariable(&r_motionblur_minblur);
	Cvar_RegisterVariable(&r_motionblur_maxblur);
	Cvar_RegisterVariable(&r_motionblur_velocityfactor);
	Cvar_RegisterVariable(&r_motionblur_velocityfactor_minspeed);
	Cvar_RegisterVariable(&r_motionblur_velocityfactor_maxspeed);
	Cvar_RegisterVariable(&r_motionblur_mousefactor);
	Cvar_RegisterVariable(&r_motionblur_mousefactor_minspeed);
	Cvar_RegisterVariable(&r_motionblur_mousefactor_maxspeed);
	Cvar_RegisterVariable(&r_depthfirst);
	Cvar_RegisterVariable(&r_useinfinitefarclip);
	Cvar_RegisterVariable(&r_farclip_base);
	Cvar_RegisterVariable(&r_farclip_world);
	Cvar_RegisterVariable(&r_nearclip);
	Cvar_RegisterVariable(&r_deformvertexes);
	Cvar_RegisterVariable(&r_transparent);
	Cvar_RegisterVariable(&r_transparent_alphatocoverage);
	Cvar_RegisterVariable(&r_transparent_sortsurfacesbynearest);
	Cvar_RegisterVariable(&r_transparent_useplanardistance);
	Cvar_RegisterVariable(&r_showoverdraw);
	Cvar_RegisterVariable(&r_showbboxes);
	Cvar_RegisterVariable(&r_showbboxes_client);
	Cvar_RegisterVariable(&r_showsurfaces);
	Cvar_RegisterVariable(&r_showtris);
	Cvar_RegisterVariable(&r_shownormals);
	Cvar_RegisterVariable(&r_showlighting);
	Cvar_RegisterVariable(&r_showcollisionbrushes);
	Cvar_RegisterVariable(&r_showcollisionbrushes_polygonfactor);
	Cvar_RegisterVariable(&r_showcollisionbrushes_polygonoffset);
	Cvar_RegisterVariable(&r_showdisabledepthtest);
	Cvar_RegisterVariable(&r_showspriteedges);
	Cvar_RegisterVariable(&r_showparticleedges);
	Cvar_RegisterVariable(&r_drawportals);
	Cvar_RegisterVariable(&r_drawentities);
	Cvar_RegisterVariable(&r_draw2d);
	Cvar_RegisterVariable(&r_drawworld);
	Cvar_RegisterVariable(&r_cullentities_trace);
	Cvar_RegisterVariable(&r_cullentities_trace_entityocclusion);
	Cvar_RegisterVariable(&r_cullentities_trace_samples);
	Cvar_RegisterVariable(&r_cullentities_trace_tempentitysamples);
	Cvar_RegisterVariable(&r_cullentities_trace_enlarge);
	Cvar_RegisterVariable(&r_cullentities_trace_expand);
	Cvar_RegisterVariable(&r_cullentities_trace_pad);
	Cvar_RegisterVariable(&r_cullentities_trace_delay);
	Cvar_RegisterVariable(&r_cullentities_trace_eyejitter);
	Cvar_RegisterVariable(&r_sortentities);
	Cvar_RegisterVariable(&r_drawviewmodel);
	Cvar_RegisterVariable(&r_drawexteriormodel);
	Cvar_RegisterVariable(&r_speeds);
	Cvar_RegisterVariable(&r_fullbrights);
	Cvar_RegisterVariable(&r_wateralpha);
	Cvar_RegisterVariable(&r_dynamic);
	Cvar_RegisterVariable(&r_fullbright_directed);
	Cvar_RegisterVariable(&r_fullbright_directed_ambient);
	Cvar_RegisterVariable(&r_fullbright_directed_diffuse);
	Cvar_RegisterVariable(&r_fullbright_directed_pitch);
	Cvar_RegisterVariable(&r_fullbright_directed_pitch_relative);
	Cvar_RegisterVariable(&r_fullbright);
	Cvar_RegisterVariable(&r_shadows);
	Cvar_RegisterVariable(&r_shadows_darken);
	Cvar_RegisterVariable(&r_shadows_drawafterrtlighting);
	Cvar_RegisterVariable(&r_shadows_castfrombmodels);
	Cvar_RegisterVariable(&r_shadows_throwdistance);
	Cvar_RegisterVariable(&r_shadows_throwdirection);
	Cvar_RegisterVariable(&r_shadows_focus);
	Cvar_RegisterVariable(&r_shadows_shadowmapscale);
	Cvar_RegisterVariable(&r_shadows_shadowmapbias);
	Cvar_RegisterVariable(&r_q1bsp_skymasking);
	Cvar_RegisterVariable(&r_polygonoffset_submodel_factor);
	Cvar_RegisterVariable(&r_polygonoffset_submodel_offset);
	Cvar_RegisterVariable(&r_polygonoffset_decals_factor);
	Cvar_RegisterVariable(&r_polygonoffset_decals_offset);
	Cvar_RegisterVariable(&r_fog_exp2);
	Cvar_RegisterVariable(&r_fog_clear);
	Cvar_RegisterVariable(&r_drawfog);
	Cvar_RegisterVariable(&r_transparentdepthmasking);
	Cvar_RegisterVariable(&r_transparent_sortmindist);
	Cvar_RegisterVariable(&r_transparent_sortmaxdist);
	Cvar_RegisterVariable(&r_transparent_sortarraysize);
	Cvar_RegisterVariable(&r_texture_dds_load);
	Cvar_RegisterVariable(&r_texture_dds_save);
	Cvar_RegisterVariable(&r_usedepthtextures);
	Cvar_RegisterVariable(&r_viewfbo);
	Cvar_RegisterVariable(&r_rendertarget_debug);
	Cvar_RegisterVariable(&r_viewscale);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling_min);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling_multiply);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling_stepsize);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling_stepmax);
	Cvar_RegisterVariable(&r_viewscale_fpsscaling_target);
	Cvar_RegisterVariable(&r_glsl_deluxemapping);
	Cvar_RegisterVariable(&r_glsl_offsetmapping);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_steps);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_reliefmapping);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_reliefmapping_steps);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_reliefmapping_refinesteps);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_scale);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_lod);
	Cvar_RegisterVariable(&r_glsl_offsetmapping_lod_distance);
	Cvar_RegisterVariable(&r_glsl_postprocess);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec1);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec2);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec3);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec4);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec1_enable);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec2_enable);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec3_enable);
	Cvar_RegisterVariable(&r_glsl_postprocess_uservec4_enable);
	Cvar_RegisterVariable(&r_celshading);
	Cvar_RegisterVariable(&r_celoutlines);
	Cvar_RegisterVariable(&r_fxaa);

	Cvar_RegisterVariable(&r_water);
	Cvar_RegisterVariable(&r_water_cameraentitiesonly);
	Cvar_RegisterVariable(&r_water_resolutionmultiplier);
	Cvar_RegisterVariable(&r_water_clippingplanebias);
	Cvar_RegisterVariable(&r_water_refractdistort);
	Cvar_RegisterVariable(&r_water_reflectdistort);
	Cvar_RegisterVariable(&r_water_scissormode);
	Cvar_RegisterVariable(&r_water_lowquality);
	Cvar_RegisterVariable(&r_water_hideplayer);

	Cvar_RegisterVariable(&r_lerpsprites);
	Cvar_RegisterVariable(&r_lerpmodels);
	Cvar_RegisterVariable(&r_nolerp_list);
	Cvar_RegisterVariable(&r_lerplightstyles);
	Cvar_RegisterVariable(&r_waterscroll);
	Cvar_RegisterVariable(&r_bloom);
	Cvar_RegisterVariable(&r_colorfringe);
	Cvar_RegisterVariable(&r_bloom_colorscale);
	Cvar_RegisterVariable(&r_bloom_brighten);
	Cvar_RegisterVariable(&r_bloom_blur);
	Cvar_RegisterVariable(&r_bloom_resolution);
	Cvar_RegisterVariable(&r_bloom_colorexponent);
	Cvar_RegisterVariable(&r_bloom_colorsubtract);
	Cvar_RegisterVariable(&r_bloom_scenebrightness);
	Cvar_RegisterVariable(&r_hdr_scenebrightness);
	Cvar_RegisterVariable(&r_hdr_glowintensity);
	Cvar_RegisterVariable(&r_hdr_irisadaptation);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_multiplier);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_minvalue);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_maxvalue);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_value);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_fade_up);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_fade_down);
	Cvar_RegisterVariable(&r_hdr_irisadaptation_radius);
	Cvar_RegisterVariable(&r_smoothnormals_areaweighting);
	Cvar_RegisterVariable(&developer_texturelogging);
	Cvar_RegisterVariable(&gl_lightmaps);
	Cvar_RegisterVariable(&r_test);
	Cvar_RegisterVariable(&r_batch_multidraw);
	Cvar_RegisterVariable(&r_batch_multidraw_mintriangles);
	Cvar_RegisterVariable(&r_batch_debugdynamicvertexpath);
	Cvar_RegisterVariable(&r_glsl_skeletal);
	Cvar_RegisterVariable(&r_glsl_saturation);
	Cvar_RegisterVariable(&r_glsl_saturation_redcompensate);
	Cvar_RegisterVariable(&r_glsl_vertextextureblend_usebothalphas);
	Cvar_RegisterVariable(&r_framedatasize);
	for (i = 0;i < R_BUFFERDATA_COUNT;i++)
		Cvar_RegisterVariable(&r_buffermegs[i]);
	Cvar_RegisterVariable(&r_batch_dynamicbuffer);
	Cvar_RegisterVariable(&r_q1bsp_lightmap_updates_enabled);
	Cvar_RegisterVariable(&r_q1bsp_lightmap_updates_combine);
	Cvar_RegisterVariable(&r_q1bsp_lightmap_updates_hidden_surfaces);
	if (gamemode == GAME_NEHAHRA || gamemode == GAME_TENEBRAE)
		Cvar_SetValue(&cvars_all, "r_fullbrights", 0);
#ifdef DP_MOBILETOUCH
	// GLES devices have terrible depth precision in general, so...
	Cvar_SetValueQuick(&r_nearclip, 4);
	Cvar_SetValueQuick(&r_farclip_base, 4096);
	Cvar_SetValueQuick(&r_farclip_world, 0);
	Cvar_SetValueQuick(&r_useinfinitefarclip, 0);
#endif
	R_RegisterModule("GL_Main", gl_main_start, gl_main_shutdown, gl_main_newmap, NULL, NULL);
}

void Render_Init(void)
{
	gl_backend_init();
	R_Textures_Init();
	GL_Main_Init();
	Font_Init();
	GL_Draw_Init();
	R_Shadow_Init();
	R_Sky_Init();
	GL_Surf_Init();
	Sbar_Init();
	R_Particles_Init();
	R_Explosion_Init();
	R_LightningBeams_Init();
	CL_MeshEntities_Init();
	Mod_RenderInit();
}

static void R_GetCornerOfBox(vec3_t out, const vec3_t mins, const vec3_t maxs, int signbits)
{
	out[0] = ((signbits & 1) ? mins : maxs)[0];
	out[1] = ((signbits & 2) ? mins : maxs)[1];
	out[2] = ((signbits & 4) ? mins : maxs)[2];
}

static qbool _R_CullBox(const vec3_t mins, const vec3_t maxs, int numplanes, const mplane_t *planes, int ignore)
{
	int i;
	const mplane_t *p;
	vec3_t corner;
	if (r_trippy.integer)
		return false;
	for (i = 0;i < numplanes;i++)
	{
		if(i == ignore)
			continue;
		p = planes + i;
		R_GetCornerOfBox(corner, mins, maxs, p->signbits);
		if (DotProduct(p->normal, corner) < p->dist)
			return true;
	}
	return false;
}

qbool R_CullFrustum(const vec3_t mins, const vec3_t maxs)
{
	// skip nearclip plane, it often culls portals when you are very close, and is almost never useful
	return _R_CullBox(mins, maxs, r_refdef.view.numfrustumplanes, r_refdef.view.frustum, 4);
}

qbool R_CullBox(const vec3_t mins, const vec3_t maxs, int numplanes, const mplane_t *planes)
{
	// nothing to ignore
	return _R_CullBox(mins, maxs, numplanes, planes, -1);
}

//==================================================================================

// LadyHavoc: this stores temporary data used within the same frame

typedef struct r_framedata_mem_s
{
	struct r_framedata_mem_s *purge; // older mem block to free on next frame
	size_t size; // how much usable space
	size_t current; // how much space in use
	size_t mark; // last "mark" location, temporary memory can be freed by returning to this
	size_t wantedsize; // how much space was allocated
	unsigned char *data; // start of real data (16byte aligned)
}
r_framedata_mem_t;

static r_framedata_mem_t *r_framedata_mem;

void R_FrameData_Reset(void)
{
	while (r_framedata_mem)
	{
		r_framedata_mem_t *next = r_framedata_mem->purge;
		Mem_Free(r_framedata_mem);
		r_framedata_mem = next;
	}
}

static void R_FrameData_Resize(qbool mustgrow)
{
	size_t wantedsize;
	wantedsize = (size_t)(r_framedatasize.value * 1024*1024);
	wantedsize = bound(65536, wantedsize, 1000*1024*1024);
	if (!r_framedata_mem || r_framedata_mem->wantedsize != wantedsize || mustgrow)
	{
		r_framedata_mem_t *newmem = (r_framedata_mem_t *)Mem_Alloc(r_main_mempool, wantedsize);
		newmem->wantedsize = wantedsize;
		newmem->data = (unsigned char *)(((size_t)(newmem+1) + 15) & ~15);
		newmem->size = (unsigned char *)newmem + wantedsize - newmem->data;
		newmem->current = 0;
		newmem->mark = 0;
		newmem->purge = r_framedata_mem;
		r_framedata_mem = newmem;
	}
}

void R_FrameData_NewFrame(void)
{
	R_FrameData_Resize(false);
	if (!r_framedata_mem)
		return;
	// if we ran out of space on the last frame, free the old memory now
	while (r_framedata_mem->purge)
	{
		// repeatedly remove the second item in the list, leaving only head
		r_framedata_mem_t *next = r_framedata_mem->purge->purge;
		Mem_Free(r_framedata_mem->purge);
		r_framedata_mem->purge = next;
	}
	// reset the current mem pointer
	r_framedata_mem->current = 0;
	r_framedata_mem->mark = 0;
}

void *R_FrameData_Alloc(size_t size)
{
	void *data;
	float newvalue;

	// align to 16 byte boundary - the data pointer is already aligned, so we
	// only need to ensure the size of every allocation is also aligned
	size = (size + 15) & ~15;

	while (!r_framedata_mem || r_framedata_mem->current + size > r_framedata_mem->size)
	{
		// emergency - we ran out of space, allocate more memory
		// note: this has no upper-bound, we'll fail to allocate memory eventually and just die
		newvalue = r_framedatasize.value * 2.0f;
		// upper bound based on architecture - if we try to allocate more than this we could overflow, better to loop until we error out on allocation failure
		if (sizeof(size_t) >= 8)
			newvalue = bound(0.25f, newvalue, (float)(1ll << 42));
		else
			newvalue = bound(0.25f, newvalue, (float)(1 << 10));
		// this might not be a growing it, but we'll allocate another buffer every time
		Cvar_SetValueQuick(&r_framedatasize, newvalue);
		R_FrameData_Resize(true);
	}

	data = r_framedata_mem->data + r_framedata_mem->current;
	r_framedata_mem->current += size;

	// count the usage for stats
	r_refdef.stats[r_stat_framedatacurrent] = max(r_refdef.stats[r_stat_framedatacurrent], (int)r_framedata_mem->current);
	r_refdef.stats[r_stat_framedatasize] = max(r_refdef.stats[r_stat_framedatasize], (int)r_framedata_mem->size);

	return (void *)data;
}

void *R_FrameData_Store(size_t size, void *data)
{
	void *d = R_FrameData_Alloc(size);
	if (d && data)
		memcpy(d, data, size);
	return d;
}

void R_FrameData_SetMark(void)
{
	if (!r_framedata_mem)
		return;
	r_framedata_mem->mark = r_framedata_mem->current;
}

void R_FrameData_ReturnToMark(void)
{
	if (!r_framedata_mem)
		return;
	r_framedata_mem->current = r_framedata_mem->mark;
}

//==================================================================================

// avoid reusing the same buffer objects on consecutive frames
#define R_BUFFERDATA_CYCLE 3

typedef struct r_bufferdata_buffer_s
{
	struct r_bufferdata_buffer_s *purge; // older buffer to free on next frame
	size_t size; // how much usable space
	size_t current; // how much space in use
	r_meshbuffer_t *buffer; // the buffer itself
}
r_bufferdata_buffer_t;

static int r_bufferdata_cycle = 0; // incremented and wrapped each frame
static r_bufferdata_buffer_t *r_bufferdata_buffer[R_BUFFERDATA_CYCLE][R_BUFFERDATA_COUNT];

/// frees all dynamic buffers
void R_BufferData_Reset(void)
{
	int cycle, type;
	r_bufferdata_buffer_t **p, *mem;
	for (cycle = 0;cycle < R_BUFFERDATA_CYCLE;cycle++)
	{
		for (type = 0;type < R_BUFFERDATA_COUNT;type++)
		{
			// free all buffers
			p = &r_bufferdata_buffer[cycle][type];
			while (*p)
			{
				mem = *p;
				*p = (*p)->purge;
				if (mem->buffer)
					R_Mesh_DestroyMeshBuffer(mem->buffer);
				Mem_Free(mem);
			}
		}
	}
}

// resize buffer as needed (this actually makes a new one, the old one will be recycled next frame)
static void R_BufferData_Resize(r_bufferdata_type_t type, qbool mustgrow, size_t minsize)
{
	r_bufferdata_buffer_t *mem = r_bufferdata_buffer[r_bufferdata_cycle][type];
	size_t size;
	float newvalue = r_buffermegs[type].value;

	// increase the cvar if we have to (but only if we already have a mem)
	if (mustgrow && mem)
		newvalue *= 2.0f;
	newvalue = bound(0.25f, newvalue, 256.0f);
	while (newvalue * 1024*1024 < minsize)
		newvalue *= 2.0f;

	// clamp the cvar to valid range
	newvalue = bound(0.25f, newvalue, 256.0f);
	if (r_buffermegs[type].value != newvalue)
		Cvar_SetValueQuick(&r_buffermegs[type], newvalue);

	// calculate size in bytes
	size = (size_t)(newvalue * 1024*1024);
	size = bound(131072, size, 256*1024*1024);

	// allocate a new buffer if the size is different (purge old one later)
	// or if we were told we must grow the buffer
	if (!mem || mem->size != size || mustgrow)
	{
		mem = (r_bufferdata_buffer_t *)Mem_Alloc(r_main_mempool, sizeof(*mem));
		mem->size = size;
		mem->current = 0;
		if (type == R_BUFFERDATA_VERTEX)
			mem->buffer = R_Mesh_CreateMeshBuffer(NULL, mem->size, "dynamicbuffervertex", false, false, true, false);
		else if (type == R_BUFFERDATA_INDEX16)
			mem->buffer = R_Mesh_CreateMeshBuffer(NULL, mem->size, "dynamicbufferindex16", true, false, true, true);
		else if (type == R_BUFFERDATA_INDEX32)
			mem->buffer = R_Mesh_CreateMeshBuffer(NULL, mem->size, "dynamicbufferindex32", true, false, true, false);
		else if (type == R_BUFFERDATA_UNIFORM)
			mem->buffer = R_Mesh_CreateMeshBuffer(NULL, mem->size, "dynamicbufferuniform", false, true, true, false);
		mem->purge = r_bufferdata_buffer[r_bufferdata_cycle][type];
		r_bufferdata_buffer[r_bufferdata_cycle][type] = mem;
	}
}

void R_BufferData_NewFrame(void)
{
	int type;
	r_bufferdata_buffer_t **p, *mem;
	// cycle to the next frame's buffers
	r_bufferdata_cycle = (r_bufferdata_cycle + 1) % R_BUFFERDATA_CYCLE;
	// if we ran out of space on the last time we used these buffers, free the old memory now
	for (type = 0;type < R_BUFFERDATA_COUNT;type++)
	{
		if (r_bufferdata_buffer[r_bufferdata_cycle][type])
		{
			R_BufferData_Resize((r_bufferdata_type_t)type, false, 131072);
			// free all but the head buffer, this is how we recycle obsolete
			// buffers after they are no longer in use
			p = &r_bufferdata_buffer[r_bufferdata_cycle][type]->purge;
			while (*p)
			{
				mem = *p;
				*p = (*p)->purge;
				if (mem->buffer)
					R_Mesh_DestroyMeshBuffer(mem->buffer);
				Mem_Free(mem);
			}
			// reset the current offset
			r_bufferdata_buffer[r_bufferdata_cycle][type]->current = 0;
		}
	}
}

r_meshbuffer_t *R_BufferData_Store(size_t datasize, const void *data, r_bufferdata_type_t type, int *returnbufferoffset)
{
	r_bufferdata_buffer_t *mem;
	int offset = 0;
	int padsize;

	*returnbufferoffset = 0;

	// align size to a byte boundary appropriate for the buffer type, this
	// makes all allocations have aligned start offsets
	if (type == R_BUFFERDATA_UNIFORM)
		padsize = (datasize + r_uniformbufferalignment - 1) & ~(r_uniformbufferalignment - 1);
	else
		padsize = (datasize + 15) & ~15;

	// if we ran out of space in this buffer we must allocate a new one
	if (!r_bufferdata_buffer[r_bufferdata_cycle][type] || r_bufferdata_buffer[r_bufferdata_cycle][type]->current + padsize > r_bufferdata_buffer[r_bufferdata_cycle][type]->size)
		R_BufferData_Resize(type, true, padsize);

	// if the resize did not give us enough memory, fail
	if (!r_bufferdata_buffer[r_bufferdata_cycle][type] || r_bufferdata_buffer[r_bufferdata_cycle][type]->current + padsize > r_bufferdata_buffer[r_bufferdata_cycle][type]->size)
		Sys_Error("R_BufferData_Store: failed to create a new buffer of sufficient size\n");

	mem = r_bufferdata_buffer[r_bufferdata_cycle][type];
	offset = (int)mem->current;
	mem->current += padsize;

	// upload the data to the buffer at the chosen offset
	if (offset == 0)
		R_Mesh_UpdateMeshBuffer(mem->buffer, NULL, mem->size, false, 0);
	R_Mesh_UpdateMeshBuffer(mem->buffer, data, datasize, true, offset);

	// count the usage for stats
	r_refdef.stats[r_stat_bufferdatacurrent_vertex + type] = max(r_refdef.stats[r_stat_bufferdatacurrent_vertex + type], (int)mem->current);
	r_refdef.stats[r_stat_bufferdatasize_vertex + type] = max(r_refdef.stats[r_stat_bufferdatasize_vertex + type], (int)mem->size);

	// return the buffer offset
	*returnbufferoffset = offset;

	return mem->buffer;
}

//==================================================================================

// LadyHavoc: animcache originally written by Echon, rewritten since then

/**
 * Animation cache prevents re-generating mesh data for an animated model
 * multiple times in one frame for lighting, shadowing, reflections, etc.
 */

void R_AnimCache_Free(void)
{
}

void R_AnimCache_ClearCache(void)
{
	int i;
	entity_render_t *ent;

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		ent = r_refdef.scene.entities[i];
		ent->animcache_vertex3f = NULL;
		ent->animcache_vertex3f_vertexbuffer = NULL;
		ent->animcache_vertex3f_bufferoffset = 0;
		ent->animcache_normal3f = NULL;
		ent->animcache_normal3f_vertexbuffer = NULL;
		ent->animcache_normal3f_bufferoffset = 0;
		ent->animcache_svector3f = NULL;
		ent->animcache_svector3f_vertexbuffer = NULL;
		ent->animcache_svector3f_bufferoffset = 0;
		ent->animcache_tvector3f = NULL;
		ent->animcache_tvector3f_vertexbuffer = NULL;
		ent->animcache_tvector3f_bufferoffset = 0;
		ent->animcache_skeletaltransform3x4 = NULL;
		ent->animcache_skeletaltransform3x4buffer = NULL;
		ent->animcache_skeletaltransform3x4offset = 0;
		ent->animcache_skeletaltransform3x4size = 0;
	}
}

qbool R_AnimCache_GetEntity(entity_render_t *ent, qbool wantnormals, qbool wanttangents)
{
	model_t *model = ent->model;
	int numvertices;

	// see if this ent is worth caching
	if (!model || !model->Draw || !model->AnimateVertices)
		return false;
	// nothing to cache if it contains no animations and has no skeleton
	if (!model->surfmesh.isanimated && !(model->num_bones && ent->skeleton && ent->skeleton->relativetransforms))
		return false;
	// see if it is already cached for gpuskeletal
	if (ent->animcache_skeletaltransform3x4)
		return false;
	// see if it is already cached as a mesh
	if (ent->animcache_vertex3f)
	{
		// check if we need to add normals or tangents
		if (ent->animcache_normal3f)
			wantnormals = false;
		if (ent->animcache_svector3f)
			wanttangents = false;
		if (!wantnormals && !wanttangents)
			return false;
	}

	// check which kind of cache we need to generate
	if (r_gpuskeletal && model->num_bones > 0 && model->surfmesh.data_skeletalindex4ub)
	{
		// cache the skeleton so the vertex shader can use it
		r_refdef.stats[r_stat_animcache_skeletal_count] += 1;
		r_refdef.stats[r_stat_animcache_skeletal_bones] += model->num_bones;
		r_refdef.stats[r_stat_animcache_skeletal_maxbones] = max(r_refdef.stats[r_stat_animcache_skeletal_maxbones], model->num_bones);
		ent->animcache_skeletaltransform3x4 = (float *)R_FrameData_Alloc(sizeof(float[3][4]) * model->num_bones);
		Mod_Skeletal_BuildTransforms(model, ent->frameblend, ent->skeleton, NULL, ent->animcache_skeletaltransform3x4);
		// note: this can fail if the buffer is at the grow limit
		ent->animcache_skeletaltransform3x4size = sizeof(float[3][4]) * model->num_bones;
		ent->animcache_skeletaltransform3x4buffer = R_BufferData_Store(ent->animcache_skeletaltransform3x4size, ent->animcache_skeletaltransform3x4, R_BUFFERDATA_UNIFORM, &ent->animcache_skeletaltransform3x4offset);
	}
	else if (ent->animcache_vertex3f)
	{
		// mesh was already cached but we may need to add normals/tangents
		// (this only happens with multiple views, reflections, cameras, etc)
		if (wantnormals || wanttangents)
		{
			numvertices = model->surfmesh.num_vertices;
			if (wantnormals)
				ent->animcache_normal3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
			if (wanttangents)
			{
				ent->animcache_svector3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
				ent->animcache_tvector3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
			}
			model->AnimateVertices(model, ent->frameblend, ent->skeleton, NULL, wantnormals ? ent->animcache_normal3f : NULL, wanttangents ? ent->animcache_svector3f : NULL, wanttangents ? ent->animcache_tvector3f : NULL);
			r_refdef.stats[r_stat_animcache_shade_count] += 1;
			r_refdef.stats[r_stat_animcache_shade_vertices] += numvertices;
			r_refdef.stats[r_stat_animcache_shade_maxvertices] = max(r_refdef.stats[r_stat_animcache_shade_maxvertices], numvertices);
		}
	}
	else
	{
		// generate mesh cache
		numvertices = model->surfmesh.num_vertices;
		ent->animcache_vertex3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
		if (wantnormals)
			ent->animcache_normal3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
		if (wanttangents)
		{
			ent->animcache_svector3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
			ent->animcache_tvector3f = (float *)R_FrameData_Alloc(sizeof(float[3])*numvertices);
		}
		model->AnimateVertices(model, ent->frameblend, ent->skeleton, ent->animcache_vertex3f, ent->animcache_normal3f, ent->animcache_svector3f, ent->animcache_tvector3f);
		if (wantnormals || wanttangents)
		{
			r_refdef.stats[r_stat_animcache_shade_count] += 1;
			r_refdef.stats[r_stat_animcache_shade_vertices] += numvertices;
			r_refdef.stats[r_stat_animcache_shade_maxvertices] = max(r_refdef.stats[r_stat_animcache_shade_maxvertices], numvertices);
		}
		r_refdef.stats[r_stat_animcache_shape_count] += 1;
		r_refdef.stats[r_stat_animcache_shape_vertices] += numvertices;
		r_refdef.stats[r_stat_animcache_shape_maxvertices] = max(r_refdef.stats[r_stat_animcache_shape_maxvertices], numvertices);
	}
	return true;
}

void R_AnimCache_CacheVisibleEntities(void)
{
	int i;

	// TODO: thread this
	// NOTE: R_PrepareRTLights() also caches entities

	for (i = 0;i < r_refdef.scene.numentities;i++)
		if (r_refdef.viewcache.entityvisible[i])
			R_AnimCache_GetEntity(r_refdef.scene.entities[i], true, true);
}

//==================================================================================

qbool R_CanSeeBox(int numsamples, vec_t eyejitter, vec_t entboxenlarge, vec_t entboxexpand, vec_t pad, vec3_t eye, vec3_t entboxmins, vec3_t entboxmaxs)
{
	long unsigned int i;
	int j;
	vec3_t eyemins, eyemaxs;
	vec3_t boxmins, boxmaxs;
	vec3_t padmins, padmaxs;
	vec3_t start;
	vec3_t end;
	model_t *model = r_refdef.scene.worldmodel;
	static vec3_t positions[] = {
		{ 0.5f, 0.5f, 0.5f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f },
	};

	// sample count can be set to -1 to skip this logic, for flicker-prone objects
	if (numsamples < 0)
		return true;

	// view origin is not used for culling in portal/reflection/refraction renders or isometric views
	if (!r_refdef.view.usevieworiginculling)
		return true;

	if (!r_cullentities_trace_entityocclusion.integer && (!model || !model->brush.TraceLineOfSight))
		return true;

	// expand the eye box a little
	eyemins[0] = eye[0] - eyejitter;
	eyemaxs[0] = eye[0] + eyejitter;
	eyemins[1] = eye[1] - eyejitter;
	eyemaxs[1] = eye[1] + eyejitter;
	eyemins[2] = eye[2] - eyejitter;
	eyemaxs[2] = eye[2] + eyejitter;
	// expand the box a little
	boxmins[0] = (entboxenlarge + 1) * entboxmins[0] - entboxenlarge * entboxmaxs[0] - entboxexpand;
	boxmaxs[0] = (entboxenlarge + 1) * entboxmaxs[0] - entboxenlarge * entboxmins[0] + entboxexpand;
	boxmins[1] = (entboxenlarge + 1) * entboxmins[1] - entboxenlarge * entboxmaxs[1] - entboxexpand;
	boxmaxs[1] = (entboxenlarge + 1) * entboxmaxs[1] - entboxenlarge * entboxmins[1] + entboxexpand;
	boxmins[2] = (entboxenlarge + 1) * entboxmins[2] - entboxenlarge * entboxmaxs[2] - entboxexpand;
	boxmaxs[2] = (entboxenlarge + 1) * entboxmaxs[2] - entboxenlarge * entboxmins[2] + entboxexpand;
	// make an even larger box for the acceptable area
	padmins[0] = boxmins[0] - pad;
	padmaxs[0] = boxmaxs[0] + pad;
	padmins[1] = boxmins[1] - pad;
	padmaxs[1] = boxmaxs[1] + pad;
	padmins[2] = boxmins[2] - pad;
	padmaxs[2] = boxmaxs[2] + pad;

	// return true if eye overlaps enlarged box
	if (BoxesOverlap(boxmins, boxmaxs, eyemins, eyemaxs))
		return true;

	VectorCopy(eye, start);
	// try specific positions in the box first - note that these can be cached
	if (r_cullentities_trace_entityocclusion.integer)
	{
		for (i = 0; i < sizeof(positions) / sizeof(positions[0]); i++)
		{
			trace_t trace;
			end[0] = boxmins[0] + (boxmaxs[0] - boxmins[0]) * positions[i][0];
			end[1] = boxmins[1] + (boxmaxs[1] - boxmins[1]) * positions[i][1];
			end[2] = boxmins[2] + (boxmaxs[2] - boxmins[2]) * positions[i][2];
			//trace_t trace = CL_TraceLine(start, end, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, SUPERCONTENTS_SKY, MATERIALFLAGMASK_TRANSLUCENT, 0.0f, true, false, NULL, true, true);
			trace = CL_Cache_TraceLineSurfaces(start, end, MOVE_NORMAL, SUPERCONTENTS_SOLID, 0, MATERIALFLAGMASK_TRANSLUCENT);
			// not picky - if the trace ended anywhere in the box we're good
			if (BoxesOverlap(trace.endpos, trace.endpos, padmins, padmaxs))
				return true;
		}
	}
	else
	{
		// try center
		VectorMAM(0.5f, boxmins, 0.5f, boxmaxs, end);
		if (model->brush.TraceLineOfSight(model, start, end, padmins, padmaxs))
			return true;
	}

	// try various random positions
	for (j = 0; j < numsamples; j++)
	{
		VectorSet(start, lhrandom(eyemins[0], eyemaxs[0]), lhrandom(eyemins[1], eyemaxs[1]), lhrandom(eyemins[2], eyemaxs[2]));
		VectorSet(end, lhrandom(boxmins[0], boxmaxs[0]), lhrandom(boxmins[1], boxmaxs[1]), lhrandom(boxmins[2], boxmaxs[2]));
		if (r_cullentities_trace_entityocclusion.integer)
		{
			trace_t trace = CL_TraceLine(start, end, MOVE_NORMAL, NULL, SUPERCONTENTS_SOLID, SUPERCONTENTS_SKY, MATERIALFLAGMASK_TRANSLUCENT, 0.0f, true, false, NULL, true, true);
			// not picky - if the trace ended anywhere in the box we're good
			if (BoxesOverlap(trace.endpos, trace.endpos, padmins, padmaxs))
				return true;
		}
		else if (model->brush.TraceLineOfSight(model, start, end, padmins, padmaxs))
			return true;
	}

	return false;
}


static void R_View_UpdateEntityVisible (void)
{
	int i;
	int renderimask;
	int samples;
	entity_render_t *ent;

	if (r_refdef.envmap || r_fb.water.hideplayer)
		renderimask = RENDER_EXTERIORMODEL | RENDER_VIEWMODEL;
	else if (chase_active.integer || r_fb.water.renderingscene)
		renderimask = RENDER_VIEWMODEL;
	else
		renderimask = RENDER_EXTERIORMODEL;
	if (!r_drawviewmodel.integer)
		renderimask |= RENDER_VIEWMODEL;
	if (!r_drawexteriormodel.integer)
		renderimask |= RENDER_EXTERIORMODEL;
	memset(r_refdef.viewcache.entityvisible, 0, r_refdef.scene.numentities);
	if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs)
	{
		// worldmodel can check visibility
		for (i = 0;i < r_refdef.scene.numentities;i++)
		{
			ent = r_refdef.scene.entities[i];
			if (r_refdef.viewcache.world_novis && !(ent->flags & RENDER_VIEWMODEL))
			{
				r_refdef.viewcache.entityvisible[i] = false;
				continue;
			}
			if (!(ent->flags & renderimask))
			if (!R_CullFrustum(ent->mins, ent->maxs) || (ent->model && ent->model->type == mod_sprite && (ent->model->sprite.sprnum_type == SPR_LABEL || ent->model->sprite.sprnum_type == SPR_LABEL_SCALE)))
			if ((ent->flags & (RENDER_NODEPTHTEST | RENDER_WORLDOBJECT | RENDER_VIEWMODEL)) || r_refdef.scene.worldmodel->brush.BoxTouchingVisibleLeafs(r_refdef.scene.worldmodel, r_refdef.viewcache.world_leafvisible, ent->mins, ent->maxs))
				r_refdef.viewcache.entityvisible[i] = true;
		}
	}
	else
	{
		// no worldmodel or it can't check visibility
		for (i = 0;i < r_refdef.scene.numentities;i++)
		{
			ent = r_refdef.scene.entities[i];
			if (!(ent->flags & renderimask))
			if (!R_CullFrustum(ent->mins, ent->maxs) || (ent->model && ent->model->type == mod_sprite && (ent->model->sprite.sprnum_type == SPR_LABEL || ent->model->sprite.sprnum_type == SPR_LABEL_SCALE)))
				r_refdef.viewcache.entityvisible[i] = true;
		}
	}
	if (r_cullentities_trace.integer)
	{
		for (i = 0;i < r_refdef.scene.numentities;i++)
		{
			if (!r_refdef.viewcache.entityvisible[i])
				continue;
			ent = r_refdef.scene.entities[i];
			if (!(ent->flags & (RENDER_VIEWMODEL | RENDER_WORLDOBJECT | RENDER_NODEPTHTEST)) && !(ent->model && (ent->model->name[0] == '*')))
			{
				samples = ent->last_trace_visibility == 0 ? r_cullentities_trace_tempentitysamples.integer : r_cullentities_trace_samples.integer;
				if (R_CanSeeBox(samples, r_cullentities_trace_eyejitter.value, r_cullentities_trace_enlarge.value, r_cullentities_trace_expand.value, r_cullentities_trace_pad.value, r_refdef.view.origin, ent->mins, ent->maxs))
					ent->last_trace_visibility = host.realtime;
				if (ent->last_trace_visibility < host.realtime - r_cullentities_trace_delay.value)
					r_refdef.viewcache.entityvisible[i] = 0;
			}
		}
	}
}

/// only used if skyrendermasked, and normally returns false
static int R_DrawBrushModelsSky (void)
{
	int i, sky;
	entity_render_t *ent;

	sky = false;
	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (!ent->model || !ent->model->DrawSky)
			continue;
		ent->model->DrawSky(ent);
		sky = true;
	}
	return sky;
}

static void R_DrawNoModel(entity_render_t *ent);
static void R_DrawModels(void)
{
	int i;
	entity_render_t *ent;

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		r_refdef.stats[r_stat_entities]++;

		if (ent->model && ent->model->Draw != NULL)
			ent->model->Draw(ent);
		else
			R_DrawNoModel(ent);
	}
}

static void R_DrawModelsDepth(void)
{
	int i;
	entity_render_t *ent;

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (ent->model && ent->model->DrawDepth != NULL)
			ent->model->DrawDepth(ent);
	}
}

static void R_DrawModelsDebug(void)
{
	int i;
	entity_render_t *ent;

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (ent->model && ent->model->DrawDebug != NULL)
			ent->model->DrawDebug(ent);
	}
}

static void R_DrawModelsAddWaterPlanes(void)
{
	int i;
	entity_render_t *ent;

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		ent = r_refdef.scene.entities[i];
		if (ent->model && ent->model->DrawAddWaterPlanes != NULL)
			ent->model->DrawAddWaterPlanes(ent);
	}
}

static float irisvecs[7][3] = {{0, 0, 0}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};

void R_HDR_UpdateIrisAdaptation(const vec3_t point)
{
	if (r_hdr_irisadaptation.integer)
	{
		vec3_t p;
		vec3_t ambient;
		vec3_t diffuse;
		vec3_t diffusenormal;
		vec3_t forward;
		vec_t brightness = 0.0f;
		vec_t goal;
		vec_t current;
		vec_t d;
		int c;
		VectorCopy(r_refdef.view.forward, forward);
		for (c = 0;c < (int)(sizeof(irisvecs)/sizeof(irisvecs[0]));c++)
		{
			p[0] = point[0] + irisvecs[c][0] * r_hdr_irisadaptation_radius.value;
			p[1] = point[1] + irisvecs[c][1] * r_hdr_irisadaptation_radius.value;
			p[2] = point[2] + irisvecs[c][2] * r_hdr_irisadaptation_radius.value;
			R_CompleteLightPoint(ambient, diffuse, diffusenormal, p, LP_LIGHTMAP | LP_RTWORLD | LP_DYNLIGHT, r_refdef.scene.lightmapintensity, r_refdef.scene.ambientintensity);
			d = DotProduct(forward, diffusenormal);
			brightness += VectorLength(ambient);
			if (d > 0)
				brightness += d * VectorLength(diffuse);
		}
		brightness *= 1.0f / c;
		brightness += 0.00001f; // make sure it's never zero
		goal = r_hdr_irisadaptation_multiplier.value / brightness;
		goal = bound(r_hdr_irisadaptation_minvalue.value, goal, r_hdr_irisadaptation_maxvalue.value);
		current = r_hdr_irisadaptation_value.value;
		if (current < goal)
			current = min(current + r_hdr_irisadaptation_fade_up.value * cl.realframetime, goal);
		else if (current > goal)
			current = max(current - r_hdr_irisadaptation_fade_down.value * cl.realframetime, goal);
		if (fabs(r_hdr_irisadaptation_value.value - current) > 0.0001f)
			Cvar_SetValueQuick(&r_hdr_irisadaptation_value, current);
	}
	else if (r_hdr_irisadaptation_value.value != 1.0f)
		Cvar_SetValueQuick(&r_hdr_irisadaptation_value, 1.0f);
}

extern cvar_t r_lockvisibility;
extern cvar_t r_lockpvs;

static void R_View_SetFrustum(const int *scissor)
{
	int i;
	double fpx = +1, fnx = -1, fpy = +1, fny = -1;
	vec3_t forward, left, up, origin, v;
	if(r_lockvisibility.integer)
		return;
	if(scissor)
	{
		// flipped x coordinates (because x points left here)
		fpx =  1.0 - 2.0 * (scissor[0]              - r_refdef.view.viewport.x) / (double) (r_refdef.view.viewport.width);
		fnx =  1.0 - 2.0 * (scissor[0] + scissor[2] - r_refdef.view.viewport.x) / (double) (r_refdef.view.viewport.width);
		// non-flipped y coordinates
		fny = -1.0 + 2.0 * (scissor[1]              - r_refdef.view.viewport.y) / (double) (r_refdef.view.viewport.height);
		fpy = -1.0 + 2.0 * (scissor[1] + scissor[3] - r_refdef.view.viewport.y) / (double) (r_refdef.view.viewport.height);
	}

	// we can't trust r_refdef.view.forward and friends in reflected scenes
	Matrix4x4_ToVectors(&r_refdef.view.matrix, forward, left, up, origin);

#if 0
	r_refdef.view.frustum[0].normal[0] = 0 - 1.0 / r_refdef.view.frustum_x;
	r_refdef.view.frustum[0].normal[1] = 0 - 0;
	r_refdef.view.frustum[0].normal[2] = -1 - 0;
	r_refdef.view.frustum[1].normal[0] = 0 + 1.0 / r_refdef.view.frustum_x;
	r_refdef.view.frustum[1].normal[1] = 0 + 0;
	r_refdef.view.frustum[1].normal[2] = -1 + 0;
	r_refdef.view.frustum[2].normal[0] = 0 - 0;
	r_refdef.view.frustum[2].normal[1] = 0 - 1.0 / r_refdef.view.frustum_y;
	r_refdef.view.frustum[2].normal[2] = -1 - 0;
	r_refdef.view.frustum[3].normal[0] = 0 + 0;
	r_refdef.view.frustum[3].normal[1] = 0 + 1.0 / r_refdef.view.frustum_y;
	r_refdef.view.frustum[3].normal[2] = -1 + 0;
#endif

#if 0
	zNear = r_refdef.nearclip;
	nudge = 1.0 - 1.0 / (1<<23);
	r_refdef.view.frustum[4].normal[0] = 0 - 0;
	r_refdef.view.frustum[4].normal[1] = 0 - 0;
	r_refdef.view.frustum[4].normal[2] = -1 - -nudge;
	r_refdef.view.frustum[4].dist = 0 - -2 * zNear * nudge;
	r_refdef.view.frustum[5].normal[0] = 0 + 0;
	r_refdef.view.frustum[5].normal[1] = 0 + 0;
	r_refdef.view.frustum[5].normal[2] = -1 + -nudge;
	r_refdef.view.frustum[5].dist = 0 + -2 * zNear * nudge;
#endif



#if 0
	r_refdef.view.frustum[0].normal[0] = m[3] - m[0];
	r_refdef.view.frustum[0].normal[1] = m[7] - m[4];
	r_refdef.view.frustum[0].normal[2] = m[11] - m[8];
	r_refdef.view.frustum[0].dist = m[15] - m[12];

	r_refdef.view.frustum[1].normal[0] = m[3] + m[0];
	r_refdef.view.frustum[1].normal[1] = m[7] + m[4];
	r_refdef.view.frustum[1].normal[2] = m[11] + m[8];
	r_refdef.view.frustum[1].dist = m[15] + m[12];

	r_refdef.view.frustum[2].normal[0] = m[3] - m[1];
	r_refdef.view.frustum[2].normal[1] = m[7] - m[5];
	r_refdef.view.frustum[2].normal[2] = m[11] - m[9];
	r_refdef.view.frustum[2].dist = m[15] - m[13];

	r_refdef.view.frustum[3].normal[0] = m[3] + m[1];
	r_refdef.view.frustum[3].normal[1] = m[7] + m[5];
	r_refdef.view.frustum[3].normal[2] = m[11] + m[9];
	r_refdef.view.frustum[3].dist = m[15] + m[13];

	r_refdef.view.frustum[4].normal[0] = m[3] - m[2];
	r_refdef.view.frustum[4].normal[1] = m[7] - m[6];
	r_refdef.view.frustum[4].normal[2] = m[11] - m[10];
	r_refdef.view.frustum[4].dist = m[15] - m[14];

	r_refdef.view.frustum[5].normal[0] = m[3] + m[2];
	r_refdef.view.frustum[5].normal[1] = m[7] + m[6];
	r_refdef.view.frustum[5].normal[2] = m[11] + m[10];
	r_refdef.view.frustum[5].dist = m[15] + m[14];
#endif

	if (r_refdef.view.useperspective)
	{
		// calculate frustum corners, which are used to calculate deformed frustum planes for shadow caster culling
		VectorMAMAM(1024, forward, fnx * 1024.0 * r_refdef.view.frustum_x, left, fny * 1024.0 * r_refdef.view.frustum_y, up, r_refdef.view.frustumcorner[0]);
		VectorMAMAM(1024, forward, fpx * 1024.0 * r_refdef.view.frustum_x, left, fny * 1024.0 * r_refdef.view.frustum_y, up, r_refdef.view.frustumcorner[1]);
		VectorMAMAM(1024, forward, fnx * 1024.0 * r_refdef.view.frustum_x, left, fpy * 1024.0 * r_refdef.view.frustum_y, up, r_refdef.view.frustumcorner[2]);
		VectorMAMAM(1024, forward, fpx * 1024.0 * r_refdef.view.frustum_x, left, fpy * 1024.0 * r_refdef.view.frustum_y, up, r_refdef.view.frustumcorner[3]);

		// then the normals from the corners relative to origin
		CrossProduct(r_refdef.view.frustumcorner[2], r_refdef.view.frustumcorner[0], r_refdef.view.frustum[0].normal);
		CrossProduct(r_refdef.view.frustumcorner[1], r_refdef.view.frustumcorner[3], r_refdef.view.frustum[1].normal);
		CrossProduct(r_refdef.view.frustumcorner[0], r_refdef.view.frustumcorner[1], r_refdef.view.frustum[2].normal);
		CrossProduct(r_refdef.view.frustumcorner[3], r_refdef.view.frustumcorner[2], r_refdef.view.frustum[3].normal);

		// in a NORMAL view, forward cross left == up
		// in a REFLECTED view, forward cross left == down
		// so our cross products above need to be adjusted for a left handed coordinate system
		CrossProduct(forward, left, v);
		if(DotProduct(v, up) < 0)
		{
			VectorNegate(r_refdef.view.frustum[0].normal, r_refdef.view.frustum[0].normal);
			VectorNegate(r_refdef.view.frustum[1].normal, r_refdef.view.frustum[1].normal);
			VectorNegate(r_refdef.view.frustum[2].normal, r_refdef.view.frustum[2].normal);
			VectorNegate(r_refdef.view.frustum[3].normal, r_refdef.view.frustum[3].normal);
		}

		// Leaving those out was a mistake, those were in the old code, and they
		// fix a reproducable bug in this one: frustum culling got fucked up when viewmatrix was an identity matrix
		// I couldn't reproduce it after adding those normalizations. --blub
		VectorNormalize(r_refdef.view.frustum[0].normal);
		VectorNormalize(r_refdef.view.frustum[1].normal);
		VectorNormalize(r_refdef.view.frustum[2].normal);
		VectorNormalize(r_refdef.view.frustum[3].normal);

		// make the corners absolute
		VectorAdd(r_refdef.view.frustumcorner[0], r_refdef.view.origin, r_refdef.view.frustumcorner[0]);
		VectorAdd(r_refdef.view.frustumcorner[1], r_refdef.view.origin, r_refdef.view.frustumcorner[1]);
		VectorAdd(r_refdef.view.frustumcorner[2], r_refdef.view.origin, r_refdef.view.frustumcorner[2]);
		VectorAdd(r_refdef.view.frustumcorner[3], r_refdef.view.origin, r_refdef.view.frustumcorner[3]);

		// one more normal
		VectorCopy(forward, r_refdef.view.frustum[4].normal);

		r_refdef.view.frustum[0].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[0].normal);
		r_refdef.view.frustum[1].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[1].normal);
		r_refdef.view.frustum[2].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[2].normal);
		r_refdef.view.frustum[3].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[3].normal);
		r_refdef.view.frustum[4].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[4].normal) + r_refdef.nearclip;
	}
	else
	{
		VectorScale(left, -1.0f, r_refdef.view.frustum[0].normal);
		VectorScale(left,  1.0f, r_refdef.view.frustum[1].normal);
		VectorScale(up, -1.0f, r_refdef.view.frustum[2].normal);
		VectorScale(up,  1.0f, r_refdef.view.frustum[3].normal);
		VectorScale(forward, -1.0f, r_refdef.view.frustum[4].normal);
		r_refdef.view.frustum[0].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[0].normal) - r_refdef.view.ortho_x;
		r_refdef.view.frustum[1].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[1].normal) - r_refdef.view.ortho_x;
		r_refdef.view.frustum[2].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[2].normal) - r_refdef.view.ortho_y;
		r_refdef.view.frustum[3].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[3].normal) - r_refdef.view.ortho_y;
		r_refdef.view.frustum[4].dist = DotProduct (r_refdef.view.origin, r_refdef.view.frustum[4].normal) - r_refdef.farclip;
	}
	r_refdef.view.numfrustumplanes = 5;

	if (r_refdef.view.useclipplane)
	{
		r_refdef.view.numfrustumplanes = 6;
		r_refdef.view.frustum[5] = r_refdef.view.clipplane;
	}

	for (i = 0;i < r_refdef.view.numfrustumplanes;i++)
		PlaneClassify(r_refdef.view.frustum + i);

	// LadyHavoc: note to all quake engine coders, Quake had a special case
	// for 90 degrees which assumed a square view (wrong), so I removed it,
	// Quake2 has it disabled as well.

	// rotate R_VIEWFORWARD right by FOV_X/2 degrees
	//RotatePointAroundVector( r_refdef.view.frustum[0].normal, up, forward, -(90 - r_refdef.fov_x / 2));
	//r_refdef.view.frustum[0].dist = DotProduct (r_refdef.view.origin, frustum[0].normal);
	//PlaneClassify(&frustum[0]);

	// rotate R_VIEWFORWARD left by FOV_X/2 degrees
	//RotatePointAroundVector( r_refdef.view.frustum[1].normal, up, forward, (90 - r_refdef.fov_x / 2));
	//r_refdef.view.frustum[1].dist = DotProduct (r_refdef.view.origin, frustum[1].normal);
	//PlaneClassify(&frustum[1]);

	// rotate R_VIEWFORWARD up by FOV_X/2 degrees
	//RotatePointAroundVector( r_refdef.view.frustum[2].normal, left, forward, -(90 - r_refdef.fov_y / 2));
	//r_refdef.view.frustum[2].dist = DotProduct (r_refdef.view.origin, frustum[2].normal);
	//PlaneClassify(&frustum[2]);

	// rotate R_VIEWFORWARD down by FOV_X/2 degrees
	//RotatePointAroundVector( r_refdef.view.frustum[3].normal, left, forward, (90 - r_refdef.fov_y / 2));
	//r_refdef.view.frustum[3].dist = DotProduct (r_refdef.view.origin, frustum[3].normal);
	//PlaneClassify(&frustum[3]);

	// nearclip plane
	//VectorCopy(forward, r_refdef.view.frustum[4].normal);
	//r_refdef.view.frustum[4].dist = DotProduct (r_refdef.view.origin, frustum[4].normal) + r_nearclip.value;
	//PlaneClassify(&frustum[4]);
}

static void R_View_Update(const int *myscissor)
{
	R_Main_ResizeViewCache();
	R_View_SetFrustum(myscissor);
	R_View_WorldVisibility(!r_refdef.view.usevieworiginculling);
	R_View_UpdateEntityVisible();
}

float viewscalefpsadjusted = 1.0f;

void R_SetupView(qbool allowwaterclippingplane, int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	const float *customclipplane = NULL;
	float plane[4];
	int viewy_adjusted;
	if (r_refdef.view.useclipplane && allowwaterclippingplane)
	{
		// LadyHavoc: couldn't figure out how to make this approach work the same in DPSOFTRAST
		vec_t dist = r_refdef.view.clipplane.dist - r_water_clippingplanebias.value;
		vec_t viewdist = DotProduct(r_refdef.view.origin, r_refdef.view.clipplane.normal);
		if (viewdist < r_refdef.view.clipplane.dist + r_water_clippingplanebias.value)
			dist = r_refdef.view.clipplane.dist;
		plane[0] = r_refdef.view.clipplane.normal[0];
		plane[1] = r_refdef.view.clipplane.normal[1];
		plane[2] = r_refdef.view.clipplane.normal[2];
		plane[3] = -dist;
		customclipplane = plane;
	}

	// GL is weird because it's bottom to top, r_refdef.view.y is top to bottom.
	// Unless the render target is a FBO...
	viewy_adjusted = viewfbo ? viewy : vid.mode.height - viewheight - viewy;

	if (!r_refdef.view.useperspective)
		R_Viewport_InitOrtho3D(&r_refdef.view.viewport, &r_refdef.view.matrix, viewx, viewy_adjusted, viewwidth, viewheight, r_refdef.view.ortho_x, r_refdef.view.ortho_y, -r_refdef.farclip, r_refdef.farclip, customclipplane);
	else if (vid.stencil && r_useinfinitefarclip.integer)
		R_Viewport_InitPerspectiveInfinite(&r_refdef.view.viewport, &r_refdef.view.matrix, viewx, viewy_adjusted, viewwidth, viewheight, r_refdef.view.frustum_x, r_refdef.view.frustum_y, r_refdef.nearclip, customclipplane);
	else
		R_Viewport_InitPerspective(&r_refdef.view.viewport, &r_refdef.view.matrix, viewx, viewy_adjusted, viewwidth, viewheight, r_refdef.view.frustum_x, r_refdef.view.frustum_y, r_refdef.nearclip, r_refdef.farclip, customclipplane);
	R_Mesh_SetRenderTargets(viewfbo, viewdepthtexture, viewcolortexture, NULL, NULL, NULL);
	R_SetViewport(&r_refdef.view.viewport);
}

void R_EntityMatrix(const matrix4x4_t *matrix)
{
	if (gl_modelmatrixchanged || memcmp(matrix, &gl_modelmatrix, sizeof(matrix4x4_t)))
	{
		gl_modelmatrixchanged = false;
		gl_modelmatrix = *matrix;
		Matrix4x4_Concat(&gl_modelviewmatrix, &gl_viewmatrix, &gl_modelmatrix);
		Matrix4x4_Concat(&gl_modelviewprojectionmatrix, &gl_projectionmatrix, &gl_modelviewmatrix);
		Matrix4x4_ToArrayFloatGL(&gl_modelviewmatrix, gl_modelview16f);
		Matrix4x4_ToArrayFloatGL(&gl_modelviewprojectionmatrix, gl_modelviewprojection16f);
		CHECKGLERROR
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			if (r_glsl_permutation && r_glsl_permutation->loc_ModelViewProjectionMatrix >= 0) qglUniformMatrix4fv(r_glsl_permutation->loc_ModelViewProjectionMatrix, 1, false, gl_modelviewprojection16f);
			if (r_glsl_permutation && r_glsl_permutation->loc_ModelViewMatrix >= 0) qglUniformMatrix4fv(r_glsl_permutation->loc_ModelViewMatrix, 1, false, gl_modelview16f);
			break;
		}
	}
}

void R_ResetViewRendering2D_Common(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight, float x2, float y2)
{
	r_viewport_t viewport;
	int viewy_adjusted;

	CHECKGLERROR

	// GL is weird because it's bottom to top, r_refdef.view.y is top to bottom.
	// Unless the render target is a FBO...
	viewy_adjusted = viewfbo ? viewy : vid.mode.height - viewheight - viewy;

	R_Viewport_InitOrtho(&viewport, &identitymatrix, viewx, viewy_adjusted, viewwidth, viewheight, 0, 0, x2, y2, -10, 100, NULL);
	R_Mesh_SetRenderTargets(viewfbo, viewdepthtexture, viewcolortexture, NULL, NULL, NULL);
	R_SetViewport(&viewport);
	GL_Scissor(viewport.x, viewport.y, viewport.width, viewport.height);
	GL_Color(1, 1, 1, 1);
	GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_ScissorTest(false);
	GL_DepthMask(false);
	GL_DepthRange(0, 1);
	GL_DepthTest(false);
	GL_DepthFunc(GL_LEQUAL);
	R_EntityMatrix(&identitymatrix);
	R_Mesh_ResetTextureState();
	GL_PolygonOffset(0, 0);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		qglEnable(GL_POLYGON_OFFSET_FILL);CHECKGLERROR
		break;
	}
	GL_CullFace(GL_NONE);

	CHECKGLERROR
}

void R_ResetViewRendering2D(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	R_ResetViewRendering2D_Common(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight, 1.0f, 1.0f);
}

void R_ResetViewRendering3D(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	R_SetupView(true, viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	GL_Color(1, 1, 1, 1);
	GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_ScissorTest(true);
	GL_DepthMask(true);
	GL_DepthRange(0, 1);
	GL_DepthTest(true);
	GL_DepthFunc(GL_LEQUAL);
	R_EntityMatrix(&identitymatrix);
	R_Mesh_ResetTextureState();
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		qglEnable(GL_POLYGON_OFFSET_FILL);CHECKGLERROR
		break;
	}
	GL_CullFace(r_refdef.view.cullface_back);
}

/*
================
R_RenderView_UpdateViewVectors
================
*/
void R_RenderView_UpdateViewVectors(void)
{
	// break apart the view matrix into vectors for various purposes
	// it is important that this occurs outside the RenderScene function because that can be called from reflection renders, where the vectors come out wrong
	// however the r_refdef.view.origin IS updated in RenderScene intentionally - otherwise the sky renders at the wrong origin, etc
	Matrix4x4_ToVectors(&r_refdef.view.matrix, r_refdef.view.forward, r_refdef.view.left, r_refdef.view.up, r_refdef.view.origin);
	VectorNegate(r_refdef.view.left, r_refdef.view.right);
	// make an inverted copy of the view matrix for tracking sprites
	Matrix4x4_Invert_Full(&r_refdef.view.inverse_matrix, &r_refdef.view.matrix);
}

void R_RenderTarget_FreeUnused(qbool force)
{
	unsigned int i, j, end;
	end = (unsigned int)Mem_ExpandableArray_IndexRange(&r_fb.rendertargets); // checked
	for (i = 0; i < end; i++)
	{
		r_rendertarget_t *r = (r_rendertarget_t *)Mem_ExpandableArray_RecordAtIndex(&r_fb.rendertargets, i);
		// free resources for rendertargets that have not been used for a while
		// (note: this check is run after the frame render, so any targets used
		// this frame will not be affected even at low framerates)
		if (r && (host.realtime - r->lastusetime > 0.2 || force))
		{
			if (r->fbo)
				R_Mesh_DestroyFramebufferObject(r->fbo);
			for (j = 0; j < sizeof(r->colortexture) / sizeof(r->colortexture[0]); j++)
				if (r->colortexture[j])
					R_FreeTexture(r->colortexture[j]);
			if (r->depthtexture)
				R_FreeTexture(r->depthtexture);
			Mem_ExpandableArray_FreeRecord(&r_fb.rendertargets, r);
		}
	}
}

static void R_CalcTexCoordsForView(float x, float y, float w, float h, float tw, float th, float *texcoord2f)
{
	float iw = 1.0f / tw, ih = 1.0f / th, x1, y1, x2, y2;
	x1 = x * iw;
	x2 = (x + w) * iw;
	y1 = (th - y) * ih;
	y2 = (th - y - h) * ih;
	texcoord2f[0] = x1;
	texcoord2f[2] = x2;
	texcoord2f[4] = x2;
	texcoord2f[6] = x1;
	texcoord2f[1] = y1;
	texcoord2f[3] = y1;
	texcoord2f[5] = y2;
	texcoord2f[7] = y2;
}

r_rendertarget_t *R_RenderTarget_Get(int texturewidth, int textureheight, textype_t depthtextype, qbool depthisrenderbuffer, textype_t colortextype0, textype_t colortextype1, textype_t colortextype2, textype_t colortextype3)
{
	unsigned int i, j, end;
	r_rendertarget_t *r = NULL;
	char vabuf[256];
	// first try to reuse an existing slot if possible
	end = (unsigned int)Mem_ExpandableArray_IndexRange(&r_fb.rendertargets); // checked
	for (i = 0; i < end; i++)
	{
		r = (r_rendertarget_t *)Mem_ExpandableArray_RecordAtIndex(&r_fb.rendertargets, i);
		if (r && r->lastusetime != host.realtime && r->texturewidth == texturewidth && r->textureheight == textureheight && r->depthtextype == depthtextype && r->colortextype[0] == colortextype0 && r->colortextype[1] == colortextype1 && r->colortextype[2] == colortextype2 && r->colortextype[3] == colortextype3)
			break;
	}
	if (i == end)
	{
		// no unused exact match found, so we have to make one in the first unused slot
		r = (r_rendertarget_t *)Mem_ExpandableArray_AllocRecord(&r_fb.rendertargets);
		r->texturewidth = texturewidth;
		r->textureheight = textureheight;
		r->colortextype[0] = colortextype0;
		r->colortextype[1] = colortextype1;
		r->colortextype[2] = colortextype2;
		r->colortextype[3] = colortextype3;
		r->depthtextype = depthtextype;
		r->depthisrenderbuffer = depthisrenderbuffer;
		for (j = 0; j < 4; j++)
			if (r->colortextype[j])
				r->colortexture[j] = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "rendertarget%i_%i_type%i", i, j, (int)r->colortextype[j]), r->texturewidth, r->textureheight, NULL, r->colortextype[j], TEXF_RENDERTARGET | TEXF_FORCELINEAR | TEXF_CLAMP, -1, NULL);
		if (r->depthtextype)
		{
			if (r->depthisrenderbuffer)
				r->depthtexture = R_LoadTextureRenderBuffer(r_main_texturepool, va(vabuf, sizeof(vabuf), "renderbuffer%i_depth_type%i", i, (int)r->depthtextype), r->texturewidth, r->textureheight, r->depthtextype);
			else
				r->depthtexture = R_LoadTexture2D(r_main_texturepool, va(vabuf, sizeof(vabuf), "rendertarget%i_depth_type%i", i, (int)r->depthtextype), r->texturewidth, r->textureheight, NULL, r->depthtextype, TEXF_RENDERTARGET | TEXF_FORCELINEAR | TEXF_CLAMP, -1, NULL);
		}
		r->fbo = R_Mesh_CreateFramebufferObject(r->depthtexture, r->colortexture[0], r->colortexture[1], r->colortexture[2], r->colortexture[3]);
	}
	r_refdef.stats[r_stat_rendertargets_used]++;
	r_refdef.stats[r_stat_rendertargets_pixels] += r->texturewidth * r->textureheight;
	r->lastusetime = host.realtime;
	R_CalcTexCoordsForView(0, 0, r->texturewidth, r->textureheight, r->texturewidth, r->textureheight, r->texcoord2f);
	return r;
}

static void R_Water_StartFrame(int viewwidth, int viewheight)
{
	int waterwidth, waterheight;

	if (viewwidth > (int)vid.maxtexturesize_2d || viewheight > (int)vid.maxtexturesize_2d)
		return;

	// set waterwidth and waterheight to the water resolution that will be
	// used (often less than the screen resolution for faster rendering)
	waterwidth = (int)bound(16, viewwidth * r_water_resolutionmultiplier.value, viewwidth);
	waterheight = (int)bound(16, viewheight * r_water_resolutionmultiplier.value, viewheight);

	if (!r_water.integer || r_showsurfaces.integer || r_lockvisibility.integer || r_lockpvs.integer)
		waterwidth = waterheight = 0;

	// set up variables that will be used in shader setup
	r_fb.water.waterwidth = waterwidth;
	r_fb.water.waterheight = waterheight;
	r_fb.water.texturewidth = waterwidth;
	r_fb.water.textureheight = waterheight;
	r_fb.water.camerawidth = waterwidth;
	r_fb.water.cameraheight = waterheight;
	r_fb.water.screenscale[0] = 0.5f;
	r_fb.water.screenscale[1] = 0.5f;
	r_fb.water.screencenter[0] = 0.5f;
	r_fb.water.screencenter[1] = 0.5f;
	r_fb.water.enabled = waterwidth != 0;

	r_fb.water.maxwaterplanes = MAX_WATERPLANES;
	r_fb.water.numwaterplanes = 0;
}

void R_Water_AddWaterPlane(msurface_t *surface, int entno)
{
	int planeindex, bestplaneindex, vertexindex;
	vec3_t mins, maxs, normal, center, v, n;
	vec_t planescore, bestplanescore;
	mplane_t plane;
	r_waterstate_waterplane_t *p;
	texture_t *t = R_GetCurrentTexture(surface->texture);

	rsurface.texture = t;
	RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_NOGAPS, 1, ((const msurface_t **)&surface));
	// if the model has no normals, it's probably off-screen and they were not generated, so don't add it anyway
	if (!rsurface.batchnormal3f || rsurface.batchnumvertices < 1)
		return;
	// average the vertex normals, find the surface bounds (after deformvertexes)
	Matrix4x4_Transform(&rsurface.matrix, rsurface.batchvertex3f, v);
	Matrix4x4_Transform3x3(&rsurface.matrix, rsurface.batchnormal3f, n);
	VectorCopy(n, normal);
	VectorCopy(v, mins);
	VectorCopy(v, maxs);
	for (vertexindex = 1;vertexindex < rsurface.batchnumvertices;vertexindex++)
	{
		Matrix4x4_Transform(&rsurface.matrix, rsurface.batchvertex3f + vertexindex*3, v);
		Matrix4x4_Transform3x3(&rsurface.matrix, rsurface.batchnormal3f + vertexindex*3, n);
		VectorAdd(normal, n, normal);
		mins[0] = min(mins[0], v[0]);
		mins[1] = min(mins[1], v[1]);
		mins[2] = min(mins[2], v[2]);
		maxs[0] = max(maxs[0], v[0]);
		maxs[1] = max(maxs[1], v[1]);
		maxs[2] = max(maxs[2], v[2]);
	}
	VectorNormalize(normal);
	VectorMAM(0.5f, mins, 0.5f, maxs, center);

	VectorCopy(normal, plane.normal);
	VectorNormalize(plane.normal);
	plane.dist = DotProduct(center, plane.normal);
	PlaneClassify(&plane);
	if (PlaneDiff(r_refdef.view.origin, &plane) < 0)
	{
		// skip backfaces (except if nocullface is set)
//		if (!(t->currentmaterialflags & MATERIALFLAG_NOCULLFACE))
//			return;
		VectorNegate(plane.normal, plane.normal);
		plane.dist *= -1;
		PlaneClassify(&plane);
	}


	// find a matching plane if there is one
	bestplaneindex = -1;
	bestplanescore = 1048576.0f;
	for (planeindex = 0, p = r_fb.water.waterplanes;planeindex < r_fb.water.numwaterplanes;planeindex++, p++)
	{
		if(p->camera_entity == t->camera_entity)
		{
			planescore = 1.0f - DotProduct(plane.normal, p->plane.normal) + fabs(plane.dist - p->plane.dist) * 0.001f;
			if (bestplaneindex < 0 || bestplanescore > planescore)
			{
				bestplaneindex = planeindex;
				bestplanescore = planescore;
			}
		}
	}
	planeindex = bestplaneindex;

	// if this surface does not fit any known plane rendered this frame, add one
	if (planeindex < 0 || bestplanescore > 0.001f)
	{
		if (r_fb.water.numwaterplanes < r_fb.water.maxwaterplanes)
		{
			// store the new plane
			planeindex = r_fb.water.numwaterplanes++;
			p = r_fb.water.waterplanes + planeindex;
			p->plane = plane;
			// clear materialflags and pvs
			p->materialflags = 0;
			p->pvsvalid = false;
			p->camera_entity = t->camera_entity;
			VectorCopy(mins, p->mins);
			VectorCopy(maxs, p->maxs);
		}
		else
		{
			// We're totally screwed.
			return;
		}
	}
	else
	{
		// merge mins/maxs when we're adding this surface to the plane
		p = r_fb.water.waterplanes + planeindex;
		p->mins[0] = min(p->mins[0], mins[0]);
		p->mins[1] = min(p->mins[1], mins[1]);
		p->mins[2] = min(p->mins[2], mins[2]);
		p->maxs[0] = max(p->maxs[0], maxs[0]);
		p->maxs[1] = max(p->maxs[1], maxs[1]);
		p->maxs[2] = max(p->maxs[2], maxs[2]);
	}
	// merge this surface's materialflags into the waterplane
	p->materialflags |= t->currentmaterialflags;
	if(!(p->materialflags & MATERIALFLAG_CAMERA))
	{
		// merge this surface's PVS into the waterplane
		if (p->materialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_REFLECTION) && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.FatPVS
		 && r_refdef.scene.worldmodel->brush.PointInLeaf && r_refdef.scene.worldmodel->brush.PointInLeaf(r_refdef.scene.worldmodel, center)->clusterindex >= 0)
		{
			r_refdef.scene.worldmodel->brush.FatPVS(r_refdef.scene.worldmodel, center, 2, p->pvsbits, sizeof(p->pvsbits), p->pvsvalid);
			p->pvsvalid = true;
		}
	}
}

extern cvar_t r_drawparticles;
extern cvar_t r_drawdecals;

static void R_Water_ProcessPlanes(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	int myscissor[4];
	r_refdef_view_t originalview;
	r_refdef_view_t myview;
	int planeindex, qualityreduction = 0, old_r_dynamic = 0, old_r_shadows = 0, old_r_worldrtlight = 0, old_r_dlight = 0, old_r_particles = 0, old_r_decals = 0;
	r_waterstate_waterplane_t *p;
	vec3_t visorigin;
	r_rendertarget_t *rt;

	originalview = r_refdef.view;

	// lowquality hack, temporarily shut down some cvars and restore afterwards
	qualityreduction = r_water_lowquality.integer;
	if (qualityreduction > 0)
	{
		if (qualityreduction >= 1)
		{
			old_r_shadows = r_shadows.integer;
			old_r_worldrtlight = r_shadow_realtime_world.integer;
			old_r_dlight = r_shadow_realtime_dlight.integer;
			Cvar_SetValueQuick(&r_shadows, 0);
			Cvar_SetValueQuick(&r_shadow_realtime_world, 0);
			Cvar_SetValueQuick(&r_shadow_realtime_dlight, 0);
		}
		if (qualityreduction >= 2)
		{
			old_r_dynamic = r_dynamic.integer;
			old_r_particles = r_drawparticles.integer;
			old_r_decals = r_drawdecals.integer;
			Cvar_SetValueQuick(&r_dynamic, 0);
			Cvar_SetValueQuick(&r_drawparticles, 0);
			Cvar_SetValueQuick(&r_drawdecals, 0);
		}
	}

	for (planeindex = 0, p = r_fb.water.waterplanes; planeindex < r_fb.water.numwaterplanes; planeindex++, p++)
	{
		p->rt_reflection = NULL;
		p->rt_refraction = NULL;
		p->rt_camera = NULL;
	}

	// render views
	r_refdef.view = originalview;
	r_refdef.view.showdebug = false;
	r_refdef.view.width = r_fb.water.waterwidth;
	r_refdef.view.height = r_fb.water.waterheight;
	r_refdef.view.useclipplane = true;
	myview = r_refdef.view;
	r_fb.water.renderingscene = true;
	for (planeindex = 0, p = r_fb.water.waterplanes;planeindex < r_fb.water.numwaterplanes;planeindex++, p++)
	{
		if (r_water_cameraentitiesonly.value != 0 && !p->camera_entity)
			continue;

		if (p->materialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFLECTION))
		{
			rt = R_RenderTarget_Get(r_fb.water.waterwidth, r_fb.water.waterheight, TEXTYPE_DEPTHBUFFER24STENCIL8, true, r_fb.rt_screen->colortextype[0], TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
			if (rt->colortexture[0] == NULL || rt->depthtexture == NULL)
				goto error;
			r_refdef.view = myview;
			Matrix4x4_Reflect(&r_refdef.view.matrix, p->plane.normal[0], p->plane.normal[1], p->plane.normal[2], p->plane.dist, -2);
			Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, r_refdef.view.origin);
			if(r_water_scissormode.integer)
			{
				R_SetupView(true, rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, r_fb.water.waterwidth, r_fb.water.waterheight);
				if (R_ScissorForBBox(p->mins, p->maxs, myscissor))
				{
					p->rt_reflection = NULL;
					p->rt_refraction = NULL;
					p->rt_camera = NULL;
					continue;
				}
			}

			r_refdef.view.clipplane = p->plane;
			// reflected view origin may be in solid, so don't cull with it
			r_refdef.view.usevieworiginculling = false;
			// reverse the cullface settings for this render
			r_refdef.view.cullface_front = GL_FRONT;
			r_refdef.view.cullface_back = GL_BACK;
			// combined pvs (based on what can be seen from each surface center)
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.num_pvsclusterbytes)
			{
				r_refdef.view.usecustompvs = true;
				if (p->pvsvalid)
					memcpy(r_refdef.viewcache.world_pvsbits, p->pvsbits, r_refdef.scene.worldmodel->brush.num_pvsclusterbytes);
				else
					memset(r_refdef.viewcache.world_pvsbits, 0xFF, r_refdef.scene.worldmodel->brush.num_pvsclusterbytes);
			}

			r_fb.water.hideplayer = ((r_water_hideplayer.integer >= 2) && !chase_active.integer);
			R_ResetViewRendering3D(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);
			GL_ScissorTest(false);
			R_ClearScreen(r_refdef.fogenabled);
			GL_ScissorTest(true);
			R_View_Update(r_water_scissormode.integer & 2 ? myscissor : NULL);
			R_AnimCache_CacheVisibleEntities();
			if(r_water_scissormode.integer & 1)
				GL_Scissor(myscissor[0], myscissor[1], myscissor[2], myscissor[3]);
			R_RenderScene(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);

			r_fb.water.hideplayer = false;
			p->rt_reflection = rt;
		}

		// render the normal view scene and copy into texture
		// (except that a clipping plane should be used to hide everything on one side of the water, and the viewer's weapon model should be omitted)
		if (p->materialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION))
		{
			rt = R_RenderTarget_Get(r_fb.water.waterwidth, r_fb.water.waterheight, TEXTYPE_DEPTHBUFFER24STENCIL8, true, r_fb.rt_screen->colortextype[0], TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
			if (rt->colortexture[0] == NULL || rt->depthtexture == NULL)
				goto error;
			r_refdef.view = myview;
			if(r_water_scissormode.integer)
			{
				R_SetupView(true, rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, r_fb.water.waterwidth, r_fb.water.waterheight);
				if (R_ScissorForBBox(p->mins, p->maxs, myscissor))
				{
					p->rt_reflection = NULL;
					p->rt_refraction = NULL;
					p->rt_camera = NULL;
					continue;
				}
			}

			// combined pvs (based on what can be seen from each surface center)
			if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.num_pvsclusterbytes)
			{
				r_refdef.view.usecustompvs = true;
				if (p->pvsvalid)
					memcpy(r_refdef.viewcache.world_pvsbits, p->pvsbits, r_refdef.scene.worldmodel->brush.num_pvsclusterbytes);
				else
					memset(r_refdef.viewcache.world_pvsbits, 0xFF, r_refdef.scene.worldmodel->brush.num_pvsclusterbytes);
			}

			r_fb.water.hideplayer = ((r_water_hideplayer.integer >= 1) && !chase_active.integer);

			r_refdef.view.clipplane = p->plane;
			VectorNegate(r_refdef.view.clipplane.normal, r_refdef.view.clipplane.normal);
			r_refdef.view.clipplane.dist = -r_refdef.view.clipplane.dist;

			if((p->materialflags & MATERIALFLAG_CAMERA) && p->camera_entity)
			{
				// we need to perform a matrix transform to render the view... so let's get the transformation matrix
				r_fb.water.hideplayer = false; // we don't want to hide the player model from these ones
				CL_VM_TransformView(p->camera_entity - MAX_EDICTS, &r_refdef.view.matrix, &r_refdef.view.clipplane, visorigin);
				R_RenderView_UpdateViewVectors();
				if(r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.FatPVS)
				{
					r_refdef.view.usecustompvs = true;
					r_refdef.scene.worldmodel->brush.FatPVS(r_refdef.scene.worldmodel, visorigin, 2, r_refdef.viewcache.world_pvsbits, (r_refdef.viewcache.world_numclusters+7)>>3, false);
				}
			}

			PlaneClassify(&r_refdef.view.clipplane);

			R_ResetViewRendering3D(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);
			GL_ScissorTest(false);
			R_ClearScreen(r_refdef.fogenabled);
			GL_ScissorTest(true);
			R_View_Update(r_water_scissormode.integer & 2 ? myscissor : NULL);
			R_AnimCache_CacheVisibleEntities();
			if(r_water_scissormode.integer & 1)
				GL_Scissor(myscissor[0], myscissor[1], myscissor[2], myscissor[3]);
			R_RenderScene(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);

			r_fb.water.hideplayer = false;
			p->rt_refraction = rt;
		}
		else if (p->materialflags & MATERIALFLAG_CAMERA)
		{
			rt = R_RenderTarget_Get(r_fb.water.waterwidth, r_fb.water.waterheight, TEXTYPE_DEPTHBUFFER24STENCIL8, true, r_fb.rt_screen->colortextype[0], TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
			if (rt->colortexture[0] == NULL || rt->depthtexture == NULL)
				goto error;
			r_refdef.view = myview;

			r_refdef.view.clipplane = p->plane;
			VectorNegate(r_refdef.view.clipplane.normal, r_refdef.view.clipplane.normal);
			r_refdef.view.clipplane.dist = -r_refdef.view.clipplane.dist;

			r_refdef.view.width = r_fb.water.camerawidth;
			r_refdef.view.height = r_fb.water.cameraheight;
			r_refdef.view.frustum_x = 1; // tan(45 * M_PI / 180.0);
			r_refdef.view.frustum_y = 1; // tan(45 * M_PI / 180.0);
			r_refdef.view.ortho_x = 90; // abused as angle by VM_CL_R_SetView
			r_refdef.view.ortho_y = 90; // abused as angle by VM_CL_R_SetView

			if(p->camera_entity)
			{
				// we need to perform a matrix transform to render the view... so let's get the transformation matrix
				CL_VM_TransformView(p->camera_entity - MAX_EDICTS, &r_refdef.view.matrix, &r_refdef.view.clipplane, visorigin);
			}

			// note: all of the view is used for displaying... so
			// there is no use in scissoring

			// reverse the cullface settings for this render
			r_refdef.view.cullface_front = GL_FRONT;
			r_refdef.view.cullface_back = GL_BACK;
			// also reverse the view matrix
			Matrix4x4_ConcatScale3(&r_refdef.view.matrix, 1, 1, -1); // this serves to invert texcoords in the result, as the copied texture is mapped the wrong way round
			R_RenderView_UpdateViewVectors();
			if(p->camera_entity && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.FatPVS)
			{
				r_refdef.view.usecustompvs = true;
				r_refdef.scene.worldmodel->brush.FatPVS(r_refdef.scene.worldmodel, visorigin, 2, r_refdef.viewcache.world_pvsbits, (r_refdef.viewcache.world_numclusters+7)>>3, false);
			}

			// camera needs no clipplane
			r_refdef.view.useclipplane = false;
			// TODO: is the camera origin always valid?  if so we don't need to clear this
			r_refdef.view.usevieworiginculling = false;

			PlaneClassify(&r_refdef.view.clipplane);

			r_fb.water.hideplayer = false;

			R_ResetViewRendering3D(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);
			GL_ScissorTest(false);
			R_ClearScreen(r_refdef.fogenabled);
			GL_ScissorTest(true);
			R_View_Update(NULL);
			R_AnimCache_CacheVisibleEntities();
			R_RenderScene(rt->fbo, rt->depthtexture, rt->colortexture[0], 0, 0, rt->texturewidth, rt->textureheight);

			r_fb.water.hideplayer = false;
			p->rt_camera = rt;
		}

	}
	r_fb.water.renderingscene = false;
	r_refdef.view = originalview;
	R_ResetViewRendering3D(fbo, depthtexture, colortexture, viewx, viewy, viewwidth, viewheight);
	R_View_Update(NULL);
	R_AnimCache_CacheVisibleEntities();
	goto finish;
error:
	r_refdef.view = originalview;
	r_fb.water.renderingscene = false;
	Cvar_SetValueQuick(&r_water, 0);
	Con_Printf("R_Water_ProcessPlanes: Error: texture creation failed!  Turned off r_water.\n");
finish:
	// lowquality hack, restore cvars
	if (qualityreduction > 0)
	{
		if (qualityreduction >= 1)
		{
			Cvar_SetValueQuick(&r_shadows, old_r_shadows);
			Cvar_SetValueQuick(&r_shadow_realtime_world, old_r_worldrtlight);
			Cvar_SetValueQuick(&r_shadow_realtime_dlight, old_r_dlight);
		}
		if (qualityreduction >= 2)
		{
			Cvar_SetValueQuick(&r_dynamic, old_r_dynamic);
			Cvar_SetValueQuick(&r_drawparticles, old_r_particles);
			Cvar_SetValueQuick(&r_drawdecals, old_r_decals);
		}
	}
}

static void R_Bloom_StartFrame(void)
{
	int screentexturewidth, screentextureheight;
	textype_t textype = TEXTYPE_COLORBUFFER;
	double scale;

	// clear the pointers to rendertargets from last frame as they're stale
	r_fb.rt_screen = NULL;
	r_fb.rt_bloom = NULL;

	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
		r_fb.usedepthtextures = r_usedepthtextures.integer != 0;
		if (r_viewfbo.integer == 2) textype = TEXTYPE_COLORBUFFER16F;
		if (r_viewfbo.integer == 3) textype = TEXTYPE_COLORBUFFER32F;
		break;
	case RENDERPATH_GLES2:
		r_fb.usedepthtextures = false;
		break;
	}

	if (r_viewscale_fpsscaling.integer)
	{
		double actualframetime;
		double targetframetime;
		double adjust;
		actualframetime = r_refdef.lastdrawscreentime;
		targetframetime = (1.0 / r_viewscale_fpsscaling_target.value);
		adjust = (targetframetime - actualframetime) * r_viewscale_fpsscaling_multiply.value;
		adjust = bound(-r_viewscale_fpsscaling_stepmax.value, adjust, r_viewscale_fpsscaling_stepmax.value);
		if (r_viewscale_fpsscaling_stepsize.value > 0)
		{
			if (adjust > 0)
				adjust = floor(adjust / r_viewscale_fpsscaling_stepsize.value) * r_viewscale_fpsscaling_stepsize.value;
			else
				adjust = ceil(adjust / r_viewscale_fpsscaling_stepsize.value) * r_viewscale_fpsscaling_stepsize.value;
		}
		viewscalefpsadjusted += adjust;
		viewscalefpsadjusted = bound(r_viewscale_fpsscaling_min.value, viewscalefpsadjusted, 1.0f);
	}
	else
		viewscalefpsadjusted = 1.0f;

	scale = r_viewscale.value * sqrt(viewscalefpsadjusted);
	if (vid.mode.samples)
		scale *= sqrt(vid.mode.samples); // supersampling
	scale = bound(0.03125f, scale, 4.0f);
	screentexturewidth = (int)ceil(r_refdef.view.width * scale);
	screentextureheight = (int)ceil(r_refdef.view.height * scale);
	screentexturewidth = bound(1, screentexturewidth, (int)vid.maxtexturesize_2d);
	screentextureheight = bound(1, screentextureheight, (int)vid.maxtexturesize_2d);

	// set bloomwidth and bloomheight to the bloom resolution that will be
	// used (often less than the screen resolution for faster rendering)
	r_fb.bloomheight = bound(1, r_bloom_resolution.value * 0.75f, screentextureheight);
	r_fb.bloomwidth = r_fb.bloomheight * screentexturewidth / screentextureheight;
	r_fb.bloomwidth = bound(1, r_fb.bloomwidth, screentexturewidth);
	r_fb.bloomwidth = bound(1, r_fb.bloomwidth, (int)vid.maxtexturesize_2d);
	r_fb.bloomheight = bound(1, r_fb.bloomheight, (int)vid.maxtexturesize_2d);

	if ((r_bloom.integer || (!R_Stereo_Active() && (r_motionblur.value > 0 || r_damageblur.value > 0))) && ((r_bloom_resolution.integer < 4 || r_bloom_blur.value < 1 || r_bloom_blur.value >= 512) || r_refdef.view.width > (int)vid.maxtexturesize_2d || r_refdef.view.height > (int)vid.maxtexturesize_2d))
	{
		Cvar_SetValueQuick(&r_bloom, 0);
		Cvar_SetValueQuick(&r_motionblur, 0);
		Cvar_SetValueQuick(&r_damageblur, 0);
	}
	if (!r_bloom.integer)
		r_fb.bloomwidth = r_fb.bloomheight = 0;

	// allocate motionblur ghost texture if needed - this is the only persistent texture and is only useful on the main view
	if (r_refdef.view.ismain && (r_fb.screentexturewidth != screentexturewidth || r_fb.screentextureheight != screentextureheight || r_fb.textype != textype))
	{
		if (r_fb.ghosttexture)
			R_FreeTexture(r_fb.ghosttexture);
		r_fb.ghosttexture = NULL;

		r_fb.screentexturewidth = screentexturewidth;
		r_fb.screentextureheight = screentextureheight;
		r_fb.textype = textype;

		if (r_fb.screentexturewidth && r_fb.screentextureheight)
		{
			if (r_motionblur.value > 0 || r_damageblur.value > 0)
				r_fb.ghosttexture = R_LoadTexture2D(r_main_texturepool, "framebuffermotionblur", r_fb.screentexturewidth, r_fb.screentextureheight, NULL, r_fb.textype, TEXF_RENDERTARGET | TEXF_FORCELINEAR | TEXF_CLAMP, -1, NULL);
			r_fb.ghosttexture_valid = false;
		}
	}

	r_fb.rt_screen = R_RenderTarget_Get(screentexturewidth, screentextureheight, TEXTYPE_DEPTHBUFFER24STENCIL8, true, textype, TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);

	r_refdef.view.clear = true;
}

static void R_Bloom_MakeTexture(void)
{
	int x, range, dir;
	float xoffset, yoffset, r, brighten;
	float colorscale = r_bloom_colorscale.value;
	r_viewport_t bloomviewport;
	r_rendertarget_t *prev, *cur;
	textype_t textype = r_fb.rt_screen->colortextype[0];

	r_refdef.stats[r_stat_bloom]++;

	R_Viewport_InitOrtho(&bloomviewport, &identitymatrix, 0, 0, r_fb.bloomwidth, r_fb.bloomheight, 0, 0, 1, 1, -10, 100, NULL);

	// scale down screen texture to the bloom texture size
	CHECKGLERROR
	prev = r_fb.rt_screen;
	cur = R_RenderTarget_Get(r_fb.bloomwidth, r_fb.bloomheight, TEXTYPE_UNUSED, false, textype, TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
	R_Mesh_SetRenderTargets(cur->fbo, NULL, cur->colortexture[0], NULL, NULL, NULL);
	R_SetViewport(&bloomviewport);
	GL_CullFace(GL_NONE);
	GL_DepthTest(false);
	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_Color(colorscale, colorscale, colorscale, 1);
	R_Mesh_PrepareVertices_Generic_Arrays(4, r_screenvertex3f, NULL, prev->texcoord2f);
	// TODO: do boxfilter scale-down in shader?
	R_SetupShader_Generic(prev->colortexture[0], false, true, true);
	R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
	r_refdef.stats[r_stat_bloom_drawpixels] += r_fb.bloomwidth * r_fb.bloomheight;
	// we now have a properly scaled bloom image

	// multiply bloom image by itself as many times as desired to darken it
	// TODO: if people actually use this it could be done more quickly in the previous shader pass
	for (x = 1;x < min(r_bloom_colorexponent.value, 32);)
	{
		prev = cur;
		cur = R_RenderTarget_Get(r_fb.bloomwidth, r_fb.bloomheight, TEXTYPE_UNUSED, false, textype, TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
		R_Mesh_SetRenderTargets(cur->fbo, NULL, cur->colortexture[0], NULL, NULL, NULL);
		x *= 2;
		r = bound(0, r_bloom_colorexponent.value / x, 1); // always 0.5 to 1
		if(x <= 2)
			GL_Clear(GL_COLOR_BUFFER_BIT, NULL, 1.0f, 0);
		GL_BlendFunc(GL_SRC_COLOR, GL_ZERO); // square it
		GL_Color(1,1,1,1); // no fix factor supported here
		R_Mesh_PrepareVertices_Generic_Arrays(4, r_screenvertex3f, NULL, prev->texcoord2f);
		R_SetupShader_Generic(prev->colortexture[0], false, true, false);
		R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
		r_refdef.stats[r_stat_bloom_drawpixels] += r_fb.bloomwidth * r_fb.bloomheight;
	}
	CHECKGLERROR

	range = r_bloom_blur.integer * r_fb.bloomwidth / 320;
	brighten = r_bloom_brighten.value;
	brighten = sqrt(brighten);
	if(range >= 1)
		brighten *= (3 * range) / (2 * range - 1); // compensate for the "dot particle"

	for (dir = 0;dir < 2;dir++)
	{
		prev = cur;
		cur = R_RenderTarget_Get(r_fb.bloomwidth, r_fb.bloomheight, TEXTYPE_UNUSED, false, textype, TEXTYPE_UNUSED, TEXTYPE_UNUSED, TEXTYPE_UNUSED);
		R_Mesh_SetRenderTargets(cur->fbo, NULL, cur->colortexture[0], NULL, NULL, NULL);
		// blend on at multiple vertical offsets to achieve a vertical blur
		// TODO: do offset blends using GLSL
		// TODO instead of changing the texcoords, change the target positions to prevent artifacts at edges
		CHECKGLERROR
		GL_BlendFunc(GL_ONE, GL_ZERO);
		CHECKGLERROR
		R_SetupShader_Generic(prev->colortexture[0], false, true, false);
		CHECKGLERROR
		for (x = -range;x <= range;x++)
		{
			if (!dir){xoffset = 0;yoffset = x;}
			else {xoffset = x;yoffset = 0;}
			xoffset /= (float)prev->texturewidth;
			yoffset /= (float)prev->textureheight;
			// compute a texcoord array with the specified x and y offset
			r_fb.offsettexcoord2f[0] = xoffset+prev->texcoord2f[0];
			r_fb.offsettexcoord2f[1] = yoffset+prev->texcoord2f[1];
			r_fb.offsettexcoord2f[2] = xoffset+prev->texcoord2f[2];
			r_fb.offsettexcoord2f[3] = yoffset+prev->texcoord2f[3];
			r_fb.offsettexcoord2f[4] = xoffset+prev->texcoord2f[4];
			r_fb.offsettexcoord2f[5] = yoffset+prev->texcoord2f[5];
			r_fb.offsettexcoord2f[6] = xoffset+prev->texcoord2f[6];
			r_fb.offsettexcoord2f[7] = yoffset+prev->texcoord2f[7];
			// this r value looks like a 'dot' particle, fading sharply to
			// black at the edges
			// (probably not realistic but looks good enough)
			//r = ((range*range+1)/((float)(x*x+1)))/(range*2+1);
			//r = brighten/(range*2+1);
			r = brighten / (range * 2 + 1);
			if(range >= 1)
				r *= (1 - x*x/(float)((range+1)*(range+1)));
			if (r <= 0)
				continue;
			CHECKGLERROR
			GL_Color(r, r, r, 1);
			CHECKGLERROR
			R_Mesh_PrepareVertices_Generic_Arrays(4, r_screenvertex3f, NULL, r_fb.offsettexcoord2f);
			CHECKGLERROR
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
			r_refdef.stats[r_stat_bloom_drawpixels] += r_fb.bloomwidth * r_fb.bloomheight;
			CHECKGLERROR
			GL_BlendFunc(GL_ONE, GL_ONE);
			CHECKGLERROR
		}
	}

	// now we have the bloom image, so keep track of it
	r_fb.rt_bloom = cur;
}

static qbool R_BlendView_IsTrivial(int viewwidth, int viewheight, int width, int height)
{
	// Scaling requested?
	if (viewwidth != width || viewheight != height)
		return false;
	// Higher bit depth or explicit FBO requested?
	if (r_viewfbo.integer)
		return false;
	// Non-trivial postprocessing shader permutation?
	if (r_fb.bloomwidth
	|| r_refdef.viewblend[3] > 0
	|| !vid_gammatables_trivial
	|| r_glsl_postprocess.integer
	|| ((!R_Stereo_ColorMasking() && r_glsl_saturation.value != 1)))
		return false;
	// Other reasons for a non-trivial default postprocessing shader?
	// (See R_CompileShader_CheckStaticParms but only those relevant for MODE_POSTPROCESS in shader_glsl.h)
	// Skip: if (r_glsl_saturation_redcompensate.integer) (already covered by saturation above).
	// Skip: if (r_glsl_postprocess.integer) (already covered by r_glsl_postprocess above).
	// Skip: if (r_glsl_postprocess_uservec1_enable.integer) (already covered by r_glsl_postprocessing above).
	if (r_fxaa.integer)
		return false;
	if (r_colorfringe.value)
		return false;
	return true;
}

static void R_MotionBlurView(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	R_EntityMatrix(&identitymatrix);

	if(r_refdef.view.ismain && !R_Stereo_Active() && (r_motionblur.value > 0 || (r_damageblur.value > 0 && cl.cshifts[CSHIFT_DAMAGE].percent != 0)) && r_fb.ghosttexture)
	{
		// declare variables
		float blur_factor, blur_mouseaccel, blur_velocity;
		static float blur_average;
		static vec3_t blur_oldangles; // used to see how quickly the mouse is moving

		// set a goal for the factoring
		blur_velocity = bound(0, (VectorLength(cl.movement_velocity) - r_motionblur_velocityfactor_minspeed.value)
			/ max(1, r_motionblur_velocityfactor_maxspeed.value - r_motionblur_velocityfactor_minspeed.value), 1);
		blur_mouseaccel = bound(0, ((fabs(VectorLength(cl.viewangles) - VectorLength(blur_oldangles)) * 10) - r_motionblur_mousefactor_minspeed.value)
			/ max(1, r_motionblur_mousefactor_maxspeed.value - r_motionblur_mousefactor_minspeed.value), 1);
		blur_factor = ((blur_velocity * r_motionblur_velocityfactor.value)
			+ (blur_mouseaccel * r_motionblur_mousefactor.value));

		// from the goal, pick an averaged value between goal and last value
		cl.motionbluralpha = bound(0, (cl.time - cl.oldtime) / max(0.001, r_motionblur_averaging.value), 1);
		blur_average = blur_average * (1 - cl.motionbluralpha) + blur_factor * cl.motionbluralpha;

		// enforce minimum amount of blur
		blur_factor = blur_average * (1 - r_motionblur_minblur.value) + r_motionblur_minblur.value;

		//Con_Printf("motionblur: direct factor: %f, averaged factor: %f, velocity: %f, mouse accel: %f \n", blur_factor, blur_average, blur_velocity, blur_mouseaccel);

		// calculate values into a standard alpha
		cl.motionbluralpha = 1 - exp(-
				(
					(r_motionblur.value * blur_factor / 80)
					+
					(r_damageblur.value * (cl.cshifts[CSHIFT_DAMAGE].percent / 1600))
				)
				/
				max(0.0001, cl.time - cl.oldtime) // fps independent
				);

		// randomization for the blur value to combat persistent ghosting
		cl.motionbluralpha *= lhrandom(1 - r_motionblur_randomize.value, 1 + r_motionblur_randomize.value);
		cl.motionbluralpha = bound(0, cl.motionbluralpha, r_motionblur_maxblur.value);

		// apply the blur on top of the current view
		R_ResetViewRendering2D(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
		if (cl.motionbluralpha > 0 && !r_refdef.envmap && r_fb.ghosttexture_valid)
		{
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GL_Color(1, 1, 1, cl.motionbluralpha);
			R_CalcTexCoordsForView(0, 0, viewwidth, viewheight, viewwidth, viewheight, r_fb.ghosttexcoord2f);
			R_Mesh_PrepareVertices_Generic_Arrays(4, r_screenvertex3f, NULL, r_fb.ghosttexcoord2f);
			R_SetupShader_Generic(r_fb.ghosttexture, false, true, true);
			R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
			r_refdef.stats[r_stat_bloom_drawpixels] += viewwidth * viewheight;
		}

		// updates old view angles for next pass
		VectorCopy(cl.viewangles, blur_oldangles);

		// copy view into the ghost texture
		R_Mesh_CopyToTexture(r_fb.ghosttexture, 0, 0, viewx, viewy, viewwidth, viewheight);
		r_refdef.stats[r_stat_bloom_copypixels] += viewwidth * viewheight;
		r_fb.ghosttexture_valid = true;
	}
}

static void R_BlendView(rtexture_t *viewcolortexture, int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, int x, int y, int width, int height)
{
	uint64_t permutation;
	float uservecs[4][4];
	rtexture_t *viewtexture;
	rtexture_t *bloomtexture;

	R_EntityMatrix(&identitymatrix);

	if (r_fb.bloomwidth)
	{
		// make the bloom texture
		R_Bloom_MakeTexture();
	}

#if _MSC_VER >= 1400
#define sscanf sscanf_s
#endif
	memset(uservecs, 0, sizeof(uservecs));
	if (r_glsl_postprocess_uservec1_enable.integer)
		sscanf(r_glsl_postprocess_uservec1.string, "%f %f %f %f", &uservecs[0][0], &uservecs[0][1], &uservecs[0][2], &uservecs[0][3]);
	if (r_glsl_postprocess_uservec2_enable.integer)
		sscanf(r_glsl_postprocess_uservec2.string, "%f %f %f %f", &uservecs[1][0], &uservecs[1][1], &uservecs[1][2], &uservecs[1][3]);
	if (r_glsl_postprocess_uservec3_enable.integer)
		sscanf(r_glsl_postprocess_uservec3.string, "%f %f %f %f", &uservecs[2][0], &uservecs[2][1], &uservecs[2][2], &uservecs[2][3]);
	if (r_glsl_postprocess_uservec4_enable.integer)
		sscanf(r_glsl_postprocess_uservec4.string, "%f %f %f %f", &uservecs[3][0], &uservecs[3][1], &uservecs[3][2], &uservecs[3][3]);

	// render to the screen fbo
	R_ResetViewRendering2D(fbo, depthtexture, colortexture, x, y, width, height);
	GL_Color(1, 1, 1, 1);
	GL_BlendFunc(GL_ONE, GL_ZERO);

	viewtexture = viewcolortexture;
	bloomtexture = r_fb.rt_bloom ? r_fb.rt_bloom->colortexture[0] : NULL;

	if (r_rendertarget_debug.integer >= 0)
	{
		r_rendertarget_t *rt = (r_rendertarget_t *)Mem_ExpandableArray_RecordAtIndex(&r_fb.rendertargets, r_rendertarget_debug.integer);
		if (rt && rt->colortexture[0])
		{
			viewtexture = rt->colortexture[0];
			bloomtexture = NULL;
		}
	}

	R_Mesh_PrepareVertices_Mesh_Arrays(4, r_screenvertex3f, NULL, NULL, NULL, NULL, r_fb.rt_screen->texcoord2f, bloomtexture ? r_fb.rt_bloom->texcoord2f : NULL);
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		permutation =
			(r_fb.bloomwidth ? SHADERPERMUTATION_BLOOM : 0)
			| (r_refdef.viewblend[3] > 0 ? SHADERPERMUTATION_VIEWTINT : 0)
			| (!vid_gammatables_trivial ? SHADERPERMUTATION_GAMMARAMPS : 0)
			| (r_glsl_postprocess.integer ? SHADERPERMUTATION_POSTPROCESSING : 0)
			| ((!R_Stereo_ColorMasking() && r_glsl_saturation.value != 1) ? SHADERPERMUTATION_SATURATION : 0);
		R_SetupShader_SetPermutationGLSL(SHADERMODE_POSTPROCESS, permutation);
		if (r_glsl_permutation->tex_Texture_First           >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_First     , viewtexture);
		if (r_glsl_permutation->tex_Texture_Second          >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_Second    , bloomtexture);
		if (r_glsl_permutation->tex_Texture_GammaRamps      >= 0) R_Mesh_TexBind(r_glsl_permutation->tex_Texture_GammaRamps, r_texture_gammaramps       );
		if (r_glsl_permutation->loc_ViewTintColor           >= 0) qglUniform4f(r_glsl_permutation->loc_ViewTintColor     , r_refdef.viewblend[0], r_refdef.viewblend[1], r_refdef.viewblend[2], r_refdef.viewblend[3]);
		if (r_glsl_permutation->loc_PixelSize               >= 0) qglUniform2f(r_glsl_permutation->loc_PixelSize         , 1.0/r_fb.screentexturewidth, 1.0/r_fb.screentextureheight);
		if (r_glsl_permutation->loc_UserVec1                >= 0) qglUniform4f(r_glsl_permutation->loc_UserVec1          , uservecs[0][0], uservecs[0][1], uservecs[0][2], uservecs[0][3]);
		if (r_glsl_permutation->loc_UserVec2                >= 0) qglUniform4f(r_glsl_permutation->loc_UserVec2          , uservecs[1][0], uservecs[1][1], uservecs[1][2], uservecs[1][3]);
		if (r_glsl_permutation->loc_UserVec3                >= 0) qglUniform4f(r_glsl_permutation->loc_UserVec3          , uservecs[2][0], uservecs[2][1], uservecs[2][2], uservecs[2][3]);
		if (r_glsl_permutation->loc_UserVec4                >= 0) qglUniform4f(r_glsl_permutation->loc_UserVec4          , uservecs[3][0], uservecs[3][1], uservecs[3][2], uservecs[3][3]);
		if (r_glsl_permutation->loc_Saturation              >= 0) qglUniform1f(r_glsl_permutation->loc_Saturation        , r_glsl_saturation.value);
		if (r_glsl_permutation->loc_PixelToScreenTexCoord   >= 0) qglUniform2f(r_glsl_permutation->loc_PixelToScreenTexCoord, 1.0f/r_fb.screentexturewidth, 1.0f/r_fb.screentextureheight);
		if (r_glsl_permutation->loc_BloomColorSubtract      >= 0) qglUniform4f(r_glsl_permutation->loc_BloomColorSubtract   , r_bloom_colorsubtract.value, r_bloom_colorsubtract.value, r_bloom_colorsubtract.value, 0.0f);
		if (r_glsl_permutation->loc_ColorFringe             >= 0) qglUniform1f(r_glsl_permutation->loc_ColorFringe, r_colorfringe.value );
		break;
	}
	R_Mesh_Draw(0, 4, 0, 2, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
	r_refdef.stats[r_stat_bloom_drawpixels] += r_refdef.view.width * r_refdef.view.height;
}

matrix4x4_t r_waterscrollmatrix;

void R_UpdateFog(void)
{
	// Nehahra fog
	if (gamemode == GAME_NEHAHRA)
	{
		if (gl_fogenable.integer)
		{
			r_refdef.oldgl_fogenable = true;
			r_refdef.fog_density = gl_fogdensity.value;
			r_refdef.fog_red = gl_fogred.value;
			r_refdef.fog_green = gl_foggreen.value;
			r_refdef.fog_blue = gl_fogblue.value;
			r_refdef.fog_alpha = 1;
			r_refdef.fog_start = 0;
			r_refdef.fog_end = gl_skyclip.value;
			r_refdef.fog_height = 1<<30;
			r_refdef.fog_fadedepth = 128;
		}
		else if (r_refdef.oldgl_fogenable)
		{
			r_refdef.oldgl_fogenable = false;
			r_refdef.fog_density = 0;
			r_refdef.fog_red = 0;
			r_refdef.fog_green = 0;
			r_refdef.fog_blue = 0;
			r_refdef.fog_alpha = 0;
			r_refdef.fog_start = 0;
			r_refdef.fog_end = 0;
			r_refdef.fog_height = 1<<30;
			r_refdef.fog_fadedepth = 128;
		}
	}

	// fog parms
	r_refdef.fog_alpha = bound(0, r_refdef.fog_alpha, 1);
	r_refdef.fog_start = max(0, r_refdef.fog_start);
	r_refdef.fog_end = max(r_refdef.fog_start + 0.01, r_refdef.fog_end);

	if (r_refdef.fog_density && r_drawfog.integer)
	{
		r_refdef.fogenabled = true;
		// this is the point where the fog reaches 0.9986 alpha, which we
		// consider a good enough cutoff point for the texture
		// (0.9986 * 256 == 255.6)
		if (r_fog_exp2.integer)
			r_refdef.fogrange = 32 / (r_refdef.fog_density * r_refdef.fog_density) + r_refdef.fog_start;
		else
			r_refdef.fogrange = 2048 / r_refdef.fog_density + r_refdef.fog_start;
		r_refdef.fogrange = bound(r_refdef.fog_start, r_refdef.fogrange, r_refdef.fog_end);
		r_refdef.fograngerecip = 1.0f / r_refdef.fogrange;
		r_refdef.fogmasktabledistmultiplier = FOGMASKTABLEWIDTH * r_refdef.fograngerecip;
		if (strcmp(r_refdef.fogheighttexturename, r_refdef.fog_height_texturename))
			R_BuildFogHeightTexture();
		// fog color was already set
		// update the fog texture
		if (r_refdef.fogmasktable_start != r_refdef.fog_start || r_refdef.fogmasktable_alpha != r_refdef.fog_alpha || r_refdef.fogmasktable_density != r_refdef.fog_density || r_refdef.fogmasktable_range != r_refdef.fogrange)
			R_BuildFogTexture();
		r_refdef.fog_height_texcoordscale = 1.0f / max(0.125f, r_refdef.fog_fadedepth);
		r_refdef.fog_height_tablescale = r_refdef.fog_height_tablesize * r_refdef.fog_height_texcoordscale;
	}
	else
		r_refdef.fogenabled = false;

	// fog color
	if (r_refdef.fog_density)
	{
		r_refdef.fogcolor[0] = r_refdef.fog_red;
		r_refdef.fogcolor[1] = r_refdef.fog_green;
		r_refdef.fogcolor[2] = r_refdef.fog_blue;

		Vector4Set(r_refdef.fogplane, 0, 0, 1, -r_refdef.fog_height);
		r_refdef.fogplaneviewdist = DotProduct(r_refdef.fogplane, r_refdef.view.origin) + r_refdef.fogplane[3];
		r_refdef.fogplaneviewabove = r_refdef.fogplaneviewdist >= 0;
		r_refdef.fogheightfade = -0.5f/max(0.125f, r_refdef.fog_fadedepth);

		{
			vec3_t fogvec;
			VectorCopy(r_refdef.fogcolor, fogvec);
			//   color.rgb *= ContrastBoost * SceneBrightness;
			VectorScale(fogvec, r_refdef.view.colorscale, fogvec);
			r_refdef.fogcolor[0] = bound(0.0f, fogvec[0], 1.0f);
			r_refdef.fogcolor[1] = bound(0.0f, fogvec[1], 1.0f);
			r_refdef.fogcolor[2] = bound(0.0f, fogvec[2], 1.0f);
		}
	}
}

void R_UpdateVariables(void)
{
	R_Textures_Frame();

	r_refdef.scene.ambientintensity = r_ambient.value * (1.0f / 64.0f);

	r_refdef.farclip = r_farclip_base.value;
	if (r_refdef.scene.worldmodel)
		r_refdef.farclip += r_refdef.scene.worldmodel->radius * r_farclip_world.value * 2;
	r_refdef.nearclip = bound (0.001f, r_nearclip.value, r_refdef.farclip - 1.0f);

	if (r_shadow_frontsidecasting.integer < 0 || r_shadow_frontsidecasting.integer > 1)
		Cvar_SetValueQuick(&r_shadow_frontsidecasting, 1);
	r_refdef.polygonfactor = 0;
	r_refdef.polygonoffset = 0;

	r_refdef.scene.rtworld = r_shadow_realtime_world.integer != 0;
	r_refdef.scene.rtworldshadows = r_shadow_realtime_world_shadows.integer && vid.stencil;
	r_refdef.scene.rtdlight = r_shadow_realtime_dlight.integer != 0 && !gl_flashblend.integer && r_dynamic.integer;
	r_refdef.scene.rtdlightshadows = r_refdef.scene.rtdlight && r_shadow_realtime_dlight_shadows.integer && vid.stencil;
	r_refdef.scene.lightmapintensity = r_refdef.scene.rtworld ? r_shadow_realtime_world_lightmaps.value : 1;
	if (r_refdef.scene.worldmodel)
	{
		r_refdef.scene.lightmapintensity *= r_refdef.scene.worldmodel->lightmapscale;

		// Apply the default lightstyle to the lightmap even on q3bsp
		if (cl.worldmodel && cl.worldmodel->type == mod_brushq3) {
			r_refdef.scene.lightmapintensity *= r_refdef.scene.rtlightstylevalue[0];
		}
	}
	if (r_showsurfaces.integer)
	{
		r_refdef.scene.rtworld = false;
		r_refdef.scene.rtworldshadows = false;
		r_refdef.scene.rtdlight = false;
		r_refdef.scene.rtdlightshadows = false;
		r_refdef.scene.lightmapintensity = 0;
	}

	r_gpuskeletal = false;
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
		r_gpuskeletal = r_glsl_skeletal.integer && !r_showsurfaces.integer;
	case RENDERPATH_GLES2:
		if(!vid_gammatables_trivial)
		{
			if(!r_texture_gammaramps || vid_gammatables_serial != r_texture_gammaramps_serial)
			{
				// build GLSL gamma texture
#define RAMPWIDTH 256
				unsigned short ramp[RAMPWIDTH * 3];
				unsigned char rampbgr[RAMPWIDTH][4];
				int i;

				r_texture_gammaramps_serial = vid_gammatables_serial;

				VID_BuildGammaTables(&ramp[0], RAMPWIDTH);
				for(i = 0; i < RAMPWIDTH; ++i)
				{
					rampbgr[i][0] = (unsigned char) (ramp[i + 2 * RAMPWIDTH] * 255.0 / 65535.0 + 0.5);
					rampbgr[i][1] = (unsigned char) (ramp[i + RAMPWIDTH] * 255.0 / 65535.0 + 0.5);
					rampbgr[i][2] = (unsigned char) (ramp[i] * 255.0 / 65535.0 + 0.5);
					rampbgr[i][3] = 0;
				}
				if (r_texture_gammaramps)
				{
					R_UpdateTexture(r_texture_gammaramps, &rampbgr[0][0], 0, 0, 0, RAMPWIDTH, 1, 1, 0);
				}
				else
				{
					r_texture_gammaramps = R_LoadTexture2D(r_main_texturepool, "gammaramps", RAMPWIDTH, 1, &rampbgr[0][0], TEXTYPE_BGRA, TEXF_FORCELINEAR | TEXF_CLAMP | TEXF_PERSISTENT, -1, NULL);
				}
			}
		}
		else
		{
			// remove GLSL gamma texture
		}
		break;
	}
}

static r_refdef_scene_type_t r_currentscenetype = RST_CLIENT;
static r_refdef_scene_t r_scenes_store[ RST_COUNT ];
/*
================
R_SelectScene
================
*/
void R_SelectScene( r_refdef_scene_type_t scenetype ) {
	if( scenetype != r_currentscenetype ) {
		// store the old scenetype
		r_scenes_store[ r_currentscenetype ] = r_refdef.scene;
		r_currentscenetype = scenetype;
		// move in the new scene
		r_refdef.scene = r_scenes_store[ r_currentscenetype ];
	}
}

/*
================
R_GetScenePointer
================
*/
r_refdef_scene_t * R_GetScenePointer( r_refdef_scene_type_t scenetype )
{
	// of course, we could also add a qbool that provides a lock state and a ReleaseScenePointer function..
	if( scenetype == r_currentscenetype ) {
		return &r_refdef.scene;
	} else {
		return &r_scenes_store[ scenetype ];
	}
}

static int R_SortEntities_Compare(const void *ap, const void *bp)
{
	const entity_render_t *a = *(const entity_render_t **)ap;
	const entity_render_t *b = *(const entity_render_t **)bp;

	// 1. compare model
	if(a->model < b->model)
		return -1;
	if(a->model > b->model)
		return +1;

	// 2. compare skin
	// TODO possibly calculate the REAL skinnum here first using
	// skinscenes?
	if(a->skinnum < b->skinnum)
		return -1;
	if(a->skinnum > b->skinnum)
		return +1;

	// everything we compared is equal
	return 0;
}
static void R_SortEntities(void)
{
	// below or equal 2 ents, sorting never gains anything
	if(r_refdef.scene.numentities <= 2)
		return;
	// sort
	qsort(r_refdef.scene.entities, r_refdef.scene.numentities, sizeof(*r_refdef.scene.entities), R_SortEntities_Compare);
}

/*
================
R_RenderView
================
*/
extern cvar_t r_shadow_bouncegrid;
extern cvar_t v_isometric;
extern void V_MakeViewIsometric(void);
void R_RenderView(int fbo, rtexture_t *depthtexture, rtexture_t *colortexture, int x, int y, int width, int height)
{
	matrix4x4_t originalmatrix = r_refdef.view.matrix, offsetmatrix;
	int viewfbo = 0;
	rtexture_t *viewdepthtexture = NULL;
	rtexture_t *viewcolortexture = NULL;
	int viewx = r_refdef.view.x, viewy = r_refdef.view.y, viewwidth = r_refdef.view.width, viewheight = r_refdef.view.height;
	qbool skipblend;

	// finish any 2D rendering that was queued
	DrawQ_Finish();

	if (r_timereport_active)
		R_TimeReport("start");
	r_textureframe++; // used only by R_GetCurrentTexture
	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity

	if(R_CompileShader_CheckStaticParms())
		R_GLSL_Restart_f(cmd_local);

	if (!r_drawentities.integer)
		r_refdef.scene.numentities = 0;
	else if (r_sortentities.integer)
		R_SortEntities();

	R_AnimCache_ClearCache();

	/* adjust for stereo display */
	if(R_Stereo_Active())
	{
		Matrix4x4_CreateFromQuakeEntity(&offsetmatrix, 0, r_stereo_separation.value * (0.5f - r_stereo_side), 0, 0, r_stereo_angle.value * (0.5f - r_stereo_side), 0, 1);
		Matrix4x4_Concat(&r_refdef.view.matrix, &originalmatrix, &offsetmatrix);
	}

	if (r_refdef.view.isoverlay)
	{
		// TODO: FIXME: move this into its own backend function maybe? [2/5/2008 Andreas]
		R_Mesh_SetRenderTargets(0, NULL, NULL, NULL, NULL, NULL);
		GL_Clear(GL_DEPTH_BUFFER_BIT, NULL, 1.0f, 0);
		R_TimeReport("depthclear");

		r_refdef.view.showdebug = false;

		r_fb.water.enabled = false;
		r_fb.water.numwaterplanes = 0;

		R_RenderScene(0, NULL, NULL, r_refdef.view.x, r_refdef.view.y, r_refdef.view.width, r_refdef.view.height);

		r_refdef.view.matrix = originalmatrix;

		CHECKGLERROR
		return;
	}

	if (!r_refdef.scene.entities || r_refdef.view.width * r_refdef.view.height == 0 || !r_renderview.integer || cl_videoplaying/* || !r_refdef.scene.worldmodel*/)
	{
		r_refdef.view.matrix = originalmatrix;
		return;
	}

	r_refdef.view.usevieworiginculling = !r_trippy.value && r_refdef.view.useperspective;
	if (v_isometric.integer && r_refdef.view.ismain)
		V_MakeViewIsometric();

	r_refdef.view.colorscale = r_hdr_scenebrightness.value * r_hdr_irisadaptation_value.value;

	if(vid_sRGB.integer && vid_sRGB_fallback.integer && !vid.sRGB3D)
		// in sRGB fallback, behave similar to true sRGB: convert this
		// value from linear to sRGB
		r_refdef.view.colorscale = Image_sRGBFloatFromLinearFloat(r_refdef.view.colorscale);

	R_RenderView_UpdateViewVectors();

	R_Shadow_UpdateWorldLightSelection();

	// this will set up r_fb.rt_screen
	R_Bloom_StartFrame();

	// apply bloom brightness offset
	if(r_fb.rt_bloom)
		r_refdef.view.colorscale *= r_bloom_scenebrightness.value;

	skipblend = R_BlendView_IsTrivial(r_fb.rt_screen->texturewidth, r_fb.rt_screen->textureheight, width, height);
	if (skipblend)
	{
		// Render to the screen right away.
		viewfbo = fbo;
		viewdepthtexture = depthtexture;
		viewcolortexture = colortexture;
		viewx = x;
		viewy = y;
		viewwidth = width;
		viewheight = height;
	}
	else if (r_fb.rt_screen)
	{
		// R_Bloom_StartFrame probably set up an fbo for us to render into, it will be rendered to the window later in R_BlendView
		viewfbo = r_fb.rt_screen->fbo;
		viewdepthtexture = r_fb.rt_screen->depthtexture;
		viewcolortexture = r_fb.rt_screen->colortexture[0];
		viewx = 0;
		viewy = 0;
		viewwidth = r_fb.rt_screen->texturewidth;
		viewheight = r_fb.rt_screen->textureheight;
	}

	R_Water_StartFrame(viewwidth, viewheight);

	CHECKGLERROR
	if (r_timereport_active)
		R_TimeReport("viewsetup");

	R_ResetViewRendering3D(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);

	// clear the whole fbo every frame - otherwise the driver will consider
	// it to be an inter-frame texture and stall in multi-gpu configurations
	if (r_fb.rt_screen)
		GL_ScissorTest(false);
	R_ClearScreen(r_refdef.fogenabled);
	if (r_timereport_active)
		R_TimeReport("viewclear");

	r_refdef.view.clear = true;

	r_refdef.view.showdebug = true;

	R_View_Update(NULL);
	if (r_timereport_active)
		R_TimeReport("visibility");

	R_AnimCache_CacheVisibleEntities();
	if (r_timereport_active)
		R_TimeReport("animcache");

	R_Shadow_UpdateBounceGridTexture();
	// R_Shadow_UpdateBounceGridTexture called R_TimeReport a few times internally, so we don't need to do that here.

	r_fb.water.numwaterplanes = 0;
	if (r_fb.water.enabled)
		R_RenderWaterPlanes(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);

	// for the actual view render we use scissoring a fair amount, so scissor
	// test needs to be on
	if (r_fb.rt_screen)
		GL_ScissorTest(true);
	GL_Scissor(r_refdef.view.viewport.x, r_refdef.view.viewport.y, r_refdef.view.viewport.width, r_refdef.view.viewport.height);
	R_RenderScene(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
	r_fb.water.numwaterplanes = 0;

	// postprocess uses textures that are not aligned with the viewport we're rendering, so no scissoring
	GL_ScissorTest(false);

	R_MotionBlurView(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
	if (!skipblend)
		R_BlendView(viewcolortexture, fbo, depthtexture, colortexture, x, y, width, height);
	if (r_timereport_active)
		R_TimeReport("blendview");

	r_refdef.view.matrix = originalmatrix;

	CHECKGLERROR

	// go back to 2d rendering
	DrawQ_Start();
}

void R_RenderWaterPlanes(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	if (cl.csqc_vidvars.drawworld && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawAddWaterPlanes)
	{
		r_refdef.scene.worldmodel->DrawAddWaterPlanes(r_refdef.scene.worldentity);
		if (r_timereport_active)
			R_TimeReport("waterworld");
	}

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	R_DrawModelsAddWaterPlanes();
	if (r_timereport_active)
		R_TimeReport("watermodels");

	if (r_fb.water.numwaterplanes)
	{
		R_Water_ProcessPlanes(viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
		if (r_timereport_active)
			R_TimeReport("waterscenes");
	}
}

extern cvar_t cl_locs_show;
static void R_DrawLocs(void);
static void R_DrawEntityBBoxes(prvm_prog_t *prog);
static void R_DrawModelDecals(void);
extern qbool r_shadow_usingdeferredprepass;
extern int r_shadow_shadowmapatlas_modelshadows_size;
void R_RenderScene(int viewfbo, rtexture_t *viewdepthtexture, rtexture_t *viewcolortexture, int viewx, int viewy, int viewwidth, int viewheight)
{
	qbool shadowmapping = false;

	if (r_timereport_active)
		R_TimeReport("beginscene");

	r_refdef.stats[r_stat_renders]++;

	R_UpdateFog();

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	R_MeshQueue_BeginScene();

	R_SkyStartFrame();

	Matrix4x4_CreateTranslate(&r_waterscrollmatrix, sin(r_refdef.scene.time) * 0.025 * r_waterscroll.value, sin(r_refdef.scene.time * 0.8f) * 0.025 * r_waterscroll.value, 0);

	if (r_timereport_active)
		R_TimeReport("skystartframe");

	if (cl.csqc_vidvars.drawworld)
	{
		// don't let sound skip if going slow
		if (r_refdef.scene.extraupdate)
			S_ExtraUpdate ();

		if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawSky)
		{
			r_refdef.scene.worldmodel->DrawSky(r_refdef.scene.worldentity);
			if (r_timereport_active)
				R_TimeReport("worldsky");
		}

		if (R_DrawBrushModelsSky() && r_timereport_active)
			R_TimeReport("bmodelsky");

		if (skyrendermasked && skyrenderlater)
		{
			// we have to force off the water clipping plane while rendering sky
			R_SetupView(false, viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
			R_Sky();
			R_SetupView(true, viewfbo, viewdepthtexture, viewcolortexture, viewx, viewy, viewwidth, viewheight);
			if (r_timereport_active)
				R_TimeReport("sky");
		}
	}

	// save the framebuffer info for R_Shadow_RenderMode_Reset during this view render
	r_shadow_viewfbo = viewfbo;
	r_shadow_viewdepthtexture = viewdepthtexture;
	r_shadow_viewcolortexture = viewcolortexture;
	r_shadow_viewx = viewx;
	r_shadow_viewy = viewy;
	r_shadow_viewwidth = viewwidth;
	r_shadow_viewheight = viewheight;

	R_Shadow_PrepareModelShadows();
	R_Shadow_PrepareLights();
	if (r_timereport_active)
		R_TimeReport("preparelights");

	// render all the shadowmaps that will be used for this view
	shadowmapping = R_Shadow_ShadowMappingEnabled();
	if (shadowmapping || r_shadow_shadowmapatlas_modelshadows_size)
	{
		R_Shadow_DrawShadowMaps();
		if (r_timereport_active)
			R_TimeReport("shadowmaps");
	}

	// render prepass deferred lighting if r_shadow_deferred is on, this produces light buffers that will be sampled in forward pass
	if (r_shadow_usingdeferredprepass)
		R_Shadow_DrawPrepass();

	// now we begin the forward pass of the view render
	if (r_depthfirst.integer >= 1 && cl.csqc_vidvars.drawworld && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawDepth)
	{
		r_refdef.scene.worldmodel->DrawDepth(r_refdef.scene.worldentity);
		if (r_timereport_active)
			R_TimeReport("worlddepth");
	}
	if (r_depthfirst.integer >= 2)
	{
		R_DrawModelsDepth();
		if (r_timereport_active)
			R_TimeReport("modeldepth");
	}

	if (cl.csqc_vidvars.drawworld && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->Draw)
	{
		r_refdef.scene.worldmodel->Draw(r_refdef.scene.worldentity);
		if (r_timereport_active)
			R_TimeReport("world");
	}

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	R_DrawModels();
	if (r_timereport_active)
		R_TimeReport("models");

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	if (!r_shadow_usingdeferredprepass)
	{
		R_Shadow_DrawLights();
		if (r_timereport_active)
			R_TimeReport("rtlights");
	}

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();

	if (cl.csqc_vidvars.drawworld)
	{
		R_DrawModelDecals();
		if (r_timereport_active)
			R_TimeReport("modeldecals");

		R_DrawParticles();
		if (r_timereport_active)
			R_TimeReport("particles");

		R_DrawExplosions();
		if (r_timereport_active)
			R_TimeReport("explosions");
	}

	if (r_refdef.view.showdebug)
	{
		if (cl_locs_show.integer)
		{
			R_DrawLocs();
			if (r_timereport_active)
				R_TimeReport("showlocs");
		}

		if (r_drawportals.integer)
		{
			R_DrawPortals();
			if (r_timereport_active)
				R_TimeReport("portals");
		}

		if (r_showbboxes_client.value > 0)
		{
			R_DrawEntityBBoxes(CLVM_prog);
			if (r_timereport_active)
				R_TimeReport("clbboxes");
		}
		if (r_showbboxes.value > 0)
		{
			R_DrawEntityBBoxes(SVVM_prog);
			if (r_timereport_active)
				R_TimeReport("svbboxes");
		}
	}

	if (r_transparent.integer)
	{
		R_MeshQueue_RenderTransparent();
		if (r_timereport_active)
			R_TimeReport("drawtrans");
	}

	if (r_refdef.view.showdebug && r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->DrawDebug && (r_showtris.value > 0 || r_shownormals.value != 0 || r_showcollisionbrushes.value > 0 || r_showoverdraw.value > 0))
	{
		r_refdef.scene.worldmodel->DrawDebug(r_refdef.scene.worldentity);
		if (r_timereport_active)
			R_TimeReport("worlddebug");
		R_DrawModelsDebug();
		if (r_timereport_active)
			R_TimeReport("modeldebug");
	}

	if (cl.csqc_vidvars.drawworld)
	{
		R_Shadow_DrawCoronas();
		if (r_timereport_active)
			R_TimeReport("coronas");
	}

	// don't let sound skip if going slow
	if (r_refdef.scene.extraupdate)
		S_ExtraUpdate ();
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

#define BBOXEDGES 13
static const float bboxedges[BBOXEDGES][6] =
{
	// whole box
	{ 0, 0, 0, 1, 1, 1 },
	// bottom edges
	{ 0, 0, 0, 0, 1, 0 },
	{ 0, 0, 0, 1, 0, 0 },
	{ 0, 1, 0, 1, 1, 0 },
	{ 1, 0, 0, 1, 1, 0 },
	// top edges
	{ 0, 0, 1, 0, 1, 1 },
	{ 0, 0, 1, 1, 0, 1 },
	{ 0, 1, 1, 1, 1, 1 },
	{ 1, 0, 1, 1, 1, 1 },
	// vertical edges
	{ 0, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 1, 0, 1 },
	{ 0, 1, 0, 0, 1, 1 },
	{ 1, 1, 0, 1, 1, 1 },
};

static void R_DrawBBoxMesh(vec3_t mins, vec3_t maxs, float cr, float cg, float cb, float ca)
{
	int numvertices = BBOXEDGES * 8;
	float vertex3f[BBOXEDGES * 8 * 3], color4f[BBOXEDGES * 8 * 4];
	int numtriangles = BBOXEDGES * 12;
	unsigned short elements[BBOXEDGES * 36];
	int i, edge;
	float *v, *c, f1, f2, edgemins[3], edgemaxs[3];

	RSurf_ActiveModelEntity(r_refdef.scene.worldentity, false, false, false);

	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthMask(false);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);

	for (edge = 0; edge < BBOXEDGES; edge++)
	{
		for (i = 0; i < 3; i++)
		{
			edgemins[i] = mins[i] + (maxs[i] - mins[i]) * bboxedges[edge][i] - 0.25f;
			edgemaxs[i] = mins[i] + (maxs[i] - mins[i]) * bboxedges[edge][3 + i] + 0.25f;
		}
		vertex3f[edge * 24 + 0] = edgemins[0]; vertex3f[edge * 24 + 1] = edgemins[1]; vertex3f[edge * 24 + 2] = edgemins[2];
		vertex3f[edge * 24 + 3] = edgemaxs[0]; vertex3f[edge * 24 + 4] = edgemins[1]; vertex3f[edge * 24 + 5] = edgemins[2];
		vertex3f[edge * 24 + 6] = edgemins[0]; vertex3f[edge * 24 + 7] = edgemaxs[1]; vertex3f[edge * 24 + 8] = edgemins[2];
		vertex3f[edge * 24 + 9] = edgemaxs[0]; vertex3f[edge * 24 + 10] = edgemaxs[1]; vertex3f[edge * 24 + 11] = edgemins[2];
		vertex3f[edge * 24 + 12] = edgemins[0]; vertex3f[edge * 24 + 13] = edgemins[1]; vertex3f[edge * 24 + 14] = edgemaxs[2];
		vertex3f[edge * 24 + 15] = edgemaxs[0]; vertex3f[edge * 24 + 16] = edgemins[1]; vertex3f[edge * 24 + 17] = edgemaxs[2];
		vertex3f[edge * 24 + 18] = edgemins[0]; vertex3f[edge * 24 + 19] = edgemaxs[1]; vertex3f[edge * 24 + 20] = edgemaxs[2];
		vertex3f[edge * 24 + 21] = edgemaxs[0]; vertex3f[edge * 24 + 22] = edgemaxs[1]; vertex3f[edge * 24 + 23] = edgemaxs[2];
		for (i = 0; i < 36; i++)
			elements[edge * 36 + i] = edge * 8 + bboxelements[i];
	}
	R_FillColors(color4f, numvertices, cr, cg, cb, ca);
	if (r_refdef.fogenabled)
	{
		for (i = 0, v = vertex3f, c = color4f; i < numvertices; i++, v += 3, c += 4)
		{
			f1 = RSurf_FogVertex(v);
			f2 = 1 - f1;
			c[0] = c[0] * f1 + r_refdef.fogcolor[0] * f2;
			c[1] = c[1] * f1 + r_refdef.fogcolor[1] * f2;
			c[2] = c[2] * f1 + r_refdef.fogcolor[2] * f2;
		}
	}
	R_Mesh_PrepareVertices_Generic_Arrays(numvertices, vertex3f, color4f, NULL);
	R_Mesh_ResetTextureState();
	R_SetupShader_Generic_NoTexture(false, false);
	R_Mesh_Draw(0, numvertices, 0, numtriangles, NULL, NULL, 0, elements, NULL, 0);
}

static void R_DrawEntityBBoxes_Callback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	// hacky overloading of the parameters
	prvm_prog_t *prog = (prvm_prog_t *)rtlight;
	int i;
	float color[4];
	prvm_edict_t *edict;

	GL_CullFace(GL_NONE);
	R_SetupShader_Generic_NoTexture(false, false);

	for (i = 0;i < numsurfaces;i++)
	{
		edict = PRVM_EDICT_NUM(surfacelist[i]);
		switch ((int)PRVM_serveredictfloat(edict, solid))
		{
			case SOLID_NOT:      Vector4Set(color, 1, 1, 1, 0.05);break;
			case SOLID_TRIGGER:  Vector4Set(color, 1, 0, 1, 0.10);break;
			case SOLID_BBOX:     Vector4Set(color, 0, 1, 0, 0.10);break;
			case SOLID_SLIDEBOX: Vector4Set(color, 1, 0, 0, 0.10);break;
			case SOLID_BSP:      Vector4Set(color, 0, 0, 1, 0.05);break;
			case SOLID_CORPSE:   Vector4Set(color, 1, 0.5, 0, 0.05);break;
			default:             Vector4Set(color, 0, 0, 0, 0.50);break;
		}
		if (prog == CLVM_prog)
			color[3] *= r_showbboxes_client.value;
		else
			color[3] *= r_showbboxes.value;
		color[3] = bound(0, color[3], 1);
		GL_DepthTest(!r_showdisabledepthtest.integer);
		R_DrawBBoxMesh(edict->priv.server->areamins, edict->priv.server->areamaxs, color[0], color[1], color[2], color[3]);
	}
}

static void R_DrawEntityBBoxes(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t *edict;
	vec3_t center;

	if (prog == NULL)
		return;

	for (i = 0; i < prog->num_edicts; i++)
	{
		edict = PRVM_EDICT_NUM(i);
		if (edict->free)
			continue;
		// exclude the following for now, as they don't live in world coordinate space and can't be solid:
		if (PRVM_gameedictedict(edict, tag_entity) != 0)
			continue;
		if (prog == SVVM_prog && PRVM_serveredictedict(edict, viewmodelforclient) != 0)
			continue;
		VectorLerp(edict->priv.server->areamins, 0.5f, edict->priv.server->areamaxs, center);
		R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, center, R_DrawEntityBBoxes_Callback, (entity_render_t *)NULL, i, (rtlight_t *)prog);
	}
}

static const int nomodelelement3i[24] =
{
	5, 2, 0,
	5, 1, 2,
	5, 0, 3,
	5, 3, 1,
	0, 2, 4,
	2, 1, 4,
	3, 0, 4,
	1, 3, 4
};

static const unsigned short nomodelelement3s[24] =
{
	5, 2, 0,
	5, 1, 2,
	5, 0, 3,
	5, 3, 1,
	0, 2, 4,
	2, 1, 4,
	3, 0, 4,
	1, 3, 4
};

static const float nomodelvertex3f[6*3] =
{
	-16,   0,   0,
	 16,   0,   0,
	  0, -16,   0,
	  0,  16,   0,
	  0,   0, -16,
	  0,   0,  16
};

static const float nomodelcolor4f[6*4] =
{
	0.0f, 0.0f, 0.5f, 1.0f,
	0.0f, 0.0f, 0.5f, 1.0f,
	0.0f, 0.5f, 0.0f, 1.0f,
	0.0f, 0.5f, 0.0f, 1.0f,
	0.5f, 0.0f, 0.0f, 1.0f,
	0.5f, 0.0f, 0.0f, 1.0f
};

static void R_DrawNoModel_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int i;
	float f1, f2, *c;
	float color4f[6*4];

	RSurf_ActiveCustomEntity(&ent->matrix, &ent->inversematrix, ent->flags, ent->shadertime, ent->colormod[0], ent->colormod[1], ent->colormod[2], ent->alpha, 6, nomodelvertex3f, NULL, NULL, NULL, NULL, nomodelcolor4f, 8, nomodelelement3i, nomodelelement3s, false, false);

	// this is only called once per entity so numsurfaces is always 1, and
	// surfacelist is always {0}, so this code does not handle batches

	if (rsurface.ent_flags & RENDER_ADDITIVE)
	{
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE);
		GL_DepthMask(false);
	}
	else if (ent->alpha < 1)
	{
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_DepthMask(false);
	}
	else
	{
		GL_BlendFunc(GL_ONE, GL_ZERO);
		GL_DepthMask(true);
	}
	GL_DepthRange(0, (rsurface.ent_flags & RENDER_VIEWMODEL) ? 0.0625 : 1);
	GL_PolygonOffset(rsurface.basepolygonfactor, rsurface.basepolygonoffset);
	GL_DepthTest(!(rsurface.ent_flags & RENDER_NODEPTHTEST));
	GL_CullFace((rsurface.ent_flags & RENDER_DOUBLESIDED) ? GL_NONE : r_refdef.view.cullface_back);
	memcpy(color4f, nomodelcolor4f, sizeof(float[6*4]));
	for (i = 0, c = color4f;i < 6;i++, c += 4)
	{
		c[0] *= ent->render_fullbright[0] * r_refdef.view.colorscale;
		c[1] *= ent->render_fullbright[1] * r_refdef.view.colorscale;
		c[2] *= ent->render_fullbright[2] * r_refdef.view.colorscale;
		c[3] *= ent->alpha;
	}
	if (r_refdef.fogenabled)
	{
		for (i = 0, c = color4f;i < 6;i++, c += 4)
		{
			f1 = RSurf_FogVertex(nomodelvertex3f + 3*i);
			f2 = 1 - f1;
			c[0] = (c[0] * f1 + r_refdef.fogcolor[0] * f2);
			c[1] = (c[1] * f1 + r_refdef.fogcolor[1] * f2);
			c[2] = (c[2] * f1 + r_refdef.fogcolor[2] * f2);
		}
	}
//	R_Mesh_ResetTextureState();
	R_SetupShader_Generic_NoTexture(false, false);
	R_Mesh_PrepareVertices_Generic_Arrays(6, nomodelvertex3f, color4f, NULL);
	R_Mesh_Draw(0, 6, 0, 8, nomodelelement3i, NULL, 0, nomodelelement3s, NULL, 0);
}

void R_DrawNoModel(entity_render_t *ent)
{
	vec3_t org;
	Matrix4x4_OriginFromMatrix(&ent->matrix, org);
	if ((ent->flags & RENDER_ADDITIVE) || (ent->alpha < 1))
		R_MeshQueue_AddTransparent((ent->flags & RENDER_NODEPTHTEST) ? TRANSPARENTSORT_HUD : TRANSPARENTSORT_DISTANCE, org, R_DrawNoModel_TransparentCallback, ent, 0, rsurface.rtlight);
	else
		R_DrawNoModel_TransparentCallback(ent, rsurface.rtlight, 0, NULL);
}

void R_CalcBeam_Vertex3f (float *vert, const float *org1, const float *org2, float width)
{
	vec3_t right1, right2, diff, normal;

	VectorSubtract (org2, org1, normal);

	// calculate 'right' vector for start
	VectorSubtract (r_refdef.view.origin, org1, diff);
	CrossProduct (normal, diff, right1);
	VectorNormalize (right1);

	// calculate 'right' vector for end
	VectorSubtract (r_refdef.view.origin, org2, diff);
	CrossProduct (normal, diff, right2);
	VectorNormalize (right2);

	vert[ 0] = org1[0] + width * right1[0];
	vert[ 1] = org1[1] + width * right1[1];
	vert[ 2] = org1[2] + width * right1[2];
	vert[ 3] = org1[0] - width * right1[0];
	vert[ 4] = org1[1] - width * right1[1];
	vert[ 5] = org1[2] - width * right1[2];
	vert[ 6] = org2[0] - width * right2[0];
	vert[ 7] = org2[1] - width * right2[1];
	vert[ 8] = org2[2] - width * right2[2];
	vert[ 9] = org2[0] + width * right2[0];
	vert[10] = org2[1] + width * right2[1];
	vert[11] = org2[2] + width * right2[2];
}

void R_CalcSprite_Vertex3f(float *vertex3f, const vec3_t origin, const vec3_t left, const vec3_t up, float scalex1, float scalex2, float scaley1, float scaley2)
{
	vertex3f[ 0] = origin[0] + left[0] * scalex2 + up[0] * scaley1;
	vertex3f[ 1] = origin[1] + left[1] * scalex2 + up[1] * scaley1;
	vertex3f[ 2] = origin[2] + left[2] * scalex2 + up[2] * scaley1;
	vertex3f[ 3] = origin[0] + left[0] * scalex2 + up[0] * scaley2;
	vertex3f[ 4] = origin[1] + left[1] * scalex2 + up[1] * scaley2;
	vertex3f[ 5] = origin[2] + left[2] * scalex2 + up[2] * scaley2;
	vertex3f[ 6] = origin[0] + left[0] * scalex1 + up[0] * scaley2;
	vertex3f[ 7] = origin[1] + left[1] * scalex1 + up[1] * scaley2;
	vertex3f[ 8] = origin[2] + left[2] * scalex1 + up[2] * scaley2;
	vertex3f[ 9] = origin[0] + left[0] * scalex1 + up[0] * scaley1;
	vertex3f[10] = origin[1] + left[1] * scalex1 + up[1] * scaley1;
	vertex3f[11] = origin[2] + left[2] * scalex1 + up[2] * scaley1;
}

static int R_Mesh_AddVertex(rmesh_t *mesh, float x, float y, float z)
{
	int i;
	float *vertex3f;
	float v[3];
	VectorSet(v, x, y, z);
	for (i = 0, vertex3f = mesh->vertex3f;i < mesh->numvertices;i++, vertex3f += 3)
		if (VectorDistance2(v, vertex3f) < mesh->epsilon2)
			break;
	if (i == mesh->numvertices)
	{
		if (mesh->numvertices < mesh->maxvertices)
		{
			VectorCopy(v, vertex3f);
			mesh->numvertices++;
		}
		return mesh->numvertices;
	}
	else
		return i;
}

void R_Mesh_AddPolygon3f(rmesh_t *mesh, int numvertices, float *vertex3f)
{
	int i;
	int *e, element[3];
	element[0] = R_Mesh_AddVertex(mesh, vertex3f[0], vertex3f[1], vertex3f[2]);vertex3f += 3;
	element[1] = R_Mesh_AddVertex(mesh, vertex3f[0], vertex3f[1], vertex3f[2]);vertex3f += 3;
	e = mesh->element3i + mesh->numtriangles * 3;
	for (i = 0;i < numvertices - 2;i++, vertex3f += 3)
	{
		element[2] = R_Mesh_AddVertex(mesh, vertex3f[0], vertex3f[1], vertex3f[2]);
		if (mesh->numtriangles < mesh->maxtriangles)
		{
			*e++ = element[0];
			*e++ = element[1];
			*e++ = element[2];
			mesh->numtriangles++;
		}
		element[1] = element[2];
	}
}

static void R_Mesh_AddPolygon3d(rmesh_t *mesh, int numvertices, double *vertex3d)
{
	int i;
	int *e, element[3];
	element[0] = R_Mesh_AddVertex(mesh, vertex3d[0], vertex3d[1], vertex3d[2]);vertex3d += 3;
	element[1] = R_Mesh_AddVertex(mesh, vertex3d[0], vertex3d[1], vertex3d[2]);vertex3d += 3;
	e = mesh->element3i + mesh->numtriangles * 3;
	for (i = 0;i < numvertices - 2;i++, vertex3d += 3)
	{
		element[2] = R_Mesh_AddVertex(mesh, vertex3d[0], vertex3d[1], vertex3d[2]);
		if (mesh->numtriangles < mesh->maxtriangles)
		{
			*e++ = element[0];
			*e++ = element[1];
			*e++ = element[2];
			mesh->numtriangles++;
		}
		element[1] = element[2];
	}
}

#define R_MESH_PLANE_DIST_EPSILON (1.0 / 32.0)
void R_Mesh_AddBrushMeshFromPlanes(rmesh_t *mesh, int numplanes, mplane_t *planes)
{
	int planenum, planenum2;
	int w;
	int tempnumpoints;
	mplane_t *plane, *plane2;
	double maxdist;
	double temppoints[2][256*3];
	// figure out how large a bounding box we need to properly compute this brush
	maxdist = 0;
	for (w = 0;w < numplanes;w++)
		maxdist = max(maxdist, fabs(planes[w].dist));
	// now make it large enough to enclose the entire brush, and round it off to a reasonable multiple of 1024
	maxdist = floor(maxdist * (4.0 / 1024.0) + 1) * 1024.0;
	for (planenum = 0, plane = planes;planenum < numplanes;planenum++, plane++)
	{
		w = 0;
		tempnumpoints = 4;
		PolygonD_QuadForPlane(temppoints[w], plane->normal[0], plane->normal[1], plane->normal[2], plane->dist, maxdist);
		for (planenum2 = 0, plane2 = planes;planenum2 < numplanes && tempnumpoints >= 3;planenum2++, plane2++)
		{
			if (planenum2 == planenum)
				continue;
			PolygonD_Divide(tempnumpoints, temppoints[w], plane2->normal[0], plane2->normal[1], plane2->normal[2], plane2->dist, R_MESH_PLANE_DIST_EPSILON, 0, NULL, NULL, 256, temppoints[!w], &tempnumpoints, NULL);
			w = !w;
		}
		if (tempnumpoints < 3)
			continue;
		// generate elements forming a triangle fan for this polygon
		R_Mesh_AddPolygon3d(mesh, tempnumpoints, temppoints[w]);
	}
}

static qbool R_TestQ3WaveFunc(q3wavefunc_t func, const float *parms)
{
	if(parms[0] == 0 && parms[1] == 0)
		return false;
	if(func >> Q3WAVEFUNC_USER_SHIFT) // assumes rsurface to be set!
		if(rsurface.userwavefunc_param[bound(0, (func >> Q3WAVEFUNC_USER_SHIFT) - 1, Q3WAVEFUNC_USER_COUNT - 1)] == 0)
			return false;
	return true;
}

static float R_EvaluateQ3WaveFunc(q3wavefunc_t func, const float *parms)
{
	double index, f;
	index = parms[2] + rsurface.shadertime * parms[3];
	index -= floor(index);
	switch (func & ((1 << Q3WAVEFUNC_USER_SHIFT) - 1))
	{
	default:
	case Q3WAVEFUNC_NONE:
	case Q3WAVEFUNC_NOISE:
	case Q3WAVEFUNC_COUNT:
		f = 0;
		break;
	case Q3WAVEFUNC_SIN: f = sin(index * M_PI * 2);break;
	case Q3WAVEFUNC_SQUARE: f = index < 0.5 ? 1 : -1;break;
	case Q3WAVEFUNC_SAWTOOTH: f = index;break;
	case Q3WAVEFUNC_INVERSESAWTOOTH: f = 1 - index;break;
	case Q3WAVEFUNC_TRIANGLE:
		index *= 4;
		f = index - floor(index);
		if (index < 1)
		{
			// f = f;
		}
		else if (index < 2)
			f = 1 - f;
		else if (index < 3)
			f = -f;
		else
			f = -(1 - f);
		break;
	}
	f = parms[0] + parms[1] * f;
	if(func >> Q3WAVEFUNC_USER_SHIFT) // assumes rsurface to be set!
		f *= rsurface.userwavefunc_param[bound(0, (func >> Q3WAVEFUNC_USER_SHIFT) - 1, Q3WAVEFUNC_USER_COUNT - 1)];
	return (float) f;
}

static void R_tcMod_ApplyToMatrix(matrix4x4_t *texmatrix, q3shaderinfo_layer_tcmod_t *tcmod, int currentmaterialflags)
{
	int w, h, idx;
	float shadertime;
	float f;
	float offsetd[2];
	float tcmat[12];
	matrix4x4_t matrix, temp;
	// if shadertime exceeds about 9 hours (32768 seconds), just wrap it,
	// it's better to have one huge fixup every 9 hours than gradual
	// degradation over time which looks consistently bad after many hours.
	//
	// tcmod scroll in particular suffers from this degradation which can't be
	// effectively worked around even with floor() tricks because we don't
	// know if tcmod scroll is the last tcmod being applied, and for clampmap
	// a workaround involving floor() would be incorrect anyway...
	shadertime = rsurface.shadertime;
	if (shadertime >= 32768.0f)
		shadertime -= floor(rsurface.shadertime * (1.0f / 32768.0f)) * 32768.0f;
	switch(tcmod->tcmod)
	{
		case Q3TCMOD_COUNT:
		case Q3TCMOD_NONE:
			if (currentmaterialflags & MATERIALFLAG_WATERSCROLL)
				matrix = r_waterscrollmatrix;
			else
				matrix = identitymatrix;
			break;
		case Q3TCMOD_ENTITYTRANSLATE:
			// this is used in Q3 to allow the gamecode to control texcoord
			// scrolling on the entity, which is not supported in darkplaces yet.
			Matrix4x4_CreateTranslate(&matrix, 0, 0, 0);
			break;
		case Q3TCMOD_ROTATE:
			Matrix4x4_CreateTranslate(&matrix, 0.5, 0.5, 0);
			Matrix4x4_ConcatRotate(&matrix, tcmod->parms[0] * rsurface.shadertime, 0, 0, 1);
			Matrix4x4_ConcatTranslate(&matrix, -0.5, -0.5, 0);
			break;
		case Q3TCMOD_SCALE:
			Matrix4x4_CreateScale3(&matrix, tcmod->parms[0], tcmod->parms[1], 1);
			break;
		case Q3TCMOD_SCROLL:
			// this particular tcmod is a "bug for bug" compatible one with regards to
			// Quake3, the wrapping is unnecessary with our shadetime fix but quake3
			// specifically did the wrapping and so we must mimic that...
			offsetd[0] = tcmod->parms[0] * rsurface.shadertime;
			offsetd[1] = tcmod->parms[1] * rsurface.shadertime;
			Matrix4x4_CreateTranslate(&matrix, offsetd[0] - floor(offsetd[0]), offsetd[1] - floor(offsetd[1]), 0);
			break;
		case Q3TCMOD_PAGE: // poor man's animmap (to store animations into a single file, useful for HTTP downloaded textures)
			w = (int) tcmod->parms[0];
			h = (int) tcmod->parms[1];
			f = rsurface.shadertime / (tcmod->parms[2] * w * h);
			f = f - floor(f);
			idx = (int) floor(f * w * h);
			Matrix4x4_CreateTranslate(&matrix, (idx % w) / tcmod->parms[0], (idx / w) / tcmod->parms[1], 0);
			break;
		case Q3TCMOD_STRETCH:
			f = 1.0f / R_EvaluateQ3WaveFunc(tcmod->wavefunc, tcmod->waveparms);
			Matrix4x4_CreateFromQuakeEntity(&matrix, 0.5f * (1 - f), 0.5 * (1 - f), 0, 0, 0, 0, f);
			break;
		case Q3TCMOD_TRANSFORM:
			VectorSet(tcmat +  0, tcmod->parms[0], tcmod->parms[1], 0);
			VectorSet(tcmat +  3, tcmod->parms[2], tcmod->parms[3], 0);
			VectorSet(tcmat +  6, 0                   , 0                , 1);
			VectorSet(tcmat +  9, tcmod->parms[4], tcmod->parms[5], 0);
			Matrix4x4_FromArray12FloatGL(&matrix, tcmat);
			break;
		case Q3TCMOD_TURBULENT:
			// this is handled in the RSurf_PrepareVertices function
			matrix = identitymatrix;
			break;
	}
	temp = *texmatrix;
	Matrix4x4_Concat(texmatrix, &matrix, &temp);
}

static void R_LoadQWSkin(r_qwskincache_t *cache, const char *skinname)
{
	int textureflags = (r_mipskins.integer ? TEXF_MIPMAP : 0) | TEXF_PICMIP;
	char name[MAX_QPATH];
	skinframe_t *skinframe;
	unsigned char pixels[296*194];
	dp_strlcpy(cache->name, skinname, sizeof(cache->name));
	dpsnprintf(name, sizeof(name), "skins/%s.pcx", cache->name);
	if (developer_loading.integer)
		Con_Printf("loading %s\n", name);
	skinframe = R_SkinFrame_Find(name, textureflags, 0, 0, 0, false);
	if (!skinframe || !skinframe->base)
	{
		unsigned char *f;
		fs_offset_t filesize;
		skinframe = NULL;
		f = FS_LoadFile(name, tempmempool, true, &filesize);
		if (f)
		{
			if (LoadPCX_QWSkin(f, (int)filesize, pixels, 296, 194))
				skinframe = R_SkinFrame_LoadInternalQuake(name, textureflags, true, r_fullbrights.integer, pixels, image_width, image_height);
			Mem_Free(f);
		}
	}
	cache->skinframe = skinframe;
}

texture_t *R_GetCurrentTexture(texture_t *t)
{
	int i, q;
	const entity_render_t *ent = rsurface.entity;
	model_t *model = ent->model; // when calling this, ent must not be NULL
	q3shaderinfo_layer_tcmod_t *tcmod;
	float specularscale = 0.0f;

	if (t->update_lastrenderframe == r_textureframe && t->update_lastrenderentity == (void *)ent && !rsurface.forcecurrenttextureupdate)
		return t->currentframe;
	t->update_lastrenderframe = r_textureframe;
	t->update_lastrenderentity = (void *)ent;

	if(ent->entitynumber >= MAX_EDICTS && ent->entitynumber < 2 * MAX_EDICTS)
		t->camera_entity = ent->entitynumber;
	else
		t->camera_entity = 0;

	// switch to an alternate material if this is a q1bsp animated material
	{
		texture_t *texture = t;
		int s = rsurface.ent_skinnum;
		if ((unsigned int)s >= (unsigned int)model->numskins)
			s = 0;
		if (model->skinscenes)
		{
			if (model->skinscenes[s].framecount > 1)
				s = model->skinscenes[s].firstframe + (unsigned int) (rsurface.shadertime * model->skinscenes[s].framerate) % model->skinscenes[s].framecount;
			else
				s = model->skinscenes[s].firstframe;
		}
		if (s > 0)
			t = t + s * model->num_surfaces;
		if (t->animated)
		{
			// use an alternate animation if the entity's frame is not 0,
			// and only if the texture has an alternate animation
			if (t->animated == 2) // q2bsp
				t = t->anim_frames[0][ent->framegroupblend[0].frame % t->anim_total[0]];
			else if (rsurface.ent_alttextures && t->anim_total[1])
				t = t->anim_frames[1][(t->anim_total[1] >= 2) ? ((int)(rsurface.shadertime * 5.0f) % t->anim_total[1]) : 0];
			else
				t = t->anim_frames[0][(t->anim_total[0] >= 2) ? ((int)(rsurface.shadertime * 5.0f) % t->anim_total[0]) : 0];
		}
		texture->currentframe = t;
	}

	// update currentskinframe to be a qw skin or animation frame
	if (rsurface.ent_qwskin >= 0)
	{
		i = rsurface.ent_qwskin;
		if (!r_qwskincache || r_qwskincache_size != cl.maxclients)
		{
			r_qwskincache_size = cl.maxclients;
			if (r_qwskincache)
				Mem_Free(r_qwskincache);
			r_qwskincache = (r_qwskincache_t *)Mem_Alloc(r_main_mempool, sizeof(*r_qwskincache) * r_qwskincache_size);
		}
		if (strcmp(r_qwskincache[i].name, cl.scores[i].qw_skin))
			R_LoadQWSkin(&r_qwskincache[i], cl.scores[i].qw_skin);
		t->currentskinframe = r_qwskincache[i].skinframe;
		if (t->materialshaderpass && t->currentskinframe == NULL)
			t->currentskinframe = t->materialshaderpass->skinframes[LoopingFrameNumberFromDouble(rsurface.shadertime * t->materialshaderpass->framerate, t->materialshaderpass->numframes)];
	}
	else if (t->materialshaderpass && t->materialshaderpass->numframes >= 2)
		t->currentskinframe = t->materialshaderpass->skinframes[LoopingFrameNumberFromDouble(rsurface.shadertime * t->materialshaderpass->framerate, t->materialshaderpass->numframes)];
	if (t->backgroundshaderpass && t->backgroundshaderpass->numframes >= 2)
		t->backgroundcurrentskinframe = t->backgroundshaderpass->skinframes[LoopingFrameNumberFromDouble(rsurface.shadertime * t->backgroundshaderpass->framerate, t->backgroundshaderpass->numframes)];

	t->currentmaterialflags = t->basematerialflags;
	t->currentalpha = rsurface.entity->alpha * t->basealpha;
	if (t->basematerialflags & MATERIALFLAG_WATERALPHA && (model->brush.supportwateralpha || r_water.integer || r_novis.integer || r_trippy.integer))
		t->currentalpha *= r_wateralpha.value;
	if(t->basematerialflags & MATERIALFLAG_WATERSHADER && r_fb.water.enabled && !r_refdef.view.isoverlay)
		t->currentmaterialflags |= MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW; // we apply wateralpha later
	if(!r_fb.water.enabled || r_refdef.view.isoverlay)
		t->currentmaterialflags &= ~(MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_REFLECTION | MATERIALFLAG_CAMERA);

	// decide on which type of lighting to use for this surface
	if (rsurface.entity->render_modellight_forced)
		t->currentmaterialflags |= MATERIALFLAG_MODELLIGHT;
	if (rsurface.entity->render_rtlight_disabled)
		t->currentmaterialflags |= MATERIALFLAG_NORTLIGHT;
	if (rsurface.entity->render_lightgrid)
		t->currentmaterialflags |= MATERIALFLAG_LIGHTGRID;
	if (t->currentmaterialflags & MATERIALFLAG_CUSTOMBLEND && !(R_BlendFuncFlags(t->customblendfunc[0], t->customblendfunc[1]) & BLENDFUNC_ALLOWS_COLORMOD))
	{
		// some CUSTOMBLEND blendfuncs are too weird, we have to ignore colormod and view colorscale
		t->currentmaterialflags = (t->currentmaterialflags | MATERIALFLAG_MODELLIGHT | MATERIALFLAG_NORTLIGHT) & ~MATERIALFLAG_LIGHTGRID;
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->glowmod[q];
			t->render_modellight_lightdir_world[q] = q == 2;
			t->render_modellight_lightdir_local[q] = q == 2;
			t->render_modellight_ambient[q] = 1;
			t->render_modellight_diffuse[q] = 0;
			t->render_modellight_specular[q] = 0;
			t->render_lightmap_ambient[q] = 0;
			t->render_lightmap_diffuse[q] = 0;
			t->render_lightmap_specular[q] = 0;
			t->render_rtlight_diffuse[q] = 0;
			t->render_rtlight_specular[q] = 0;
		}
	}
	else if ((t->currentmaterialflags & MATERIALFLAG_FULLBRIGHT) || !(rsurface.ent_flags & RENDER_LIGHT))
	{
		// fullbright is basically MATERIALFLAG_MODELLIGHT but with ambient locked to 1,1,1 and no shading
		t->currentmaterialflags = (t->currentmaterialflags | MATERIALFLAG_NORTLIGHT | MATERIALFLAG_MODELLIGHT) & ~MATERIALFLAG_LIGHTGRID;
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->render_glowmod[q] * r_refdef.view.colorscale;
			t->render_modellight_ambient[q] = rsurface.entity->render_fullbright[q] * r_refdef.view.colorscale;
			t->render_modellight_lightdir_world[q] = q == 2;
			t->render_modellight_lightdir_local[q] = q == 2;
			t->render_modellight_diffuse[q] = 0;
			t->render_modellight_specular[q] = 0;
			t->render_lightmap_ambient[q] = 0;
			t->render_lightmap_diffuse[q] = 0;
			t->render_lightmap_specular[q] = 0;
			t->render_rtlight_diffuse[q] = 0;
			t->render_rtlight_specular[q] = 0;
		}
	}
	else if (t->currentmaterialflags & MATERIALFLAG_LIGHTGRID)
	{
		t->currentmaterialflags &= ~MATERIALFLAG_MODELLIGHT;
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->render_glowmod[q] * r_refdef.view.colorscale;
			t->render_modellight_lightdir_world[q] = q == 2;
			t->render_modellight_lightdir_local[q] = q == 2;
			t->render_modellight_ambient[q] = 0;
			t->render_modellight_diffuse[q] = 0;
			t->render_modellight_specular[q] = 0;
			t->render_lightmap_ambient[q] = rsurface.entity->render_lightmap_ambient[q] * r_refdef.view.colorscale;
			t->render_lightmap_diffuse[q] = rsurface.entity->render_lightmap_diffuse[q] * 2 * r_refdef.view.colorscale;
			t->render_lightmap_specular[q] = rsurface.entity->render_lightmap_specular[q] * 2 * r_refdef.view.colorscale;
			t->render_rtlight_diffuse[q] = rsurface.entity->render_rtlight_diffuse[q] * r_refdef.view.colorscale;
			t->render_rtlight_specular[q] = rsurface.entity->render_rtlight_specular[q] * r_refdef.view.colorscale;
		}
	}
	else if ((rsurface.ent_flags & (RENDER_DYNAMICMODELLIGHT | RENDER_CUSTOMIZEDMODELLIGHT)) || rsurface.modeltexcoordlightmap2f == NULL)
	{
		// ambient + single direction light (modellight)
		t->currentmaterialflags = (t->currentmaterialflags | MATERIALFLAG_MODELLIGHT) & ~MATERIALFLAG_LIGHTGRID;
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->render_glowmod[q] * r_refdef.view.colorscale;
			t->render_modellight_lightdir_world[q] = rsurface.entity->render_modellight_lightdir_world[q];
			t->render_modellight_lightdir_local[q] = rsurface.entity->render_modellight_lightdir_local[q];
			t->render_modellight_ambient[q] = rsurface.entity->render_modellight_ambient[q] * r_refdef.view.colorscale;
			t->render_modellight_diffuse[q] = rsurface.entity->render_modellight_diffuse[q] * r_refdef.view.colorscale;
			t->render_modellight_specular[q] = rsurface.entity->render_modellight_specular[q] * r_refdef.view.colorscale;
			t->render_lightmap_ambient[q] = 0;
			t->render_lightmap_diffuse[q] = 0;
			t->render_lightmap_specular[q] = 0;
			t->render_rtlight_diffuse[q] = rsurface.entity->render_rtlight_diffuse[q] * r_refdef.view.colorscale;
			t->render_rtlight_specular[q] = rsurface.entity->render_rtlight_specular[q] * r_refdef.view.colorscale;
		}
	}
	else
	{
		// lightmap - 2x diffuse and specular brightness because bsp files have 0-2 colors as 0-1
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->render_glowmod[q] * r_refdef.view.colorscale;
			t->render_modellight_lightdir_world[q] = q == 2;
			t->render_modellight_lightdir_local[q] = q == 2;
			t->render_modellight_ambient[q] = 0;
			t->render_modellight_diffuse[q] = 0;
			t->render_modellight_specular[q] = 0;
			t->render_lightmap_ambient[q] = rsurface.entity->render_lightmap_ambient[q] * r_refdef.view.colorscale;
			t->render_lightmap_diffuse[q] = rsurface.entity->render_lightmap_diffuse[q] * 2 * r_refdef.view.colorscale;
			t->render_lightmap_specular[q] = rsurface.entity->render_lightmap_specular[q] * 2 * r_refdef.view.colorscale;
			t->render_rtlight_diffuse[q] = rsurface.entity->render_rtlight_diffuse[q] * r_refdef.view.colorscale;
			t->render_rtlight_specular[q] = rsurface.entity->render_rtlight_specular[q] * r_refdef.view.colorscale;
		}
	}

	if (t->currentmaterialflags & MATERIALFLAG_VERTEXCOLOR)
	{
		// since MATERIALFLAG_VERTEXCOLOR uses the lightmapcolor4f vertex
		// attribute, we punt it to the lightmap path and hope for the best,
		// but lighting doesn't work.
		//
		// FIXME: this is fine for effects but CSQC polygons should be subject
		// to lighting.
		t->currentmaterialflags &= ~(MATERIALFLAG_MODELLIGHT | MATERIALFLAG_LIGHTGRID);
		for (q = 0; q < 3; q++)
		{
			t->render_glowmod[q] = rsurface.entity->render_glowmod[q] * r_refdef.view.colorscale;
			t->render_modellight_lightdir_world[q] = q == 2;
			t->render_modellight_lightdir_local[q] = q == 2;
			t->render_modellight_ambient[q] = 0;
			t->render_modellight_diffuse[q] = 0;
			t->render_modellight_specular[q] = 0;
			t->render_lightmap_ambient[q] = 0;
			t->render_lightmap_diffuse[q] = rsurface.entity->render_fullbright[q] * r_refdef.view.colorscale;
			t->render_lightmap_specular[q] = 0;
			t->render_rtlight_diffuse[q] = 0;
			t->render_rtlight_specular[q] = 0;
		}
	}

	for (q = 0; q < 3; q++)
	{
		t->render_colormap_pants[q] = rsurface.entity->colormap_pantscolor[q];
		t->render_colormap_shirt[q] = rsurface.entity->colormap_shirtcolor[q];
	}

	if (rsurface.ent_flags & RENDER_ADDITIVE)
		t->currentmaterialflags |= MATERIALFLAG_ADD | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
	else if (t->currentalpha < 1)
		t->currentmaterialflags |= MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
	// LadyHavoc: prevent bugs where code checks add or alpha at higher priority than customblend by clearing these flags
	if (t->currentmaterialflags & MATERIALFLAG_CUSTOMBLEND)
		t->currentmaterialflags &= ~(MATERIALFLAG_ADD | MATERIALFLAG_ALPHA);
	if (rsurface.ent_flags & RENDER_DOUBLESIDED)
		t->currentmaterialflags |= MATERIALFLAG_NOSHADOW | MATERIALFLAG_NOCULLFACE;
	if (rsurface.ent_flags & (RENDER_NODEPTHTEST | RENDER_VIEWMODEL))
		t->currentmaterialflags |= MATERIALFLAG_SHORTDEPTHRANGE;
	if (t->backgroundshaderpass)
		t->currentmaterialflags |= MATERIALFLAG_VERTEXTEXTUREBLEND;
	if (t->currentmaterialflags & MATERIALFLAG_BLENDED)
	{
		if (t->currentmaterialflags & (MATERIALFLAG_REFRACTION | MATERIALFLAG_WATERSHADER | MATERIALFLAG_CAMERA))
			t->currentmaterialflags &= ~MATERIALFLAG_BLENDED;
	}
	else
		t->currentmaterialflags &= ~(MATERIALFLAG_REFRACTION | MATERIALFLAG_WATERSHADER | MATERIALFLAG_CAMERA);
	if (vid.allowalphatocoverage && r_transparent_alphatocoverage.integer >= 2 && ((t->currentmaterialflags & (MATERIALFLAG_BLENDED | MATERIALFLAG_ALPHA | MATERIALFLAG_ADD | MATERIALFLAG_CUSTOMBLEND)) == (MATERIALFLAG_BLENDED | MATERIALFLAG_ALPHA)))
	{
		// promote alphablend to alphatocoverage (a type of alphatest) if antialiasing is on
		t->currentmaterialflags = (t->currentmaterialflags & ~(MATERIALFLAG_BLENDED | MATERIALFLAG_ALPHA)) | MATERIALFLAG_ALPHATEST;
	}
	if ((t->currentmaterialflags & (MATERIALFLAG_BLENDED | MATERIALFLAG_NODEPTHTEST)) == MATERIALFLAG_BLENDED && r_transparentdepthmasking.integer && !(t->basematerialflags & MATERIALFLAG_BLENDED))
		t->currentmaterialflags |= MATERIALFLAG_TRANSDEPTH;

	// there is no tcmod
	if (t->currentmaterialflags & MATERIALFLAG_WATERSCROLL)
	{
		t->currenttexmatrix = r_waterscrollmatrix;
		t->currentbackgroundtexmatrix = r_waterscrollmatrix;
	}
	else if (!(t->currentmaterialflags & MATERIALFLAG_CUSTOMSURFACE))
	{
		Matrix4x4_CreateIdentity(&t->currenttexmatrix);
		Matrix4x4_CreateIdentity(&t->currentbackgroundtexmatrix);
	}

	if (t->materialshaderpass)
		for (i = 0, tcmod = t->materialshaderpass->tcmods;i < Q3MAXTCMODS && tcmod->tcmod;i++, tcmod++)
			R_tcMod_ApplyToMatrix(&t->currenttexmatrix, tcmod, t->currentmaterialflags);

	t->colormapping = VectorLength2(t->render_colormap_pants) + VectorLength2(t->render_colormap_shirt) >= (1.0f / 1048576.0f);
	if (t->currentskinframe->qpixels)
		R_SkinFrame_GenerateTexturesFromQPixels(t->currentskinframe, t->colormapping);
	t->basetexture = (!t->colormapping && t->currentskinframe->merged) ? t->currentskinframe->merged : t->currentskinframe->base;
	if (!t->basetexture)
		t->basetexture = r_texture_notexture;
	t->pantstexture = t->colormapping ? t->currentskinframe->pants : NULL;
	t->shirttexture = t->colormapping ? t->currentskinframe->shirt : NULL;
	t->nmaptexture = t->currentskinframe->nmap;
	if (!t->nmaptexture)
		t->nmaptexture = r_texture_blanknormalmap;
	t->glosstexture = r_texture_black;
	t->glowtexture = t->currentskinframe->glow;
	t->fogtexture = t->currentskinframe->fog;
	t->reflectmasktexture = t->currentskinframe->reflect;
	if (t->backgroundshaderpass)
	{
		for (i = 0, tcmod = t->backgroundshaderpass->tcmods; i < Q3MAXTCMODS && tcmod->tcmod; i++, tcmod++)
			R_tcMod_ApplyToMatrix(&t->currentbackgroundtexmatrix, tcmod, t->currentmaterialflags);
		t->backgroundbasetexture = (!t->colormapping && t->backgroundcurrentskinframe->merged) ? t->backgroundcurrentskinframe->merged : t->backgroundcurrentskinframe->base;
		t->backgroundnmaptexture = t->backgroundcurrentskinframe->nmap;
		t->backgroundglosstexture = r_texture_black;
		t->backgroundglowtexture = t->backgroundcurrentskinframe->glow;
		if (!t->backgroundnmaptexture)
			t->backgroundnmaptexture = r_texture_blanknormalmap;
		// make sure that if glow is going to be used, both textures are not NULL
		if (!t->backgroundglowtexture && t->glowtexture)
			t->backgroundglowtexture = r_texture_black;
		if (!t->glowtexture && t->backgroundglowtexture)
			t->glowtexture = r_texture_black;
	}
	else
	{
		t->backgroundbasetexture = r_texture_white;
		t->backgroundnmaptexture = r_texture_blanknormalmap;
		t->backgroundglosstexture = r_texture_black;
		t->backgroundglowtexture = NULL;
	}
	t->specularpower = r_shadow_glossexponent.value;
	// TODO: store reference values for these in the texture?
	if (r_shadow_gloss.integer > 0)
	{
		if (t->currentskinframe->gloss || (t->backgroundcurrentskinframe && t->backgroundcurrentskinframe->gloss))
		{
			if (r_shadow_glossintensity.value > 0)
			{
				t->glosstexture = t->currentskinframe->gloss ? t->currentskinframe->gloss : r_texture_white;
				t->backgroundglosstexture = (t->backgroundcurrentskinframe && t->backgroundcurrentskinframe->gloss) ? t->backgroundcurrentskinframe->gloss : r_texture_white;
				specularscale = r_shadow_glossintensity.value;
			}
		}
		else if (r_shadow_gloss.integer >= 2 && r_shadow_gloss2intensity.value > 0)
		{
			t->glosstexture = r_texture_white;
			t->backgroundglosstexture = r_texture_white;
			specularscale = r_shadow_gloss2intensity.value;
			t->specularpower = r_shadow_gloss2exponent.value;
		}
	}
	specularscale *= t->specularscalemod;
	t->specularpower *= t->specularpowermod;

	// lightmaps mode looks bad with dlights using actual texturing, so turn
	// off the colormap and glossmap, but leave the normalmap on as it still
	// accurately represents the shading involved
	if (gl_lightmaps.integer && ent != &cl_meshentities[MESH_UI].render)
	{
		t->basetexture = r_texture_grey128;
		t->pantstexture = r_texture_black;
		t->shirttexture = r_texture_black;
		if (gl_lightmaps.integer < 2)
			t->nmaptexture = r_texture_blanknormalmap;
		t->glosstexture = r_texture_black;
		t->glowtexture = NULL;
		t->fogtexture = NULL;
		t->reflectmasktexture = NULL;
		t->backgroundbasetexture = NULL;
		if (gl_lightmaps.integer < 2)
			t->backgroundnmaptexture = r_texture_blanknormalmap;
		t->backgroundglosstexture = r_texture_black;
		t->backgroundglowtexture = NULL;
		specularscale = 0;
		t->currentmaterialflags = MATERIALFLAG_WALL | (t->currentmaterialflags & (MATERIALFLAG_NOCULLFACE | MATERIALFLAG_MODELLIGHT | MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_SHORTDEPTHRANGE));
	}

	if (specularscale != 1.0f)
	{
		for (q = 0; q < 3; q++)
		{
			t->render_modellight_specular[q] *= specularscale;
			t->render_lightmap_specular[q] *= specularscale;
			t->render_rtlight_specular[q] *= specularscale;
		}
	}

	t->currentblendfunc[0] = GL_ONE;
	t->currentblendfunc[1] = GL_ZERO;
	if (t->currentmaterialflags & MATERIALFLAG_ADD)
	{
		t->currentblendfunc[0] = GL_SRC_ALPHA;
		t->currentblendfunc[1] = GL_ONE;
	}
	else if (t->currentmaterialflags & MATERIALFLAG_ALPHA)
	{
		t->currentblendfunc[0] = GL_SRC_ALPHA;
		t->currentblendfunc[1] = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (t->currentmaterialflags & MATERIALFLAG_CUSTOMBLEND)
	{
		t->currentblendfunc[0] = t->customblendfunc[0];
		t->currentblendfunc[1] = t->customblendfunc[1];
	}

	return t;
}

rsurfacestate_t rsurface;

void RSurf_ActiveModelEntity(const entity_render_t *ent, qbool wantnormals, qbool wanttangents, qbool prepass)
{
	model_t *model = ent->model;
	//if (rsurface.entity == ent && (!model->surfmesh.isanimated || (!wantnormals && !wanttangents)))
	//	return;
	rsurface.entity = (entity_render_t *)ent;
	rsurface.skeleton = ent->skeleton;
	memcpy(rsurface.userwavefunc_param, ent->userwavefunc_param, sizeof(rsurface.userwavefunc_param));
	rsurface.ent_skinnum = ent->skinnum;
	rsurface.ent_qwskin = (ent->entitynumber <= cl.maxclients && ent->entitynumber >= 1 && cls.protocol == PROTOCOL_QUAKEWORLD && cl.scores[ent->entitynumber - 1].qw_skin[0] && !strcmp(ent->model->name, "progs/player.mdl")) ? (ent->entitynumber - 1) : -1;
	rsurface.ent_flags = ent->flags;
	if (r_fullbright_directed.integer && (r_fullbright.integer || !model->lit))
		rsurface.ent_flags |= RENDER_LIGHT | RENDER_DYNAMICMODELLIGHT;
	rsurface.shadertime = r_refdef.scene.time - ent->shadertime;
	rsurface.matrix = ent->matrix;
	rsurface.inversematrix = ent->inversematrix;
	rsurface.matrixscale = Matrix4x4_ScaleFromMatrix(&rsurface.matrix);
	rsurface.inversematrixscale = 1.0f / rsurface.matrixscale;
	R_EntityMatrix(&rsurface.matrix);
	Matrix4x4_Transform(&rsurface.inversematrix, r_refdef.view.origin, rsurface.localvieworigin);
	Matrix4x4_TransformStandardPlane(&rsurface.inversematrix, r_refdef.fogplane[0], r_refdef.fogplane[1], r_refdef.fogplane[2], r_refdef.fogplane[3], rsurface.fogplane);
	rsurface.fogplaneviewdist = r_refdef.fogplaneviewdist * rsurface.inversematrixscale;
	rsurface.fograngerecip = r_refdef.fograngerecip * rsurface.matrixscale;
	rsurface.fogheightfade = r_refdef.fogheightfade * rsurface.matrixscale;
	rsurface.fogmasktabledistmultiplier = FOGMASKTABLEWIDTH * rsurface.fograngerecip;
	memcpy(rsurface.frameblend, ent->frameblend, sizeof(ent->frameblend));
	rsurface.ent_alttextures = ent->framegroupblend[0].frame != 0;
	rsurface.basepolygonfactor = r_refdef.polygonfactor;
	rsurface.basepolygonoffset = r_refdef.polygonoffset;
	if (ent->model->brush.submodel && !prepass)
	{
		rsurface.basepolygonfactor += r_polygonoffset_submodel_factor.value;
		rsurface.basepolygonoffset += r_polygonoffset_submodel_offset.value;
	}
	// if the animcache code decided it should use the shader path, skip the deform step
	rsurface.entityskeletaltransform3x4 = ent->animcache_skeletaltransform3x4;
	rsurface.entityskeletaltransform3x4buffer = ent->animcache_skeletaltransform3x4buffer;
	rsurface.entityskeletaltransform3x4offset = ent->animcache_skeletaltransform3x4offset;
	rsurface.entityskeletaltransform3x4size = ent->animcache_skeletaltransform3x4size;
	rsurface.entityskeletalnumtransforms = rsurface.entityskeletaltransform3x4 ? model->num_bones : 0;
	if (model->surfmesh.isanimated && model->AnimateVertices && !rsurface.entityskeletaltransform3x4)
	{
		if (ent->animcache_vertex3f)
		{
			r_refdef.stats[r_stat_batch_entitycache_count]++;
			r_refdef.stats[r_stat_batch_entitycache_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entitycache_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entitycache_triangles] += model->surfmesh.num_triangles;
			rsurface.modelvertex3f = ent->animcache_vertex3f;
			rsurface.modelvertex3f_vertexbuffer = ent->animcache_vertex3f_vertexbuffer;
			rsurface.modelvertex3f_bufferoffset = ent->animcache_vertex3f_bufferoffset;
			rsurface.modelsvector3f = wanttangents ? ent->animcache_svector3f : NULL;
			rsurface.modelsvector3f_vertexbuffer = wanttangents ? ent->animcache_svector3f_vertexbuffer : NULL;
			rsurface.modelsvector3f_bufferoffset = wanttangents ? ent->animcache_svector3f_bufferoffset : 0;
			rsurface.modeltvector3f = wanttangents ? ent->animcache_tvector3f : NULL;
			rsurface.modeltvector3f_vertexbuffer = wanttangents ? ent->animcache_tvector3f_vertexbuffer : NULL;
			rsurface.modeltvector3f_bufferoffset = wanttangents ? ent->animcache_tvector3f_bufferoffset : 0;
			rsurface.modelnormal3f = wantnormals ? ent->animcache_normal3f : NULL;
			rsurface.modelnormal3f_vertexbuffer = wantnormals ? ent->animcache_normal3f_vertexbuffer : NULL;
			rsurface.modelnormal3f_bufferoffset = wantnormals ? ent->animcache_normal3f_bufferoffset : 0;
		}
		else if (wanttangents)
		{
			r_refdef.stats[r_stat_batch_entityanimate_count]++;
			r_refdef.stats[r_stat_batch_entityanimate_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entityanimate_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entityanimate_triangles] += model->surfmesh.num_triangles;
			rsurface.modelvertex3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			rsurface.modelsvector3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			rsurface.modeltvector3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			rsurface.modelnormal3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			model->AnimateVertices(model, rsurface.frameblend, rsurface.skeleton, rsurface.modelvertex3f, rsurface.modelnormal3f, rsurface.modelsvector3f, rsurface.modeltvector3f);
			rsurface.modelvertex3f_vertexbuffer = NULL;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelvertex3f_vertexbuffer = 0;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelsvector3f_vertexbuffer = 0;
			rsurface.modelsvector3f_bufferoffset = 0;
			rsurface.modeltvector3f_vertexbuffer = 0;
			rsurface.modeltvector3f_bufferoffset = 0;
			rsurface.modelnormal3f_vertexbuffer = 0;
			rsurface.modelnormal3f_bufferoffset = 0;
		}
		else if (wantnormals)
		{
			r_refdef.stats[r_stat_batch_entityanimate_count]++;
			r_refdef.stats[r_stat_batch_entityanimate_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entityanimate_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entityanimate_triangles] += model->surfmesh.num_triangles;
			rsurface.modelvertex3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			rsurface.modelsvector3f = NULL;
			rsurface.modeltvector3f = NULL;
			rsurface.modelnormal3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			model->AnimateVertices(model, rsurface.frameblend, rsurface.skeleton, rsurface.modelvertex3f, rsurface.modelnormal3f, NULL, NULL);
			rsurface.modelvertex3f_vertexbuffer = NULL;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelvertex3f_vertexbuffer = 0;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelsvector3f_vertexbuffer = 0;
			rsurface.modelsvector3f_bufferoffset = 0;
			rsurface.modeltvector3f_vertexbuffer = 0;
			rsurface.modeltvector3f_bufferoffset = 0;
			rsurface.modelnormal3f_vertexbuffer = 0;
			rsurface.modelnormal3f_bufferoffset = 0;
		}
		else
		{
			r_refdef.stats[r_stat_batch_entityanimate_count]++;
			r_refdef.stats[r_stat_batch_entityanimate_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entityanimate_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entityanimate_triangles] += model->surfmesh.num_triangles;
			rsurface.modelvertex3f = (float *)R_FrameData_Alloc(model->surfmesh.num_vertices * sizeof(float[3]));
			rsurface.modelsvector3f = NULL;
			rsurface.modeltvector3f = NULL;
			rsurface.modelnormal3f = NULL;
			model->AnimateVertices(model, rsurface.frameblend, rsurface.skeleton, rsurface.modelvertex3f, NULL, NULL, NULL);
			rsurface.modelvertex3f_vertexbuffer = NULL;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelvertex3f_vertexbuffer = 0;
			rsurface.modelvertex3f_bufferoffset = 0;
			rsurface.modelsvector3f_vertexbuffer = 0;
			rsurface.modelsvector3f_bufferoffset = 0;
			rsurface.modeltvector3f_vertexbuffer = 0;
			rsurface.modeltvector3f_bufferoffset = 0;
			rsurface.modelnormal3f_vertexbuffer = 0;
			rsurface.modelnormal3f_bufferoffset = 0;
		}
		rsurface.modelgeneratedvertex = true;
	}
	else
	{
		if (rsurface.entityskeletaltransform3x4)
		{
			r_refdef.stats[r_stat_batch_entityskeletal_count]++;
			r_refdef.stats[r_stat_batch_entityskeletal_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entityskeletal_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entityskeletal_triangles] += model->surfmesh.num_triangles;
		}
		else
		{
			r_refdef.stats[r_stat_batch_entitystatic_count]++;
			r_refdef.stats[r_stat_batch_entitystatic_surfaces] += model->num_surfaces;
			r_refdef.stats[r_stat_batch_entitystatic_vertices] += model->surfmesh.num_vertices;
			r_refdef.stats[r_stat_batch_entitystatic_triangles] += model->surfmesh.num_triangles;
		}
		rsurface.modelvertex3f  = model->surfmesh.data_vertex3f;
		rsurface.modelvertex3f_vertexbuffer = model->surfmesh.data_vertex3f_vertexbuffer;
		rsurface.modelvertex3f_bufferoffset = model->surfmesh.data_vertex3f_bufferoffset;
		rsurface.modelsvector3f = model->surfmesh.data_svector3f;
		rsurface.modelsvector3f_vertexbuffer = model->surfmesh.data_svector3f_vertexbuffer;
		rsurface.modelsvector3f_bufferoffset = model->surfmesh.data_svector3f_bufferoffset;
		rsurface.modeltvector3f = model->surfmesh.data_tvector3f;
		rsurface.modeltvector3f_vertexbuffer = model->surfmesh.data_tvector3f_vertexbuffer;
		rsurface.modeltvector3f_bufferoffset = model->surfmesh.data_tvector3f_bufferoffset;
		rsurface.modelnormal3f  = model->surfmesh.data_normal3f;
		rsurface.modelnormal3f_vertexbuffer = model->surfmesh.data_normal3f_vertexbuffer;
		rsurface.modelnormal3f_bufferoffset = model->surfmesh.data_normal3f_bufferoffset;
		rsurface.modelgeneratedvertex = false;
	}
	rsurface.modellightmapcolor4f  = model->surfmesh.data_lightmapcolor4f;
	rsurface.modellightmapcolor4f_vertexbuffer = model->surfmesh.data_lightmapcolor4f_vertexbuffer;
	rsurface.modellightmapcolor4f_bufferoffset = model->surfmesh.data_lightmapcolor4f_bufferoffset;
	rsurface.modeltexcoordtexture2f  = model->surfmesh.data_texcoordtexture2f;
	rsurface.modeltexcoordtexture2f_vertexbuffer = model->surfmesh.data_texcoordtexture2f_vertexbuffer;
	rsurface.modeltexcoordtexture2f_bufferoffset = model->surfmesh.data_texcoordtexture2f_bufferoffset;
	rsurface.modeltexcoordlightmap2f  = model->surfmesh.data_texcoordlightmap2f;
	rsurface.modeltexcoordlightmap2f_vertexbuffer = model->surfmesh.data_texcoordlightmap2f_vertexbuffer;
	rsurface.modeltexcoordlightmap2f_bufferoffset = model->surfmesh.data_texcoordlightmap2f_bufferoffset;
	rsurface.modelskeletalindex4ub = model->surfmesh.data_skeletalindex4ub;
	rsurface.modelskeletalindex4ub_vertexbuffer = model->surfmesh.data_skeletalindex4ub_vertexbuffer;
	rsurface.modelskeletalindex4ub_bufferoffset = model->surfmesh.data_skeletalindex4ub_bufferoffset;
	rsurface.modelskeletalweight4ub = model->surfmesh.data_skeletalweight4ub;
	rsurface.modelskeletalweight4ub_vertexbuffer = model->surfmesh.data_skeletalweight4ub_vertexbuffer;
	rsurface.modelskeletalweight4ub_bufferoffset = model->surfmesh.data_skeletalweight4ub_bufferoffset;
	rsurface.modelelement3i = model->surfmesh.data_element3i;
	rsurface.modelelement3i_indexbuffer = model->surfmesh.data_element3i_indexbuffer;
	rsurface.modelelement3i_bufferoffset = model->surfmesh.data_element3i_bufferoffset;
	rsurface.modelelement3s = model->surfmesh.data_element3s;
	rsurface.modelelement3s_indexbuffer = model->surfmesh.data_element3s_indexbuffer;
	rsurface.modelelement3s_bufferoffset = model->surfmesh.data_element3s_bufferoffset;
	rsurface.modellightmapoffsets = model->surfmesh.data_lightmapoffsets;
	rsurface.modelnumvertices = model->surfmesh.num_vertices;
	rsurface.modelnumtriangles = model->surfmesh.num_triangles;
	rsurface.modelsurfaces = model->data_surfaces;
	rsurface.batchgeneratedvertex = false;
	rsurface.batchfirstvertex = 0;
	rsurface.batchnumvertices = 0;
	rsurface.batchfirsttriangle = 0;
	rsurface.batchnumtriangles = 0;
	rsurface.batchvertex3f  = NULL;
	rsurface.batchvertex3f_vertexbuffer = NULL;
	rsurface.batchvertex3f_bufferoffset = 0;
	rsurface.batchsvector3f = NULL;
	rsurface.batchsvector3f_vertexbuffer = NULL;
	rsurface.batchsvector3f_bufferoffset = 0;
	rsurface.batchtvector3f = NULL;
	rsurface.batchtvector3f_vertexbuffer = NULL;
	rsurface.batchtvector3f_bufferoffset = 0;
	rsurface.batchnormal3f  = NULL;
	rsurface.batchnormal3f_vertexbuffer = NULL;
	rsurface.batchnormal3f_bufferoffset = 0;
	rsurface.batchlightmapcolor4f = NULL;
	rsurface.batchlightmapcolor4f_vertexbuffer = NULL;
	rsurface.batchlightmapcolor4f_bufferoffset = 0;
	rsurface.batchtexcoordtexture2f = NULL;
	rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
	rsurface.batchtexcoordtexture2f_bufferoffset = 0;
	rsurface.batchtexcoordlightmap2f = NULL;
	rsurface.batchtexcoordlightmap2f_vertexbuffer = NULL;
	rsurface.batchtexcoordlightmap2f_bufferoffset = 0;
	rsurface.batchskeletalindex4ub = NULL;
	rsurface.batchskeletalindex4ub_vertexbuffer = NULL;
	rsurface.batchskeletalindex4ub_bufferoffset = 0;
	rsurface.batchskeletalweight4ub = NULL;
	rsurface.batchskeletalweight4ub_vertexbuffer = NULL;
	rsurface.batchskeletalweight4ub_bufferoffset = 0;
	rsurface.batchelement3i = NULL;
	rsurface.batchelement3i_indexbuffer = NULL;
	rsurface.batchelement3i_bufferoffset = 0;
	rsurface.batchelement3s = NULL;
	rsurface.batchelement3s_indexbuffer = NULL;
	rsurface.batchelement3s_bufferoffset = 0;
	rsurface.forcecurrenttextureupdate = false;
}

void RSurf_ActiveCustomEntity(const matrix4x4_t *matrix, const matrix4x4_t *inversematrix, int entflags, double shadertime, float r, float g, float b, float a, int numvertices, const float *vertex3f, const float *texcoord2f, const float *normal3f, const float *svector3f, const float *tvector3f, const float *color4f, int numtriangles, const int *element3i, const unsigned short *element3s, qbool wantnormals, qbool wanttangents)
{
	rsurface.entity = r_refdef.scene.worldentity;
	if (r != 1.0f || g != 1.0f || b != 1.0f || a != 1.0f) {
		// HACK to provide a valid entity with modded colors to R_GetCurrentTexture.
		// A better approach could be making this copy only once per frame.
		static entity_render_t custom_entity;
		int q;
		custom_entity = *rsurface.entity;
		for (q = 0; q < 3; ++q) {
			float colormod = q == 0 ? r : q == 1 ? g : b;
			custom_entity.render_fullbright[q] *= colormod;
			custom_entity.render_modellight_ambient[q] *= colormod;
			custom_entity.render_modellight_diffuse[q] *= colormod;
			custom_entity.render_lightmap_ambient[q] *= colormod;
			custom_entity.render_lightmap_diffuse[q] *= colormod;
			custom_entity.render_rtlight_diffuse[q] *= colormod;
		}
		custom_entity.alpha *= a;
		rsurface.entity = &custom_entity;
	}
	rsurface.skeleton = NULL;
	rsurface.ent_skinnum = 0;
	rsurface.ent_qwskin = -1;
	rsurface.ent_flags = entflags;
	rsurface.shadertime = r_refdef.scene.time - shadertime;
	rsurface.modelnumvertices = numvertices;
	rsurface.modelnumtriangles = numtriangles;
	rsurface.matrix = *matrix;
	rsurface.inversematrix = *inversematrix;
	rsurface.matrixscale = Matrix4x4_ScaleFromMatrix(&rsurface.matrix);
	rsurface.inversematrixscale = 1.0f / rsurface.matrixscale;
	R_EntityMatrix(&rsurface.matrix);
	Matrix4x4_Transform(&rsurface.inversematrix, r_refdef.view.origin, rsurface.localvieworigin);
	Matrix4x4_TransformStandardPlane(&rsurface.inversematrix, r_refdef.fogplane[0], r_refdef.fogplane[1], r_refdef.fogplane[2], r_refdef.fogplane[3], rsurface.fogplane);
	rsurface.fogplaneviewdist *= rsurface.inversematrixscale;
	rsurface.fograngerecip = r_refdef.fograngerecip * rsurface.matrixscale;
	rsurface.fogheightfade = r_refdef.fogheightfade * rsurface.matrixscale;
	rsurface.fogmasktabledistmultiplier = FOGMASKTABLEWIDTH * rsurface.fograngerecip;
	memset(rsurface.frameblend, 0, sizeof(rsurface.frameblend));
	rsurface.frameblend[0].lerp = 1;
	rsurface.ent_alttextures = false;
	rsurface.basepolygonfactor = r_refdef.polygonfactor;
	rsurface.basepolygonoffset = r_refdef.polygonoffset;
	rsurface.entityskeletaltransform3x4 = NULL;
	rsurface.entityskeletaltransform3x4buffer = NULL;
	rsurface.entityskeletaltransform3x4offset = 0;
	rsurface.entityskeletaltransform3x4size = 0;
	rsurface.entityskeletalnumtransforms = 0;
	r_refdef.stats[r_stat_batch_entitycustom_count]++;
	r_refdef.stats[r_stat_batch_entitycustom_surfaces] += 1;
	r_refdef.stats[r_stat_batch_entitycustom_vertices] += rsurface.modelnumvertices;
	r_refdef.stats[r_stat_batch_entitycustom_triangles] += rsurface.modelnumtriangles;
	if (wanttangents)
	{
		rsurface.modelvertex3f = (float *)vertex3f;
		rsurface.modelsvector3f = svector3f ? (float *)svector3f : (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
		rsurface.modeltvector3f = tvector3f ? (float *)tvector3f : (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
		rsurface.modelnormal3f = normal3f ? (float *)normal3f : (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
	}
	else if (wantnormals)
	{
		rsurface.modelvertex3f = (float *)vertex3f;
		rsurface.modelsvector3f = NULL;
		rsurface.modeltvector3f = NULL;
		rsurface.modelnormal3f = normal3f ? (float *)normal3f : (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
	}
	else
	{
		rsurface.modelvertex3f = (float *)vertex3f;
		rsurface.modelsvector3f = NULL;
		rsurface.modeltvector3f = NULL;
		rsurface.modelnormal3f = NULL;
	}
	rsurface.modelvertex3f_vertexbuffer = 0;
	rsurface.modelvertex3f_bufferoffset = 0;
	rsurface.modelsvector3f_vertexbuffer = 0;
	rsurface.modelsvector3f_bufferoffset = 0;
	rsurface.modeltvector3f_vertexbuffer = 0;
	rsurface.modeltvector3f_bufferoffset = 0;
	rsurface.modelnormal3f_vertexbuffer = 0;
	rsurface.modelnormal3f_bufferoffset = 0;
	rsurface.modelgeneratedvertex = true;
	rsurface.modellightmapcolor4f  = (float *)color4f;
	rsurface.modellightmapcolor4f_vertexbuffer = 0;
	rsurface.modellightmapcolor4f_bufferoffset = 0;
	rsurface.modeltexcoordtexture2f  = (float *)texcoord2f;
	rsurface.modeltexcoordtexture2f_vertexbuffer = 0;
	rsurface.modeltexcoordtexture2f_bufferoffset = 0;
	rsurface.modeltexcoordlightmap2f  = NULL;
	rsurface.modeltexcoordlightmap2f_vertexbuffer = 0;
	rsurface.modeltexcoordlightmap2f_bufferoffset = 0;
	rsurface.modelskeletalindex4ub = NULL;
	rsurface.modelskeletalindex4ub_vertexbuffer = NULL;
	rsurface.modelskeletalindex4ub_bufferoffset = 0;
	rsurface.modelskeletalweight4ub = NULL;
	rsurface.modelskeletalweight4ub_vertexbuffer = NULL;
	rsurface.modelskeletalweight4ub_bufferoffset = 0;
	rsurface.modelelement3i = (int *)element3i;
	rsurface.modelelement3i_indexbuffer = NULL;
	rsurface.modelelement3i_bufferoffset = 0;
	rsurface.modelelement3s = (unsigned short *)element3s;
	rsurface.modelelement3s_indexbuffer = NULL;
	rsurface.modelelement3s_bufferoffset = 0;
	rsurface.modellightmapoffsets = NULL;
	rsurface.modelsurfaces = NULL;
	rsurface.batchgeneratedvertex = false;
	rsurface.batchfirstvertex = 0;
	rsurface.batchnumvertices = 0;
	rsurface.batchfirsttriangle = 0;
	rsurface.batchnumtriangles = 0;
	rsurface.batchvertex3f  = NULL;
	rsurface.batchvertex3f_vertexbuffer = NULL;
	rsurface.batchvertex3f_bufferoffset = 0;
	rsurface.batchsvector3f = NULL;
	rsurface.batchsvector3f_vertexbuffer = NULL;
	rsurface.batchsvector3f_bufferoffset = 0;
	rsurface.batchtvector3f = NULL;
	rsurface.batchtvector3f_vertexbuffer = NULL;
	rsurface.batchtvector3f_bufferoffset = 0;
	rsurface.batchnormal3f  = NULL;
	rsurface.batchnormal3f_vertexbuffer = NULL;
	rsurface.batchnormal3f_bufferoffset = 0;
	rsurface.batchlightmapcolor4f = NULL;
	rsurface.batchlightmapcolor4f_vertexbuffer = NULL;
	rsurface.batchlightmapcolor4f_bufferoffset = 0;
	rsurface.batchtexcoordtexture2f = NULL;
	rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
	rsurface.batchtexcoordtexture2f_bufferoffset = 0;
	rsurface.batchtexcoordlightmap2f = NULL;
	rsurface.batchtexcoordlightmap2f_vertexbuffer = NULL;
	rsurface.batchtexcoordlightmap2f_bufferoffset = 0;
	rsurface.batchskeletalindex4ub = NULL;
	rsurface.batchskeletalindex4ub_vertexbuffer = NULL;
	rsurface.batchskeletalindex4ub_bufferoffset = 0;
	rsurface.batchskeletalweight4ub = NULL;
	rsurface.batchskeletalweight4ub_vertexbuffer = NULL;
	rsurface.batchskeletalweight4ub_bufferoffset = 0;
	rsurface.batchelement3i = NULL;
	rsurface.batchelement3i_indexbuffer = NULL;
	rsurface.batchelement3i_bufferoffset = 0;
	rsurface.batchelement3s = NULL;
	rsurface.batchelement3s_indexbuffer = NULL;
	rsurface.batchelement3s_bufferoffset = 0;
	rsurface.forcecurrenttextureupdate = true;

	if (rsurface.modelnumvertices && rsurface.modelelement3i)
	{
		if ((wantnormals || wanttangents) && !normal3f)
		{
			rsurface.modelnormal3f = (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
			Mod_BuildNormals(0, rsurface.modelnumvertices, rsurface.modelnumtriangles, rsurface.modelvertex3f, rsurface.modelelement3i, rsurface.modelnormal3f, r_smoothnormals_areaweighting.integer != 0);
		}
		if (wanttangents && !svector3f)
		{
			rsurface.modelsvector3f = (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
			rsurface.modeltvector3f = (float *)R_FrameData_Alloc(rsurface.modelnumvertices * sizeof(float[3]));
			Mod_BuildTextureVectorsFromNormals(0, rsurface.modelnumvertices, rsurface.modelnumtriangles, rsurface.modelvertex3f, rsurface.modeltexcoordtexture2f, rsurface.modelnormal3f, rsurface.modelelement3i, rsurface.modelsvector3f, rsurface.modeltvector3f, r_smoothnormals_areaweighting.integer != 0);
		}
	}
}

float RSurf_FogPoint(const float *v)
{
	// this code is identical to the USEFOGINSIDE/USEFOGOUTSIDE code in the shader
	float FogPlaneViewDist = r_refdef.fogplaneviewdist;
	float FogPlaneVertexDist = DotProduct(r_refdef.fogplane, v) + r_refdef.fogplane[3];
	float FogHeightFade = r_refdef.fogheightfade;
	float fogfrac;
	unsigned int fogmasktableindex;
	if (r_refdef.fogplaneviewabove)
		fogfrac = min(0.0f, FogPlaneVertexDist) / (FogPlaneVertexDist - FogPlaneViewDist) * min(1.0f, min(0.0f, FogPlaneVertexDist) * FogHeightFade);
	else
		fogfrac = FogPlaneViewDist / (FogPlaneViewDist - max(0.0f, FogPlaneVertexDist)) * min(1.0f, (min(0.0f, FogPlaneVertexDist) + FogPlaneViewDist) * FogHeightFade);
	fogmasktableindex = (unsigned int)(VectorDistance(r_refdef.view.origin, v) * fogfrac * r_refdef.fogmasktabledistmultiplier);
	return r_refdef.fogmasktable[min(fogmasktableindex, FOGMASKTABLEWIDTH - 1)];
}

float RSurf_FogVertex(const float *v)
{
	// this code is identical to the USEFOGINSIDE/USEFOGOUTSIDE code in the shader
	float FogPlaneViewDist = rsurface.fogplaneviewdist;
	float FogPlaneVertexDist = DotProduct(rsurface.fogplane, v) + rsurface.fogplane[3];
	float FogHeightFade = rsurface.fogheightfade;
	float fogfrac;
	unsigned int fogmasktableindex;
	if (r_refdef.fogplaneviewabove)
		fogfrac = min(0.0f, FogPlaneVertexDist) / (FogPlaneVertexDist - FogPlaneViewDist) * min(1.0f, min(0.0f, FogPlaneVertexDist) * FogHeightFade);
	else
		fogfrac = FogPlaneViewDist / (FogPlaneViewDist - max(0.0f, FogPlaneVertexDist)) * min(1.0f, (min(0.0f, FogPlaneVertexDist) + FogPlaneViewDist) * FogHeightFade);
	fogmasktableindex = (unsigned int)(VectorDistance(rsurface.localvieworigin, v) * fogfrac * rsurface.fogmasktabledistmultiplier);
	return r_refdef.fogmasktable[min(fogmasktableindex, FOGMASKTABLEWIDTH - 1)];
}

void RSurf_UploadBuffersForBatch(void)
{
	// upload buffer data for generated vertex data (dynamicvertex case) or index data (copytriangles case) and models that lack it to begin with (e.g. DrawQ_FlushUI)
	// note that if rsurface.batchvertex3f_vertexbuffer is NULL, dynamicvertex is forced as we don't account for the proper base vertex here.
	if (rsurface.batchvertex3f && !rsurface.batchvertex3f_vertexbuffer)
		rsurface.batchvertex3f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f, R_BUFFERDATA_VERTEX, &rsurface.batchvertex3f_bufferoffset);
	if (rsurface.batchsvector3f && !rsurface.batchsvector3f_vertexbuffer)
		rsurface.batchsvector3f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[3]), rsurface.batchsvector3f, R_BUFFERDATA_VERTEX, &rsurface.batchsvector3f_bufferoffset);
	if (rsurface.batchtvector3f && !rsurface.batchtvector3f_vertexbuffer)
		rsurface.batchtvector3f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[3]), rsurface.batchtvector3f, R_BUFFERDATA_VERTEX, &rsurface.batchtvector3f_bufferoffset);
	if (rsurface.batchnormal3f && !rsurface.batchnormal3f_vertexbuffer)
		rsurface.batchnormal3f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[3]), rsurface.batchnormal3f, R_BUFFERDATA_VERTEX, &rsurface.batchnormal3f_bufferoffset);
	if (rsurface.batchlightmapcolor4f && !rsurface.batchlightmapcolor4f_vertexbuffer)
		rsurface.batchlightmapcolor4f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[4]), rsurface.batchlightmapcolor4f, R_BUFFERDATA_VERTEX, &rsurface.batchlightmapcolor4f_bufferoffset);
	if (rsurface.batchtexcoordtexture2f && !rsurface.batchtexcoordtexture2f_vertexbuffer)
		rsurface.batchtexcoordtexture2f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[2]), rsurface.batchtexcoordtexture2f, R_BUFFERDATA_VERTEX, &rsurface.batchtexcoordtexture2f_bufferoffset);
	if (rsurface.batchtexcoordlightmap2f && !rsurface.batchtexcoordlightmap2f_vertexbuffer)
		rsurface.batchtexcoordlightmap2f_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(float[2]), rsurface.batchtexcoordlightmap2f, R_BUFFERDATA_VERTEX, &rsurface.batchtexcoordlightmap2f_bufferoffset);
	if (rsurface.batchskeletalindex4ub && !rsurface.batchskeletalindex4ub_vertexbuffer)
		rsurface.batchskeletalindex4ub_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(unsigned char[4]), rsurface.batchskeletalindex4ub, R_BUFFERDATA_VERTEX, &rsurface.batchskeletalindex4ub_bufferoffset);
	if (rsurface.batchskeletalweight4ub && !rsurface.batchskeletalweight4ub_vertexbuffer)
		rsurface.batchskeletalweight4ub_vertexbuffer = R_BufferData_Store(rsurface.batchnumvertices * sizeof(unsigned char[4]), rsurface.batchskeletalweight4ub, R_BUFFERDATA_VERTEX, &rsurface.batchskeletalweight4ub_bufferoffset);

	if (rsurface.batchelement3s && !rsurface.batchelement3s_indexbuffer)
		rsurface.batchelement3s_indexbuffer = R_BufferData_Store(rsurface.batchnumtriangles * sizeof(short[3]), rsurface.batchelement3s, R_BUFFERDATA_INDEX16, &rsurface.batchelement3s_bufferoffset);
	else if (rsurface.batchelement3i && !rsurface.batchelement3i_indexbuffer)
		rsurface.batchelement3i_indexbuffer = R_BufferData_Store(rsurface.batchnumtriangles * sizeof(int[3]), rsurface.batchelement3i, R_BUFFERDATA_INDEX32, &rsurface.batchelement3i_bufferoffset);

	R_Mesh_VertexPointer(     3, GL_FLOAT, sizeof(float[3]), rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
	R_Mesh_ColorPointer(      4, GL_FLOAT, sizeof(float[4]), rsurface.batchlightmapcolor4f, rsurface.batchlightmapcolor4f_vertexbuffer, rsurface.batchlightmapcolor4f_bufferoffset);
	R_Mesh_TexCoordPointer(0, 2, GL_FLOAT, sizeof(float[2]), rsurface.batchtexcoordtexture2f, rsurface.batchtexcoordtexture2f_vertexbuffer, rsurface.batchtexcoordtexture2f_bufferoffset);
	R_Mesh_TexCoordPointer(1, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchsvector3f, rsurface.batchsvector3f_vertexbuffer, rsurface.batchsvector3f_bufferoffset);
	R_Mesh_TexCoordPointer(2, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchtvector3f, rsurface.batchtvector3f_vertexbuffer, rsurface.batchtvector3f_bufferoffset);
	R_Mesh_TexCoordPointer(3, 3, GL_FLOAT, sizeof(float[3]), rsurface.batchnormal3f, rsurface.batchnormal3f_vertexbuffer, rsurface.batchnormal3f_bufferoffset);
	R_Mesh_TexCoordPointer(4, 2, GL_FLOAT, sizeof(float[2]), rsurface.batchtexcoordlightmap2f, rsurface.batchtexcoordlightmap2f_vertexbuffer, rsurface.batchtexcoordlightmap2f_bufferoffset);
	R_Mesh_TexCoordPointer(5, 2, GL_FLOAT, sizeof(float[2]), NULL, NULL, 0);
	R_Mesh_TexCoordPointer(6, 4, GL_UNSIGNED_BYTE | 0x80000000, sizeof(unsigned char[4]), rsurface.batchskeletalindex4ub, rsurface.batchskeletalindex4ub_vertexbuffer, rsurface.batchskeletalindex4ub_bufferoffset);
	R_Mesh_TexCoordPointer(7, 4, GL_UNSIGNED_BYTE, sizeof(unsigned char[4]), rsurface.batchskeletalweight4ub, rsurface.batchskeletalweight4ub_vertexbuffer, rsurface.batchskeletalweight4ub_bufferoffset);
}

static void RSurf_RenumberElements(const int *inelement3i, int *outelement3i, int numelements, int adjust)
{
	int i;
	for (i = 0;i < numelements;i++)
		outelement3i[i] = inelement3i[i] + adjust;
}

static const int quadedges[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
void RSurf_PrepareVerticesForBatch(int batchneed, int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	int deformindex;
	int firsttriangle;
	int numtriangles;
	int firstvertex;
	int endvertex;
	int numvertices;
	int surfacefirsttriangle;
	int surfacenumtriangles;
	int surfacefirstvertex;
	int surfaceendvertex;
	int surfacenumvertices;
	int batchnumsurfaces = texturenumsurfaces;
	int batchnumvertices;
	int batchnumtriangles;
	int i, j;
	qbool gaps;
	qbool dynamicvertex;
	float amplitude;
	float animpos;
	float center[3], forward[3], right[3], up[3], v[3], newforward[3], newright[3], newup[3];
	float waveparms[4];
	unsigned char *ub;
	q3shaderinfo_deform_t *deform;
	const msurface_t *surface, *firstsurface;
	if (!texturenumsurfaces)
		return;
	// find vertex range of this surface batch
	gaps = false;
	firstsurface = texturesurfacelist[0];
	firsttriangle = firstsurface->num_firsttriangle;
	batchnumvertices = 0;
	batchnumtriangles = 0;
	firstvertex = endvertex = firstsurface->num_firstvertex;
	for (i = 0;i < texturenumsurfaces;i++)
	{
		surface = texturesurfacelist[i];
		if (surface != firstsurface + i)
			gaps = true;
		surfacefirstvertex = surface->num_firstvertex;
		surfaceendvertex = surfacefirstvertex + surface->num_vertices;
		surfacenumvertices = surface->num_vertices;
		surfacenumtriangles = surface->num_triangles;
		if (firstvertex > surfacefirstvertex)
			firstvertex = surfacefirstvertex;
		if (endvertex < surfaceendvertex)
			endvertex = surfaceendvertex;
		batchnumvertices += surfacenumvertices;
		batchnumtriangles += surfacenumtriangles;
	}

	r_refdef.stats[r_stat_batch_batches]++;
	if (gaps)
		r_refdef.stats[r_stat_batch_withgaps]++;
	r_refdef.stats[r_stat_batch_surfaces] += batchnumsurfaces;
	r_refdef.stats[r_stat_batch_vertices] += batchnumvertices;
	r_refdef.stats[r_stat_batch_triangles] += batchnumtriangles;

	// we now know the vertex range used, and if there are any gaps in it
	rsurface.batchfirstvertex = firstvertex;
	rsurface.batchnumvertices = endvertex - firstvertex;
	rsurface.batchfirsttriangle = firsttriangle;
	rsurface.batchnumtriangles = batchnumtriangles;

	// check if any dynamic vertex processing must occur
	dynamicvertex = false;

	// we must use vertexbuffers for rendering, we can upload vertex buffers
	// easily enough but if the basevertex is non-zero it becomes more
	// difficult, so force dynamicvertex path in that case - it's suboptimal
	// but the most optimal case is to have the geometry sources provide their
	// own anyway.
	if (!rsurface.modelvertex3f_vertexbuffer && firstvertex != 0)
		dynamicvertex = true;

	// a cvar to force the dynamic vertex path to be taken, for debugging
	if (r_batch_debugdynamicvertexpath.integer)
	{
		if (!dynamicvertex)
		{
			r_refdef.stats[r_stat_batch_dynamic_batches_because_cvar] += 1;
			r_refdef.stats[r_stat_batch_dynamic_surfaces_because_cvar] += batchnumsurfaces;
			r_refdef.stats[r_stat_batch_dynamic_vertices_because_cvar] += batchnumvertices;
			r_refdef.stats[r_stat_batch_dynamic_triangles_because_cvar] += batchnumtriangles;
		}
		dynamicvertex = true;
	}

	// if there is a chance of animated vertex colors, it's a dynamic batch
	if ((batchneed & BATCHNEED_ARRAY_VERTEXCOLOR) && texturesurfacelist[0]->lightmapinfo)
	{
		if (!dynamicvertex)
		{
			r_refdef.stats[r_stat_batch_dynamic_batches_because_lightmapvertex] += 1;
			r_refdef.stats[r_stat_batch_dynamic_surfaces_because_lightmapvertex] += batchnumsurfaces;
			r_refdef.stats[r_stat_batch_dynamic_vertices_because_lightmapvertex] += batchnumvertices;
			r_refdef.stats[r_stat_batch_dynamic_triangles_because_lightmapvertex] += batchnumtriangles;
		}
		dynamicvertex = true;
	}

	for (deformindex = 0, deform = rsurface.texture->deforms;deformindex < Q3MAXDEFORMS && deform->deform && r_deformvertexes.integer;deformindex++, deform++)
	{
		switch (deform->deform)
		{
		default:
		case Q3DEFORM_PROJECTIONSHADOW:
		case Q3DEFORM_TEXT0:
		case Q3DEFORM_TEXT1:
		case Q3DEFORM_TEXT2:
		case Q3DEFORM_TEXT3:
		case Q3DEFORM_TEXT4:
		case Q3DEFORM_TEXT5:
		case Q3DEFORM_TEXT6:
		case Q3DEFORM_TEXT7:
		case Q3DEFORM_NONE:
			break;
		case Q3DEFORM_AUTOSPRITE:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_autosprite] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_autosprite] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_autosprite] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_autosprite] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR | BATCHNEED_ARRAY_TEXCOORD;
			break;
		case Q3DEFORM_AUTOSPRITE2:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_autosprite2] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_autosprite2] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_autosprite2] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_autosprite2] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_TEXCOORD;
			break;
		case Q3DEFORM_NORMAL:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_normal] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_normal] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_normal] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_normal] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_TEXCOORD;
			break;
		case Q3DEFORM_WAVE:
			if(!R_TestQ3WaveFunc(deform->wavefunc, deform->waveparms))
				break; // if wavefunc is a nop, ignore this transform
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_wave] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_wave] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_wave] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_wave] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_TEXCOORD;
			break;
		case Q3DEFORM_BULGE:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_bulge] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_bulge] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_bulge] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_bulge] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_TEXCOORD;
			break;
		case Q3DEFORM_MOVE:
			if(!R_TestQ3WaveFunc(deform->wavefunc, deform->waveparms))
				break; // if wavefunc is a nop, ignore this transform
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_deformvertexes_move] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_deformvertexes_move] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_deformvertexes_move] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_deformvertexes_move] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX;
			break;
		}
	}
	if (rsurface.texture->materialshaderpass)
	{
		switch (rsurface.texture->materialshaderpass->tcgen.tcgen)
		{
		default:
		case Q3TCGEN_TEXTURE:
			break;
		case Q3TCGEN_LIGHTMAP:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_tcgen_lightmap] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_tcgen_lightmap] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_tcgen_lightmap] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_tcgen_lightmap] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_LIGHTMAP;
			break;
		case Q3TCGEN_VECTOR:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_tcgen_vector] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_tcgen_vector] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_tcgen_vector] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_tcgen_vector] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX;
			break;
		case Q3TCGEN_ENVIRONMENT:
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_tcgen_environment] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_tcgen_environment] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_tcgen_environment] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_tcgen_environment] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL;
			break;
		}
		if (rsurface.texture->materialshaderpass->tcmods[0].tcmod == Q3TCMOD_TURBULENT)
		{
			if (!dynamicvertex)
			{
				r_refdef.stats[r_stat_batch_dynamic_batches_because_tcmod_turbulent] += 1;
				r_refdef.stats[r_stat_batch_dynamic_surfaces_because_tcmod_turbulent] += batchnumsurfaces;
				r_refdef.stats[r_stat_batch_dynamic_vertices_because_tcmod_turbulent] += batchnumvertices;
				r_refdef.stats[r_stat_batch_dynamic_triangles_because_tcmod_turbulent] += batchnumtriangles;
			}
			dynamicvertex = true;
			batchneed |= BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_TEXCOORD;
		}
	}

	// the caller can specify BATCHNEED_NOGAPS to force a batch with
	// firstvertex = 0 and endvertex = numvertices (no gaps, no firstvertex),
	// we ensure this by treating the vertex batch as dynamic...
	if ((batchneed & BATCHNEED_ALWAYSCOPY) || ((batchneed & BATCHNEED_NOGAPS) && (gaps || firstvertex > 0)))
	{
		if (!dynamicvertex)
		{
			r_refdef.stats[r_stat_batch_dynamic_batches_because_nogaps] += 1;
			r_refdef.stats[r_stat_batch_dynamic_surfaces_because_nogaps] += batchnumsurfaces;
			r_refdef.stats[r_stat_batch_dynamic_vertices_because_nogaps] += batchnumvertices;
			r_refdef.stats[r_stat_batch_dynamic_triangles_because_nogaps] += batchnumtriangles;
		}
		dynamicvertex = true;
	}

	// if we're going to have to apply the skeletal transform manually, we need to batch the skeletal data
	if (dynamicvertex && rsurface.entityskeletaltransform3x4)
		batchneed |= BATCHNEED_ARRAY_SKELETAL;

	rsurface.batchvertex3f = rsurface.modelvertex3f;
	rsurface.batchvertex3f_vertexbuffer = rsurface.modelvertex3f_vertexbuffer;
	rsurface.batchvertex3f_bufferoffset = rsurface.modelvertex3f_bufferoffset;
	rsurface.batchsvector3f = rsurface.modelsvector3f;
	rsurface.batchsvector3f_vertexbuffer = rsurface.modelsvector3f_vertexbuffer;
	rsurface.batchsvector3f_bufferoffset = rsurface.modelsvector3f_bufferoffset;
	rsurface.batchtvector3f = rsurface.modeltvector3f;
	rsurface.batchtvector3f_vertexbuffer = rsurface.modeltvector3f_vertexbuffer;
	rsurface.batchtvector3f_bufferoffset = rsurface.modeltvector3f_bufferoffset;
	rsurface.batchnormal3f = rsurface.modelnormal3f;
	rsurface.batchnormal3f_vertexbuffer = rsurface.modelnormal3f_vertexbuffer;
	rsurface.batchnormal3f_bufferoffset = rsurface.modelnormal3f_bufferoffset;
	rsurface.batchlightmapcolor4f = rsurface.modellightmapcolor4f;
	rsurface.batchlightmapcolor4f_vertexbuffer  = rsurface.modellightmapcolor4f_vertexbuffer;
	rsurface.batchlightmapcolor4f_bufferoffset  = rsurface.modellightmapcolor4f_bufferoffset;
	rsurface.batchtexcoordtexture2f = rsurface.modeltexcoordtexture2f;
	rsurface.batchtexcoordtexture2f_vertexbuffer  = rsurface.modeltexcoordtexture2f_vertexbuffer;
	rsurface.batchtexcoordtexture2f_bufferoffset  = rsurface.modeltexcoordtexture2f_bufferoffset;
	rsurface.batchtexcoordlightmap2f = rsurface.modeltexcoordlightmap2f;
	rsurface.batchtexcoordlightmap2f_vertexbuffer = rsurface.modeltexcoordlightmap2f_vertexbuffer;
	rsurface.batchtexcoordlightmap2f_bufferoffset = rsurface.modeltexcoordlightmap2f_bufferoffset;
	rsurface.batchskeletalindex4ub = rsurface.modelskeletalindex4ub;
	rsurface.batchskeletalindex4ub_vertexbuffer = rsurface.modelskeletalindex4ub_vertexbuffer;
	rsurface.batchskeletalindex4ub_bufferoffset = rsurface.modelskeletalindex4ub_bufferoffset;
	rsurface.batchskeletalweight4ub = rsurface.modelskeletalweight4ub;
	rsurface.batchskeletalweight4ub_vertexbuffer = rsurface.modelskeletalweight4ub_vertexbuffer;
	rsurface.batchskeletalweight4ub_bufferoffset = rsurface.modelskeletalweight4ub_bufferoffset;
	rsurface.batchelement3i = rsurface.modelelement3i;
	rsurface.batchelement3i_indexbuffer = rsurface.modelelement3i_indexbuffer;
	rsurface.batchelement3i_bufferoffset = rsurface.modelelement3i_bufferoffset;
	rsurface.batchelement3s = rsurface.modelelement3s;
	rsurface.batchelement3s_indexbuffer = rsurface.modelelement3s_indexbuffer;
	rsurface.batchelement3s_bufferoffset = rsurface.modelelement3s_bufferoffset;
	rsurface.batchskeletaltransform3x4 = rsurface.entityskeletaltransform3x4;
	rsurface.batchskeletaltransform3x4buffer = rsurface.entityskeletaltransform3x4buffer;
	rsurface.batchskeletaltransform3x4offset = rsurface.entityskeletaltransform3x4offset;
	rsurface.batchskeletaltransform3x4size = rsurface.entityskeletaltransform3x4size;
	rsurface.batchskeletalnumtransforms = rsurface.entityskeletalnumtransforms;

	// if any dynamic vertex processing has to occur in software, we copy the
	// entire surface list together before processing to rebase the vertices
	// to start at 0 (otherwise we waste a lot of room in a vertex buffer).
	//
	// if any gaps exist and we do not have a static vertex buffer, we have to
	// copy the surface list together to avoid wasting upload bandwidth on the
	// vertices in the gaps.
	//
	// if gaps exist and we have a static vertex buffer, we can choose whether
	// to combine the index buffer ranges into one dynamic index buffer or
	// simply issue multiple glDrawElements calls (BATCHNEED_ALLOWMULTIDRAW).
	//
	// in many cases the batch is reduced to one draw call.

	rsurface.batchmultidraw = false;
	rsurface.batchmultidrawnumsurfaces = 0;
	rsurface.batchmultidrawsurfacelist = NULL;

	if (!dynamicvertex)
	{
		// static vertex data, just set pointers...
		rsurface.batchgeneratedvertex = false;
		// if there are gaps, we want to build a combined index buffer,
		// otherwise use the original static buffer with an appropriate offset
		if (gaps)
		{
			r_refdef.stats[r_stat_batch_copytriangles_batches] += 1;
			r_refdef.stats[r_stat_batch_copytriangles_surfaces] += batchnumsurfaces;
			r_refdef.stats[r_stat_batch_copytriangles_vertices] += batchnumvertices;
			r_refdef.stats[r_stat_batch_copytriangles_triangles] += batchnumtriangles;
			if ((batchneed & BATCHNEED_ALLOWMULTIDRAW) && r_batch_multidraw.integer && batchnumtriangles >= r_batch_multidraw_mintriangles.integer)
			{
				rsurface.batchmultidraw = true;
				rsurface.batchmultidrawnumsurfaces = texturenumsurfaces;
				rsurface.batchmultidrawsurfacelist = texturesurfacelist;
				return;
			}
			// build a new triangle elements array for this batch
			rsurface.batchelement3i = (int *)R_FrameData_Alloc(batchnumtriangles * sizeof(int[3]));
			rsurface.batchfirsttriangle = 0;
			numtriangles = 0;
			for (i = 0;i < texturenumsurfaces;i++)
			{
				surfacefirsttriangle = texturesurfacelist[i]->num_firsttriangle;
				surfacenumtriangles = texturesurfacelist[i]->num_triangles;
				memcpy(rsurface.batchelement3i + 3*numtriangles, rsurface.modelelement3i + 3*surfacefirsttriangle, surfacenumtriangles*sizeof(int[3]));
				numtriangles += surfacenumtriangles;
			}
			rsurface.batchelement3i_indexbuffer = NULL;
			rsurface.batchelement3i_bufferoffset = 0;
			rsurface.batchelement3s = NULL;
			rsurface.batchelement3s_indexbuffer = NULL;
			rsurface.batchelement3s_bufferoffset = 0;
			if (endvertex <= 65536)
			{
				// make a 16bit (unsigned short) index array if possible
				rsurface.batchelement3s = (unsigned short *)R_FrameData_Alloc(batchnumtriangles * sizeof(unsigned short[3]));
				for (i = 0;i < numtriangles*3;i++)
					rsurface.batchelement3s[i] = rsurface.batchelement3i[i];
			}
		}
		else
		{
			r_refdef.stats[r_stat_batch_fast_batches] += 1;
			r_refdef.stats[r_stat_batch_fast_surfaces] += batchnumsurfaces;
			r_refdef.stats[r_stat_batch_fast_vertices] += batchnumvertices;
			r_refdef.stats[r_stat_batch_fast_triangles] += batchnumtriangles;
		}
		return;
	}

	// something needs software processing, do it for real...
	// we only directly handle separate array data in this case and then
	// generate interleaved data if needed...
	rsurface.batchgeneratedvertex = true;
	r_refdef.stats[r_stat_batch_dynamic_batches] += 1;
	r_refdef.stats[r_stat_batch_dynamic_surfaces] += batchnumsurfaces;
	r_refdef.stats[r_stat_batch_dynamic_vertices] += batchnumvertices;
	r_refdef.stats[r_stat_batch_dynamic_triangles] += batchnumtriangles;

	// now copy the vertex data into a combined array and make an index array
	// (this is what Quake3 does all the time)
	// we also apply any skeletal animation here that would have been done in
	// the vertex shader, because most of the dynamic vertex animation cases
	// need actual vertex positions and normals
	//if (dynamicvertex)
	{
		rsurface.batchvertex3f = NULL;
		rsurface.batchvertex3f_vertexbuffer = NULL;
		rsurface.batchvertex3f_bufferoffset = 0;
		rsurface.batchsvector3f = NULL;
		rsurface.batchsvector3f_vertexbuffer = NULL;
		rsurface.batchsvector3f_bufferoffset = 0;
		rsurface.batchtvector3f = NULL;
		rsurface.batchtvector3f_vertexbuffer = NULL;
		rsurface.batchtvector3f_bufferoffset = 0;
		rsurface.batchnormal3f = NULL;
		rsurface.batchnormal3f_vertexbuffer = NULL;
		rsurface.batchnormal3f_bufferoffset = 0;
		rsurface.batchlightmapcolor4f = NULL;
		rsurface.batchlightmapcolor4f_vertexbuffer = NULL;
		rsurface.batchlightmapcolor4f_bufferoffset = 0;
		rsurface.batchtexcoordtexture2f = NULL;
		rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
		rsurface.batchtexcoordtexture2f_bufferoffset = 0;
		rsurface.batchtexcoordlightmap2f = NULL;
		rsurface.batchtexcoordlightmap2f_vertexbuffer = NULL;
		rsurface.batchtexcoordlightmap2f_bufferoffset = 0;
		rsurface.batchskeletalindex4ub = NULL;
		rsurface.batchskeletalindex4ub_vertexbuffer = NULL;
		rsurface.batchskeletalindex4ub_bufferoffset = 0;
		rsurface.batchskeletalweight4ub = NULL;
		rsurface.batchskeletalweight4ub_vertexbuffer = NULL;
		rsurface.batchskeletalweight4ub_bufferoffset = 0;
		rsurface.batchelement3i = (int *)R_FrameData_Alloc(batchnumtriangles * sizeof(int[3]));
		rsurface.batchelement3i_indexbuffer = NULL;
		rsurface.batchelement3i_bufferoffset = 0;
		rsurface.batchelement3s = NULL;
		rsurface.batchelement3s_indexbuffer = NULL;
		rsurface.batchelement3s_bufferoffset = 0;
		rsurface.batchskeletaltransform3x4buffer = NULL;
		rsurface.batchskeletaltransform3x4offset = 0;
		rsurface.batchskeletaltransform3x4size = 0;
		// we'll only be setting up certain arrays as needed
		if (batchneed & BATCHNEED_ARRAY_VERTEX)
			rsurface.batchvertex3f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
		if (batchneed & BATCHNEED_ARRAY_NORMAL)
			rsurface.batchnormal3f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
		if (batchneed & BATCHNEED_ARRAY_VECTOR)
		{
			rsurface.batchsvector3f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
			rsurface.batchtvector3f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
		}
		if (batchneed & BATCHNEED_ARRAY_VERTEXCOLOR)
			rsurface.batchlightmapcolor4f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[4]));
		if (batchneed & BATCHNEED_ARRAY_TEXCOORD)
			rsurface.batchtexcoordtexture2f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
		if (batchneed & BATCHNEED_ARRAY_LIGHTMAP)
			rsurface.batchtexcoordlightmap2f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
		if (batchneed & BATCHNEED_ARRAY_SKELETAL)
		{
			rsurface.batchskeletalindex4ub = (unsigned char *)R_FrameData_Alloc(batchnumvertices * sizeof(unsigned char[4]));
			rsurface.batchskeletalweight4ub = (unsigned char *)R_FrameData_Alloc(batchnumvertices * sizeof(unsigned char[4]));
		}
		numvertices = 0;
		numtriangles = 0;
		for (i = 0;i < texturenumsurfaces;i++)
		{
			surfacefirstvertex = texturesurfacelist[i]->num_firstvertex;
			surfacenumvertices = texturesurfacelist[i]->num_vertices;
			surfacefirsttriangle = texturesurfacelist[i]->num_firsttriangle;
			surfacenumtriangles = texturesurfacelist[i]->num_triangles;
			// copy only the data requested
			if (batchneed & (BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR | BATCHNEED_ARRAY_VERTEXCOLOR | BATCHNEED_ARRAY_TEXCOORD | BATCHNEED_ARRAY_LIGHTMAP))
			{
				if (batchneed & BATCHNEED_ARRAY_VERTEX)
				{
					if (rsurface.batchvertex3f)
						memcpy(rsurface.batchvertex3f + 3*numvertices, rsurface.modelvertex3f + 3*surfacefirstvertex, surfacenumvertices * sizeof(float[3]));
					else
						memset(rsurface.batchvertex3f + 3*numvertices, 0, surfacenumvertices * sizeof(float[3]));
				}
				if (batchneed & BATCHNEED_ARRAY_NORMAL)
				{
					if (rsurface.modelnormal3f)
						memcpy(rsurface.batchnormal3f + 3*numvertices, rsurface.modelnormal3f + 3*surfacefirstvertex, surfacenumvertices * sizeof(float[3]));
					else
						memset(rsurface.batchnormal3f + 3*numvertices, 0, surfacenumvertices * sizeof(float[3]));
				}
				if (batchneed & BATCHNEED_ARRAY_VECTOR)
				{
					if (rsurface.modelsvector3f)
					{
						memcpy(rsurface.batchsvector3f + 3*numvertices, rsurface.modelsvector3f + 3*surfacefirstvertex, surfacenumvertices * sizeof(float[3]));
						memcpy(rsurface.batchtvector3f + 3*numvertices, rsurface.modeltvector3f + 3*surfacefirstvertex, surfacenumvertices * sizeof(float[3]));
					}
					else
					{
						memset(rsurface.batchsvector3f + 3*numvertices, 0, surfacenumvertices * sizeof(float[3]));
						memset(rsurface.batchtvector3f + 3*numvertices, 0, surfacenumvertices * sizeof(float[3]));
					}
				}
				if (batchneed & BATCHNEED_ARRAY_VERTEXCOLOR)
				{
					if (rsurface.modellightmapcolor4f)
						memcpy(rsurface.batchlightmapcolor4f + 4*numvertices, rsurface.modellightmapcolor4f + 4*surfacefirstvertex, surfacenumvertices * sizeof(float[4]));
					else
						memset(rsurface.batchlightmapcolor4f + 4*numvertices, 0, surfacenumvertices * sizeof(float[4]));
				}
				if (batchneed & BATCHNEED_ARRAY_TEXCOORD)
				{
					if (rsurface.modeltexcoordtexture2f)
						memcpy(rsurface.batchtexcoordtexture2f + 2*numvertices, rsurface.modeltexcoordtexture2f + 2*surfacefirstvertex, surfacenumvertices * sizeof(float[2]));
					else
						memset(rsurface.batchtexcoordtexture2f + 2*numvertices, 0, surfacenumvertices * sizeof(float[2]));
				}
				if (batchneed & BATCHNEED_ARRAY_LIGHTMAP)
				{
					if (rsurface.modeltexcoordlightmap2f)
						memcpy(rsurface.batchtexcoordlightmap2f + 2*numvertices, rsurface.modeltexcoordlightmap2f + 2*surfacefirstvertex, surfacenumvertices * sizeof(float[2]));
					else
						memset(rsurface.batchtexcoordlightmap2f + 2*numvertices, 0, surfacenumvertices * sizeof(float[2]));
				}
				if (batchneed & BATCHNEED_ARRAY_SKELETAL)
				{
					if (rsurface.modelskeletalindex4ub)
					{
						memcpy(rsurface.batchskeletalindex4ub + 4*numvertices, rsurface.modelskeletalindex4ub + 4*surfacefirstvertex, surfacenumvertices * sizeof(unsigned char[4]));
						memcpy(rsurface.batchskeletalweight4ub + 4*numvertices, rsurface.modelskeletalweight4ub + 4*surfacefirstvertex, surfacenumvertices * sizeof(unsigned char[4]));
					}
					else
					{
						memset(rsurface.batchskeletalindex4ub + 4*numvertices, 0, surfacenumvertices * sizeof(unsigned char[4]));
						memset(rsurface.batchskeletalweight4ub + 4*numvertices, 0, surfacenumvertices * sizeof(unsigned char[4]));
						ub = rsurface.batchskeletalweight4ub + 4*numvertices;
						for (j = 0;j < surfacenumvertices;j++)
							ub[j*4] = 255;
					}
				}
			}
			RSurf_RenumberElements(rsurface.modelelement3i + 3*surfacefirsttriangle, rsurface.batchelement3i + 3*numtriangles, 3*surfacenumtriangles, numvertices - surfacefirstvertex);
			numvertices += surfacenumvertices;
			numtriangles += surfacenumtriangles;
		}

		// generate a 16bit index array as well if possible
		// (in general, dynamic batches fit)
		if (numvertices <= 65536)
		{
			rsurface.batchelement3s = (unsigned short *)R_FrameData_Alloc(batchnumtriangles * sizeof(unsigned short[3]));
			for (i = 0;i < numtriangles*3;i++)
				rsurface.batchelement3s[i] = rsurface.batchelement3i[i];
		}

		// since we've copied everything, the batch now starts at 0
		rsurface.batchfirstvertex = 0;
		rsurface.batchnumvertices = batchnumvertices;
		rsurface.batchfirsttriangle = 0;
		rsurface.batchnumtriangles = batchnumtriangles;
	}

	// apply skeletal animation that would have been done in the vertex shader
	if (rsurface.batchskeletaltransform3x4)
	{
		const unsigned char *si;
		const unsigned char *sw;
		const float *t[4];
		const float *b = rsurface.batchskeletaltransform3x4;
		float *vp, *vs, *vt, *vn;
		float w[4];
		float m[3][4], n[3][4];
		float tp[3], ts[3], tt[3], tn[3];
		r_refdef.stats[r_stat_batch_dynamicskeletal_batches] += 1;
		r_refdef.stats[r_stat_batch_dynamicskeletal_surfaces] += batchnumsurfaces;
		r_refdef.stats[r_stat_batch_dynamicskeletal_vertices] += batchnumvertices;
		r_refdef.stats[r_stat_batch_dynamicskeletal_triangles] += batchnumtriangles;
		si = rsurface.batchskeletalindex4ub;
		sw = rsurface.batchskeletalweight4ub;
		vp = rsurface.batchvertex3f;
		vs = rsurface.batchsvector3f;
		vt = rsurface.batchtvector3f;
		vn = rsurface.batchnormal3f;
		memset(m[0], 0, sizeof(m));
		memset(n[0], 0, sizeof(n));
		for (i = 0;i < batchnumvertices;i++)
		{
			t[0] = b + si[0]*12;
			if (sw[0] == 255)
			{
				// common case - only one matrix
				m[0][0] = t[0][ 0];
				m[0][1] = t[0][ 1];
				m[0][2] = t[0][ 2];
				m[0][3] = t[0][ 3];
				m[1][0] = t[0][ 4];
				m[1][1] = t[0][ 5];
				m[1][2] = t[0][ 6];
				m[1][3] = t[0][ 7];
				m[2][0] = t[0][ 8];
				m[2][1] = t[0][ 9];
				m[2][2] = t[0][10];
				m[2][3] = t[0][11];
			}
			else if (sw[2] + sw[3])
			{
				// blend 4 matrices
				t[1] = b + si[1]*12;
				t[2] = b + si[2]*12;
				t[3] = b + si[3]*12;
				w[0] = sw[0] * (1.0f / 255.0f);
				w[1] = sw[1] * (1.0f / 255.0f);
				w[2] = sw[2] * (1.0f / 255.0f);
				w[3] = sw[3] * (1.0f / 255.0f);
				// blend the matrices
				m[0][0] = t[0][ 0] * w[0] + t[1][ 0] * w[1] + t[2][ 0] * w[2] + t[3][ 0] * w[3];
				m[0][1] = t[0][ 1] * w[0] + t[1][ 1] * w[1] + t[2][ 1] * w[2] + t[3][ 1] * w[3];
				m[0][2] = t[0][ 2] * w[0] + t[1][ 2] * w[1] + t[2][ 2] * w[2] + t[3][ 2] * w[3];
				m[0][3] = t[0][ 3] * w[0] + t[1][ 3] * w[1] + t[2][ 3] * w[2] + t[3][ 3] * w[3];
				m[1][0] = t[0][ 4] * w[0] + t[1][ 4] * w[1] + t[2][ 4] * w[2] + t[3][ 4] * w[3];
				m[1][1] = t[0][ 5] * w[0] + t[1][ 5] * w[1] + t[2][ 5] * w[2] + t[3][ 5] * w[3];
				m[1][2] = t[0][ 6] * w[0] + t[1][ 6] * w[1] + t[2][ 6] * w[2] + t[3][ 6] * w[3];
				m[1][3] = t[0][ 7] * w[0] + t[1][ 7] * w[1] + t[2][ 7] * w[2] + t[3][ 7] * w[3];
				m[2][0] = t[0][ 8] * w[0] + t[1][ 8] * w[1] + t[2][ 8] * w[2] + t[3][ 8] * w[3];
				m[2][1] = t[0][ 9] * w[0] + t[1][ 9] * w[1] + t[2][ 9] * w[2] + t[3][ 9] * w[3];
				m[2][2] = t[0][10] * w[0] + t[1][10] * w[1] + t[2][10] * w[2] + t[3][10] * w[3];
				m[2][3] = t[0][11] * w[0] + t[1][11] * w[1] + t[2][11] * w[2] + t[3][11] * w[3];
			}
			else
			{
				// blend 2 matrices
				t[1] = b + si[1]*12;
				w[0] = sw[0] * (1.0f / 255.0f);
				w[1] = sw[1] * (1.0f / 255.0f);
				// blend the matrices
				m[0][0] = t[0][ 0] * w[0] + t[1][ 0] * w[1];
				m[0][1] = t[0][ 1] * w[0] + t[1][ 1] * w[1];
				m[0][2] = t[0][ 2] * w[0] + t[1][ 2] * w[1];
				m[0][3] = t[0][ 3] * w[0] + t[1][ 3] * w[1];
				m[1][0] = t[0][ 4] * w[0] + t[1][ 4] * w[1];
				m[1][1] = t[0][ 5] * w[0] + t[1][ 5] * w[1];
				m[1][2] = t[0][ 6] * w[0] + t[1][ 6] * w[1];
				m[1][3] = t[0][ 7] * w[0] + t[1][ 7] * w[1];
				m[2][0] = t[0][ 8] * w[0] + t[1][ 8] * w[1];
				m[2][1] = t[0][ 9] * w[0] + t[1][ 9] * w[1];
				m[2][2] = t[0][10] * w[0] + t[1][10] * w[1];
				m[2][3] = t[0][11] * w[0] + t[1][11] * w[1];
			}
			si += 4;
			sw += 4;
			// modify the vertex
			VectorCopy(vp, tp);
			vp[0] = tp[0] * m[0][0] + tp[1] * m[0][1] + tp[2] * m[0][2] + m[0][3];
			vp[1] = tp[0] * m[1][0] + tp[1] * m[1][1] + tp[2] * m[1][2] + m[1][3];
			vp[2] = tp[0] * m[2][0] + tp[1] * m[2][1] + tp[2] * m[2][2] + m[2][3];
			vp += 3;
			if (vn)
			{
				// the normal transformation matrix is a set of cross products...
				CrossProduct(m[1], m[2], n[0]);
				CrossProduct(m[2], m[0], n[1]);
				CrossProduct(m[0], m[1], n[2]); // is actually transpose(inverse(m)) * det(m)
				VectorCopy(vn, tn);
				vn[0] = tn[0] * n[0][0] + tn[1] * n[0][1] + tn[2] * n[0][2];
				vn[1] = tn[0] * n[1][0] + tn[1] * n[1][1] + tn[2] * n[1][2];
				vn[2] = tn[0] * n[2][0] + tn[1] * n[2][1] + tn[2] * n[2][2];
				VectorNormalize(vn);
				vn += 3;
				if (vs)
				{
					VectorCopy(vs, ts);
					vs[0] = ts[0] * n[0][0] + ts[1] * n[0][1] + ts[2] * n[0][2];
					vs[1] = ts[0] * n[1][0] + ts[1] * n[1][1] + ts[2] * n[1][2];
					vs[2] = ts[0] * n[2][0] + ts[1] * n[2][1] + ts[2] * n[2][2];
					VectorNormalize(vs);
					vs += 3;
					VectorCopy(vt, tt);
					vt[0] = tt[0] * n[0][0] + tt[1] * n[0][1] + tt[2] * n[0][2];
					vt[1] = tt[0] * n[1][0] + tt[1] * n[1][1] + tt[2] * n[1][2];
					vt[2] = tt[0] * n[2][0] + tt[1] * n[2][1] + tt[2] * n[2][2];
					VectorNormalize(vt);
					vt += 3;
				}
			}
		}
		rsurface.batchskeletaltransform3x4 = NULL;
		rsurface.batchskeletalnumtransforms = 0;
	}

	// q1bsp surfaces rendered in vertex color mode have to have colors
	// calculated based on lightstyles
	if ((batchneed & BATCHNEED_ARRAY_VERTEXCOLOR) && texturesurfacelist[0]->lightmapinfo)
	{
		// generate color arrays for the surfaces in this list
		int c[4];
		int scale;
		int size3;
		const int *offsets;
		const unsigned char *lm;
		rsurface.batchlightmapcolor4f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[4]));
		rsurface.batchlightmapcolor4f_vertexbuffer = NULL;
		rsurface.batchlightmapcolor4f_bufferoffset = 0;
		numvertices = 0;
		for (i = 0;i < texturenumsurfaces;i++)
		{
			surface = texturesurfacelist[i];
			offsets = rsurface.modellightmapoffsets + surface->num_firstvertex;
			surfacenumvertices = surface->num_vertices;
			if (surface->lightmapinfo->samples)
			{
				for (j = 0;j < surfacenumvertices;j++)
				{
					lm = surface->lightmapinfo->samples + offsets[j];
					scale = r_refdef.scene.lightstylevalue[surface->lightmapinfo->styles[0]];
					VectorScale(lm, scale, c);
					if (surface->lightmapinfo->styles[1] != 255)
					{
						size3 = ((surface->lightmapinfo->extents[0]>>4)+1)*((surface->lightmapinfo->extents[1]>>4)+1)*3;
						lm += size3;
						scale = r_refdef.scene.lightstylevalue[surface->lightmapinfo->styles[1]];
						VectorMA(c, scale, lm, c);
						if (surface->lightmapinfo->styles[2] != 255)
						{
							lm += size3;
							scale = r_refdef.scene.lightstylevalue[surface->lightmapinfo->styles[2]];
							VectorMA(c, scale, lm, c);
							if (surface->lightmapinfo->styles[3] != 255)
							{
								lm += size3;
								scale = r_refdef.scene.lightstylevalue[surface->lightmapinfo->styles[3]];
								VectorMA(c, scale, lm, c);
							}
						}
					}
					c[0] >>= 7;
					c[1] >>= 7;
					c[2] >>= 7;
					Vector4Set(rsurface.batchlightmapcolor4f + 4*numvertices, min(c[0], 255) * (1.0f / 255.0f), min(c[1], 255) * (1.0f / 255.0f), min(c[2], 255) * (1.0f / 255.0f), 1);
					numvertices++;
				}
			}
			else
			{
				for (j = 0;j < surfacenumvertices;j++)
				{
					Vector4Set(rsurface.batchlightmapcolor4f + 4*numvertices, 0, 0, 0, 1);
					numvertices++;
				}
			}
		}
	}

	// if vertices are deformed (sprite flares and things in maps, possibly
	// water waves, bulges and other deformations), modify the copied vertices
	// in place
	for (deformindex = 0, deform = rsurface.texture->deforms;deformindex < Q3MAXDEFORMS && deform->deform && r_deformvertexes.integer;deformindex++, deform++)
	{
		float scale;
		switch (deform->deform)
		{
		default:
		case Q3DEFORM_PROJECTIONSHADOW:
		case Q3DEFORM_TEXT0:
		case Q3DEFORM_TEXT1:
		case Q3DEFORM_TEXT2:
		case Q3DEFORM_TEXT3:
		case Q3DEFORM_TEXT4:
		case Q3DEFORM_TEXT5:
		case Q3DEFORM_TEXT6:
		case Q3DEFORM_TEXT7:
		case Q3DEFORM_NONE:
			break;
		case Q3DEFORM_AUTOSPRITE:
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.forward, newforward);
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.right, newright);
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.up, newup);
			VectorNormalize(newforward);
			VectorNormalize(newright);
			VectorNormalize(newup);
//			rsurface.batchvertex3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f);
//			rsurface.batchvertex3f_vertexbuffer = NULL;
//			rsurface.batchvertex3f_bufferoffset = 0;
//			rsurface.batchsvector3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchsvector3f);
//			rsurface.batchsvector3f_vertexbuffer = NULL;
//			rsurface.batchsvector3f_bufferoffset = 0;
//			rsurface.batchtvector3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchtvector3f);
//			rsurface.batchtvector3f_vertexbuffer = NULL;
//			rsurface.batchtvector3f_bufferoffset = 0;
//			rsurface.batchnormal3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchnormal3f);
//			rsurface.batchnormal3f_vertexbuffer = NULL;
//			rsurface.batchnormal3f_bufferoffset = 0;
			// sometimes we're on a renderpath that does not use vectors (GL11/GL13/GLES1)
			if (!VectorLength2(rsurface.batchnormal3f + 3*rsurface.batchfirstvertex))
				Mod_BuildNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchnormal3f, r_smoothnormals_areaweighting.integer != 0);
			if (!VectorLength2(rsurface.batchsvector3f + 3*rsurface.batchfirstvertex))
				Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			// a single autosprite surface can contain multiple sprites...
			for (j = 0;j < batchnumvertices - 3;j += 4)
			{
				VectorClear(center);
				for (i = 0;i < 4;i++)
					VectorAdd(center, rsurface.batchvertex3f + 3*(j+i), center);
				VectorScale(center, 0.25f, center);
				VectorCopy(rsurface.batchnormal3f + 3*j, forward);
				VectorCopy(rsurface.batchsvector3f + 3*j, right);
				VectorCopy(rsurface.batchtvector3f + 3*j, up);
				for (i = 0;i < 4;i++)
				{
					VectorSubtract(rsurface.batchvertex3f + 3*(j+i), center, v);
					VectorMAMAMAM(1, center, DotProduct(forward, v), newforward, DotProduct(right, v), newright, DotProduct(up, v), newup, rsurface.batchvertex3f + 3*(j+i));
				}
			}
			// if we get here, BATCHNEED_ARRAY_NORMAL and BATCHNEED_ARRAY_VECTOR are in batchneed, so no need to check
			Mod_BuildNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchnormal3f, r_smoothnormals_areaweighting.integer != 0);
			Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			break;
		case Q3DEFORM_AUTOSPRITE2:
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.forward, newforward);
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.right, newright);
			Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.up, newup);
			VectorNormalize(newforward);
			VectorNormalize(newright);
			VectorNormalize(newup);
//			rsurface.batchvertex3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f);
//			rsurface.batchvertex3f_vertexbuffer = NULL;
//			rsurface.batchvertex3f_bufferoffset = 0;
			{
				const float *v1, *v2;
				vec3_t start, end;
				float f, l;
				struct
				{
					float length2;
					const float *v1;
					const float *v2;
				}
				shortest[2];
				memset(shortest, 0, sizeof(shortest));
				// a single autosprite surface can contain multiple sprites...
				for (j = 0;j < batchnumvertices - 3;j += 4)
				{
					VectorClear(center);
					for (i = 0;i < 4;i++)
						VectorAdd(center, rsurface.batchvertex3f + 3*(j+i), center);
					VectorScale(center, 0.25f, center);
					// find the two shortest edges, then use them to define the
					// axis vectors for rotating around the central axis
					for (i = 0;i < 6;i++)
					{
						v1 = rsurface.batchvertex3f + 3*(j+quadedges[i][0]);
						v2 = rsurface.batchvertex3f + 3*(j+quadedges[i][1]);
						l = VectorDistance2(v1, v2);
						// this length bias tries to make sense of square polygons, assuming they are meant to be upright
						if (v1[2] != v2[2])
							l += (1.0f / 1024.0f);
						if (shortest[0].length2 > l || i == 0)
						{
							shortest[1] = shortest[0];
							shortest[0].length2 = l;
							shortest[0].v1 = v1;
							shortest[0].v2 = v2;
						}
						else if (shortest[1].length2 > l || i == 1)
						{
							shortest[1].length2 = l;
							shortest[1].v1 = v1;
							shortest[1].v2 = v2;
						}
					}
					VectorLerp(shortest[0].v1, 0.5f, shortest[0].v2, start);
					VectorLerp(shortest[1].v1, 0.5f, shortest[1].v2, end);
					// this calculates the right vector from the shortest edge
					// and the up vector from the edge midpoints
					VectorSubtract(shortest[0].v1, shortest[0].v2, right);
					VectorNormalize(right);
					VectorSubtract(end, start, up);
					VectorNormalize(up);
					// calculate a forward vector to use instead of the original plane normal (this is how we get a new right vector)
					VectorSubtract(rsurface.localvieworigin, center, forward);
					//Matrix4x4_Transform3x3(&rsurface.inversematrix, r_refdef.view.forward, forward);
					VectorNegate(forward, forward);
					VectorReflect(forward, 0, up, forward);
					VectorNormalize(forward);
					CrossProduct(up, forward, newright);
					VectorNormalize(newright);
					// rotate the quad around the up axis vector, this is made
					// especially easy by the fact we know the quad is flat,
					// so we only have to subtract the center position and
					// measure distance along the right vector, and then
					// multiply that by the newright vector and add back the
					// center position
					// we also need to subtract the old position to undo the
					// displacement from the center, which we do with a
					// DotProduct, the subtraction/addition of center is also
					// optimized into DotProducts here
					l = DotProduct(right, center);
					for (i = 0;i < 4;i++)
					{
						v1 = rsurface.batchvertex3f + 3*(j+i);
						f = DotProduct(right, v1) - l;
						VectorMAMAM(1, v1, -f, right, f, newright, rsurface.batchvertex3f + 3*(j+i));
					}
				}
			}
			if(batchneed & (BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR)) // otherwise these can stay NULL
			{
//				rsurface.batchnormal3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchnormal3f_vertexbuffer = NULL;
//				rsurface.batchnormal3f_bufferoffset = 0;
				Mod_BuildNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchnormal3f, r_smoothnormals_areaweighting.integer != 0);
			}
			if(batchneed & BATCHNEED_ARRAY_VECTOR) // otherwise these can stay NULL
			{
//				rsurface.batchsvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchsvector3f_vertexbuffer = NULL;
//				rsurface.batchsvector3f_bufferoffset = 0;
//				rsurface.batchtvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchtvector3f_vertexbuffer = NULL;
//				rsurface.batchtvector3f_bufferoffset = 0;
				Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			}
			break;
		case Q3DEFORM_NORMAL:
			// deform the normals to make reflections wavey
			rsurface.batchnormal3f = (float *)R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchnormal3f);
			rsurface.batchnormal3f_vertexbuffer = NULL;
			rsurface.batchnormal3f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				float vertex[3];
				float *normal = rsurface.batchnormal3f + 3*j;
				VectorScale(rsurface.batchvertex3f + 3*j, 0.98f, vertex);
				normal[0] = rsurface.batchnormal3f[j*3+0] + deform->parms[0] * noise4f(      vertex[0], vertex[1], vertex[2], rsurface.shadertime * deform->parms[1]);
				normal[1] = rsurface.batchnormal3f[j*3+1] + deform->parms[0] * noise4f( 98 + vertex[0], vertex[1], vertex[2], rsurface.shadertime * deform->parms[1]);
				normal[2] = rsurface.batchnormal3f[j*3+2] + deform->parms[0] * noise4f(196 + vertex[0], vertex[1], vertex[2], rsurface.shadertime * deform->parms[1]);
				VectorNormalize(normal);
			}
			if(batchneed & BATCHNEED_ARRAY_VECTOR) // otherwise these can stay NULL
			{
//				rsurface.batchsvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchsvector3f_vertexbuffer = NULL;
//				rsurface.batchsvector3f_bufferoffset = 0;
//				rsurface.batchtvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchtvector3f_vertexbuffer = NULL;
//				rsurface.batchtvector3f_bufferoffset = 0;
				Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			}
			break;
		case Q3DEFORM_WAVE:
			// deform vertex array to make wavey water and flags and such
			waveparms[0] = deform->waveparms[0];
			waveparms[1] = deform->waveparms[1];
			waveparms[2] = deform->waveparms[2];
			waveparms[3] = deform->waveparms[3];
			if(!R_TestQ3WaveFunc(deform->wavefunc, waveparms))
				break; // if wavefunc is a nop, don't make a dynamic vertex array
			// this is how a divisor of vertex influence on deformation
			animpos = deform->parms[0] ? 1.0f / deform->parms[0] : 100.0f;
			scale = R_EvaluateQ3WaveFunc(deform->wavefunc, waveparms);
//			rsurface.batchvertex3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f);
//			rsurface.batchvertex3f_vertexbuffer = NULL;
//			rsurface.batchvertex3f_bufferoffset = 0;
//			rsurface.batchnormal3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchnormal3f);
//			rsurface.batchnormal3f_vertexbuffer = NULL;
//			rsurface.batchnormal3f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				// if the wavefunc depends on time, evaluate it per-vertex
				if (waveparms[3])
				{
					waveparms[2] = deform->waveparms[2] + (rsurface.batchvertex3f[j*3+0] + rsurface.batchvertex3f[j*3+1] + rsurface.batchvertex3f[j*3+2]) * animpos;
					scale = R_EvaluateQ3WaveFunc(deform->wavefunc, waveparms);
				}
				VectorMA(rsurface.batchvertex3f + 3*j, scale, rsurface.batchnormal3f + 3*j, rsurface.batchvertex3f + 3*j);
			}
			// if we get here, BATCHNEED_ARRAY_NORMAL is in batchneed, so no need to check
			Mod_BuildNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchnormal3f, r_smoothnormals_areaweighting.integer != 0);
			if(batchneed & BATCHNEED_ARRAY_VECTOR) // otherwise these can stay NULL
			{
//				rsurface.batchsvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchsvector3f_vertexbuffer = NULL;
//				rsurface.batchsvector3f_bufferoffset = 0;
//				rsurface.batchtvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchtvector3f_vertexbuffer = NULL;
//				rsurface.batchtvector3f_bufferoffset = 0;
				Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			}
			break;
		case Q3DEFORM_BULGE:
			// deform vertex array to make the surface have moving bulges
//			rsurface.batchvertex3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f);
//			rsurface.batchvertex3f_vertexbuffer = NULL;
//			rsurface.batchvertex3f_bufferoffset = 0;
//			rsurface.batchnormal3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchnormal3f);
//			rsurface.batchnormal3f_vertexbuffer = NULL;
//			rsurface.batchnormal3f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				scale = sin(rsurface.batchtexcoordtexture2f[j*2+0] * deform->parms[0] + rsurface.shadertime * deform->parms[2]) * deform->parms[1];
				VectorMA(rsurface.batchvertex3f + 3*j, scale, rsurface.batchnormal3f + 3*j, rsurface.batchvertex3f + 3*j);
			}
			// if we get here, BATCHNEED_ARRAY_NORMAL is in batchneed, so no need to check
			Mod_BuildNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchnormal3f, r_smoothnormals_areaweighting.integer != 0);
			if(batchneed & BATCHNEED_ARRAY_VECTOR) // otherwise these can stay NULL
			{
//				rsurface.batchsvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchsvector3f_vertexbuffer = NULL;
//				rsurface.batchsvector3f_bufferoffset = 0;
//				rsurface.batchtvector3f = R_FrameData_Alloc(batchnumvertices * sizeof(float[3]));
//				rsurface.batchtvector3f_vertexbuffer = NULL;
//				rsurface.batchtvector3f_bufferoffset = 0;
				Mod_BuildTextureVectorsFromNormals(rsurface.batchfirstvertex, batchnumvertices, batchnumtriangles, rsurface.batchvertex3f, rsurface.batchtexcoordtexture2f, rsurface.batchnormal3f, rsurface.batchelement3i + 3 * rsurface.batchfirsttriangle, rsurface.batchsvector3f, rsurface.batchtvector3f, r_smoothnormals_areaweighting.integer != 0);
			}
			break;
		case Q3DEFORM_MOVE:
			// deform vertex array
			if(!R_TestQ3WaveFunc(deform->wavefunc, deform->waveparms))
				break; // if wavefunc is a nop, don't make a dynamic vertex array
			scale = R_EvaluateQ3WaveFunc(deform->wavefunc, deform->waveparms);
			VectorScale(deform->parms, scale, waveparms);
//			rsurface.batchvertex3f = R_FrameData_Store(batchnumvertices * sizeof(float[3]), rsurface.batchvertex3f);
//			rsurface.batchvertex3f_vertexbuffer = NULL;
//			rsurface.batchvertex3f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
				VectorAdd(rsurface.batchvertex3f + 3*j, waveparms, rsurface.batchvertex3f + 3*j);
			break;
		}
	}

	if (rsurface.batchtexcoordtexture2f && rsurface.texture->materialshaderpass)
	{
	// generate texcoords based on the chosen texcoord source
		switch(rsurface.texture->materialshaderpass->tcgen.tcgen)
		{
		default:
		case Q3TCGEN_TEXTURE:
			break;
		case Q3TCGEN_LIGHTMAP:
	//		rsurface.batchtexcoordtexture2f = R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
	//		rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
	//		rsurface.batchtexcoordtexture2f_bufferoffset = 0;
			if (rsurface.batchtexcoordlightmap2f)
				memcpy(rsurface.batchtexcoordtexture2f, rsurface.batchtexcoordlightmap2f, batchnumvertices * sizeof(float[2]));
			break;
		case Q3TCGEN_VECTOR:
	//		rsurface.batchtexcoordtexture2f = R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
	//		rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
	//		rsurface.batchtexcoordtexture2f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				rsurface.batchtexcoordtexture2f[j*2+0] = DotProduct(rsurface.batchvertex3f + 3*j, rsurface.texture->materialshaderpass->tcgen.parms);
				rsurface.batchtexcoordtexture2f[j*2+1] = DotProduct(rsurface.batchvertex3f + 3*j, rsurface.texture->materialshaderpass->tcgen.parms + 3);
			}
			break;
		case Q3TCGEN_ENVIRONMENT:
			// make environment reflections using a spheremap
			rsurface.batchtexcoordtexture2f = (float *)R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
			rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
			rsurface.batchtexcoordtexture2f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				// identical to Q3A's method, but executed in worldspace so
				// carried models can be shiny too

				float viewer[3], d, reflected[3], worldreflected[3];

				VectorSubtract(rsurface.localvieworigin, rsurface.batchvertex3f + 3*j, viewer);
				// VectorNormalize(viewer);

				d = DotProduct(rsurface.batchnormal3f + 3*j, viewer);

				reflected[0] = rsurface.batchnormal3f[j*3+0]*2*d - viewer[0];
				reflected[1] = rsurface.batchnormal3f[j*3+1]*2*d - viewer[1];
				reflected[2] = rsurface.batchnormal3f[j*3+2]*2*d - viewer[2];
				// note: this is proportinal to viewer, so we can normalize later

				Matrix4x4_Transform3x3(&rsurface.matrix, reflected, worldreflected);
				VectorNormalize(worldreflected);

				// note: this sphere map only uses world x and z!
				// so positive and negative y will LOOK THE SAME.
				rsurface.batchtexcoordtexture2f[j*2+0] = 0.5 + 0.5 * worldreflected[1];
				rsurface.batchtexcoordtexture2f[j*2+1] = 0.5 - 0.5 * worldreflected[2];
			}
			break;
		}
		// the only tcmod that needs software vertex processing is turbulent, so
		// check for it here and apply the changes if needed
		// and we only support that as the first one
		// (handling a mixture of turbulent and other tcmods would be problematic
		//  without punting it entirely to a software path)
		if (rsurface.texture->materialshaderpass->tcmods[0].tcmod == Q3TCMOD_TURBULENT)
		{
			amplitude = rsurface.texture->materialshaderpass->tcmods[0].parms[1];
			animpos = rsurface.texture->materialshaderpass->tcmods[0].parms[2] + rsurface.shadertime * rsurface.texture->materialshaderpass->tcmods[0].parms[3];
	//		rsurface.batchtexcoordtexture2f = R_FrameData_Alloc(batchnumvertices * sizeof(float[2]));
	//		rsurface.batchtexcoordtexture2f_vertexbuffer = NULL;
	//		rsurface.batchtexcoordtexture2f_bufferoffset = 0;
			for (j = 0;j < batchnumvertices;j++)
			{
				rsurface.batchtexcoordtexture2f[j*2+0] += amplitude * sin(((rsurface.batchvertex3f[j*3+0] + rsurface.batchvertex3f[j*3+2]) * 1.0 / 1024.0f + animpos) * M_PI * 2);
				rsurface.batchtexcoordtexture2f[j*2+1] += amplitude * sin(((rsurface.batchvertex3f[j*3+1]                                ) * 1.0 / 1024.0f + animpos) * M_PI * 2);
			}
		}
	}
}

void RSurf_DrawBatch(void)
{
	// sometimes a zero triangle surface (usually a degenerate patch) makes it
	// through the pipeline, killing it earlier in the pipeline would have
	// per-surface overhead rather than per-batch overhead, so it's best to
	// reject it here, before it hits glDraw.
	if (rsurface.batchnumtriangles == 0)
		return;
#if 0
	// batch debugging code
	if (r_test.integer && rsurface.entity == r_refdef.scene.worldentity && rsurface.batchvertex3f == r_refdef.scene.worldentity->model->surfmesh.data_vertex3f)
	{
		int i;
		int j;
		int c;
		const int *e;
		e = rsurface.batchelement3i + rsurface.batchfirsttriangle*3;
		for (i = 0;i < rsurface.batchnumtriangles*3;i++)
		{
			c = e[i];
			for (j = 0;j < rsurface.entity->model->num_surfaces;j++)
			{
				if (c >= rsurface.modelsurfaces[j].num_firstvertex && c < (rsurface.modelsurfaces[j].num_firstvertex + rsurface.modelsurfaces[j].num_vertices))
				{
					if (rsurface.modelsurfaces[j].texture != rsurface.texture)
						Sys_Error("RSurf_DrawBatch: index %i uses different texture (%s) than surface %i which it belongs to (which uses %s)\n", c, rsurface.texture->name, j, rsurface.modelsurfaces[j].texture->name);
					break;
				}
			}
		}
	}
#endif
	if (rsurface.batchmultidraw)
	{
		// issue multiple draws rather than copying index data
		int numsurfaces = rsurface.batchmultidrawnumsurfaces;
		const msurface_t **surfacelist = rsurface.batchmultidrawsurfacelist;
		int i, j, k, firstvertex, endvertex, firsttriangle, endtriangle;
		for (i = 0;i < numsurfaces;)
		{
			// combine consecutive surfaces as one draw
			for (k = i, j = i + 1;j < numsurfaces;k = j, j++)
				if (surfacelist[j] != surfacelist[k] + 1)
					break;
			firstvertex = surfacelist[i]->num_firstvertex;
			endvertex = surfacelist[k]->num_firstvertex + surfacelist[k]->num_vertices;
			firsttriangle = surfacelist[i]->num_firsttriangle;
			endtriangle = surfacelist[k]->num_firsttriangle + surfacelist[k]->num_triangles;
			R_Mesh_Draw(firstvertex, endvertex - firstvertex, firsttriangle, endtriangle - firsttriangle, rsurface.batchelement3i, rsurface.batchelement3i_indexbuffer, rsurface.batchelement3i_bufferoffset, rsurface.batchelement3s, rsurface.batchelement3s_indexbuffer, rsurface.batchelement3s_bufferoffset);
			i = j;
		}
	}
	else
	{
		// there is only one consecutive run of index data (may have been combined)
		R_Mesh_Draw(rsurface.batchfirstvertex, rsurface.batchnumvertices, rsurface.batchfirsttriangle, rsurface.batchnumtriangles, rsurface.batchelement3i, rsurface.batchelement3i_indexbuffer, rsurface.batchelement3i_bufferoffset, rsurface.batchelement3s, rsurface.batchelement3s_indexbuffer, rsurface.batchelement3s_bufferoffset);
	}
}

static int RSurf_FindWaterPlaneForSurface(const msurface_t *surface)
{
	// pick the closest matching water plane
	int planeindex, vertexindex, bestplaneindex = -1;
	float d, bestd;
	vec3_t vert;
	const float *v;
	r_waterstate_waterplane_t *p;
	qbool prepared = false;
	bestd = 0;
	for (planeindex = 0, p = r_fb.water.waterplanes;planeindex < r_fb.water.numwaterplanes;planeindex++, p++)
	{
		if(p->camera_entity != rsurface.texture->camera_entity)
			continue;
		d = 0;
		if(!prepared)
		{
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX, 1, &surface);
			prepared = true;
			if(rsurface.batchnumvertices == 0)
				break;
		}
		for (vertexindex = 0, v = rsurface.batchvertex3f + rsurface.batchfirstvertex * 3;vertexindex < rsurface.batchnumvertices;vertexindex++, v += 3)
		{
			Matrix4x4_Transform(&rsurface.matrix, v, vert);
			d += fabs(PlaneDiff(vert, &p->plane));
		}
		if (bestd > d || bestplaneindex < 0)
		{
			bestd = d;
			bestplaneindex = planeindex;
		}
	}
	return bestplaneindex;
	// NOTE: this MAY return a totally unrelated water plane; we can ignore
	// this situation though, as it might be better to render single larger
	// batches with useless stuff (backface culled for example) than to
	// render multiple smaller batches
}

void RSurf_SetupDepthAndCulling(bool ui)
{
	// submodels are biased to avoid z-fighting with world surfaces that they
	// may be exactly overlapping (avoids z-fighting artifacts on certain
	// doors and things in Quake maps)
	GL_DepthRange(0, (rsurface.texture->currentmaterialflags & MATERIALFLAG_SHORTDEPTHRANGE) ? 0.0625 : 1);
	GL_PolygonOffset(rsurface.basepolygonfactor + rsurface.texture->biaspolygonfactor, rsurface.basepolygonoffset + rsurface.texture->biaspolygonoffset);
	GL_DepthTest(!ui && !(rsurface.texture->currentmaterialflags & MATERIALFLAG_NODEPTHTEST));
	GL_CullFace((rsurface.texture->currentmaterialflags & MATERIALFLAG_NOCULLFACE) ? GL_NONE : r_refdef.view.cullface_back);
}

static void R_DrawTextureSurfaceList_Sky(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	int j;
	const float *v;
	float p[3], mins[3], maxs[3];
	int scissor[4];
	// transparent sky would be ridiculous
	if (rsurface.texture->currentmaterialflags & MATERIALFLAGMASK_DEPTHSORTED)
		return;
	R_SetupShader_Generic_NoTexture(false, false);
	skyrenderlater = true;
	RSurf_SetupDepthAndCulling(false);
	GL_DepthMask(true);

	// add the vertices of the surfaces to a world bounding box so we can scissor the sky render later
	if (r_sky_scissor.integer)
	{
		RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_NOGAPS, texturenumsurfaces, texturesurfacelist);
		for (j = 0, v = rsurface.batchvertex3f + 3 * rsurface.batchfirstvertex; j < rsurface.batchnumvertices; j++, v += 3)
		{
			Matrix4x4_Transform(&rsurface.matrix, v, p);
			if (j > 0)
			{
				if (mins[0] > p[0]) mins[0] = p[0];
				if (mins[1] > p[1]) mins[1] = p[1];
				if (mins[2] > p[2]) mins[2] = p[2];
				if (maxs[0] < p[0]) maxs[0] = p[0];
				if (maxs[1] < p[1]) maxs[1] = p[1];
				if (maxs[2] < p[2]) maxs[2] = p[2];
			}
			else
			{
				VectorCopy(p, mins);
				VectorCopy(p, maxs);
			}
		}
		if (!R_ScissorForBBox(mins, maxs, scissor))
		{
			if (skyscissor[2])
			{
				if (skyscissor[0] > scissor[0])
				{
					skyscissor[2] += skyscissor[0] - scissor[0];
					skyscissor[0] = scissor[0];
				}
				if (skyscissor[1] > scissor[1])
				{
					skyscissor[3] += skyscissor[1] - scissor[1];
					skyscissor[1] = scissor[1];
				}
				if (skyscissor[0] + skyscissor[2] < scissor[0] + scissor[2])
					skyscissor[2] = scissor[0] + scissor[2] - skyscissor[0];
				if (skyscissor[1] + skyscissor[3] < scissor[1] + scissor[3])
					skyscissor[3] = scissor[1] + scissor[3] - skyscissor[1];
			}
			else
				Vector4Copy(scissor, skyscissor);
		}
	}

	// LadyHavoc: HalfLife maps have freaky skypolys so don't use
	// skymasking on them, and Quake3 never did sky masking (unlike
	// software Quake and software Quake2), so disable the sky masking
	// in Quake3 maps as it causes problems with q3map2 sky tricks,
	// and skymasking also looks very bad when noclipping outside the
	// level, so don't use it then either.
	if (r_refdef.scene.worldmodel && r_refdef.scene.worldmodel->brush.skymasking && (r_refdef.scene.worldmodel->brush.isq3bsp ? r_q3bsp_renderskydepth.integer : r_q1bsp_skymasking.integer) && !r_refdef.viewcache.world_novis && !r_trippy.integer)
	{
		R_Mesh_ResetTextureState();
		if (skyrendermasked)
		{
			R_SetupShader_DepthOrShadow(false, false, false);
			// depth-only (masking)
			GL_ColorMask(0, 0, 0, 0);
			// just to make sure that braindead drivers don't draw
			// anything despite that colormask...
			GL_BlendFunc(GL_ZERO, GL_ONE);
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ALLOWMULTIDRAW, texturenumsurfaces, texturesurfacelist);
			R_Mesh_PrepareVertices_Vertex3f(rsurface.batchnumvertices, rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
		}
		else
		{
			R_SetupShader_Generic_NoTexture(false, false);
			// fog sky
			GL_BlendFunc(GL_ONE, GL_ZERO);
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_NOGAPS, texturenumsurfaces, texturesurfacelist);
			GL_Color(r_refdef.fogcolor[0], r_refdef.fogcolor[1], r_refdef.fogcolor[2], 1);
			R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, NULL, NULL);
		}
		RSurf_DrawBatch();
		if (skyrendermasked)
			GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	}
	R_Mesh_ResetTextureState();
	GL_Color(1, 1, 1, 1);
}

extern rtexture_t *r_shadow_prepasslightingdiffusetexture;
extern rtexture_t *r_shadow_prepasslightingspeculartexture;
static void R_DrawTextureSurfaceList_GL20(int texturenumsurfaces, const msurface_t **texturesurfacelist, qbool writedepth, qbool prepass, qbool ui)
{
	if (r_fb.water.renderingscene && (rsurface.texture->currentmaterialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_REFLECTION | MATERIALFLAG_CAMERA)))
		return;
	if (prepass)
	{
		// render screenspace normalmap to texture
		GL_DepthMask(true);
		R_SetupShader_Surface(vec3_origin, vec3_origin, vec3_origin, RSURFPASS_DEFERREDGEOMETRY, texturenumsurfaces, texturesurfacelist, NULL, false, false);
		RSurf_DrawBatch();
		return;
	}

	// bind lightmap texture

	// water/refraction/reflection/camera surfaces have to be handled specially
	if ((rsurface.texture->currentmaterialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_CAMERA | MATERIALFLAG_REFLECTION)))
	{
		int start, end, startplaneindex;
		for (start = 0;start < texturenumsurfaces;start = end)
		{
			startplaneindex = RSurf_FindWaterPlaneForSurface(texturesurfacelist[start]);
			if(startplaneindex < 0)
			{
				// this happens if the plane e.g. got backface culled and thus didn't get a water plane. We can just ignore this.
				// Con_Printf("No matching water plane for surface with material flags 0x%08x - PLEASE DEBUG THIS\n", rsurface.texture->currentmaterialflags);
				end = start + 1;
				continue;
			}
			for (end = start + 1;end < texturenumsurfaces && startplaneindex == RSurf_FindWaterPlaneForSurface(texturesurfacelist[end]);end++)
				;
			// now that we have a batch using the same planeindex, render it
			if ((rsurface.texture->currentmaterialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION | MATERIALFLAG_CAMERA)))
			{
				// render water or distortion background
				GL_DepthMask(true);
				R_SetupShader_Surface(vec3_origin, vec3_origin, vec3_origin, RSURFPASS_BACKGROUND, end-start, texturesurfacelist + start, (void *)(r_fb.water.waterplanes + startplaneindex), false, false);
				RSurf_DrawBatch();
				// blend surface on top
				GL_DepthMask(false);
				R_SetupShader_Surface(vec3_origin, vec3_origin, vec3_origin, RSURFPASS_BASE, end-start, texturesurfacelist + start, NULL, false, false);
				RSurf_DrawBatch();
			}
			else if ((rsurface.texture->currentmaterialflags & MATERIALFLAG_REFLECTION))
			{
				// render surface with reflection texture as input
				GL_DepthMask(writedepth && !(rsurface.texture->currentmaterialflags & MATERIALFLAG_BLENDED));
				R_SetupShader_Surface(vec3_origin, vec3_origin, vec3_origin, RSURFPASS_BASE, end-start, texturesurfacelist + start, (void *)(r_fb.water.waterplanes + startplaneindex), false, false);
				RSurf_DrawBatch();
			}
		}
		return;
	}

	// render surface batch normally
	GL_DepthMask(writedepth && !(rsurface.texture->currentmaterialflags & MATERIALFLAG_BLENDED));
	R_SetupShader_Surface(vec3_origin, vec3_origin, vec3_origin, RSURFPASS_BASE, texturenumsurfaces, texturesurfacelist, NULL, (rsurface.texture->currentmaterialflags & MATERIALFLAG_SKY) != 0 || ui, ui);
	RSurf_DrawBatch();
}

static void R_DrawTextureSurfaceList_ShowSurfaces(int texturenumsurfaces, const msurface_t **texturesurfacelist, qbool writedepth)
{
	int vi;
	int j;
	int texturesurfaceindex;
	int k;
	const msurface_t *surface;
	float surfacecolor4f[4];
	float c[4];
	texture_t *t = rsurface.texture;

//	R_Mesh_ResetTextureState();
	R_SetupShader_Generic_NoTexture(false, false);

	GL_BlendFunc(GL_ONE, GL_ZERO);
	GL_DepthMask(writedepth);

	switch (r_showsurfaces.integer)
	{
		case 1:
		default:
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_VERTEXCOLOR | BATCHNEED_ALWAYSCOPY, texturenumsurfaces, texturesurfacelist);
			vi = 0;
			for (texturesurfaceindex = 0;texturesurfaceindex < texturenumsurfaces;texturesurfaceindex++)
			{
				surface = texturesurfacelist[texturesurfaceindex];
				k = (int)(((size_t)surface) / sizeof(msurface_t));
				Vector4Set(surfacecolor4f, (k & 0xF) * (1.0f / 16.0f), (k & 0xF0) * (1.0f / 256.0f), (k & 0xF00) * (1.0f / 4096.0f), 1);
				for (j = 0;j < surface->num_vertices;j++)
				{
					Vector4Copy(surfacecolor4f, rsurface.batchlightmapcolor4f + 4 * vi);
					vi++;
				}
			}
			break;
		case 3:
			if(t && t->currentskinframe)
			{
				Vector4Copy(t->currentskinframe->avgcolor, c);
				c[3] *= t->currentalpha;
			}
			else
			{
				Vector4Set(c, 1, 0, 1, 1);
			}
			if (t && (t->pantstexture || t->shirttexture))
			{
				VectorMAM(0.7, t->render_colormap_pants, 0.3, t->render_colormap_shirt, c);
			}
			VectorScale(c, 2 * r_refdef.view.colorscale, c);
			if(t->currentmaterialflags & MATERIALFLAG_WATERALPHA)
				c[3] *= r_wateralpha.value;
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_VERTEXCOLOR | BATCHNEED_ALWAYSCOPY, texturenumsurfaces, texturesurfacelist);
			vi = 0;
			if (rsurface.modellightmapcolor4f)
			{
				for (texturesurfaceindex = 0;texturesurfaceindex < texturenumsurfaces;texturesurfaceindex++)
				{
					surface = texturesurfacelist[texturesurfaceindex];
					for (j = 0;j < surface->num_vertices;j++)
					{
						float *ptr = rsurface.batchlightmapcolor4f + 4 * vi;
						Vector4Multiply(ptr, c, ptr);
						vi++;
					}
				}
			}
			else
			{
				for (texturesurfaceindex = 0;texturesurfaceindex < texturenumsurfaces;texturesurfaceindex++)
				{
					surface = texturesurfacelist[texturesurfaceindex];
					for (j = 0;j < surface->num_vertices;j++)
					{
						float *ptr = rsurface.batchlightmapcolor4f + 4 * vi;
						Vector4Copy(c, ptr);
						vi++;
					}
				}
			}
			break;
	}
	R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, rsurface.batchlightmapcolor4f, rsurface.batchtexcoordtexture2f);
	RSurf_DrawBatch();
}

static void R_DrawModelTextureSurfaceList(int texturenumsurfaces, const msurface_t **texturesurfacelist, qbool writedepth, qbool prepass, qbool ui)
{
	CHECKGLERROR
	RSurf_SetupDepthAndCulling(ui);
	if (r_showsurfaces.integer && r_refdef.view.showdebug)
	{
		R_DrawTextureSurfaceList_ShowSurfaces(texturenumsurfaces, texturesurfacelist, writedepth);
		return;
	}
	switch (vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		R_DrawTextureSurfaceList_GL20(texturenumsurfaces, texturesurfacelist, writedepth, prepass, ui);
		break;
	}
	CHECKGLERROR
}

static void R_DrawSurface_TransparentCallback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int i, j;
	int texturenumsurfaces, endsurface;
	texture_t *texture;
	const msurface_t *surface;
	const msurface_t *texturesurfacelist[MESHQUEUE_TRANSPARENT_BATCHSIZE];

	RSurf_ActiveModelEntity(ent, true, true, false);

	if (r_transparentdepthmasking.integer)
	{
		qbool setup = false;
		for (i = 0;i < numsurfaces;i = j)
		{
			j = i + 1;
			surface = rsurface.modelsurfaces + surfacelist[i];
			texture = surface->texture;
			rsurface.texture = R_GetCurrentTexture(texture);
			rsurface.lightmaptexture = NULL;
			rsurface.deluxemaptexture = NULL;
			rsurface.uselightmaptexture = false;
			// scan ahead until we find a different texture
			endsurface = min(i + 1024, numsurfaces);
			texturenumsurfaces = 0;
			texturesurfacelist[texturenumsurfaces++] = surface;
			for (;j < endsurface;j++)
			{
				surface = rsurface.modelsurfaces + surfacelist[j];
				if (texture != surface->texture)
					break;
				texturesurfacelist[texturenumsurfaces++] = surface;
			}
			if (!(rsurface.texture->currentmaterialflags & MATERIALFLAG_TRANSDEPTH))
				continue;
			// render the range of surfaces as depth
			if (!setup)
			{
				setup = true;
				GL_ColorMask(0,0,0,0);
				GL_Color(1,1,1,1);
				GL_DepthTest(true);
				GL_BlendFunc(GL_ONE, GL_ZERO);
				GL_DepthMask(true);
//				R_Mesh_ResetTextureState();
			}
			RSurf_SetupDepthAndCulling(false);
			RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ALLOWMULTIDRAW, texturenumsurfaces, texturesurfacelist);
			R_SetupShader_DepthOrShadow(false, false, !!rsurface.batchskeletaltransform3x4);
			R_Mesh_PrepareVertices_Vertex3f(rsurface.batchnumvertices, rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
			RSurf_DrawBatch();
		}
		if (setup)
			GL_ColorMask(r_refdef.view.colormask[0], r_refdef.view.colormask[1], r_refdef.view.colormask[2], 1);
	}

	for (i = 0;i < numsurfaces;i = j)
	{
		j = i + 1;
		surface = rsurface.modelsurfaces + surfacelist[i];
		texture = surface->texture;
		rsurface.texture = R_GetCurrentTexture(texture);
		// scan ahead until we find a different texture
		endsurface = min(i + MESHQUEUE_TRANSPARENT_BATCHSIZE, numsurfaces);
		texturenumsurfaces = 0;
		texturesurfacelist[texturenumsurfaces++] = surface;
			rsurface.lightmaptexture = surface->lightmaptexture;
			rsurface.deluxemaptexture = surface->deluxemaptexture;
			rsurface.uselightmaptexture = surface->lightmaptexture != NULL;
			for (;j < endsurface;j++)
			{
				surface = rsurface.modelsurfaces + surfacelist[j];
				if (texture != surface->texture || rsurface.lightmaptexture != surface->lightmaptexture)
					break;
				texturesurfacelist[texturenumsurfaces++] = surface;
			}
		// render the range of surfaces
		R_DrawModelTextureSurfaceList(texturenumsurfaces, texturesurfacelist, false, false, false);
	}
	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

static void R_ProcessTransparentTextureSurfaceList(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	// transparent surfaces get pushed off into the transparent queue
	int surfacelistindex;
	const msurface_t *surface;
	vec3_t tempcenter, center;
	for (surfacelistindex = 0;surfacelistindex < texturenumsurfaces;surfacelistindex++)
	{
		surface = texturesurfacelist[surfacelistindex];
		if (r_transparent_sortsurfacesbynearest.integer)
		{
			tempcenter[0] = bound(surface->mins[0], rsurface.localvieworigin[0], surface->maxs[0]);
			tempcenter[1] = bound(surface->mins[1], rsurface.localvieworigin[1], surface->maxs[1]);
			tempcenter[2] = bound(surface->mins[2], rsurface.localvieworigin[2], surface->maxs[2]);
		}
		else
		{
			tempcenter[0] = (surface->mins[0] + surface->maxs[0]) * 0.5f;
			tempcenter[1] = (surface->mins[1] + surface->maxs[1]) * 0.5f;
			tempcenter[2] = (surface->mins[2] + surface->maxs[2]) * 0.5f;
		}
		Matrix4x4_Transform(&rsurface.matrix, tempcenter, center);
		if (rsurface.entity->transparent_offset) // transparent offset
		{
			center[0] += r_refdef.view.forward[0]*rsurface.entity->transparent_offset;
			center[1] += r_refdef.view.forward[1]*rsurface.entity->transparent_offset;
			center[2] += r_refdef.view.forward[2]*rsurface.entity->transparent_offset;
		}
		R_MeshQueue_AddTransparent((rsurface.entity->flags & RENDER_WORLDOBJECT) ? TRANSPARENTSORT_SKY : (rsurface.texture->currentmaterialflags & MATERIALFLAG_NODEPTHTEST) ? TRANSPARENTSORT_HUD : rsurface.texture->transparentsort, center, R_DrawSurface_TransparentCallback, rsurface.entity, surface - rsurface.modelsurfaces, rsurface.rtlight);
	}
}

static void R_DrawTextureSurfaceList_DepthOnly(int texturenumsurfaces, const msurface_t **texturesurfacelist)
{
	if ((rsurface.texture->currentmaterialflags & (MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_BLENDED | MATERIALFLAG_ALPHATEST)))
		return;
	if (r_fb.water.renderingscene && (rsurface.texture->currentmaterialflags & (MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFLECTION)))
		return;
	RSurf_SetupDepthAndCulling(false);
	RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ALLOWMULTIDRAW, texturenumsurfaces, texturesurfacelist);
	R_Mesh_PrepareVertices_Vertex3f(rsurface.batchnumvertices, rsurface.batchvertex3f, rsurface.batchvertex3f_vertexbuffer, rsurface.batchvertex3f_bufferoffset);
	R_SetupShader_DepthOrShadow(false, false, !!rsurface.batchskeletaltransform3x4);
	RSurf_DrawBatch();
}

static void R_ProcessModelTextureSurfaceList(int texturenumsurfaces, const msurface_t **texturesurfacelist, qbool writedepth, qbool depthonly, qbool prepass, qbool ui)
{
	CHECKGLERROR
	if (ui)
		R_DrawModelTextureSurfaceList(texturenumsurfaces, texturesurfacelist, writedepth, prepass, ui);
	else if (depthonly)
		R_DrawTextureSurfaceList_DepthOnly(texturenumsurfaces, texturesurfacelist);
	else if (prepass)
	{
		if (!(rsurface.texture->currentmaterialflags & MATERIALFLAG_WALL))
			return;
		if (rsurface.texture->currentmaterialflags & MATERIALFLAGMASK_DEPTHSORTED)
			R_ProcessTransparentTextureSurfaceList(texturenumsurfaces, texturesurfacelist);
		else
			R_DrawModelTextureSurfaceList(texturenumsurfaces, texturesurfacelist, writedepth, prepass, ui);
	}
	else if ((rsurface.texture->currentmaterialflags & MATERIALFLAG_SKY) && (!r_showsurfaces.integer || r_showsurfaces.integer == 3))
		R_DrawTextureSurfaceList_Sky(texturenumsurfaces, texturesurfacelist);
	else if (!(rsurface.texture->currentmaterialflags & MATERIALFLAG_WALL))
		return;
	else if (((rsurface.texture->currentmaterialflags & MATERIALFLAGMASK_DEPTHSORTED) || (r_showsurfaces.integer == 3 && (rsurface.texture->currentmaterialflags & MATERIALFLAG_ALPHATEST))))
	{
		// in the deferred case, transparent surfaces were queued during prepass
		if (!r_shadow_usingdeferredprepass)
			R_ProcessTransparentTextureSurfaceList(texturenumsurfaces, texturesurfacelist);
	}
	else
	{
		// the alphatest check is to make sure we write depth for anything we skipped on the depth-only pass earlier
		R_DrawModelTextureSurfaceList(texturenumsurfaces, texturesurfacelist, writedepth || (rsurface.texture->currentmaterialflags & MATERIALFLAG_ALPHATEST), prepass, ui);
	}
	CHECKGLERROR
}

static void R_QueueModelSurfaceList(entity_render_t *ent, int numsurfaces, const msurface_t **surfacelist, int flagsmask, qbool writedepth, qbool depthonly, qbool prepass, qbool ui)
{
	int i, j;
	texture_t *texture;
	R_FrameData_SetMark();
	// break the surface list down into batches by texture and use of lightmapping
	for (i = 0;i < numsurfaces;i = j)
	{
		j = i + 1;
		// texture is the base texture pointer, rsurface.texture is the
		// current frame/skin the texture is directing us to use (for example
		// if a model has 2 skins and it is on skin 1, then skin 0 tells us to
		// use skin 1 instead)
		texture = surfacelist[i]->texture;
		rsurface.texture = R_GetCurrentTexture(texture);
		if (!(rsurface.texture->currentmaterialflags & flagsmask) || (rsurface.texture->currentmaterialflags & MATERIALFLAG_NODRAW))
		{
			// if this texture is not the kind we want, skip ahead to the next one
			for (;j < numsurfaces && texture == surfacelist[j]->texture;j++)
				;
			continue;
		}
		if(depthonly || prepass)
		{
			rsurface.lightmaptexture = NULL;
			rsurface.deluxemaptexture = NULL;
			rsurface.uselightmaptexture = false;
			// simply scan ahead until we find a different texture or lightmap state
			for (;j < numsurfaces && texture == surfacelist[j]->texture;j++)
				;
		}
		else
		{
			rsurface.lightmaptexture = surfacelist[i]->lightmaptexture;
			rsurface.deluxemaptexture = surfacelist[i]->deluxemaptexture;
			rsurface.uselightmaptexture = surfacelist[i]->lightmaptexture != NULL;
			// simply scan ahead until we find a different texture or lightmap state
			for (;j < numsurfaces && texture == surfacelist[j]->texture && rsurface.lightmaptexture == surfacelist[j]->lightmaptexture;j++)
				;
		}
		// render the range of surfaces
		R_ProcessModelTextureSurfaceList(j - i, surfacelist + i, writedepth, depthonly, prepass, ui);
	}
	R_FrameData_ReturnToMark();
}

float locboxvertex3f[6*4*3] =
{
	1,0,1, 1,0,0, 1,1,0, 1,1,1,
	0,1,1, 0,1,0, 0,0,0, 0,0,1,
	1,1,1, 1,1,0, 0,1,0, 0,1,1,
	0,0,1, 0,0,0, 1,0,0, 1,0,1,
	0,0,1, 1,0,1, 1,1,1, 0,1,1,
	1,0,0, 0,0,0, 0,1,0, 1,1,0
};

unsigned short locboxelements[6*2*3] =
{
	 0, 1, 2, 0, 2, 3,
	 4, 5, 6, 4, 6, 7,
	 8, 9,10, 8,10,11,
	12,13,14, 12,14,15,
	16,17,18, 16,18,19,
	20,21,22, 20,22,23
};

static void R_DrawLoc_Callback(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist)
{
	int i, j;
	cl_locnode_t *loc = (cl_locnode_t *)ent;
	vec3_t mins, size;
	float vertex3f[6*4*3];
	CHECKGLERROR
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DepthMask(false);
	GL_DepthRange(0, 1);
	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);
	GL_DepthTest(true);
	GL_CullFace(GL_NONE);
	R_EntityMatrix(&identitymatrix);

//	R_Mesh_ResetTextureState();

	i = surfacelist[0];
	GL_Color(((i & 0x0007) >> 0) * (1.0f / 7.0f) * r_refdef.view.colorscale,
			 ((i & 0x0038) >> 3) * (1.0f / 7.0f) * r_refdef.view.colorscale,
			 ((i & 0x01C0) >> 6) * (1.0f / 7.0f) * r_refdef.view.colorscale,
			surfacelist[0] < 0 ? 0.5f : 0.125f);

	if (VectorCompare(loc->mins, loc->maxs))
	{
		VectorSet(size, 2, 2, 2);
		VectorMA(loc->mins, -0.5f, size, mins);
	}
	else
	{
		VectorCopy(loc->mins, mins);
		VectorSubtract(loc->maxs, loc->mins, size);
	}

	for (i = 0;i < 6*4*3;)
		for (j = 0;j < 3;j++, i++)
			vertex3f[i] = mins[j] + size[j] * locboxvertex3f[i];

	R_Mesh_PrepareVertices_Generic_Arrays(6*4, vertex3f, NULL, NULL);
	R_SetupShader_Generic_NoTexture(false, false);
	R_Mesh_Draw(0, 6*4, 0, 6*2, NULL, NULL, 0, locboxelements, NULL, 0);
}

void R_DrawLocs(void)
{
	int index;
	cl_locnode_t *loc, *nearestloc;
	vec3_t center;
	nearestloc = CL_Locs_FindNearest(cl.movement_origin);
	for (loc = cl.locnodes, index = 0;loc;loc = loc->next, index++)
	{
		VectorLerp(loc->mins, 0.5f, loc->maxs, center);
		R_MeshQueue_AddTransparent(TRANSPARENTSORT_DISTANCE, center, R_DrawLoc_Callback, (entity_render_t *)loc, loc == nearestloc ? -1 : index, NULL);
	}
}

void R_DecalSystem_Reset(decalsystem_t *decalsystem)
{
	if (decalsystem->decals)
		Mem_Free(decalsystem->decals);
	memset(decalsystem, 0, sizeof(*decalsystem));
}

static void R_DecalSystem_SpawnTriangle(decalsystem_t *decalsystem, const float *v0, const float *v1, const float *v2, const float *t0, const float *t1, const float *t2, const float *c0, const float *c1, const float *c2, int triangleindex, int surfaceindex, unsigned int decalsequence)
{
	tridecal_t *decal;
	tridecal_t *decals;
	int i;

	// expand or initialize the system
	if (decalsystem->maxdecals <= decalsystem->numdecals)
	{
		decalsystem_t old = *decalsystem;
		qbool useshortelements;
		decalsystem->maxdecals = max(16, decalsystem->maxdecals * 2);
		useshortelements = decalsystem->maxdecals * 3 <= 65536;
		decalsystem->decals = (tridecal_t *)Mem_Alloc(cls.levelmempool, decalsystem->maxdecals * (sizeof(tridecal_t) + sizeof(float[3][3]) + sizeof(float[3][2]) + sizeof(float[3][4]) + sizeof(int[3]) + (useshortelements ? sizeof(unsigned short[3]) : 0)));
		decalsystem->color4f = (float *)(decalsystem->decals + decalsystem->maxdecals);
		decalsystem->texcoord2f = (float *)(decalsystem->color4f + decalsystem->maxdecals*12);
		decalsystem->vertex3f = (float *)(decalsystem->texcoord2f + decalsystem->maxdecals*6);
		decalsystem->element3i = (int *)(decalsystem->vertex3f + decalsystem->maxdecals*9);
		decalsystem->element3s = (useshortelements ? ((unsigned short *)(decalsystem->element3i + decalsystem->maxdecals*3)) : NULL);
		if (decalsystem->numdecals)
			memcpy(decalsystem->decals, old.decals, decalsystem->numdecals * sizeof(tridecal_t));
		if (old.decals)
			Mem_Free(old.decals);
		for (i = 0;i < decalsystem->maxdecals*3;i++)
			decalsystem->element3i[i] = i;
		if (useshortelements)
			for (i = 0;i < decalsystem->maxdecals*3;i++)
				decalsystem->element3s[i] = i;
	}

	// grab a decal and search for another free slot for the next one
	decals = decalsystem->decals;
	decal = decalsystem->decals + (i = decalsystem->freedecal++);
	for (i = decalsystem->freedecal;i < decalsystem->numdecals && decals[i].color4f[0][3];i++)
		;
	decalsystem->freedecal = i;
	if (decalsystem->numdecals <= i)
		decalsystem->numdecals = i + 1;

	// initialize the decal
	decal->lived = 0;
	decal->triangleindex = triangleindex;
	decal->surfaceindex = surfaceindex;
	decal->decalsequence = decalsequence;
	decal->color4f[0][0] = c0[0];
	decal->color4f[0][1] = c0[1];
	decal->color4f[0][2] = c0[2];
	decal->color4f[0][3] = 1;
	decal->color4f[1][0] = c1[0];
	decal->color4f[1][1] = c1[1];
	decal->color4f[1][2] = c1[2];
	decal->color4f[1][3] = 1;
	decal->color4f[2][0] = c2[0];
	decal->color4f[2][1] = c2[1];
	decal->color4f[2][2] = c2[2];
	decal->color4f[2][3] = 1;
	decal->vertex3f[0][0] = v0[0];
	decal->vertex3f[0][1] = v0[1];
	decal->vertex3f[0][2] = v0[2];
	decal->vertex3f[1][0] = v1[0];
	decal->vertex3f[1][1] = v1[1];
	decal->vertex3f[1][2] = v1[2];
	decal->vertex3f[2][0] = v2[0];
	decal->vertex3f[2][1] = v2[1];
	decal->vertex3f[2][2] = v2[2];
	decal->texcoord2f[0][0] = t0[0];
	decal->texcoord2f[0][1] = t0[1];
	decal->texcoord2f[1][0] = t1[0];
	decal->texcoord2f[1][1] = t1[1];
	decal->texcoord2f[2][0] = t2[0];
	decal->texcoord2f[2][1] = t2[1];
	TriangleNormal(v0, v1, v2, decal->plane);
	VectorNormalize(decal->plane);
	decal->plane[3] = DotProduct(v0, decal->plane);
}

extern cvar_t cl_decals_bias;
extern cvar_t cl_decals_models;
extern cvar_t cl_decals_newsystem_intensitymultiplier;
// baseparms, parms, temps
static void R_DecalSystem_SplatTriangle(decalsystem_t *decalsystem, float r, float g, float b, float a, float s1, float t1, float s2, float t2, unsigned int decalsequence, qbool dynamic, float (*planes)[4], matrix4x4_t *projection, int triangleindex, int surfaceindex)
{
	int cornerindex;
	int index;
	float v[9][3];
	const float *vertex3f;
	const float *normal3f;
	int numpoints;
	float points[2][9][3];
	float temp[3];
	float tc[9][2];
	float f;
	float c[9][4];
	const int *e;

	e = rsurface.modelelement3i + 3*triangleindex;

	vertex3f = rsurface.modelvertex3f;
	normal3f = rsurface.modelnormal3f;

	if (normal3f)
	{
		for (cornerindex = 0;cornerindex < 3;cornerindex++)
		{
			index = 3*e[cornerindex];
			VectorMA(vertex3f + index, cl_decals_bias.value, normal3f + index, v[cornerindex]);
		}
	}
	else
	{
		for (cornerindex = 0;cornerindex < 3;cornerindex++)
		{
			index = 3*e[cornerindex];
			VectorCopy(vertex3f + index, v[cornerindex]);
		}
	}

	// cull backfaces
	//TriangleNormal(v[0], v[1], v[2], normal);
	//if (DotProduct(normal, localnormal) < 0.0f)
	//	continue;
	// clip by each of the box planes formed from the projection matrix
	// if anything survives, we emit the decal
	numpoints = PolygonF_Clip(3        , v[0]        , planes[0][0], planes[0][1], planes[0][2], planes[0][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), points[1][0]);
	if (numpoints < 3)
		return;
	numpoints = PolygonF_Clip(numpoints, points[1][0], planes[1][0], planes[1][1], planes[1][2], planes[1][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), points[0][0]);
	if (numpoints < 3)
		return;
	numpoints = PolygonF_Clip(numpoints, points[0][0], planes[2][0], planes[2][1], planes[2][2], planes[2][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), points[1][0]);
	if (numpoints < 3)
		return;
	numpoints = PolygonF_Clip(numpoints, points[1][0], planes[3][0], planes[3][1], planes[3][2], planes[3][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), points[0][0]);
	if (numpoints < 3)
		return;
	numpoints = PolygonF_Clip(numpoints, points[0][0], planes[4][0], planes[4][1], planes[4][2], planes[4][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), points[1][0]);
	if (numpoints < 3)
		return;
	numpoints = PolygonF_Clip(numpoints, points[1][0], planes[5][0], planes[5][1], planes[5][2], planes[5][3], 1.0f/64.0f, sizeof(points[0])/sizeof(points[0][0]), v[0]);
	if (numpoints < 3)
		return;
	// some part of the triangle survived, so we have to accept it...
	if (dynamic)
	{
		// dynamic always uses the original triangle
		numpoints = 3;
		for (cornerindex = 0;cornerindex < 3;cornerindex++)
		{
			index = 3*e[cornerindex];
			VectorCopy(vertex3f + index, v[cornerindex]);
		}
	}
	for (cornerindex = 0;cornerindex < numpoints;cornerindex++)
	{
		// convert vertex positions to texcoords
		Matrix4x4_Transform(projection, v[cornerindex], temp);
		tc[cornerindex][0] = (temp[1]+1.0f)*0.5f * (s2-s1) + s1;
		tc[cornerindex][1] = (temp[2]+1.0f)*0.5f * (t2-t1) + t1;
		// calculate distance fade from the projection origin
		f = a * (1.0f-fabs(temp[0])) * cl_decals_newsystem_intensitymultiplier.value;
		f = bound(0.0f, f, 1.0f);
		c[cornerindex][0] = r * f;
		c[cornerindex][1] = g * f;
		c[cornerindex][2] = b * f;
		c[cornerindex][3] = 1.0f;
		//VectorMA(v[cornerindex], cl_decals_bias.value, localnormal, v[cornerindex]);
	}
	if (dynamic)
		R_DecalSystem_SpawnTriangle(decalsystem, v[0], v[1], v[2], tc[0], tc[1], tc[2], c[0], c[1], c[2], triangleindex, surfaceindex, decalsequence);
	else
		for (cornerindex = 0;cornerindex < numpoints-2;cornerindex++)
			R_DecalSystem_SpawnTriangle(decalsystem, v[0], v[cornerindex+1], v[cornerindex+2], tc[0], tc[cornerindex+1], tc[cornerindex+2], c[0], c[cornerindex+1], c[cornerindex+2], -1, surfaceindex, decalsequence);
}
static void R_DecalSystem_SplatEntity(entity_render_t *ent, const vec3_t worldorigin, const vec3_t worldnormal, float r, float g, float b, float a, float s1, float t1, float s2, float t2, float worldsize, unsigned int decalsequence)
{
	matrix4x4_t projection;
	decalsystem_t *decalsystem;
	qbool dynamic;
	model_t *model;
	const msurface_t *surface;
	const msurface_t *surfaces;
	const texture_t *texture;
	int numtriangles;
	int surfaceindex;
	int triangleindex;
	float localorigin[3];
	float localnormal[3];
	float localmins[3];
	float localmaxs[3];
	float localsize;
	//float normal[3];
	float planes[6][4];
	float angles[3];
	bih_t *bih;
	int bih_triangles_count;
	int bih_triangles[256];
	int bih_surfaces[256];

	decalsystem = &ent->decalsystem;
	model = ent->model;
	if (!model || !ent->allowdecals || ent->alpha < 1 || (ent->flags & (RENDER_ADDITIVE | RENDER_NODEPTHTEST)))
	{
		R_DecalSystem_Reset(&ent->decalsystem);
		return;
	}

	if (!model->brush.data_leafs && !cl_decals_models.integer)
	{
		if (decalsystem->model)
			R_DecalSystem_Reset(decalsystem);
		return;
	}

	if (decalsystem->model != model)
		R_DecalSystem_Reset(decalsystem);
	decalsystem->model = model;

	RSurf_ActiveModelEntity(ent, true, false, false);

	Matrix4x4_Transform(&rsurface.inversematrix, worldorigin, localorigin);
	Matrix4x4_Transform3x3(&rsurface.inversematrix, worldnormal, localnormal);
	VectorNormalize(localnormal);
	localsize = worldsize*rsurface.inversematrixscale;
	localmins[0] = localorigin[0] - localsize;
	localmins[1] = localorigin[1] - localsize;
	localmins[2] = localorigin[2] - localsize;
	localmaxs[0] = localorigin[0] + localsize;
	localmaxs[1] = localorigin[1] + localsize;
	localmaxs[2] = localorigin[2] + localsize;

	//VectorCopy(localnormal, planes[4]);
	//VectorVectors(planes[4], planes[2], planes[0]);
	AnglesFromVectors(angles, localnormal, NULL, false);
	AngleVectors(angles, planes[0], planes[2], planes[4]);
	VectorNegate(planes[0], planes[1]);
	VectorNegate(planes[2], planes[3]);
	VectorNegate(planes[4], planes[5]);
	planes[0][3] = DotProduct(planes[0], localorigin) - localsize;
	planes[1][3] = DotProduct(planes[1], localorigin) - localsize;
	planes[2][3] = DotProduct(planes[2], localorigin) - localsize;
	planes[3][3] = DotProduct(planes[3], localorigin) - localsize;
	planes[4][3] = DotProduct(planes[4], localorigin) - localsize;
	planes[5][3] = DotProduct(planes[5], localorigin) - localsize;

#if 1
// works
{
	matrix4x4_t forwardprojection;
	Matrix4x4_CreateFromQuakeEntity(&forwardprojection, localorigin[0], localorigin[1], localorigin[2], angles[0], angles[1], angles[2], localsize);
	Matrix4x4_Invert_Simple(&projection, &forwardprojection);
}
#else
// broken
{
	float projectionvector[4][3];
	VectorScale(planes[0], ilocalsize, projectionvector[0]);
	VectorScale(planes[2], ilocalsize, projectionvector[1]);
	VectorScale(planes[4], ilocalsize, projectionvector[2]);
	projectionvector[0][0] = planes[0][0] * ilocalsize;
	projectionvector[0][1] = planes[1][0] * ilocalsize;
	projectionvector[0][2] = planes[2][0] * ilocalsize;
	projectionvector[1][0] = planes[0][1] * ilocalsize;
	projectionvector[1][1] = planes[1][1] * ilocalsize;
	projectionvector[1][2] = planes[2][1] * ilocalsize;
	projectionvector[2][0] = planes[0][2] * ilocalsize;
	projectionvector[2][1] = planes[1][2] * ilocalsize;
	projectionvector[2][2] = planes[2][2] * ilocalsize;
	projectionvector[3][0] = -(localorigin[0]*projectionvector[0][0]+localorigin[1]*projectionvector[1][0]+localorigin[2]*projectionvector[2][0]);
	projectionvector[3][1] = -(localorigin[0]*projectionvector[0][1]+localorigin[1]*projectionvector[1][1]+localorigin[2]*projectionvector[2][1]);
	projectionvector[3][2] = -(localorigin[0]*projectionvector[0][2]+localorigin[1]*projectionvector[1][2]+localorigin[2]*projectionvector[2][2]);
	Matrix4x4_FromVectors(&projection, projectionvector[0], projectionvector[1], projectionvector[2], projectionvector[3]);
}
#endif

	dynamic = model->surfmesh.isanimated;
	surfaces = model->data_surfaces;

	bih = NULL;
	bih_triangles_count = -1;
	if(!dynamic)
	{
		if(model->render_bih.numleafs)
			bih = &model->render_bih;
		else if(model->collision_bih.numleafs)
			bih = &model->collision_bih;
	}
	if(bih)
		bih_triangles_count = BIH_GetTriangleListForBox(bih, sizeof(bih_triangles) / sizeof(*bih_triangles), bih_triangles, bih_surfaces, localmins, localmaxs);
	if(bih_triangles_count == 0)
		return;
	if(bih_triangles_count > (int) (sizeof(bih_triangles) / sizeof(*bih_triangles))) // hit too many, likely bad anyway
		return;
	if(bih_triangles_count > 0)
	{
		for (triangleindex = 0; triangleindex < bih_triangles_count; ++triangleindex)
		{
			surfaceindex = bih_surfaces[triangleindex];
			surface = surfaces + surfaceindex;
			texture = surface->texture;
			if (!texture)
				continue;
			if (texture->currentmaterialflags & (MATERIALFLAG_BLENDED | MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_SKY | MATERIALFLAG_SHORTDEPTHRANGE | MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION))
				continue;
			if (texture->surfaceflags & Q3SURFACEFLAG_NOMARKS)
				continue;
			R_DecalSystem_SplatTriangle(decalsystem, r, g, b, a, s1, t1, s2, t2, decalsequence, dynamic, planes, &projection, bih_triangles[triangleindex], surfaceindex);
		}
	}
	else
	{
		for (surfaceindex = model->submodelsurfaces_start;surfaceindex < model->submodelsurfaces_end;surfaceindex++)
		{
			surface = surfaces + surfaceindex;
			// check cull box first because it rejects more than any other check
			if (!dynamic && !BoxesOverlap(surface->mins, surface->maxs, localmins, localmaxs))
				continue;
			// skip transparent surfaces
			texture = surface->texture;
			if (!texture)
				continue;
			if (texture->currentmaterialflags & (MATERIALFLAG_BLENDED | MATERIALFLAG_NODEPTHTEST | MATERIALFLAG_SKY | MATERIALFLAG_SHORTDEPTHRANGE | MATERIALFLAG_WATERSHADER | MATERIALFLAG_REFRACTION))
				continue;
			if (texture->surfaceflags & Q3SURFACEFLAG_NOMARKS)
				continue;
			numtriangles = surface->num_triangles;
			for (triangleindex = 0; triangleindex < numtriangles; triangleindex++)
				R_DecalSystem_SplatTriangle(decalsystem, r, g, b, a, s1, t1, s2, t2, decalsequence, dynamic, planes, &projection, triangleindex + surface->num_firsttriangle, surfaceindex);
		}
	}
}

// do not call this outside of rendering code - use R_DecalSystem_SplatEntities instead
static void R_DecalSystem_ApplySplatEntities(const vec3_t worldorigin, const vec3_t worldnormal, float r, float g, float b, float a, float s1, float t1, float s2, float t2, float worldsize, unsigned int decalsequence)
{
	int renderentityindex;
	float worldmins[3];
	float worldmaxs[3];
	entity_render_t *ent;

	worldmins[0] = worldorigin[0] - worldsize;
	worldmins[1] = worldorigin[1] - worldsize;
	worldmins[2] = worldorigin[2] - worldsize;
	worldmaxs[0] = worldorigin[0] + worldsize;
	worldmaxs[1] = worldorigin[1] + worldsize;
	worldmaxs[2] = worldorigin[2] + worldsize;

	R_DecalSystem_SplatEntity(r_refdef.scene.worldentity, worldorigin, worldnormal, r, g, b, a, s1, t1, s2, t2, worldsize, decalsequence);

	for (renderentityindex = 0;renderentityindex < r_refdef.scene.numentities;renderentityindex++)
	{
		ent = r_refdef.scene.entities[renderentityindex];
		if (!BoxesOverlap(ent->mins, ent->maxs, worldmins, worldmaxs))
			continue;

		R_DecalSystem_SplatEntity(ent, worldorigin, worldnormal, r, g, b, a, s1, t1, s2, t2, worldsize, decalsequence);
	}
}

typedef struct r_decalsystem_splatqueue_s
{
	vec3_t worldorigin;
	vec3_t worldnormal;
	float color[4];
	float tcrange[4];
	float worldsize;
	unsigned int decalsequence;
}
r_decalsystem_splatqueue_t;

int r_decalsystem_numqueued = 0;
r_decalsystem_splatqueue_t r_decalsystem_queue[MAX_DECALSYSTEM_QUEUE];

void R_DecalSystem_SplatEntities(const vec3_t worldorigin, const vec3_t worldnormal, float r, float g, float b, float a, float s1, float t1, float s2, float t2, float worldsize)
{
	r_decalsystem_splatqueue_t *queue;

	if (r_decalsystem_numqueued == MAX_DECALSYSTEM_QUEUE)
		return;

	queue = &r_decalsystem_queue[r_decalsystem_numqueued++];
	VectorCopy(worldorigin, queue->worldorigin);
	VectorCopy(worldnormal, queue->worldnormal);
	Vector4Set(queue->color, r, g, b, a);
	Vector4Set(queue->tcrange, s1, t1, s2, t2);
	queue->worldsize = worldsize;
	queue->decalsequence = cl.decalsequence++;
}

static void R_DecalSystem_ApplySplatEntitiesQueue(void)
{
	int i;
	r_decalsystem_splatqueue_t *queue;

	for (i = 0, queue = r_decalsystem_queue;i < r_decalsystem_numqueued;i++, queue++)
		R_DecalSystem_ApplySplatEntities(queue->worldorigin, queue->worldnormal, queue->color[0], queue->color[1], queue->color[2], queue->color[3], queue->tcrange[0], queue->tcrange[1], queue->tcrange[2], queue->tcrange[3], queue->worldsize, queue->decalsequence);
	r_decalsystem_numqueued = 0;
}

extern cvar_t cl_decals_max;
static void R_DrawModelDecals_FadeEntity(entity_render_t *ent)
{
	int i;
	decalsystem_t *decalsystem = &ent->decalsystem;
	int numdecals;
	unsigned int killsequence;
	tridecal_t *decal;
	float frametime;
	float lifetime;

	if (!decalsystem->numdecals)
		return;

	if (r_showsurfaces.integer)
		return;

	if (ent->model != decalsystem->model || ent->alpha < 1 || (ent->flags & RENDER_ADDITIVE))
	{
		R_DecalSystem_Reset(decalsystem);
		return;
	}

	killsequence = cl.decalsequence - bound(1, (unsigned int) cl_decals_max.integer, cl.decalsequence);
	lifetime = cl_decals_time.value + cl_decals_fadetime.value;

	if (decalsystem->lastupdatetime)
		frametime = (r_refdef.scene.time - decalsystem->lastupdatetime);
	else
		frametime = 0;
	decalsystem->lastupdatetime = r_refdef.scene.time;
	numdecals = decalsystem->numdecals;

	for (i = 0, decal = decalsystem->decals;i < numdecals;i++, decal++)
	{
		if (decal->color4f[0][3])
		{
			decal->lived += frametime;
			if (killsequence > decal->decalsequence || decal->lived >= lifetime)
			{
				memset(decal, 0, sizeof(*decal));
				if (decalsystem->freedecal > i)
					decalsystem->freedecal = i;
			}
		}
	}
	decal = decalsystem->decals;
	while (numdecals > 0 && !decal[numdecals-1].color4f[0][3])
		numdecals--;

	// collapse the array by shuffling the tail decals into the gaps
	for (;;)
	{
		while (decalsystem->freedecal < numdecals && decal[decalsystem->freedecal].color4f[0][3])
			decalsystem->freedecal++;
		if (decalsystem->freedecal == numdecals)
			break;
		decal[decalsystem->freedecal] = decal[--numdecals];
	}

	decalsystem->numdecals = numdecals;

	if (numdecals <= 0)
	{
		// if there are no decals left, reset decalsystem
		R_DecalSystem_Reset(decalsystem);
	}
}

extern skinframe_t *decalskinframe;
static void R_DrawModelDecals_Entity(entity_render_t *ent)
{
	int i;
	decalsystem_t *decalsystem = &ent->decalsystem;
	int numdecals;
	tridecal_t *decal;
	float faderate;
	float alpha;
	float *v3f;
	float *c4f;
	float *t2f;
	const int *e;
	const unsigned char *surfacevisible = ent == r_refdef.scene.worldentity ? r_refdef.viewcache.world_surfacevisible : NULL;
	int numtris = 0;

	numdecals = decalsystem->numdecals;
	if (!numdecals)
		return;

	if (r_showsurfaces.integer)
		return;

	if (ent->model != decalsystem->model || ent->alpha < 1 || (ent->flags & RENDER_ADDITIVE))
	{
		R_DecalSystem_Reset(decalsystem);
		return;
	}

	// if the model is static it doesn't matter what value we give for
	// wantnormals and wanttangents, so this logic uses only rules applicable
	// to a model, knowing that they are meaningless otherwise
	RSurf_ActiveModelEntity(ent, false, false, false);

	decalsystem->lastupdatetime = r_refdef.scene.time;

	faderate = 1.0f / max(0.001f, cl_decals_fadetime.value);

	// update vertex positions for animated models
	v3f = decalsystem->vertex3f;
	c4f = decalsystem->color4f;
	t2f = decalsystem->texcoord2f;
	for (i = 0, decal = decalsystem->decals;i < numdecals;i++, decal++)
	{
		if (!decal->color4f[0][3])
			continue;

		if (surfacevisible && !surfacevisible[decal->surfaceindex])
			continue;

		// skip backfaces
		if (decal->triangleindex < 0 && DotProduct(r_refdef.view.origin, decal->plane) < decal->plane[3])
			continue;

		// update color values for fading decals
		if (decal->lived >= cl_decals_time.value)
			alpha = 1 - faderate * (decal->lived - cl_decals_time.value);
		else
			alpha = 1.0f;

		c4f[ 0] = decal->color4f[0][0] * alpha;
		c4f[ 1] = decal->color4f[0][1] * alpha;
		c4f[ 2] = decal->color4f[0][2] * alpha;
		c4f[ 3] = 1;
		c4f[ 4] = decal->color4f[1][0] * alpha;
		c4f[ 5] = decal->color4f[1][1] * alpha;
		c4f[ 6] = decal->color4f[1][2] * alpha;
		c4f[ 7] = 1;
		c4f[ 8] = decal->color4f[2][0] * alpha;
		c4f[ 9] = decal->color4f[2][1] * alpha;
		c4f[10] = decal->color4f[2][2] * alpha;
		c4f[11] = 1;

		t2f[0] = decal->texcoord2f[0][0];
		t2f[1] = decal->texcoord2f[0][1];
		t2f[2] = decal->texcoord2f[1][0];
		t2f[3] = decal->texcoord2f[1][1];
		t2f[4] = decal->texcoord2f[2][0];
		t2f[5] = decal->texcoord2f[2][1];

		// update vertex positions for animated models
		if (decal->triangleindex >= 0 && decal->triangleindex < rsurface.modelnumtriangles)
		{
			e = rsurface.modelelement3i + 3*decal->triangleindex;
			VectorCopy(rsurface.modelvertex3f + 3*e[0], v3f);
			VectorCopy(rsurface.modelvertex3f + 3*e[1], v3f + 3);
			VectorCopy(rsurface.modelvertex3f + 3*e[2], v3f + 6);
		}
		else
		{
			VectorCopy(decal->vertex3f[0], v3f);
			VectorCopy(decal->vertex3f[1], v3f + 3);
			VectorCopy(decal->vertex3f[2], v3f + 6);
		}

		if (r_refdef.fogenabled)
		{
			alpha = RSurf_FogVertex(v3f);
			VectorScale(c4f, alpha, c4f);
			alpha = RSurf_FogVertex(v3f + 3);
			VectorScale(c4f + 4, alpha, c4f + 4);
			alpha = RSurf_FogVertex(v3f + 6);
			VectorScale(c4f + 8, alpha, c4f + 8);
		}

		v3f += 9;
		c4f += 12;
		t2f += 6;
		numtris++;
	}

	if (numtris > 0)
	{
		r_refdef.stats[r_stat_drawndecals] += numtris;

		// now render the decals all at once
		// (this assumes they all use one particle font texture!)
		RSurf_ActiveCustomEntity(&rsurface.matrix, &rsurface.inversematrix, rsurface.ent_flags, ent->shadertime, 1, 1, 1, 1, numdecals*3, decalsystem->vertex3f, decalsystem->texcoord2f, NULL, NULL, NULL, decalsystem->color4f, numtris, decalsystem->element3i, decalsystem->element3s, false, false);
//		R_Mesh_ResetTextureState();
		R_Mesh_PrepareVertices_Generic_Arrays(numtris * 3, decalsystem->vertex3f, decalsystem->color4f, decalsystem->texcoord2f);
		GL_DepthMask(false);
		GL_DepthRange(0, 1);
		GL_PolygonOffset(rsurface.basepolygonfactor + r_polygonoffset_decals_factor.value, rsurface.basepolygonoffset + r_polygonoffset_decals_offset.value);
		GL_DepthTest(true);
		GL_CullFace(GL_NONE);
		GL_BlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		R_SetupShader_Generic(decalskinframe->base, false, false, false);
		R_Mesh_Draw(0, numtris * 3, 0, numtris, decalsystem->element3i, NULL, 0, decalsystem->element3s, NULL, 0);
	}
}

static void R_DrawModelDecals(void)
{
	int i, numdecals;

	// fade faster when there are too many decals
	numdecals = r_refdef.scene.worldentity->decalsystem.numdecals;
	for (i = 0;i < r_refdef.scene.numentities;i++)
		numdecals += r_refdef.scene.entities[i]->decalsystem.numdecals;

	R_DrawModelDecals_FadeEntity(r_refdef.scene.worldentity);
	for (i = 0;i < r_refdef.scene.numentities;i++)
		if (r_refdef.scene.entities[i]->decalsystem.numdecals)
			R_DrawModelDecals_FadeEntity(r_refdef.scene.entities[i]);

	R_DecalSystem_ApplySplatEntitiesQueue();

	numdecals = r_refdef.scene.worldentity->decalsystem.numdecals;
	for (i = 0;i < r_refdef.scene.numentities;i++)
		numdecals += r_refdef.scene.entities[i]->decalsystem.numdecals;

	r_refdef.stats[r_stat_totaldecals] += numdecals;

	if (r_showsurfaces.integer || !r_drawdecals.integer)
		return;

	R_DrawModelDecals_Entity(r_refdef.scene.worldentity);

	for (i = 0;i < r_refdef.scene.numentities;i++)
	{
		if (!r_refdef.viewcache.entityvisible[i])
			continue;
		if (r_refdef.scene.entities[i]->decalsystem.numdecals)
			R_DrawModelDecals_Entity(r_refdef.scene.entities[i]);
	}
}

static void R_DrawDebugModel(void)
{
	entity_render_t *ent = rsurface.entity;
	int j, flagsmask;
	const msurface_t *surface;
	model_t *model = ent->model;

	if (!sv.active  && !cls.demoplayback && ent != r_refdef.scene.worldentity)
		return;

	if (r_showoverdraw.value > 0)
	{
		float c = r_refdef.view.colorscale * r_showoverdraw.value * 0.125f;
		flagsmask = MATERIALFLAG_SKY | MATERIALFLAG_WALL;
		R_SetupShader_Generic_NoTexture(false, false);
		GL_DepthTest(false);
		GL_DepthMask(false);
		GL_DepthRange(0, 1);
		GL_BlendFunc(GL_ONE, GL_ONE);
		for (j = model->submodelsurfaces_start;j < model->submodelsurfaces_end;j++)
		{
			if (ent == r_refdef.scene.worldentity && !r_refdef.viewcache.world_surfacevisible[j])
				continue;
			surface = model->data_surfaces + j;
			rsurface.texture = R_GetCurrentTexture(surface->texture);
			if ((rsurface.texture->currentmaterialflags & flagsmask) && surface->num_triangles)
			{
				RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_NOGAPS, 1, &surface);
				GL_CullFace((rsurface.texture->currentmaterialflags & MATERIALFLAG_NOCULLFACE) ? GL_NONE : r_refdef.view.cullface_back);
				if ((rsurface.texture->currentmaterialflags & MATERIALFLAG_BLENDED))
					GL_Color(c, 0, 0, 1.0f);
				else if (ent == r_refdef.scene.worldentity)
					GL_Color(c, c, c, 1.0f);
				else
					GL_Color(0, c, 0, 1.0f);
				R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, NULL, NULL);
				RSurf_DrawBatch();
			}
		}
		rsurface.texture = NULL;
	}

	flagsmask = MATERIALFLAG_SKY | MATERIALFLAG_WALL;

//	R_Mesh_ResetTextureState();
	R_SetupShader_Generic_NoTexture(false, false);
	GL_DepthRange(0, 1);
	GL_DepthTest(!r_showdisabledepthtest.integer);
	GL_DepthMask(false);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (r_showcollisionbrushes.value > 0 && model->collision_bih.numleafs)
	{
		int triangleindex;
		int bihleafindex;
		qbool cullbox = false;
		const q3mbrush_t *brush;
		const bih_t *bih = &model->collision_bih;
		const bih_leaf_t *bihleaf;
		float vertex3f[3][3];
		GL_PolygonOffset(r_refdef.polygonfactor + r_showcollisionbrushes_polygonfactor.value, r_refdef.polygonoffset + r_showcollisionbrushes_polygonoffset.value);
		for (bihleafindex = 0, bihleaf = bih->leafs;bihleafindex < bih->numleafs;bihleafindex++, bihleaf++)
		{
			if (cullbox && R_CullFrustum(bihleaf->mins, bihleaf->maxs))
				continue;
			switch (bihleaf->type)
			{
			case BIH_BRUSH:
				brush = model->brush.data_brushes + bihleaf->itemindex;
				if (brush->colbrushf && brush->colbrushf->numtriangles)
				{
					GL_Color((bihleafindex & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 5) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 10) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, r_showcollisionbrushes.value);
					R_Mesh_PrepareVertices_Generic_Arrays(brush->colbrushf->numpoints, brush->colbrushf->points->v, NULL, NULL);
					R_Mesh_Draw(0, brush->colbrushf->numpoints, 0, brush->colbrushf->numtriangles, brush->colbrushf->elements, NULL, 0, NULL, NULL, 0);
				}
				break;
			case BIH_COLLISIONTRIANGLE:
				triangleindex = bihleaf->itemindex;
				VectorCopy(model->brush.data_collisionvertex3f + 3*model->brush.data_collisionelement3i[triangleindex*3+0], vertex3f[0]);
				VectorCopy(model->brush.data_collisionvertex3f + 3*model->brush.data_collisionelement3i[triangleindex*3+1], vertex3f[1]);
				VectorCopy(model->brush.data_collisionvertex3f + 3*model->brush.data_collisionelement3i[triangleindex*3+2], vertex3f[2]);
				GL_Color((bihleafindex & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 5) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 10) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, r_showcollisionbrushes.value);
				R_Mesh_PrepareVertices_Generic_Arrays(3, vertex3f[0], NULL, NULL);
				R_Mesh_Draw(0, 3, 0, 1, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
				break;
			case BIH_RENDERTRIANGLE:
				triangleindex = bihleaf->itemindex;
				VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+0], vertex3f[0]);
				VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+1], vertex3f[1]);
				VectorCopy(model->surfmesh.data_vertex3f + 3*model->surfmesh.data_element3i[triangleindex*3+2], vertex3f[2]);
				GL_Color((bihleafindex & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 5) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, ((bihleafindex >> 10) & 31) * (1.0f / 32.0f) * r_refdef.view.colorscale, r_showcollisionbrushes.value);
				R_Mesh_PrepareVertices_Generic_Arrays(3, vertex3f[0], NULL, NULL);
				R_Mesh_Draw(0, 3, 0, 1, polygonelement3i, NULL, 0, polygonelement3s, NULL, 0);
				break;
			}
		}
	}

	GL_PolygonOffset(r_refdef.polygonfactor, r_refdef.polygonoffset);

#ifndef USE_GLES2
	if (r_showtris.value > 0 && qglPolygonMode)
	{
		if (r_showdisabledepthtest.integer)
		{
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GL_DepthMask(false);
		}
		else
		{
			GL_BlendFunc(GL_ONE, GL_ZERO);
			GL_DepthMask(true);
		}
		qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);CHECKGLERROR
		for (j = model->submodelsurfaces_start; j < model->submodelsurfaces_end; j++)
		{
			if (ent == r_refdef.scene.worldentity && !r_refdef.viewcache.world_surfacevisible[j])
				continue;
			surface = model->data_surfaces + j;
			rsurface.texture = R_GetCurrentTexture(surface->texture);
			if ((rsurface.texture->currentmaterialflags & flagsmask) && surface->num_triangles)
			{
				RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR | BATCHNEED_NOGAPS, 1, &surface);
				if ((rsurface.texture->currentmaterialflags & MATERIALFLAG_BLENDED))
					GL_Color(r_refdef.view.colorscale, 0, 0, r_showtris.value);
				else if (ent == r_refdef.scene.worldentity)
					GL_Color(r_refdef.view.colorscale, r_refdef.view.colorscale, r_refdef.view.colorscale, r_showtris.value);
				else
					GL_Color(0, r_refdef.view.colorscale, 0, r_showtris.value);
				R_Mesh_PrepareVertices_Generic_Arrays(rsurface.batchnumvertices, rsurface.batchvertex3f, NULL, NULL);
				RSurf_DrawBatch();
			}
		}
		qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);CHECKGLERROR
		rsurface.texture = NULL;
	}

# if 0
	// FIXME!  implement r_shownormals with just triangles
	if (r_shownormals.value != 0 && qglBegin)
	{
		int l, k;
		vec3_t v;
		if (r_showdisabledepthtest.integer)
		{
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GL_DepthMask(false);
		}
		else
		{
			GL_BlendFunc(GL_ONE, GL_ZERO);
			GL_DepthMask(true);
		}
		for (j = model->submodelsurfaces_start; j < model->submodelsurfaces_end; j++)
		{
			if (ent == r_refdef.scene.worldentity && !r_refdef.viewcache.world_surfacevisible[j])
				continue;
			surface = model->data_surfaces + j;
			rsurface.texture = R_GetCurrentTexture(surface->texture);
			if ((rsurface.texture->currentmaterialflags & flagsmask) && surface->num_triangles)
			{
				RSurf_PrepareVerticesForBatch(BATCHNEED_ARRAY_VERTEX | BATCHNEED_ARRAY_NORMAL | BATCHNEED_ARRAY_VECTOR | BATCHNEED_NOGAPS, 1, &surface);
				qglBegin(GL_LINES);
				if (r_shownormals.value < 0 && rsurface.batchnormal3f)
				{
					for (k = 0, l = rsurface.batchfirstvertex;k < rsurface.batchnumvertices;k++, l++)
					{
						VectorCopy(rsurface.batchvertex3f + l * 3, v);
						GL_Color(0, 0, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
						VectorMA(v, -r_shownormals.value, rsurface.batchnormal3f + l * 3, v);
						GL_Color(r_refdef.view.colorscale, r_refdef.view.colorscale, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
					}
				}
				if (r_shownormals.value > 0 && rsurface.batchsvector3f)
				{
					for (k = 0, l = rsurface.batchfirstvertex;k < rsurface.batchnumvertices;k++, l++)
					{
						VectorCopy(rsurface.batchvertex3f + l * 3, v);
						GL_Color(r_refdef.view.colorscale, 0, 0, 1);
						qglVertex3f(v[0], v[1], v[2]);
						VectorMA(v, r_shownormals.value, rsurface.batchsvector3f + l * 3, v);
						GL_Color(r_refdef.view.colorscale, r_refdef.view.colorscale, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
					}
				}
				if (r_shownormals.value > 0 && rsurface.batchtvector3f)
				{
					for (k = 0, l = rsurface.batchfirstvertex;k < rsurface.batchnumvertices;k++, l++)
					{
						VectorCopy(rsurface.batchvertex3f + l * 3, v);
						GL_Color(0, r_refdef.view.colorscale, 0, 1);
						qglVertex3f(v[0], v[1], v[2]);
						VectorMA(v, r_shownormals.value, rsurface.batchtvector3f + l * 3, v);
						GL_Color(r_refdef.view.colorscale, r_refdef.view.colorscale, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
					}
				}
				if (r_shownormals.value > 0 && rsurface.batchnormal3f)
				{
					for (k = 0, l = rsurface.batchfirstvertex;k < rsurface.batchnumvertices;k++, l++)
					{
						VectorCopy(rsurface.batchvertex3f + l * 3, v);
						GL_Color(0, 0, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
						VectorMA(v, r_shownormals.value, rsurface.batchnormal3f + l * 3, v);
						GL_Color(r_refdef.view.colorscale, r_refdef.view.colorscale, r_refdef.view.colorscale, 1);
						qglVertex3f(v[0], v[1], v[2]);
					}
				}
				qglEnd();
				CHECKGLERROR
			}
		}
		rsurface.texture = NULL;
	}
# endif
#endif
}

int r_maxsurfacelist = 0;
const msurface_t **r_surfacelist = NULL;
void R_DrawModelSurfaces(entity_render_t *ent, qbool skysurfaces, qbool writedepth, qbool depthonly, qbool debug, qbool prepass, qbool ui)
{
	int i, j, flagsmask;
	model_t *model = ent->model;
	msurface_t *surfaces;
	unsigned char *update;
	int numsurfacelist = 0;
	if (model == NULL)
		return;

	if (r_maxsurfacelist < model->num_surfaces)
	{
		r_maxsurfacelist = model->num_surfaces;
		if (r_surfacelist)
			Mem_Free((msurface_t **)r_surfacelist);
		r_surfacelist = (const msurface_t **) Mem_Alloc(r_main_mempool, r_maxsurfacelist * sizeof(*r_surfacelist));
	}

	if (r_showsurfaces.integer && r_showsurfaces.integer != 3)
		RSurf_ActiveModelEntity(ent, false, false, false);
	else if (prepass)
		RSurf_ActiveModelEntity(ent, true, true, true);
	else if (depthonly)
		RSurf_ActiveModelEntity(ent, model->wantnormals, model->wanttangents, false);
	else
		RSurf_ActiveModelEntity(ent, true, true, false);

	surfaces = model->data_surfaces;
	update = model->brushq1.lightmapupdateflags;

	flagsmask = skysurfaces ? MATERIALFLAG_SKY : MATERIALFLAG_WALL;

	if (debug)
	{
		R_DrawDebugModel();
		rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
		return;
	}

	// check if this is an empty model
	if (model->submodelsurfaces_start >= model->submodelsurfaces_end)
		return;

	rsurface.lightmaptexture = NULL;
	rsurface.deluxemaptexture = NULL;
	rsurface.uselightmaptexture = false;
	rsurface.texture = NULL;
	rsurface.rtlight = NULL;
	numsurfacelist = 0;

	// add visible surfaces to draw list
	if (ent == r_refdef.scene.worldentity)
	{
		// for the world entity, check surfacevisible
		for (i = model->submodelsurfaces_start;i < model->submodelsurfaces_end;i++)
		{
			j = model->modelsurfaces_sorted[i];
			if (r_refdef.viewcache.world_surfacevisible[j])
				r_surfacelist[numsurfacelist++] = surfaces + j;
		}

		// don't do anything if there were no surfaces added (none of the world entity is visible)
		if (!numsurfacelist)
		{
			rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
			return;
		}
	}
	else if (ui)
	{
		// for ui we have to preserve the order of surfaces (not using modelsurfaces_sorted)
		for (i = model->submodelsurfaces_start; i < model->submodelsurfaces_end; i++)
			r_surfacelist[numsurfacelist++] = surfaces + i;
	}
	else
	{
		// add all surfaces
		for (i = model->submodelsurfaces_start; i < model->submodelsurfaces_end; i++)
			r_surfacelist[numsurfacelist++] = surfaces + model->modelsurfaces_sorted[i];
	}

	/*
	 * Mark lightmaps as dirty if their lightstyle's value changed. We do this by
	 * using style chains because most styles do not change on most frames, and most
	 * surfaces do not have styles on them. Mods like Arcane Dimensions (e.g. ad_necrokeep)
	 * break this rule and animate most surfaces.
	 */
	if (update && !skysurfaces && !depthonly && !prepass && model->brushq1.num_lightstyles && r_refdef.scene.lightmapintensity > 0 && r_q1bsp_lightmap_updates_enabled.integer)
	{
		model_brush_lightstyleinfo_t *style;

		// For each lightstyle, check if its value changed and mark the lightmaps as dirty if so
		for (i = 0, style = model->brushq1.data_lightstyleinfo; i < model->brushq1.num_lightstyles; i++, style++)
		{
			if (style->value != r_refdef.scene.lightstylevalue[style->style])
			{
				int* list = style->surfacelist;
				style->value = r_refdef.scene.lightstylevalue[style->style];
				// Value changed - mark the surfaces belonging to this style chain as dirty
				for (j = 0; j < style->numsurfaces; j++)
					update[list[j]] = true;
			}
		}
		// Now check if update flags are set on any surfaces that are visible
		if (r_q1bsp_lightmap_updates_hidden_surfaces.integer)
		{
			/*
			 * We can do less frequent texture uploads (approximately 10hz for animated
			 * lightstyles) by rebuilding lightmaps on surfaces that are not currently visible.
			 * For optimal efficiency, this includes the submodels of the worldmodel, so we
			 * use model->num_surfaces, not nummodelsurfaces.
			 */
			for (i = 0; i < model->num_surfaces;i++)
				if (update[i])
					R_BuildLightMap(ent, surfaces + i, r_q1bsp_lightmap_updates_combine.integer);
		}
		else
		{
			for (i = 0; i < numsurfacelist; i++)
				if (update[r_surfacelist[i] - surfaces])
					R_BuildLightMap(ent, (msurface_t *)r_surfacelist[i], r_q1bsp_lightmap_updates_combine.integer);
		}
	}

	R_QueueModelSurfaceList(ent, numsurfacelist, r_surfacelist, flagsmask, writedepth, depthonly, prepass, ui);

	// add to stats if desired
	if (r_speeds.integer && !skysurfaces && !depthonly)
	{
		r_refdef.stats[r_stat_entities_surfaces] += numsurfacelist;
		for (j = 0;j < numsurfacelist;j++)
			r_refdef.stats[r_stat_entities_triangles] += r_surfacelist[j]->num_triangles;
	}

	rsurface.entity = NULL; // used only by R_GetCurrentTexture and RSurf_ActiveModelEntity
}

void R_DebugLine(vec3_t start, vec3_t end)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	float offsetx, offsety, x1, y1, x2, y2, width = 1.0f;
	float r1 = 1.0f, g1 = 0.0f, b1 = 0.0f, alpha1 = 0.25f;
	float r2 = 1.0f, g2 = 1.0f, b2 = 0.0f, alpha2 = 0.25f;
	vec4_t w[2], s[2];

	// transform to screen coords first
	Vector4Set(w[0], start[0], start[1], start[2], 1);
	Vector4Set(w[1], end[0], end[1], end[2], 1);
	R_Viewport_TransformToScreen(&r_refdef.view.viewport, w[0], s[0]);
	R_Viewport_TransformToScreen(&r_refdef.view.viewport, w[1], s[1]);
	x1 = s[0][0] * vid_conwidth.value / vid.mode.width;
	y1 = (vid.mode.height - s[0][1]) * vid_conheight.value / vid.mode.height;
	x2 = s[1][0] * vid_conwidth.value / vid.mode.width;
	y2 = (vid.mode.height - s[1][1]) * vid_conheight.value / vid.mode.height;
	//Con_DPrintf("R_DebugLine: %.0f,%.0f to %.0f,%.0f\n", x1, y1, x2, y2);

	// add the line to the UI mesh for drawing later

	// width is measured in real pixels
	if (fabs(x2 - x1) > fabs(y2 - y1))
	{
		offsetx = 0;
		offsety = 0.5f * width * vid_conheight.value / vid.mode.height;
	}
	else
	{
		offsetx = 0.5f * width * vid_conwidth.value / vid.mode.width;
		offsety = 0;
	}
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, "white", 0, 0, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x1 - offsetx, y1 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r1, g1, b1, alpha1);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x2 - offsetx, y2 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r2, g2, b2, alpha2);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x2 + offsetx, y2 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r2, g2, b2, alpha2);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x1 + offsetx, y1 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r1, g1, b1, alpha1);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);

}


void R_DrawCustomSurface(skinframe_t *skinframe, const matrix4x4_t *texmatrix, int materialflags, int firstvertex, int numvertices, int firsttriangle, int numtriangles, qbool writedepth, qbool prepass, qbool ui)
{
	static texture_t texture;

	// fake enough texture and surface state to render this geometry

	texture.update_lastrenderframe = -1; // regenerate this texture
	texture.basematerialflags = materialflags | MATERIALFLAG_CUSTOMSURFACE | MATERIALFLAG_WALL;
	texture.basealpha = 1.0f;
	texture.currentskinframe = skinframe;
	texture.currenttexmatrix = *texmatrix; // requires MATERIALFLAG_CUSTOMSURFACE
	texture.offsetmapping = OFFSETMAPPING_OFF;
	texture.offsetscale = 1;
	texture.specularscalemod = 1;
	texture.specularpowermod = 1;
	texture.transparentsort = TRANSPARENTSORT_DISTANCE;

	R_DrawCustomSurface_Texture(&texture, texmatrix, materialflags, firstvertex, numvertices, firsttriangle, numtriangles, writedepth, prepass, ui);
}

void R_DrawCustomSurface_Texture(texture_t *texture, const matrix4x4_t *texmatrix, int materialflags, int firstvertex, int numvertices, int firsttriangle, int numtriangles, qbool writedepth, qbool prepass, qbool ui)
{
	static msurface_t surface;
	const msurface_t *surfacelist = &surface;

	// fake enough texture and surface state to render this geometry
	surface.texture = texture;
	surface.num_triangles = numtriangles;
	surface.num_firsttriangle = firsttriangle;
	surface.num_vertices = numvertices;
	surface.num_firstvertex = firstvertex;

	// now render it
	rsurface.texture = R_GetCurrentTexture(surface.texture);
	rsurface.lightmaptexture = NULL;
	rsurface.deluxemaptexture = NULL;
	rsurface.uselightmaptexture = false;
	R_DrawModelTextureSurfaceList(1, &surfacelist, writedepth, prepass, ui);
}
