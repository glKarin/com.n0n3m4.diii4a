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

#include "quakedef.h"
#include "image.h"
#include "wad.h"

#include "cl_video.h"

#include "ft2.h"
#include "ft2_fontdefs.h"

struct cachepic_s
{
	// size of pic
	int width, height;
	// this flag indicates that it should be loaded and unloaded on demand
	int autoload;
	// texture flags to upload with
	int texflags;
	// texture may be freed after a while
	int lastusedframe;
	// renderable texture
	skinframe_t *skinframe;
	// used for hash lookups
	struct cachepic_s *chain;
	// flags - CACHEPICFLAG_NEWPIC for example
	unsigned int flags;
	// name of pic
	char name[MAX_QPATH];
};

dp_fonts_t dp_fonts;
static mempool_t *fonts_mempool = NULL;

cvar_t r_textshadow = {CF_CLIENT | CF_ARCHIVE, "r_textshadow", "0", "draws a shadow on all text to improve readability (note: value controls offset, 1 = 1 pixel, 1.5 = 1.5 pixels, etc)"};
// these are also read by the dedicated server when sys_colortranslation > 1
cvar_t r_textbrightness = {CF_SHARED | CF_ARCHIVE, "r_textbrightness", "0", "additional brightness for text color codes (0 keeps colors as is, 1 makes them all white)"};
cvar_t r_textcontrast = {CF_SHARED | CF_ARCHIVE, "r_textcontrast", "1", "additional contrast for text color codes (1 keeps colors as is, 0 makes them all black)"};

cvar_t r_font_postprocess_blur = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_blur", "0", "font blur amount"};
cvar_t r_font_postprocess_outline = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_outline", "0", "font outline amount"};
cvar_t r_font_postprocess_shadow_x = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_x", "0", "font shadow X shift amount, applied during outlining"};
cvar_t r_font_postprocess_shadow_y = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_y", "0", "font shadow Y shift amount, applied during outlining"};
cvar_t r_font_postprocess_shadow_z = {CF_CLIENT | CF_ARCHIVE, "r_font_postprocess_shadow_z", "0", "font shadow Z shift amount, applied during blurring"};
cvar_t r_font_hinting = {CF_CLIENT | CF_ARCHIVE, "r_font_hinting", "3", "0 = no hinting, 1 = light autohinting, 2 = full autohinting, 3 = full hinting"};
cvar_t r_font_antialias = {CF_CLIENT | CF_ARCHIVE, "r_font_antialias", "1", "0 = monochrome, 1 = grey" /* , 2 = rgb, 3 = bgr" */};
cvar_t r_font_always_reload = {CF_CLIENT | CF_ARCHIVE, "r_font_always_reload", "0", "reload a font even given the same loadfont command. useful for trying out different versions of the same font file"};
cvar_t r_nearest_2d = {CF_CLIENT | CF_ARCHIVE, "r_nearest_2d", "0", "use nearest filtering on all 2d textures (including conchars)"};
cvar_t r_nearest_conchars = {CF_CLIENT | CF_ARCHIVE, "r_nearest_conchars", "0", "use nearest filtering on conchars texture"};

//=============================================================================
/* Support Routines */

static cachepic_t *cachepichash[CACHEPICHASHSIZE];
static cachepic_t cachepics[MAX_CACHED_PICS];
static int numcachepics;

rtexturepool_t *drawtexturepool;

int draw_frame = 1;

/*
================
Draw_CachePic
================
*/
// FIXME: move this to client somehow
cachepic_t *Draw_CachePic_Flags(const char *path, unsigned int cachepicflags)
{
	int crc, hashkey;
	cachepic_t *pic;
	int texflags;

	texflags = TEXF_ALPHA;
	if (!(cachepicflags & CACHEPICFLAG_NOCLAMP))
		texflags |= TEXF_CLAMP;
	if (cachepicflags & CACHEPICFLAG_MIPMAP)
		texflags |= TEXF_MIPMAP;
	if (!(cachepicflags & CACHEPICFLAG_NOCOMPRESSION) && gl_texturecompression_2d.integer && gl_texturecompression.integer)
		texflags |= TEXF_COMPRESS;
	if (cachepicflags & CACHEPICFLAG_LINEAR)
		texflags |= TEXF_FORCELINEAR;
	else if ((cachepicflags & CACHEPICFLAG_NEAREST) || r_nearest_2d.integer)
		texflags |= TEXF_FORCENEAREST;

	// check whether the picture has already been cached
	crc = CRC_Block((unsigned char *)path, strlen(path));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE;
	for (pic = cachepichash[hashkey];pic;pic = pic->chain)
	{
		if (!strcmp(path, pic->name))
		{
			// if it was created (or replaced) by Draw_NewPic, just return it
			if (!(pic->flags & CACHEPICFLAG_NEWPIC))
			{
				// reload the pic if texflags changed in important ways
				// ignore TEXF_COMPRESS when comparing, because fallback pics remove the flag, and ignore TEXF_MIPMAP because QC specifies that
				if ((pic->texflags ^ texflags) & ~(TEXF_COMPRESS | TEXF_MIPMAP))
				{
					Con_DPrintf("Draw_CachePic(\"%s\"): frame %i: reloading pic due to mismatch on flags\n", path, draw_frame);
					goto reload;
				}
				if (!pic->skinframe || !pic->skinframe->base)
				{
					if (pic->flags & CACHEPICFLAG_FAILONMISSING)
						return NULL;
					Con_DPrintf("Draw_CachePic(\"%s\"): frame %i: reloading pic\n", path, draw_frame);
					goto reload;
				}
				if (!(cachepicflags & CACHEPICFLAG_NOTPERSISTENT))
					pic->autoload = false; // caller is making this pic persistent
			}
			if (pic->skinframe)
				R_SkinFrame_MarkUsed(pic->skinframe);
			pic->lastusedframe = draw_frame;
			return pic;
		}
	}

	if (numcachepics == MAX_CACHED_PICS)
	{
		Con_DPrintf ("Draw_CachePic(\"%s\"): frame %i: numcachepics == MAX_CACHED_PICS\n", path, draw_frame);
		// FIXME: support NULL in callers?
		return cachepics; // return the first one
	}
	Con_DPrintf("Draw_CachePic(\"%s\"): frame %i: loading pic%s\n", path, draw_frame, (cachepicflags & CACHEPICFLAG_NOTPERSISTENT) ? " notpersist" : "");
	pic = cachepics + (numcachepics++);
	memset(pic, 0, sizeof(*pic));
	dp_strlcpy (pic->name, path, sizeof(pic->name));
	// link into list
	pic->chain = cachepichash[hashkey];
	cachepichash[hashkey] = pic;

reload:
	if (pic->skinframe)
		R_SkinFrame_PurgeSkinFrame(pic->skinframe);

	pic->flags = cachepicflags;
	pic->texflags = texflags;
	pic->autoload = (cachepicflags & CACHEPICFLAG_NOTPERSISTENT) != 0;
	pic->lastusedframe = draw_frame;

	if (pic->skinframe)
	{
		// reload image after it was unloaded or texflags changed significantly
		R_SkinFrame_LoadExternal_SkinFrame(pic->skinframe, pic->name, texflags | TEXF_FORCE_RELOAD, (cachepicflags & CACHEPICFLAG_QUIET) == 0, (cachepicflags & CACHEPICFLAG_FAILONMISSING) == 0);
	}
	else
	{
		// load high quality image (this falls back to low quality too)
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, texflags | TEXF_FORCE_RELOAD, (cachepicflags & CACHEPICFLAG_QUIET) == 0, (cachepicflags & CACHEPICFLAG_FAILONMISSING) == 0);
	}

	// get the dimensions of the image we loaded (if it was successful)
	if (pic->skinframe && pic->skinframe->base)
	{
		pic->width = R_TextureWidth(pic->skinframe->base);
		pic->height = R_TextureHeight(pic->skinframe->base);
	}

	// check for a low quality version of the pic and use its size if possible, to match the stock hud
	Image_GetStockPicSize(pic->name, &pic->width, &pic->height);

	return pic;
}

cachepic_t *Draw_CachePic (const char *path)
{
	return Draw_CachePic_Flags (path, 0); // default to persistent!
}

const char *Draw_GetPicName(cachepic_t *pic)
{
	if (pic == NULL)
		return "";
	return pic->name;
}

int Draw_GetPicWidth(cachepic_t *pic)
{
	if (pic == NULL)
		return 0;
	return pic->width;
}

int Draw_GetPicHeight(cachepic_t *pic)
{
	if (pic == NULL)
		return 0;
	return pic->height;
}

qbool Draw_IsPicLoaded(cachepic_t *pic)
{
	if (pic == NULL)
		return false;
	if (pic->autoload && (!pic->skinframe || !pic->skinframe->base))
	{
		Con_DPrintf("Draw_IsPicLoaded(\"%s\"): Loading external skin\n", pic->name);
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, pic->texflags | TEXF_FORCE_RELOAD, false, true);
	}
	// skinframe will only be NULL if the pic was created with CACHEPICFLAG_FAILONMISSING and not found
	return pic->skinframe != NULL && pic->skinframe->base != NULL;
}

rtexture_t *Draw_GetPicTexture(cachepic_t *pic)
{
	if (pic == NULL)
		return NULL;
	if (pic->autoload && (!pic->skinframe || !pic->skinframe->base))
	{
		Con_DPrintf("Draw_GetPicTexture(\"%s\"): Loading external skin\n", pic->name);
		pic->skinframe = R_SkinFrame_LoadExternal(pic->name, pic->texflags | TEXF_FORCE_RELOAD, false, true);
	}
	pic->lastusedframe = draw_frame;
	return pic->skinframe ? pic->skinframe->base : NULL;
}

void Draw_Frame(void)
{
	int i;
	cachepic_t *pic;
	static double nextpurgetime;
	if (nextpurgetime > host.realtime)
		return;
	nextpurgetime = host.realtime + 0.05;
	for (i = 0, pic = cachepics;i < numcachepics;i++, pic++)
	{
		if (pic->autoload && pic->skinframe && pic->skinframe->base && pic->lastusedframe < draw_frame - 3)
		{
			Con_DPrintf("Draw_Frame(%i): Unloading \"%s\"\n", draw_frame, pic->name);
			R_SkinFrame_PurgeSkinFrame(pic->skinframe);
		}
	}
	draw_frame++;
}

cachepic_t *Draw_NewPic(const char *picname, int width, int height, unsigned char *pixels_bgra, textype_t textype, int texflags)
{
	int crc, hashkey;
	cachepic_t *pic;

	crc = CRC_Block((unsigned char *)picname, strlen(picname));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE;
	for (pic = cachepichash[hashkey];pic;pic = pic->chain)
		if (!strcmp (picname, pic->name))
			break;

	if (pic)
	{
		if (pic->flags & CACHEPICFLAG_NEWPIC && pic->skinframe && pic->skinframe->base && pic->width == width && pic->height == height)
		{
			Con_DPrintf("Draw_NewPic(\"%s\"): frame %i: updating texture\n", picname, draw_frame);
			R_UpdateTexture(pic->skinframe->base, pixels_bgra, 0, 0, 0, width, height, 1, 0);
			R_SkinFrame_MarkUsed(pic->skinframe);
			pic->lastusedframe = draw_frame;
			return pic;
		}
		Con_DPrintf("Draw_NewPic(\"%s\"): frame %i: reloading pic because flags/size changed\n", picname, draw_frame);
	}
	else
	{
		if (numcachepics == MAX_CACHED_PICS)
		{
			Con_DPrintf ("Draw_NewPic(\"%s\"): frame %i: numcachepics == MAX_CACHED_PICS\n", picname, draw_frame);
			// FIXME: support NULL in callers?
			return cachepics; // return the first one
		}
		Con_DPrintf("Draw_NewPic(\"%s\"): frame %i: creating new cachepic\n", picname, draw_frame);
		pic = cachepics + (numcachepics++);
		memset(pic, 0, sizeof(*pic));
		dp_strlcpy (pic->name, picname, sizeof(pic->name));
		// link into list
		pic->chain = cachepichash[hashkey];
		cachepichash[hashkey] = pic;
	}

	R_SkinFrame_PurgeSkinFrame(pic->skinframe);

	pic->autoload = false;
	pic->flags = CACHEPICFLAG_NEWPIC; // disable texflags checks in Draw_CachePic
	pic->flags |= (texflags & TEXF_CLAMP) ? 0 : CACHEPICFLAG_NOCLAMP;
	pic->flags |= (texflags & TEXF_FORCENEAREST) ? CACHEPICFLAG_NEAREST : 0;
	pic->width = width;
	pic->height = height;
	pic->skinframe = R_SkinFrame_LoadInternalBGRA(picname, texflags | TEXF_FORCE_RELOAD, pixels_bgra, width, height, 0, 0, 0, vid.sRGB2D);
	pic->lastusedframe = draw_frame;
	return pic;
}

void Draw_FreePic(const char *picname)
{
	int crc;
	int hashkey;
	cachepic_t *pic;
	// this doesn't really free the pic, but does free its texture
	crc = CRC_Block((unsigned char *)picname, strlen(picname));
	hashkey = ((crc >> 8) ^ crc) % CACHEPICHASHSIZE;
	for (pic = cachepichash[hashkey];pic;pic = pic->chain)
	{
		if (!strcmp (picname, pic->name) && pic->skinframe)
		{
			Con_DPrintf("Draw_FreePic(\"%s\"): frame %i: freeing pic\n", picname, draw_frame);
			R_SkinFrame_PurgeSkinFrame(pic->skinframe);
			return;
		}
	}
}

static float snap_to_pixel_x(float x, float roundUpAt);
extern int con_linewidth; // to force rewrapping
void LoadFont(qbool override, const char *name, dp_font_t *fnt, float scale, float voffset)
{
	int i, ch;
	float maxwidth;
	char widthfile[MAX_QPATH];
	char *widthbuf;
	fs_offset_t widthbufsize;

	if(override || !fnt->texpath[0])
	{
		dp_strlcpy(fnt->texpath, name, sizeof(fnt->texpath));
		// load the cvars when the font is FIRST loader
		fnt->settings.scale = scale;
		fnt->settings.voffset = voffset;
		fnt->settings.antialias = r_font_antialias.integer;
		fnt->settings.hinting = r_font_hinting.integer;
		fnt->settings.outline = r_font_postprocess_outline.value;
		fnt->settings.blur = r_font_postprocess_blur.value;
		fnt->settings.shadowx = r_font_postprocess_shadow_x.value;
		fnt->settings.shadowy = r_font_postprocess_shadow_y.value;
		fnt->settings.shadowz = r_font_postprocess_shadow_z.value;
	}

	// fix bad scale
	if (fnt->settings.scale <= 0)
		fnt->settings.scale = 1;

	if(drawtexturepool == NULL)
		return; // before gl_draw_start, so will be loaded later

	if(fnt->ft2)
	{
		// we are going to reload. clear old ft2 data
		Font_UnloadFont(fnt->ft2);
		Mem_Free(fnt->ft2);
		fnt->ft2 = NULL;
	}

	if(fnt->req_face != -1)
	{
		if(!Font_LoadFont(fnt->texpath, fnt))
			Con_DPrintf("Failed to load font-file for '%s', it will not support as many characters.\n", fnt->texpath);
	}

	fnt->pic = Draw_CachePic_Flags(fnt->texpath, CACHEPICFLAG_QUIET | CACHEPICFLAG_NOCOMPRESSION | (r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0) | CACHEPICFLAG_FAILONMISSING);
	if(!Draw_IsPicLoaded(fnt->pic))
	{
		for (i = 0; i < MAX_FONT_FALLBACKS; ++i)
		{
			if (!fnt->fallbacks[i][0])
				break;
			fnt->pic = Draw_CachePic_Flags(fnt->fallbacks[i], CACHEPICFLAG_QUIET | CACHEPICFLAG_NOCOMPRESSION | (r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0) | CACHEPICFLAG_FAILONMISSING);
			if(Draw_IsPicLoaded(fnt->pic))
				break;
		}
		if(!Draw_IsPicLoaded(fnt->pic))
		{
			fnt->pic = Draw_CachePic_Flags("gfx/conchars", CACHEPICFLAG_NOCOMPRESSION | (r_nearest_conchars.integer ? CACHEPICFLAG_NEAREST : 0));
			dp_strlcpy(widthfile, "gfx/conchars.width", sizeof(widthfile));
		}
		else
			dpsnprintf(widthfile, sizeof(widthfile), "%s.width", fnt->fallbacks[i]);
	}
	else
		dpsnprintf(widthfile, sizeof(widthfile), "%s.width", fnt->texpath);

	// unspecified width == 1 (base width)
	for(ch = 0; ch < 256; ++ch)
		fnt->width_of[ch] = 1;

	// FIXME load "name.width", if it fails, fill all with 1
	if((widthbuf = (char *) FS_LoadFile(widthfile, tempmempool, true, &widthbufsize)))
	{
		float extraspacing = 0;
		const char *p = widthbuf;

		ch = 0;
		while(ch < 256)
		{
			if(!COM_ParseToken_Simple(&p, false, false, true))
				return;

			switch(*com_token)
			{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '+':
				case '-':
				case '.':
					fnt->width_of[ch] = atof(com_token) + extraspacing;
					ch++;
					break;
				default:
					if(!strcmp(com_token, "extraspacing"))
					{
						if(!COM_ParseToken_Simple(&p, false, false, true))
							return;
						extraspacing = atof(com_token);
					}
					else if(!strcmp(com_token, "scale"))
					{
						if(!COM_ParseToken_Simple(&p, false, false, true))
							return;
						fnt->settings.scale = atof(com_token);
					}
					else
					{
						Con_DPrintf("Warning: skipped unknown font property %s\n", com_token);
						if(!COM_ParseToken_Simple(&p, false, false, true))
							return;
					}
					break;
			}
		}

		Mem_Free(widthbuf);
	}

	if(fnt->ft2)
	{
		for (i = 0; i < MAX_FONT_SIZES; ++i)
		{
			ft2_font_map_t *map = Font_MapForIndex(fnt->ft2, i);
			if (!map)
				break;
			for(ch = 0; ch < 256; ++ch)
				fnt->width_of_ft2[i][ch] = Font_SnapTo(fnt->width_of[ch], 1/map->size);
		}
	}

	maxwidth = fnt->width_of[0];
	for(i = 1; i < 256; ++i)
		maxwidth = max(maxwidth, fnt->width_of[i]);
	fnt->maxwidth = maxwidth;

	// fix up maxwidth for overlap
	fnt->maxwidth *= fnt->settings.scale;

	if(fnt == FONT_CONSOLE)
		con_linewidth = -1; // rewrap console in next frame
}

extern cvar_t developer_font;
dp_font_t *FindFont(const char *title, qbool allocate_new)
{
	int i, oldsize;

	// find font
	for(i = 0; i < dp_fonts.maxsize; ++i)
		if(!strcmp(dp_fonts.f[i].title, title))
			return &dp_fonts.f[i];
	// if not found - try allocate
	if (allocate_new)
	{
		// find any font with empty title
		for(i = 0; i < dp_fonts.maxsize; ++i)
		{
			if(!strcmp(dp_fonts.f[i].title, ""))
			{
				dp_strlcpy(dp_fonts.f[i].title, title, sizeof(dp_fonts.f[i].title));
				return &dp_fonts.f[i];
			}
		}
		// if no any 'free' fonts - expand buffer
		oldsize = dp_fonts.maxsize;
		dp_fonts.maxsize = dp_fonts.maxsize + FONTS_EXPAND;
		if (developer_font.integer)
			Con_Printf("FindFont: enlarging fonts buffer (%i -> %i)\n", oldsize, dp_fonts.maxsize);
		dp_fonts.f = (dp_font_t *)Mem_Realloc(fonts_mempool, dp_fonts.f, sizeof(dp_font_t) * dp_fonts.maxsize);
		// relink ft2 structures
		for(i = 0; i < oldsize; ++i)
			if (dp_fonts.f[i].ft2)
				dp_fonts.f[i].ft2->settings = &dp_fonts.f[i].settings;
		// register a font in first expanded slot
		dp_strlcpy(dp_fonts.f[oldsize].title, title, sizeof(dp_fonts.f[oldsize].title));
		return &dp_fonts.f[oldsize];
	}
	return NULL;
}

static float snap_to_pixel_x(float x, float roundUpAt)
{
	float pixelpos = x * vid.mode.width / vid_conwidth.value;
	int snap = (int) pixelpos;
	if (pixelpos - snap >= roundUpAt) ++snap;
	return ((float)snap * vid_conwidth.value / vid.mode.width);
	/*
	x = (int)(x * vid.width / vid_conwidth.value);
	x = (x * vid_conwidth.value / vid.width);
	return x;
	*/
}

static float snap_to_pixel_y(float y, float roundUpAt)
{
	float pixelpos = y * vid.mode.height / vid_conheight.value;
	int snap = (int) pixelpos;
	if (pixelpos - snap > roundUpAt) ++snap;
	return ((float)snap * vid_conheight.value / vid.mode.height);
	/*
	y = (int)(y * vid.height / vid_conheight.value);
	y = (y * vid_conheight.value / vid.height);
	return y;
	*/
}

static void LoadFont_f(cmd_state_t *cmd)
{
	dp_font_t *f;
	int i, sizes;
	const char *filelist, *c, *cm;
	float sz, scale, voffset;
	char mainfont[MAX_QPATH];

	if(Cmd_Argc(cmd) < 2)
	{
		Con_Printf("Available font commands:\n");
		for(i = 0; i < dp_fonts.maxsize; ++i)
			if (dp_fonts.f[i].title[0])
				Con_Printf("  loadfont %s gfx/tgafile[...] [custom switches] [sizes...]\n", dp_fonts.f[i].title);
		Con_Printf("A font can simply be gfx/tgafile, or alternatively you\n"
			   "can specify multiple fonts and faces\n"
			   "Like this: gfx/vera-sans:2,gfx/fallback:1\n"
			   "to load face 2 of the font gfx/vera-sans and use face 1\n"
			   "of gfx/fallback as fallback font.\n"
			   "You can also specify a list of font sizes to load, like this:\n"
			   "loadfont console gfx/conchars,gfx/fallback 8 12 16 24 32\n"
			   "In many cases, 8 12 16 24 32 should be a good choice.\n"
			   "custom switches:\n"
			   " scale x : scale all characters by this amount when rendering (doesnt change line height)\n"
			   " voffset x : offset all chars vertical when rendering, this is multiplied to character height\n"
			);
		return;
	}
	f = FindFont(Cmd_Argv(cmd, 1), true);
	if(f == NULL)
	{
		Con_Printf("font function not found\n");
		return;
	}
	else
	{
		if (strcmp(cmd->cmdline, f->cmdline) != 0 || r_font_always_reload.integer)
			dp_strlcpy(f->cmdline, cmd->cmdline, MAX_FONT_CMDLINE);
		else
		{
			Con_DPrintf("LoadFont: font %s is unchanged\n", Cmd_Argv(cmd, 1));
			return;
		}
	}

	if(Cmd_Argc(cmd) < 3)
		filelist = "gfx/conchars";
	else
		filelist = Cmd_Argv(cmd, 2);

	memset(f->fallbacks, 0, sizeof(f->fallbacks));
	memset(f->fallback_faces, 0, sizeof(f->fallback_faces));

	// first font is handled "normally"
	c = strchr(filelist, ':');
	cm = strchr(filelist, ',');
	if(c && (!cm || c < cm))
		f->req_face = atoi(c+1);
	else
	{
		f->req_face = 0;
		c = cm;
	}

	if(!c || (c - filelist) >= MAX_QPATH)
		dp_strlcpy(mainfont, filelist, sizeof(mainfont));
	else
	{
		memcpy(mainfont, filelist, c - filelist);
		mainfont[c - filelist] = 0;
	}

	for(i = 0; i < MAX_FONT_FALLBACKS; ++i)
	{
		c = strchr(filelist, ',');
		if(!c)
			break;
		filelist = c + 1;
		if(!*filelist)
			break;
		c = strchr(filelist, ':');
		cm = strchr(filelist, ',');
		if(c && (!cm || c < cm))
			f->fallback_faces[i] = atoi(c+1);
		else
		{
			f->fallback_faces[i] = 0; // f->req_face; could make it stick to the default-font's face index
			c = cm;
		}
		if(!c || (c-filelist) >= MAX_QPATH)
		{
			dp_strlcpy(f->fallbacks[i], filelist, sizeof(mainfont));
		}
		else
		{
			memcpy(f->fallbacks[i], filelist, c - filelist);
			f->fallbacks[i][c - filelist] = 0;
		}
	}

	// for now: by default load only one size: the default size
	f->req_sizes[0] = 0;
	for(i = 1; i < MAX_FONT_SIZES; ++i)
		f->req_sizes[i] = -1;

	scale = 1;
	voffset = 0;
	if(Cmd_Argc(cmd) >= 4)
	{
		for(sizes = 0, i = 3; i < Cmd_Argc(cmd); ++i)
		{
			// special switches
			if (!strcmp(Cmd_Argv(cmd, i), "scale"))
			{
				i++;
				if (i < Cmd_Argc(cmd))
					scale = atof(Cmd_Argv(cmd, i));
				continue;
			}
			if (!strcmp(Cmd_Argv(cmd, i), "voffset"))
			{
				i++;
				if (i < Cmd_Argc(cmd))
					voffset = atof(Cmd_Argv(cmd, i));
				continue;
			}

			if (sizes == -1)
				continue; // no slot for other sizes

			// parse one of sizes
			sz = atof(Cmd_Argv(cmd, i));
			if (sz > 0.001f && sz < 1000.0f) // do not use crap sizes
			{
				// search for duplicated sizes
				int j;
				for (j=0; j<sizes; j++)
					if (f->req_sizes[j] == sz)
						break;
				if (j != sizes)
					continue; // sz already in req_sizes, don't add it again

				if (sizes == MAX_FONT_SIZES)
				{
					Con_Printf(CON_WARN "Warning: specified more than %i different font sizes, exceding ones are ignored\n", MAX_FONT_SIZES);
					sizes = -1;
					continue;
				}
				f->req_sizes[sizes] = sz;
				sizes++;
			}
		}
	}

	LoadFont(true, mainfont, f, scale, voffset);
}

/*
===============
Draw_Init
===============
*/
static void gl_draw_start(void)
{
	int i;
	char vabuf[1024];
	drawtexturepool = R_AllocTexturePool();

	numcachepics = 0;
	memset(cachepichash, 0, sizeof(cachepichash));

	font_start();

	// load default font textures
	for(i = 0; i < dp_fonts.maxsize; ++i)
		if (dp_fonts.f[i].title[0])
			LoadFont(false, va(vabuf, sizeof(vabuf), "gfx/font_%s", dp_fonts.f[i].title), &dp_fonts.f[i], 1, 0);
}

static void gl_draw_shutdown(void)
{
	font_shutdown();

	R_FreeTexturePool(&drawtexturepool);

	numcachepics = 0;
	memset(cachepichash, 0, sizeof(cachepichash));
}

static void gl_draw_newmap(void)
{
	int i;
	font_newmap();

	// mark all of the persistent pics so they are not purged...
	for (i = 0; i < numcachepics; i++)
	{
		cachepic_t *pic = cachepics + i;
		if (!pic->autoload && pic->skinframe)
			R_SkinFrame_MarkUsed(pic->skinframe);
	}
}

void GL_Draw_Init (void)
{
	int i, j;

	Cvar_RegisterVariable(&r_font_postprocess_blur);
	Cvar_RegisterVariable(&r_font_postprocess_outline);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_x);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_y);
	Cvar_RegisterVariable(&r_font_postprocess_shadow_z);
	Cvar_RegisterVariable(&r_font_hinting);
	Cvar_RegisterVariable(&r_font_antialias);
	Cvar_RegisterVariable(&r_font_always_reload);
	Cvar_RegisterVariable(&r_textshadow);
	Cvar_RegisterVariable(&r_textbrightness);
	Cvar_RegisterVariable(&r_textcontrast);
	Cvar_RegisterVariable(&r_nearest_2d);
	Cvar_RegisterVariable(&r_nearest_conchars);

	// allocate fonts storage
	fonts_mempool = Mem_AllocPool("FONTS", 0, NULL);
	dp_fonts.maxsize = MAX_FONTS;
	dp_fonts.f = (dp_font_t *)Mem_Alloc(fonts_mempool, sizeof(dp_font_t) * dp_fonts.maxsize);
	memset(dp_fonts.f, 0, sizeof(dp_font_t) * dp_fonts.maxsize);

	// assign starting font names
	dp_strlcpy(FONT_DEFAULT->title, "default", sizeof(FONT_DEFAULT->title));
	dp_strlcpy(FONT_DEFAULT->texpath, "gfx/conchars", sizeof(FONT_DEFAULT->texpath));
	dp_strlcpy(FONT_CONSOLE->title, "console", sizeof(FONT_CONSOLE->title));
	dp_strlcpy(FONT_SBAR->title, "sbar", sizeof(FONT_SBAR->title));
	dp_strlcpy(FONT_NOTIFY->title, "notify", sizeof(FONT_NOTIFY->title));
	dp_strlcpy(FONT_CHAT->title, "chat", sizeof(FONT_CHAT->title));
	dp_strlcpy(FONT_CENTERPRINT->title, "centerprint", sizeof(FONT_CENTERPRINT->title));
	dp_strlcpy(FONT_INFOBAR->title, "infobar", sizeof(FONT_INFOBAR->title));
	dp_strlcpy(FONT_MENU->title, "menu", sizeof(FONT_MENU->title));
	for(i = 0, j = 0; i < MAX_USERFONTS; ++i)
		if(!FONT_USER(i)->title[0])
			dpsnprintf(FONT_USER(i)->title, sizeof(FONT_USER(i)->title), "user%d", j++);

	Cmd_AddCommand(CF_CLIENT, "loadfont", LoadFont_f, "loadfont function tganame loads a font; example: loadfont console gfx/veramono; loadfont without arguments lists the available functions");
	R_RegisterModule("GL_Draw", gl_draw_start, gl_draw_shutdown, gl_draw_newmap, NULL, NULL);
}

void DrawQ_Start(void)
{
	r_refdef.draw2dstage = 1;
	R_ResetViewRendering2D_Common(0, NULL, NULL, 0, 0, vid.mode.width, vid.mode.height, vid_conwidth.integer, vid_conheight.integer);
}

qbool r_draw2d_force = false;

void DrawQ_Pic(float x, float y, cachepic_t *pic, float width, float height, float red, float green, float blue, float alpha, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, pic->name, flags, pic->texflags, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x        , y         , 0, 0, 0, -1, 0, 0, 0, 0, red, green, blue, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y         , 0, 0, 0, -1, 1, 0, 0, 0, red, green, blue, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y + height, 0, 0, 0, -1, 1, 1, 0, 0, red, green, blue, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x        , y + height, 0, 0, 0, -1, 0, 1, 0, 0, red, green, blue, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_RotPic(float x, float y, cachepic_t *pic, float width, float height, float org_x, float org_y, float angle, float red, float green, float blue, float alpha, int flags)
{
	float af = DEG2RAD(-angle); // forward
	float ar = DEG2RAD(-angle + 90); // right
	float sinaf = sin(af);
	float cosaf = cos(af);
	float sinar = sin(ar);
	float cosar = cos(ar);
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, pic->name, flags, pic->texflags, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x - cosaf *          org_x  - cosar *           org_y , y - sinaf *          org_x  - sinar *           org_y , 0, 0, 0, -1, 0, 0, 0, 0, red, green, blue, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + cosaf * (width - org_x) - cosar *           org_y , y + sinaf * (width - org_x) - sinar *           org_y , 0, 0, 0, -1, 1, 0, 0, 0, red, green, blue, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + cosaf * (width - org_x) + cosar * (height - org_y), y + sinaf * (width - org_x) + sinar * (height - org_y), 0, 0, 0, -1, 1, 1, 0, 0, red, green, blue, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x - cosaf *          org_x  + cosar * (height - org_y), y - sinaf *          org_x  + sinar * (height - org_y), 0, 0, 0, -1, 0, 1, 0, 0, red, green, blue, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_Fill(float x, float y, float width, float height, float red, float green, float blue, float alpha, int flags)
{
	DrawQ_Pic(x, y, Draw_CachePic("white"), width, height, red, green, blue, alpha, flags);
}

/// color tag printing
const vec4_t string_colors[] =
{
	// Quake3 colors
	// LadyHavoc: why on earth is cyan before magenta in Quake3?
	// LadyHavoc: note: Doom3 uses white for [0] and [7]
	{0.0, 0.0, 0.0, 1.0}, // black
	{1.0, 0.0, 0.0, 1.0}, // red
	{0.0, 1.0, 0.0, 1.0}, // green
	{1.0, 1.0, 0.0, 1.0}, // yellow
	//{0.0, 0.0, 1.0, 1.0}, // blue
	{0.05, 0.15, 1.0, 1.0}, // lighter blue, readable unlike the above
	{0.0, 1.0, 1.0, 1.0}, // cyan
	{1.0, 0.0, 1.0, 1.0}, // magenta
	{1.0, 1.0, 1.0, 1.0}, // white
	// [515]'s BX_COLOREDTEXT extension
	{1.0, 1.0, 1.0, 0.5}, // half transparent
	{0.5, 0.5, 0.5, 1.0}  // half brightness
	// Black's color table
	//{1.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 0.0, 1.0},
	//{0.0, 1.0, 0.0, 1.0},
	//{0.0, 0.0, 1.0, 1.0},
	//{1.0, 1.0, 0.0, 1.0},
	//{0.0, 1.0, 1.0, 1.0},
	//{1.0, 0.0, 1.0, 1.0},
	//{0.1, 0.1, 0.1, 1.0}
};

#define STRING_COLORS_COUNT	(sizeof(string_colors) / sizeof(vec4_t))

static void DrawQ_GetTextColor(float color[4], int colorindex, float r, float g, float b, float a, qbool shadow)
{
	float C = r_textcontrast.value;
	float B = r_textbrightness.value;
	if (colorindex & 0x10000) // that bit means RGB color
	{
		color[0] = ((colorindex >> 12) & 0xf) / 15.0;
		color[1] = ((colorindex >> 8) & 0xf) / 15.0;
		color[2] = ((colorindex >> 4) & 0xf) / 15.0;
		color[3] = (colorindex & 0xf) / 15.0;
	}
	else
		Vector4Copy(string_colors[colorindex], color);
	Vector4Set(color, color[0] * r * C + B, color[1] * g * C + B, color[2] * b * C + B, color[3] * a);
	if (shadow)
	{
		float shadowalpha = (color[0]+color[1]+color[2]) * 0.8;
		Vector4Set(color, 0, 0, 0, color[3] * bound(0, shadowalpha, 1));
	}
}

// returns a colorindex (format 0x1RGBA) if str is a valid RGB string
// returns 0 otherwise
static int RGBstring_to_colorindex(const char *str)
{
	Uchar ch; 
	int ind = 0x0001 << 4;
	do {
		if (*str <= '9' && *str >= '0')
			ind |= (*str - '0');
		else
		{
			ch = tolower(*str);
			if (ch >= 'a' && ch <= 'f')
				ind |= (ch - 87);
			else
				return 0;
		}
		++str;
		ind <<= 4;
	} while(!(ind & 0x10000));
	return ind | 0xf; // add costant alpha value
}

// NOTE: this function always draws exactly one character if maxwidth <= 0
float DrawQ_TextWidth_UntilWidth_TrackColors_Scale(const char *text, size_t *maxlen, float w, float h, float sw, float sh, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt, float maxwidth)
{
	const char *text_start = text;
	int colorindex;
	size_t i;
	float x = 0;
	Uchar ch, mapch, nextch;
	Uchar prevch = 0; // used for kerning
	float kx;
	int map_index = 0;
	size_t bytes_left;
	ft2_font_map_t *fontmap = NULL;
	ft2_font_map_t *map = NULL;
	ft2_font_t *ft2 = fnt->ft2;
	// float ftbase_x;
	qbool snap = true;
	qbool least_one = false;
	float dw; // display w
	//float dh; // display h
	const float *width_of;

	if (!h) h = w;
	if (!h) {
		w = h = 1;
		snap = false;
	}
	// do this in the end
	w *= fnt->settings.scale;
	h *= fnt->settings.scale;

	// find the most fitting size:
	if (ft2 != NULL)
	{
		if (snap)
			map_index = Font_IndexForSize(ft2, h, &w, &h);
		else
			map_index = Font_IndexForSize(ft2, h, NULL, NULL);
		fontmap = Font_MapForIndex(ft2, map_index);
	}

	dw = w * sw;
	//dh = h * sh;

	if (*maxlen < 1)
		*maxlen = 1<<30;

	if (!outcolor || *outcolor == -1)
		colorindex = STRING_COLOR_DEFAULT;
	else
		colorindex = *outcolor;

	// maxwidth /= fnt->scale; // w and h are multiplied by it already
	// ftbase_x = snap_to_pixel_x(0);

	if(maxwidth <= 0)
	{
		least_one = true;
		maxwidth = -maxwidth;
	}

	//if (snap)
	//	x = snap_to_pixel_x(x, 0.4); // haha, it's 0 anyway

	if (fontmap)
		width_of = fnt->width_of_ft2[map_index];
	else
		width_of = fnt->width_of;

	i = 0;
	while (((bytes_left = *maxlen - (text - text_start)) > 0) && *text)
	{
		size_t i0 = i;
		nextch = ch = u8_getnchar(text, &text, bytes_left);
		i = text - text_start;
		if (!ch)
			break;
		if (ch == ' ' && !fontmap)
		{
			if(!least_one || i0) // never skip the first character
				if(x + width_of[(int) ' '] * dw > maxwidth)
				{
					i = i0;
					break; // oops, can't draw this
				}
			x += width_of[(int) ' '] * dw;
			continue;
		}
		if (ch == STRING_COLOR_TAG && !ignorecolorcodes && i < *maxlen)
		{
			ch = *text; // colors are ascii, so no u8_ needed
			if (ch <= '9' && ch >= '0') // ^[0-9] found
			{
				colorindex = ch - '0';
				++text;
				++i;
				continue;
			}
			else if (ch == STRING_COLOR_RGB_TAG_CHAR && i + 3 < *maxlen ) // ^x found
			{
				const char *text_p = &text[1];
				int tempcolorindex = RGBstring_to_colorindex(text_p);
				if (tempcolorindex)
				{
					colorindex = tempcolorindex;
					i+=4;
					text += 4;
					continue;
				}
			}
			else if (ch == STRING_COLOR_TAG) // ^^ found
			{
				i++;
				text++;
			}
			i--;
		}
		ch = nextch;

		if (!fontmap || (ch <= 0xFF && fontmap->glyphs[ch].image) || (ch >= 0xE000 && ch <= 0xE0FF))
		{
			if (ch > 0xE000)
				ch -= 0xE000;
			if (ch > 0xFF)
				continue;
			if (fontmap)
				map = ft2_oldstyle_map;
			prevch = 0;
			if(!least_one || i0) // never skip the first character
				if(x + width_of[ch] * dw > maxwidth)
				{
					i = i0;
					break; // oops, can't draw this
				}
			x += width_of[ch] * dw;
		} else {
			if (!map || map == ft2_oldstyle_map || ch != prevch)
			{
				Font_GetMapForChar(ft2, map_index, ch, &map, &mapch);
				if (!map)
					break;
			}
			if (prevch && Font_GetKerningForMap(ft2, map_index, w, h, prevch, ch, &kx, NULL))
				x += kx * dw;
			x += map->glyphs[mapch].advance_x * dw;
			prevch = ch;
		}
	}

	*maxlen = i;

	if (outcolor)
		*outcolor = colorindex;

	return x;
}

float DrawQ_Color[4];
float DrawQ_String_Scale(float startx, float starty, const char *text, size_t maxlen, float w, float h, float sw, float sh, float basered, float basegreen, float baseblue, float basealpha, int flags, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	int shadow, colorindex = STRING_COLOR_DEFAULT;
	size_t i;
	float x = startx, y, s, t, u, v, thisw;
	Uchar ch, mapch, nextch;
	Uchar prevch = 0; // used for kerning
	int map_index = 0;
	ft2_font_map_t *map = NULL;     // the currently used map
	ft2_font_map_t *fontmap = NULL; // the font map for the size
	float ftbase_y;
	const char *text_start = text;
	float kx, ky;
	ft2_font_t *ft2 = fnt->ft2;
	qbool snap = true;
	float pix_x, pix_y;
	size_t bytes_left;
	float dw, dh;
	const float *width_of;
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf = NULL;
	int e0, e1, e2, e3;
	int tw, th;
	tw = Draw_GetPicWidth(fnt->pic);
	th = Draw_GetPicHeight(fnt->pic);

	if (!h) h = w;
	if (!h) {
		h = w = 1;
		snap = false;
	}

	starty -= (fnt->settings.scale - 1) * h * 0.5 - fnt->settings.voffset*h; // center & offset
	w *= fnt->settings.scale;
	h *= fnt->settings.scale;

	if (ft2 != NULL)
	{
		if (snap)
			map_index = Font_IndexForSize(ft2, h, &w, &h);
		else
			map_index = Font_IndexForSize(ft2, h, NULL, NULL);
		fontmap = Font_MapForIndex(ft2, map_index);
	}

	dw = w * sw;
	dh = h * sh;

	// draw the font at its baseline when using freetype
	//ftbase_x = 0;
	ftbase_y = dh * (4.5/6.0);

	if (maxlen < 1)
		maxlen = 1<<30;

	if(!r_draw2d.integer && !r_draw2d_force)
		return startx + DrawQ_TextWidth_UntilWidth_TrackColors_Scale(text, &maxlen, w, h, sw, sh, NULL, ignorecolorcodes, fnt, 1000000000);

	//ftbase_x = snap_to_pixel_x(ftbase_x);
	if(snap)
	{
		startx = snap_to_pixel_x(startx, 0.4);
		starty = snap_to_pixel_y(starty, 0.4);
		ftbase_y = snap_to_pixel_y(ftbase_y, 0.3);
	}

	pix_x = vid.mode.width / vid_conwidth.value;
	pix_y = vid.mode.height / vid_conheight.value;

	if (fontmap)
		width_of = fnt->width_of_ft2[map_index];
	else
		width_of = fnt->width_of;

	for (shadow = r_textshadow.value != 0 && basealpha > 0;shadow >= 0;shadow--)
	{
		prevch = 0;
		text = text_start;

		if (!outcolor || *outcolor == -1)
			colorindex = STRING_COLOR_DEFAULT;
		else
			colorindex = *outcolor;

		DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);

		x = startx;
		y = starty;
		/*
		if (shadow)
		{
			x += r_textshadow.value * vid.width / vid_conwidth.value;
			y += r_textshadow.value * vid.height / vid_conheight.value;
		}
		*/
		while (((bytes_left = maxlen - (text - text_start)) > 0) && *text)
		{
			nextch = ch = u8_getnchar(text, &text, bytes_left);
			i = text - text_start;
			if (!ch)
				break;
			if (ch == ' ' && !fontmap)
			{
				x += width_of[(int) ' '] * dw;
				continue;
			}
			if (ch == STRING_COLOR_TAG && !ignorecolorcodes && i < maxlen)
			{
				ch = *text; // colors are ascii, so no u8_ needed
				if (ch <= '9' && ch >= '0') // ^[0-9] found
				{
					colorindex = ch - '0';
					DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);
					++text;
					++i;
					continue;
				}
				else if (ch == STRING_COLOR_RGB_TAG_CHAR && i+3 < maxlen ) // ^x found
				{
					const char *text_p = &text[1];
					int tempcolorindex = RGBstring_to_colorindex(text_p);
					if(tempcolorindex)
					{
						colorindex = tempcolorindex;
						DrawQ_GetTextColor(DrawQ_Color, colorindex, basered, basegreen, baseblue, basealpha, shadow != 0);
						i+=4;
						text+=4;
						continue;
					}
				}
				else if (ch == STRING_COLOR_TAG)
				{
					i++;
					text++;
				}
				i--;
			}
			// get the backup
			ch = nextch;
			// using a value of -1 for the oldstyle map because NULL means uninitialized...
			// this way we don't need to rebind fnt->tex for every old-style character
			// E000..E0FF: emulate old-font characters (to still have smileys and such available)
			if (shadow)
			{
				x += 1.0/pix_x * r_textshadow.value;
				y += 1.0/pix_y * r_textshadow.value;
			}
			if (!fontmap || (ch <= 0xFF && fontmap->glyphs[ch].image) || (ch >= 0xE000 && ch <= 0xE0FF))
			{
				if (ch >= 0xE000)
					ch -= 0xE000;
				if (ch > 0xFF)
					goto out;
				if (fontmap)
					map = ft2_oldstyle_map;
				prevch = 0;
				//num = (unsigned char) text[i];
				//thisw = fnt->width_of[num];
				thisw = fnt->width_of[ch];
				// FIXME make these smaller to just include the occupied part of the character for slightly faster rendering
				if (r_nearest_conchars.integer)
				{
					s = (ch & 15)*0.0625f;
					t = (ch >> 4)*0.0625f;
					u = 0.0625f * thisw;
					v = 0.0625f;
				}
				else
				{
					s = (ch & 15)*0.0625f + (0.5f / tw);
					t = (ch >> 4)*0.0625f + (0.5f / th);
					u = 0.0625f * thisw - (1.0f / tw);
					v = 0.0625f - (1.0f / th);
				}
				surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, fnt->pic->name, flags, TEXF_ALPHA | TEXF_CLAMP, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
				e0 = Mod_Mesh_IndexForVertex(mod, surf, x         , y   , 10, 0, 0, -1, s  , t  , 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e1 = Mod_Mesh_IndexForVertex(mod, surf, x+dw*thisw, y   , 10, 0, 0, -1, s+u, t  , 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e2 = Mod_Mesh_IndexForVertex(mod, surf, x+dw*thisw, y+dh, 10, 0, 0, -1, s+u, t+v, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e3 = Mod_Mesh_IndexForVertex(mod, surf, x         , y+dh, 10, 0, 0, -1, s  , t+v, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
				Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
				x += width_of[ch] * dw;
			} else {
				if (!map || map == ft2_oldstyle_map || ch != prevch)
				{
					Font_GetMapForChar(ft2, map_index, ch, &map, &mapch);
					if (!map)
					{
						shadow = -1;
						break;
					}
				}

				thisw = map->glyphs[mapch].advance_x;

				//x += ftbase_x;
				y += ftbase_y;
				if (prevch && Font_GetKerningForMap(ft2, map_index, w, h, prevch, ch, &kx, &ky))
				{
					x += kx * dw;
					y += ky * dh;
				}
				else
					kx = ky = 0;
				surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, map->pic->name, flags, TEXF_ALPHA | TEXF_CLAMP, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
				e0 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmin, y + dh * map->glyphs[mapch].vymin, 10, 0, 0, -1, map->glyphs[mapch].txmin, map->glyphs[mapch].tymin, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e1 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmax, y + dh * map->glyphs[mapch].vymin, 10, 0, 0, -1, map->glyphs[mapch].txmax, map->glyphs[mapch].tymin, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e2 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmax, y + dh * map->glyphs[mapch].vymax, 10, 0, 0, -1, map->glyphs[mapch].txmax, map->glyphs[mapch].tymax, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				e3 = Mod_Mesh_IndexForVertex(mod, surf, x + dw * map->glyphs[mapch].vxmin, y + dh * map->glyphs[mapch].vymax, 10, 0, 0, -1, map->glyphs[mapch].txmin, map->glyphs[mapch].tymax, 0, 0, DrawQ_Color[0], DrawQ_Color[1], DrawQ_Color[2], DrawQ_Color[3]);
				Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
				Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
				//x -= ftbase_x;
				y -= ftbase_y;

				x += thisw * dw;

				//prevmap = map;
				prevch = ch;
			}
out:
			if (shadow)
			{
				x -= 1.0/pix_x * r_textshadow.value;
				y -= 1.0/pix_y * r_textshadow.value;
			}
		}
	}

	if (outcolor)
		*outcolor = colorindex;

	// note: this relies on the proper text (not shadow) being drawn last
	return x;
}

float DrawQ_String(float startx, float starty, const char *text, size_t maxlen, float w, float h, float basered, float basegreen, float baseblue, float basealpha, int flags, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	return DrawQ_String_Scale(startx, starty, text, maxlen, w, h, 1, 1, basered, basegreen, baseblue, basealpha, flags, outcolor, ignorecolorcodes, fnt);
}

float DrawQ_TextWidth_UntilWidth_TrackColors(const char *text, size_t *maxlen, float w, float h, int *outcolor, qbool ignorecolorcodes, const dp_font_t *fnt, float maxwidth)
{
	return DrawQ_TextWidth_UntilWidth_TrackColors_Scale(text, maxlen, w, h, 1, 1, outcolor, ignorecolorcodes, fnt, maxwidth);
}

float DrawQ_TextWidth(const char *text, size_t maxlen, float w, float h, qbool ignorecolorcodes, const dp_font_t *fnt)
{
	return DrawQ_TextWidth_UntilWidth(text, &maxlen, w, h, ignorecolorcodes, fnt, 1000000000);
}

float DrawQ_TextWidth_UntilWidth(const char *text, size_t *maxlen, float w, float h, qbool ignorecolorcodes, const dp_font_t *fnt, float maxWidth)
{
	return DrawQ_TextWidth_UntilWidth_TrackColors(text, maxlen, w, h, NULL, ignorecolorcodes, fnt, maxWidth);
}

#if 0
// not used
// no ^xrgb management
static int DrawQ_BuildColoredText(char *output2c, size_t maxoutchars, const char *text, int maxreadchars, qbool ignorecolorcodes, int *outcolor)
{
	int color, numchars = 0;
	char *outputend2c = output2c + maxoutchars - 2;
	if (!outcolor || *outcolor == -1)
		color = STRING_COLOR_DEFAULT;
	else
		color = *outcolor;
	if (!maxreadchars)
		maxreadchars = 1<<30;
	textend = text + maxreadchars;
	while (text != textend && *text)
	{
		if (*text == STRING_COLOR_TAG && !ignorecolorcodes && text + 1 != textend)
		{
			if (text[1] == STRING_COLOR_TAG)
				text++;
			else if (text[1] >= '0' && text[1] <= '9')
			{
				color = text[1] - '0';
				text += 2;
				continue;
			}
		}
		if (output2c >= outputend2c)
			break;
		*output2c++ = *text++;
		*output2c++ = color;
		numchars++;
	}
	output2c[0] = output2c[1] = 0;
	if (outcolor)
		*outcolor = color;
	return numchars;
}
#endif

void DrawQ_SuperPic(float x, float y, cachepic_t *pic, float width, float height, float s1, float t1, float r1, float g1, float b1, float a1, float s2, float t2, float r2, float g2, float b2, float a2, float s3, float t3, float r3, float g3, float b3, float a3, float s4, float t4, float r4, float g4, float b4, float a4, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	if (!pic)
		pic = Draw_CachePic("white");
	// make sure pic is loaded - we don't use the texture here, Mod_Mesh_GetTexture looks up the skinframe by name
	Draw_GetPicTexture(pic);
	if (width == 0)
		width = pic->width;
	if (height == 0)
		height = pic->height;
	surf = Mod_Mesh_AddSurface(mod, Mod_Mesh_GetTexture(mod, pic->name, flags, pic->texflags, MATERIALFLAG_WALL | MATERIALFLAG_VERTEXCOLOR | MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW), true);
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x        , y         , 0, 0, 0, -1, s1, t1, 0, 0, r1, g1, b1, a1);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y         , 0, 0, 0, -1, s2, t2, 0, 0, r2, g2, b2, a2);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x + width, y + height, 0, 0, 0, -1, s4, t4, 0, 0, r4, g4, b4, a4);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x        , y + height, 0, 0, 0, -1, s3, t3, 0, 0, r3, g3, b3, a3);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_Line (float width, float x1, float y1, float x2, float y2, float r, float g, float b, float alpha, int flags)
{
	model_t *mod = CL_Mesh_UI();
	msurface_t *surf;
	int e0, e1, e2, e3;
	float offsetx, offsety;
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
	e0 = Mod_Mesh_IndexForVertex(mod, surf, x1 - offsetx, y1 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e1 = Mod_Mesh_IndexForVertex(mod, surf, x2 - offsetx, y2 - offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e2 = Mod_Mesh_IndexForVertex(mod, surf, x2 + offsetx, y2 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	e3 = Mod_Mesh_IndexForVertex(mod, surf, x1 + offsetx, y1 + offsety, 10, 0, 0, -1, 0, 0, 0, 0, r, g, b, alpha);
	Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
	Mod_Mesh_AddTriangle(mod, surf, e0, e2, e3);
}

void DrawQ_SetClipArea(float x, float y, float width, float height)
{
	int ix, iy, iw, ih;
	DrawQ_FlushUI();

	// We have to convert the con coords into real coords
	// OGL uses bottom to top (origin is in bottom left)
	ix = (int)(0.5 + x * ((float)r_refdef.view.width / vid_conwidth.integer)) + r_refdef.view.x;
	iy = (int)(0.5 + y * ((float)r_refdef.view.height / vid_conheight.integer)) + r_refdef.view.y;
	iw = (int)(0.5 + width * ((float)r_refdef.view.width / vid_conwidth.integer));
	ih = (int)(0.5 + height * ((float)r_refdef.view.height / vid_conheight.integer));
	switch(vid.renderpath)
	{
	case RENDERPATH_GL32:
	case RENDERPATH_GLES2:
		GL_Scissor(ix, vid.mode.height - iy - ih, iw, ih);
		break;
	}

	GL_ScissorTest(true);
}

void DrawQ_ResetClipArea(void)
{
	DrawQ_FlushUI();
	GL_ScissorTest(false);
}

void DrawQ_Finish(void)
{
	DrawQ_FlushUI();
	r_refdef.draw2dstage = 0;
}

void DrawQ_RecalcView(void)
{
	DrawQ_FlushUI();
	if(r_refdef.draw2dstage)
		r_refdef.draw2dstage = -1; // next draw call will set viewport etc. again
}

void DrawQ_FlushUI(void)
{
	model_t *mod = CL_Mesh_UI();
	if (mod->num_surfaces == 0)
		return;

	if (!r_draw2d.integer && !r_draw2d_force)
	{
		Mod_Mesh_Reset(mod);
		return;
	}

	// this is roughly equivalent to R_Mod_Draw, so the UI can use full material feature set
	r_refdef.view.colorscale = 1;
	r_textureframe++; // used only by R_GetCurrentTexture
	GL_DepthMask(false);

	Mod_Mesh_Finalize(mod);
	R_DrawModelSurfaces(&cl_meshentities[MESH_UI].render, false, false, false, false, false, true);

	Mod_Mesh_Reset(mod);
}
