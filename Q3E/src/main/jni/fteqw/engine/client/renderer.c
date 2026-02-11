#include "quakedef.h"
#include "winquake.h"
#include "pr_common.h"
#include "gl_draw.h"
#include "shader.h"
#include "glquake.h"
#include "vr.h"
#include <string.h>

#ifdef __GLIBC__
#include <malloc.h>	//for malloc_trim
#endif


#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_BPP 32

refdef_t	r_refdef;
vec3_t		r_origin, vpn, vright, vup;
entity_t	r_worldentity;
entity_t	*currententity;	//nnggh
int			r_framecount;
qboolean	r_forceheadless;
struct texture_s	*r_notexture_mip;

int	r_blockvidrestart;	//'block' is a bit of a misnomer. 0=filesystem, configs, cinematics, video are all okay as they are. 1=starting up, waiting for filesystem, will restart after. 2=configs execed, but still need cinematics. 3=video will be restarted without any other init needed
int r_regsequence;

int rspeeds[RSPEED_MAX];
int rquant[RQUANT_MAX];

static void R_RegisterBuiltinRenderers(void);
void R_InitParticleTexture (void);
void R_RestartRenderer (rendererstate_t *newr);
static void R_UpdateRendererOpts(void);

qboolean vid_isfullscreen;

#define VIDCOMMANDGROUP "Video config"
#define GRAPHICALNICETIES "Graphical Nicaties"	//or eyecandy, which ever you prefer.
#define SCREENOPTIONS	"Screen Options"

#define GLRENDEREROPTIONS	"GL Renderer Options" //fixme: often used for generic cvars that apply to more than just gl...
#define D3DRENDEREROPTIONS	"D3D Renderer Options"

unsigned int	d_8to24rgbtable[256];
unsigned int	d_8to24srgbtable[256];
unsigned int	d_8to24bgrtable[256];
unsigned int	d_quaketo24srgbtable[256];

extern int gl_anisotropy_factor;

// callbacks used for cvars
void QDECL SCR_Viewsize_Callback (struct cvar_s *var, char *oldvalue);
void QDECL SCR_Fov_Callback (struct cvar_s *var, char *oldvalue);
void QDECL Image_TextureMode_Callback (struct cvar_s *var, char *oldvalue);
static void QDECL R_Lightmap_Format_Changed(struct cvar_s *var, char *oldvalue)
{
	if (qrenderer)
		Surf_BuildLightmaps();
}
static void QDECL R_HDR_FramebufferFormat_Changed(struct cvar_s *var, char *oldvalue)
{
	int i;
	char *e;
	for (i = 0; i < PTI_MAX; i++)
	{
		if (!Q_strcasecmp(var->string, Image_FormatName(i)))
		{
			var->ival = -i;
			return;
		}
	}
	var->ival = strtol(var->string, &e, 0);
	if (*e && e == var->string)
		Con_Printf("%s set to unknown image format\n", var->name);
	if (var->ival < 0)
		var->ival = 0;
}
static void QDECL R_ClearColour_Changed(struct cvar_s *var, char *oldvalue)
{	//just exists to force ival=0 when string=="R G B", so we don't have to do it every frame.
	//don't bother baking the palette. that isn't quite robust when vid_reloading etc.
	char *e;
	strtol(var->string, &e, 0);
	if (*e)
		var->ival = 0;	//junk at the end means its an RGB value instead of a simple palette index.
}

#ifdef FTE_TARGET_WEB	//webgl sucks too much to get a stable framerate without vsync.
cvar_t vid_vsync							= CVARAF  ("vid_vsync", "1",
													   "vid_wait", CVAR_ARCHIVE);
#else
cvar_t vid_vsync							= CVARAF  ("vid_vsync", "0",
													   "vid_wait", CVAR_ARCHIVE);
#endif

cvar_t in_windowed_mouse						= CVARF ("in_windowed_mouse","1",
													 CVAR_ARCHIVE);	//renamed this, because of freecs users complaining that it doesn't work. I don't personally see why you'd want it set to 0, but that's winquake's default so boo hiss to that.

cvar_t con_ocranaleds						= CVAR  ("con_ocranaleds", "2");

cvar_t cl_cursor							= CVAR  ("cl_cursor", "");
cvar_t cl_cursorscale						= CVAR  ("cl_cursor_scale", "1.0");
cvar_t cl_cursorbiasx						= CVAR  ("cl_cursor_bias_x", "0.0");
cvar_t cl_cursorbiasy						= CVAR  ("cl_cursor_bias_y", "0.0");

#ifdef QWSKINS
cvar_t gl_nocolors							= CVARFD  ("gl_nocolors", "0", CVAR_ARCHIVE, "Ignores player colours and skins, reducing texture memory usage at the cost of not knowing whether you're killing your team mates.");
#endif
cvar_t gl_part_flame						= CVARFD  ("gl_part_flame", "1", CVAR_ARCHIVE, "Enable particle emitting from models. Mainly used for torch and flame effects.");

//opengl library, blank means try default.
static cvar_t gl_driver						= CVARFD ("gl_driver", "", CVAR_ARCHIVE | CVAR_VIDEOLATCH, "Specifies the graphics driver name to load. This is typically a filename. Blank for default.");
cvar_t vid_devicename						= CVARFD ("vid_devicename", "", CVAR_ARCHIVE | CVAR_VIDEOLATCH, "Specifies which video device to try to use. If blank or invalid then one will be guessed.");
cvar_t gl_shadeq1_name						= CVARD  ("gl_shadeq1_name", "*", "Rename all surfaces from quake1 bsps using this pattern for the purposes of shader names.");
extern cvar_t r_vertexlight;
extern cvar_t r_forceprogramify;
extern cvar_t r_glsl_precache;
extern cvar_t r_halfrate;
extern cvar_t dpcompat_nopremulpics;
#ifdef PSKMODELS
cvar_t dpcompat_psa_ungroup					= CVAR  ("dpcompat_psa_ungroup", "0");
#endif

#ifdef HAVE_LEGACY
cvar_t r_ignoreentpvs						= CVARD ("r_ignoreentpvs", "1", "Disables pvs culling of entities that have been submitted to the renderer.");
#else
cvar_t r_ignoreentpvs						= CVARD ("r_ignoreentpvs", "0", "Disables pvs culling of entities that have been submitted to the renderer.");
#endif

cvar_t mod_md3flags							= CVARD  ("mod_md3flags", "1", "The flags field of md3s was never officially defined. If this is set to 1, the flags will be treated identically to mdl files. Otherwise they will be ignored. Naturally, this is required to provide rotating pickups in quake.");

cvar_t r_ambient							= CVARF ("r_ambient", "0",
													CVAR_CHEAT);
cvar_t r_bloodstains						= CVARF  ("r_bloodstains", "1", CVAR_ARCHIVE);
cvar_t r_bouncysparks						= CVARFD ("r_bouncysparks", "1",
													CVAR_ARCHIVE,
													"Enables particle interaction with world surfaces, allowing for bouncy particles, stains, and decals.");
cvar_t r_drawentities						= CVARFD  ("r_drawentities", "1", CVAR_CHEAT, "Controls whether to draw entities or not.\n0: Draw no entities.\n1: Draw everything as normal.\n2: Draw everything but bmodels.\n3: Draw bmodels only.");
cvar_t r_max_gpu_bones						= CVARD  ("r_max_gpu_bones", "", "Specifies the maximum number of bones that can be handled on the GPU. If empty, will guess.");
cvar_t r_drawflat							= CVARAF ("r_drawflat", "0", "gl_textureless",
													CVAR_ARCHIVE | CVAR_SEMICHEAT | CVAR_RENDERERCALLBACK | CVAR_SHADERSYSTEM);
cvar_t r_lightmap							= CVARF ("r_lightmap", "0",
													CVAR_ARCHIVE | CVAR_SEMICHEAT | CVAR_RENDERERCALLBACK | CVAR_SHADERSYSTEM);
cvar_t r_wireframe							= CVARAFD ("r_showtris", "0",
													"r_wireframe", CVAR_CHEAT, "Developer feature where everything is drawn with wireframe over the top. Only active where cheats are permitted.");
cvar_t r_outline							= CVARD ("gl_outline", "0", "Draw some stylised outlines.");
cvar_t r_outline_width						= CVARD ("gl_outline_width", "2", "The width of those outlines.");
cvar_t r_wireframe_smooth					= CVAR ("r_wireframe_smooth", "0");
cvar_t r_refract_fbo						= CVARD ("r_refract_fbo", "1", "Use an fbo for refraction. If 0, just renders as a portal and uses a copy of the current framebuffer.");
cvar_t r_refractreflect_scale				= CVARD ("r_refractreflect_scale", "0.5", "Use a different scale for refraction and reflection texturemaps. Because $reasons.");
cvar_t r_drawviewmodel						= CVARF  ("r_drawviewmodel", "1", CVAR_ARCHIVE);
cvar_t r_drawviewmodelinvis					= CVAR  ("r_drawviewmodelinvis", "0");
cvar_t r_dynamic							= CVARFD ("r_dynamic", IFMINIMAL("0","1"),
													  CVAR_ARCHIVE, "0: no standard dlights at all.\n1: coloured dlights will be used, they may show through walls. These are not realtime things.\n2: The dlights will be forced to monochrome (this does not affect coronas/flashblends/rtlights attached to the same light).");
extern cvar_t r_temporalscenecache;
cvar_t r_fastturb							= CVARF ("r_fastturb", "0",
													CVAR_SHADERSYSTEM);
cvar_t r_fb_bmodels							= CVARAFD("r_fb_bmodels", "1",
													"gl_fb_bmodels", CVAR_SEMICHEAT|CVAR_RENDERERLATCH, "Enables loading lumas on the map, as well as any external bsp models.");
cvar_t r_fb_models							= CVARAFD  ("r_fb_models", "1",
													"gl_fb_models", CVAR_SEMICHEAT, "Enables the use of lumas on models. Note that if ruleset_allow_fbmodels is enabled, then all models are unconditionally fullbright in deathmatch, because cheaters would set up their models like that anyway, hurrah for beating them at their own game. QuakeWorld players suck.");
cvar_t gl_overbright_models					= CVARFD("gl_overbright_models", "0", CVAR_SEMICHEAT|CVAR_ARCHIVE, "Doubles the brightness of models, to match QuakeSpasm's misfeature of the same name.");
//cvar_t r_skin_overlays						= CVARF  ("r_skin_overlays", "1",
//													CVAR_SEMICHEAT|CVAR_RENDERERLATCH);
cvar_t r_globalskin_first					= CVARFD  ("r_globalskin_first", "100", CVAR_RENDERERLATCH, "Specifies the first .skin value that is a global skin. Entities within this range will use the shader/image called 'gfx/skinSKIN.lmp' instead of their regular skin. See also: r_globalskin_count.");
cvar_t r_globalskin_count					= CVARFD  ("r_globalskin_count", "10", CVAR_RENDERERLATCH, "Specifies how many globalskins there are.");
cvar_t r_coronas							= CVARFD ("r_coronas", "0",	CVAR_ARCHIVE, "Draw coronas on realtime lights. Overrides glquake-esque flashblends.");
cvar_t r_coronas_intensity					= CVARFD ("r_coronas_intensity", "1",	CVAR_ARCHIVE, "Alternative intensity multiplier for coronas.");
cvar_t r_coronas_occlusion					= CVARFD ("r_coronas_occlusion", "", CVAR_ARCHIVE, "Specifies that coronas should be occluded more carefully.\n0: No occlusion, at all.\n1: BSP occlusion only (simple tracelines).\n2: non-bsp occlusion also (complex tracelines).\n3: Depthbuffer reads (forces synchronisation).\n4: occlusion queries.");
cvar_t r_coronas_mindist					= CVARFD ("r_coronas_mindist", "128", CVAR_ARCHIVE, "Coronas closer than this will be invisible, preventing near clip plane issues.");
cvar_t r_coronas_fadedist					= CVARFD ("r_coronas_fadedist", "256", CVAR_ARCHIVE, "Coronas will fade out over this distance.");

cvar_t r_flashblend							= CVARF ("gl_flashblend", "0",
													CVAR_ARCHIVE);
cvar_t r_flashblendscale					= CVARF ("gl_flashblendscale", "0.35",
													CVAR_ARCHIVE);
cvar_t r_floorcolour						= CVARAF ("r_floorcolour", "64 64 128",
													"r_floorcolor", CVAR_RENDERERCALLBACK|CVAR_SHADERSYSTEM);
//cvar_t r_floortexture						= SCVARF ("r_floortexture", "",
//												CVAR_RENDERERCALLBACK|CVAR_SHADERSYSTEM);
cvar_t r_fullbright							= CVARFD ("r_fullbright", "0",
												CVAR_CHEAT|CVAR_SHADERSYSTEM, "Ignore world lightmaps, drawing *everything* fully lit.");
cvar_t r_fullbrightSkins					= CVARFD	("r_fullbrightSkins", "0.8", /*don't default to 1, as it looks a little ugly (too bright), but don't default to 0 either because then you're handicapped in the dark*/
												CVAR_SEMICHEAT|CVAR_SHADERSYSTEM, "Force the use of fullbright skins on other players. No more hiding in the dark.");
cvar_t r_lightmap_saturation				= CVAR	("r_lightmap_saturation", "1");
cvar_t r_lightstylesmooth					= CVARF	("r_lightstylesmooth", "0", CVAR_ARCHIVE);
cvar_t r_lightstylesmooth_limit				= CVAR	("r_lightstylesmooth_limit", "2");
cvar_t r_lightstylespeed					= CVAR	("r_lightstylespeed", "10");
cvar_t r_lightstylescale					= CVAR	("r_lightstylescale", "1");
cvar_t r_lightmap_scale						= CVARFD ("r_shadow_realtime_nonworld_lightmaps", "1", 0, "Scaler for lightmaps used when not using realtime world lighting. Probably broken.");
cvar_t r_hdr_framebuffer					= CVARFCD("r_hdr_framebuffer", "0", CVAR_ARCHIVE, R_HDR_FramebufferFormat_Changed, "If enabled, the map will be rendered into a high-precision image framebuffer. This avoids issues with shaders that contribute more than 1 in any single pass (like overbrights). Can also be set to the name of an image format, to force rendering to that format first - interesting formats are L8, RGB565, B10G11R11F, and others.");
cvar_t r_hdr_irisadaptation					= CVARF	("r_hdr_irisadaptation", "0", CVAR_ARCHIVE);
cvar_t r_hdr_irisadaptation_multiplier		= CVAR	("r_hdr_irisadaptation_multiplier", "2");
cvar_t r_hdr_irisadaptation_minvalue		= CVAR	("r_hdr_irisadaptation_minvalue", "0.5");
cvar_t r_hdr_irisadaptation_maxvalue		= CVAR	("r_hdr_irisadaptation_maxvalue", "4");
cvar_t r_hdr_irisadaptation_fade_down		= CVAR	("r_hdr_irisadaptation_fade_down", "0.5");
cvar_t r_hdr_irisadaptation_fade_up			= CVAR	("r_hdr_irisadaptation_fade_up", "0.1");
#ifdef RUNTIMELIGHTING
cvar_t r_loadlits							= CVARFD("r_loadlit", "1", CVAR_ARCHIVE, "Whether to load lit files.\n0: Do not load external rgb lightmap data.\n1: Load but don't generate.\n2: Generate ldr lighting (if none found).\n3: Generate hdr lighting (if none found).\nNote that regeneration of lightmap data may be unreliable if the map was made for more advanced lighting tools.\nDeluxemap information will also be generated, as appropriate.");
#else
cvar_t r_loadlits							= CVARFD("r_loadlit", "1", CVAR_ARCHIVE, "Whether to load lit files.");
#endif
cvar_t r_menutint							= CVARF	("r_menutint", "0.68 0.4 0.13",
												CVAR_RENDERERCALLBACK);
cvar_t r_netgraph							= CVARD	("r_netgraph", "0", "Displays a graph of packet latency. A value of 2 will give additional info about what sort of data is being received from the server.");
extern cvar_t r_lerpmuzzlehack;
extern cvar_t mod_h2holey_bugged, mod_halftexel, mod_nomipmap;
cvar_t r_nolerp								= CVARF	("r_nolerp", "0", CVAR_ARCHIVE);
cvar_t r_noframegrouplerp					= CVARF	("r_noframegrouplerp", "0", CVAR_ARCHIVE);
cvar_t r_nolightdir							= CVARF	("r_nolightdir", "0", CVAR_ARCHIVE);
cvar_t r_novis								= CVARF	("r_novis", "0", CVAR_ARCHIVE);
cvar_t r_part_rain							= CVARFD ("r_part_rain", "0",
												CVAR_ARCHIVE,
												"Enable particle effects to emit off of surfaces. Mainly used for weather or lava/slime effects.");
cvar_t r_softwarebanding_cvar				= CVARFD ("r_softwarebanding", "0", CVAR_SHADERSYSTEM|CVAR_RENDERERLATCH|CVAR_ARCHIVE, "Utilise the Quake colormap in order to emulate 8bit software rendering. This results in banding as well as other artifacts that some believe adds character. Also forces nearest sampling on affected surfaces (palette indicies do not interpolate well).");
qboolean r_softwarebanding;
cvar_t r_speeds								= CVAR ("r_speeds", "0");
cvar_t r_stainfadeammount					= CVAR  ("r_stainfadeammount", "1");
cvar_t r_stainfadetime						= CVAR  ("r_stainfadetime", "1");
cvar_t r_stains								= CVARFC("r_stains", IFMINIMAL("0","0"),
												CVAR_ARCHIVE,
												Cvar_Limiter_ZeroToOne_Callback);
cvar_t r_renderscale						= CVARFD("r_renderscale", "1", CVAR_ARCHIVE, "Provides a way to enable subsampling or super-sampling");
cvar_t r_fxaa								= CVARFD("r_fxaa", "0", CVAR_ARCHIVE, "Runs a post-procesing pass to strip the jaggies.");
cvar_t r_graphics							= CVARFD("r_graphics", "1", CVAR_ARCHIVE, "Turning this off will result in ascii-style rendering.");
cvar_t r_postprocshader						= CVARD("r_postprocshader", "", "Specifies a custom shader to use as a post-processing shader");
cvar_t r_wallcolour							= CVARAF ("r_wallcolour", "128 128 128",
													  "r_wallcolor", CVAR_RENDERERCALLBACK|CVAR_SHADERSYSTEM);//FIXME: broken
//cvar_t r_walltexture						= CVARF ("r_walltexture", "",
//												CVAR_RENDERERCALLBACK|CVAR_SHADERSYSTEM);	//FIXME: broken
cvar_t r_wateralpha							= CVARF  ("r_wateralpha", "1",
												CVAR_ARCHIVE | CVAR_SHADERSYSTEM);
cvar_t r_lavaalpha							= CVARF  ("r_lavaalpha", "",
												CVAR_ARCHIVE | CVAR_SHADERSYSTEM);
cvar_t r_slimealpha							= CVARF  ("r_slimealpha", "",
												CVAR_ARCHIVE | CVAR_SHADERSYSTEM);
cvar_t r_telealpha							= CVARF  ("r_telealpha", "",
												CVAR_ARCHIVE | CVAR_SHADERSYSTEM);
cvar_t r_waterwarp							= CVARFD ("r_waterwarp", "1",
												CVAR_ARCHIVE, "Enables fullscreen warp, preferably via glsl. -1 specifies to force the fov warp fallback instead which can give a smidge more performance.");

cvar_t r_replacemodels						= CVARFD ("r_replacemodels", IFMINIMAL("","md3 md2 md5mesh"),
												CVAR_ARCHIVE, "A list of filename extensions to attempt to use instead of mdl.");

cvar_t r_lightmap_nearest					= CVARFD ("gl_lightmap_nearest", "0", CVAR_ARCHIVE, "Use nearest sampling for lightmaps. This will give a more blocky look. Meaningless when gl_lightmap_average is enabled.");
cvar_t r_lightmap_average					= CVARFD ("gl_lightmap_average", "0", CVAR_ARCHIVE, "Determine lightmap values based upon the center of the polygon. This will give a more buggy look, quite probably.");
cvar_t r_lightmap_format					= CVARFCD ("r_lightmap_format", "", CVAR_ARCHIVE, R_Lightmap_Format_Changed, "Overrides the default texture format used for lightmaps. rgb9e5 is a good choice for HDR.");

//otherwise it would defeat the point.
cvar_t scr_allowsnap						= CVARFD ("scr_allowsnap", "0",
												CVAR_NOTFROMSERVER, "Whether to allow server-requested screenshot requests to check for gpu driver hacks.");
cvar_t scr_centersbar						= CVAR  ("scr_centersbar", "2");
cvar_t scr_centertime						= CVARD  ("scr_centertime", "2", "Centerprint messages will be displayed for this long before disappearing.");
cvar_t scr_logcenterprint					= CVARD  ("con_logcenterprint", "1", "Specifies whether to print centerprints on the console.\n0: never\n1: single-player or coop only.\n2: always.");
cvar_t scr_conalpha							= CVARCD ("scr_conalpha", "0.7",
												Cvar_Limiter_ZeroToOne_Callback, "How opaque the console should be when there's still stuff going on behind.");
cvar_t scr_consize							= CVARD  ("scr_consize", "0.5", "Proportion of the screen to be covered by the console when its focused. Valid range is 0ish to 1.");
cvar_t scr_conspeed							= CVARD  ("scr_conspeed", "2000", "How fast the console careers onto the screen.");
cvar_t scr_fov_mode							= CVARFD  ("scr_fov_mode", "4", CVAR_ARCHIVE, "Controls what the fov cvar actually controls:\n0: largest axis (ultra-wide monitors means less height will be visible).\n1: smallest axis (ultra-wide monitors will distort at the edges).\n2: horizontal axis.\n3: vertical axis.\n4: 4:3 horizontal axis, padded for wider resolutions (for a more classic fov)");
cvar_t scr_fov								= CVARFCD("fov", "90", CVAR_ARCHIVE, SCR_Fov_Callback,
												"field of vision, 1-170 degrees, standard fov is 90, nquake defaults to 108.");
cvar_t scr_fov_viewmodel					= CVARFD("r_viewmodel_fov", "", CVAR_ARCHIVE,
												"field of vision, 1-170 degrees, standard fov is 90, nquake defaults to 108.");
cvar_t scr_printspeed						= CVARD  ("scr_printspeed", "16", "How rapidly each character is displayed during outro messages.");
cvar_t scr_showpause						= CVAR  ("showpause", "1");
cvar_t scr_showturtle						= CVARD  ("showturtle", "0", "Enables a low-framerate indicator.");
cvar_t scr_turtlefps						= CVAR  ("scr_turtlefps", "10");
cvar_t scr_sshot_compression				= CVARD  ("scr_sshot_compression", "75", "Requsted compression ratio as a percentage. For jpeg this is the quantisation quality and has a direct impact on image quality vs size. For png this ranges between 0 for best and 100 for worst with final image quality being unchanged because png is loseless.");
cvar_t scr_sshot_type						= CVARD  ("scr_sshot_type", "png", "This specifies the default extension(and thus file format) for screenshots.\nKnown extensions are: png, jpg/jpeg, bmp, pcx, tga, ktx, dds.");
cvar_t scr_sshot_prefix						= CVARF  ("scr_sshot_prefix", "screenshots/fte-", CVAR_NOTFROMSERVER);
cvar_t scr_viewsize							= CVARFC("viewsize", "100", CVAR_ARCHIVE, SCR_Viewsize_Callback);

#ifdef ANDROID
cvar_t vid_conautoscale						= CVARAF ("vid_conautoscale", "2",
												"scr_conscale"/*qs*/ /*"vid_conscale"ez*/, CVAR_ARCHIVE | CVAR_RENDERERCALLBACK);
#else
cvar_t vid_conautoscale						= CVARAFD ("vid_conautoscale", "0",
												"scr_conscale"/*qs*/ /*"vid_conscale"ez*/, CVAR_ARCHIVE | CVAR_RENDERERCALLBACK, "Changes the 2d scale, including hud, console, and fonts. To specify an explicit font size, divide the desired 'point' size by 8 to get the scale. High values will be clamped to maintain at least a 320*200 virtual size.");
#endif
cvar_t vid_baseheight						= CVARD ("vid_baseheight", "", "Specifies a mod's target height and used only when the 2d scale is not otherwise forced. Unlike vid_conheight the size is not fixed and will be padded to avoid inconsistent filtering.");
cvar_t vid_minsize							= CVARFD ("vid_minsize", "320 200",
												0, "Specifies a mod's minimum virtual size (to be set inside the mod's default.cfg file). The virtual size will always be at least this big. Windows may not be resized below this value either (which may reduce video mode options), but might require a vid_restart to take effect when switching between mods.");
cvar_t vid_conheight						= CVARFD ("vid_conheight", "0",
												CVAR_ARCHIVE, "Specifies the virtual height of the screen. If 0, will be inferred by vid_conwidth or other settings, according to aspect etc.");
cvar_t vid_conwidth							= CVARFD ("vid_conwidth", "0",
												CVAR_ARCHIVE | CVAR_RENDERERCALLBACK, "Specifies the virtual width of the screen. Should generally be left as 0 to allow the correct aspect to be used despite video mode changes.");
//see R_RestartRenderer_f for the effective default 'if (newr.renderer == -1)'.
cvar_t vid_renderer							= CVARFD ("vid_renderer", "",
													 CVAR_ARCHIVE | CVAR_VIDEOLATCH, "Specifies which backend is used. Values that might work are: sv (dedicated server), headless (null renderer), vk (vulkan), gl (opengl), egl (opengl es), d3d9 (direct3d 9), d3d11 (direct3d 11, with default hardware rendering), d3d11 warp (direct3d 11, with software rendering).");
cvar_t vid_renderer_opts					= CVARFD ("_vid_renderer_opts", NULL, CVAR_NOSET|CVAR_NOSAVE, "The possible video renderer apis, in \"value\" \"description\" pairs, for gamecode to read.");

cvar_t vid_bpp								= CVARFD ("vid_bpp", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH, "The number of colour bits to request from the renedering context");
cvar_t vid_depthbits						= CVARFD ("vid_depthbits", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH, "The number of depth bits to request from the renedering context. Try 24.");
cvar_t vid_desktopsettings					= CVARFD ("vid_desktopsettings", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH, "Ignore the values of vid_width and vid_height, and just use the same settings that are used for the desktop.");
#ifdef FTE_TARGET_WEB
cvar_t vid_fullscreen						= CVARF ("vid_fullscreen", "0",	CVAR_ARCHIVE|CVAR_VIDEOLATCH);
#else
cvar_t vid_fullscreen						= CVARFD ("vid_fullscreen", "2",	CVAR_ARCHIVE|CVAR_VIDEOLATCH, "Specifies whether the game should be fullscreen or not (requires vid_restart).\n0: Run in a resizable window, which can be manually maximized (with borders).\n1: Traditional fullscreen-exclusive video mode with mode switching and everything.\n2: Simply maximize the window and hide any borders without interfering with any other parts of the system.");
#endif
cvar_t vid_height							= CVARFD ("vid_height", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH, "The screen height to attempt to use, in physical pixels. 0 means use desktop resolution.");
cvar_t vid_multisample						= CVARAFD ("vid_multisample", "0", "vid_samples",
													CVAR_ARCHIVE, "The number of samples to use for Multisample AntiAliasing (aka: msaa). A value of 1 explicitly disables it.");
cvar_t vid_refreshrate						= CVARF ("vid_displayfrequency", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH);
cvar_t vid_srgb								= CVARFD ("vid_srgb", "0",
													  CVAR_ARCHIVE, "-1: Only the framebuffer should use sRGB colourspace, textures and colours will be assumed to be linear. This has the effect of just brightening the screen according to a gamma ramp of about 2.2 (or .45 in quake's backwards gamma terms).\n0: Off. Colour blending will be wrong, you're likely to see obvious banding.\n1: Use sRGB extensions/support to ensure that the lighting aproximately matches real-world lighting (required for PBR).\n2: Attempt to use a linear floating-point framebuffer, which should enable monitor support for HDR.\nNote that driver behaviour varies by a disturbing amount, and much of the documentation conflicts with itself (the term 'linear' is awkward when the eye's perception of linear is non-linear).");
cvar_t vid_wndalpha							= CVARD ("vid_wndalpha", "1", "When running windowed, specifies the window's transparency level.");
#if defined(_WIN32) && defined(MULTITHREAD)
cvar_t vid_winthread						= CVARFD ("vid_winthread", "", CVAR_ARCHIVE|CVAR_VIDEOLATCH, "When enabled, window messages will be handled by a separate thread. This allows the game to continue rendering when Microsoft Windows blocks while resizing, etc.");
#endif
//more readable defaults to match conwidth/conheight.
cvar_t vid_width							= CVARFD ("vid_width", "0",
												CVAR_ARCHIVE | CVAR_VIDEOLATCH, "The screen width to attempt to use, in physical pixels. 0 means use desktop resolution.");
cvar_t vid_dpi_x							= CVARFD ("vid_dpi_x", "0", CVAR_NOSET, "For mods that need to determine the physical screen size (like with touchscreens). 0 means unknown");
cvar_t vid_dpi_y							= CVARFD ("vid_dpi_y", "0", CVAR_NOSET, "For mods that need to determine the physical screen size (like with touchscreens). 0 means unknown");

cvar_t	r_stereo_separation					= CVARD("r_stereo_separation", "4", "How far apart your eyes are, in quake units. A non-zero value will enable stereoscoping rendering. You might need some of them retro 3d glasses. Hardware support is recommended, see r_stereo_context.");
cvar_t	r_stereo_convergence				= CVARD("r_stereo_convergence", "0", "Nudges the angle of each eye inwards when using stereoscopic rendering.");
cvar_t	r_stereo_method						= CVARFD("r_stereo_method", "0", CVAR_ARCHIVE, "Value 0 = Off.\nValue 1 = Attempt hardware acceleration. Requires vid_restart.\nValue 2 = red/cyan.\nValue 3 = red/blue.\nValue 4=red/green.\nValue 5=eye strain.");

cvar_t	r_xflip = CVAR("leftisright", "0");

extern cvar_t r_dodgytgafiles;
extern cvar_t r_dodgypcxfiles;
extern cvar_t r_keepimages;
extern cvar_t r_ignoremapprefixes;
extern cvar_t r_dodgymiptex;
extern char *r_defaultimageextensions;
extern cvar_t r_imageextensions;
extern cvar_t r_image_downloadsizelimit;
extern cvar_t r_drawentities;
extern cvar_t r_drawviewmodel;
extern cvar_t r_drawworld;
extern cvar_t r_fullbright;
cvar_t	r_mirroralpha = CVARFD("r_mirroralpha","1", CVAR_CHEAT|CVAR_SHADERSYSTEM|CVAR_RENDERERLATCH, "Specifies how the default shader is generated for the 'window02_1' texture. Values less than 1 will turn it into a mirror.");
extern cvar_t r_netgraph;
cvar_t	r_norefresh = CVAR("r_norefresh","0");
extern cvar_t r_novis;
extern cvar_t r_speeds;
extern cvar_t r_waterwarp;

#ifdef BEF_PUSHDEPTH
cvar_t	r_polygonoffset_submodel_factor = CVARD("r_polygonoffset_submodel_factor", "0", "z-fighting avoidance. Pushes submodel depth values slightly towards the camera depending on the slope of the surface.");
cvar_t	r_polygonoffset_submodel_offset = CVARD("r_polygonoffset_submodel_offset", "0", "z-fighting avoidance. Pushes submodel depth values slightly towards the camera by a consistent distance.");
cvar_t	r_polygonoffset_submodel_maps = CVARD("r_polygonoffset_submodel_map", "e?m? r?m? hip?m?", "List of maps on which z-fighting reduction should be used. wildcards accepted.");
#endif
cvar_t	r_polygonoffset_shadowmap_offset = CVAR("r_polygonoffset_shadowmap_factor", "0.05");
cvar_t	r_polygonoffset_shadowmap_factor = CVAR("r_polygonoffset_shadowmap_offset", "0");

cvar_t	r_polygonoffset_stencil_factor = CVAR("r_polygonoffset_stencil_factor", "0.01");
cvar_t	r_polygonoffset_stencil_offset = CVAR("r_polygonoffset_stencil_offset", "1");

rendererstate_t currentrendererstate;

#if defined(GLQUAKE)
cvar_t	gl_workaround_ati_shadersource		= CVARD	 ("gl_workaround_ati_shadersource", "1", "Work around ATI driver bugs in the glShaderSource function. Can safely be enabled with other drivers too.");
cvar_t	vid_gl_context_version				= CVARD  ("vid_gl_context_version", "", "Specifies the version of OpenGL to try to create.");
cvar_t	vid_gl_context_forwardcompatible	= CVARD  ("vid_gl_context_forwardcompatible", "0", "Requests an opengl context with no depricated features enabled.");
cvar_t	vid_gl_context_compatibility		= CVARD  ("vid_gl_context_compatibility", "1", "Requests an OpenGL context with fixed-function backwards compat.");
cvar_t	vid_gl_context_debug				= CVARD  ("vid_gl_context_debug", "0", "Requests a debug opengl context. This provides better error oreporting.");	//for my ati drivers, debug 1 only works if version >= 3
cvar_t	vid_gl_context_es					= CVARD  ("vid_gl_context_es", "0", "Requests an OpenGLES context. Be sure to set vid_gl_context_version to 2 or so."); //requires version set correctly, no debug, no compat
cvar_t	vid_gl_context_robustness			= CVARD	("vid_gl_context_robustness", "1", "Attempt to enforce extra buffer protection in the gl driver, but can be slower with pre-gl3 hardware.");
cvar_t	vid_gl_context_selfreset			= CVARD	("vid_gl_context_selfreset", "1", "Upon hardware failure, have the engine create a new context instead of depending on the drivers to restore everything. This can help to avoid graphics drivers randomly killing your game, and can help reduce memory requirements.");
cvar_t	vid_gl_context_noerror				= CVARD	("vid_gl_context_noerror", "", "Disables OpenGL's error checks for a small performance speedup. May cause segfaults if stuff wasn't properly implemented/tested.");

cvar_t	gl_immutable_textures				= CVARFD ("gl_immutable_textures", "1", CVAR_VIDEOLATCH, "Controls whether to use immutable GPU memory allocations for OpenGL textures. This potentially means less work for the drivers and thus higher framerates.");
cvar_t	gl_immutable_buffers				= CVARFD ("gl_immutable_buffers", "1", CVAR_VIDEOLATCH, "Controls whether to use immutable GPU memory allocations for static OpenGL vertex buffers. This potentially means less work for the drivers and thus higher framerates.");
cvar_t	gl_pbolightmaps						= CVARFD ("gl_pbolightmaps", "1", CVAR_RENDERERLATCH, "Controls whether to use PBOs for streaming lightmap updates. This prevents CPU stalls while the driver reads out the lightmap data (lightmap updates are still not free though).");
#endif

#if 1
cvar_t r_tessellation						= CVARAFD  ("r_tessellation", "0", "gl_ati_truform", CVAR_SHADERSYSTEM, "Enables+controls the use of blinn tessellation on the fallback shader for meshes, equivelent to a shader with 'program defaultskin#TESS'. This will look stupid unless the meshes were actually designed for it and have suitable vertex normals.");
cvar_t gl_ati_truform_type					= CVAR  ("gl_ati_truform_type", "1");
cvar_t r_tessellation_level					= CVAR  ("r_tessellation_level", "5");
cvar_t gl_blend2d							= CVAR  ("gl_blend2d", "1");
cvar_t gl_blendsprites						= CVARFD  ("gl_blendsprites", "0", CVAR_SHADERSYSTEM, "Specifies how sprites are blended.\n0: Alpha tested.\n1: Premultiplied blend.\n2: Additive blend.");
cvar_t r_deluxemapping_cvar					= CVARAFD ("r_deluxemapping", "1", "r_glsl_deluxemapping",
												CVAR_ARCHIVE|CVAR_RENDERERLATCH, "Enables bumpmapping based upon precomputed light directions.\n0=off\n1=use if available\n2=auto-generate (if possible)");
cvar_t mod_loadsurfenvmaps					= CVARD ("r_loadsurfenvmaps", "1", "Load local reflection environment-maps, where available. These are normally defined via env_cubemap entities dotted around the place.");
qboolean r_deluxemapping;
cvar_t r_shaderblobs						= CVARD ("r_shaderblobs", "0", "If enabled, can massively accelerate vid restarts / loading (especially with the d3d renderer). Can cause issues when upgrading engine versions, so this is disabled by default.");
cvar_t gl_compress							= CVARAFD ("gl_compress", "0", "r_ext_compressed_textures"/*q3*/, CVAR_ARCHIVE, "Enable automatic texture compression even for textures which are not pre-compressed.");
cvar_t gl_conback							= CVARFCD ("gl_conback", "",
												CVAR_RENDERERCALLBACK, R2D_Conback_Callback, "Specifies which conback shader/image to use. The Quake fallback is gfx/conback.lmp");
//cvar_t gl_detail							= CVARF ("gl_detail", "0",
//												CVAR_ARCHIVE);
//cvar_t gl_detailscale						= CVAR  ("gl_detailscale", "5");
cvar_t gl_font								= CVARFD ("gl_font", "",
													  CVAR_RENDERERCALLBACK|CVAR_ARCHIVE, "Specifies the font file to use. a value such as FONT:ALTFONT specifies an alternative font to be used when ^^a is used.\n"
													  "When using TTF fonts, you will likely need to scale text to at least 150% - vid_conautoscale 1.5 will do this.\n"
													  "TTF fonts may be loaded from your windows directory. \'gl_font cour?col=1,1,1:couri?col=0,1,0\' loads eg: c:\\windows\\fonts\\cour.ttf, and uses the italic version of courier for alternative text, with specific colour tints."
													  );
cvar_t con_textfont							= CVARAFD ("con_textfont", "", "gl_consolefont",
													  CVAR_RENDERERCALLBACK|CVAR_ARCHIVE, "Specifies the font file to use. a value such as FONT:ALTFONT specifies an alternative font to be used when ^^a is used.\n"
													  "When using TTF fonts, you will likely need to scale text to at least 150% - vid_conautoscale 1.5 will do this.\n"
													  "TTF fonts may be loaded from your windows directory. \'gl_font cour?col=1,1,1:couri?col=0,1,0\' loads eg: c:\\windows\\fonts\\cour.ttf, and uses the italic version of courier for alternative text, with specific colour tints."
													  );
cvar_t gl_lateswap							= CVAR  ("gl_lateswap", "0");
cvar_t gl_lerpimages						= CVARFD  ("gl_lerpimages", "1", CVAR_ARCHIVE, "Enables smoother resampling for images which are not power-of-two, when the drivers do not support non-power-of-two textures.");
//cvar_t gl_lightmapmode						= SCVARF("gl_lightmapmode", "",
//												CVAR_ARCHIVE);
cvar_t gl_load24bit							= CVARF ("gl_load24bit", "1",
												CVAR_ARCHIVE|CVAR_RENDERERLATCH);

cvar_t	r_clear								= CVARAF("r_clear","0",
													 "gl_clear", 0);
cvar_t	r_clearcolour						= CVARAFC("r_clearcolour", "0.12 0.12 0.12", "r_clearcolor"/*american spelling*/, 0, R_ClearColour_Changed);
cvar_t gl_max_size							= CVARFD  ("gl_max_size", "8192", CVAR_RENDERERLATCH, "Specifies the maximum texture size that the engine may use. Textures larger than this will be downsized. Clamped by the value the driver supports.");
cvar_t gl_menutint_shader					= CVARD  ("gl_menutint_shader", "1", "Controls the use of GLSL to desaturate the background when drawing the menu, like quake's dos software renderer used to do before the ugly dithering of winquake.");

//by setting to 64 or something, you can use this as a wallhack
cvar_t gl_mindist							= CVARAD ("gl_mindist", "1", "r_nearclip",
												"Distance to the near clip plane. Smaller values may damage depth precision, high values can potentialy be used to see through walls...");

cvar_t gl_motionblur						= CVARF ("gl_motionblur", "0",
												CVAR_ARCHIVE);
cvar_t gl_motionblurscale					= CVAR  ("gl_motionblurscale", "1");
cvar_t gl_overbright						= CVARFC ("gl_overbright", "1",
												CVAR_ARCHIVE,
												Surf_RebuildLightmap_Callback);
cvar_t gl_overbright_all					= CVARF ("gl_overbright_all", "0",
												CVAR_ARCHIVE);
cvar_t gl_picmip							= CVARFD  ("gl_picmip", "0", CVAR_ARCHIVE, "Reduce world/model texture sizes by some exponential factor.");
cvar_t gl_picmip2d							= CVARFD  ("gl_picmip2d", "0", CVAR_ARCHIVE, "Reduce hud/menu texture sizes by some exponential factor.");
cvar_t gl_picmip_world						= CVARFD  ("gl_picmip_world", "0", CVAR_ARCHIVE, "Effectively added to gl_picmip for the purposes of world textures.");
cvar_t gl_picmip_sprites					= CVARFD  ("gl_picmip_sprites", "0", CVAR_ARCHIVE, "Effectively added to gl_picmip for the purposes of sprite textures.");
cvar_t gl_picmip_other						= CVARFD  ("gl_picmip_other", "0", CVAR_ARCHIVE, "Effectively added to gl_picmip for the purposes of model textures.");
cvar_t gl_nohwblend							= CVARD  ("gl_nohwblend","1", "If 1, don't use hardware gamma ramps for transient effects that change each frame (does not affect long-term effects like holding quad or underwater tints).");
//cvar_t gl_schematics						= CVARD  ("gl_schematics", "0", "Gimmick rendering mode that draws the length of various world edges.");
cvar_t gl_smoothcrosshair					= CVAR  ("gl_smoothcrosshair", "1");
cvar_t	gl_maxdist							= CVARAD	("gl_maxdist", "0", "gl_farclip", "The distance of the far clip plane. If set to 0, some fancy maths will be used to place it at an infinite distance.");

#ifdef SPECULAR
cvar_t gl_specular							= CVARFD  ("gl_specular", "0.3", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Multiplier for specular effects.");
cvar_t gl_specular_power					= CVARF  ("gl_specular_power", "32", CVAR_ARCHIVE|CVAR_SHADERSYSTEM);
cvar_t gl_specular_fallback					= CVARF  ("gl_specular_fallback", "0.05", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
cvar_t gl_specular_fallbackexp				= CVARF  ("gl_specular_fallbackexp", "1", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
#endif

// The callbacks are not in D3D yet (also ugly way of seperating this)
cvar_t gl_texture_anisotropic_filtering		= CVARAFCD("gl_texture_anisotropy", "4",
												"gl_texture_anisotropic_filtering"/*old*/, CVAR_ARCHIVE | CVAR_RENDERERCALLBACK,
												Image_TextureMode_Callback, "Allows for higher quality textures on surfaces that slope away from the camera (like the floor). Set to 16 or something. Only supported with trilinear filtering.");
cvar_t gl_texturemode						= CVARAFCD("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", "r_texturemode"/*q3*/,
												CVAR_ARCHIVE | CVAR_RENDERERCALLBACK | CVAR_SAVE, Image_TextureMode_Callback,
												"Specifies how world/model textures appear. Typically 3 letters eg "S_COLOR_GREEN"nll"S_COLOR_WHITE" or "S_COLOR_GREEN"lll"S_COLOR_WHITE".\nFirst letter can be l(inear) or n(earest) and says how to upscale low-res textures (n for the classic look - often favoured for embedded textures, l for blurry - best for high-res textures).\nThe middle letter can be set to '.' to disable mipmaps, or n for ugly banding with distance, or l for smooth mipmap transitions.\nThe third letter says what to do when the texture is too high resolution, and should generally be set to 'l' to reduce sparkles including when aiming for the classic lego look.");
cvar_t gl_texture_lodbias					= CVARAFCD("d_lodbias", "0", "gl_texture_lodbias",
												CVAR_ARCHIVE | CVAR_RENDERERCALLBACK,
												Image_TextureMode_Callback, "Biases choice of mipmap levels. Positive values will give more blury textures, while negative values will give crisper images (but will also give some mid-surface aliasing artifacts).");
cvar_t gl_mipcap							= CVARAFCD("d_mipcap", "0 1000", "gl_miptexLevel",
												CVAR_ARCHIVE | CVAR_RENDERERCALLBACK,
												Image_TextureMode_Callback, "Specifies the range of mipmap levels to use.");
cvar_t gl_texturemode2d						= CVARFCD("gl_texturemode2d", "GL_LINEAR",
												CVAR_ARCHIVE | CVAR_RENDERERCALLBACK, Image_TextureMode_Callback,
												"Specifies how 2d images are sampled. format is a 3-tupple ");
cvar_t r_font_linear						= CVARF("r_font_linear", "1", CVAR_ARCHIVE);
cvar_t r_font_postprocess_outline			= CVARFD("r_font_postprocess_outline", "0", 0, "Controls the number of pixels of dark borders to use around fonts.");
cvar_t r_font_postprocess_mono				= CVARFD("r_font_postprocess_mono", "0", 0, "Disables anti-aliasing on fonts.");

#if defined(HAVE_LEGACY) && defined(AVAIL_FREETYPE)
cvar_t dpcompat_smallerfonts				= CVARFD("dpcompat_smallerfonts", "0", 0, "Mimics DP's behaviour of using a smaller font size than was actually requested.");
#endif

cvar_t vid_triplebuffer						= CVARAFD ("vid_triplebuffer", "1", "gl_triplebuffer", CVAR_ARCHIVE, "Specifies whether the hardware is forcing tripplebuffering on us, this is the number of extra page swaps required before old data has been completely overwritten.");

cvar_t r_portalrecursion					= CVARD  ("r_portalrecursion", "1", "The number of portals the camera is allowed to recurse through.");
cvar_t r_portaldrawplanes					= CVARD  ("r_portaldrawplanes", "0", "Draw front and back planes in portals. Debug feature.");
cvar_t r_portalonly							= CVARD  ("r_portalonly", "0", "Don't draw things which are not portals. Debug feature.");
cvar_t r_noaliasshadows						= CVARF ("r_noaliasshadows", "0", CVAR_ARCHIVE);
cvar_t r_lodscale							= CVARFD ("r_lodscale", "5", CVAR_ARCHIVE, "Scales the level-of-detail reduction on models (for those that have lod).");
cvar_t r_lodbias							= CVARFD ("r_lodbias", "0", CVAR_ARCHIVE, "Biases the level-of-detail on models (for those that have lod).");
cvar_t r_shadows							= CVARFD ("r_shadows", "0", CVAR_ARCHIVE, "Draw basic blob shadows underneath entities without using realtime lighting.");
cvar_t r_showbboxes							= CVARFD("r_showbboxes", "0", CVAR_CHEAT, "Debugging. Shows bounding boxes. 1=ssqc, 2=csqc. Red=solid, Green=stepping/toss/bounce, Blue=onground.");
cvar_t r_showfields							= CVARD("r_showfields", "0", "Debugging. Shows entity fields boxes (entity closest to crosshair). 1=ssqc, 2=csqc, 3=snapshots.");
cvar_t r_showshaders						= CVARD("r_showshaders", "0", "Debugging. Shows the name of the (worldmodel) shader being pointed to.");
cvar_t r_lightprepass_cvar					= CVARFD("r_lightprepass", "0", CVAR_ARCHIVE, "Experimental. Attempt to use a different lighting mechanism (aka: deferred lighting). Requires vid_reload to take effect.");
int r_lightprepass;

cvar_t r_shadow_bumpscale_basetexture		= CVARD  ("r_shadow_bumpscale_basetexture", "0", "bumpyness scaler for generation of fallback normalmap textures from models");
cvar_t r_shadow_bumpscale_bumpmap			= CVARD  ("r_shadow_bumpscale_bumpmap", "4", "bumpyness scaler for _bump textures");

cvar_t r_shadow_heightscale_basetexture		= CVARD  ("r_shadow_heightscale_basetexture", "0", "scaler for generation of height maps from legacy paletted content.");
cvar_t r_shadow_heightscale_bumpmap			= CVARD  ("r_shadow_heightscale_bumpmap", "1", "height scaler for 8bit _bump textures");

cvar_t r_glsl_pbr							= CVARFD  ("r_glsl_pbr", "0", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Force PBR shading.");
cvar_t r_glsl_offsetmapping					= CVARFD  ("r_glsl_offsetmapping", "0", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Enables the use of paralax mapping, adding fake depth to textures.");
cvar_t r_glsl_offsetmapping_scale			= CVAR  ("r_glsl_offsetmapping_scale", "0.04");
cvar_t r_glsl_offsetmapping_reliefmapping	= CVARFD("r_glsl_offsetmapping_reliefmapping", "0", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Changes the paralax sampling mode to be a bit nicer, but noticably more expensive at high resolutions. r_glsl_offsetmapping must be set.");
cvar_t r_glsl_turbscale_reflect				= CVARFD  ("r_glsl_turbscale_reflect", "1", CVAR_ARCHIVE, "Controls the strength of the water reflection ripples (used by the altwater glsl code).");
cvar_t r_glsl_turbscale_refract				= CVARFD  ("r_glsl_turbscale_refract", "1", CVAR_ARCHIVE, "Controls the strength of the underwater ripples (used by the altwater glsl code).");
cvar_t r_glsl_emissive						= CVARFD  ("r_glsl_emissive", "1", CVAR_SHADERSYSTEM, "When set, specifies that the _luma or _glow textures are emissive... When 0 they are taken as a mask for the proportion of the lightmap that will apply (for q2e compat, has issues with overbrights).");

cvar_t r_fastturbcolour						= CVARFD ("r_fastturbcolour", "0.1 0.2 0.3", CVAR_ARCHIVE, "The colour to use for water surfaces draw with r_waterstyle 0.");
cvar_t r_waterstyle							= CVARFD ("r_waterstyle", "1", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "Changes how water, and teleporters are drawn. Possible values are:\n0: fastturb-style block colour.\n1: regular q1-style water.\n2: refraction(ripply and transparent)\n3: refraction with reflection at an angle\n4: ripplemapped without reflections (requires particle effects)\n5: ripples+reflections");
cvar_t r_slimestyle							= CVARFD ("r_slimestyle", "", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "See r_waterstyle, but affects only slime. If empty, defers to r_waterstyle.");
cvar_t r_lavastyle							= CVARFD ("r_lavastyle", "1", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "See r_waterstyle, but affects only lava. If empty, defers to r_waterstyle.");
cvar_t r_telestyle							= CVARFD ("r_telestyle", "1", CVAR_ARCHIVE|CVAR_SHADERSYSTEM, "See r_waterstyle, but affects only teleporters. If empty, defers to r_waterstyle.");

cvar_t r_vertexdlights						= CVARD	("r_vertexdlights", "0", "Determine model lighting with respect to nearby dlights. Poor-man's rtlights.");

cvar_t vid_preservegamma					= CVARD ("vid_preservegamma", "0", "Restore initial hardware gamma ramps when quitting.");
cvar_t vid_hardwaregamma					= CVARFD ("vid_hardwaregamma", "1",
												CVAR_ARCHIVE | CVAR_RENDERERLATCH, "Use hardware gamma ramps. 0=ugly texture-based gamma, 1=glsl(windowed) or hardware(fullscreen), 2=always glsl, 3=always hardware gamma (disabled if hardware doesn't support), 4=scene-only gamma.");
cvar_t vid_desktopgamma						= CVARFD ("vid_desktopgamma", "0",
												CVAR_ARCHIVE | CVAR_RENDERERLATCH, "Apply gamma ramps upon the desktop rather than the window.");

cvar_t r_fog_cullentities					= CVARD ("r_fog_cullentities", "1", "0: Never cull entities by fog...\n1: Automatically cull entities according to fog.\n2: Force fog culling regardless ");
cvar_t r_fog_linear							= CVARD ("r_fog_linear", "0", "0: Use Exp/Exp2 fog. 1: Use linear fog.");
cvar_t r_fog_exp2							= CVARD ("r_fog_exp2", "1", "Expresses how fog fades with distance. 0 (matching DarkPlaces's default) is typically more realistic, while 1 (matching FitzQuake and others) is more common.");
cvar_t r_fog_permutation					= CVARFD ("r_fog_permutation", "1", CVAR_SHADERSYSTEM, "Renders fog using a material permutation. 0 plays nicer with q3 shaders, but 1 is otherwise a better choice.");

extern cvar_t gl_dither;
cvar_t	gl_screenangle = CVAR("gl_screenangle", "0");
#endif

#ifdef D3D9QUAKE
cvar_t d3d9_hlsl							= CVAR("d3d_hlsl", "1");
#endif

#if defined(D3DQUAKE)
void GLD3DRenderer_Init(void)
{
#ifdef D3D9QUAKE
	Cvar_Register (&d3d9_hlsl, D3DRENDEREROPTIONS);
#endif
}
#endif

#if defined(GLQUAKE)
extern cvar_t gl_blacklist_texture_compression;
extern cvar_t gl_blacklist_generatemipmap;
void GLRenderer_Init(void)
{
	//gl-specific video vars
	Cvar_Register (&gl_workaround_ati_shadersource, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_version, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_debug, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_forwardcompatible, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_compatibility, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_es, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_robustness, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_selfreset, GLRENDEREROPTIONS);
	Cvar_Register (&vid_gl_context_noerror, GLRENDEREROPTIONS);

	Cvar_Register (&gl_immutable_textures, GLRENDEREROPTIONS);
	Cvar_Register (&gl_immutable_buffers, GLRENDEREROPTIONS);
	Cvar_Register (&gl_pbolightmaps, GLRENDEREROPTIONS);
//renderer

	Cvar_Register (&gl_affinemodels, GLRENDEREROPTIONS);
	Cvar_Register (&gl_nohwblend, GLRENDEREROPTIONS);
#ifdef QWSKINS
	Cvar_Register (&gl_nocolors, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&gl_finish, GLRENDEREROPTIONS);
	Cvar_Register (&gl_lateswap, GLRENDEREROPTIONS);
	Cvar_Register (&gl_lerpimages, GLRENDEREROPTIONS);

#ifdef PSKMODELS
	Cvar_Register (&dpcompat_psa_ungroup, GLRENDEREROPTIONS);
#endif
#ifdef MD1MODELS
	Cvar_Register (&mod_h2holey_bugged, GLRENDEREROPTIONS);
	Cvar_Register (&mod_halftexel, GLRENDEREROPTIONS);
	Cvar_Register (&mod_nomipmap, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&r_lerpmuzzlehack, GLRENDEREROPTIONS);
	Cvar_Register (&r_noframegrouplerp, GLRENDEREROPTIONS);
	Cvar_Register (&r_portalrecursion, GLRENDEREROPTIONS);
	Cvar_Register (&r_portaldrawplanes, GLRENDEREROPTIONS);
	Cvar_Register (&r_portalonly, GLRENDEREROPTIONS);
	Cvar_Register (&r_noaliasshadows, GLRENDEREROPTIONS);

	Cvar_Register (&r_lodscale, GRAPHICALNICETIES);
	Cvar_Register (&r_lodbias, GRAPHICALNICETIES);

	Cvar_Register (&gl_motionblur, GLRENDEREROPTIONS);
	Cvar_Register (&gl_motionblurscale, GLRENDEREROPTIONS);

	Cvar_Register (&gl_smoothcrosshair, GRAPHICALNICETIES);

//	Cvar_Register (&gl_lightmapmode, GLRENDEREROPTIONS);

	Cvar_Register (&gl_picmip, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip2d, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip_world, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip_sprites, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip_other, GLRENDEREROPTIONS);

	Cvar_Register (&r_shaderblobs, GLRENDEREROPTIONS);
	Cvar_Register (&gl_compress, GLRENDEREROPTIONS);
	Cvar_Register (&gl_blacklist_texture_compression, "gl blacklists");
	Cvar_Register (&gl_blacklist_generatemipmap,		"gl blacklists");
//	Cvar_Register (&gl_detail, GRAPHICALNICETIES);
//	Cvar_Register (&gl_detailscale, GRAPHICALNICETIES);
	Cvar_Register (&gl_overbright, GRAPHICALNICETIES);
	Cvar_Register (&gl_overbright_all, GRAPHICALNICETIES);
	Cvar_Register (&gl_dither, GRAPHICALNICETIES);
	Cvar_Register (&r_fog_cullentities, GRAPHICALNICETIES);
	Cvar_Register (&r_fog_linear, GLRENDEREROPTIONS);
	Cvar_Register (&r_fog_exp2, GLRENDEREROPTIONS);
	Cvar_Register (&r_fog_permutation, GLRENDEREROPTIONS);

	Cvar_Register (&r_tessellation, GRAPHICALNICETIES);
	Cvar_Register (&gl_ati_truform_type, GRAPHICALNICETIES);
	Cvar_Register (&r_tessellation_level, GRAPHICALNICETIES);

	Cvar_Register (&gl_screenangle, GLRENDEREROPTIONS);

	Cvar_Register (&r_wallcolour, GLRENDEREROPTIONS);
	Cvar_Register (&r_floorcolour, GLRENDEREROPTIONS);
//	Cvar_Register (&r_walltexture, GLRENDEREROPTIONS);
//	Cvar_Register (&r_floortexture, GLRENDEREROPTIONS);

	Cvar_Register (&r_vertexdlights, GLRENDEREROPTIONS);

//	Cvar_Register (&gl_schematics, GLRENDEREROPTIONS);

	Cvar_Register (&r_vertexlight, GLRENDEREROPTIONS);

	Cvar_Register (&gl_blend2d, GLRENDEREROPTIONS);

	Cvar_Register (&gl_menutint_shader, GLRENDEREROPTIONS);

	Cvar_Register (&r_lightmap_nearest, GLRENDEREROPTIONS);
	Cvar_Register (&r_lightmap_average, GLRENDEREROPTIONS);
	Cvar_Register (&mod_loadsurfenvmaps, GLRENDEREROPTIONS);
}
#endif

void	R_InitTextures (void)
{
	int		x,y, m;
	qbyte	*dest;
	static FTE_ALIGN(4) char r_notexture_mip_mem[(sizeof(texture_t) + 16*16)];

// create a simple checkerboard texture for the default
	r_notexture_mip = (texture_t*)r_notexture_mip_mem;

	r_notexture_mip->vwidth = r_notexture_mip->vheight = 16;
	r_notexture_mip->srcwidth = r_notexture_mip->srcheight = 16;
	r_notexture_mip->srcfmt = TF_SOLID8;

	for (m=0 ; m<1 ; m++)
	{
		dest = (qbyte *)(r_notexture_mip+1);
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}

static int QDECL ShowFileList (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	//ignore non-diffuse texture filenames, because they're annoying as heck.
	if (!strstr(name, "_pants.") && !strstr(name, "_shirt.") && !strstr(name, "_upper.") && !strstr(name, "_lower.") && !strstr(name, "_bump.") && !strstr(name, "_norm.") && !strstr(name, "_gloss.") && !strstr(name, "_luma."))
	{
		Con_Printf("%s\n", name);
	}
	return true;
}
void R_ListConfigs_f(void)
{
	COM_EnumerateFiles("*.cfg", ShowFileList, NULL);
	COM_EnumerateFiles("configs/*.cfg", ShowFileList, NULL);
}
void R_ListFonts_f(void)
{
	COM_EnumerateFiles("charsets/*.*", ShowFileList, NULL);
	COM_EnumerateFiles("textures/charsets/*.*", ShowFileList, NULL);
}
void R_ListSkins_f(void)
{
	COM_EnumerateFiles("skins/*.*", ShowFileList, NULL);
}


void R_SetRenderer_f (void);
void R_ReloadRenderer_f (void);

#ifdef _DEBUG
static void R_ShowBatches_f(void)
{
	sh_config.showbatches = true;
}
#endif

void R_ToggleFullscreen_f(void)
{
	double time;
	rendererstate_t newr;

	if (currentrendererstate.renderer == NULL)
	{
		Con_Printf("vid_toggle: no renderer currently set\n");
		return;
	}
	//vid toggle makes no sense with these two...
	if (currentrendererstate.renderer->rtype == QR_HEADLESS || currentrendererstate.renderer->rtype == QR_NONE)
		return;

	Cvar_ApplyLatches(CVAR_VIDEOLATCH|CVAR_RENDERERLATCH, false);

	newr = currentrendererstate;
	if (newr.fullscreen)
		newr.fullscreen = 0;	//if we're currently any sort of fullscreen then go windowed
	else if (vid_fullscreen.ival)
		newr.fullscreen = vid_fullscreen.ival;	//if we're normally meant to be fullscreen, use that
	else
		newr.fullscreen = 2;	//otherwise use native resolution
	if (newr.fullscreen)
	{
		int dbpp, dheight, dwidth, drate;
		if (!Sys_GetDesktopParameters(&dwidth, &dheight, &dbpp, &drate))
		{
			dwidth = DEFAULT_WIDTH;
			dheight = DEFAULT_HEIGHT;
			dbpp = DEFAULT_BPP;
			drate = 0;
		}

		if (newr.fullscreen == 1 && vid_width.ival>0)
			newr.width = vid_width.ival;
		else
			newr.width = dwidth;
		if (newr.fullscreen == 1 && vid_height.ival>0)
			newr.height = vid_height.ival;
		else
			newr.height = dheight;
	}
	else
	{
		newr.width = DEFAULT_WIDTH;
		newr.height = DEFAULT_HEIGHT;
	}

	time = Sys_DoubleTime();
	R_RestartRenderer(&newr);
	Con_DPrintf("main thread video restart took %f secs\n", Sys_DoubleTime() - time);
//	COM_WorkerFullSync();
//	Con_Printf("full video restart took %f secs\n", Sys_DoubleTime() - time);
}

void Renderer_Init(void)
{
	currentrendererstate.renderer = NULL;
	qrenderer = QR_NONE;

	r_blockvidrestart = true;
	Cmd_AddCommand("setrenderer", R_SetRenderer_f);
	Cmd_AddCommand("vid_restart", R_RestartRenderer_f);
	Cmd_AddCommand("vid_reload", R_ReloadRenderer_f);
	Cmd_AddCommand("vid_toggle", R_ToggleFullscreen_f);

#ifdef RTLIGHTS
	R_EditLights_RegisterCommands();
#endif

	Cmd_AddCommand("r_dumpshaders", Shader_WriteOutGenerics_f);
	Cmd_AddCommand("r_remapshader", Shader_RemapShader_f);
	Cmd_AddCommand("r_showshader", Shader_ShowShader_f);
	Cmd_AddCommandD("r_shaderlist", Shader_ShaderList_f, "Prints out a list of the currently-loaded shaders.");

#ifdef _DEBUG
	Cmd_AddCommand("r_showbatches", R_ShowBatches_f);
#endif

#ifdef SWQUAKE
	{
	extern cvar_t sw_interlace;
	extern cvar_t sw_vthread;
	extern cvar_t sw_fthreads;
	Cvar_Register(&sw_interlace, "Software Rendering Options");
	Cvar_Register(&sw_vthread, "Software Rendering Options");
	Cvar_Register(&sw_fthreads, "Software Rendering Options");
	}
#endif

	Cvar_Register (&gl_conback, GRAPHICALNICETIES);

	Cvar_Register (&r_novis, GLRENDEREROPTIONS);

	//but register ALL vid_ commands.
	Cvar_Register (&gl_driver, VIDCOMMANDGROUP);
	Cvar_Register (&vid_devicename, VIDCOMMANDGROUP);
	Cvar_Register (&vid_vsync, VIDCOMMANDGROUP);
	Cvar_Register (&vid_wndalpha, VIDCOMMANDGROUP);
#if defined(_WIN32) && defined(MULTITHREAD)
	Cvar_Register (&vid_winthread, VIDCOMMANDGROUP);
#endif
	Cvar_Register (&in_windowed_mouse, VIDCOMMANDGROUP);
	Cvar_Register (&vid_renderer, VIDCOMMANDGROUP);

	R_UpdateRendererOpts();

	Cvar_Register (&vid_renderer_opts, VIDCOMMANDGROUP);

	Cvar_Register (&vid_fullscreen, VIDCOMMANDGROUP);
	Cvar_Register (&vid_bpp, VIDCOMMANDGROUP);
	Cvar_Register (&vid_depthbits, VIDCOMMANDGROUP);

	Cvar_Register (&vid_baseheight, VIDCOMMANDGROUP);
	Cvar_Register (&vid_conwidth, VIDCOMMANDGROUP);
	Cvar_Register (&vid_conheight, VIDCOMMANDGROUP);
	Cvar_Register (&vid_conautoscale, VIDCOMMANDGROUP);
	Cvar_Register (&vid_minsize, VIDCOMMANDGROUP);

	Cvar_Register (&vid_triplebuffer, VIDCOMMANDGROUP);
	Cvar_Register (&vid_width, VIDCOMMANDGROUP);
	Cvar_Register (&vid_height, VIDCOMMANDGROUP);
	Cvar_Register (&vid_refreshrate, VIDCOMMANDGROUP);
	Cvar_Register (&vid_multisample, GLRENDEREROPTIONS);
	Cvar_Register (&vid_srgb, GLRENDEREROPTIONS);
	Cvar_Register (&vid_dpi_x, GLRENDEREROPTIONS);
	Cvar_Register (&vid_dpi_y, GLRENDEREROPTIONS);

	Cvar_Register (&vid_desktopsettings, VIDCOMMANDGROUP);
	Cvar_Register (&vid_preservegamma, GLRENDEREROPTIONS);
	Cvar_Register (&vid_hardwaregamma, GLRENDEREROPTIONS);
	Cvar_Register (&vid_desktopgamma, GLRENDEREROPTIONS);


	Cvar_Register (&r_norefresh, GLRENDEREROPTIONS);
	Cvar_Register (&r_mirroralpha, GLRENDEREROPTIONS);
	Cvar_Register (&r_softwarebanding_cvar, GRAPHICALNICETIES);

	Cvar_Register(&r_keepimages, GRAPHICALNICETIES);
	Cvar_Register(&r_ignoremapprefixes, GRAPHICALNICETIES);
	Cvar_ForceCallback(&r_keepimages);
	Cvar_ForceCallback(&r_ignoremapprefixes);
#ifdef IMAGEFMT_TGA
	Cvar_Register(&r_dodgytgafiles, "Hacky bug workarounds");
#endif
#ifdef IMAGEFMT_PCX
	Cvar_Register(&r_dodgypcxfiles, "Hacky bug workarounds");
#endif
	Cvar_Register(&r_dodgymiptex, "Hacky bug workarounds");
	r_imageextensions.enginevalue = r_defaultimageextensions;
	Cvar_Register(&r_imageextensions, GRAPHICALNICETIES);
	r_imageextensions.callback(&r_imageextensions, NULL);
	Cvar_Register(&r_image_downloadsizelimit, GRAPHICALNICETIES);
	Cvar_Register(&r_loadlits, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylesmooth, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylesmooth_limit, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylespeed, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylescale, GRAPHICALNICETIES);
	Cvar_Register(&r_lightmap_scale, GRAPHICALNICETIES);
	Cvar_Register(&r_lightmap_format, GRAPHICALNICETIES);

	Cvar_Register(&r_hdr_framebuffer, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation_multiplier, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation_minvalue, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation_maxvalue, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation_fade_down, GRAPHICALNICETIES);
	Cvar_Register(&r_hdr_irisadaptation_fade_up, GRAPHICALNICETIES);

	Cvar_Register(&r_stains, GRAPHICALNICETIES);
	Cvar_Register(&r_stainfadetime, GRAPHICALNICETIES);
	Cvar_Register(&r_stainfadeammount, GRAPHICALNICETIES);
	Cvar_Register(&r_lightprepass_cvar, GLRENDEREROPTIONS);
	Cvar_Register (&r_coronas, GRAPHICALNICETIES);
	Cvar_Register (&r_coronas_intensity, GRAPHICALNICETIES);
	Cvar_Register (&r_coronas_occlusion, GRAPHICALNICETIES);
	Cvar_Register (&r_coronas_mindist, GRAPHICALNICETIES);
	Cvar_Register (&r_coronas_fadedist, GRAPHICALNICETIES);
	Cvar_Register (&r_flashblend, GRAPHICALNICETIES);
	Cvar_Register (&r_flashblendscale, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_pbr, GRAPHICALNICETIES);
	Cvar_Register (&gl_specular, GRAPHICALNICETIES);
	Cvar_Register (&gl_specular_power, GRAPHICALNICETIES);
	Cvar_Register (&gl_specular_fallback, GRAPHICALNICETIES);
	Cvar_Register (&gl_specular_fallbackexp, GRAPHICALNICETIES);

	Sh_RegisterCvars();

	Cvar_Register (&r_fastturbcolour, GRAPHICALNICETIES);
	Cvar_Register (&r_waterstyle, GRAPHICALNICETIES);
	Cvar_Register (&r_lavastyle, GRAPHICALNICETIES);
	Cvar_Register (&r_slimestyle, GRAPHICALNICETIES);
	Cvar_Register (&r_telestyle, GRAPHICALNICETIES);
	Cvar_Register (&r_wireframe, GRAPHICALNICETIES);
	Cvar_Register (&r_wireframe_smooth, GRAPHICALNICETIES);
	Cvar_Register (&r_outline, GRAPHICALNICETIES);
	Cvar_Register (&r_outline_width, GRAPHICALNICETIES);
	Cvar_Register (&r_refract_fbo, GRAPHICALNICETIES);
	Cvar_Register (&r_refractreflect_scale, GRAPHICALNICETIES);
	Cvar_Register (&r_postprocshader, GRAPHICALNICETIES);
	Cvar_Register (&r_fxaa, GRAPHICALNICETIES);
	Cvar_Register (&r_graphics, GRAPHICALNICETIES);
	Cvar_Register (&r_renderscale, GRAPHICALNICETIES);
	Cvar_Register (&r_stereo_separation, GRAPHICALNICETIES);
	Cvar_Register (&r_stereo_convergence, GRAPHICALNICETIES);
	Cvar_Register (&r_stereo_method, GRAPHICALNICETIES);

	Cvar_Register (&r_shadow_bumpscale_basetexture, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_bumpscale_bumpmap, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_heightscale_basetexture, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_heightscale_bumpmap, GRAPHICALNICETIES);

	Cvar_Register (&r_glsl_offsetmapping, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_offsetmapping_scale, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_offsetmapping_reliefmapping, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_turbscale_reflect, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_turbscale_refract, GRAPHICALNICETIES);
	Cvar_Register (&r_glsl_emissive, GRAPHICALNICETIES);

	Cvar_Register(&scr_viewsize, SCREENOPTIONS);
	Cvar_Register(&scr_fov, SCREENOPTIONS);
	Cvar_Register(&scr_fov_viewmodel, SCREENOPTIONS);
	Cvar_Register(&scr_fov_mode, SCREENOPTIONS);

	Cvar_Register (&scr_sshot_type, SCREENOPTIONS);
	Cvar_Register (&scr_sshot_compression, SCREENOPTIONS);
	Cvar_Register (&scr_sshot_prefix, SCREENOPTIONS);

	Cvar_Register(&cl_cursor,	SCREENOPTIONS);
	Cvar_Register(&cl_cursorscale,	SCREENOPTIONS);
	Cvar_Register(&cl_cursorbiasx,	SCREENOPTIONS);
	Cvar_Register(&cl_cursorbiasy,	SCREENOPTIONS);


//screen
	Cvar_Register (&con_textfont, GRAPHICALNICETIES);
	Cvar_Register (&gl_font, GRAPHICALNICETIES);
	Cvar_Register (&scr_conspeed, SCREENOPTIONS);
	Cvar_Register (&scr_conalpha, SCREENOPTIONS);
	Cvar_Register (&scr_showturtle, SCREENOPTIONS);
	Cvar_Register (&scr_turtlefps, SCREENOPTIONS);
	Cvar_Register (&scr_showpause, SCREENOPTIONS);
	Cvar_Register (&scr_centertime, SCREENOPTIONS);
	Cvar_Register (&scr_logcenterprint, SCREENOPTIONS);
	Cvar_Register (&scr_printspeed, SCREENOPTIONS);
	Cvar_Register (&scr_allowsnap, SCREENOPTIONS);
	Cvar_Register (&scr_consize, SCREENOPTIONS);
	Cvar_Register (&scr_centersbar, SCREENOPTIONS);
	Cvar_Register (&r_xflip, GLRENDEREROPTIONS);

	Cvar_Register(&r_bloodstains, GRAPHICALNICETIES);

	Cvar_Register(&r_fullbrightSkins, GRAPHICALNICETIES);

	Cvar_Register (&mod_md3flags, GRAPHICALNICETIES);


//renderer
	Cvar_Register (&r_ignoreentpvs, "Hacky bug workarounds");
	Cvar_Register (&r_fullbright, SCREENOPTIONS);
	Cvar_Register (&r_drawentities, GRAPHICALNICETIES);
	Cvar_Register (&r_drawviewmodel, GRAPHICALNICETIES);
	Cvar_Register (&r_drawviewmodelinvis, GRAPHICALNICETIES);
	Cvar_Register (&r_waterwarp, GRAPHICALNICETIES);
	Cvar_Register (&r_speeds, SCREENOPTIONS);
	Cvar_Register (&r_netgraph, SCREENOPTIONS);

	Cvar_Register (&r_temporalscenecache, GRAPHICALNICETIES);
	Cvar_Register (&r_dynamic, GRAPHICALNICETIES);
	Cvar_Register (&r_lightmap_saturation, GRAPHICALNICETIES);

	Cvar_Register (&r_nolerp, GRAPHICALNICETIES);
	Cvar_Register (&r_nolightdir, GRAPHICALNICETIES);

	Cvar_Register (&r_fastturb, GRAPHICALNICETIES);
	Cvar_Register (&r_wateralpha, GRAPHICALNICETIES);
	Cvar_Register (&r_lavaalpha, GRAPHICALNICETIES);
	Cvar_Register (&r_slimealpha, GRAPHICALNICETIES);
	Cvar_Register (&r_telealpha, GRAPHICALNICETIES);
	Cvar_Register (&gl_shadeq1_name, GLRENDEREROPTIONS);

	Cvar_Register (&gl_mindist, GLRENDEREROPTIONS);
	Cvar_Register (&gl_load24bit, GRAPHICALNICETIES);
	Cvar_Register (&gl_blendsprites, GLRENDEREROPTIONS);

	Cvar_Register (&r_clear, GLRENDEREROPTIONS);
	Cvar_Register (&r_clearcolour, GLRENDEREROPTIONS);
	Cvar_Register (&gl_max_size, GLRENDEREROPTIONS);
	Cvar_Register (&gl_maxdist, GLRENDEREROPTIONS);
	Cvar_Register (&gl_texturemode, GLRENDEREROPTIONS);
	Cvar_Register (&gl_texturemode2d, GLRENDEREROPTIONS);
	Cvar_Register (&r_font_linear, GLRENDEREROPTIONS);
	Cvar_Register (&r_font_postprocess_outline, GLRENDEREROPTIONS);
	Cvar_Register (&r_font_postprocess_mono, GLRENDEREROPTIONS);
#if defined(HAVE_LEGACY) && defined(AVAIL_FREETYPE)
	Cvar_Register (&dpcompat_smallerfonts, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&gl_mipcap, GLRENDEREROPTIONS);
	Cvar_Register (&gl_texture_lodbias, GLRENDEREROPTIONS);
	Cvar_Register (&gl_texture_anisotropic_filtering, GLRENDEREROPTIONS);
	Cvar_Register (&r_deluxemapping_cvar, GRAPHICALNICETIES);
	Cvar_Register (&r_max_gpu_bones, GRAPHICALNICETIES);
	Cvar_Register (&r_drawflat, GRAPHICALNICETIES);
	Cvar_Register (&r_lightmap, GRAPHICALNICETIES);
	Cvar_Register (&r_menutint, GRAPHICALNICETIES);

	Cvar_Register (&r_fb_bmodels, GRAPHICALNICETIES);
	Cvar_Register (&r_fb_models, GRAPHICALNICETIES);
	Cvar_Register (&gl_overbright_models, GRAPHICALNICETIES);
//	Cvar_Register (&r_fullbrights, GRAPHICALNICETIES);	//dpcompat: 1 if r_fb_bmodels&&r_fb_models
//	Cvar_Register (&r_skin_overlays, GRAPHICALNICETIES);
	Cvar_Register (&r_globalskin_first, GRAPHICALNICETIES);
	Cvar_Register (&r_globalskin_count, GRAPHICALNICETIES);
	Cvar_Register (&r_shadows, GRAPHICALNICETIES);

	Cvar_Register (&r_replacemodels, GRAPHICALNICETIES);

	Cvar_Register (&r_showbboxes, GLRENDEREROPTIONS);
	Cvar_Register (&r_showfields, GLRENDEREROPTIONS);
	Cvar_Register (&r_showshaders, GLRENDEREROPTIONS);
#ifdef BEF_PUSHDEPTH
	Cvar_Register (&r_polygonoffset_submodel_factor, GLRENDEREROPTIONS);
	Cvar_Register (&r_polygonoffset_submodel_offset, GLRENDEREROPTIONS);
	Cvar_Register (&r_polygonoffset_submodel_maps, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&r_polygonoffset_shadowmap_factor, GLRENDEREROPTIONS);
	Cvar_Register (&r_polygonoffset_shadowmap_offset, GLRENDEREROPTIONS);
	Cvar_Register (&r_polygonoffset_stencil_factor, GLRENDEREROPTIONS);
	Cvar_Register (&r_polygonoffset_stencil_offset, GLRENDEREROPTIONS);

	Cvar_Register (&r_forceprogramify, GLRENDEREROPTIONS);
	Cvar_Register (&r_glsl_precache, GLRENDEREROPTIONS);
	Cvar_Register (&r_halfrate, GRAPHICALNICETIES);
#ifdef HAVE_LEGACY
	Cvar_Register (&dpcompat_nopremulpics, GLRENDEREROPTIONS);
#endif
#ifdef VKQUAKE
	VK_RegisterVulkanCvars();
#endif

// misc
	Cvar_Register(&con_ocranaleds, "Console controls");

	Cmd_AddCommandD ("listfonts", R_ListFonts_f, "Displays a list of every installed font.");
	Cmd_AddCommandD ("listskins", R_ListSkins_f, "Displays a list of every installed or downloaded QuakeWorld player skin.");
	Cmd_AddCommandD ("listconfigs", R_ListConfigs_f, "Displays a list of every installed config file.");

	R_Sky_Register();

#if defined(D3DQUAKE)
	GLD3DRenderer_Init();
#endif
#if defined(GLQUAKE)
	GLRenderer_Init();
#endif

#if defined(GLQUAKE) || defined(VKQUAKE)
	R_BloomRegister();
#endif

	P_InitParticleSystem();
	R_InitTextures();


	R_RegisterBuiltinRenderers();
}

qboolean Renderer_Started(void)
{
	return !r_blockvidrestart && !!currentrendererstate.renderer;
}

void Renderer_Start(void)
{
	r_blockvidrestart = false;
	Cvar_ApplyLatches(CVAR_VIDEOLATCH|CVAR_RENDERERLATCH, false);

	//renderer = none && currentrendererstate.bpp == -1 means we've never applied any mode at all
	//if we currently have none, we do actually need to apply it still
	if (qrenderer == QR_NONE && *vid_renderer.string)
	{
		Cmd_ExecuteString("vid_restart\n", RESTRICT_LOCAL);
	}
	if (!currentrendererstate.renderer)
	{	//we still failed. Try again, but use the default renderer.
		Cvar_Set(&vid_renderer, "");
		Cmd_ExecuteString("vid_restart\n", RESTRICT_LOCAL);
	}
	if (!currentrendererstate.renderer)
		Sys_Error("No renderer was set!\n");

	if (qrenderer == QR_NONE)
		Con_Printf("Use the setrenderer command to use a gui\n");
}


void	(*Draw_Init)				(void);
void	(*Draw_Shutdown)			(void);

void	(*R_Init)					(void);
void	(*R_DeInit)					(void);
void	(*R_RenderView)				(void);		// must set r_refdef first

qboolean (*VID_Init)				(rendererstate_t *info, unsigned char *palette);
void	 (*VID_DeInit)				(void);
char	*(*VID_GetRGBInfo)			(int *stride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt);
void	(*VID_SetWindowCaption)		(const char *msg);

qboolean (*SCR_UpdateScreen)			(void);

r_qrenderer_t qrenderer;
char *q_renderername = "Non-Selected renderer";

static struct
{
	void *module;
	rendererinfo_t *ri;
} rendererinfo[16];

qboolean R_RegisterRenderer(void *module, rendererinfo_t *ri)
{
	size_t i;
	for (i = 0; i < countof(rendererinfo); i++)
	{	//already registered
		if (rendererinfo[i].ri == ri)
			return true;
	}
	for (i = 0; i < countof(rendererinfo); i++)
	{	//register it in the first empty slot
		if (!rendererinfo[i].ri)
		{
			rendererinfo[i].module = module;
			rendererinfo[i].ri = ri;
			R_UpdateRendererOpts();
			return true;
		}
	}
	Sys_Printf("unable to register renderer %s\n", ri->description);
	return false;
}

static plugvrfuncs_t *vrfuncs;
static void *vrmodule;
qboolean R_RegisterVRDriver(void *module, plugvrfuncs_t *vr)
{
	if (!vr)
	{
		if (vrmodule == module)
		{
			vrfuncs = NULL;
			vrmodule = NULL;
		}
		return false;
	}

	if (vrfuncs && vrfuncs!=vr)
	{
		Con_Printf("unable to register renderer %s (%s already registered)\n", vr->description, vrfuncs->description);
		return false;
	}
	vrfuncs = vr;
	vrmodule = module;
	return true;
}



static rendererinfo_t dedicatedrendererinfo = {
	//ALL builds need a 'none' renderer, as 0.
	"No renderer",
	{
		"none",
		"dedicated",
		"terminal",
		"sv"
	},
	QR_NONE,

	NULL,	//Draw_Init;
	NULL,	//Draw_Shutdown;

	NULL,	//IMG_UpdateFiltering
	NULL,	//IMG_LoadTextureMips
	NULL,	//IMG_DestroyTexture

	NULL,	//R_Init;
	NULL,	//R_DeInit;
	NULL,	//R_RenderView;

	NULL, //VID_Init,
	NULL, //VID_DeInit,
	NULL, //VID_SwapBuffers
	NULL, //VID_ApplyGammaRamps,
	NULL,
	NULL,
	NULL,
	NULL,	//set caption
	NULL, //VID_GetRGBInfo,

	NULL,	//SCR_UpdateScreen;

	/*backend*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,

	""
};
static void R_RegisterBuiltinRenderers(void)
{
	#ifdef GLQUAKE
	{
		extern rendererinfo_t openglrendererinfo;
		#ifdef FTE_RPI
		{
			extern rendererinfo_t rpirendererinfo;
			R_RegisterRenderer(NULL, &rpirendererinfo);
		}
		#endif
		R_RegisterRenderer(NULL, &openglrendererinfo);
		#ifdef USE_EGL
		{
			extern rendererinfo_t eglrendererinfo;
			R_RegisterRenderer(NULL, &eglrendererinfo);
		}
		#endif
	}
	#endif


	#ifdef D3D9QUAKE
	{
		extern rendererinfo_t d3d9rendererinfo;
		R_RegisterRenderer(NULL, &d3d9rendererinfo);
	}
	#endif
	#ifdef VKQUAKE
	{
		extern rendererinfo_t vkrendererinfo;
		R_RegisterRenderer(NULL, &vkrendererinfo);
		#if defined(_WIN32) && defined(GLQUAKE) && !defined(FTE_SDL)
		{
			extern rendererinfo_t nvvkrendererinfo;
			R_RegisterRenderer(NULL, &nvvkrendererinfo);
		}
		#endif
	}
	#endif
	#ifdef D3D11QUAKE
	{
		extern rendererinfo_t d3d11rendererinfo;
		R_RegisterRenderer(NULL, &d3d11rendererinfo);
	}
	#endif
	#ifdef SWQUAKE
	{
		extern rendererinfo_t swrendererinfo;
		R_RegisterRenderer(NULL, &swrendererinfo);
	}
	#endif
	#ifdef D3D8QUAKE
	{
		extern rendererinfo_t d3d8rendererinfo;
		R_RegisterRenderer(NULL, &d3d8rendererinfo);
	}
	#endif
	#ifdef WAYLANDQUAKE
		#ifdef GLQUAKE
		{
			extern rendererinfo_t rendererinfo_wayland_gl;
			R_RegisterRenderer(NULL, &rendererinfo_wayland_gl);
		}
		#endif
		#ifdef VKQUAKE
		{
			extern rendererinfo_t rendererinfo_wayland_vk;
			R_RegisterRenderer(NULL, &rendererinfo_wayland_vk);
		}
		#endif
	#endif
	#if defined(GLQUAKE) && defined(USE_FBDEV)
	{
		extern rendererinfo_t fbdevrendererinfo;
		R_RegisterRenderer(NULL, &fbdevrendererinfo);	//direct stuff that doesn't interact well with the system should always be low priority
	}
	#endif
		R_RegisterRenderer(NULL, &dedicatedrendererinfo);
	#ifdef HEADLESSQUAKE
	{
		extern rendererinfo_t headlessrenderer;
		R_RegisterRenderer(NULL, &headlessrenderer);
		#ifdef VKQUAKE
			//R_RegisterRenderer(NULL, &headlessvkrendererinfo);
		#endif
	}
	#endif
	#if defined(GLQUAKE) && defined(USE_EGL)
	{
		extern rendererinfo_t rendererinfo_headless_egl;
		R_RegisterRenderer(NULL, &rendererinfo_headless_egl);
	}
	#endif
}


void R_SetRenderer(rendererinfo_t *ri)
{
	currentrendererstate.renderer = ri;
	if (!ri)
		ri = &dedicatedrendererinfo;

	qrenderer = ri->rtype;
	q_renderername = ri->name[0];

	Draw_Init				= ri->Draw_Init;
	Draw_Shutdown			= ri->Draw_Shutdown;

	R_Init					= ri->R_Init;
	R_DeInit				= ri->R_DeInit;
	R_RenderView			= ri->R_RenderView;

	VID_Init				= ri->VID_Init;
	VID_DeInit				= ri->VID_DeInit;
	VID_GetRGBInfo			= ri->VID_GetRGBInfo;
	VID_SetWindowCaption	= ri->VID_SetWindowCaption;

	SCR_UpdateScreen		= ri->SCR_UpdateScreen;
}

qbyte default_quakepal[768] =
{
0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,
139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,
55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,
171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,
51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,
43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83
};

qboolean R_ApplyRenderer_Load (rendererstate_t *newr);
void D3DSucks(void)
{
	SCR_DeInit();

	if (!R_ApplyRenderer_Load(NULL))//&currentrendererstate))
		Sys_Error("Failed to reload content after mode switch\n");
}

void R_ShutdownRenderer(qboolean devicetoo)
{
#ifdef MENU_NATIVECODE
	if (mn_entry)
		mn_entry->Shutdown(MI_RENDERER);
#endif

	//make sure the worker isn't still loading stuff
	COM_WorkerFullSync();

	CL_AllowIndependantSendCmd(false);	//FIXME: figure out exactly which parts are going to affect the model loading.

#ifdef QWSKINS
	Skin_FlushAll();
#endif

	P_Shutdown();
	Mod_Shutdown(false);

	IN_Shutdown();

	Media_VideoRestarting();

	//these functions need to be able to cope with vid_reload, so don't clear them.
	//they also need to be able to cope with being re-execed in the case of failed startup.
	if (R_DeInit)
	{
		TRACE(("dbg: R_ApplyRenderer: R_DeInit\n"));
		R_DeInit();
	}

	if (Draw_Shutdown)
		Draw_Shutdown();

	TRACE(("dbg: R_ApplyRenderer: SCR_DeInit\n"));
	SCR_DeInit();

	if (VID_DeInit && devicetoo)
	{
		if (vid.vr)
			vid.vr->Shutdown();
		vid.vr = NULL;

		TRACE(("dbg: R_ApplyRenderer: VID_DeInit\n"));
		VID_DeInit();
	}

	COM_FlushTempoaryPacks();

	W_Shutdown();
	if (h2playertranslations)
		BZ_Free(h2playertranslations);
	h2playertranslations = NULL;
	if (host_basepal)
		BZ_Free(host_basepal);
	host_basepal = NULL;
	Surf_ClearSceneCache();

	RQ_Shutdown();

	if (devicetoo)
		S_Shutdown(false);
	else
		S_StopAllSounds (true);
}

void R_GenPaletteLookup(void)
{
	extern qbyte default_quakepal[];
	int r,g,b,i;
	unsigned char *pal = host_basepal;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;

		d_8to24rgbtable[i] = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24srgbtable[i] = (255<<24) + (SRGBb(r)<<0) + (SRGBb(g)<<8) + (SRGBb(b)<<16);
		d_8to24bgrtable[i] = (255<<24) + (b<<0) + (g<<8) + (r<<16);
	}
	d_8to24rgbtable[255] &= 0xffffff;	// 255 is transparent
	d_8to24srgbtable[255] &= 0xffffff;	// 255 is transparent
	d_8to24bgrtable[255] &= 0xffffff;	// 255 is transparent

	for (i=0 ; i<256 ; i++)
		d_quaketo24srgbtable[i] = (255<<24) | (SRGBb(default_quakepal[(i)*3+0])<<0) | (SRGBb(default_quakepal[(i)*3+1])<<8) | (SRGBb(default_quakepal[(i)*3+2])<<16);
}
qboolean R_ApplyRenderer (rendererstate_t *newr)
{
	double time;
	if (newr->bpp == -1)
		return false;
	if (!newr->renderer)
		return false;

	COM_WorkerFullSync();

	time = Sys_DoubleTime();

#ifndef NOBUILTINMENUS
//	M_RemoveAllMenus(true);
#endif
	Media_CaptureDemoEnd();
	R_ShutdownRenderer(true);
	Con_DPrintf("video shutdown took %f seconds\n", Sys_DoubleTime() - time);

#ifdef __GLIBC__
	malloc_trim(0);
#endif

	if (qrenderer == QR_NONE)
	{
		if (newr->renderer->rtype == qrenderer && currentrendererstate.renderer)
		{
			R_SetRenderer(newr->renderer);
			return true;	//no point
		}

		Sys_CloseTerminal ();
	}

	time = Sys_DoubleTime();
	R_SetRenderer(newr->renderer);
	Con_DPrintf("video startup took %f seconds\n", Sys_DoubleTime() - time);

	return R_ApplyRenderer_Load(newr);
}
qboolean R_ApplyRenderer_Load (rendererstate_t *newr)
{
	int i, j;
	double start = Sys_DoubleTime();

	COM_WorkerFullSync();

	Cache_Flush();
	COM_FlushFSCache(false, true);	//make sure the fs cache is built if needed. there's lots of loading here.

	TRACE(("dbg: R_ApplyRenderer: old renderer closed\n"));

	pmove.numphysent = 0;
	pmove.physents[0].model = NULL;

	vid.dpi_x = 96;
	vid.dpi_y = 96;

	Cvar_ApplyLatches(CVAR_RENDEREROVERRIDE, true);

#ifndef CLIENTONLY
	sv.world.lastcheckpvs = NULL;
#endif

	if (qrenderer != QR_NONE)	//graphics stuff only when not dedicated
	{
		size_t sz;
		qbyte *data;
#ifndef CLIENTONLY
		isDedicated = false;
#endif
		if (newr)
			if (!r_forceheadless || newr->renderer->rtype != QR_HEADLESS)
			{
				if (newr->fullscreen == 2)
					Con_TPrintf("Setting fullscreen windowed %s%s\n", newr->srgb?"SRGB ":"", newr->renderer->description);
				else if (newr->fullscreen)
				{
					if (newr->rate)
					{
						if (newr->width || newr->height)
							Con_TPrintf("Setting mode %i*%i %ibpp %ihz %s%s\n", newr->width, newr->height, newr->bpp, newr->rate, newr->srgb?"SRGB ":"", newr->renderer->description);
						else
							Con_TPrintf("Setting mode auto %ibpp %ihz %s%s\n", newr->bpp, newr->rate, newr->srgb?"SRGB ":"", newr->renderer->description);
					}
					else
					{
						if (newr->width || newr->height)
							Con_TPrintf("Setting mode %i*%i %ibpp %s%s\n", newr->width, newr->height, newr->bpp, newr->srgb?"SRGB ":"", newr->renderer->description);
						else
							Con_TPrintf("Setting mode auto %ibpp %s%s\n", newr->bpp, newr->srgb?"SRGB ":"", newr->renderer->description);
					}
				}
				else
					Con_TPrintf("Setting windowed mode %i*%i %s%s\n", newr->width, newr->height, newr->srgb?"SRGB ":"", newr->renderer->description);
			}

		vid.fullbright=0;

		if (host_basepal)
			BZ_Free(host_basepal);
		host_basepal = (qbyte *)FS_LoadMallocFile ("gfx/palette.lmp", &sz);
		vid.fullbright = host_basepal?32:0;	//q1-like mods are assumed to have 32 fullbright pixels, even if the colormap is missing.
		if (!host_basepal)
		{
			host_basepal = (qbyte *)FS_LoadMallocFile ("wad/playpal", &sz);
			if (host_basepal && sz > 768)
				sz = 768;
		}
		if (!host_basepal || sz != 768)
		{
#if defined(Q2CLIENT) && defined(IMAGEFMT_PCX)
			qbyte *pcx = COM_LoadTempFile("pics/colormap.pcx", 0, &sz);
#endif
			if (host_basepal)
				Z_Free(host_basepal);
			host_basepal = BZ_Malloc(768);
#if defined(Q2CLIENT) && defined(IMAGEFMT_PCX)
			if (pcx && ReadPCXPalette(pcx, sz, host_basepal))
				goto q2colormap;	//skip the colormap.lmp file as we already read it
			else
#endif
			{
				memcpy(host_basepal, default_quakepal, 768);
			}
		}
		if (!Ruleset_FileLoaded("gfx/palette.lmp", host_basepal, 768))
			memcpy(host_basepal, default_quakepal, 768);

		{
			size_t csize;
			qbyte *colormap = (qbyte *)FS_LoadMallocFile ("gfx/colormap.lmp", &csize);

			if (colormap && csize == VID_GRADES*256+1 && Ruleset_FileLoaded("gfx/colormap.lmp", colormap, csize))
			{
				j = VID_GRADES-1;
				data = colormap + j*256;
				vid.fullbright = 0;
				for (i = 255; i >= 0; i--)
				{
					if (colormap[i] == data[i])
						vid.fullbright++;
					else
						break;
				}
			}
			BZ_Free(colormap);
		}

#ifdef HEXEN2
		if (h2playertranslations)
			BZ_Free(h2playertranslations);
		h2playertranslations = FS_LoadMallocFile ("gfx/player.lmp", NULL);
#endif

		if (vid.fullbright < 2)
			vid.fullbright = 0;	//transparent colour doesn't count.

#if defined(Q2CLIENT) && defined(IMAGEFMT_PCX)
q2colormap:
#endif

TRACE(("dbg: R_ApplyRenderer: Palette loaded\n"));

		if (newr)
		{
			vid.flags = 0;
			vid.gammarampsize = 256;	//make a guess.
			if (!VID_Init(newr, host_basepal))
			{
				return false;
			}
		}
TRACE(("dbg: R_ApplyRenderer: vid applied\n"));

		//update palettes now that we know whether srgb is to be used etc
		R_GenPaletteLookup();

		r_softwarebanding = false;
		r_deluxemapping = false;
		r_lightprepass = false;

		W_LoadWadFile("gfx.wad");
TRACE(("dbg: R_ApplyRenderer: wad loaded\n"));
		Image_Init();
		Draw_Init();
TRACE(("dbg: R_ApplyRenderer: draw inited\n"));
#ifdef MENU_NATIVECODE
		if (mn_entry)
			mn_entry->Init(MI_RENDERER, vid.width, vid.height, vid.rotpixelwidth, vid.rotpixelheight);
#endif
		R_Init();
		RQ_Init();
		R_InitParticleTexture ();
TRACE(("dbg: R_ApplyRenderer: renderer inited\n"));
		SCR_Init();
TRACE(("dbg: R_ApplyRenderer: screen inited\n"));
		Sbar_Flush();

		IN_ReInit();
		Media_VideoRestarted();

		Cvar_ForceCallback(&v_gamma);
	}
	else
	{
#ifdef CLIENTONLY
		Sys_Error("Tried setting dedicated mode\n");
		//we could support this, but there's no real reason to actually do so.

		//fixme: despite the checks in the setrenderer command, we can still get here via a config using vid_renderer.
#else
TRACE(("dbg: R_ApplyRenderer: isDedicated = true\n"));
		isDedicated = true;
		if (cls.state)
		{
			int os = sv.state;
			sv.state = ss_dead;	//prevents server from being killed off too.
			CL_Disconnect("Graphics rendering disabled");
			sv.state = os;
		}
		if (!Sys_InitTerminal())
			return false;
		Con_PrintToSys();
#endif
	}
TRACE(("dbg: R_ApplyRenderer: initing mods\n"));
	Mod_Init(false);

//	host_hunklevel = Hunk_LowMark();

	Cvar_ForceSetValue(&vid_dpi_x, vid.dpi_x);
	Cvar_ForceSetValue(&vid_dpi_y, vid.dpi_y);
#if 0//def HAVE_LEGACY
	{	//if dp's vid_pixelheight cvar exists then lets force it to what we know.
		//that said, some dp mods just end up trying to use a fov of 0('auto') when this is 0, which fixes all our fov woes, so don't break that if its explictly 0 (a bit of a hack, but quite handy).
		//setting this properly seems to fuck over xonotic.
		cvar_t *pixelheight = Cvar_FindVar("vid_pixelheight");
		if (pixelheight && (pixelheight->value || !*pixelheight->string))
		{
			if (vid.dpi_x && vid.dpi_y)
			{
				float ipd_x = 1/vid.dpi_x;
				float ipd_y = 1/vid.dpi_y;
				Cvar_ForceSetValue(pixelheight, ipd_y/ipd_x);
			}
			else
				Cvar_ForceSetValue(pixelheight, 1);
		}
	}
#endif

	TRACE(("dbg: R_ApplyRenderer: R_PreNewMap (how handy)\n"));
	Surf_PreNewMap();

#ifndef CLIENTONLY
	if (sv.world.worldmodel)
	{
TRACE(("dbg: R_ApplyRenderer: reloading server map\n"));
		sv.world.worldmodel = Mod_ForName (sv.modelname, MLV_WARNSYNC);
TRACE(("dbg: R_ApplyRenderer: loaded\n"));
		if (sv.world.worldmodel->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(sv.world.worldmodel, &sv.world.worldmodel->loadstate, MLS_LOADING);
TRACE(("dbg: R_ApplyRenderer: doing that funky phs thang\n"));
		SV_CalcPHS ();

TRACE(("dbg: R_ApplyRenderer: clearing world\n"));

		if (sv.world.worldmodel->loadstate != MLS_LOADED)
			SV_UnspawnServer();
		else if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
		{
			for (i = 0; i < MAX_PRECACHE_MODELS; i++)
			{
				if (sv.strings.model_precache[i] && *sv.strings.model_precache[i] && (!strcmp(sv.strings.model_precache[i] + strlen(sv.strings.model_precache[i]) - 4, ".bsp") || i-1 < sv.world.worldmodel->numsubmodels))
					sv.models[i] = Mod_FindName(Mod_FixName(sv.strings.model_precache[i], sv.strings.model_precache[1]));
				else
					sv.models[i] = NULL;
			}

			World_ClearWorld (&sv.world, true);
//			ent = sv.world.edicts;
//			ent->v->model = PR_NewString(svprogfuncs, sv.worldmodel->name);	//FIXME: is this a problem for normal ents?
		}
#ifdef Q2SERVER
		else if (svs.gametype == GT_QUAKE2)
		{
			q2edict_t *q2ent;
			for (i = 0; i < Q2MAX_MODELS; i++)
			{
				if (sv.strings.configstring[Q2CS_MODELS+i] && *sv.strings.configstring[Q2CS_MODELS+i] && (!strcmp(sv.strings.configstring[Q2CS_MODELS+i] + strlen(sv.strings.configstring[Q2CS_MODELS+i]) - 4, ".bsp") || i-1 < sv.world.worldmodel->numsubmodels))
					sv.models[i] = Mod_FindName(Mod_FixName(sv.strings.configstring[Q2CS_MODELS+i], sv.modelname));
				else
					sv.models[i] = NULL;
			}
			for (; i < MAX_PRECACHE_MODELS; i++)
			{
				if (sv.strings.q2_extramodels[i] && *sv.strings.q2_extramodels[i] && (!strcmp(sv.strings.q2_extramodels[i] + strlen(sv.strings.q2_extramodels[i]) - 4, ".bsp") || i-1 < sv.world.worldmodel->numsubmodels))
					sv.models[i] = Mod_FindName(Mod_FixName(sv.strings.q2_extramodels[i], sv.modelname));
				else
					sv.models[i] = NULL;
			}

			World_ClearWorld (&sv.world, false);
			q2ent = ge->edicts;
			for (i=0 ; i<ge->num_edicts ; i++, q2ent = (q2edict_t *)((char *)q2ent + ge->edict_size))
			{
				if (!q2ent)
					continue;
				if (!q2ent->inuse)
					continue;

				if (q2ent->area.prev)
				{
					q2ent->area.prev = q2ent->area.next = NULL;
					WorldQ2_LinkEdict (&sv.world, q2ent);	// relink ents so touch functions continue to work.
				}
			}
		}
#endif
#ifdef Q3SERVER
		else if (svs.gametype == GT_QUAKE3)
		{
			memset(&sv.models, 0, sizeof(sv.models));
			sv.models[1] = Mod_FindName(sv.modelname);
			//traditionally a q3 server can just keep hold of its world cmodel and nothing is harmed.
			//this means we just need to reload the worldmodel and all is fine...
			//there are some edge cases however, like lingering pointers refering to entities.
		}
#endif
		else
			SV_UnspawnServer();
	}
#endif
#ifdef PLUGINS
	Plug_ResChanged(true);
#endif
	Cvar_ForceCallback(&r_particlesystem);
#ifdef MENU_NATIVECODE
	if (mn_entry)
		mn_entry->Init(MI_RENDERER, vid.width, vid.height, vid.rotpixelwidth, vid.rotpixelheight);
#endif

	CL_InitDlights();

TRACE(("dbg: R_ApplyRenderer: starting on client state\n"));

	if (newr)
		memcpy(&currentrendererstate, newr, sizeof(currentrendererstate));

	TRACE(("dbg: R_ApplyRenderer: S_Restart_f\n"));
	if (!isDedicated && newr)
		S_DoRestart(true);

#ifdef VM_UI
	if (q3)
		q3->ui.Reset();
#endif

#ifdef Q3SERVER
	if (svs.gametype == GT_QUAKE3)
	{
		cl.worldmodel = NULL;
		if (q3)
			q3->cg.VideoRestarted();
	}
	else
#endif
	if (cl.worldmodel)
	{
		int wmidx = 0;
		model_t *oldwm = cl.worldmodel;
		cl.worldmodel = NULL;
		CL_ClearEntityLists();	//shouldn't really be needed, but we're paranoid

		//FIXME: this code should not be here. call CL_LoadModels instead? that does csqc loading etc though. :s
TRACE(("dbg: R_ApplyRenderer: reloading ALL models\n"));
		for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
		{
			if (!cl.model_name[i])
				break;

			TRACE(("dbg: R_ApplyRenderer: reloading model %s\n", cl.model_name[i]));

			if (oldwm == cl.model_precache[i])
				wmidx = i;

#ifdef Q2CLIENT	//skip vweps
			if (cls.protocol == CP_QUAKE2 && *cl.model_name[i] == '#')
				cl.model_precache[i] = NULL;
			else
#endif
				if (i == 1)
					cl.model_precache[i] = Mod_ForName (cl.model_name[i], MLV_SILENT);
				else
					cl.model_precache[i] = Mod_FindName (Mod_FixName(cl.model_name[i], cl.model_name[1]));
		}

#ifdef HAVE_LEGACY
		for (i=0; i < MAX_VWEP_MODELS; i++)
		{
			if (cl.model_name_vwep[i])
				cl.model_precache_vwep[i] = Mod_ForName (cl.model_name_vwep[i], MLV_SILENT);
			else
				cl.model_precache_vwep[i] = NULL;
		}
#endif

#ifdef CSQC_DAT
		for (i=1 ; i<MAX_CSMODELS ; i++)
		{
			if (!cl.model_csqcname[i][0])
				break;

			if (oldwm == cl.model_csqcprecache[i])
				wmidx = -i;

			cl.model_csqcprecache[i] = NULL;
			TRACE(("dbg: R_ApplyRenderer: reloading csqc model %s\n", cl.model_csqcname[i]));
			cl.model_csqcprecache[i] = Mod_ForName (Mod_FixName(cl.model_csqcname[i], cl.model_name[1]), MLV_SILENT);
		}

		if (wmidx < 0)
			cl.worldmodel = cl.model_csqcprecache[-wmidx];
		else
#endif
			cl.worldmodel = cl.model_precache[wmidx];

		if (cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(cl.worldmodel, &cl.worldmodel->loadstate, MLS_LOADING);

TRACE(("dbg: R_ApplyRenderer: done the models\n"));
		if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		{
//			Con_Printf ("\nThe required model file '%s' could not be found.\n\n", cl.model_name[i]);
//			Con_Printf ("You may need to download or purchase a client pack in order to play on this server.\n\n");

			CL_Disconnect ("Worldmodel missing after video reload");

			if (newr)
				memcpy(&currentrendererstate, newr, sizeof(currentrendererstate));
			return true;
		}

TRACE(("dbg: R_ApplyRenderer: checking any wad textures\n"));
		Mod_NowLoadExternal(cl.worldmodel);

		for (i = 0; i < cl.num_statics; i++)	//make the static entities reappear.
			cl_static_entities[i].ent.model = NULL;

TRACE(("dbg: R_ApplyRenderer: Surf_NewMap\n"));
		Surf_NewMap(cl.worldmodel);
TRACE(("dbg: R_ApplyRenderer: efrags\n"));

//		Skin_FlushAll();
		Skin_FlushPlayers();

	}

#ifdef SKELETALOBJECTS
	skel_reload();
#endif
#ifdef CSQC_DAT
	Shader_DoReload();
	CSQC_RendererRestarted(false);
#endif
#ifdef MENU_DAT
	MP_RendererRestarted();
#endif

	if (newr && qrenderer != QR_NONE)
	{
		if (!r_forceheadless || newr->renderer->rtype != QR_HEADLESS)
			Con_TPrintf("%s renderer initialized\n", newr->renderer->description);
	}

	TRACE(("dbg: R_ApplyRenderer: done\n"));


	Con_DPrintf("video restart took %f seconds\n", Sys_DoubleTime() - start);
	return true;
}

void R_ReloadRenderer_f (void)
{
#if !defined(CLIENTONLY)
	void *portalblob = NULL;
	size_t portalsize = 0;
#endif

	float time = Sys_DoubleTime();
	if (qrenderer == QR_NONE || qrenderer == QR_HEADLESS)
		return;	//don't bother reloading the renderer if its not actually rendering anything anyway.

#if !defined(CLIENTONLY)
	if (sv.state == ss_active && sv.world.worldmodel && sv.world.worldmodel->loadstate == MLS_LOADED && sv.world.worldmodel->funcs.SaveAreaPortalBlob)
	{
		void *t;
		portalsize = sv.world.worldmodel->funcs.SaveAreaPortalBlob(sv.world.worldmodel, &t);
		if (portalsize && (portalblob = BZ_Malloc(portalsize)))
			memcpy(portalblob, t, portalsize);
	}
#endif

	Cvar_ApplyLatches(CVAR_VIDEOLATCH|CVAR_RENDERERLATCH, false);
	R_ShutdownRenderer(false);
	Con_DPrintf("teardown = %f\n", Sys_DoubleTime() - time);
	//reloads textures without destroying video context.
	R_ApplyRenderer_Load(NULL);
	Cvar_ApplyCallbacks(CVAR_RENDERERCALLBACK);

#if !defined(CLIENTONLY)
	if (portalblob)
	{
		if (sv.world.worldmodel && sv.world.worldmodel->loadstate == MLS_LOADED && sv.world.worldmodel->funcs.LoadAreaPortalBlob)
			sv.world.worldmodel->funcs.LoadAreaPortalBlob(sv.world.worldmodel, portalblob, portalsize);
		BZ_Free(portalblob);
	}
#endif
	Con_DPrintf("vid_reload time: %f\n", Sys_DoubleTime() - time);
}

static int R_PriorityForRenderer(rendererinfo_t *r)
{
	if (r && r->name[0])
	{
		if (r->VID_GetPriority)
			return r->VID_GetPriority();
		else if (r->rtype == QR_HEADLESS)
			return -1;	//headless renderers are a really poor choice, and will make the user think it buggy.
		else if (r->rtype == QR_NONE)
			return 0;	//dedicated servers are possible, but we really don't want to use them unless we have no other choice.
		else
			return 1;	//assume 1 for most renderers.
	}
	return -2;	//invalid renderer
}

//use Cvar_ApplyLatches(CVAR_RENDERERLATCH) beforehand.
qboolean R_BuildRenderstate(rendererstate_t *newr, char *rendererstring)
{
	int i, j;

	memset(newr, 0, sizeof(*newr));

	newr->width = vid_width.value;
	newr->height = vid_height.value;

	newr->triplebuffer = vid_triplebuffer.value;
	newr->multisample = vid_multisample.value;
	newr->bpp = vid_bpp.value;
	newr->depthbits = vid_depthbits.value;
	newr->fullscreen = vid_fullscreen.value;
	newr->rate = vid_refreshrate.value;
	newr->stereo = (r_stereo_method.ival == 1);
	newr->srgb = vid_srgb.ival;
	newr->vr = vrfuncs;

#if defined(_WIN32) && !defined(FTE_SDL)
	if (newr->bpp && newr->bpp < 24)
	{
		extern int qwinvermaj;
		extern int qwinvermin;
		if ((qwinvermaj == 6 && qwinvermin >= 2) || qwinvermaj>6)
		{
			/*
			Note  Apps that you design to target Windows 8 and later can no longer query or set display modes that are less than 32 bits per pixel (bpp);
				these operations will fail. These apps have a compatibility manifest that targets Windows 8.
			Windows 8 still supports 8-bit and 16-bit color modes for desktop apps that were built without a Windows 8 manifest;
				Windows 8 emulates these modes but still runs in 32-bit color mode.
			*/
			Con_Printf("Starting with windows 8, windows no longer supports 16-bit video modes\n");
			newr->bpp = 24;	//we don't count alpha as part of colour depth. windows does, so this'll end up as 32bit regardless.
		}
	}
#endif

	if (com_installer)
	{
		newr->fullscreen = false;
		newr->width = 640;
		newr->height = 480;
	}

	if (!*vid_vsync.string || vid_vsync.value < 0)
		newr->wait = -1;
	else
		newr->wait = vid_vsync.value;

	newr->renderer = NULL;

	rendererstring = COM_Parse(rendererstring);
	if (r_forceheadless)
	{	//special hack so that android doesn't weird out when not focused.
		for (i = 0; i < countof(rendererinfo); i++)
		{
			if (rendererinfo[i].ri && rendererinfo[i].ri->name[0] && rendererinfo[i].ri->rtype == QR_HEADLESS)
			{
				newr->renderer = rendererinfo[i].ri;
				break;
			}
		}
	}
	else if (!*com_token)
	{
		int bestpri = -2;
		int pri;
		newr->renderer = NULL;
		//I'd like to just qsort the renderers, but that isn't stable and might reorder gl+d3d etc.
		for (i = 0; i < countof(rendererinfo); i++)
		{
			pri = R_PriorityForRenderer(rendererinfo[i].ri);
			if (pri > bestpri)
			{
				bestpri = pri;
				newr->renderer = rendererinfo[i].ri;
			}
		}
	}
	else if (!strcmp(com_token, "random"))
	{
		int count;
		for (i = 0, count = 0; i < countof(rendererinfo); i++)
		{
			if (!rendererinfo[i].ri || !rendererinfo[i].ri->description)
				continue;	//not valid in this build. :(
			if (rendererinfo[i].ri->rtype == QR_NONE		||	//dedicated servers are not useful
				rendererinfo[i].ri->rtype == QR_HEADLESS	||	//headless appears buggy
				rendererinfo[i].ri->rtype == QR_SOFTWARE	)	//software is just TOO buggy/limited for us to care.
				continue;
			count++;
		}
		count = rand()%count;
		for (i = 0; i < countof(rendererinfo); i++)
		{
			if (!rendererinfo[i].ri || !rendererinfo[i].ri->description)
				continue;	//not valid in this build. :(
			if (rendererinfo[i].ri->rtype == QR_NONE		||
				rendererinfo[i].ri->rtype == QR_HEADLESS	||
				rendererinfo[i].ri->rtype == QR_SOFTWARE	)
				continue;
			if (!count--)
			{
				newr->renderer = rendererinfo[i].ri;
				Con_Printf("randomly selected renderer: %s\n", rendererinfo[i].ri->description);
				break;
			}
		}
	}
	else
	{
		int bestpri = -2, pri;
		for (i = 0; i < countof(rendererinfo); i++)
		{
			if (!rendererinfo[i].ri || !rendererinfo[i].ri->description)
				continue;	//not valid in this build. :(
			for (j = 4-1; j >= 0; j--)
			{
				if (!rendererinfo[i].ri->name[j])
					continue;
				if (!stricmp(rendererinfo[i].ri->name[j], com_token))
				{
					pri = R_PriorityForRenderer(rendererinfo[i].ri);

					if (pri > bestpri)
					{
						bestpri = pri;
						newr->renderer = rendererinfo[i].ri;
					}
					break; //try the next renderer now.
				}
			}
		}
	}

	rendererstring = COM_Parse(rendererstring);
	if (*com_token)
		Q_strncpyz(newr->subrenderer, com_token, sizeof(newr->subrenderer));
	else if (newr->renderer && newr->renderer->rtype == QR_OPENGL)
	{
		Q_strncpyz(newr->subrenderer, gl_driver.string, sizeof(newr->subrenderer));
		if (strchr(newr->subrenderer, '/') || strchr(newr->subrenderer, '\\'))
			*newr->subrenderer = 0;	//don't allow this to contain paths. that would be too exploitable - this often takes the form of dll/so names.
	}
	
	Q_strncpyz(newr->devicename, vid_devicename.string, sizeof(newr->devicename));

	// use desktop settings if set to 0 and not dedicated
	if (newr->renderer && newr->renderer->rtype != QR_NONE)
	{
		extern int isPlugin;

		if (vid_desktopsettings.value)
		{
			newr->width = 0;
			newr->height = 0;
			newr->bpp = 0;
			newr->rate = 0;
		}

		if (newr->width <= 0 || newr->height <= 0 || newr->bpp <= 0)
		{
			int dbpp, dheight, dwidth, drate;
			
			if (!newr->fullscreen || isPlugin || !Sys_GetDesktopParameters(&dwidth, &dheight, &dbpp, &drate))
			{
				dwidth = DEFAULT_WIDTH;
				dheight = DEFAULT_HEIGHT;
				dbpp = DEFAULT_BPP;
				drate = 0;
			}

			if (newr->width <= 0)
				newr->width = dwidth;
			if (newr->height <= 0)
				newr->height = dheight;
			if (newr->bpp <= 0)
				newr->bpp = dbpp;
		}
	}

#ifdef CLIENTONLY
	if (newr->renderer && newr->renderer->rtype == QR_NONE)
	{
		Con_Printf("Client-only builds cannot use dedicated modes.\n");
		return false;
	}
#endif

	return newr->renderer != NULL;
}

struct sortedrenderers_s
{
	int index;	//original index, to try to retain stable sort orders.
	int pri;
	rendererinfo_t *r;
};
static int QDECL R_SortRenderers(const void *av, const void *bv)
{
	const struct sortedrenderers_s *a = av;
	const struct sortedrenderers_s *b = bv;
	if (a->pri == b->pri)
		return (a->index > b->index)?1:-1;
	return (a->pri < b->pri)?1:-1;
}

void R_RestartRenderer (rendererstate_t *newr)
{
#if !defined(CLIENTONLY)
	void *portalblob = NULL;
	size_t portalsize = 0;
#endif
	rendererstate_t oldr;
	if (r_blockvidrestart)
	{
		if (r_blockvidrestart != 2)
			Con_TPrintf("Unable to restart renderer at this time\n");
		return;
	}

#ifdef HAVE_SERVER
	if (sv.state == ss_active && sv.world.worldmodel && sv.world.worldmodel->loadstate == MLS_LOADED && sv.world.worldmodel->funcs.SaveAreaPortalBlob)
	{
		void *t;
		portalsize = sv.world.worldmodel->funcs.SaveAreaPortalBlob(sv.world.worldmodel, &t);
		if (portalsize && (portalblob = BZ_Malloc(portalsize)))
			memcpy(portalblob, t, portalsize);
	}
#endif

	TRACE(("dbg: R_RestartRenderer_f renderer %p\n", newr->renderer));

	memcpy(&oldr, &currentrendererstate, sizeof(rendererstate_t));
	if (!R_ApplyRenderer(newr))
	{
		TRACE(("dbg: R_RestartRenderer_f failed\n"));
		if (R_ApplyRenderer(&oldr))
		{
			TRACE(("dbg: R_RestartRenderer_f old restored\n"));
			Con_Printf(CON_ERROR "Video mode switch failed. Old mode restored.\n");	//go back to the old mode, the new one failed.
		}
		else
		{
			int i;
			qboolean failed = true;
			rendererinfo_t *skip = newr->renderer;
			struct sortedrenderers_s sorted[countof(rendererinfo)];

			if (failed && newr->vr)
			{
				Con_Printf(CON_NOTICE "Trying without vr\n");
				newr->vr = NULL;
				failed = !R_ApplyRenderer(newr);
			}
			if (failed && newr->fullscreen == 1)
			{
				Con_Printf(CON_NOTICE "Trying fullscreen windowed"CON_DEFAULT"\n");
				newr->fullscreen = 2;
				failed = !R_ApplyRenderer(newr);
			}
			if (failed && newr->rate != 0)
			{
				Con_Printf(CON_NOTICE "Trying default refresh rate"CON_DEFAULT"\n");
				newr->rate = 0;
				failed = !R_ApplyRenderer(newr);
			}
			if (failed && newr->width != DEFAULT_WIDTH && newr->height != DEFAULT_HEIGHT)
			{
				Con_Printf(CON_NOTICE "Trying %i*%i"CON_DEFAULT"\n", DEFAULT_WIDTH, DEFAULT_HEIGHT);
				if (newr->fullscreen == 2)
					newr->fullscreen = 1;
				newr->width = DEFAULT_WIDTH;
				newr->height = DEFAULT_HEIGHT;
				failed = !R_ApplyRenderer(newr);
			}
			if (failed && newr->fullscreen)
			{
				Con_Printf(CON_NOTICE "Trying windowed"CON_DEFAULT"\n");
				newr->fullscreen = 0;
				failed = !R_ApplyRenderer(newr);
			}

			for (i = 0; i < countof(sorted); i++)
			{
				sorted[i].index = i;
				sorted[i].r = rendererinfo[i].ri;
				sorted[i].pri = R_PriorityForRenderer(sorted[i].r);
			}
			qsort(sorted, countof(sorted), sizeof(sorted[0]), R_SortRenderers);
			for (i = 0; failed && i < countof(sorted); i++)
			{
				newr->renderer = sorted[i].r;
				if (newr->renderer && newr->renderer != skip && newr->renderer->rtype != QR_HEADLESS)
				{
					Con_Printf(CON_NOTICE "Trying %s"CON_DEFAULT"\n", newr->renderer->description);
					failed = !R_ApplyRenderer(newr);
				}
			}

			//if we ended up resorting to our last choice (dedicated) then print some informative message about it
			//fixme: on unixy systems, we should make sure we're actually printing to something (ie: that we're not running via some x11 shortcut with our stdout redirected to /dev/nul
			if (!failed && (!newr->renderer || newr->renderer->rtype == QR_NONE))
			{
				Con_Printf(CON_ERROR "Video mode switch failed. Console forced.\n\nPlease change the following vars to something useable, and then use the setrenderer command.\n");
				Con_Printf("%s: %s\n", vid_width.name, vid_width.string);
				Con_Printf("%s: %s\n", vid_height.name, vid_height.string);
				Con_Printf("%s: %s\n", vid_bpp.name, vid_bpp.string);
				Con_Printf("%s: %s\n", vid_depthbits.name, vid_depthbits.string);
				Con_Printf("%s: %s\n", vid_refreshrate.name, vid_refreshrate.string);
				Con_Printf("%s: %s\n", vid_renderer.name, vid_renderer.string);
				Con_Printf("%s: %s\n", gl_driver.name, gl_driver.string);
			}

			if (failed)
				Sys_Error("Unable to initialise any video mode\n");
		}
	}

#ifdef HAVE_SERVER
	if (portalblob)
	{
		if (sv.world.worldmodel && sv.world.worldmodel->loadstate == MLS_LOADED && sv.world.worldmodel->funcs.LoadAreaPortalBlob)
			sv.world.worldmodel->funcs.LoadAreaPortalBlob(sv.world.worldmodel, portalblob, portalsize);
		BZ_Free(portalblob);
	}
#endif

	Cvar_ApplyCallbacks(CVAR_RENDERERCALLBACK);
	SCR_EndLoadingPlaque();

	TRACE(("dbg: R_RestartRenderer_f success\n"));
//	M_Reinit();
}

void R_RestartRenderer_f (void)
{
	double time;
	rendererstate_t newr;

	if (r_blockvidrestart)
	{
		if (r_blockvidrestart!=2)
			Con_TPrintf("Ignoring vid_restart from config\n");
		return;
	}

	Cvar_ApplyLatches(CVAR_VIDEOLATCH|CVAR_RENDERERLATCH, false);
	if (!R_BuildRenderstate(&newr, vid_renderer.string))
	{
		Con_Printf("vid_renderer \"%s\" unsupported. Using default.\n", vid_renderer.string);

		//gotta do this after main hunk is saved off.
		Cmd_ExecuteString("setrenderer \"\"\n", RESTRICT_LOCAL);
		return;
	}

	time = Sys_DoubleTime();
	R_RestartRenderer(&newr);
	Con_DPrintf("main thread video restart took %f secs\n", Sys_DoubleTime() - time);
//	COM_WorkerFullSync();
//	Con_Printf("full video restart took %f secs\n", Sys_DoubleTime() - time);
}

static void R_EnumeratedRenderer(void *ctx, const char *devname, const char *outputname, const char *desc)
{
	rendererinfo_t *r = ctx;
	char quoteddesc[1024];

	qboolean iscurrent = (currentrendererstate.renderer == r && (!*devname || !strcmp(devname, currentrendererstate.subrenderer)));

	const char *dev;
	if (*outputname)
		dev = va("%s %s %s", r->name[0], devname, outputname);
	else if (*devname)
		dev = va("%s %s", r->name[0], devname);
	else
		dev = r->name[0];
	dev = COM_QuotedString(dev, quoteddesc,sizeof(quoteddesc), false);

	if (*outputname)
		Con_Printf("^[%s (%s, %s)\\type\\/setrenderer %s^]^7: %s%s\n",
			r->name[0], devname, outputname,	//link text
			dev,	//link itself.
			desc, iscurrent?" ^2(current)":"");
	else if (*devname)
		Con_Printf("^[%s (%s)\\type\\/setrenderer %s^]^7: %s%s\n",
			r->name[0], devname,	//link text
			dev,	//link itself.
			desc, iscurrent?" ^2(current)":"");
	else
		Con_Printf("^[%s\\type\\/setrenderer %s^]^7: %s%s\n", r->name[0], dev, r->description, iscurrent?" ^2(current)":"");
}

void R_SetRenderer_f (void)
{
	int i;
	char *param = Cmd_Argv(1);
	rendererstate_t newr;

	if (Cmd_Argc() == 1 || !stricmp(param, "help"))
	{
		struct sortedrenderers_s sorted[countof(rendererinfo)];
		for (i = 0; i < countof(sorted); i++)
		{
			sorted[i].index = i;
			sorted[i].r = rendererinfo[i].ri;
			sorted[i].pri = R_PriorityForRenderer(sorted[i].r);
		}
		qsort(sorted, countof(sorted), sizeof(sorted[0]), R_SortRenderers);

		Con_Printf ("\nValid setrenderer parameters are:\n");
		for (i = 0; i < countof(rendererinfo); i++)
		{
			rendererinfo_t *r = sorted[i].r;
			if (r && r->description)
			{
				if (!r->VID_EnumerateDevices || !r->VID_EnumerateDevices(r, R_EnumeratedRenderer))
					R_EnumeratedRenderer(r, "", "", r->description);
			}
		}
		return;
	}

	Cvar_ApplyLatches(CVAR_VIDEOLATCH|CVAR_RENDERERLATCH, false);
	if (!R_BuildRenderstate(&newr, param))
	{
		Con_Printf("setrenderer: parameter not supported (%s)\n", param);
		return;
	}
	else
	{
		if (Cmd_Argc() == 3)
			Cvar_Set(&vid_bpp, Cmd_Argv(2));
	}

	if (newr.renderer->rtype != QR_HEADLESS && !strstr(param, "headless"))	//don't save headless in the vid_renderer cvar via the setrenderer command. 'setrenderer headless;vid_restart' can then do what is most sane.
		Cvar_ForceSet(&vid_renderer, param);

	if (!r_blockvidrestart)
		R_RestartRenderer(&newr);
}

struct videnumctx_s
{
	char *v;
	char *apiname;
};
static void R_DeviceEnumerated(void *context, const char *devicename, const char *outputname, const char *description)
{
	struct videnumctx_s *ctx = context;
	char quotedname[1024];
	char quoteddesc[1024];
	char *name = va("%s %s %s", ctx->apiname,
			COM_QuotedString(devicename, quotedname,sizeof(quotedname), false),
			COM_QuotedString(outputname, quoteddesc,sizeof(quoteddesc), false));
	Z_StrCat(&ctx->v, va("%s %s ", COM_QuotedString(name, quotedname, sizeof(quotedname), false), COM_QuotedString(description, quoteddesc, sizeof(quoteddesc), false)));
}

static qboolean rendereroptsupdated;
static void R_UpdateRendererOptsNow(int iarg, void *data)
{
	struct videnumctx_s e = {NULL};
	size_t i;
	struct sortedrenderers_s sorted[countof(rendererinfo)];
	qboolean safe = COM_CheckParm("-noenumerate") || COM_CheckParm("-safe");
	if (!rendereroptsupdated)
		return;	//multiple got queued...
	rendereroptsupdated = false;

	for (i = 0; i < countof(sorted); i++)
	{
		sorted[i].index = i;
		sorted[i].r = rendererinfo[i].ri;
		sorted[i].pri = R_PriorityForRenderer(sorted[i].r);
	}
	qsort(sorted, countof(sorted), sizeof(sorted[0]), R_SortRenderers);


	e.v = NULL;
	for (i = 0; i < countof(rendererinfo); i++)
	{
		rendererinfo_t *r = sorted[i].r;
		if (r && r->description)
		{
			if (r->rtype == QR_HEADLESS || r->rtype == QR_NONE)
				continue;	//skip these, they're kinda dangerous.
			e.apiname = r->name[0];
			if (safe || !r->VID_EnumerateDevices || !r->VID_EnumerateDevices(&e, R_DeviceEnumerated))
				R_DeviceEnumerated(&e, r->name[0], "", r->description);
		}
	}

	Cvar_SetEngineDefault(&vid_renderer_opts, e.v?e.v:"");
	Cvar_ForceSet(&vid_renderer_opts, e.v?e.v:"");
	Z_Free(e.v);
}
static void R_UpdateRendererOpts(void)
{	//use a timer+flag, so we don't reenumerate everything any time any device gets registered.
	rendereroptsupdated	= true;
	Cmd_AddTimer(0, R_UpdateRendererOptsNow, 0, NULL, 0);
}

















/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = currententity->model->meshinfo;
	frame = currententity->framestate.g[FS_REG].frame[0];

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_DrawSprite: no such frame %d (%s)\n", frame, currententity->model->name);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else if (psprite->frames[frame].type == SPR_ANGLED)
	{
		float f = DotProduct(vpn,currententity->axis[0]);
		float r = DotProduct(vright,currententity->axis[0]);
		float ang;
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		ang = (atan2(r, f)+M_PI)/(2*M_PI);	//to give 0 - 1 range
		i = (ang*pspritegroup->numframes) + 0.5;

//		pspriteframe = pspritegroup->frames[(int)((r_refdef.viewangles[1]-currententity->angles[1])/360*pspritegroup->numframes + 0.5-4)%pspritegroup->numframes];
		//int dir = (int)((r_refdef.viewangles[1]-currententity->angles[1])/360*8 + 8 + 0.5-4)&7;
		pspriteframe = pspritegroup->frames[i%pspritegroup->numframes];
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = currententity->framestate.g[FS_REG].frametime[0];

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (int frame, texture_t *base)
{
	unsigned int	relative;
	int				count;

	if (frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	relative = (unsigned int)(cl.time*10) % base->anim_total;

	count = 0;
	while (base->anim_min > relative || base->anim_max <= relative)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}

texture_t *R_TextureAnimation_Q2 (texture_t *base)
{
	int		reletive;
	int		frame;
	
	if (!base->anim_total)
		return base;

	//this is only ever used on world. everything other than rtlights have proper batches.
	frame = cl.time*2;	//q2 is lame

	reletive = frame % base->anim_total;

	while (reletive --> 0)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
	}

	return base;
}




unsigned int	r_viewcontents;
//mleaf_t		*r_viewleaf, *r_oldviewleaf;
//mleaf_t		*r_viewleaf2, *r_oldviewleaf2;
int r_viewarea;
int		r_viewcluster, r_viewcluster2;


/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	//this isn't very precise.
	//checking each plane individually can be problematic
	//if you have a large object behind the view, it can cross multiple planes, and be infront of each one at some point, yet should still be outside the view.
	//this is quite noticable with terrain where the potential height of a section is essentually infinite.
	//note that this is not a concern for spheres, just boxes.
	int		i;

	for (i = 0; i < r_refdef.frustum_numplanes; i++)
		if (BOX_ON_PLANE_SIDE (mins, maxs, &r_refdef.frustum[i]) == 2)
			return true;
	return false;
}

qboolean R_CullSphere (vec3_t org, float radius)
{
	//four frustrum planes all point inwards in an expanding 'cone'.
	int		i;
	float d;

	for (i = 0; i < r_refdef.frustum_numplanes; i++)
	{
		d = DotProduct(r_refdef.frustum[i].normal, org)-r_refdef.frustum[i].dist;
		if (d <= -radius)
			return true;
	}
	return false;
}

qboolean R_CullEntityBox(entity_t *e, vec3_t modmins, vec3_t modmaxs)
{
	int i;
	vec3_t wmin, wmax;

#if 1
	float mrad = 0, v;

	if (e->axis[0][0]==1 && e->axis[0][1]==0 && e->axis[0][2]==0 &&
		e->axis[1][0]==0 && e->axis[1][1]==1 && e->axis[1][2]==0 &&
		e->axis[2][0]==0 && e->axis[2][1]==0 && e->axis[2][2]==1)
	{
		for (i = 0; i < 3; i++)
		{
			wmin[i] = e->origin[i]+modmins[i]*e->scale;
			wmax[i] = e->origin[i]+modmaxs[i]*e->scale;
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			v = fabs(modmins[i]);
			if (mrad < v)
				mrad = v;
			v = fabs(modmaxs[i]);
			if (mrad < v)
				mrad = v;
		}
		mrad *= e->scale;
		for (i = 0; i < 3; i++)
		{
			wmin[i] = e->origin[i]-mrad;
			wmax[i] = e->origin[i]+mrad;
		}
	}
#else
	float fmin, fmax;

	//convert the model's bbox to the expanded maximum size of the entity, as drawn with this model.
	//The result is an axial box, which we pass to R_CullBox

	for (i = 0; i < 3; i++)
	{
		fmin = DotProduct(modmins, e->axis[i]);
		fmax = DotProduct(modmaxs, e->axis[i]);

		if (fmin > -16)
			fmin = -16;
		if (fmax < 16)
			fmax = 16;

		if (fmin < fmax)
		{
			wmin[i] = e->origin[i]+fmin;
			wmax[i] = e->origin[i]+fmax;
		}
		else
		{       //box went inside out
			wmin[i] = e->origin[i]+fmax;
			wmax[i] = e->origin[i]+fmin;
		}
	}
#endif

	return R_CullBox(wmin, wmax);
}




int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}
void R_SetFrustum (float projmat[16], float viewmat[16])
{
	float scale;
	int i;
	float mvp[16];
	mplane_t *p;

	if (r_novis.ival & 4)
		return;

	Matrix4_Multiply(projmat, viewmat, mvp);

	r_refdef.frustum_numplanes = 0;

	for (i = 0; i < 4; i++)
	{
		//each of the four side planes
		p = &r_refdef.frustum[r_refdef.frustum_numplanes++];
		if (i & 1)
		{
			p->normal[0] = mvp[3] + mvp[0+i/2];
			p->normal[1] = mvp[7] + mvp[4+i/2];
			p->normal[2] = mvp[11] + mvp[8+i/2];
			p->dist		 = mvp[15] + mvp[12+i/2];
		}
		else
		{
			p->normal[0] = mvp[3] - mvp[0+i/2];
			p->normal[1] = mvp[7] - mvp[4+i/2];
			p->normal[2] = mvp[11] - mvp[8+i/2];
			p->dist		 = mvp[15] - mvp[12+i/2];
		}

		scale = 1/sqrt(DotProduct(p->normal, p->normal));
		p->normal[0] *= scale;
		p->normal[1] *= scale;
		p->normal[2] *= scale;
		p->dist		 *= -scale;

		p->type		 = PLANE_ANYZ;
		p->signbits	 = SignbitsForPlane (p);
	}

	//the near clip plane.
	p = &r_refdef.frustum[r_refdef.frustum_numplanes++];

	p->normal[0] = mvp[3] - mvp[2];
	p->normal[1] = mvp[7] - mvp[6];
	p->normal[2] = mvp[11] - mvp[10];
	p->dist      = mvp[15] - mvp[14];

	scale = 1/sqrt(DotProduct(p->normal, p->normal));
	p->normal[0] *= scale;
	p->normal[1] *= scale;
	p->normal[2] *= scale;
	p->dist *= -scale;

	p->type      = PLANE_ANYZ;
	p->signbits  = SignbitsForPlane (p);

	r_refdef.frustum_numworldplanes = r_refdef.frustum_numplanes;

	//do far plane
	//fog will logically not actually reach 0, though precision issues will force it. we cut off at an exponant of -500
	if (r_refdef.globalfog.density && r_refdef.globalfog.alpha>=1 && (r_fog_cullentities.ival==2||(r_fog_cullentities.ival&&r_skyfog.value>=1)) && !r_refdef.globalfog.depthbias)
	{
		float culldist;
		float fog;
		extern cvar_t r_fog_exp2;

		/*Documentation: the GLSL/GL will do this maths:
		float dist = 1024;
		if (r_fog_exp2.ival)
			fog = pow(2, -r_refdef.globalfog.density * r_refdef.globalfog.density * dist * dist * 1.442695);
		else
			fog = pow(2, -r_refdef.globalfog.density * dist * 1.442695);
		*/

		//the fog factor cut-off where its pointless to allow it to get closer to 0 (0 is technically infinite)
		fog = 2/255.0f;

		//figure out the eyespace distance required to reach that fog value
		culldist = log(fog);
		if (r_fog_exp2.ival)
			culldist = sqrt(culldist / (-r_refdef.globalfog.density * r_refdef.globalfog.density));
		else
			culldist = culldist / (-r_refdef.globalfog.density);
		//anything drawn beyond this point is fully obscured by fog

		p = &r_refdef.frustum[r_refdef.frustum_numplanes++];
		p->normal[0] = mvp[3] - mvp[2];
		p->normal[1] = mvp[7] - mvp[6];
		p->normal[2] = mvp[11] - mvp[10];
		p->dist      = mvp[15] - mvp[14];

		scale = 1/sqrt(DotProduct(p->normal, p->normal));
		p->normal[0] *= -scale;
		p->normal[1] *= -scale;
		p->normal[2] *= -scale;
//		p->dist *= scale;
		p->dist	= DotProduct(r_refdef.vieworg, p->normal)-culldist;

		p->type      = PLANE_ANYZ;
		p->signbits  = SignbitsForPlane (p);
	}
}




#include "glquake.h"

//we could go for nice smooth round particles... but then we would loose a little bit of the chaotic nature of the particles.
static qbyte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,0,1,1,0,0,0},
	{0,0,1,1,1,1,0,0},
	{0,1,1,1,1,1,1,0},
	{0,1,1,1,1,1,1,0},
	{0,0,1,1,1,1,0,0},
	{0,0,0,1,1,0,0,0},
	{0,0,0,0,0,0,0,0},
};
static qbyte	exptexture[16][16] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,1,0,0,1,0,0,0,0},
	{0,0,0,1,1,1,1,1,3,1,1,2,1,0,0,0},
	{0,0,0,1,1,1,1,4,4,4,5,4,2,1,1,0},
	{0,0,1,1,6,5,5,8,6,8,3,6,3,2,1,0},
	{0,0,1,5,6,7,5,6,8,8,8,3,3,1,0,0},
	{0,0,0,1,6,8,9,9,9,9,4,6,3,1,0,0},
	{0,0,2,1,7,7,9,9,9,9,5,3,1,0,0,0},
	{0,0,2,4,6,8,9,9,9,9,8,6,1,0,0,0},
	{0,0,2,2,3,5,6,8,9,8,8,4,4,1,0,0},
	{0,0,1,2,4,1,8,7,8,8,6,5,4,1,0,0},
	{0,1,1,1,7,8,1,6,7,5,4,7,1,0,0,0},
	{0,1,2,1,1,5,1,3,4,3,1,1,0,0,0,0},
	{0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

texid_t			particletexture;	// little dot for particles
texid_t			particlecqtexture;	// little dot for particles
texid_t			explosiontexture;
texid_t			balltexture;
texid_t			beamtexture;
texid_t			ptritexture;
void R_InitParticleTexture (void)
{
#define PARTICLETEXTURESIZE 64
	int		x,y;
	float dx, dy, d;
	qbyte	data[PARTICLETEXTURESIZE*PARTICLETEXTURESIZE][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y*8+x][0] = 255;
			data[y*8+x][1] = 255;
			data[y*8+x][2] = 255;
			data[y*8+x][3] = dottexture[x][y]*255;
		}
	}

	TEXASSIGN(particletexture, R_LoadTexture32("dotparticle", 8, 8, data, IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP|IF_NOPURGE|IF_INEXACT));


	//
	// particle triangle texture
	//

	// clear to transparent white
	for (x = 0; x < 32 * 32; x++)
	{
			data[x][0] = 255;
			data[x][1] = 255;
			data[x][2] = 255;
			data[x][3] = 0;
	}
	//draw a circle in the top left.
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			if ((x - 7.5) * (x - 7.5) + (y - 7.5) * (y - 7.5) <= 8 * 8)
				data[y*32+x][3] = 255;
		}
	}
	particlecqtexture = Image_GetTexture("classicparticle", "particles", IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP|IF_NOPURGE|IF_INEXACT, data, NULL, 32, 32, TF_RGBA32);

	//draw a square in the top left. still a triangle.
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y*32+x][3] = 255;
		}
	}
	Image_GetTexture("classicparticle_square", "particles", IF_NOMIPMAP|IF_NOPICMIP|IF_CLAMP|IF_NOPURGE|IF_INEXACT, data, NULL, 32, 32, TF_RGBA32);


	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y*16+x][0] = 255;
			data[y*16+x][1] = 255;
			data[y*16+x][2] = 255;
			data[y*16+x][3] = exptexture[x][y]*255/9.0;
		}
	}
	explosiontexture = Image_GetTexture("fte_fuzzyparticle", "particles", IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT, data, NULL, 16, 16, TF_RGBA32);

	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y*16+x][0] = exptexture[x][y]*255/9.0;
			data[y*16+x][1] = exptexture[x][y]*255/9.0;
			data[y*16+x][2] = exptexture[x][y]*255/9.0;
			data[y*16+x][3] = exptexture[x][y]*255/9.0;
		}
	}
	Image_GetTexture("fte_bloodparticle", "particles", IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT, data, NULL, 16, 16, TF_RGBA32);

	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y*16+x][0] = min(255, exptexture[x][y]*255/9.0);
			data[y*16+x][1] = min(255, exptexture[x][y]*255/5.0);
			data[y*16+x][2] = min(255, exptexture[x][y]*255/5.0);
			data[y*16+x][3] = 255;
		}
	}
	Image_GetTexture("fte_blooddecal", "particles", IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT, data, NULL, 16, 16, TF_RGBA32);

	memset(data, 255, sizeof(data));
	for (y = 0;y < PARTICLETEXTURESIZE;y++)
	{
		dy = (y - 0.5f*PARTICLETEXTURESIZE) / (PARTICLETEXTURESIZE*0.5f-1);
		for (x = 0;x < PARTICLETEXTURESIZE;x++)
		{
			dx = (x - 0.5f*PARTICLETEXTURESIZE) / (PARTICLETEXTURESIZE*0.5f-1);
			d = 256 * (1 - (dx*dx+dy*dy));
			d = bound(0, d, 255);
			data[y*PARTICLETEXTURESIZE+x][3] = (qbyte) d;
		}
	}
	balltexture = R_LoadTexture32("balltexture", PARTICLETEXTURESIZE, PARTICLETEXTURESIZE, data, IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT);

	memset(data, 255, sizeof(data));
	for (y = 0;y < PARTICLETEXTURESIZE;y++)
	{
		dy = (y - 0.5f*PARTICLETEXTURESIZE) / (PARTICLETEXTURESIZE*0.5f-1);
		d = 256 * (1 - (dy*dy));
		d = bound(0, d, 255);
		for (x = 0;x < PARTICLETEXTURESIZE;x++)
		{
			data[y*PARTICLETEXTURESIZE+x][3] = (qbyte) d;
		}
	}
	beamtexture = R_LoadTexture32("beamparticle", PARTICLETEXTURESIZE, PARTICLETEXTURESIZE, data, IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT);

	for (y = 0;y < PARTICLETEXTURESIZE;y++)
	{
		dy = y / (PARTICLETEXTURESIZE*0.5f-1);
		d = 256 * (1 - (dy*dy));
		d = bound(0, d, 255);
		for (x = 0;x < PARTICLETEXTURESIZE;x++)
		{
			dx = x / (PARTICLETEXTURESIZE*0.5f-1);
			d = 256 * (1 - (dx+dy));
			d = bound(0, d, 255);
			data[y*PARTICLETEXTURESIZE+x][0] = (qbyte) d;
			data[y*PARTICLETEXTURESIZE+x][1] = (qbyte) d;
			data[y*PARTICLETEXTURESIZE+x][2] = (qbyte) d;
			data[y*PARTICLETEXTURESIZE+x][3] = (qbyte) d/2;
		}
	}
	ptritexture = R_LoadTexture32("ptritexture", PARTICLETEXTURESIZE, PARTICLETEXTURESIZE, data, IF_NOMIPMAP|IF_NOPICMIP|IF_NOPURGE|IF_INEXACT);
}

