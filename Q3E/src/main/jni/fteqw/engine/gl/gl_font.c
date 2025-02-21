#include "quakedef.h"

#ifndef SERVERONLY
#include "shader.h"
#include "gl_draw.h"

#ifdef _WIN32
	#ifdef _XBOX
		#include <xtl.h>
	#else
		#include <windows.h>
	#endif
#endif
#include <ctype.h>

/* Font Names:
	primaryface[,altface[...]][?primaryarg[&arg[...]]][:[secondaryface[,altface[...]]][?secondaryargs]]

	args:
		col=r,g,b -- tint the font (overriding the game's normal tint). only arg allowed when the secondary face is omitted.
		fmt=q	-- quake-style raster font with quake's own codepage (no fallbacks needed for the weird glyphs).
		fmt=l	-- quake-style raster font with io8859-1(latin-1) codepage
		fmt=w	-- quake-style raster font with windows1252 codepage (for more glyphs than latin-1)
		fmt=k	-- quake-style raster font with koi8-u codepage (apparently its somewhat common in the quake community)
		fmt=h	-- halflife-style all-on-one-line raster font
		aspect=0.5 -- raster font is squished horizontally
		style	-- list of modifiers for inexact family font matching for system fonts
*/

void Font_Init(void);
void Font_Shutdown(void);
struct font_s *Font_LoadFont(const char *fontfilename, float height, float scale, int outline, unsigned int flags);
void Font_Free(struct font_s *f);
void Font_BeginString(struct font_s *font, float vx, float vy, int *px, int *py);
void Font_BeginScaledString(struct font_s *font, float vx, float vy, float szx, float szy, float *px, float *py); /*avoid using*/
void Font_Transform(float vx, float vy, int *px, int *py);
int Font_CharHeight(void);
float Font_CharScaleHeight(void);

//FIXME: if we want to do emoji properly, then we're going to need support for variants somehow
//       easiest is probably to make the next codepoint available too (and consumable)
//       handling variants in the font cache is another issue. gah. not worth it.
int Font_CharWidth(unsigned int charflags, unsigned int codepoint);
float Font_CharScaleWidth(unsigned int charflags, unsigned int codepoint);
int Font_CharEndCoord(struct font_s *font, int x, unsigned int charflags, unsigned int codepoint);
int Font_DrawChar(int px, int py, unsigned int charflags, unsigned int codepoint);
float Font_DrawScaleChar(float px, float py, unsigned int charflags, unsigned int codepoint); /*avoid using*/

void Font_EndString(struct font_s *font);
int Font_LineBreaks(conchar_t *start, conchar_t *end, int maxpixelwidth, int maxlines, conchar_t **starts, conchar_t **ends);
struct font_s *font_menu;
struct font_s *font_default;
struct font_s *font_console;
struct font_s *font_tiny;
static int font_be_flags;
extern unsigned int r2d_be_flags;

#ifdef AVAIL_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H 
struct fontface_s;
int Font_ChangeFTSize(struct fontface_s *qface, int pixelheight);

#ifndef FT_LOAD_COLOR
#define FT_LOAD_COLOR (1L<<20)
#endif

static FT_Library fontlib;

#ifdef FREETYPE_STATIC
#define pFT_Init_FreeType	FT_Init_FreeType
#define pFT_Load_Char		FT_Load_Char
#define pFT_Get_Char_Index	FT_Get_Char_Index
#define pFT_Set_Pixel_Sizes	FT_Set_Pixel_Sizes
#define pFT_Select_Size		FT_Select_Size
#define pFT_New_Face		FT_New_Face
#define pFT_New_Memory_Face	FT_New_Memory_Face
#define pFT_Done_Face		FT_Done_Face
#define pFT_Error_String	FT_Error_String
#else
qboolean triedtoloadfreetype;
dllhandle_t *fontmodule;
FT_Error (VARGS *pFT_Init_FreeType)		(FT_Library  *alibrary);
FT_Error (VARGS *pFT_Load_Char)			(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
FT_UInt	 (VARGS *pFT_Get_Char_Index)	(FT_Face face, FT_ULong charcode);
FT_Error (VARGS *pFT_Set_Pixel_Sizes)	(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
FT_Error (VARGS *pFT_Select_Size)		(FT_Face face, FT_Int strike_index);
FT_Error (VARGS *pFT_New_Face)			(FT_Library library, const char *pathname, FT_Long face_index, FT_Face *aface);
FT_Error (VARGS *pFT_New_Memory_Face)	(FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face *aface);
FT_Error (VARGS *pFT_Done_Face)			(FT_Face face);
const char *(VARGS *pFT_Error_String)	(FT_Error  error_code);
#endif
#else
typedef unsigned int FT_Pixel_Mode; //for consistency even without freetype support.
#endif

#ifndef FT_PIXEL_MODE_MONO
#define FT_PIXEL_MODE_MONO 1
#endif
#ifndef FT_PIXEL_MODE_GRAY
#define FT_PIXEL_MODE_GRAY 2
#endif
#ifndef FT_PIXEL_MODE_BGRA
#define FT_PIXEL_MODE_BGRA 7	//added in FT 2.5
#endif
#define FT_PIXEL_MODE_RGBA_SA (100)	//RGBA, straight alpha. not in freetype.
#define FT_PIXEL_MODE_RGBA (101)	//RGBA, premultiplied alpha. not in freetype.

#ifdef QUAKEHUD
static const char *imgs[] =
{
	//0xe10X
	"e100","e101",
	"inv_shotgun",
	"inv_sshotgun",
	"inv_nailgun",
	"inv_snailgun",
	"inv_rlaunch",
	"inv_srlaunch",
	"inv_lightng",	//8
	"e109","e10a","e10b","e10c","e10d","e10e","e10f",

	//0xe11X
	"e110","e111",
	"inv2_shotgun",
	"inv2_sshotgun",
	"inv2_nailgun",
	"inv2_snailgun",
	"inv2_rlaunch",
	"inv2_srlaunch",
	"inv2_lightng",
	"e119","e11a","e11b","e11c","e11d","e11e","e11f",

	//0xe12X
	"sb_shells",
	"sb_nails",
	"sb_rocket",
	"sb_cells",

	"sb_armor1",
	"sb_armor2",
	"sb_armor3",
	"e127","e128","e129","e12a","e12b","e12c","e12d","e12e","e12f",

	//0xe13X
	"sb_key1",
	"sb_key2",
	"sb_invis",
	"sb_invuln",
	"sb_suit",
	"sb_quad",

	"sb_sigil1",
	"sb_sigil2",
	"sb_sigil3",
	"sb_sigil4",
	"e13a","e13b","e13c","e13d","e13e","e13f",

	//0xe14X
	"face1",
	"face_p1",
	"face2",
	"face_p2",
	"face3",
	"face_p3",
	"face4",
	"face_p4",
	"face5",
	"face_p5",

	"face_invis",
	"face_invul2",
	"face_inv2",
	"face_quad",
	"e14e",
	"e14f",

	//0xe15X
	"e150",
	"e151",
	"e152",
	"e153",
	"e154",
	"e155",
	"e156",
	"e157",
	"e158",
	"e159",
	"e15a",
	"e15b",
	"e15c",
	"e15d",
	"e15e",
	"e15f",

	//0xe16X
	"e160",
	"e161",
	"e162",
	"e163",
	"e164",
	"e165",
	"e166",
	"e167",
	"e168",
	"e169",
	"e16a",
	"e16b",
	"e16c",
	"e16d",
	"e16e",
	"e16f"
};
#endif

#define FONT_MAXCHARS 0x110000 //as defined by UTF-16, and thus applied to all unicode because UTF-16 is the crappy limited one.
#define FONTBLOCKS ((FONT_MAXCHARS+FONTBLOCKSIZE-1)/FONTBLOCKSIZE)
#define FONTBLOCKSIZE 0x100	//must be power-of-two
#define FONTBLOCKMASK (FONTBLOCKSIZE-1)
#define FONTIMAGES (1<<2)	//this is total, not per font.
#define FIMAGEIDXTYPE unsigned char
#define CHARIDXTYPE unsigned int

#define INVALIDPLANE ((1<<(8*sizeof(FIMAGEIDXTYPE)))-1)
#define BITMAPPLANE ((1<<(8*sizeof(FIMAGEIDXTYPE)))-2)
#define DEFAULTPLANE ((1<<(8*sizeof(FIMAGEIDXTYPE)))-3)
#define SINGLEPLANE ((1<<(8*sizeof(FIMAGEIDXTYPE)))-4)
#define TRACKERIMAGE ((1<<(8*sizeof(FIMAGEIDXTYPE)))-5)
#define FIMAGEWIDTH (1<<10)
#define FIMAGEHEIGHT FIMAGEWIDTH

#define FONTPLANES FONTIMAGES
#define PLANEWIDTH FIMAGEWIDTH
#define PLANEHEIGHT FIMAGEHEIGHT

//windows' font linking allows linking multiple extra fonts to a main font.
//this means that a single selected font can use chars from lots of different files if the first one(s) didn't provide that font.
//they're provided as fallbacks.
#define MAX_FACES 32

enum fontfmt_e
{
	FMT_AUTO,		//freetype, or quake
	FMT_QUAKE,		//first is default
	FMT_ISO88591,	//latin-1 (first 256 chars of unicode too, c1 glyphs are usually invisible)
	FMT_WINDOWS1252,//variation of latin-1 with extra glyphs
	FMT_KOI8U,		//image is 16*16 koi8-u codepage.
	FMT_HORIZONTAL,	//unicode, charcount=width/(height-2). single strip of chars, like halflife.
};

typedef struct fontface_s
{
	struct fontface_s *fnext;
	struct fontface_s **flink;	//like prev, but not.
	char name[MAX_OSPATH];
	int refs;

	struct
	{
		qbyte *data;
		size_t rows;	//urgh.
		size_t stride;
		size_t charheight;
		enum fontfmt_e codepage;
		qboolean paletted;
	} horiz;

#ifdef HALFLIFEMODELS
	struct
	{
		int fontheight1;
		int imgheight;
		int rows;
		int fontheight2;

		struct
		{
			qbyte x;
			qbyte y;
			qbyte width;
			qbyte pad;
		} chartab[256];

//		int unk[4];

		qbyte data[1];//[256*imgheight];
		//int pad
		//palette[256*3];
	} *halflife;
#endif

#ifdef AVAIL_FREETYPE
	struct
	{
		int activeheight;	//needs reconfiguring when different sizes are used (so the same face can be used at multiple different sizes
		int actualsize;		//sometimes that activeheight isn't usable and we need to manually rescale. :(
		FT_Face face;
		void *membuf;
	} ft;
#endif
} fontface_t;
static fontface_t *faces;


#define GEN_CONCHAR_GLYPHS 0	//set to 0 or 1 to define whether to generate glyphs from conchars too, or if it should just draw them as glquake always used to
extern cvar_t cl_noblink;
extern cvar_t con_ocranaleds;

typedef struct font_s
{
	//FIXME: use a hash table? will need it if we go beyond ucs-2.
	//currently they're in a single block so the font can be checked from scanning the active chars when shutting down. we could instead scan all 65k chars in every font instead...
	struct charcache_s
	{
		struct charcache_s *nextchar;

		FIMAGEIDXTYPE texplane;
		unsigned char advance;	//how wide this char is, when drawn
		unsigned short block;	//to quickly find the char again

		//texture offsets.
		unsigned short bmx;
		unsigned short bmy;

		//texture/screen pixel sizes.
		unsigned char bmw;
		unsigned char bmh;
		unsigned short flags;
#define CHARF_FORCEWHITE (1u<<0)	//coloured emoji should not be tinted.

		//screen offsets.
		short top;
		short left;
	} *chars[FONTBLOCKS];
	char name[MAX_OSPATH];

	texid_t singletexture;
	unsigned short charheight;		//requested height (space between lines)
	unsigned short truecharheight;	//what you actually got, for compat with dp's lets-use-the-wrong-size-for-double-padding-between-lines thing.
	float scale;	//some sort of poop
	short outline;
	unsigned int flags;

	unsigned short faces;
	fontface_t *face[MAX_FACES];

	struct font_s *alt;
	vec3_t tint;
	vec3_t alttint;
} font_t;

union byte_vec4_u
{
	byte_vec4_t rgba;
	quint32_t c;
};

//shared between fonts.
typedef struct {
	texid_t texnum[FONTIMAGES];
	texid_t defaultfont;
	texid_t trackerimage;

	union byte_vec4_u plane[PLANEWIDTH*PLANEHEIGHT];	//tracks the current plane
	FIMAGEIDXTYPE activeplane;
	unsigned short planerowx;
	unsigned short planerowy;
	unsigned short planerowh;
	qboolean planechanged;

	struct charcache_s *oldestchar;
	struct charcache_s *newestchar;
	shader_t *shader;
	shader_t *backshader;
} fontplanes_t;
static fontplanes_t fontplanes;

#define FONT_CHAR_BUFFER 512
static index_t font_indicies[FONT_CHAR_BUFFER*6];
static vecV_t font_coord[FONT_CHAR_BUFFER*4];
static vecV_t font_backcoord[FONT_CHAR_BUFFER*4];
static vec2_t font_texcoord[FONT_CHAR_BUFFER*4];
static union byte_vec4_u font_forecoloura[FONT_CHAR_BUFFER*4];
static union byte_vec4_u font_backcoloura[FONT_CHAR_BUFFER*4];
static mesh_t font_foremesh;
static mesh_t font_backmesh;
static texid_t font_texture;
static int font_colourmask;
static union byte_vec4_u font_forecolour;
static union byte_vec4_u font_backcolour;
static avec4_t	font_foretint;

static struct font_s *curfont;
static float curfont_scale[2];
//static qboolean curfont_scaled;

extern cvar_t r_font_linear;

//^Ue2XX
static struct
{
	image_t *image;
	char name[64];
} trackerimages[256];
static int numtrackerimages;
#define TRACKERFIRST 0xe200
#define TRACKERCOUNT  0x100	//an upper bound. so misused codepoints won't go weird.
int Font_RegisterTrackerImage(const char *image)
{
	int i;
	for (i = 0; i < numtrackerimages; i++)
	{
		if (!strcmp(trackerimages[i].name, image))
			return TRACKERFIRST + i;
	}
	if (numtrackerimages == TRACKERCOUNT)
		return 0;
	trackerimages[i].image = NULL;	//actually load it elsewhere, because we're lazy.
	Q_strncpyz(trackerimages[i].name, image, sizeof(trackerimages[i].name));
	numtrackerimages++;
	return TRACKERFIRST + i;
}
//called from the font display code for tracker images
static image_t *Font_GetTrackerImage(unsigned int imid)
{
	if (!trackerimages[imid].image)
	{
		if (!*trackerimages[imid].name)
			return NULL;
		trackerimages[imid].image = Image_GetTexture(trackerimages[imid].name, NULL, IF_PREMULTIPLYALPHA|IF_UIPIC|IF_NOPURGE, NULL, NULL, 0, 0, TF_INVALID);
	}
	if (!trackerimages[imid].image)
		return NULL;
	if (trackerimages[imid].image->status != TEX_LOADED)
		return NULL;
	return trackerimages[imid].image;
}
qboolean Font_TrackerValid(unsigned int imid)
{
	imid -= TRACKERFIRST;
	if (imid >= countof(trackerimages))
		return false;
	if (!trackerimages[imid].image)
	{
		if (!*trackerimages[imid].name)
			return false;
		trackerimages[imid].image = Image_GetTexture(trackerimages[imid].name, NULL, IF_PREMULTIPLYALPHA|IF_UIPIC|IF_NOPURGE, NULL, NULL, 0, 0, TF_INVALID);
	}
	if (!trackerimages[imid].image)
		return false;
	if (trackerimages[imid].image->status == TEX_LOADING)
		COM_WorkerPartialSync(trackerimages[imid].image, &trackerimages[imid].image->status, TEX_LOADING);
	if (trackerimages[imid].image->status != TEX_LOADED)
		return false;
	return true;
}

//called at load time - initalises font buffers
void Font_Init(void)
{
	int i;
	TEXASSIGN(fontplanes.defaultfont, r_nulltex);

	//clear tracker images, just in case they were still set for the previous renderer context
	for (i = 0; i < sizeof(trackerimages)/sizeof(trackerimages[0]); i++)
		trackerimages[i].image = NULL;

	font_foremesh.indexes = font_indicies;
	font_foremesh.xyz_array = font_coord;
	font_foremesh.st_array = font_texcoord;
	font_foremesh.colors4b_array = &font_forecoloura->rgba;

	font_backmesh.indexes = font_indicies;
	font_backmesh.xyz_array = font_backcoord;
	font_backmesh.st_array = font_texcoord;
	font_backmesh.colors4b_array = &font_backcoloura->rgba;

	for (i = 0; i < FONT_CHAR_BUFFER; i++)
	{
		font_indicies[i*6+0] = i*4+0;
		font_indicies[i*6+1] = i*4+1;
		font_indicies[i*6+2] = i*4+2;
		font_indicies[i*6+3] = i*4+0;
		font_indicies[i*6+4] = i*4+2;
		font_indicies[i*6+5] = i*4+3;
	}

	for (i = 0; i < FONTPLANES; i++)
	{
		TEXASSIGN(fontplanes.texnum[i], Image_CreateTexture("***fontplane***", NULL, IF_UIPIC|(r_font_linear.ival?IF_LINEAR:IF_NEAREST|IF_NOPURGE)|IF_NOPICMIP|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE));
	}

	fontplanes.shader = R_RegisterShader("ftefont", SUF_2D,
		"{\n"
			"fullrate\n"	//don't hurt readability of text.
			"if $nofixed\n"
				"program default2d\n"
			"endif\n"
			"nomipmaps\n"
			"{\n"
				"if r_font_linear\n"
					"map $linear:$diffuse\n"
				"else\n"
					"map $nearest:$diffuse\n"
				"endif\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"blendfunc gl_one gl_one_minus_src_alpha\n"
			"}\n"
		"}\n"
		);

	fontplanes.backshader = R_RegisterShader("ftefontback", SUF_2D,
		"{\n"
			"nomipmaps\n"
			"{\n"
				"map $whiteimage\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"blendfunc gl_one gl_one_minus_src_alpha\n"
			"}\n"
		"}\n"
		);

	font_colourmask = ~0;
}

//flush the font buffer, by drawing it to the screen
static void Font_Flush(void)
{
	R2D_Flush = NULL;
	if (!font_foremesh.numindexes)
		return;
	if (fontplanes.planechanged)
	{
		Image_Upload(fontplanes.texnum[fontplanes.activeplane], TF_RGBA32, (void*)fontplanes.plane, NULL, PLANEWIDTH, PLANEHEIGHT, 1, IF_UIPIC|IF_NEAREST|IF_NOPICMIP|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE);

		fontplanes.planechanged = false;
	}
	font_foremesh.istrifan = (font_foremesh.numvertexes == 4);
	if ((font_colourmask & (CON_RICHFORECOLOUR|CON_NONCLEARBG)) == CON_NONCLEARBG && font_foremesh.numindexes)
	{
		font_backmesh.numindexes = font_foremesh.numindexes;
		font_backmesh.numvertexes = font_foremesh.numvertexes;
		font_backmesh.istrifan = font_foremesh.istrifan;

		BE_DrawMesh_Single(fontplanes.backshader, &font_backmesh, NULL, font_be_flags);
	}
	TEXASSIGN(fontplanes.shader->defaulttextures->base, font_texture);
	BE_DrawMesh_Single(fontplanes.shader, &font_foremesh, NULL, font_be_flags);
	font_foremesh.numindexes = 0;
	font_foremesh.numvertexes = 0;
}

static int Font_BeginChar(texid_t tex)
{
	int fvert;

	if (font_foremesh.numindexes >= FONT_CHAR_BUFFER*6 || font_texture != tex)
	{
		Font_Flush();
		TEXASSIGNF(font_texture, tex);
	}

	fvert = font_foremesh.numvertexes;
	
	font_foremesh.numindexes += 6;
	font_foremesh.numvertexes += 4;

	return fvert;
}

//clear the shared planes and free memory etc
void Font_Shutdown(void)
{
	int i;

	for (i = 0; i < FONTPLANES; i++)
		TEXASSIGN(fontplanes.texnum[i], r_nulltex);
	fontplanes.activeplane = 0;
	fontplanes.oldestchar = NULL;
	fontplanes.newestchar = NULL;
	fontplanes.planechanged = 0;
	fontplanes.planerowx = 0;
	fontplanes.planerowy = 0;
	fontplanes.planerowh = 0;
}

//we got too many chars and switched to a new plane - purge the chars in that plane
void Font_FlushPlane(void)
{
	/*
	assumption:
	oldest chars must be of the oldest plane
	*/

	//we've not broken anything yet, flush while we can
	Font_Flush();

	if (fontplanes.planechanged)
	{
		Image_Upload(fontplanes.texnum[fontplanes.activeplane], TF_RGBA32, (void*)fontplanes.plane, NULL, PLANEWIDTH, PLANEHEIGHT, 1, IF_UIPIC|IF_NEAREST|IF_NOPICMIP|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE);

		fontplanes.planechanged = false;
	}

	fontplanes.activeplane++;
	fontplanes.activeplane = fontplanes.activeplane % FONTPLANES;
	fontplanes.planerowh = 0;
	fontplanes.planerowx = 0;
	fontplanes.planerowy = 0;

	while (fontplanes.oldestchar)
	{
		if (fontplanes.oldestchar->texplane != fontplanes.activeplane)
			break;

		//remove it from the list of active chars, and invalidate it
		fontplanes.oldestchar->texplane = INVALIDPLANE;
		fontplanes.oldestchar = fontplanes.oldestchar->nextchar;
	}
	if (!fontplanes.oldestchar)
		fontplanes.newestchar = NULL;
}

static struct charcache_s *Font_GetCharIfLoaded(font_t *f, unsigned int charidx)
{
	struct charcache_s *c = f->chars[charidx/FONTBLOCKSIZE];
	if (c)
	{
		c += charidx&FONTBLOCKMASK;
		if (c->texplane == INVALIDPLANE)
			c = NULL;
	}
	return c;
}
static struct charcache_s *Font_GetCharStore(font_t *f, unsigned int charidx)
{	//should only be called if generating a char cache
	struct charcache_s *c;
	size_t i;
	c = f->chars[charidx/FONTBLOCKSIZE];
	if (!c)
	{
		c = Z_Malloc(sizeof(struct charcache_s) * FONTBLOCKSIZE);
		f->chars[charidx/FONTBLOCKSIZE] = c;
		for (i = 0; i < FONTBLOCKSIZE; i++)
		{
			c[i].texplane = INVALIDPLANE;
		}
	}
	c += charidx&FONTBLOCKMASK;
	c->block = charidx/FONTBLOCKSIZE;
	return c;
}
static struct charcache_s *Font_CopyChar(font_t *f, unsigned int oldcharidx, unsigned int newcharidx)
{
	struct charcache_s *new, *old = Font_GetCharIfLoaded(f, oldcharidx);
	if (!old)
		return NULL;
	new = Font_GetCharStore(f, newcharidx);
	*new = *old;
	new->block = newcharidx/FONTBLOCKSIZE;
	return new;
}

//loads a new image into a given character slot for the given font.
//note: make sure it doesn't already exist or things will get cyclic
//alphaonly says if its a greyscale image. false means rgba.
static struct charcache_s *Font_LoadGlyphData(font_t *f, CHARIDXTYPE charidx, FT_Pixel_Mode pixelmode, void *data, unsigned int bmw, unsigned int bmh, unsigned int pitch){
	int x, y;
	union byte_vec4_u *out;
	struct charcache_s *c = Font_GetCharStore(f, charidx);
	int pad = 0;
#define BORDERCOLOUR 0

#define MAXOUTLINE 4

	int outline = min(f->outline, MAXOUTLINE);

	if (!bmw || !bmh)
		outline = 0;

	pad+=outline;
	
	if (fontplanes.texnum[0]->flags & IF_LINEAR)
		pad += 1;	//pad the image data to avoid sampling outside

	if (fontplanes.planerowx + (int)bmw+pad*2 >= PLANEWIDTH)
	{
		fontplanes.planerowx = 0;
		fontplanes.planerowy += fontplanes.planerowh;
		fontplanes.planerowh = 0;
	}

	if (fontplanes.planerowy+(int)bmh+pad*2 >= PLANEHEIGHT)
		Font_FlushPlane();

	if (fontplanes.newestchar)
		fontplanes.newestchar->nextchar = c;
	else
		fontplanes.oldestchar = c;
	fontplanes.newestchar = c;
	c->nextchar = NULL;
	c->flags = 0;

	c->texplane = fontplanes.activeplane;
	c->bmx = fontplanes.planerowx+pad;
	c->bmy = fontplanes.planerowy+pad;
	c->bmw = bmw;
	c->bmh = bmh;

	if (fontplanes.planerowh < (int)bmh+pad*2)
		fontplanes.planerowh = bmh+pad*2;
	fontplanes.planerowx += bmw+pad*2;

	out = &fontplanes.plane[c->bmx+((int)c->bmy-pad)*PLANEHEIGHT];
	if (pixelmode == FT_PIXEL_MODE_GRAY)
	{	//8bit font
		for (y = -pad; y < 0; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
		for (; y < bmh; y++)
		{
			for (x = -pad; x < 0; x++)
				out[x].c = BORDERCOLOUR;
			for (; x < bmw; x++)
				out[x].c = 0x01010101 * ((unsigned char*)data)[x];
			for (; x < bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			data = (char*)data + pitch;
			out += PLANEWIDTH;
		}
		for (; y < bmh+pad; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
	}
	else if (pixelmode == FT_PIXEL_MODE_MONO)
	{	//1bit font (
		for (y = -pad; y < 0; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
		for (; y < bmh; y++)
		{
			for (x = -pad; x < 0; x++)
				out[x].c = BORDERCOLOUR;
			for (; x < bmw; x++)
				out[x].c = (((unsigned char*)data)[x>>3]&(1<<(7-(x&7))))?0xffffffff:0;
			for (; x < bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			data = (char*)data + pitch;
			out += PLANEWIDTH;
		}
		for (; y < bmh+pad; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
	}
	else if ((unsigned int)pixelmode == FT_PIXEL_MODE_RGBA_SA)
	{	//rgba source using standard alpha.
		//(we'll multiply out the alpha for the gpu)
		for (y = -pad; y < 0; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
		for (; y < bmh; y++)
		{
			for (x = -pad; x < 0; x++)
				out[x].c = BORDERCOLOUR;
			for (; x < bmw; x++)
			{
				if (((unsigned char*)data)[x*4+3] == 255)
					out[x] = ((union byte_vec4_u*)data)[x];
				else
				{
					out[x].rgba[0] = (((unsigned char*)data)[x*4+3]*((unsigned char*)data)[x*4+0])<<8;
					out[x].rgba[1] = (((unsigned char*)data)[x*4+3]*((unsigned char*)data)[x*4+1])<<8;
					out[x].rgba[2] = (((unsigned char*)data)[x*4+3]*((unsigned char*)data)[x*4+2])<<8;
					out[x].rgba[3] = ((unsigned char*)data)[x*4+3];
				}
			}
			for (; x < bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			data = (char*)data + pitch;
			out += PLANEWIDTH;
		}
		for (; y < (int)bmh+pad; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
	}
	else if (pixelmode == FT_PIXEL_MODE_BGRA)
	{	//bgra srgb font, already premultiplied
		for (y = -pad; y < 0; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
		for (; y < bmh; y++)
		{
			for (x = -pad; x < 0; x++)
				out[x].c = BORDERCOLOUR;
			for (; x < bmw; x++)
			{
				out[x].rgba[0] = ((unsigned char*)data)[x*4+2];
				out[x].rgba[1] = ((unsigned char*)data)[x*4+1];
				out[x].rgba[2] = ((unsigned char*)data)[x*4+0];
				out[x].rgba[3] = ((unsigned char*)data)[x*4+3];
			}
			for (; x < bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			data = (char*)data + pitch;
			out += PLANEWIDTH;
		}
		for (; y < bmh+pad; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}

		c->flags = CHARF_FORCEWHITE;	//private glyph colours
	}
	else if ((unsigned int)pixelmode == FT_PIXEL_MODE_RGBA)
	{	//bgra srgb font, already premultiplied
		for (y = -pad; y < 0; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}
		for (; y < bmh; y++)
		{
			for (x = -pad; x < 0; x++)
				out[x].c = BORDERCOLOUR;
			for (; x < bmw; x++)
				out[x].c = ((unsigned int*)data)[x];
			for (; x < bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			data = (char*)data + pitch;
			out += PLANEWIDTH;
		}
		for (; y < bmh+pad; y++)
		{
			for (x = -pad; x < (int)bmw+pad; x++)
				out[x].c = BORDERCOLOUR;
			out += PLANEWIDTH;
		}

		c->flags = CHARF_FORCEWHITE;	//private glyph colours
	}

	if (outline)
	{
		static int filter_outline;
		static unsigned char filter_highest[MAXOUTLINE*2+1][MAXOUTLINE*2+1];
		if (outline != filter_outline)
		{
			filter_outline = outline;
			for (y = -outline; y <= outline; y++)
				for (x = -outline; x <= outline; x++)
					filter_highest[outline+y][outline+x] = bound(0, (outline + 1 - sqrt(x*x + y*y))*255+.5, 255);
		}

		//expand it to out full(ish) size

		if (pixelmode == FT_PIXEL_MODE_MONO)
		{
			qbyte *alpha = (char*)data - pitch*bmh;
			qbyte a;
			int bit;

			alpha -= pitch*outline;
			out = &fontplanes.plane[c->bmx+((int)c->bmy-outline)*PLANEHEIGHT];
			for (y = -outline; y < (int)bmh+outline; y++, out += PLANEWIDTH)
				for (x = -outline; x < (int)bmw+outline; x++)
				{
					int xn, x1 = max(outline-x, 0), x2 = min(2*outline, (int)bmw-1-x+outline);
					int yn, y1 = max(outline-y, 0), y2 = min(2*outline, (int)bmh-1-y+outline);
					int v, m = out[x].rgba[3]*255;
					for (yn = y1; yn <= y2; yn++)
						for (xn = x1; xn <= x2; xn++)
						{
							bit = (xn+x)-outline;
							a = alpha[(bit>>3)+(yn+y)*pitch];
							a = (a&(1<<(7-(bit&7))))?0xff:0;

							v = filter_highest[yn][xn] * a;
							m = max(m, v);
						}
					//out[x].c = 0;
					out[x].rgba[3] = m/255;
				}
		}
		else
		{
			int bytes = (pixelmode == FT_PIXEL_MODE_GRAY)?1:4;
			qbyte *alpha = (char*)data + bytes-1 - pitch*bmh;

			alpha -= pitch*outline + bytes*outline;
			out = &fontplanes.plane[c->bmx+((int)c->bmy-outline)*PLANEHEIGHT];
			for (y = -outline; y < (int)bmh+outline; y++, out += PLANEWIDTH)
				for (x = -outline; x < (int)bmw+outline; x++)
				{
					int xn, x1 = max(outline-x, 0), x2 = min(2*outline, (int)bmw-1-x+outline);
					int yn, y1 = max(outline-y, 0), y2 = min(2*outline, (int)bmh-1-y+outline);
					int v, m = out[x].rgba[3]*255;
					for (yn = y1; yn <= y2; yn++)
						for (xn = x1; xn <= x2; xn++)
						{
							v = filter_highest[yn][xn] * alpha[(xn+x)*bytes+(yn+y)*pitch];
							m = max(m, v);
						}
					//out[x].c = 0;
					out[x].rgba[3] = m/255;
				}
		}


		c->bmx -= outline;
		c->bmy -= outline;
		c->bmw += outline*2;
		c->bmh += outline*2;
	}

	fontplanes.planechanged = true;
	return c;
}

unsigned short hex[16] = {
/*0*/	(7<<0)|(5<<3)|(5<<6)|(5<<9)|(7<<12),
/*1*/	(2<<0)|(2<<3)|(2<<6)|(2<<9)|(2<<12),
/*2*/	(7<<0)|(1<<3)|(7<<6)|(4<<9)|(7<<12),
/*3*/	(7<<0)|(1<<3)|(3<<6)|(1<<9)|(7<<12),
/*4*/	(5<<0)|(5<<3)|(7<<6)|(1<<9)|(1<<12),
/*5*/	(7<<0)|(4<<3)|(7<<6)|(1<<9)|(7<<12),
/*6*/	(4<<0)|(4<<3)|(7<<6)|(5<<9)|(7<<12),
/*7*/	(7<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12),
/*8*/	(7<<0)|(5<<3)|(7<<6)|(5<<9)|(7<<12),
/*9*/	(7<<0)|(5<<3)|(7<<6)|(1<<9)|(1<<12),
/*A*/	(7<<0)|(5<<3)|(7<<6)|(5<<9)|(5<<12),
/*B*/	(6<<0)|(5<<3)|(7<<6)|(5<<9)|(6<<12),
/*C*/	(7<<0)|(5<<3)|(4<<6)|(5<<9)|(7<<12),
/*D*/	(6<<0)|(5<<3)|(5<<6)|(5<<9)|(6<<12),
/*E*/	(7<<0)|(4<<3)|(6<<6)|(4<<9)|(7<<12),
/*F*/	(7<<0)|(4<<3)|(6<<6)|(4<<9)|(4<<12)
};
static struct charcache_s *Font_LoadPlaceholderGlyph(font_t *f, CHARIDXTYPE charidx)
{
	struct charcache_s *c;
	unsigned int out[169*4], i, o, g, b, w, h, d, n;
	if (charidx == 0xe080 || charidx == 0xe081 || charidx == 0xe082 || charidx == 0xe083)
	{	//scrollbar glyps
		w = min(16,f->charheight);
		h = max(1,w/8);
		d = 0;

		if (charidx == 0xe083)	//centre
		{
			h *= 3;
			memset(out+w*h*0, 0xff, w*4*h);
		}
		else
		{
			memset(out+w*h*0, 0xff, w*4*h);
			memset(out+w*h*1, 0x00, w*4*h);
			memset(out+w*h*2, 0xff, w*4*h);
			for (i = 0; i < h; i++)
			{
				if (charidx == 0xe080)	//left
					memset(out+w*(i+h*1), 0xff, h*4);
				if (charidx == 0xe082)	//right
					memset(out+w*(i+h*1)+w-h, 0xff, h*4);
			}
			h *= 3;
		}
	}
	else if (charidx == 0xe01d || charidx == 0xe01e || charidx == 0xe01f)
	{	//horizontal separator
		w = min(16,f->charheight);
		h = max(1,w/8);
		d = 0;
		memset(out, 0xff, w*4*h);
	}
	else if (charidx == 0xe00b)
	{	//console input cursor
		h = min(16,f->charheight);
		w = max(1,h/8);
		d = 1;
		memset(out, 0xff, w*4*h);
	}
	else if (charidx == 0xe00d)
	{	//filled > arrow indicator (used by the menus)
		h = min(16,f->charheight);
		w = (h+1)/2;
		d = 1;
		memset(out, 0xff, w*4*h);
		for (i = 0; i < w; i++)
		{
			memset(out + w*i + (i+1), 0x0, (w-i-1)*4);
			memset(out + w*(h-i-1) + (i+1), 0x0, (w-i-1)*4);
		}
	}
	else
	{
		d = (f->charheight >= 11);
		if (d)
		{	//two rows
			h = 11;
			if (charidx > 0xffff)
				n = 3;
			else if (charidx > 0xff)
				n = 2;
			else
				n = 1;
			w = n*4-1;
			n*=2;
		}
		else
		{	//single row. bye bye fixed-width.
			if (charidx > 0xfffff)
				n = 6;
			else if (charidx > 0xffff)
				n = 5;
			else if (charidx > 0xfff)
				n = 4;
			else if (charidx > 0xff)
				n = 3;
			else
				n = 2;
			w = n*4-1;
			h = 5;
		}

		//figure out if we can get away with giving it a little more border to boost readability
		b = (h+2 < f->charheight);
		w += b*2;
		h += b*2;

		memset(out, 0xff, sizeof(out));

		for (i = 0; i < n; i++)
		{
			g = hex[0xf & (charidx>>(i<<2))];
			o = b + w*b;
			if (d)
			{	//stradle them over the two rows.
				if (i >= (n>>1))
					o += 4*(n-i-1);
				else
				{
					o += 4*((n>>1)-i-1);
					o += w * 6;
				}
			}
			else
				o += 4*(n-i-1);	//just arrange them in order
			for (; g; g>>=3, o+=w)
			{
				if (g & 4)	out[o+0] = 0xff0000ff;
				if (g & 2)	out[o+1] = 0xff0000ff;
				if (g & 1)	out[o+2] = 0xff0000ff;
			}
		}

		d = 1;
	}
	c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, out, w, h, w*4);
	if (c)
	{
		c->advance = w+d;
		c->left = 0;
		c->top = (f->charheight-h-1)/2;
	}
	return c;
}

static struct charcache_s *Font_TryLoadGlyphRaster(font_t *f, fontface_t *qface, CHARIDXTYPE charidx)
{
	size_t maxchar = 256;
	const unsigned short *c1tab;
	size_t c1tabsize;
	size_t glyph = charidx;

	safeswitch(qface->horiz.codepage)
	{
	safedefault:
	case FMT_AUTO:		//shouldn't happen.
	case FMT_ISO88591:	//all identity.
	case FMT_HORIZONTAL: //erk...
		c1tab = NULL;
		c1tabsize = 0;
		break;
	case FMT_WINDOWS1252:
		{
			static const unsigned short win1252[] = {
				0x20ac,  0x81,0x201a,0x0192, 0x201e,0x2026,0x2020,0x2021, 0x02c6,0x2030,0x0160,0x2039, 0x0152,  0x8d,0x017d,  0x8f,
				  0x90,0x2018,0x2019,0x101c, 0x201d,0x2022,0x2013,0x2014, 0x02dc,0x2122,0x0161,0x203a, 0x0153,  0x9d,0x017e,0x0178};
			c1tab = win1252;
			c1tabsize = sizeof(win1252);
		}
		break;
	case FMT_KOI8U:
		{
			static const unsigned short koi8u[] = {
				0x2500,0x2502,0x250C,0x2510, 0x2514,0x2518,0x251C,0x2524, 0x252C,0x2534,0x253C,0x2580, 0x2584,0x2588,0x258C,0x2590,
				0x2591,0x2592,0x2593,0x2320, 0x25A0,0x2219,0x221A,0x2248, 0x2264,0x2265,0x00A0,0x2321, 0x00B0,0x00B2,0x00B7,0x00F7,
				0x2550,0x2551,0x2552,0x0451, 0x0454,0x2554,0x0456,0x0457, 0x2557,0x2558,0x2559,0x255A, 0x255B,0x0491,0x255D,0x255E,
				0x255F,0x2560,0x2561,0x0401, 0x0404,0x2563,0x0406,0x0407, 0x2566,0x2567,0x2568,0x2569, 0x256A,0x0490,0x256C,0x00A9,
				0x044E,0x0430,0x0431,0x0446, 0x0434,0x0435,0x0444,0x0433, 0x0445,0x0438,0x0439,0x043A, 0x043B,0x043C,0x043D,0x043E,
				0x043F,0x044F,0x0440,0x0441, 0x0442,0x0443,0x0436,0x0432, 0x044C,0x044B,0x0437,0x0448, 0x044D,0x0449,0x0447,0x044A,
				0x042E,0x0410,0x0411,0x0426, 0x0414,0x0415,0x0424,0x0413, 0x0425,0x0418,0x0419,0x041A, 0x041B,0x041C,0x041D,0x041E,
				0x041F,0x042F,0x0420,0x0421, 0x0422,0x0423,0x0416,0x0412, 0x042C,0x042B,0x0417,0x0428, 0x042D,0x0429,0x0427,0x042A};
			c1tab = koi8u;
			c1tabsize = sizeof(koi8u);
		}
		break;
	case FMT_QUAKE:
		{
			c1tab = NULL;
			c1tabsize = 0;
			if (glyph >= 0xe000 && glyph < 0xe100)
				glyph -= 0xe000;
			else if (glyph < 32 || glyph >= 0x80)
				return NULL;
		}
		break;
	}
	if (glyph >= maxchar)
		return NULL;
	if (glyph >= 0x80 && glyph < 0x80+c1tabsize)
	{
		int i;
		for (i = 0; ; i++)
		{
			if (i == c1tabsize)
				return NULL;
			if (glyph == c1tab[i])
			{
				glyph = 0x80+i;
				break;
			}
		}
	}

	{
		int gw = (qface->horiz.stride)/(maxchar/qface->horiz.rows);
		int gr = glyph / (maxchar/qface->horiz.rows);
		int gc = glyph - (gr*(maxchar/qface->horiz.rows));
		int gs = qface->horiz.stride;
		int gh = qface->horiz.charheight;
		struct charcache_s *c;
		if (qface->horiz.paletted)
		{
			qbyte *in = (qbyte*)qface->horiz.data + gc*gw + gr*gs*gh;
			/*while (gw >= 1)
			{
				int y;
				gw--;	//see if we can strip this column
				for (y = 0; y < gh; y++)
					if (in[gw+y*gs])
						break;
				if (y < gh)
				{
					gw++;
					break;
				}
			}*/
			{
				int ngw = (gw * f->charheight) / gh;
				int ngh = f->charheight;
				int x, y;
				unsigned int *out2 = alloca(ngw*ngh*4);
				if (ngw&&ngh)
				{
					unsigned int *out1 = alloca(gw*gh*4);
					for (y = 0; y < gh; y++)
						for (x = 0; x < gw; x++)
							out1[x+y*gw] = in[x+y*gs]?d_8to24rgbtable[in[x+y*gs]]:0;
					Image_ResampleTexture(PTI_RGBA8, out1, gw, gh, out2, ngw, ngh);
				}
				c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, out2, ngw, ngh, ngw*4);
				gw = ngw;
			}
		}
		else
		{
			unsigned int *in = (unsigned int*)qface->horiz.data + gc*gw + gr*gs*gh;
			while (gw >= 1)
			{
				int y;
				gw--;	//see if we can strip this column
				for (y = 0; y < gh; y++)
					if (in[gw+y*gs] & 0x00ffffff)
						break;
				if (y < gh)
				{
					gw++;
					break;
				}
			}
			if (f->charheight != gh)
			{
				int ngw = (gw * f->charheight) / gh;
				int ngh = f->charheight;
				int x, y;
				unsigned int *out2 = alloca(ngw*ngh*4);
				if (ngw&&ngh)
				{	//we need to repack the input, because Image_ResampleTexture can't handle strides
					unsigned int *out1 = alloca(gw*gh*4);
					for (y = 0; y < gh; y++)
						for (x = 0; x < gw; x++)
							out1[x+y*gw] = in[x+y*gs];
					Image_ResampleTexture(PTI_RGBA8, out1, gw, gh, out2, ngw, ngh);
				}
				c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, out2, ngw, ngh, ngw*4);
				gw = ngw;
			}
			else
				c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, in, gw, gh, gs*4);
		}
		if (!gw)	//for invisble glyphs (eg: space), we attempt to ensure that there's some substance there. missing spaces is weird.
			gw = gh/3;
		if (c)
		{
			c->advance = gw;
			c->left = 0;
			c->top = 0;
			c->flags &= ~CHARF_FORCEWHITE;
			return c;
		}
	}
	return NULL;
}

//loads the given charidx for the given font, importing from elsewhere if needed.
static struct charcache_s *Font_TryLoadGlyph(font_t *f, CHARIDXTYPE charidx)
{
	struct charcache_s *c;

#if GEN_CONCHAR_GLYPHS != 0
	if (charidx >= 0xe000 && charidx <= 0xe0ff)
	{
		int cpos = charidx & 0xff;
		unsigned int img[64*64], *d;
		unsigned char *s;
		int scale;
		int x,y, ys;
		qbyte *draw_chars = W_GetLumpName("conchars");
		if (draw_chars)
		{
			d = img;
			s = draw_chars + 8*(cpos&15)+128*8*(cpos/16);

			scale = f->charheight/8;
			if (scale < 1)
				scale = 1;
			if (scale > 64/8)
				scale = 64/8;
			
			for (y = 0; y < 8; y++)
			{
				for (ys = 0; ys < scale; ys++)
				{
					for (x = 0; x < 8*scale; x++)
						d[x] = d_8to24rgbtable[s[x/scale]];
					d+=8*scale;
				}
				s+=128;
			}
			c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA_SA, img, 8*scale, 8*scale, 8*scale*4);
			if (c)
			{
				c->advance = 8*scale;
				c->left = 0;
				c->top = 7*scale;
			}
			return c;
		}
		if ((charidx&0x7f) > 0x20)
			charidx &= 0x7f;
	}
#endif

#ifdef QUAKEHUD
	if (charidx >= 0xe100 && charidx <= 0xe1ff)
	{
		qpic_t *wadimg;
		qbyte lumptype = 0;
		unsigned char *src;
		unsigned int img[64*64];
		int nw, nh;
		int x, y;
		unsigned int stepx, stepy;
		unsigned int srcx, srcy;
		size_t lumpsize = 0;

		if (charidx-0xe100 >= sizeof(imgs)/sizeof(imgs[0]))
			wadimg = NULL;
		else
			wadimg = W_GetLumpName(imgs[charidx-0xe100], &lumpsize, &lumptype);
		if (wadimg && lumptype == TYP_QPIC && lumpsize == 8+wadimg->height*wadimg->width)
		{
			nh = wadimg->height;
			nw = wadimg->width;
			while (nh < f->charheight)
			{
				nh *= 2;
				nw *= 2;
			}
			if (nh > f->charheight)
			{
				nw = (nw * f->charheight)/nh;
				nh = f->charheight;
			}
			stepy = 0x10000*((float)wadimg->height/nh);
			stepx = 0x10000*((float)wadimg->width/nw);
			if (nh > 64)
				nh = 64;
			if (nw > 64)
				nw = 64;
			srcy = 0;
			for (y = 0; y < nh; y++)
			{
				src = (unsigned char *)(wadimg->data);
				src += wadimg->width * (srcy>>16);
				srcy += stepy;
				srcx = 0;
				for (x = 0; x < nw; x++)
				{
					img[x+y*64] = d_8to24rgbtable[src[srcx>>16]];
					srcx += stepx;
				}
			}

			c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA_SA, img, nw, nh, 64*4);
			if (c)
			{
				c->flags = CHARF_FORCEWHITE;	//private glyph colours
				c->left = 0;
				c->top = f->charheight - nh;
				c->advance = nw;
				return c;
			}
		}
	}
#endif
	/*make tab invisible*/
	if (charidx == '\t' || charidx == '\n')
	{
		c = &f->chars[charidx/FONTBLOCKSIZE][charidx&FONTBLOCKMASK];
		c->left = 0;
		c->advance = f->charheight;
		c->top = 0;

		c->texplane = 0;
		c->bmx = 0;
		c->bmy = 0;
		c->bmw = 0;
		c->bmh = 0;
		return c;
	}

	if (f->faces)
	{
		int file;
		for (file = 0; file < f->faces; file++)
		{
			fontface_t *qface = f->face[file];
#ifdef AVAIL_FREETYPE
			if (qface->ft.face)
			{
				FT_Face face = qface->ft.face;

				if (qface->ft.activeheight != f->charheight)
				{
					if (!Font_ChangeFTSize(qface, f->charheight))
						return NULL;	//some sort of error.
				}
				if (charidx == 0xfffe || pFT_Get_Char_Index(face, charidx))	//ignore glyph 0 (undefined)
					if (pFT_Load_Char(face, charidx, FT_LOAD_RENDER|(((f->flags&FONT_MONO)&&qface->ft.activeheight==qface->ft.actualsize/*FIXME*/)?FT_LOAD_TARGET_MONO:FT_LOAD_TARGET_NORMAL)|FT_LOAD_COLOR) == 0)
					{
						FT_GlyphSlot slot;
						FT_Bitmap *bm;

						slot = face->glyph;
						bm = &slot->bitmap;
						if (qface->ft.activeheight!=qface->ft.actualsize)
						{
							//I'm just going to assume full-height raster glyphs here. I'm sure I'll be proven wrong some time but w/e.
							int nw, nh;
							if (bm->rows)
							{
								nh = f->charheight;
								nw = (bm->width*nh)/bm->rows;
							}
							else
								nw = f->charheight, nh = 0;
							if (!nw || !nh)
								c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_GRAY, NULL, nw, nh, 0);
							else if (bm->pixel_mode == FT_PIXEL_MODE_BGRA)
							{
								unsigned int *out = alloca(nw*nh*sizeof(*out));
								Image_ResampleTexture(PTI_BGRA8, (void*)bm->buffer, bm->width, bm->rows, out, nw, nh);
								c = Font_LoadGlyphData(f, charidx, bm->pixel_mode, out, nw, nh, nw*sizeof(*out));
							}
							else if (bm->pixel_mode == FT_PIXEL_MODE_GRAY)
							{
								unsigned char *out = alloca(nw*nh*sizeof(*out));
								Image_ResampleTexture(PTI_L8, (void*)bm->buffer, bm->width, bm->rows, out, nw, nh);
								c = Font_LoadGlyphData(f, charidx, bm->pixel_mode, out, nw, nh, nw*sizeof(*out));
							}
							/*else if (bm->pixel_mode == FT_PIXEL_MODE_MONO)
							{
								unsigned char *out = alloca(nw*nh*sizeof(*out));
								Image_ResampleTexture(PTI_L1, (void*)bm->buffer, bm->width, bm->rows, out, nw, nh);
								c = Font_LoadGlyphData(f, charidx, bm->pixel_mode, out, nw, nh, nw*sizeof(*out));
							}*/
							else
								c = NULL;
							if (c)
							{
								c->advance = nw;
								c->left = 0;
								c->top = 0;
								return c;
							}
						}
						else
						{
							c = Font_LoadGlyphData(f, charidx, bm->pixel_mode, bm->buffer, bm->width, bm->rows, bm->pitch);

							if (c)
							{
								c->advance = slot->advance.x >> 6;
								c->left = slot->bitmap_left;
								/*if(1)
									c->top = f->truecharheight - slot->bitmap_top;
								else*/
									c->top = f->charheight*3/4 - slot->bitmap_top;
								return c;
							}
						}
					}
			}
			else
#endif
#ifdef HALFLIFEMODELS
			if (qface->halflife)
			{
				size_t glyph = charidx;
				if (glyph > 0xe000)
					glyph -= 0xe000;
				if (glyph < 0x100)
				{
					int gw = qface->halflife->chartab[glyph].width;
					int gh = qface->halflife->fontheight1;
					qbyte *in = qface->halflife->data + 256 * qface->halflife->chartab[glyph].y + qface->halflife->chartab[glyph].x;
					qbyte *pal = qface->halflife->data + 256 * qface->halflife->imgheight+2;
					qbyte *out = alloca(gw*gh*4);
					int x, y;
					for (y = 0; y < gh; y++, in += 256-gw)
						for (x = 0; x < gw; x++, in++)
						{
							if (*in==0xff)
							{
								out[(x+y*gw)*4+0] = 0;
								out[(x+y*gw)*4+1] = 0;
								out[(x+y*gw)*4+2] = 0;
								out[(x+y*gw)*4+3] = 0;
							}
							else
							{
								qbyte val;
								val = max(pal[*in*3+0], pal[*in*3+1]);
								val = max(val, pal[*in*3+2]);
								out[(x+y*gw)*4+0] =
								out[(x+y*gw)*4+1] =
								out[(x+y*gw)*4+2] = val;
								out[(x+y*gw)*4+3] = 0xff;
							}
						}

					if (f->charheight != gh)
					{
						int ngw = (gw * f->charheight) / gh;
						int ngh = f->charheight;
						qbyte *out2 = alloca(ngw*ngh*4);
						if (ngw&&ngh)
							Image_ResampleTexture(PTI_RGBA8, out, gw, gh, out2, ngw, ngh);
						c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, out2, ngw, ngh, ngw*4); 
						gw = ngw;
					}
					else
						c = Font_LoadGlyphData(f, charidx, FT_PIXEL_MODE_RGBA, out, gw, gh, gw*4); 
					if (c)
					{
						c->flags = 0;	//private glyph colours
						c->advance = gw;
						c->left = 0;
						c->top = 0;
						return c;
					}
				}
			}
			else
#endif
			if (qface->horiz.data)
			{
				c = Font_TryLoadGlyphRaster(f, qface, charidx);
				if (c)
					return c;
			}
		}
	}

	if (charidx == '\r')
		return Font_CopyChar(f, charidx|0xe000, charidx);

	return NULL;
}

//obtains a cached char, null if not cached
static struct charcache_s *Font_GetChar(font_t *f, unsigned int codepoint)
{
	CHARIDXTYPE charidx;
	struct charcache_s *c;
	if (codepoint > FONT_MAXCHARS)
		charidx = 0xfffd;
	else
		charidx = codepoint;
	c = Font_GetCharIfLoaded(f, charidx);
	if (!c)
	{
		if (charidx >= TRACKERFIRST && charidx < TRACKERFIRST+TRACKERCOUNT)
		{
			static struct charcache_s tc;
			tc.texplane = TRACKERIMAGE;
			fontplanes.trackerimage = Font_GetTrackerImage(charidx-TRACKERFIRST);
			if (!fontplanes.trackerimage)
				return Font_GetChar(f, '?');
			tc.advance = fontplanes.trackerimage->width * ((float)f->charheight / fontplanes.trackerimage->height);
			return &tc;
		}

		if (charidx == 0x00a0)	//nbsp
		{
			c = Font_GetCharIfLoaded(f, ' ');
			if (c)
				return c;
		}

		//not cached, can't get.
		c = Font_TryLoadGlyph(f, charidx);

		if (!c && charidx >= 0x400 && charidx <= 0x45f)
		{	//apparently there's a lot of russian players out there.
			//if we just replace all their chars with a '?', they're gonna get pissed.
			//so lets at least attempt to provide some default mapping that makes sense even if they don't have a full font.
			//koi8-u is a mapping useful with 7-bit email because the message is still vaugely readable in latin if the high bits get truncated.
			//not being a language specialist, I'm just going to use that mapping, with the high bit truncated to ascii (which mostly exists in the quake charset).
			//this exact table is from ezquake. because I'm too lazy to figure out the proper mapping. (beware of triglyphs)
			static char *wc2koi_table =
				"?3??4?67??" "??" "??" ">?"	//400
				"abwgdevzijklmnop"	//410
				"rstufhc~{}/yx|`q"	//420
				"ABWGDEVZIJKLMNOP"	//430
				"RSTUFHC^[]_YX\\@Q"	//440
				"?#??$?&'??" "??" "??.?";	//450
			charidx = wc2koi_table[charidx - 0x400];
			if (charidx != '?')
			{
				c = Font_GetCharIfLoaded(f, charidx);
				if (!c)
					c = Font_TryLoadGlyph(f, charidx);
			}
		}

		if (!c && charidx >= 0xA0 && charidx <= 0x17F)
		{	//try to make sense of iso8859-1
			//(mostly for zerstorer's o-umlout...)
			static char *latin_table =
				" ?c###|S?c?<??R?"	//A0
				"??""??""'u?.,??"">??""??"	//B0
				"AAAAAAECEEEEIIII"	//C0
				"DNOOOOO*OUUUUYYs"	//D0
				"aaaaaaeceeeeiiii"	//E0
				"onooooo/ouuuuyyy"	//F0

				"AaAaAaCcCcCcCcDd"	//100
				"DdEeEeEeEeEeGgGg"	//110
				"GgGgHhHhIiIiIiIi"	//120
				"IiIiJjKkkLlLlLlL"	//130
				"lllNnNnNnnNnOoOo"	//140
				"OoEeRrRrRrSsSsSs"	//150
				"SsTtTtTtUuUuUuUu"	//160
				"UuUuWwYyYZzZzZzf";	//170
			charidx = latin_table[charidx - 0xA0];
			if (charidx != '?')
			{
				c = Font_GetCharIfLoaded(f, charidx);
				if (!c)
					c = Font_TryLoadGlyph(f, charidx);
			}
		}

		if (!c)
			c = Font_LoadPlaceholderGlyph(f, charidx);
		if (!c)
		{
			charidx = 0xfffd;	//unicode's replacement char
			c = Font_GetCharIfLoaded(f, charidx);
			if (!c)
				c = Font_TryLoadGlyph(f, charidx);
		}
		if (!c)
		{
			charidx = '?';	//meh
			c = Font_GetCharIfLoaded(f, charidx);
			if (!c)
				c = Font_TryLoadGlyph(f, charidx);
		}
	}
	return c;
}

qboolean Font_LoadHorizontalFont(struct font_s *f, int fheight, const char *fontfilename)
{	//halflife-style.
	fontface_t *qface;
	void *rawdata;
	qofs_t rawsize;
	qbyte *rgbadata = NULL;
	int width=0,height=0;
	uploadfmt_t format;

	if (fheight < 1)
		fheight = 1;

	//ran out of font slots.
	if (f->faces == MAX_FACES)
		return false;

	for (qface = faces; qface; qface = qface->fnext)
	{
		if (!strcmp(qface->name, fontfilename) && qface->horiz.data)
		{
			qface->refs++;
			f->face[f->faces++] = qface;
			return true;
		}
	}

	rawdata = FS_MallocFile(fontfilename, FS_GAME, &rawsize);
	if (rawdata)
		rgbadata = ReadRawImageFile(rawdata, rawsize, &width, &height, &format, true, fontfilename);
	FS_FreeFile(rawdata);

	if (rgbadata)
	{
		/*success!*/
		qface = Z_Malloc(sizeof(*qface));
		qface->flink = &faces;
		qface->fnext = *qface->flink;
		*qface->flink = qface;
		if (qface->fnext)
			qface->fnext->flink = &qface->fnext;
		qface->horiz.data = rgbadata;
		qface->horiz.stride = width;
		qface->horiz.charheight = height;
		qface->horiz.rows = 1;
		qface->horiz.paletted = false;
		qface->horiz.codepage = FMT_ISO88591;
		qface->refs++;
		Q_strncpyz(qface->name, fontfilename, sizeof(qface->name));

		f->face[f->faces++] = qface;
		return true;
	}
	return false;
}

qboolean Font_LoadKexFont(struct font_s *f, int fheight, const char *fontfilename)
{
	vfsfile_t *kfont = FS_OpenVFS(va("fonts/%s.kfont", fontfilename), "rb", FS_GAME);
	char line[256];
	char val[256], *s;
	struct charcache_s *c;
	fontface_t *qface;
	if (!kfont)
		return false;
	while(VFS_GETS(kfont, line, sizeof(line)))
	{
		s = COM_ParseOut(line, val, sizeof(val));
		if (!s)
			continue;
		if (!strcmp(val, "texture") && (s=COM_ParseOut(s, val, sizeof(val))))
		{
			TEXDOWAIT(f->singletexture);
			if (!TEXLOADED(f->singletexture))
			{	//noworker is needed because we need to know the size to make sense of char positions
				//exactextension is needed to work around quakeex fuckups
				f->singletexture = Image_GetTexture(val, NULL, IF_NOWORKER|IF_EXACTEXTENSION|IF_UIPIC|(r_font_linear.ival?IF_LINEAR:IF_NEAREST)|IF_NOPICMIP|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE, NULL, NULL, 0, 0, PTI_INVALID);
			}
		}
		else if (*val >= '0' && *val <='9')
		{
			unsigned int codepoint = atoi(val);
			unsigned int x, y, w, h;//, u;
			if (!TEXLOADED(f->singletexture))
				break;

			s=COM_ParseOut(s, val, sizeof(val));
			x = atoi(val);
			s=COM_ParseOut(s, val, sizeof(val));
			y = atoi(val);
			s=COM_ParseOut(s, val, sizeof(val));
			w = atoi(val);
			s=COM_ParseOut(s, val, sizeof(val));
			h = atoi(val);
			s=COM_ParseOut(s, val, sizeof(val));
			//u = atoi(val);
			if (!s)
				continue;	//something truncated.
			if (codepoint >= FONT_MAXCHARS || h<=0)
				continue;	//out of range.
			c = Font_GetCharStore(f, codepoint);
			if (codepoint != ' ')
			{
				y+=2;
				h-=4;
			}
			c->advance = max(1,(f->charheight * w)/h);

			c->bmw = (w * PLANEWIDTH) / f->singletexture->width;
			c->bmh = (h * PLANEHEIGHT) / f->singletexture->height;
			c->bmx = (x * PLANEWIDTH) / f->singletexture->width;
			c->bmy = (y * PLANEHEIGHT) / f->singletexture->height;
			c->left = 0;
			c->nextchar = 0;	//these chars are not linked in
			c->texplane = BITMAPPLANE;
		}
	}


	VFS_CLOSE(kfont);
	if (!TEXLOADED(f->singletexture))
		return false;

	/*success!*/
	qface = Z_Malloc(sizeof(*qface));
	qface->flink = &faces;
	qface->fnext = *qface->flink;
	*qface->flink = qface;
	if (qface->fnext)
		qface->fnext->flink = &qface->fnext;
	qface->refs++;
	Q_snprintfz(qface->name, sizeof(qface->name), "fonts/%s.kfont", fontfilename);

	f->face[f->faces++] = qface;

	return true;
}

#ifdef AVAIL_FREETYPE
#if defined(LIBFONTCONFIG_STATIC)
#include <fontconfig/fontconfig.h>
#endif
extern cvar_t dpcompat_smallerfonts;
int Font_ChangeFTSize(fontface_t *qface, int pixelheight)
{
	FT_Face face = qface->ft.face;
	qface->ft.activeheight = pixelheight;
#ifdef HAVE_LEGACY
	if (dpcompat_smallerfonts.ival)
	{	//sizes specified include extra spacing in dp, giving extra padding somewhere.
		int s = pixelheight;
		for(s = pixelheight; s; s--)
		{
			if (0==pFT_Set_Pixel_Sizes(face, 0, s))
				if (face->size->metrics.height>>6 <= pixelheight)
					break;
		}

		if (!s)
		{
			qface->ft.activeheight = 0; //something invalid
			qface->ft.actualsize = qface->ft.activeheight;
			return 0;
		}
		qface->ft.actualsize = qface->ft.activeheight;
		return s;
	}
	else
#endif
		if (FT_HAS_FIXED_SIZES(face) && !FT_IS_SCALABLE(face))
	{	//freetype doesn't like scaling these for us, so we have to pick a usable size ourselves.
		FT_Int best = 0, s;
		int bestheight = 0, h;
		for (s = 0; s < qface->ft.face->num_fixed_sizes; s++)
		{
			h = qface->ft.face->available_sizes[s].height;
			//always try to pick the smallest size that is also >= our target size
			if ((h > bestheight && bestheight < pixelheight) || (h >= pixelheight && h < bestheight))
			{
				bestheight = h;
				best = s;
			}
		}
		qface->ft.actualsize = qface->ft.face->available_sizes[best].height;
		pFT_Select_Size(face, best);
	}
	else
	{
		pFT_Set_Pixel_Sizes(face, 0, pixelheight);
		qface->ft.actualsize = pixelheight;
	}
	return pixelheight;
}
qboolean Font_LoadFreeTypeFont(struct font_s *f, int height, const char *fontfilename, const char *styles)
{
	fontface_t *qface;
	FT_Face face = NULL;
	FT_Error error;
	flocation_t loc;
	void *fbase = NULL;
	if (!*fontfilename)
		return false;

	if (height < 1)
		height = 1;

	//ran out of font slots.
	if (f->faces == MAX_FACES)
		return false;

	for (qface = faces; qface; qface = qface->fnext)
	{
		if (!strcmp(qface->name, fontfilename) && qface->ft.face)
		{
			qface->refs++;
			if (!f->faces)
				f->truecharheight = Font_ChangeFTSize(qface, f->charheight);
			f->face[f->faces++] = qface;
			return true;
		}
	}

	if (!fontlib)
	{
#ifndef FREETYPE_STATIC
		dllfunction_t ft2funcs[] =
		{
			{(void**)&pFT_Init_FreeType, "FT_Init_FreeType"},
			{(void**)&pFT_Load_Char, "FT_Load_Char"},
			{(void**)&pFT_Get_Char_Index, "FT_Get_Char_Index"},
			{(void**)&pFT_Set_Pixel_Sizes, "FT_Set_Pixel_Sizes"},
			{(void**)&pFT_Select_Size, "FT_Select_Size"},
			{(void**)&pFT_New_Face, "FT_New_Face"},
			{(void**)&pFT_New_Memory_Face, "FT_New_Memory_Face"},
			{(void**)&pFT_Init_FreeType, "FT_Init_FreeType"},
			{(void**)&pFT_Done_Face, "FT_Done_Face"},
			{(void**)&pFT_Error_String, "FT_Error_String"},
			{NULL, NULL}
		};
		if (triedtoloadfreetype)
			return false;
		triedtoloadfreetype = true;

#ifdef _WIN32
		fontmodule = Sys_LoadLibrary("libfreetype-6", ft2funcs);
		if (!fontmodule)
			fontmodule = Sys_LoadLibrary("freetype6", ft2funcs);
#else
		fontmodule = Sys_LoadLibrary("libfreetype.so.6", ft2funcs);
#endif
		if (!fontmodule)
		{
			Con_DPrintf("Couldn't load freetype library.\n");
			return false;
		}
#endif
		error = pFT_Init_FreeType(&fontlib);
		if (error)
		{
			Con_Printf("FT_Init_FreeType failed.\n");
#ifndef FREETYPE_STATIC
			Sys_CloseLibrary(fontmodule);
#endif
			return false;
		}
		/*any other errors leave freetype open*/
	}

	error = FT_Err_Cannot_Open_Resource;
	if (FS_FLocateFile(fontfilename, FSLF_IFFOUND|FSLF_QUIET, &loc) || FS_FLocateFile(va("%s.ttf", fontfilename), FSLF_IFFOUND|FSLF_QUIET, &loc) || FS_FLocateFile(va("%s.otf", fontfilename), FSLF_IFFOUND|FSLF_QUIET, &loc))
	{
		if (*loc.rawname && !loc.offset)
		{
			fbase = NULL;
			/*File is directly fopenable with no bias (not in a pk3/pak). Use the system-path form, so we don't have to eat the memory cost*/
			error = pFT_New_Face(fontlib, loc.rawname, 0, &face);
		}
		else
		{
			/*File is inside an archive, we need to read it and pass it as memory (and keep it available)*/
			vfsfile_t *f;
			f = FS_OpenReadLocation(loc.rawname, &loc);
			if (f && loc.len > 0)
			{
				fbase = BZ_Malloc(loc.len);
				VFS_READ(f, fbase, loc.len);
				VFS_CLOSE(f);

				error = pFT_New_Memory_Face(fontlib, fbase, loc.len, 0, &face);
			}
		}
	}

#if defined(LIBFONTCONFIG_STATIC)
	if (error && !strchr(fontfilename, '/'))
	{
		FcConfig *config = FcInitLoadConfigAndFonts();
		FcResult res;
		FcPattern *font;
		FcPattern *pat;

		//FcNameParse takes something of the form: "family:style1:style2". we already swapped spaces for : in styles.
		if (styles)
			pat = FcNameParse((const FcChar8*)va("%s:%s", fontfilename, styles));
		else
			pat = FcNameParse((const FcChar8*)fontfilename);
		FcConfigSubstitute(config, pat, FcMatchPattern);
		FcDefaultSubstitute(pat);

		// find the font
		font = FcFontMatch(config, pat, &res);
		if (font)
		{
			FcChar8 *file = NULL;
			if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
				error = pFT_New_Face(fontlib, file, 0, &face);	//'file' should be a system path
			FcPatternDestroy(font);
		}
		FcPatternDestroy(pat);
	}
#elif defined(_WIN32)
	if (error)
	{
		static qboolean firsttime = true;
		static char fontdir[MAX_OSPATH];

		if (firsttime)
		{
			HRESULT (WINAPI *dSHGetFolderPath) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);
			dllfunction_t shfolderfuncs[] =
			{
				{(void**)&dSHGetFolderPath, "SHGetFolderPathA"},
				{NULL, NULL}
			};
			dllhandle_t *shfolder = Sys_LoadLibrary("shfolder.dll", shfolderfuncs);

			firsttime = false;
			if (shfolder)
			{
				// 0x14 == CSIDL_FONTS
				if (dSHGetFolderPath(NULL, 0x14, NULL, 0, fontdir) != S_OK)
					*fontdir = 0;
				Sys_CloseLibrary(shfolder);
			}
		}
		if (*fontdir)
		{
			error = pFT_New_Face(fontlib, va("%s/%s", fontdir, fontfilename), 0, &face);
			if (error)
				error = pFT_New_Face(fontlib, va("%s/%s.ttf", fontdir, fontfilename), 0, &face);
		}
	}
#else
	if (error)
	{	//eg: /usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf
		error = pFT_New_Face(fontlib, va("/usr/share/fonts/%s", fontfilename), 0, &face);
		if (error)
			error = pFT_New_Face(fontlib, va("/usr/share/fonts/truetype/%s.ttf", fontfilename), 0, &face);
		if (error)	//just to give a chance of the same names working on more than one os, with the right package installed.
			error = pFT_New_Face(fontlib, va("/usr/share/fonts/truetype/msttcorefonts/%s.ttf", fontfilename), 0, &face);
	}
#endif
	if (!error)
	{
		int trueheight;
		/*success!*/
		qface = Z_Malloc(sizeof(*qface));
		qface->ft.face = face;
		qface->ft.activeheight = qface->ft.actualsize = 0;//height;
		qface->ft.membuf = fbase;
		qface->refs++;
		Q_strncpyz(qface->name, fontfilename, sizeof(qface->name));

		trueheight = Font_ChangeFTSize(qface, f->charheight);
		if (trueheight)
		{	//okay, we can use it, link it in.
			qface->flink = &faces;
			qface->fnext = *qface->flink;
			*qface->flink = qface;
			if (qface->fnext)
				qface->fnext->flink = &qface->fnext;

			if (!f->faces)
				f->truecharheight = trueheight;
			f->face[f->faces++] = qface;
			return true;
		}
		Z_Free(qface);
	}
	if (error && error != FT_Err_Cannot_Open_Resource)
		Con_Printf("Freetype(%s): error %i - %s\n", fontfilename, error, pFT_Error_String(error));
	if (fbase)
		BZ_Free(fbase);

	return false;
}
#endif

static texid_t Font_LoadReplacementConchars(void)
{
	texid_t tex;
	//q1 replacement
	tex = R_LoadHiResTexture("gfx/conchars.lmp", NULL, (r_font_linear.ival?IF_LINEAR:IF_NEAREST)|IF_PREMULTIPLYALPHA|IF_LOADNOW|IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE);
	TEXDOWAIT(tex);
	if (TEXLOADED(tex))
		return tex;
	//q2
	tex = R_LoadHiResTexture("pics/conchars.pcx", NULL, (r_font_linear.ival?IF_LINEAR:IF_NEAREST)|IF_PREMULTIPLYALPHA|IF_LOADNOW|IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE);
	TEXDOWAIT(tex);
	if (TEXLOADED(tex))
		return tex;
	//q3
	tex = R_LoadHiResTexture("gfx/2d/bigchars.tga", NULL, (r_font_linear.ival?IF_LINEAR:IF_NEAREST)|IF_PREMULTIPLYALPHA|IF_LOADNOW|IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE);
	TEXDOWAIT(tex);
	if (TEXLOADED(tex))
		return tex;
	return r_nulltex;
}

#ifdef HEXEN2
static texid_t Font_LoadHexen2Conchars(qboolean iso88591)
{
	//gulp... so it's come to this has it? rework the hexen2 conchars into the q1 system.
	texid_t tex;
	unsigned int i, x;
	unsigned char *tempchars;
	unsigned char *in, *out, *outbuf;
	FS_LoadFile("gfx/menu/conchars.lmp", (void**)&tempchars);

	/*hexen2's conchars are arranged 32-wide, 16 high.
	the upper 8 rows are 256 8859-1 chars
	the lower 8 rows are a separate set of recoloured 8859-1 chars.

	if we're loading for the fallback then we're loading this data for quake compatibility, 
	so we grab only the first 4 rows of each set of chars (128 low chars, 128 high chars).

	if we're loading a proper charset, then we load only the first set of chars, we can recolour the rest anyway (com_parseutf8 will do so anyway).
	as a final note, parsing iso8859-1 french/german/etc as utf8 will generally result in decoding errors which can gracefully revert to 8859-1 safely. If this premise fails too much, we can always change the parser for different charsets - the engine always uses unicode and thus 8859-1 internally.
	*/
	if (tempchars)
	{
		outbuf = BZ_Malloc(8*8*256*8);

		out = outbuf;
		i = 0;
		/*read the low chars*/
		for (; i < 8*8*1; i+=1)
		{
			if (i&(1<<3))
				in = tempchars + (i>>3)*16*8*8+(i&7)*32*8 - 256*4+128;
			else
				in = tempchars + (i>>3)*16*8*8+(i&7)*32*8;
			for (x = 0; x < 16*8; x++)
				*out++ = *in++;
		}
		if (iso88591)
		{
			/*read the non 8859-1 quake-compat control chars*/
			for (; i < 8*8*1 + 16; i+=1)
			{
				if (i&(1<<3))
					in = tempchars+128*128 + ((i>>3)&7)*16*8*8+(i&7)*32*8 - 256*4+128;
				else
					in = tempchars+128*128 + ((i>>3)&7)*16*8*8+(i&7)*32*8;
				for (x = 0; x < 16*8; x++)
					*out++ = *in++;
			}

			/*read the final low chars (final if 8859-1 anyway)*/
			for (; i < 8*8*2; i+=1)
			{
				if (i&(1<<3))
					in = tempchars + (i>>3)*16*8*8+(i&7)*32*8 - 256*4+128;
				else
					in = tempchars + (i>>3)*16*8*8+(i&7)*32*8;
				for (x = 0; x < 16*8; x++)
					*out++ = *in++;
			}
		}
		else
		{
			/*read the high chars*/
			for (; i < 8*8*2; i+=1)
			{
				if (i&(1<<3))
					in = tempchars+128*128 + ((i>>3)&15)*16*8*8+(i&7)*32*8 - 256*4+128;
				else
					in = tempchars+128*128 + ((i>>3)&15)*16*8*8+(i&7)*32*8;
				for (x = 0; x < 16*8; x++)
					*out++ = *in++;
			}
		}
		FS_FreeFile(tempchars);

		// add ocrana leds
		if (!iso88591 && con_ocranaleds.value && con_ocranaleds.value != 2)
			AddOcranaLEDsIndexed (outbuf, 128, 128);

		for (i=0 ; i<128*128 ; i++)
			if (outbuf[i] == 0)
				outbuf[i] = 255;	// proper transparent color
		tex = R_LoadTexture8 (iso88591?"gfx/menu/8859-1.lmp":"charset", 128, 128, outbuf, IF_PREMULTIPLYALPHA|IF_LOADNOW|IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA, 1);
		Z_Free(outbuf);
		return tex;
	}
	return r_nulltex;
}
#endif

FTE_ALIGN(4) qbyte default_conchar[/*11356*/] =
{
#include "lhfont.h"
};

static void Font_CopyGlyph(int src, int dst, void *data)
{
	int glyphsize = 16;
	int y;
	int x;
	char *srcptr = (char*)data + (src&15)*glyphsize*4 + (src>>4)*glyphsize*256*4;
	char *dstptr = (char*)data + (dst&15)*glyphsize*4 + (dst>>4)*glyphsize*256*4;
	for (y = 0; y < glyphsize; y++)
	{
		for (x = 0; x < glyphsize; x++)
		{
			dstptr[x*4+0] = srcptr[x*4+0];
			dstptr[x*4+1] = srcptr[x*4+1];
			dstptr[x*4+2] = srcptr[x*4+2];
			dstptr[x*4+3] = srcptr[x*4+3];
		}
		dstptr += 256*4;
		srcptr += 256*4;
	}
}
static texid_t Font_LoadFallbackConchars(void)
{
	texid_t tex;
	int width, height;
	unsigned int i;
	uploadfmt_t format;
	qbyte *lump = ReadRawImageFile(default_conchar, sizeof(default_conchar), &width, &height, &format, false, "conchars");
	if (!lump || (format != PTI_RGBX8 && format != PTI_RGBA8 && format != PTI_LLLX8))
		return r_nulltex;
	/*convert greyscale to alpha*/
	for (i = 0; i < width*height; i++)
	{
		lump[i*4+3] = lump[i*4];
		lump[i*4+0] = 255;
		lump[i*4+1] = 255;
		lump[i*4+2] = 255;
	}
	if (width == 256 && height == 256)
	{	//make up some scroll-bar/download-progress-bar chars, so that webgl doesn't look so buggy with the initial pak file(s).
		Font_CopyGlyph('[', 128, lump);
		Font_CopyGlyph('-', 129, lump);
		Font_CopyGlyph(']', 130, lump);
		Font_CopyGlyph('|', 131, lump);
		Font_CopyGlyph('>', 13, lump);
	}
	tex = Image_GetTexture("charset", NULL, IF_PREMULTIPLYALPHA|IF_LOADNOW|IF_UIPIC|IF_NOMIPMAP|IF_NOGAMMA|IF_NOPURGE, (void*)lump, NULL, width, height, PTI_RGBA8);
	BZ_Free(lump);
	return tex;
}

/*loads a fallback image. not allowed to fail (use syserror if needed)*/
static texid_t Font_LoadDefaultConchars(enum fontfmt_e *fmt)
{
	texid_t tex;
	tex = Font_LoadReplacementConchars();
	if (TEXLOADED(tex))
		return tex;
#ifdef HEXEN2
	tex = Font_LoadHexen2Conchars(true);
	if (tex && tex->status == TEX_LOADING)
		COM_WorkerPartialSync(tex, &tex->status, TEX_LOADING);
	if (TEXLOADED(tex))
	{
		*fmt = FMT_ISO88591;
		return tex;
	}
#endif
	tex = Font_LoadFallbackConchars();
	if (tex && tex->status == TEX_LOADING)
		COM_WorkerPartialSync(tex, &tex->status, TEX_LOADING);
	if (TEXLOADED(tex))
	{
		*fmt = FMT_QUAKE;
		return tex;
	}
	Sys_Error("Unable to load any conchars\n");
}

typedef struct 
{ 
    short		width;
    short		height; 
    short		leftoffset;	// pixels to the left of origin 
    short		topoffset;	// pixels below the origin 
    int			columnofs[1];
} doompatch_t;
typedef struct
{
    unsigned char		topdelta;	// -1 is the last post in a column
    unsigned char		length; 	// length data bytes follows
} doomcolumn_t;
void Doom_ExpandPatch(doompatch_t *p, unsigned char *b, int stride)
{
	doomcolumn_t *col;
	unsigned char *src, *dst;
	int x, y;
	for (x = 0; x < p->width; x++)
	{
		col = (doomcolumn_t *)((unsigned char *)p + p->columnofs[x]);
		while(col->topdelta != 0xff)
		{
			//exploit protection
			if (col->length + col->topdelta > p->height)
				break;

			src = (unsigned char *)col + 2; /*why 3? why not, I suppose*/
			dst = b + stride*col->topdelta;
			for (y = 0; y < col->length; y++)
			{
				*dst = *src++;
				dst += stride;
			}
			src++;
			col = (doomcolumn_t *)((unsigned char*)col + col->length + 4);
		}
		b++;
	}
}

static qboolean Font_LoadFontLump(font_t *f, const char *facename)
{
	size_t lumpsize = 0;
	qbyte lumptype = 0;
	void *lumpdata;

	if (f->faces == MAX_FACES)
		return false;	//can't store it...
	lumpdata = W_GetLumpName(facename, &lumpsize, &lumptype);
	if (!lumpdata)
		return false;
	if ((lumptype == TYP_MIPTEX && lumpsize==(8*8)*(16*16)) ||	//proper format
		(lumptype == TYP_QPIC   && lumpsize==(8*8)*(16*16)+8))	//fucked up buggy format used by some people
	{
		fontface_t *fa = Z_Malloc(sizeof(*fa));
		fa->horiz.data = lumpdata;
		fa->horiz.stride = 8*16;
		fa->horiz.charheight = 8;
		fa->horiz.codepage = FMT_QUAKE;
		fa->horiz.paletted = true;
		fa->horiz.rows = 16;

		if (con_ocranaleds.ival)
		{
			if (con_ocranaleds.ival != 2 || CalcHashInt(&hash_crc16, lumpdata, 128*128) == 798)
				AddOcranaLEDsIndexed (lumpdata, 128, 128);
		}

		fa->flink = &fa->fnext;
		fa->refs = 1;
		f->face[f->faces++] = fa;
		return true;
	}
#ifdef HALFLIFEMODELS
	else if (lumptype == TYP_HLFONT)
	{
		fontface_t *fa = Z_Malloc(sizeof(*fa));
		fa->halflife = lumpdata;
		fa->flink = &fa->fnext;
		fa->refs = 1;
		f->face[f->faces++] = fa;
//		f->charheight = fa->halflife->fontheight1;	//force the font to a specific size.
		return true;
	}
#endif
	return false;	//doesn't match a valid known format
}

//creates a new font object from the given file, with each text row with the given height.
//width is implicit and scales with height and choice of font.
struct font_s *Font_LoadFont(const char *fontfilename, float vheight, float scale, int outline, unsigned int flags)
{
	struct font_s *f;
	int i = 0;
	int defaultplane;
	char *aname;
	char *parms;
	int height = ((vheight * vid.rotpixelheight)/vid.height) + 0.5;
	char facename[MAX_QPATH*12];
#ifdef AVAIL_FREETYPE
	char *styles = NULL;
#endif
	struct charcache_s *c;
	float aspect = 1;
	enum fontfmt_e fmt = FMT_AUTO;
	qboolean explicit;

	Q_strncpyz(facename, fontfilename, sizeof(facename));

	aname = strstr(facename, ":");
	if (aname)
		*aname++ = 0;
	parms = strstr(facename, "?");
	if (parms)
		*parms++ = 0;

	f = Z_Malloc(sizeof(*f));
	f->outline = outline;
	f->scale = scale;
	if (height < 1)	//doesn't make sense. especially negatives...
		height = 1;
	if (height > 128)
		height = 128;	//limit possible damage... we use alloca a bit so don't let the stack get abused too much.
	f->charheight = height;
	f->truecharheight = height;
	f->flags = flags;
	Q_strncpyz(f->name, fontfilename, sizeof(f->name));

	switch(M_GameType())
	{
	case MGT_QUAKE2:
		VectorSet(f->alttint, 0.44, 1.0, 0.2);
		break;
	default:
		VectorSet(f->alttint, 1.16, 0.54, 0.41);
		break;
	}
	VectorSet(f->tint, 1, 1, 1);
	fontfilename = facename;

	if (parms)
	{
		while (*parms)
		{
			if (!strncmp(parms, "col=", 4))
			{
				char *t = parms+4;
				f->tint[0] = strtod(t, &t);
				if (*t == ',') t++;
				if (*t == ' ') t++;
				f->tint[1] = strtod(t, &t);
				if (*t == ',') t++;
				if (*t == ' ') t++;
				f->tint[2] = strtod(t, &t);
				parms = t;
			}
			if (!strncmp(parms, "fmt=", 4))
			{
				char *t = parms+4;
				fmt = 0;
				if (*t == 'q')
					fmt = FMT_QUAKE;
				else if (*t == 'l')
					fmt = FMT_ISO88591;
				else if (*t == 'w')
					fmt = FMT_WINDOWS1252;
				else if (*t == 'k')
					fmt = FMT_KOI8U;
				else if (*t == 'h')
					fmt = FMT_HORIZONTAL;
			}
			if (!strncmp(parms, "aspect=", 7))
			{
				char *t = parms+7;
				aspect = strtod(t, &t);
				parms = t;
			}
#ifdef AVAIL_FREETYPE
			if (!strncmp(parms, "style=", 6))
			{
				char *t = parms+6;
				styles = t;
				while (*t && *t != '&')
				{
					if (*t == ' ')
						*t = ':';
					t++;
				}
				parms = t;
			}
#endif

			while(*parms && *parms != '&')
				parms++;
			if (*parms == '&')
			{
				*parms++ = 0;
				continue;
			}
		}
	}

	if (vid.flags&VID_SRGBAWARE)
	{
		f->tint[0] = M_SRGBToLinear(f->tint[0], 1);
		f->tint[1] = M_SRGBToLinear(f->tint[1], 1);
		f->tint[2] = M_SRGBToLinear(f->tint[2], 1);

		f->alttint[0] = M_SRGBToLinear(f->alttint[0], 1);
		f->alttint[1] = M_SRGBToLinear(f->alttint[1], 1);
		f->alttint[2] = M_SRGBToLinear(f->alttint[2], 1);
	}

#ifdef PACKAGE_DOOMWAD
	if (!*fontfilename)
	{
		unsigned char buf[PLANEWIDTH*PLANEHEIGHT];
		int i;
		int x=0,y=0,h=0;
		doompatch_t *dp;

		memset(buf, 0, sizeof(buf));

		for (i = '!'; i <= '_'; i++)
		{
			dp = NULL;
			FS_LoadFile(va("wad/stcfn%.3d", i), (void**)&dp);
			if (!dp)
				break;

			/*make sure it can fit*/
			if (x + dp->width > PLANEWIDTH)
			{
				x = 0;
				y += h;
				h = 0;
			}

			c = Font_GetCharStore(f, i);
			c->advance = dp->width;	/*this is how much line space the char takes*/
			c->left = -dp->leftoffset;
			c->top = -dp->topoffset;
			c->nextchar = 0;
			c->texplane = SINGLEPLANE;

			c->bmx = x;
			c->bmy = y;
			c->bmh = dp->height;
			c->bmw = dp->width;

			Doom_ExpandPatch(dp, &buf[y*PLANEWIDTH + x], PLANEWIDTH);

			x += dp->width;
			if (dp->height > h)
			{
				h = dp->height;
				if (h > f->charheight)
					f->charheight = h;
			}

			FS_FreeFile(dp);
		}

		/*if all loaded okay, replicate the chars to the quake-compat range (both white+red chars)*/
		if (i == '_'+1)
		{
			//doom doesn't have many chars, so make sure the lower case chars exist.
			for (i = 'a'; i <= 'z'; i++)
				Font_CopyChar(f, i-'a'+'A', i);

			//no space char either
			c = Font_GetCharStore(f, ' ');
			c->advance = 8;

			f->singletexture = R_LoadTexture8("doomfont", PLANEWIDTH, PLANEHEIGHT, buf, 0, true);
			for (i = 0xe000; i <= 0xe0ff; i++)
				Font_CopyChar(f, toupper(i&0x7f), i);
			return f;
		}
	}
#endif
#ifdef HEXEN2
	if (!strcmp(fontfilename, "gfx/tinyfont"))
	{
		unsigned int *img;
		int x, y;
		size_t lumpsize;
		qbyte lumptype;
		unsigned char *w = W_GetLumpName(fontfilename+4, &lumpsize, &lumptype);
		if (!w || lumpsize != 32*128 || lumptype != 'D')
		{
			Z_Free(f);
			return NULL;
		}
		img = Z_Malloc(PLANEWIDTH*PLANEWIDTH*4);
		for (y = 0; y < 32; y++)
			for (x = 0; x < 128; x++)
				img[x + y*PLANEWIDTH] = w[x + y*128]?d_8to24rgbtable[w[x + y*128]]:0;

		f->singletexture = R_LoadTexture("tinyfont",PLANEWIDTH,PLANEWIDTH,TF_RGBA32,img,IF_PREMULTIPLYALPHA|IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_NOPURGE);
		if (f->singletexture->status == TEX_LOADING)
			COM_WorkerPartialSync(f->singletexture, &f->singletexture->status, TEX_LOADING);
		Z_Free(img);

		for (i = 0x00; i <= 0xff; i++)
		{
			c = Font_GetCharStore(f, i);
			c->advance = (height*3)/4;
			c->left = 0;
			c->top = 0;
			c->nextchar = 0;	//these chars are not linked in
			c->texplane = BITMAPPLANE;	/*if its a 'raster' font, don't use the default chars, always use the raster images*/


			if (i >= 'a' && i <= 'z')
			{
				c->bmx = ((i - 64)&15)*8;
				c->bmy = ((i - 64)/16)*8;
				c->bmh = 8;
				c->bmw = 8;
			}
			else if (i >= 32 && i < 96)
			{
				c->bmx = ((i - 32)&15)*8;
				c->bmy = ((i - 32)/16)*8;
				c->bmh = 8;
				c->bmw = 8;
			}
			else
			{
				c->bmh = 0;
				c->bmw = 0;
				c->bmx = 0;
				c->bmy = 0;
			}

			Font_CopyChar(f, i, i|0xe0ff);
		}
		return f;
	}
#endif

	if (aname)
	{
		if (!strncmp(aname, "?col=", 5))
		{
			char *t = aname+5;
			f->alttint[0] = strtod(t, &t);
			if (*t == ',') t++;
			if (*t == ' ') t++;
			f->alttint[1] = strtod(t, &t);
			if (*t == ',') t++;
			if (*t == ' ') t++;
			f->alttint[2] = strtod(t, &t);
		}
		else
		{
			f->alt = Font_LoadFont(aname, vheight, scale, outline, flags);
			if (f->alt)
			{
				VectorCopy(f->alt->tint, f->alttint);
				VectorCopy(f->alt->tint, f->alt->alttint);
			}
		}
	}

	explicit = false;	//singletexture is some weird custom layout and not to be trusted.
	{
		const char *start;
		qboolean success;
		start = fontfilename;
		for(;;)
		{
			char *end = strchr(start, ',');
			if (end)
				*end = 0;

			if (fmt == FMT_AUTO && *start && Font_LoadKexFont(f, height, start))
				success = explicit = true;
			else if (fmt == FMT_HORIZONTAL)
				success = Font_LoadHorizontalFont(f, height, start);
#ifdef AVAIL_FREETYPE
			else if (fmt == FMT_AUTO && Font_LoadFreeTypeFont(f, height, start, styles))
				success = true;
#endif
			else
				success = false;

			if (!success && !TEXLOADED(f->singletexture) && *start)
			{
				const char *ext = COM_GetFileExtension(start, NULL);
				if (!Q_strcasecmp(ext, ".ttf") || !Q_strcasecmp(ext, ".otf"))
					;	//no, don't try loading it as an image-based font. just let it fail.
				else
				{
					f->singletexture = R_LoadHiResTexture(start, "fonts:charsets", IF_PREMULTIPLYALPHA|(r_font_linear.ival?IF_LINEAR:IF_NEAREST)|IF_UIPIC|IF_NOPICMIP|IF_NOMIPMAP|IF_NOPURGE|IF_LOADNOW);
					if (f->singletexture->status == TEX_LOADING)
						COM_WorkerPartialSync(f->singletexture, &f->singletexture->status, TEX_LOADING);

					if (!TEXLOADED(f->singletexture) && f->faces < MAX_FACES)
						Font_LoadFontLump(f, start);
				}
			}

			if (end)
			{
				*end = ',';
				start = end+1;
			}
			else
				break;
		}
	}

	if (!f->faces && !TEXLOADED(f->singletexture) && r_font_linear.ival)
		Font_LoadFontLump(f, "conchars");

	defaultplane = INVALIDPLANE;/*assume the bitmap plane - don't use the fallback as people don't think to use com_parseutf8*/
	if (!explicit && TEXLOADED(f->singletexture))
		defaultplane = BITMAPPLANE;
	else if (TEXLOADED(fontplanes.defaultfont))
		defaultplane = DEFAULTPLANE;

	if (defaultplane == INVALIDPLANE)
	{
		if (!TEXLOADED(fontplanes.defaultfont))
		{
			fontplanes.defaultfont = Font_LoadDefaultConchars(&fmt);
		}

#ifdef HEXEN2
		if (!strcmp(fontfilename, "gfx/hexen2"))
		{
			f->singletexture = Font_LoadHexen2Conchars(false);
			defaultplane = DEFAULTPLANE;
		}
#endif
		if (!TEXLOADED(f->singletexture))
			f->singletexture = fontplanes.defaultfont;

		if (TEXLOADED(f->singletexture))
			defaultplane = BITMAPPLANE;
		else if (TEXLOADED(fontplanes.defaultfont))
			defaultplane = DEFAULTPLANE;
	}

	if (defaultplane != INVALIDPLANE)
	{
		if (fmt==FMT_AUTO)
			fmt=FMT_QUAKE;
		if (!f->faces)
		{
			static const unsigned short iso88591[] = {
				0x80,0x81,0x82,0x83, 0x84,0x85,0x86,0x87, 0x88,0x89,0x8a,0x8b, 0x8c,0x8d,0x8e,0x8f,
				0x90,0x91,0x92,0x93, 0x94,0x95,0x96,0x97, 0x98,0x99,0x9a,0x9b, 0x9c,0x9d,0x9e,0x9f};
			static const unsigned short win1252[] = {
				0x20ac,  0x81,0x201a,0x0192, 0x201e,0x2026,0x2020,0x2021, 0x02c6,0x2030,0x0160,0x2039, 0x0152,  0x8d,0x017d,  0x8f,
				  0x90,0x2018,0x2019,0x101c, 0x201d,0x2022,0x2013,0x2014, 0x02dc,0x2122,0x0161,0x203a, 0x0153,  0x9d,0x017e,0x0178};
			static const unsigned short koi8u[] = {
				0x2500,0x2502,0x250C,0x2510, 0x2514,0x2518,0x251C,0x2524, 0x252C,0x2534,0x253C,0x2580, 0x2584,0x2588,0x258C,0x2590,
				0x2591,0x2592,0x2593,0x2320, 0x25A0,0x2219,0x221A,0x2248, 0x2264,0x2265,0x00A0,0x2321, 0x00B0,0x00B2,0x00B7,0x00F7,
				0x2550,0x2551,0x2552,0x0451, 0x0454,0x2554,0x0456,0x0457, 0x2557,0x2558,0x2559,0x255A, 0x255B,0x0491,0x255D,0x255E,
				0x255F,0x2560,0x2561,0x0401, 0x0404,0x2563,0x0406,0x0407, 0x2566,0x2567,0x2568,0x2569, 0x256A,0x0490,0x256C,0x00A9,
				0x044E,0x0430,0x0431,0x0446, 0x0434,0x0435,0x0444,0x0433, 0x0445,0x0438,0x0439,0x043A, 0x043B,0x043C,0x043D,0x043E,
				0x043F,0x044F,0x0440,0x0441, 0x0442,0x0443,0x0436,0x0432, 0x044C,0x044B,0x0437,0x0448, 0x044D,0x0449,0x0447,0x044A,
				0x042E,0x0410,0x0411,0x0426, 0x0414,0x0415,0x0424,0x0413, 0x0425,0x0418,0x0419,0x041A, 0x041B,0x041C,0x041D,0x041E,
				0x041F,0x042F,0x0420,0x0421, 0x0422,0x0423,0x0416,0x0412, 0x042C,0x042B,0x0417,0x0428, 0x042D,0x0429,0x0427,0x042A};
			const unsigned short *c1;
			unsigned int c1size;

			if (fmt == FMT_WINDOWS1252)
			{	//some tools use these extra ones (latin-1 has no visible c1 entries)
				c1 = win1252;
				c1size = countof(win1252);
			}
			else if (fmt == FMT_KOI8U)
			{	//lots of russians in the quake scene
				c1 = koi8u;
				c1size = countof(koi8u);
			}
			else
			{
				c1 = iso88591;
				c1size = countof(iso88591);
			}
			c1size += 128;

			/*force it to load, even if there's nothing there*/
			for (i = ((fmt==FMT_QUAKE)?32:0); i < ((fmt==FMT_QUAKE)?128:256); i++)
			{
				if (i >= 128 && i < c1size)
					c = Font_GetCharStore(f, c1[i-128]);
				else
					c = Font_GetCharStore(f, i);

				c->advance = f->charheight * aspect;
				c->bmh = PLANEWIDTH/16;
				c->bmw = PLANEWIDTH/16;
				c->bmx = (i&15)*(PLANEWIDTH/16);
				c->bmy = (i/16)*(PLANEWIDTH/16);
				c->left = 0;
				c->top = 0;
				c->nextchar = 0;	//these chars are not linked in
				c->texplane = defaultplane;
			}
		}

		if (fmt == FMT_QUAKE)
		{
			/*pack the default chars into it*/
			for (i = 0xe000; i <= 0xe0ff; i++)
			{
				c = Font_GetCharStore(f, i);
				c->advance = f->charheight * aspect;
				c->bmh = PLANEWIDTH/16;
				c->bmw = PLANEWIDTH/16;
				c->bmx = ((i&15))*(PLANEWIDTH/16);
				c->bmy = ((i&0xf0)/16)*(PLANEWIDTH/16);
				c->left = 0;
				c->top = 0;
				c->nextchar = 0;	//these chars are not linked in
				c->texplane = defaultplane;
			}
		}
	}
	return f;
}

//removes a font from memory.
void Font_Free(struct font_s *f)
{
	size_t i;
	struct charcache_s **link, *c, *valid;

	if (!f)
		return;

	//kill the alt font first.
	if (f->alt)
	{
		Font_Free(f->alt);
		f->alt = NULL;
	}
	valid = NULL;
	//walk all chars, unlinking any that appear to be within this font's char cache
	for (link = &fontplanes.oldestchar; *link; )
	{
		c = *link;
		if (f->chars[c->block] && c >= f->chars[c->block] && c <= f->chars[c->block] + FONTBLOCKSIZE)
		{
			c = c->nextchar;
			if (!c)
				fontplanes.newestchar = valid;
			*link = c;
		}
		else
		{
			valid = c;
			link = &c->nextchar;
		}
	}

	while(f->faces --> 0)
	{
		fontface_t *qface = f->face[f->faces];
		qface->refs--;
		if (!qface->refs)
		{
#ifdef AVAIL_FREETYPE
			if (qface->ft.face)
				pFT_Done_Face(qface->ft.face);
			if (qface->ft.membuf)
				BZ_Free(qface->ft.membuf);
#endif
			*qface->flink = qface->fnext;
			if (qface->fnext)
				qface->fnext->flink = qface->flink;
			Z_Free(qface);
		}
	}

	for (i = 0; i < FONTBLOCKS; i++)
		if (f->chars[i])
			Z_Free(f->chars[i]);
	Z_Free(f);
}

//maps a given virtual screen coord to a pixel coord, which matches the font's height/width values
void Font_BeginString(struct font_s *font, float vx, float vy, int *px, int *py)
{
	if (R2D_Flush && (R2D_Flush != Font_Flush || curfont != font || font_be_flags != r2d_be_flags))
		R2D_Flush();
	R2D_Flush = Font_Flush;
	font_be_flags = r2d_be_flags;
	curfont = font;
	*px = (vx*(int)vid.rotpixelwidth) / (float)vid.width;
	*py = (vy*(int)vid.rotpixelheight) / (float)vid.height;

	curfont_scale[0] = curfont->charheight;
	curfont_scale[1] = curfont->charheight;
//	curfont_scaled = false;
}
void Font_Transform(float vx, float vy, int *px, int *py)
{
	if (px)
		*px = (vx*(int)vid.rotpixelwidth) / (float)vid.width;
	if (py)
		*py = (vy*(int)vid.rotpixelheight) / (float)vid.height;
}
void Font_BeginScaledString(struct font_s *font, float vx, float vy, float szx, float szy, float *px, float *py)
{
	if (R2D_Flush && (R2D_Flush != Font_Flush || curfont != font || font_be_flags != r2d_be_flags))
		R2D_Flush();
	R2D_Flush = Font_Flush;
	font_be_flags = r2d_be_flags;
	curfont = font;
	*px = (vx*(float)vid.rotpixelwidth) / (float)vid.width;
	*py = (vy*(float)vid.rotpixelheight) / (float)vid.height;

	//now that its in pixels, clamp it so the text is at least consistant with its position.
	//an individual char may end straddling a pixel boundary, but at least the pixels won't jiggle around as the text moves.
	*px = (int)*px;
	*py = (int)*py;

/*	if ((int)(szx * vid.rotpixelheight/vid.height) == curfont->charheight && (int)(szy * vid.rotpixelheight/vid.height) == curfont->charheight)
		curfont_scaled = false;
	else
		curfont_scaled = true;
*/
	curfont_scale[0] = (szx * (float)vid.rotpixelheight) / (curfont->charheight * (float)vid.height);
	curfont_scale[1] = (szy * (float)vid.rotpixelheight) / (curfont->charheight * (float)vid.height);
	curfont_scale[0] *= curfont->scale;
	curfont_scale[1] *= curfont->scale;
}

void Font_EndString(struct font_s *font)
{
//	Font_Flush();
//	curfont = NULL;

	R2D_Flush = font_foremesh.numindexes?Font_Flush:NULL;
}

//obtains the font's row height (each row of chars should be drawn using this increment)
int Font_CharHeight(void)
{
	return curfont->charheight;
}
int Font_CharPHeight(struct font_s *font)
{
	return font->charheight;
}
int Font_GetTrueHeight(struct font_s *font)	//Char[P]Height lies for compat with DP.
{
	return font->truecharheight;
}
float Font_CharVHeight(struct font_s *font)
{
	return ((float)font->charheight * vid.height)/vid.rotpixelheight;
}

//obtains the font's row height (each row of chars should be drawn using this increment)
float Font_CharScaleHeight(void)
{
	return curfont->charheight * curfont_scale[1];
}

int Font_TabWidth(int x)
{
	int tabwidth = Font_CharWidth(CON_WHITEMASK, ' ');
	if (!tabwidth)
		tabwidth = curfont->charheight;
	tabwidth *= 8;

	x++;
	x = x + ((tabwidth - (x % tabwidth)) % tabwidth);
	return x;
}

/*
This is where the character ends.
Note: this function supports tabs - x must always be based off 0, with Font_LineDraw actually used to draw the line.
*/
int Font_CharEndCoord(struct font_s *font, int x, unsigned int charflags, unsigned int codepoint)
{
	struct charcache_s *c;

	if (charflags&CON_HIDDEN)
		return x;
	if (codepoint == '\t')
		return Font_TabWidth(x);

	if ((charflags & CON_2NDCHARSETTEXT) && font->alt)
		font = font->alt;

	c = Font_GetChar(font, codepoint);
	if (!c)
	{
		return x+0;
	}

	return x+c->advance;
}

//obtains the width of a character from a given font. This is how wide it is. The next char should be drawn at x + result.
//FIXME: this function cannot cope with tab and should not be used.
int Font_CharWidth(unsigned int charflags, unsigned int codepoint)
{
	struct charcache_s *c;
	struct font_s *font = curfont;

	if (charflags&CON_HIDDEN)
		return 0;

	if ((charflags & CON_2NDCHARSETTEXT) && font->alt)
		font = font->alt;

	c = Font_GetChar(curfont, codepoint);
	if (!c)
	{
		return 0;
	}

	return c->advance;
}

//obtains the width of a character from a given font. This is how wide it is. The next char should be drawn at x + result.
//FIXME: this function cannot cope with tab and should not be used.
float Font_CharScaleWidth(unsigned int charflags, unsigned int codepoint)
{
	struct charcache_s *c;
	struct font_s *font = curfont;

	if (charflags&CON_HIDDEN)
		return 0;
	if ((charflags & CON_2NDCHARSETTEXT) && font->alt)
		font = font->alt;

	c = Font_GetChar(curfont, codepoint);
	if (!c)
	{
		return 0;
	}

	return c->advance * curfont_scale[0];
}

conchar_t *Font_DecodeReverse(conchar_t *start, conchar_t *stop, unsigned int *codeflags, unsigned int *codepoint)
{
	if (start <= stop)
	{
		*codeflags = 0;
		*codepoint = 0;
		return stop;
	}

	start--;
	if (start > stop && start[-1] & CON_LONGCHAR)
		if (!(start[-1] & CON_RICHFORECOLOUR))
		{
			start--;
			*codeflags = start[1];
			*codepoint = ((start[0] & CON_CHARMASK)<<16) | (start[1] & CON_CHARMASK);
			return start;
		}
	*codeflags = start[0];
	*codepoint = start[0] & CON_CHARMASK;
	return start;
}

//for a given font, calculate the line breaks and word wrapping for a block of text
//start+end are the input string
//starts+ends are an array of line start and end points, which have maxlines elements.
//(end is the terminator, null or otherwise)
//maxpixelwidth is the width of the display area in pixels
int Font_LineBreaks(conchar_t *start, conchar_t *end, int maxpixelwidth, int maxlines, conchar_t **starts, conchar_t **ends)
{
	conchar_t *l, *bt, *n;
	int px;
	int foundlines = 0;
	struct font_s *font = curfont;
	unsigned int codeflags, codepoint;

	while (start < end)
	{
	// scan the width of the line
		for (px=0, l=start ; px <= maxpixelwidth; )
		{
			if (l >= end)
				break;
			n = Font_Decode(l, &codeflags, &codepoint);
			if (!(codeflags & CON_HIDDEN) && (codepoint == '\n' || codepoint == '\v'))
				break;
			px = Font_CharEndCoord(font, px, codeflags, codepoint);
			l = n;
		}
		//if we did get to the end
		if (px > maxpixelwidth)
		{
			bt = l;
			//backtrack until we find a space
			for(;;)
			{
				n = Font_DecodeReverse(l, start, &codeflags, &codepoint);
				if (codepoint > ' ')
					l = n;
				else
				{
					l = n;
					break;
				}
			}
			if (l == start && bt>start)
				l = Font_DecodeReverse(bt, start, &codeflags, &codepoint);
		}

		starts[foundlines] = start;
		ends[foundlines] = l;
		foundlines++;
		if (foundlines == maxlines)
			break;

		for (;;)
		{
			if (l >= end)
				break;
			n = Font_Decode(l, &codeflags, &codepoint);
			if (!(codeflags & CON_HIDDEN) && (codepoint != ' '))
				break;
			l = n;
		}

		start=l;
		if (start == end)
			break;

		if ((*start&(CON_CHARMASK|CON_HIDDEN)) == '\n' || (*start&(CON_CHARMASK|CON_HIDDEN)) == '\v')
			start++;                // skip the \n
	}

	return foundlines;
}

int Font_LineWidth(conchar_t *start, conchar_t *end)
{
	//fixme: does this do the right thing with tabs?
	int x = 0;
	struct font_s *font = curfont;
	unsigned int codeflags, codepoint;
	for (; start < end; )
	{
		start = Font_Decode(start, &codeflags, &codepoint);
		x = Font_CharEndCoord(font, x, codeflags, codepoint);
	}
	return x;
}
float Font_LineScaleWidth(conchar_t *start, conchar_t *end)
{
	int x = 0;
	struct font_s *font = curfont;
	unsigned int codeflags, codepoint;
	while(start < end)
	{
		start = Font_Decode(start, &codeflags, &codepoint);
		x = Font_CharEndCoord(font, x, codeflags, codepoint);
	}
	return x * curfont_scale[0];
}
void Font_LineDraw(int x, int y, conchar_t *start, conchar_t *end)
{
	int lx = 0;
	struct font_s *font = curfont;
	unsigned int codeflags, codepoint;
	for (; start < end; )
	{
		start = Font_Decode(start, &codeflags, &codepoint);
		Font_DrawChar(x+lx, y, codeflags, codepoint);
		lx = Font_CharEndCoord(font, lx, codeflags, codepoint);
	}
}

conchar_t *Font_CharAt(int x, conchar_t *start, conchar_t *end)
{
	int lx = 0, nx;
	struct font_s *font = curfont;
	unsigned int codeflags, codepoint;
	conchar_t *nc;
	for (; start < end; lx = nx, start = nc)
	{
		nc = Font_Decode(start, &codeflags, &codepoint);
		nx = Font_CharEndCoord(font, lx, codeflags, codepoint);
		if (x >= lx && x < nx)
			return start;
	}
	return NULL;
}

/*Note: *all* strings after the current one will inherit the same colour, until one changes it explicitly
correct usage of this function thus requires calling this with 1111 before Font_EndString*/
void Font_InvalidateColour(vec4_t newcolour)
{
	if (font_foretint[0] == newcolour[0] && font_foretint[1] == newcolour[1] && font_foretint[2] == newcolour[2] && font_foretint[3] == newcolour[3])
		return;

	if ((font_colourmask & CON_NONCLEARBG) && font_foremesh.numindexes)
	{
		if (R2D_Flush)
			R2D_Flush();
		R2D_Flush = Font_Flush;
	}
	font_colourmask = CON_WHITEMASK;

	VectorScale(newcolour, newcolour[3], font_foretint);
	font_foretint[3] = newcolour[3];
	Vector4Scale(font_foretint, 255, font_forecolour.rgba);

	font_backcolour.rgba[3] = 0;

	/*Any drawchars that are now drawn will get the forced colour*/
}

//draw a character from the current font at a pixel location.
int Font_DrawChar(int px, int py, unsigned int charflags, unsigned int codepoint)
{
	struct charcache_s *c;
	float s0, s1;
	float t0, t1;
	float nextx;
	float sx, sy, sw, sh;
	int col;
	int v;
	struct font_s *font = curfont;
#ifdef D3D11QUAKE
	float dxbias = 0;//(qrenderer == QR_DIRECT3D11)?0.5:0;
#else
#define dxbias 0
#endif
	if (charflags & CON_HIDDEN)
		return px;

	if (charflags & CON_2NDCHARSETTEXT)
	{
		if (font->alt)
		{
			font = font->alt;
//			charflags &= ~CON_2NDCHARSETTEXT;
		}
		else if ((codepoint) >= 0xe000 && (codepoint) <= 0xe0ff)
			charflags &= ~CON_2NDCHARSETTEXT;	//don't double-dip
	}

	//crash if there is no current font.
	c = Font_GetChar(font, codepoint);
	if (!c)
		return px;

	nextx = px + c->advance;

	if (codepoint == '\t')
		return Font_TabWidth(px);
	if (codepoint == ' ' && (charflags & (CON_RICHFORECOLOUR|CON_NONCLEARBG)) != CON_NONCLEARBG)
		return nextx;

/*	if (charcode & CON_BLINKTEXT)
	{
		if (!cl_noblink.ival)
			if ((int)(realtime*3) & 1)
				return nextx;
	}
*/
	if (charflags & CON_RICHFORECOLOUR)
	{
		col = charflags & (CON_2NDCHARSETTEXT|CON_BLINKTEXT|CON_RICHFORECOLOUR|(0xfff<<CON_RICHBSHIFT));
		if (col != font_colourmask)
		{
			vec4_t rgba;
			if (font_colourmask & CON_NONCLEARBG)
			{
				Font_Flush();
				R2D_Flush = Font_Flush;
			}
			font_colourmask = col;

			rgba[0] = ((col>>CON_RICHRSHIFT)&0xf)*0x11;
			rgba[1] = ((col>>CON_RICHGSHIFT)&0xf)*0x11;
			rgba[2] = ((col>>CON_RICHBSHIFT)&0xf)*0x11;
			rgba[3] = 255;

			font_backcolour.c = 0;
			if (charflags & CON_2NDCHARSETTEXT)
			{
				rgba[0] *= font->alttint[0];
				rgba[1] *= font->alttint[1];
				rgba[2] *= font->alttint[2];
			}
			else
			{
				rgba[0] *= font->tint[0];
				rgba[1] *= font->tint[1];
				rgba[2] *= font->tint[2];
			}
			rgba[0] *= font_foretint[0];
			rgba[1] *= font_foretint[1];
			rgba[2] *= font_foretint[2];
			rgba[3] *= font_foretint[3];
			if (charflags & CON_BLINKTEXT)
			{
				float a = (sin(realtime*3)+1)*0.4 + 0.2;
				Vector4Scale(rgba, a, rgba);
			}
			font_forecolour.rgba[0] = min(rgba[0], 255);
			font_forecolour.rgba[1] = min(rgba[1], 255);
			font_forecolour.rgba[2] = min(rgba[2], 255);
			font_forecolour.rgba[3] = min(rgba[3], 255);
		}
	}
	else
	{
		col = charflags & (CON_2NDCHARSETTEXT|CON_NONCLEARBG|CON_BGMASK|CON_FGMASK|CON_HALFALPHA|CON_BLINKTEXT);
		if (col != font_colourmask)
		{
			vec4_t rgba;
			if ((col ^ font_colourmask) & CON_NONCLEARBG)
			{
				Font_Flush();
				R2D_Flush = Font_Flush;
			}
			font_colourmask = col;

			col = (charflags&CON_FGMASK)>>CON_FGSHIFT;
			if(charflags & CON_HALFALPHA)
			{
				rgba[0] = consolecolours[col].fr*0x7f;
				rgba[1] = consolecolours[col].fg*0x7f;
				rgba[2] = consolecolours[col].fb*0x7f;
				rgba[3] = 0x7f;
			}
			else
			{
				rgba[0] = consolecolours[col].fr*255;
				rgba[1] = consolecolours[col].fg*255;
				rgba[2] = consolecolours[col].fb*255;
				rgba[3] = 255;
			}

			if (vid.flags&VID_SRGBAWARE)
			{
				rgba[0] = M_SRGBToLinear(rgba[0], 255);
				rgba[1] = M_SRGBToLinear(rgba[1], 255);
				rgba[2] = M_SRGBToLinear(rgba[2], 255);
			}

			col = (charflags&CON_BGMASK)>>CON_BGSHIFT;
			if (charflags & CON_NONCLEARBG)
			{
				font_backcolour.rgba[0] = consolecolours[col].fr*255;
				font_backcolour.rgba[1] = consolecolours[col].fg*255;
				font_backcolour.rgba[2] = consolecolours[col].fb*255;
				font_backcolour.rgba[3] = (charflags & CON_NONCLEARBG)?0xc0:0;
			}
			else
				font_backcolour.c = 0;

			if (charflags & CON_2NDCHARSETTEXT)
			{
				rgba[0] *= font->alttint[0];
				rgba[1] *= font->alttint[1];
				rgba[2] *= font->alttint[2];
			}
			else
			{
				rgba[0] *= font->tint[0];
				rgba[1] *= font->tint[1];
				rgba[2] *= font->tint[2];
			}
			rgba[0] *= font_foretint[0];
			rgba[1] *= font_foretint[1];
			rgba[2] *= font_foretint[2];
			rgba[3] *= font_foretint[3];
			if (charflags & CON_BLINKTEXT)
			{
				float a = (sin(realtime*3)+1)*0.4 + 0.2;
				Vector4Scale(rgba, a, rgba);
			}
			font_forecolour.rgba[0] = min(rgba[0], 255);
			font_forecolour.rgba[1] = min(rgba[1], 255);
			font_forecolour.rgba[2] = min(rgba[2], 255);
			font_forecolour.rgba[3] = min(rgba[3], 255);
		}
	}

	s0 = (float)c->bmx/PLANEWIDTH;
	t0 = (float)c->bmy/PLANEWIDTH;
	s1 = (float)(c->bmx+c->bmw)/PLANEWIDTH;
	t1 = (float)(c->bmy+c->bmh)/PLANEWIDTH;

	switch(c->texplane)
	{
	case TRACKERIMAGE:
		s0 = t0 = 0;
		s1 = t1 = 1;
		sx = ((px+c->left + dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+c->top + dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = (c->advance*vid.width) / (float)vid.rotpixelwidth;
		sh = (font->charheight*vid.height) / (float)vid.rotpixelheight;
		v = Font_BeginChar(fontplanes.trackerimage);
		break;
	case DEFAULTPLANE:
		sx = ((px+c->left + dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+c->top + dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = ((c->advance)*vid.width) / (float)vid.rotpixelwidth;
		sh = ((font->charheight)*vid.height) / (float)vid.rotpixelheight;
		v = Font_BeginChar(fontplanes.defaultfont);
		break;
	case BITMAPPLANE:
		sx = ((px+c->left + dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+c->top + dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = ((c->advance)*vid.width) / (float)vid.rotpixelwidth;
		sh = ((font->charheight)*vid.height) / (float)vid.rotpixelheight;
		v = Font_BeginChar(font->singletexture);
		break;
	case SINGLEPLANE:
		sx = ((px+c->left + dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+c->top + dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = ((c->bmw)*vid.width) / (float)vid.rotpixelwidth;
		sh = ((c->bmh)*vid.height) / (float)vid.rotpixelheight;
		v = Font_BeginChar(font->singletexture);
		break;
	default:
		sx = ((px+c->left + dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+c->top + dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = ((c->bmw)*vid.width) / (float)vid.rotpixelwidth;
		sh = ((c->bmh)*vid.height) / (float)vid.rotpixelheight;
		v = Font_BeginChar(fontplanes.texnum[c->texplane]);
		break;
	}

	font_texcoord[v+0][0] = s0;
	font_texcoord[v+0][1] = t0;
	font_texcoord[v+1][0] = s1;
	font_texcoord[v+1][1] = t0;
	font_texcoord[v+2][0] = s1;
	font_texcoord[v+2][1] = t1;
	font_texcoord[v+3][0] = s0;
	font_texcoord[v+3][1] = t1;

	font_coord[v+0][0] = sx;
	font_coord[v+0][1] = sy;
	font_coord[v+1][0] = sx+sw;
	font_coord[v+1][1] = sy;
	font_coord[v+2][0] = sx+sw;
	font_coord[v+2][1] = sy+sh;
	font_coord[v+3][0] = sx;
	font_coord[v+3][1] = sy+sh;

	if (c->flags&CHARF_FORCEWHITE)
	{
		font_forecoloura[v+0].c =
		font_forecoloura[v+1].c =
		font_forecoloura[v+2].c =
		font_forecoloura[v+3].c = 0xffffffff;
	}
	else
	{
		font_forecoloura[v+0] =
		font_forecoloura[v+1] =
		font_forecoloura[v+2] =
		font_forecoloura[v+3] = font_forecolour;
	}

	if (font_colourmask & CON_NONCLEARBG)
	{
		sx = ((px+dxbias)*(int)vid.width) / (float)vid.rotpixelwidth;
		sy = ((py+dxbias)*(int)vid.height) / (float)vid.rotpixelheight;
		sw = sx + ((c->advance)*vid.width) / (float)vid.rotpixelwidth;
		sh = sy + ((font->charheight)*vid.height) / (float)vid.rotpixelheight;

		//don't care about texcoords
		font_backcoord[v+0][0] = sx;
		font_backcoord[v+0][1] = sy;
		font_backcoord[v+1][0] = sw;
		font_backcoord[v+1][1] = sy;
		font_backcoord[v+2][0] = sw;
		font_backcoord[v+2][1] = sh;
		font_backcoord[v+3][0] = sx;
		font_backcoord[v+3][1] = sh;

		font_backcoloura[v+0] = font_backcolour;
		font_backcoloura[v+1] = font_backcolour;
		font_backcoloura[v+2] = font_backcolour;
		font_backcoloura[v+3] = font_backcolour;
	}

	return nextx;
}

/*there is no sane way to make this pixel-correct*/
float Font_DrawScaleChar(float px, float py, unsigned int charflags, unsigned int codepoint)
{
	struct charcache_s *c;
	float s0, s1;
	float t0, t1;
	float nextx;
	float sx, sy, sw, sh;
	int col;
	int v;
	struct font_s *font = curfont;
	float cw, ch;
#ifdef D3D11QUAKE
	float dxbias = 0;//(qrenderer == QR_DIRECT3D11)?0.5:0;
#else
#define dxbias 0
#endif

//	if (!curfont_scaled)
//		return Font_DrawChar(px, py, charcode);

	if (charflags & CON_2NDCHARSETTEXT)
	{
		if (font->alt)
		{
			font = font->alt;
			charflags &= ~CON_2NDCHARSETTEXT;
		}
		else if (codepoint >= 0xe000 && codepoint <= 0xe0ff)
			charflags &= ~CON_2NDCHARSETTEXT;	//don't double-dip
	}

	cw = curfont_scale[0];
	ch = curfont_scale[1];

	//crash if there is no current font.
	c = Font_GetChar(font, codepoint);
	if (!c)
		return px;

	nextx = px + c->advance*cw;

	if (codepoint == ' ' && (charflags & (CON_RICHFORECOLOUR|CON_NONCLEARBG)) != CON_NONCLEARBG)
		return nextx;

	if (charflags & CON_BLINKTEXT)
	{
		if (!cl_noblink.ival)
			if ((int)(realtime*3) & 1)
				return nextx;
	}

	if (charflags & CON_RICHFORECOLOUR)
	{
		col = charflags & (CON_2NDCHARSETTEXT|CON_RICHFORECOLOUR|(0xfff<<CON_RICHBSHIFT));
		if (col != font_colourmask)
		{
			vec4_t rgba;
			if (font_backcolour.rgba[3])
			{
				Font_Flush();
				R2D_Flush = Font_Flush;
			}
			font_colourmask = col;

			rgba[0] = ((col>>CON_RICHRSHIFT)&0xf)*0x11;
			rgba[1] = ((col>>CON_RICHGSHIFT)&0xf)*0x11;
			rgba[2] = ((col>>CON_RICHBSHIFT)&0xf)*0x11;
			rgba[3] = 255;

			font_backcolour.c = 0;

			if (charflags & CON_2NDCHARSETTEXT)
			{
				rgba[0] *= font->alttint[0];
				rgba[1] *= font->alttint[1];
				rgba[2] *= font->alttint[2];
			}
			else
			{
				rgba[0] *= font->tint[0];
				rgba[1] *= font->tint[1];
				rgba[2] *= font->tint[2];
			}
			rgba[0] *= font_foretint[0];
			rgba[1] *= font_foretint[1];
			rgba[2] *= font_foretint[2];
			rgba[3] *= font_foretint[3];
			font_forecolour.rgba[0] = min(rgba[0], 255);
			font_forecolour.rgba[1] = min(rgba[1], 255);
			font_forecolour.rgba[2] = min(rgba[2], 255);
			font_forecolour.rgba[3] = min(rgba[3], 255);
		}
	}
	else
	{
		col = charflags & (CON_2NDCHARSETTEXT|CON_NONCLEARBG|CON_BGMASK|CON_FGMASK|CON_HALFALPHA);
		if (col != font_colourmask)
		{
			vec4_t rgba;
			if (font_backcolour.rgba[3] != ((charflags & CON_NONCLEARBG)?127:0))
			{
				Font_Flush();
				R2D_Flush = Font_Flush;
			}
			font_colourmask = col;

			col = (charflags&CON_FGMASK)>>CON_FGSHIFT;
			if (charflags & CON_HALFALPHA)
			{
				rgba[0] = consolecolours[col].fr*0x7f;
				rgba[1] = consolecolours[col].fg*0x7f;
				rgba[2] = consolecolours[col].fb*0x7f;
				rgba[3] = 0x7f;
			}
			else
			{
				rgba[0] = consolecolours[col].fr*255;
				rgba[1] = consolecolours[col].fg*255;
				rgba[2] = consolecolours[col].fb*255;
				rgba[3] = 255;
			}

			col = (charflags&CON_BGMASK)>>CON_BGSHIFT;
			if (charflags & CON_NONCLEARBG)
			{
				font_backcolour.rgba[0] = consolecolours[col].fr*0xc0;
				font_backcolour.rgba[1] = consolecolours[col].fg*0xc0;
				font_backcolour.rgba[2] = consolecolours[col].fb*0xc0;
				font_backcolour.rgba[3] = 0xc0;
			}
			else
				font_backcolour.c = 0;

			if (charflags & CON_2NDCHARSETTEXT)
			{
				rgba[0] *= font->alttint[0];
				rgba[1] *= font->alttint[1];
				rgba[2] *= font->alttint[2];
			}
			else
			{
				rgba[0] *= font->tint[0];
				rgba[1] *= font->tint[1];
				rgba[2] *= font->tint[2];
			}
			rgba[0] *= font_foretint[0];
			rgba[1] *= font_foretint[1];
			rgba[2] *= font_foretint[2];
			rgba[3] *= font_foretint[3];
			font_forecolour.rgba[0] = min(rgba[0], 255);
			font_forecolour.rgba[1] = min(rgba[1], 255);
			font_forecolour.rgba[2] = min(rgba[2], 255);
			font_forecolour.rgba[3] = min(rgba[3], 255);
		}
	}

	s0 = (float)c->bmx/PLANEWIDTH;
	t0 = (float)c->bmy/PLANEWIDTH;
	s1 = (float)(c->bmx+c->bmw)/PLANEWIDTH;
	t1 = (float)(c->bmy+c->bmh)/PLANEWIDTH;

	if (c->texplane >= DEFAULTPLANE)
	{
		sx = ((px+c->left*cw));
		sy = ((py+c->top*ch));
		sw = ((font->charheight*cw));
		sh = ((font->charheight*ch));

		if (c->texplane == DEFAULTPLANE)
			v = Font_BeginChar(fontplanes.defaultfont);
		else
			v = Font_BeginChar(font->singletexture);
	}
	else
	{
		sx = (px+c->left*cw);
		sy = (py+c->top*ch);
		sw = ((c->bmw*cw));
		sh = ((c->bmh*ch));
		v = Font_BeginChar(fontplanes.texnum[c->texplane]);
	}

	sx += dxbias;
	sy += dxbias;

	sx *= (int)vid.width / (float)vid.rotpixelwidth;
	sy *= (int)vid.height / (float)vid.rotpixelheight;
	sw *= (int)vid.width / (float)vid.rotpixelwidth;
	sh *= (int)vid.height / (float)vid.rotpixelheight;

	font_texcoord[v+0][0] = s0;
	font_texcoord[v+0][1] = t0;
	font_texcoord[v+1][0] = s1;
	font_texcoord[v+1][1] = t0;
	font_texcoord[v+2][0] = s1;
	font_texcoord[v+2][1] = t1;
	font_texcoord[v+3][0] = s0;
	font_texcoord[v+3][1] = t1;

	font_coord[v+0][0] = sx;
	font_coord[v+0][1] = sy;
	font_coord[v+1][0] = sx+sw;
	font_coord[v+1][1] = sy;
	font_coord[v+2][0] = sx+sw;
	font_coord[v+2][1] = sy+sh;
	font_coord[v+3][0] = sx;
	font_coord[v+3][1] = sy+sh;

	font_forecoloura[v+0] = font_forecolour;
	font_forecoloura[v+1] = font_forecolour;
	font_forecoloura[v+2] = font_forecolour;
	font_forecoloura[v+3] = font_forecolour;

	if (font_colourmask & CON_NONCLEARBG)
	{
		sx = px + dxbias;
		sy = py + dxbias;
		sw = sx + c->advance;
		sh = sy + font->charheight;

		sx *= (int)vid.width / (float)vid.rotpixelwidth;
		sy *= (int)vid.height / (float)vid.rotpixelheight;
		sw *= (int)vid.width / (float)vid.rotpixelwidth;
		sh *= (int)vid.height / (float)vid.rotpixelheight;

		//don't care about texcoords
		font_backcoord[v+0][0] = sx;
		font_backcoord[v+0][1] = sy;
		font_backcoord[v+1][0] = sw;
		font_backcoord[v+1][1] = sy;
		font_backcoord[v+2][0] = sw;
		font_backcoord[v+2][1] = sh;
		font_backcoord[v+3][0] = sx;
		font_backcoord[v+3][1] = sh;

		font_backcoloura[v+0] = font_backcolour;
		font_backcoloura[v+1] = font_backcolour;
		font_backcoloura[v+2] = font_backcolour;
		font_backcoloura[v+3] = font_backcolour;
	}

	return nextx;
}

#endif	//!SERVERONLY
