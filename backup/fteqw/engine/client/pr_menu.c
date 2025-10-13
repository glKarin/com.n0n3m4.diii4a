#include "quakedef.h"

#include "pr_common.h"
#include "shader.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif

#if defined(MENU_DAT) || defined(CSQC_DAT)
#include "cl_master.h"

//MP_MouseMove(mx, my, mouse->qdeviceid)

static qbyte mpkeysdown[K_MAX/8];
extern qboolean csqc_dp_lastwas3d;

void M_Init_Internal (void);
void M_DeInit_Internal (void);

extern unsigned int r2d_be_flags;
static unsigned int PF_SelectDPDrawFlag(pubprogfuncs_t *prinst, int flag)
{
	if (r_refdef.warndraw)
	{
		if (!*r_refdef.rt_destcolour[0].texname)
		{
			r_refdef.warndraw = false; //don't spam too much
			PR_RunWarning(prinst, "Detected attempt to draw to framebuffer where framebuffer is not valid\n");
		}
	}
#ifdef CSQC_DAT
	csqc_dp_lastwas3d = false;	//for compat with dp's stupid beginpolygon
#endif

	//flags:
	//0 = blend
	//1 = add
	//2 = modulate
	//3 = modulate*2
	flag &= 3;
	if (flag == DRAWFLAG_ADD)
		return BEF_FORCEADDITIVE;
	else
		return 0;
}

//float	drawfill(vector position, vector size, vector rgb, float alpha, float flag) = #457;
void QCBUILTIN PF_CL_drawfill (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	float *size = G_VECTOR(OFS_PARM1);
	float *rgb = G_VECTOR(OFS_PARM2);
	float alpha = G_FLOAT(OFS_PARM3);
	int flag = prinst->callargc >= 5?G_FLOAT(OFS_PARM4):0;

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	R2D_FillBlock(pos[0], pos[1], size[0], size[1]);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}
//void	drawsetcliparea(float x, float y, float width, float height) = #458;
void QCBUILTIN PF_CL_drawsetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	srect_t srect;
	if (R2D_Flush)
		R2D_Flush();

#ifdef CSQC_DAT
	csqc_dp_lastwas3d = false;
#endif

	srect.x = G_FLOAT(OFS_PARM0) / (float)vid.fbvwidth;
	srect.y = G_FLOAT(OFS_PARM1) / (float)vid.fbvheight;
	srect.width = G_FLOAT(OFS_PARM2) / (float)vid.fbvwidth;
	srect.height = G_FLOAT(OFS_PARM3) / (float)vid.fbvheight;
	srect.dmin = -99999;
	srect.dmax = 99999;
	srect.y = (1-srect.y) - srect.height;
	BE_Scissor(&srect);

	G_FLOAT(OFS_RETURN) = 1;
}
//void	drawresetcliparea(void) = #459;
void QCBUILTIN PF_CL_drawresetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (R2D_Flush)
		R2D_Flush();

#ifdef CSQC_DAT
	csqc_dp_lastwas3d = false;
#endif

	BE_Scissor(NULL);
	G_FLOAT(OFS_RETURN) = 1;
}

#define FONT_SLOTS 32
#define FONT_SIZES 16
struct {
	unsigned int owner;	//kdm_foo. whoever has an interest in this font. font is purged when this becomes 0.
	char slotname[16];
	char facename[MAX_OSPATH];
	float scale; //poop
	int outline; //argh
	unsigned int fontflags; //erk
	int sizes;
	int size[FONT_SIZES];
	struct font_s *font[FONT_SIZES];
} fontslot[FONT_SLOTS];

static struct font_s *PR_CL_ChooseFont(float *fontsel, int szx, int szy)
{
	int fontidx = 0;	//default by default...
	struct font_s *font = font_default;

	if (fontsel)
	{
		fontidx = *fontsel;
	}

	if (fontidx >= 0 && fontidx < FONT_SLOTS)
	{
		int i, j;
		int fontdiff = 10000;
		for (i = 0; i < fontslot[fontidx].sizes; i++)
		{
			j = abs(szy - fontslot[fontidx].size[i]);
			if (j < fontdiff && fontslot[fontidx].font[i])
			{
				fontdiff = j;
				font = fontslot[fontidx].font[i];
			}
		}
	}
	return font;
}
void PR_CL_BeginString(pubprogfuncs_t *prinst, float vx, float vy, float szx, float szy, float *px, float *py)
{
	world_t *world = prinst->parms->user;
	struct font_s *font;
	if (world->g.drawfontscale && (world->g.drawfontscale[0] || world->g.drawfontscale[1]))
	{
		szx *= world->g.drawfontscale[0];
		szy *= world->g.drawfontscale[1];
	}
	font = PR_CL_ChooseFont(world->g.drawfont, szx, szy);
	Font_BeginScaledString(font, vx, vy, szx, szy, px, py);
}
int PR_findnamedfont(const char *name, qboolean isslotname)
{
	int i;
	if (isslotname)
	{
		for (i = 0; i < FONT_SLOTS; i++)
		{
			if (!stricmp(fontslot[i].slotname, name))
				return i;
		}
	}
	else
	{
		for (i = 0; i < FONT_SLOTS; i++)
		{
			if (!stricmp(fontslot[i].facename, name))
				return i;
		}
	}
	return -1;
}
int PR_findunusedfont(void)
{
	int i;
	//don't find slot 0.
	for (i = FONT_SLOTS; i-- > 1; )
	{
		if (!*fontslot[i].slotname && !*fontslot[i].facename)
			return i;
	}
	return -1;
}
//purgeowner is the bitmask of owners that are getting freed.
//if purgeowner is 0, fonts will get purged
void PR_ReleaseFonts(unsigned int purgeowner)
{
	int i, j;

	for (i = 0; i < FONT_SLOTS; i++)
	{
		if (fontslot[i].owner)
			continue;	//already free
		fontslot[i].owner &= ~purgeowner;
		if (fontslot[i].owner)
			continue;	//still owned by someone

		for (j = 0; j < fontslot[i].sizes; j++)
		{
			if (fontslot[i].font[j])
				Font_Free(fontslot[i].font[j]);
			fontslot[i].font[j] = NULL;
		}

		fontslot[i].sizes = 0;
		fontslot[i].slotname[0] = '\0';
		fontslot[i].facename[0] = '\0';
		fontslot[i].scale = 1;
		fontslot[i].outline = 0;
	}
}
void PR_ReloadFonts(qboolean reload)
{
	int i, j;

	if (qrenderer == QR_NONE)
		reload = false;

	for (i = 0; i < FONT_SLOTS; i++)
	{
		//already not loaded
		if (!fontslot[i].owner)
			continue;

		//flush it (if loaded)
		for (j = 0; j < fontslot[i].sizes; j++)
		{
			if (fontslot[i].font[j])
				Font_Free(fontslot[i].font[j]);
			fontslot[i].font[j] = NULL;
		}
		//and reload if needed
		if (reload)
		{	//otherwise load it.
			for (j = 0; j < fontslot[i].sizes; j++)
			{
				fontslot[i].font[j] = Font_LoadFont(fontslot[i].facename, fontslot[i].size[j], fontslot[i].scale, fontslot[i].outline, fontslot[i].fontflags);
			}
		}
	}
}
void QCBUILTIN PF_CL_findfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *slotname = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = PR_findnamedfont(slotname, true) + 1;	//return default on failure.
}
void QCBUILTIN PF_CL_loadfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern cvar_t r_font_postprocess_outline;
	const char *slotname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *facename = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *sizestr = PR_GetStringOfs(prinst, OFS_PARM2);
	int slotnum = (prinst->callargc>3)?G_FLOAT(OFS_PARM3):-1;
	//float fix_scale = (prinst->callargc>4)?G_FLOAT(OFS_PARM4):0;
	//float fix_voffset = (prinst->callargc>5)G_FLOAT(OFS_PARM5):0;
	int i, sz;
	world_t *world = prinst->parms->user;

	G_FLOAT(OFS_RETURN) = 0;	//return default on failure.

	if (slotnum < 0 && *slotname)
		slotnum = PR_findnamedfont(slotname, true);
	else if (slotnum < 0)
		slotnum = PR_findnamedfont(facename, false);
	if (slotnum < 0)
		slotnum = PR_findunusedfont();
	if (slotnum < 0)
		return;	//eep.

	if ((unsigned)slotnum >= FONT_SLOTS)
		return;

	//if its changed, purge it.
	if (stricmp(fontslot[slotnum].slotname, slotname) || stricmp(fontslot[slotnum].facename, facename) || !fontslot[slotnum].sizes)
	{
		Q_strncpyz(fontslot[slotnum].slotname, slotname, sizeof(fontslot[slotnum].slotname));
		Q_strncpyz(fontslot[slotnum].facename, facename, sizeof(fontslot[slotnum].facename));
		for (i = 0; i < fontslot[slotnum].sizes; i++)
		{
			if (fontslot[slotnum].font[i])
				Font_Free(fontslot[slotnum].font[i]);
			fontslot[slotnum].font[i] = NULL;
		}
		fontslot[slotnum].sizes = 0;
		fontslot[slotnum].owner = 0;
		fontslot[slotnum].scale = 1;
		fontslot[slotnum].outline = r_font_postprocess_outline.ival;
	}
	fontslot[slotnum].owner |= world->keydestmask;

	while(*sizestr)
	{
		sizestr = COM_Parse(sizestr);
		if (!strncmp(com_token, "scale=", 6))
		{
			fontslot[slotnum].scale = atof(com_token+6);
			continue;
		}
		if (!strncmp(com_token, "outline=", 8))
		{
			fontslot[slotnum].outline = atoi(com_token+8);
			continue;
		}
		if (!strncmp(com_token, "blur=", 5))
		{
			//fontslot[slotnum].blur = atoi(com_token+5);
			continue;
		}
		if (!strncmp(com_token, "voffset=", 8))
		{
			//com_token+8 unused.
			continue;
		}

		sz = atoi(com_token);
		if (!sz)
			continue;	//o.O

		for (i = 0; i < fontslot[slotnum].sizes; i++)
		{
			if (fontslot[slotnum].size[i] == sz)
				break;
		}
		if (i == fontslot[slotnum].sizes)
		{
			if (i >= FONT_SIZES)
				break;
			fontslot[slotnum].size[i] = sz;
			fontslot[slotnum].font[i] = NULL;
			fontslot[slotnum].sizes++;
		}
	}

	if (qrenderer > QR_NONE)
	{
		for (i = 0; i < fontslot[slotnum].sizes; i++)
			fontslot[slotnum].font[i] = Font_LoadFont(facename, fontslot[slotnum].size[i], fontslot[slotnum].scale, fontslot[slotnum].outline, fontslot[slotnum].fontflags);
	}

	G_FLOAT(OFS_RETURN) = slotnum;
}

#ifdef HAVE_LEGACY
void CL_LoadFont_f(void)
{
	extern cvar_t r_font_postprocess_outline, r_font_postprocess_mono;
	//console command for compat with dp/debug.
	if (Cmd_Argc() == 1)
	{
		int i, j;
		int th;
		for (i = 0; i < FONT_SLOTS; i++)
		{
			if (fontslot[i].sizes)
			{
				Con_Printf("%s[%i]: %s (", fontslot[i].slotname, i, fontslot[i].facename);
				for (j = 0; j < fontslot[i].sizes; j++)
				{
					if (j)
						Con_Printf(", ");
					Con_Printf("%i", fontslot[i].size[j]);
					if (fontslot[i].font[j])
					{
						th = Font_GetTrueHeight(fontslot[i].font[j]);
						if (th != Font_CharPHeight(fontslot[i].font[j]))
							Con_Printf("[%g]", ((float)th*vid.height)/vid.pixelheight);
					}
				}
				Con_Printf(")\n");
			}
		}
	}
	else
	{
		int i;
		int slotnum = 0;
		char *slotname = Cmd_Argv(1);
		char *facename = Cmd_Argv(2);
		int sizenum = 3;
		extern cvar_t dpcompat_console, gl_font, con_textfont;

		//loadfont slot face size1 size2...

		slotnum = PR_findnamedfont(slotname, true);
		if (slotnum < 0)
		{
			char *dpnames[] = {"default", "console", "sbar", "notify", "chat", "centerprint", "infobar", "menu", "user0", "user1", "user2", "user3", "user4", "user5", "user6", "user7", NULL};
			for (i = 0; dpnames[i]; i++)
			{
				if (!strcmp(dpnames[i], slotname))
				{
					//assign it to this slot only if this slot does not already have a face. avoids corrupting already-loaded fonts.
					if (!*fontslot[i].facename)
						slotnum = i;
					break;
				}
			}
			if (slotnum < 0)
				slotnum = PR_findnamedfont("", true);	//whatever is still free
		}
		if (slotnum < 0)
		{
			Con_Printf("out of font slots\n");
			return;
		}

		//if there's a new font in this slot, purge the old and change the name+face strings
		if (stricmp(fontslot[slotnum].slotname, slotname) || stricmp(fontslot[slotnum].facename, facename))
		{
			Q_strncpyz(fontslot[slotnum].slotname, slotname, sizeof(fontslot[slotnum].slotname));
			Q_strncpyz(fontslot[slotnum].facename, facename, sizeof(fontslot[slotnum].facename));
			for (i = 0; i < fontslot[slotnum].sizes; i++)
			{
				if (fontslot[slotnum].font[i])
					Font_Free(fontslot[slotnum].font[i]);
				fontslot[slotnum].font[i] = NULL;
			}
			fontslot[slotnum].owner = 0;
			fontslot[slotnum].scale = 1;
			fontslot[slotnum].sizes = 0;
			fontslot[slotnum].outline = r_font_postprocess_outline.ival;	//locked in at definition, so different fonts can have different settings even with vid_reload going on.
			fontslot[slotnum].fontflags = 0 |
										(r_font_postprocess_mono.ival?FONT_MONO:0);
		}
		if (!*facename)
			return;
		fontslot[slotnum].owner |= kdm_console;	//fonts owned by the console are never forgotten.
		
		while(sizenum < Cmd_Argc())
		{
			const char *a = Cmd_Argv(sizenum++);
			int sz;
			if (!strcmp(a, "scale"))
			{
				fontslot[slotnum].scale = atof(Cmd_Argv(sizenum++));
				continue;
			}
			if (!strcmp(a, "outline"))
			{
				fontslot[slotnum].outline = atoi(Cmd_Argv(sizenum++));
				continue;
			}
			if (!strcmp(a, "mono"))
			{
				if (atoi(Cmd_Argv(sizenum++)))
					fontslot[slotnum].fontflags |= FONT_MONO;
				else
					fontslot[slotnum].fontflags &= ~FONT_MONO;
				continue;
			}
			if (!strcmp(a, "blur"))
			{
				//fontslot[slotnum].blur = atoi(Cmd_Argv(sizenum++));
				sizenum++;
				continue;
			}
			if (!strcmp(a, "voffset"))
			{
//				fontslot[slotnum].voffset = atof(Cmd_Argv(sizenum++));
				sizenum++;
				continue;
			}
			sz = atoi(a);
			if (sz <= 0)
				sz = 8;

			for (i = 0; i < fontslot[slotnum].sizes; i++)
			{
				if (fontslot[slotnum].size[i] == sz)
					break;
			}
			if (i == fontslot[slotnum].sizes)
			{
				if (i >= FONT_SIZES)
					break;
				fontslot[slotnum].size[i] = sz;
				fontslot[slotnum].font[i] = NULL;
				fontslot[slotnum].sizes++;
			}
		}

		if (qrenderer > QR_NONE)
		{
			for (i = 0; i < fontslot[slotnum].sizes; i++)
				fontslot[slotnum].font[i] = Font_LoadFont(facename, fontslot[slotnum].size[i], fontslot[slotnum].scale, fontslot[slotnum].outline, fontslot[slotnum].fontflags);
		}

		//FIXME: slotnum0==default is problematic.
		if (dpcompat_console.ival && slotnum == 1)
			Cvar_Set(&con_textfont, facename);
		if (dpcompat_console.ival && slotnum == 0)
			Cvar_Set(&gl_font, facename);
	}
}
#endif

//scrolling could be done with scissoring.
//selection could be done with some substrings
void QCBUILTIN PF_CL_DrawTextField (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	float *size = G_VECTOR(OFS_PARM1);
	unsigned int flags = G_FLOAT(OFS_PARM2);
	const char *text = PR_GetStringOfs(prinst, OFS_PARM3);

	world_t *world = prinst->parms->user;
	vec2_t scale = {8, 8};
	struct font_s *font;
	if (world->g.drawfontscale && (world->g.drawfontscale[0] || world->g.drawfontscale[1]))
	{
		scale[0] *= world->g.drawfontscale[0];
		scale[1] *= world->g.drawfontscale[1];
	}
	font = PR_CL_ChooseFont(world->g.drawfont, scale[0], scale[1]);

	// Oversight ~eukara
	R2D_ImageColours(1.0f, 1.0f, 1.0f, 1.0f);

	G_FLOAT(OFS_RETURN) = R_DrawTextField(pos[0], pos[1], size[0], size[1], text, CON_WHITEMASK, flags, font, scale);
}

//float	drawstring(vector position, string text, vector scale, float alpha, float flag) = #455;
void QCBUILTIN PF_CL_drawcolouredstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	const char *text = PR_GetStringOfs(prinst, OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float alpha = 0;
	float flag = 0;
	float r, g, b;
	float px, py, ipx;
	unsigned int codeflags, codepoint;

	conchar_t buffer[2048], *str;

	if (prinst->callargc >= 6)
	{
		r = G_FLOAT(OFS_PARM3 + 0);
		g = G_FLOAT(OFS_PARM3 + 1);
		b = G_FLOAT(OFS_PARM3 + 2);
		alpha = G_FLOAT(OFS_PARM4);
		flag = G_FLOAT(OFS_PARM5);	//flag is mandatory to distinguish it.
	}
	else
	{
		r = 1;
		g = 1;
		b = 1;
		alpha = G_FLOAT(OFS_PARM3);
		flag = prinst->callargc >= 5?G_FLOAT(OFS_PARM4):0;
	}

	if (!text)
	{
		G_FLOAT(OFS_RETURN) = -1;	//was null..
		return;
	}

	COM_ParseFunString(CON_WHITEMASK, text, buffer, sizeof(buffer), false);
	str = buffer;

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	PR_CL_BeginString(prinst, pos[0], pos[1], size[0], size[1], &px, &py);
	ipx = px;
	R2D_ImageColours(r, g, b, alpha);
	while(*str)
	{
		str = Font_Decode(str, &codeflags, &codepoint);
		if (codeflags & CON_HIDDEN)
			continue;
		if (codepoint == '\n')
			py += Font_CharHeight();
		else if (codepoint == '\r')
			px = ipx;
		else
			px = Font_DrawScaleChar(px, py, codeflags, codepoint);
	}
	R2D_ImageColours(1,1,1,1);
	Font_EndString(NULL);
	r2d_be_flags = 0;
}

void QCBUILTIN PF_CL_stringwidth(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	conchar_t buffer[2048], *end;
	float px, py;
	const char *text = PR_GetStringOfs(prinst, OFS_PARM0);
	int usecolours = G_FLOAT(OFS_PARM1);
	float *size = (prinst->callargc > 2)?G_VECTOR(OFS_PARM2):NULL;

	if (!qrenderer)
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	end = COM_ParseFunString(CON_WHITEMASK, text, buffer, sizeof(buffer), !usecolours);

	PR_CL_BeginString(prinst, 0, 0, size?size[0]:8, size?size[1]:8, &px, &py);
	px = Font_LineScaleWidth(buffer, end);
	Font_EndString(NULL);

	if (!size)	//for compat with dp, divide by 8 after... because weird.
		px /= 8;

	G_FLOAT(OFS_RETURN) = (px * vid.width) / vid.rotpixelwidth;
}

//float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag) = #456;
void QCBUILTIN PF_CL_drawpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float *rgb = G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
	int flag = prinst->callargc >= 6?(int)G_FLOAT(OFS_PARM5):0;

	mpic_t *p;

	p = R2D_SafeCachePic(picname);
	if (!p || !R_GetShaderSizes(p, NULL, NULL, false))
		p = R2D_SafePicFromWad(picname);

	if (!p)
	{
		if (!CL_IsDownloading(picname))
			p = R2D_SafeCachePic("no_texture");
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
		G_FLOAT(OFS_RETURN) = 1;

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	if ((size[0] < 0) ^ (size[1] < 0))
		R2D_Image(pos[0]+size[0], pos[1]+size[1], -size[0], -size[1], 1, 1, 0, 0, p);
	else
		R2D_Image(pos[0], pos[1], size[0], size[1], 0, 0, 1, 1, p);
	r2d_be_flags = 0;
}

void QCBUILTIN PF_CL_drawrotpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pivot = G_VECTOR(OFS_PARM0);
	float *mins = G_VECTOR(OFS_PARM1);
	float *maxs = G_VECTOR(OFS_PARM2);
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM3);
	float *rgb = G_VECTOR(OFS_PARM4);
	float alpha = G_FLOAT(OFS_PARM5);
	float angle = (G_FLOAT(OFS_PARM6) * M_PI)/180;
	int flag = prinst->callargc >= 8?(int) G_FLOAT(OFS_PARM7):0;

	vec2_t points[4];
	vec2_t tcoords[4];
	vec2_t saxis;
	vec2_t taxis;

	mpic_t *p;

	p = R2D_SafeCachePic(picname);
	if (!p)
		p = R2D_SafePicFromWad(picname);

	saxis[0] = cos(angle);
	saxis[1] = sin(angle);
	taxis[0] = -sin(angle);
	taxis[1] = cos(angle);

	Vector2MA(pivot, mins[0], saxis, points[0]); Vector2MA(points[0], mins[1], taxis, points[0]);
	Vector2MA(pivot, maxs[0], saxis, points[1]); Vector2MA(points[1], mins[1], taxis, points[1]);
	Vector2MA(pivot, maxs[0], saxis, points[2]); Vector2MA(points[2], maxs[1], taxis, points[2]);
	Vector2MA(pivot, mins[0], saxis, points[3]); Vector2MA(points[3], maxs[1], taxis, points[3]);

	Vector2Set(tcoords[0], 0, 0);
	Vector2Set(tcoords[1], 1, 0);
	Vector2Set(tcoords[2], 1, 1);
	Vector2Set(tcoords[3], 0, 1);

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	R2D_Image2dQuad((const vec2_t*)points, (const vec2_t*)tcoords, NULL, p);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}

void QCBUILTIN PF_CL_drawsubpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	float *size = G_VECTOR(OFS_PARM1);
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM2);
	float *srcPos = G_VECTOR(OFS_PARM3);
	float *srcSize = G_VECTOR(OFS_PARM4);
	float *rgb = G_VECTOR(OFS_PARM5);
	float alpha = G_FLOAT(OFS_PARM6);
	int flag = prinst->callargc >= 8?(int) G_FLOAT(OFS_PARM7):0;

	mpic_t *p;

	p = R2D_SafeCachePic(picname);
	if (!p || !R_GetShaderSizes(p, NULL, NULL, false))
		p = R2D_SafePicFromWad(picname);

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	if ((size[0] < 0) ^ (size[1] < 0))
		R2D_Image(pos[0]+size[0], pos[1]+size[1], -size[0], -size[1], srcPos[0]+srcSize[0], srcPos[1]+srcSize[1], srcPos[0], srcPos[1], p);
	else
		R2D_Image(pos[0], pos[1], size[0], size[1], srcPos[0], srcPos[1], srcPos[0]+srcSize[0], srcPos[1]+srcSize[1], p);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}
void QCBUILTIN PF_CL_drawrotsubpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pivot = G_VECTOR(OFS_PARM0);
	float *mins = G_VECTOR(OFS_PARM1);
	float *maxs = G_VECTOR(OFS_PARM2);
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM3);
	float *srcPos = G_VECTOR(OFS_PARM4);
	float *srcSize = G_VECTOR(OFS_PARM5);
	float *rgb = G_VECTOR(OFS_PARM6);
	float alpha = G_FLOAT(OFS_PARM7+0);
	float angle = (G_FLOAT(OFS_PARM7+1) * M_PI) / 180;
	int flag = prinst->callargc >= 8?(int) G_FLOAT(OFS_PARM7+2):0;
	vec2_t points[4], tcoords[4];
	vec2_t saxis;
	vec2_t taxis;

	mpic_t *p;

	saxis[0] = cos(angle);
	saxis[1] = sin(angle);
	taxis[0] = -sin(angle);
	taxis[1] = cos(angle);

	p = R2D_SafeCachePic(picname);
	if (!p)
		p = R2D_SafePicFromWad(picname);

	Vector2MA(pivot, mins[0], saxis, points[0]); Vector2MA(points[0], mins[1], taxis, points[0]);
	Vector2MA(pivot, maxs[0], saxis, points[1]); Vector2MA(points[1], mins[1], taxis, points[1]);
	Vector2MA(pivot, maxs[0], saxis, points[2]); Vector2MA(points[2], maxs[1], taxis, points[2]);
	Vector2MA(pivot, mins[0], saxis, points[3]); Vector2MA(points[3], maxs[1], taxis, points[3]);

	Vector2Set(tcoords[0], srcPos[0]			, srcPos[1]				);
	Vector2Set(tcoords[1], srcPos[0]+srcSize[0]	, srcPos[1]				);
	Vector2Set(tcoords[2], srcPos[0]+srcSize[0]	, srcPos[1]+srcSize[1]	);
	Vector2Set(tcoords[3], srcPos[0]			, srcPos[1]+srcSize[1]	);

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	R2D_Image2dQuad((const vec2_t*)points, (const vec2_t*)tcoords, NULL, p);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}

#ifdef HAVE_LEGACY
/*fuck sake, why does no one give a shit about existing extension?!? seriously this stuff is pissing me off*/
void QCBUILTIN PF_CL_drawrotpic_dp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pivot = G_VECTOR(OFS_PARM0);
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float *mins = G_VECTOR(OFS_PARM3);
	float angle = (G_FLOAT(OFS_PARM4) * M_PI)/180;
	float *rgb = G_VECTOR(OFS_PARM5);
	float alpha = G_FLOAT(OFS_PARM6);
	int flag = prinst->callargc >= 8?(int) G_FLOAT(OFS_PARM7):0;

	vec3_t maxs;

	vec2_t points[4];
	vec2_t tcoords[4];
	vec2_t saxis;
	vec2_t taxis;

	mpic_t *p;

	VectorSubtract(size, mins, maxs);

	p = R2D_SafeCachePic(picname);
	if (!p)
		p = R2D_SafePicFromWad(picname);

	saxis[0] = cos(angle);
	saxis[1] = sin(angle);
	taxis[0] = -sin(angle);
	taxis[1] = cos(angle);

	Vector2MA(pivot, mins[0], saxis, points[0]); Vector2MA(points[0], mins[1], taxis, points[0]);
	Vector2MA(pivot, maxs[0], saxis, points[1]); Vector2MA(points[1], mins[1], taxis, points[1]);
	Vector2MA(pivot, maxs[0], saxis, points[2]); Vector2MA(points[2], maxs[1], taxis, points[2]);
	Vector2MA(pivot, mins[0], saxis, points[3]); Vector2MA(points[3], maxs[1], taxis, points[3]);

	Vector2Set(tcoords[0], 0, 0);
	Vector2Set(tcoords[1], 1, 0);
	Vector2Set(tcoords[2], 1, 1);
	Vector2Set(tcoords[3], 0, 1);

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	R2D_Image2dQuad((const vec2_t*)points, (const vec2_t*)tcoords, NULL, p);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}
#endif


void QCBUILTIN PF_CL_is_cached_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str;
	str = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = !!R_RegisterCustom(NULL, str, SUF_2D, NULL, NULL);
}

void QCBUILTIN PF_CL_precache_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern cvar_t pr_precachepic_slow;
	const char	*str;
	mpic_t	*pic;
	unsigned int flags;
#define PIC_FROMWAD 1				/*obsolete, probably better to just use gfx/foo.lmp instead*/
//#define PIC_TEMPORARY 2			/*DP, not meaningful here*/
//#define PIC_NOCLAMP 4				/*DP, not useful here - use a shader instead*/
//#define PIC_MIPMAP 8				/*DP, not useful here - use a shader instead*/
#define PRECACHE_PIC_DOWNLOAD 256	/*block until loaded, downloading if missing*/
#define PRECACHE_PIC_TEST 512		/*block until loaded*/

	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	str = PR_GetStringOfs(prinst, OFS_PARM0);
	if (prinst->callargc > 1)
		flags = G_FLOAT(OFS_PARM1);
	else
		flags = 0;

	if (pr_precachepic_slow.ival)
		flags |= PRECACHE_PIC_DOWNLOAD|PRECACHE_PIC_TEST;

	if (flags & PIC_FROMWAD)
		pic = R2D_SafePicFromWad(str);
	else
	{
		pic = R2D_SafeCachePic(str);

		if ((flags & PRECACHE_PIC_DOWNLOAD) && cls.state	//if we're allowed to download it...
			 && strchr(str, '.')	//only try to download it if it looks as though it contains a path.
#ifndef CLIENTONLY
			&& !sv.active			//not if we're already the server...
#endif
			&& (!pic || !R_GetShaderSizes(pic, NULL, NULL, true)))	//and it wasn't loaded...
			CL_CheckOrEnqueDownloadFile(str, str, 0);
	}

	if (flags & PRECACHE_PIC_TEST)
		if (!pic || !R_GetShaderSizes(pic, NULL, NULL, true))
			G_INT(OFS_RETURN) = 0;
}

#ifdef CSQC_DAT
//warning: not threaded.
void QCBUILTIN PF_CL_uploadimage (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *imagename = PR_GetStringOfs(prinst, OFS_PARM0);
	int width = G_INT(OFS_PARM1);
	int height = G_INT(OFS_PARM2);
	int src = G_INT(OFS_PARM3);	//ptr
	int size = (prinst->callargc > 4)?G_INT(OFS_PARM4):(width * height * 4);
	uploadfmt_t format = (prinst->callargc > 5)?PR_TranslateTextureFormat(G_INT(OFS_PARM5)):TF_RGBA32;
	void *imgptr;
	texid_t tid;

	G_INT(OFS_RETURN) = 0;	//assume the worst

	if (width < 0 || height < 0 || width > 16384 || height > 16384)
	{	//this is actually kinda likely when everyone assumes everything is a float.
		PR_BIError(prinst, "PF_CL_uploadimage: dimensions are out of range\n");
		return;
	}
	//FIXME: this should use a proper qclib function to validate more reliably / reusably
	if (src <= 0 || src+size >= prinst->stringtablesize)
	{
		PR_BIError(prinst, "PF_CL_uploadimage: invalid source\n");
		return;
	}
	imgptr = prinst->stringtable + src;


	tid = Image_FindTexture(imagename, NULL, RT_IMAGEFLAGS);
	if (!TEXVALID(tid))
		tid = Image_CreateTexture(imagename, NULL, RT_IMAGEFLAGS);

	if (!format)
	{
		void *data = BZ_Malloc(size);
		memcpy(data, imgptr, size);
		G_INT(OFS_RETURN) = Image_LoadTextureFromMemory(tid, tid->flags, tid->ident, imagename, data, size);
	}
	else
	{
		void *palette = NULL;
		unsigned int blockbytes, blockwidth, blockheight, blockdepth;
		//get format info
		Image_BlockSizeForEncoding(format, &blockbytes, &blockwidth, &blockheight, &blockdepth);
		//round up as appropriate
		blockwidth = ((width+blockwidth-1)/blockwidth)*blockwidth;
		blockheight = ((height+blockheight-1)/blockheight)*blockheight;

		//we do allow palettes on the end.
		if (format == TF_8PAL24)
			size -= 768, palette = (qbyte*)imgptr+size, blockbytes=1;
		else if (format == TF_8PAL32)
			size -= 1024, palette = (qbyte*)imgptr+size, blockbytes=1;

		if (size != blockwidth*blockheight*blockbytes)
			G_INT(OFS_RETURN) = 0;	//size isn't right. which means the pointer might be invalid too.
		else
		{
			Image_Upload(tid, format, imgptr, palette, width, height, 1, RT_IMAGEFLAGS);
			tid->width = width;
			tid->height = height;
			G_INT(OFS_RETURN) = 1;
		}
	}
}
#endif

//warning: not threadable. hopefully noone abuses it.
void QCBUILTIN PF_CL_readimage (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	size_t filesize;
	const char *filename = PR_GetStringOfs(prinst, OFS_PARM0);

	int imagewidth, imageheight;
	uploadfmt_t format;
	void *filedata;

	G_INT(OFS_RETURN) = 0;	//assume the worst
	G_INT(OFS_PARM1) = 0;	//out width
	G_INT(OFS_PARM2) = 0;	//out height
	G_INT(OFS_PARM3) = 0;	//out format

	filedata = FS_LoadMallocFile(filename, &filesize);

	if (filedata)
	{
		qbyte *imagedata = ReadRawImageFile(filedata, filesize, &imagewidth, &imageheight, &format, true, filename);
		Z_Free(filedata);

		if (imagedata)
		{
			void *ptr = prinst->AddressableAlloc(prinst, imagewidth*imageheight*4);
			if (ptr)
			{
				memcpy(ptr, imagedata, imagewidth*imageheight*4);
				G_INT(OFS_RETURN) = (char*)ptr - prinst->stringtable;
				G_INT(OFS_PARM1) = imagewidth;	//out width
				G_INT(OFS_PARM2) = imageheight;	//out height
				G_INT(OFS_PARM3) = PR_UnTranslateTextureFormat(format);	//out format...
			}
			BZ_Free(imagedata);
		}
	}
}

void QCBUILTIN PF_CL_free_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//we don't support this, as the shader could be used elsewhere also, and we have pointers to things.
/*
	char	*str;
	str = PR_GetStringOfs(prinst, OFS_PARM0);
	R_UnloadShader(R_RegisterCustom(str, NULL, NULL));
*/
}

//float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag) = #454;
void QCBUILTIN PF_CL_drawcharacter (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	int chara = G_FLOAT(OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float *rgb = G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
	int flag = prinst->callargc >= 6?G_FLOAT(OFS_PARM5):0;

	float x, y;

	if (!chara)
	{
		G_FLOAT(OFS_RETURN) = -1;	//was null..
		return;
	}

	//no control chars. use quake ones if so
	if (!(flag & 4) && !com_parseutf8.ival)
	{
		//ugly quake chars...
		if (chara >= 32 && chara < 128)
			;	//ascii-comptaible range
		else
			chara |= 0xe000;	//use quake glyphs (including for red text, unfortunately)
	}

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	PR_CL_BeginString(prinst, pos[0], pos[1], size[0], size[1], &x, &y);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	Font_DrawScaleChar(x, y, CON_WHITEMASK, chara);
	R2D_ImageColours(1,1,1,1);
	Font_EndString(NULL);
	r2d_be_flags = 0;

	G_FLOAT(OFS_RETURN) = 1;
}

//float	drawrawstring(vector position, string text, vector scale, vector rgb, float alpha, float flag) = #455;
void QCBUILTIN PF_CL_drawrawstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{	
	float *pos = G_VECTOR(OFS_PARM0);
	const char *text = PR_GetStringOfs(prinst, OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float *rgb = G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
	int flag = prinst->callargc >= 6?G_FLOAT(OFS_PARM5):0;
	float x, y;
	unsigned int c;
	int error;

	if (!text)
	{
		G_FLOAT(OFS_RETURN) = -1;	//was null..
		return;
	}

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flag);
	PR_CL_BeginString(prinst, pos[0], pos[1], size[0], size[1], &x, &y);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);

	while(*text)
	{
		if (1)//VMUTF8)
			c = unicode_decode(&error, text, &text, false);
		else
		{
			//FIXME: which charset is this meant to be using?
			//quakes? 8859-1? utf8? some weird hacky mixture?
			c = *text++&0xff;
			if ((c&0x7f) < 32)
				c |= 0xe000;	//if its a control char, just use the quake range instead.
			else if (c & 0x80)
				c |= 0xe000;	//if its a high char, just use the quake range instead. we could colour it, but why bother
		}
		x = Font_DrawScaleChar(x, y, CON_WHITEMASK, c);
	}
	R2D_ImageColours(1,1,1,1);
	Font_EndString(NULL);
	r2d_be_flags = 0;
}

//void (float width, vector pos1, vector pos2, vector rgb, float alpha, optional float flags) drawline;
void QCBUILTIN PF_CL_drawline (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//float width = G_FLOAT(OFS_PARM0);
	float *point1	= G_VECTOR(OFS_PARM1);
	float *point2	= G_VECTOR(OFS_PARM2);
	float *rgb	= G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
	int flags	= prinst->callargc >= 6?G_FLOAT(OFS_PARM5):0;
	shader_t *shader_draw_line;

	//this shader lookup might get pricy.
	shader_draw_line = R_RegisterShader("shader_draw_line", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"map $whiteimage\n"
				"rgbgen exactvertex\n"
				"alphagen vertex\n"
				"blendfunc blend\n"
			"}\n"
		"}\n");

	r2d_be_flags = PF_SelectDPDrawFlag(prinst, flags);
	R2D_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	R2D_Line(point1[0], point1[1], point2[0], point2[1], shader_draw_line);
	R2D_ImageColours(1,1,1,1);
	r2d_be_flags = 0;
}

//vector  drawgetimagesize(string pic) = #460;
void QCBUILTIN PF_CL_drawgetimagesize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *picname = PR_GetStringOfs(prinst, OFS_PARM0);
	mpic_t *p = R2D_SafeCachePic(picname);

	float *ret = G_VECTOR(OFS_RETURN);
	int iw, ih;
	
	if (R_GetShaderSizes(p, &iw, &ih, true) > 0)
	{
		ret[0] = iw;
		ret[1] = ih;
		ret[2] = 0;
	}
	else
	{
		ret[0] = 0;
		ret[1] = 0;
		ret[2] = 0;
	}
}

//vector	getmousepos(void)  	= #66;
void QCBUILTIN PF_cl_getmousepos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *ret = G_VECTOR(OFS_RETURN);
	world_t *world = prinst->parms->user;
	unsigned int target = world->keydestmask;

	if (key_dest_absolutemouse & target)
	{
		ret[0] = mousecursor_x;
		ret[1] = mousecursor_y;
	}
	else
	{
		ret[0] = mousemove_x;
		ret[1] = mousemove_y;
	}

	mousemove_x=0;
	mousemove_y=0;

//	extern int mousecursor_x, mousecursor_y;
//	ret[0] = mousecursor_x;
//	ret[1] = mousecursor_y;
	ret[2] = 0;
}


void QCBUILTIN PF_SubConGetSet (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *conname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *field = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *value = (prinst->callargc>2)?PR_GetStringOfs(prinst, OFS_PARM2):NULL;
	console_t *con = Con_FindConsole(conname);
	G_INT(OFS_RETURN) = 0;
	if (!con)
	{
		//null if it doesn't exist
		return;
	}
	if (!strcmp(field, "title"))
	{
		RETURN_TSTRING(con->title);
		if (value)
			Q_strncpyz(con->title, value, sizeof(con->title));
	}
	else if (!strcmp(field, "name"))
	{
		RETURN_TSTRING(con->name);
		if (value && *value && *con->name)
			Q_strncpyz(con->name, value, sizeof(con->name));
	}
	else if (!strcmp(field, "next"))
	{
		con = con->next;
		if (con)
			RETURN_TSTRING(con->name);
	}
	else if (!strcmp(field, "unseen"))
	{
		RETURN_TSTRING(va("%i", con->unseentext));
		if (value)
			con->unseentext = atoi(value);
	}
	else if (!strcmp(field, "markup"))
	{
		int cur;
		if (con->parseflags & PFS_NOMARKUP)
			cur = 0;
		else if (con->parseflags & PFS_KEEPMARKUP)
			cur = 2;
		else
			cur = 1;
		RETURN_TSTRING(va("%i", cur));
		if (value)
		{
			cur = atoi(value);
			con->parseflags &= ~(PFS_NOMARKUP|PFS_KEEPMARKUP);
			if (cur == 0)
				con->parseflags |= PFS_NOMARKUP;
			else if (cur == 2)
				con->parseflags |= PFS_KEEPMARKUP;
		}
	}
	else if (!strcmp(field, "forceutf8"))
	{
		RETURN_TSTRING((con->parseflags&PFS_FORCEUTF8)?"1":"0");
		if (value)
		{
			con->parseflags &= ~PFS_FORCEUTF8;
			if (atoi(value))
				con->parseflags |= PFS_FORCEUTF8;
		}
	}
	else if (!strcmp(field, "close"))
	{
		RETURN_TSTRING("0");	//meant to return the old state...
		if (value && atoi(value))
		{
			if (con->close && atoi(value) != 2 && !con->close(con, true))
				return;
			Con_Destroy(con);
		}
	}
	else if (!strcmp(field, "clear"))
	{
		RETURN_TSTRING(con->linecount?"0":"1");
		if (value && atoi(value))
			Con_ClearCon(con);
	}
	else if (!strcmp(field, "hidden"))
	{
		RETURN_TSTRING((con->flags & CONF_HIDDEN)?"1":"0");
		if (value)
			con->flags = (con->flags & ~CONF_HIDDEN) | (atoi(value)?CONF_HIDDEN:0);
	}
	else if (!strcmp(field, "linecount"))
	{
		RETURN_TSTRING(va("%i", con->linecount));
		if (value)
			con->unseentext = atoi(value);
	}
	else if (!strcmp(field, "backimage"))
	{
		RETURN_TSTRING(con->backshader?con->backshader->name:con->backimage);
		if (value)
		{
			Q_strncpyz(con->backimage, value, sizeof(con->backimage));
			if (con->backshader)
				R_UnloadShader(con->backshader);
		}
	}
	else if (!strcmp(field, "backvideomap"))
	{
		RETURN_TSTRING(con->backshader?con->backshader->name:con->backimage);
		if (value)
		{
			Q_strncpyz(con->backimage, "", sizeof(con->backimage));
			if (con->backshader)
				R_UnloadShader(con->backshader);
			con->backshader = R_RegisterCustom(NULL, va("consolevid_%s", con->name), SUF_NONE, Shader_DefaultCinematic, value);
		}
	}
}
void QCBUILTIN PF_SubConPrintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char outbuf[4096];
	const char *conname = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *fmt = PR_GetStringOfs(prinst, OFS_PARM1);
	console_t *con = Con_FindConsole(conname);
	if (!con)
	{
		con = Con_Create(conname, 0);
		if (!con)
			return;
	}
	PF_sprintf_internal(prinst, pr_globals, fmt, 2, outbuf, sizeof(outbuf));
	Con_PrintCon(con, outbuf, con->parseflags);
}
void QCBUILTIN PF_SubConDraw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *conname = PR_GetStringOfs(prinst, OFS_PARM0);
	float *pos = G_VECTOR(OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float fontsize = G_FLOAT(OFS_PARM3);
	console_t *con = Con_FindConsole(conname);
	world_t *world = prinst->parms->user;
	if (!con)
		return;

	if (world->g.drawfontscale)
	{
//		szx *= world->g.drawfontscale[0];
		fontsize *= world->g.drawfontscale[1];
	}

	Con_DrawOneConsole(con, con->flags & CONF_KEYFOCUSED, PR_CL_ChooseFont(world->g.drawfont, fontsize, fontsize), pos[0], pos[1], size[0], size[1], 0);
}
void QCBUILTIN PF_SubConInput (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *conname = PR_GetStringOfs(prinst, OFS_PARM0);
	int ie = G_FLOAT(OFS_PARM1);
	float pa = G_FLOAT(OFS_PARM2);
	float pb = G_FLOAT(OFS_PARM3);
//	float pc = G_FLOAT(OFS_PARM4);
	console_t *con = Con_FindConsole(conname);
	G_FLOAT(OFS_RETURN) = 0;
	if (!con)
		return;
	switch(ie)
	{
	case CSIE_KEYDOWN:
		//scan, char
		if ((pa && qcinput_scan != pa) || (pb && pb != qcinput_unicode))
			G_FLOAT(OFS_RETURN) = 0;
		else
			G_FLOAT(OFS_RETURN) = Key_Console(con, MP_TranslateQCtoFTECodes(pa), pb);
		break;
	case CSIE_KEYUP:
		//scan, char
		Key_ConsoleRelease(con, MP_TranslateQCtoFTECodes(pa), pb);
		G_FLOAT(OFS_RETURN) = 0;	//does not inhibit
		break;
	case CSIE_MOUSEABS:
		//x, y
		if (con == con_current && (key_dest_mask & kdm_console))
			break;	//no interfering with the main console!
		con->mousecursor[0] = pa;
		con->mousecursor[1] = pb;
		G_FLOAT(OFS_RETURN) = true;
		break;
	case CSIE_FOCUS:
		//mouse, key
		if (pb >= 0)
		{
			con->flags = (con->flags & ~CONF_KEYFOCUSED) | (pb?CONF_KEYFOCUSED:0);
			G_FLOAT(OFS_RETURN) = true;
		}
		break;
	}
}
#endif







#ifdef MENU_DAT

typedef struct menuedict_s
{
	enum ereftype_e	ereftype;
	float			freetime; // sv.time when the object was freed
	int				entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//world

	void			*fields;
} menuedict_t;



static struct
{
	evalc_t chain;
	evalc_t model;
	evalc_t modelindex;
	evalc_t mins;
	evalc_t maxs;
	evalc_t origin;
	evalc_t angles;
	evalc_t skin;
	evalc_t colormap;
	evalc_t frame1;
	evalc_t frame2;
	evalc_t lerpfrac;
	evalc_t frame1time;
	evalc_t frame2time;
	evalc_t renderflags;
	evalc_t skinobject;
	evalc_t skelobject;
	evalc_t colourmod;
	evalc_t alpha;
} menuc_eval;
static playerview_t menuview;


static menu_t menuqc;	//this is how the client forwards events etc.
static int inmenuprogs;
static progparms_t menuprogparms;
static menuedict_t *menu_edicts;
static int num_menu_edicts;
world_t menu_world;
static int menuentsize;
double  menutime;
static struct
{
	func_t init;
	func_t shutdown;
	func_t draw;	qboolean fuckeddrawsizes;
	func_t drawloading;
	func_t keydown;
	func_t keyup;
	func_t inputevent;
	func_t toggle;
	func_t consolecommand;
	func_t gethostcachecategory;
	func_t rendererrestarted;
} mpfuncs;
jmp_buf mp_abort;


// cvars
#define MENUPROGSGROUP "Menu progs control"
cvar_t forceqmenu = CVAR("forceqmenu", "0");
cvar_t pr_menu_coreonerror = CVAR("pr_menu_coreonerror", "1");
cvar_t pr_menu_memsize = CVAR("pr_menu_memsize", "64m");


//new generic functions.

const char *RemapCvarNameFromDPToFTE(const char *name)
{
	if (!stricmp(name, "vid_bitsperpixel"))
		return "vid_bpp";
	if (!stricmp(name, "_cl_playermodel"))
		return "model";
	if (!stricmp(name, "_cl_playerskin"))
		return "skin";
	if (!stricmp(name, "_cl_color"))
		return "topcolor";
	if (!stricmp(name, "_cl_name"))
		return "name";

	if (!stricmp(name, "v_contrast"))
		return "v_contrast";
	if (!stricmp(name, "v_hwgamma"))
		return "vid_hardwaregamma";
	if (!stricmp(name, "showfps"))
		return "show_fps";
	if (!stricmp(name, "sv_progs"))
		return "progs";

	return name;
}

static void QCBUILTIN PF_menu_cvar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	cvar_t	*var;
	const char	*str;

	str = PR_GetStringOfs(prinst, OFS_PARM0);

	if (!strcmp(str, "vid_conwidth"))
		G_FLOAT(OFS_RETURN) = vid.width;
	else if (!strcmp(str, "vid_conheight"))
		G_FLOAT(OFS_RETURN) = vid.height;
	else if (!strcmp(str, "vid_pixwidth"))
		G_FLOAT(OFS_RETURN) = vid.pixelwidth;
	else if (!strcmp(str, "vid_pixheight"))
		G_FLOAT(OFS_RETURN) = vid.pixelheight;
	else
	{
		str = RemapCvarNameFromDPToFTE(str);
		var = PF_Cvar_FindOrGet(str);
		if (var && !(var->flags & CVAR_NOUNSAFEEXPAND))
		{
			//menuqc sees desired settings, not latched settings.
			if (var->latched_string)
				G_FLOAT(OFS_RETURN) = atof(var->latched_string);
			else
				G_FLOAT(OFS_RETURN) = var->value;
		}
		else
			G_FLOAT(OFS_RETURN) = 0;
	}
}
static void QCBUILTIN PF_menu_cvar_set (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*var_name, *val;
	cvar_t *var;

	var_name = PR_GetStringOfs(prinst, OFS_PARM0);
	var_name = RemapCvarNameFromDPToFTE(var_name);
	val = PR_GetStringOfs(prinst, OFS_PARM1);

	var = PF_Cvar_FindOrGet(var_name);
	if (var && var->flags & CVAR_NOTFROMSERVER)
	{
		//fixme: menuqc needs some way to display a prompt to allow it anyway.
		return;
	}
	Cvar_Set (var, val);
}
static void QCBUILTIN PF_menu_cvar_string (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str = PR_GetStringOfs(prinst, OFS_PARM0);
	cvar_t *cv = PF_Cvar_FindOrGet(RemapCvarNameFromDPToFTE(str));
	if (!cv)
		G_INT(OFS_RETURN) = 0;
	else if (cv->flags & CVAR_NOUNSAFEEXPAND)
		G_INT(OFS_RETURN) = 0;
	else if (cv->latched_string)
		G_INT(OFS_RETURN) = (int)PR_TempString(prinst, cv->latched_string);
	else
		G_INT(OFS_RETURN) = (int)PR_TempString(prinst, cv->string);
}




void QCBUILTIN PF_nonfatalobjerror (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*s;
	struct edict_s	*ed;
	eval_t *selfp;

	s = PF_VarString(prinst, 0, pr_globals);

	PR_StackTrace(prinst, true);

	selfp = PR_FindGlobal(prinst, "self", PR_CURRENT, NULL);
	if (selfp && selfp->_int)
	{
		ed = PROG_TO_EDICT(prinst, selfp->_int);

		PR_PrintEdict(prinst, ed);


		if (developer.value)
		{	//enable tracing.
			PR_RunWarning(prinst, "======OBJECT ERROR======\n%s\n", s);
			return;
		}
		else
		{
			ED_Free (prinst, ed);
		}
	}

	Con_Printf ("======OBJECT ERROR======\n%s\n", s);
}






//float	isserver(void)  = #60;
void QCBUILTIN PF_isserver (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef CLIENTONLY
	G_FLOAT(OFS_RETURN) = false;
#else
	if (sv.state == ss_dead)
		G_FLOAT(OFS_RETURN) = false;
	else if (sv.allocated_client_slots == 1)
		G_FLOAT(OFS_RETURN) = 0.5;
	else
		G_FLOAT(OFS_RETURN) = true;
#endif
}
void QCBUILTIN PF_isdemo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = !!cls.demoplayback;
}

//float	clientstate(void)  = #62;
void QCBUILTIN PF_clientstate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//menuqc was originally implemented in DP, so these return values follow NQ norms.
	if (isDedicated)	//unreachable
		G_FLOAT(OFS_RETURN) = 0/*nq ca_dedicated*/;
	else if (	cls.state >= ca_connected	//we're on a server
			||	CL_TryingToConnect()		//or we're trying to connect (avoids bugs with certain menuqc mods)
#ifndef CLIENTONLY
			||	sv.state>=ss_loading
#endif
				)	//or we're going to connect to ourselves once we get our act together
		G_FLOAT(OFS_RETURN) = 2/*nq ca_connected*/;
	else
		G_FLOAT(OFS_RETURN) = 1/*nq ca_disconnected*/;
}

static void QCBUILTIN PF_Ignore (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_RETURN) = 0;
}
//too specific to the prinst's builtins.
static void QCBUILTIN PF_Fixme (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int binum;
	char fname[MAX_QPATH];
	if (!prinst->GetBuiltinCallInfo(prinst, &binum, fname, sizeof(fname)))
	{
		binum = 0;
		strcpy(fname, "?unknown?");
	}

	Con_Printf("\n");
	prinst->RunError(prinst, "\nBuiltin %i:%s not implemented.\nMenu is not compatible.", binum, fname);
	PR_BIError (prinst, "bulitin not implemented");
}
static void QCBUILTIN PF_checkbuiltin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	func_t funcref = G_INT(OFS_PARM0);
	char *funcname = NULL;
	int args;
	int builtinno;
	if (prinst->GetFunctionInfo(prinst, funcref, &args, NULL, &builtinno, funcname, sizeof(funcname)))
	{	//qc defines the function at least. nothing weird there...
		if (builtinno > 0 && builtinno < prinst->parms->numglobalbuiltins)
		{
			if (!prinst->parms->globalbuiltins[builtinno] || prinst->parms->globalbuiltins[builtinno] == PF_Fixme || prinst->parms->globalbuiltins[builtinno] == PF_Ignore)
				G_FLOAT(OFS_RETURN) = false;	//the builtin with that number isn't defined.
			else
			{
				G_FLOAT(OFS_RETURN) = true;		//its defined, within the sane range, mapped, everything. all looks good.
				//we should probably go through the available builtins and validate that the qc's name matches what would be expected
				//this is really intended more for builtins defined as #0 though, in such cases, mismatched assumptions are impossible.
			}
		}
		else
			G_FLOAT(OFS_RETURN) = false;	//not a valid builtin (#0 builtins get remapped according to the function name)
	}
	else
	{	//not valid somehow.
		G_FLOAT(OFS_RETURN) = false;
	}
}



void QCBUILTIN PF_CL_precache_sound (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char	*str;

	str = PR_GetStringOfs(prinst, OFS_PARM0);

	if (S_PrecacheSound(str))
		G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	else
		G_INT(OFS_RETURN) = 0;
}

//void	setkeydest(float dest) 	= #601;
void QCBUILTIN PF_cl_setkeydest (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//these arguments are stupid
	switch((int)G_FLOAT(OFS_PARM0))
	{
	case 0:
		// key_game
		if (Key_Dest_Has(kdm_menu))
		{
			Menu_Unlink(&menuqc, false);
			Key_Dest_Remove(kdm_menu);
//			Key_Dest_Remove(kdm_message);
//			if (cls.state == ca_disconnected)
//				Key_Dest_Add(kdm_console);
		}
		break;
	case 2:
		// key_menu
		Key_Dest_Remove(kdm_message);
		if (!Key_Dest_Has(kdm_menu))
			Key_Dest_Remove(kdm_console);
		Menu_Push(&menuqc, false);
		break;
	case 1:
		// key_message
		//Key_Dest_Remove(kdm_menu);
		//Key_Dest_Add(kdm_message);
		// break;
	default:
		PR_BIError (prinst, "PF_setkeydest: wrong destination %i !\n",(int)G_FLOAT(OFS_PARM0));
	}
}
//float	getkeydest(void)	= #602;
void QCBUILTIN PF_cl_getkeydest (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (Key_Dest_Has(kdm_menu))
		G_FLOAT(OFS_RETURN) = 2;
//	else if (Key_Dest_Has(kdm_message))
//		G_FLOAT(OFS_RETURN) = 1;
	else
		G_FLOAT(OFS_RETURN) = 0;
}

static void QCBUILTIN PF_Remove_ (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *ed;

	ed = (void*)G_EDICT(prinst, OFS_PARM0);

	if (ed->ereftype == ER_FREE)
	{
		Con_DPrintf("Tried removing free entity\n");
		PR_StackTrace(prinst, false);
		return;
	}

	ED_Free (prinst, (void*)ed);
}
static void QCBUILTIN PF_RemoveInstant (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *ed;

	ed = (void*)G_EDICT(prinst, OFS_PARM0);

	if (ed->ereftype == ER_FREE)
	{
		Con_DPrintf("Tried removing free entity\n");
		PR_StackTrace(prinst, false);
		return;
	}

	prinst->EntFree(prinst, (void*)ed, true);
}

static void QCBUILTIN PF_CopyEntity (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *in, *out;

	in = (menuedict_t*)G_EDICT(prinst, OFS_PARM0);
	out = (menuedict_t*)G_EDICT(prinst, OFS_PARM1);

	memcpy(out->fields, in->fields, menuentsize);
}

void QCBUILTIN PF_menu_checkextension (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *extname = PR_GetStringOfs(prinst, OFS_PARM0);
	int i;
	G_FLOAT(OFS_RETURN) = 0;

	for (i = 0; i < QSG_Extensions_count; i++)
	{
		if (!QSG_Extensions[i].name)
			continue;
		if (!stricmp(extname, QSG_Extensions[i].name))
		{
			G_FLOAT(OFS_RETURN) = 1;
			break;
		}
	}
}

void QCBUILTIN PF_CL_precache_file (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

//entity	findchainstring(.string _field, string match) = #26;
void QCBUILTIN PF_menu_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	const char *s;
	string_t t;
	menuedict_t *ent, *chain;	//note, all edicts share the common header, but don't use it's fields!
	eval_t *val;

	chain = (menuedict_t *) *prinst->parms->edicts;

	f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = PR_GetStringOfs(prinst, OFS_PARM1);

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = (menuedict_t *)EDICT_NUM_PB(prinst, i);
		if (ent->ereftype == ER_FREE)
			continue;
		t = *(string_t *)&((float*)ent->fields)[f];
		if (!t)
			continue;
		if (strcmp(PR_GetString(prinst, t), s))
			continue;

		val = prinst->GetEdictFieldValue(prinst, (void*)ent, "chain", ev_entity, &menuc_eval.chain);
		if (val)
			val->edict = EDICT_TO_PROG(prinst, (void*)chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, (void*)chain);
}
//entity	findchainfloat(.float _field, float match) = #27;
void QCBUILTIN PF_menu_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	float s;
	menuedict_t	*ent, *chain;	//note, all edicts share the common header, but don't use it's fields!
	eval_t *val;

	chain = (menuedict_t *) *prinst->parms->edicts;

	f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = G_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = (menuedict_t*)EDICT_NUM_PB(prinst, i);
		if (ent->ereftype == ER_FREE)
			continue;
		if (((float *)ent->fields)[f] != s)
			continue;

		val = prinst->GetEdictFieldValue(prinst, (void*)ent, "chain", ev_entity, &menuc_eval.chain);
		if (val)
			val->edict = EDICT_TO_PROG(prinst, (void*)chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, (void*)chain);
}
//entity	findchainflags(.float _field, float match);
void QCBUILTIN PF_menu_findchainflags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	int s;
	menuedict_t	*ent, *chain;	//note, all edicts share the common header, but don't use it's fields!
	eval_t *val;

	chain = (menuedict_t *) *prinst->parms->edicts;

	f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	s = G_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->num_edicts; i++)
	{
		ent = (menuedict_t*)EDICT_NUM_PB(prinst, i);
		if (ent->ereftype == ER_FREE)
			continue;
		if ((int)((float *)ent->fields)[f] & s)
			continue;

		val = prinst->GetEdictFieldValue(prinst, (void*)ent, "chain", ev_entity, &menuc_eval.chain);
		if (val)
			val->edict = EDICT_TO_PROG(prinst, (void*)chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, (void*)chain);
}

void QCBUILTIN PF_etof(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = G_EDICTNUM(prinst, OFS_PARM0);
}
void QCBUILTIN PF_ftoe(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int entnum = G_FLOAT(OFS_PARM0);

	RETURN_EDICT(prinst, EDICT_NUM_UB(prinst, entnum));
}

void QCBUILTIN PF_IsNotNull(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int str = G_INT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = !!str;
}

//float 	altstr_count(string str) = #82;
//returns number of single quoted strings in the string.
void QCBUILTIN PF_altstr_count(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *s;
	int count = 0;
	s = PR_GetStringOfs(prinst, OFS_PARM0);
	for (;*s;s++)
	{
		if (*s == '\\')
		{
			if (!*++s)
				break;
		}
		else if (*s == '\'')
			count++;
	}
	G_FLOAT(OFS_RETURN) = count/2;
}
//string  altstr_prepare(string str) = #83;
void QCBUILTIN PF_altstr_prepare(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char outstr[8192], *out;
	const char *instr, *in;
	int size;

//	VM_SAFEPARMCOUNT( 1, VM_altstr_prepare );

	instr = PR_GetStringOfs(prinst, OFS_PARM0 );
	//VM_CheckEmptyString( instr );

	for( out = outstr, in = instr, size = sizeof(outstr) - 1 ; size && *in ; size--, in++, out++ )
	{
		if( *in == '\'' )
		{
			*out++ = '\\';
			*out = '\'';
			size--;
		}
		else
			*out = *in;
	}
	*out = 0;

	G_INT( OFS_RETURN ) = (int)PR_TempString( prinst, outstr );
}
//string  altstr_get(string str, float num) = #84;
void QCBUILTIN PF_altstr_get(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *altstr, *pos;
	char outstr[8192], *out;
	int count, size;

//	VM_SAFEPARMCOUNT( 2, VM_altstr_get );

	altstr = PR_GetStringOfs(prinst, OFS_PARM0 );
	//VM_CheckEmptyString( altstr );

	count = G_FLOAT( OFS_PARM1 );
	count = count * 2 + 1;

	for( pos = altstr ; *pos && count ; pos++ )
	{
		if( *pos == '\\' && !*++pos )
			break;
		else if( *pos == '\'' )
			count--;
	}

	if( !*pos )
	{
		G_INT( OFS_RETURN ) = (int)PR_SetString( prinst, "" );
		return;
	}

	for( out = outstr, size = sizeof(outstr) - 1 ; size && *pos ; size--, pos++, out++ )
	{
		if( *pos == '\\' )
		{
			if( !*++pos )
				break;
			*out = *pos;
			size--;
		}
		else if( *pos == '\'' )
			break;
		else
			*out = *pos;
	}

	*out = 0;
	G_INT( OFS_RETURN ) = (int)PR_TempString( prinst, outstr );
}
//string  altstr_set(string str, float num, string set) = #85
void QCBUILTIN PF_altstr_set(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int num;
	const char *altstr, *str;
	const char *in;
	char outstr[8192], *out;

//	VM_SAFEPARMCOUNT( 3, VM_altstr_set );

	altstr = PR_GetStringOfs(prinst, OFS_PARM0 );
	//VM_CheckEmptyString( altstr );

	num = G_FLOAT( OFS_PARM1 );

	str = PR_GetStringOfs(prinst, OFS_PARM2 );
	//VM_CheckEmptyString( str );

	out = outstr;
	for( num = num * 2 + 1, in = altstr; *in && num; *out++ = *in++ )
	{
		if( *in == '\\' && !*++in )
			break;
		else if( *in == '\'' )
			num--;
	}

	if( !in )
	{
		G_INT( OFS_RETURN ) = (int)PR_SetString( prinst, "" );
		return;
	}
	// copy set in
	for( ; *str; *out++ = *str++ )
		;
	// now jump over the old contents
	for( ; *in ; in++ )
	{
		if( *in == '\'' || (*in == '\\' && !*++in) )
			break;
	}

	if( !in ) {
		G_INT( OFS_RETURN ) = (int)PR_SetString( prinst, "" );
		return;
	}

	strcpy( out, in );
	G_INT( OFS_RETURN ) = (int)PR_TempString( prinst, outstr );

}

//string(string serveraddress) crypto_getkeyfp
void QCBUILTIN PF_crypto_getkeyfp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//string(string serveraddress) crypto_getidfp
void QCBUILTIN PF_crypto_getidfp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//float(string serveraddress) crypto_getidstatus
void QCBUILTIN PF_crypto_getidstatus(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//string(string serveraddress) crypto_getencryptlevel
void QCBUILTIN PF_crypto_getencryptlevel(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//string(float i) crypto_getmykeyfp
void QCBUILTIN PF_crypto_getmykeyfp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//string(float i) crypto_getmyidfp
void QCBUILTIN PF_crypto_getmyidfp(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}
//float(float i) PF_crypto_getmyidstatus
void QCBUILTIN PF_crypto_getmyidstatus(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//not supported.
	G_INT(OFS_RETURN) = 0;
}

static void QCBUILTIN PF_m_precache_model(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *modelname = PR_GetStringOfs(prinst, OFS_PARM0);
	Mod_ForName(modelname, MLV_WARN);
}
static model_t *QDECL MP_GetCModel(struct world_s *w, int modelindex)
{
	extern int		mod_numknown;
	modelindex--;
	if (modelindex < 0 || modelindex >= mod_numknown)
		return NULL;
	return &mod_known[modelindex];
}
static void QCBUILTIN PF_m_getmodelindex(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *modelname = PR_GetStringOfs(prinst, OFS_PARM0);
	model_t *m = Mod_ForName(modelname, MLV_WARN);
	if (m)
		G_FLOAT(OFS_RETURN) = (m-mod_known)+1;
	else
		G_FLOAT(OFS_RETURN) = 0;
}
static void QCBUILTIN PF_m_setmodel(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	const char *modelname = PR_GetStringOfs(prinst, OFS_PARM1);
	eval_t *modelval = prinst->GetEdictFieldValue(prinst, (void*)ent, "model", ev_string, &menuc_eval.model);
	eval_t *minsval = prinst->GetEdictFieldValue(prinst, (void*)ent, "mins", ev_vector, &menuc_eval.mins);
	eval_t *maxsval = prinst->GetEdictFieldValue(prinst, (void*)ent, "maxs", ev_vector, &menuc_eval.maxs);
	eval_t *modelidxval = prinst->GetEdictFieldValue(prinst, (void*)ent, "modelindex", ev_float, &menuc_eval.modelindex);
	model_t *mod = Mod_ForName(modelname, MLV_WARN);
	if (modelval)
		modelval->string = G_INT(OFS_PARM1);	//lets hope garbage collection is enough.
	else
		Con_Printf("PF_m_setmodel: no model field!\n");

	if (mod)
		while(mod->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);
	if (modelidxval)
		modelidxval->_float = mod?(mod-mod_known)+1:0;

	if (mod && minsval)
		VectorCopy(mod->mins, minsval->_vector);
	if (mod && maxsval)
		VectorCopy(mod->maxs, maxsval->_vector);
}
static void QCBUILTIN PF_m_setcustomskin(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	const char *fname = PR_GetStringOfs(prinst, OFS_PARM1);
	const char *skindata = PF_VarString(prinst, 2, pr_globals);
	eval_t *val = prinst->GetEdictFieldValue(prinst, (void*)ent, "skinobject", ev_string, &menuc_eval.skinobject);
	if (!val)
	{
		Con_Printf("PF_m_setcustomskin: no skinobject field!\n");
		return;
	}

	if (val->_float > 0)
	{
		Mod_WipeSkin(val->_float, false);
		val->_float = 0;
	}

	if (*fname || *skindata)
	{
		if (*skindata)
			val->_float = Mod_ReadSkinFile(fname, skindata);
		else
			val->_float = -(int)Mod_RegisterSkinFile(fname);
	}
}
//trivially basic
static void QCBUILTIN PF_m_setorigin(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	float *org = G_VECTOR(OFS_PARM1);
	eval_t *val = prinst->GetEdictFieldValue(prinst, (void*)ent, "origin", ev_vector, &menuc_eval.origin);
	if (val)
		VectorCopy(org, val->_vector);
	else
		Con_Printf("PF_m_setorigin: no origin field!\n");
}
static void QCBUILTIN PF_m_clearscene(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
//	CL_DecayLights ();

#if defined(SKELETALOBJECTS) || defined(RAGDOLLS)
	world_t *world = prinst->parms->user;
	if (world)
		skel_dodelete(world);
#endif
	CL_ClearEntityLists();

	V_ClearRefdef(&menuview);
	r_refdef.drawsbar = false;
	r_refdef.drawcrosshair = false;
	V_CalcRefdef(&menuview);	//set up the defaults
	r_refdef.flags |= RDF_NOWORLDMODEL;
}
static void QDECL MP_Read_FrameState(pubprogfuncs_t *prinst, wedict_t *ent, framestate_t *fstate)
{
	eval_t *frame1val = prinst->GetEdictFieldValue(prinst, (void*)ent, "frame", ev_float, &menuc_eval.frame1);
	eval_t *frame2val = prinst->GetEdictFieldValue(prinst, (void*)ent, "frame2", ev_float, &menuc_eval.frame2);
	eval_t *lerpfracval = prinst->GetEdictFieldValue(prinst, (void*)ent, "lerpfrac", ev_float, &menuc_eval.lerpfrac);
	eval_t *frame1timeval = prinst->GetEdictFieldValue(prinst, (void*)ent, "frame1time", ev_float, &menuc_eval.frame1time);
	eval_t *frame2timeval = prinst->GetEdictFieldValue(prinst, (void*)ent, "frame2time", ev_float, &menuc_eval.frame2time);
	eval_t *skelobjectval = prinst->GetEdictFieldValue(prinst, (void*)ent, "skeletonindex", ev_float, &menuc_eval.skelobject);

	fstate->g[FST_BASE].endbone = 0;
	fstate->g[FS_REG].endbone = 0x7fffffff;
	fstate->g[FS_REG].frame[0] = frame1val?frame1val->_float:0;
	fstate->g[FS_REG].frame[1] = frame2val?frame2val->_float:0;
	fstate->g[FS_REG].lerpweight[1] = lerpfracval?lerpfracval->_float:0;
	fstate->g[FS_REG].frametime[0] = frame1timeval?frame1timeval->_float:0;
	fstate->g[FS_REG].frametime[1] = frame2timeval?frame2timeval->_float:0;

#if FRAME_BLENDS >= 4
	fstate->g[FS_REG].frame[2] = fstate->g[FS_REG].frame[0];
	fstate->g[FS_REG].lerpweight[2] = 0;
	fstate->g[FS_REG].frame[3] = fstate->g[FS_REG].frame[0];
	fstate->g[FS_REG].lerpweight[3] = 0;
	fstate->g[FS_REG].lerpweight[0] = 1-(fstate->g[FS_REG].lerpweight[1]+fstate->g[FS_REG].lerpweight[2]+fstate->g[FS_REG].lerpweight[3]);
#else
	fstate->g[FS_REG].lerpweight[0] = 1-fstate->g[FS_REG].lerpweight[1];
#endif

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	fstate->bonecount = 0;
	fstate->bonestate = NULL;
	if (skelobjectval && skelobjectval->_float)
		skel_lookup(&menu_world, skelobjectval->_float, fstate);
#endif
}
static void QDECL MP_Get_FrameState(struct world_s *w, wedict_t *ent, framestate_t *fstate)
{
	MP_Read_FrameState(w->progs, ent, fstate);
}
static qboolean CopyMenuEdictToEntity(pubprogfuncs_t *prinst, menuedict_t *in, entity_t *out)
{
	eval_t *modelval = prinst->GetEdictFieldValue(prinst, (void*)in, "model", ev_string, &menuc_eval.model);
	eval_t *originval = prinst->GetEdictFieldValue(prinst, (void*)in, "origin", ev_vector, &menuc_eval.origin);
	eval_t *anglesval = prinst->GetEdictFieldValue(prinst, (void*)in, "angles", ev_vector, &menuc_eval.angles);
	eval_t *skinval = prinst->GetEdictFieldValue(prinst, (void*)in, "skin", ev_float, &menuc_eval.skin);
	eval_t *frame1val = prinst->GetEdictFieldValue(prinst, (void*)in, "frame", ev_float, &menuc_eval.frame1);
	eval_t *frame2val = prinst->GetEdictFieldValue(prinst, (void*)in, "frame2", ev_float, &menuc_eval.frame2);
	eval_t *lerpfracval = prinst->GetEdictFieldValue(prinst, (void*)in, "lerpfrac", ev_float, &menuc_eval.lerpfrac);
	eval_t *frame1timeval = prinst->GetEdictFieldValue(prinst, (void*)in, "frame1time", ev_float, &menuc_eval.frame1time);
	eval_t *frame2timeval = prinst->GetEdictFieldValue(prinst, (void*)in, "frame2time", ev_float, &menuc_eval.frame2time);
	eval_t *skelobjectval = prinst->GetEdictFieldValue(prinst, (void*)in, "skeletonindex", ev_float, &menuc_eval.skelobject);
	eval_t *colormapval = prinst->GetEdictFieldValue(prinst, (void*)in, "colormap", ev_float, &menuc_eval.colormap);
	eval_t *renderflagsval = prinst->GetEdictFieldValue(prinst, (void*)in, "renderflags", ev_float, &menuc_eval.renderflags);
	eval_t *skinobjectval = prinst->GetEdictFieldValue(prinst, (void*)in, "skinobject", ev_float, &menuc_eval.skinobject);
	eval_t *colourmodval = prinst->GetEdictFieldValue(prinst, (void*)in, "colormod", ev_vector, &menuc_eval.colourmod);
	eval_t *alphaval = prinst->GetEdictFieldValue(prinst, (void*)in, "alpha", ev_float, &menuc_eval.alpha);
	int ival;
	int rflags;

	rflags = renderflagsval?renderflagsval->_float:0;

	memset(out, 0, sizeof(*out));
	if (modelval)
		out->model = Mod_ForName(prinst->StringToNative(prinst, modelval->_int), MLV_WARN);
	if (originval)
		VectorCopy(originval->_vector, out->origin);
	if (!anglesval)anglesval = (eval_t*)vec3_origin;
	AngleVectors(anglesval->_vector, out->axis[0], out->axis[1], out->axis[2]);
	VectorInverse(out->axis[1]);

	out->scale = 1;
	out->skinnum = skinval?skinval->_float:0;
	out->framestate.g[FS_REG].frame[0] = frame1val?frame1val->_float:0;
	out->framestate.g[FS_REG].frame[1] = frame2val?frame2val->_float:0;
	out->framestate.g[FS_REG].lerpweight[1] = lerpfracval?lerpfracval->_float:0;
	out->framestate.g[FS_REG].lerpweight[0] = 1-out->framestate.g[FS_REG].lerpweight[1];
	out->framestate.g[FS_REG].frametime[0] = frame1timeval?frame1timeval->_float:0;
	out->framestate.g[FS_REG].frametime[1] = frame2timeval?frame2timeval->_float:0;

#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	out->framestate.bonecount = 0;
	out->framestate.bonestate = NULL;
	if (skelobjectval && skelobjectval->_float)
		skel_lookup(&menu_world, skelobjectval->_float, &out->framestate);
#endif

	out->customskin = skinobjectval?skinobjectval->_float:0;

	//FIXME: colourmap
	ival = colormapval?colormapval->_float:0;
	out->playerindex = -1;
	if (ival >= 1024)
	{
		//DP COLORMAP extension
		out->topcolour = (ival>>4) & 0x0f;
		out->bottomcolour = ival & 0xf;
	}
/*	else if (ival > 0 && ival <= MAX_CLIENTS)
	{	//FIXME: tie to the current skin/topcolor/bottomcolor cvars somehow?
		out->playerindex = ival - 1;
		out->topcolour = cl.players[ival-1].ttopcolor;
		out->bottomcolour = cl.players[ival-1].tbottomcolor;
	}*/
	else
	{
		out->topcolour = TOP_DEFAULT;
		out->bottomcolour = BOTTOM_DEFAULT;
	}

	VectorSet(out->glowmod, 1,1,1);
	if (!colourmodval || (!colourmodval->_vector[0] && !colourmodval->_vector[1] && !colourmodval->_vector[2]))
		VectorSet(out->shaderRGBAf, 1, 1, 1);
	else
	{
		out->flags |= RF_FORCECOLOURMOD;
		VectorCopy(colourmodval->_vector, out->shaderRGBAf);
	}
	if (!alphaval || !alphaval->_float || alphaval->_float == 1)
		out->shaderRGBAf[3] = 1.0f;
	else
	{
		out->flags |= RF_TRANSLUCENT;
		out->shaderRGBAf[3] = alphaval->_float;
	}

	if (rflags & CSQCRF_ADDITIVE)
		out->flags |= RF_ADDITIVE;
	if (rflags & CSQCRF_DEPTHHACK)
		out->flags |= RF_DEPTHHACK;

	if (out->model)
		return true;
	return false;
}
static void QCBUILTIN PF_m_addentity(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *in = (void*)G_EDICT(prinst, OFS_PARM0);
	entity_t ent;
	if (in->ereftype == ER_FREE || in->entnum == 0)
	{
		Con_Printf("Tried drawing a free/removed/world entity\n");
		return;
	}

	if (CopyMenuEdictToEntity(prinst, in, &ent))
		V_AddAxisEntity(&ent);
}
static void QCBUILTIN PF_m_addentity_lighting(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	menuedict_t *in = (void*)G_EDICT(prinst, OFS_PARM0);
	entity_t ent;
	if (in->ereftype == ER_FREE || in->entnum == 0)
	{
		Con_Printf("Tried drawing a free/removed/world entity\n");
		return;
	}

	if (CopyMenuEdictToEntity(prinst, in, &ent))
	{
		ent.light_known = true;
		VectorCopy(G_VECTOR(OFS_PARM1), ent.light_dir);
		VectorCopy(G_VECTOR(OFS_PARM2), ent.light_avg);
		VectorCopy(G_VECTOR(OFS_PARM3), ent.light_range);

		V_AddAxisEntity(&ent);
	}
}
static void QCBUILTIN PF_m_renderscene(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	V_ApplyRefdef();
	R_RenderView();
}
void QCBUILTIN PF_R_SetViewFlag(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_R_GetViewFlag(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);


static void QCBUILTIN PF_menu_cprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *str = PF_VarString(prinst, 0, pr_globals);
	SCR_CenterPrint(0, str, true);
}
static void QCBUILTIN PF_cl_changelevel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifndef CLIENTONLY
	const char *nextmap = PR_GetStringOfs(prinst, OFS_PARM0);
	if (sv.active || !cls.state)
	{
		char buf[1024];
		Cbuf_AddText(va("changelevel %s\n", COM_QuotedString(nextmap, buf, sizeof(buf), false)), RESTRICT_INSECURE);
	}
#endif
}
static void QCBUILTIN PF_crash (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int binum;
	char fname[MAX_QPATH];
	//allow people to rename it or whatever
	if (!prinst->GetBuiltinCallInfo(prinst, &binum, fname, sizeof(fname)))
	{
		binum = 0;
		strcpy(fname, "?unknown?");
	}

	prinst->RunError(prinst, "\n%s called", fname);
}
static void QCBUILTIN PF_stackdump (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	prinst->StackTrace(prinst, true);
}
#define PF_cl_clientcommand PF_Fixme
#define PF_altstr_ins PF_Fixme	//insert after, apparently

static void MP_ConsoleCommand_f(void)
{
	char cmd[2048];
	Q_snprintfz(cmd, sizeof(cmd), "%s %s", Cmd_Argv(0), Cmd_Args());
	MP_ConsoleCommand(cmd);
}
static void QCBUILTIN PF_menu_registercommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	const char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	const char *desc = (prinst->callargc>1)?PR_GetStringOfs(prinst, OFS_PARM1):NULL;
	if (desc && !*desc)
		desc = NULL;
	if (!Cmd_Exists(str))
		Cmd_AddCommandD(str, MP_ConsoleCommand_f, desc);
}

static void PF_m_clipboard_got(void *ctx, const char *utf8)
{
	void *pr_globals;
	unsigned int unicode;
	int error;

	while (*utf8)
	{
		unicode = utf8_decode(&error, utf8, &utf8);
		if (error)
			unicode = 0xfffdu;

		if (!menu_world.progs || !mpfuncs.inputevent)
			return;
	#ifdef TEXTEDITOR
		if (editormodal)
			return;
	#endif

		pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		G_FLOAT(OFS_PARM0) = CSIE_PASTE;
		G_FLOAT(OFS_PARM1) = 0;
		G_FLOAT(OFS_PARM2) = unicode;
		G_FLOAT(OFS_PARM3) = 0;

		qcinput_scan = G_FLOAT(OFS_PARM1);
		qcinput_unicode = G_FLOAT(OFS_PARM2);
		PR_ExecuteProgram (menu_world.progs, mpfuncs.inputevent);
		qcinput_scan = 0;	//and stop replay attacks
		qcinput_unicode = 0;
	}
}
static void QCBUILTIN PF_m_clipboard_get(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	clipboardtype_t cliptype = G_FLOAT(OFS_PARM0);
	Sys_Clipboard_PasteText(cliptype, PF_m_clipboard_got, prinst);
}

static struct {
	char *name;
	builtin_t bifunc;
	int ebfsnum;
}  BuiltinList[] = {
	{"checkextension",			PF_menu_checkextension,		1},
	{"checkbuiltin",			PF_checkbuiltin,			0},
	{"error",					PF_error,					2},
	{"objerror",				PF_nonfatalobjerror,		3},
	{"print",					PF_print,					4},
	{"bprint",					PF_cl_bprint,				5},
	{"msprint",					PF_cl_sprint,				6},
	{"cprint",					PF_menu_cprint,				7},
	{"normalize",				PF_normalize,				8},
	{"vlen",					PF_vlen,					9},
	{"vectoyaw",				PF_vectoyaw,				10},
	{"vectoangles",				PF_vectoangles,				11},
	{"crossproduct",			PF_crossproduct,			0},
	{"random",					PF_random,					12},
	{"localcmd",				PF_localcmd,				13},
	{"cvar",					PF_menu_cvar,				14},
	{"cvar_set",				PF_menu_cvar_set,			15},
	{"dprint",					PF_dprint,					16},
	{"ftos",					PF_ftos,					17},
	{"fabs",					PF_fabs,					18},
	{"vtos",					PF_vtos,					19},
	{"etos",					PF_etos,					20},
	{"stof",					PF_stof,					21},

	{"json_parse",				PF_json_parse,				0},
	{"json_free",				PF_memfree,					0},
	{"json_get_value_type",		PF_json_get_value_type,		0},
	{"json_get_integer",		PF_json_get_integer,		0},
	{"json_get_float",			PF_json_get_float,			0},
	{"json_get_string",			PF_json_get_string,			0},
	{"json_find_object_child",	PF_json_find_object_child,	0},
	{"json_get_length",			PF_json_get_length,			0},
	{"json_get_child_at_index",	PF_json_get_child_at_index,	0},
	{"json_get_name",			PF_json_get_name,			0},

	{"js_run_script",			PF_js_run_script,			0},
	
	{"stoi",					PF_stoi,					0},
	{"itos",					PF_itos,					0},
	{"stoh",					PF_stoh,					0},
	{"htos",					PF_htos,					0},
	{"ftoi",					PF_ftoi,					0},
	{"itof",					PF_itof,					0},
	{"ftou",					PF_ftou,					0},
	{"utof",					PF_utof,					0},

	{"spawn",					PF_Spawn,					22},
	{"remove",					PF_Remove_,					23},
	{"removeinstant",			PF_RemoveInstant,			0},
	{"find",					PF_FindString,				24},
	{"findfloat",				PF_FindFloat,				25},
	{"findentity",				PF_FindFloat,				25},
	{"findchain",				PF_menu_findchain,			26},
	{"findchainfloat",			PF_menu_findchainfloat,		27},
#ifdef QCGC
	{"find_list",				PF_FindList,				0},
#endif
	{"precache_file",			PF_CL_precache_file,		28},
	{"precache_sound",			PF_CL_precache_sound,		29},
	{"coredump",				PF_coredump,				30},
	{"traceon",					PF_traceon,					31},
	{"traceoff",				PF_traceoff,				32},
	{"eprint",					PF_eprint,					33},
	{"rint",					PF_rint,					34},
	{"floor",					PF_floor,					35},
	{"ceil",					PF_ceil,					36},
	{"nextent",					PF_nextent,					37},
	{"sin",						PF_Sin,						38},
	{"cos",						PF_Cos,						39},
	{"sqrt",					PF_Sqrt,					40},
	{"randomvec",				PF_randomvector,			41},
	{"registercvar",			PF_registercvar,			42},
	{"min",						PF_min,						43},
	{"max",						PF_max,						44},
	{"bound",					PF_bound,					45},
	{"pow",						PF_pow,						46},
	{"logarithm",				PF_Logarithm,				0},
	{"entityprotection",		PF_entityprotection,		0},
	{"copyentity",				PF_CopyEntity,				47},
	{"fopen",					PF_fopen,					48},
	{"fclose",					PF_fclose,					49},
	{"fgets",					PF_fgets,					50},
	{"fputs",					PF_fputs,					51},
	{"fread",					PF_fread,					0},
	{"fwrite",					PF_fwrite,					0},
	{"fseek",					PF_fseek32,					0},
	{"fsize",					PF_fsize32,					0},
	{"fseek64",					PF_fseek64,					0},
	{"fsize64",					PF_fsize64,					0},
	{"strlen",					PF_strlen,					52},
	{"strcat",					PF_strcat,					53},
	{"substring",				PF_substring,				54},
	{"stov",					PF_stov,					55},
	{"strzone",					PF_strzone,					56},
	{"strunzone",				PF_strunzone,				57},
#ifdef QCGC
	{"createbuffer",			PF_createbuffer,			0},
#endif
	{"tokenize",				PF_Tokenize,				58},
	{"argv",					PF_ArgV,					59},
	{"isserver",				PF_isserver,				60},
	{"clientcount",				PF_cl_clientcount,			61},						//float	clientcount(void)  = #61;
	{"clientstate",				PF_clientstate,				62},
	{"clientcommand",			PF_cl_clientcommand,		63},						//void	clientcommand(float client, string s)  = #63;
	{"changelevel",				PF_cl_changelevel,			64},						//void	changelevel(string map)  = #64;
	{"localsound",				PF_cl_localsound,			65},
	{"getmousepos",				PF_cl_getmousepos,			66},
	{"gettime",					PF_gettimef,				67},
	{"gettimed",				PF_gettimed,				0},
	{"loadfromdata",			PF_loadfromdata,			68},
	{"loadfromfile",			PF_loadfromfile,			69},
	{"mod",						PF_mod,						70},
	{"cvar_string",				PF_menu_cvar_string,		71},
	{"crash",					PF_crash,					72},				//void	crash(void)	= #72;
	{"stackdump",				PF_stackdump,				73},			//void	stackdump(void) = #73;
	{"search_begin",			PF_search_begin,			74},
	{"search_end",				PF_search_end,				75},
	{"search_getsize",			PF_search_getsize,			76},
	{"search_getfilename",		PF_search_getfilename,		77},
	{"search_getfilesize",		PF_search_getfilesize,		0},
	{"search_getfilemtime",		PF_search_getfilemtime,		0},
	{"search_getpackagename",	PF_search_getpackagename,	0},
	{"search_fopen",			PF_search_fopen,			0},
	{"chr2str",					PF_chr2str,					78},
	{"etof",					PF_etof,					79},
	{"ftoe",					PF_ftoe,					80},
	{"validstring",				PF_IsNotNull,				81},
	{"altstr_count",			PF_altstr_count, 			82},
	{"altstr_prepare",			PF_altstr_prepare, 			83},
	{"altstr_get",				PF_altstr_get,				84},
	{"altstr_set",				PF_altstr_set, 				85},
	{"altstr_ins",				PF_altstr_ins,				86},
	{"findflags",				PF_FindFlags,				87},
	{"findchainflags",			PF_menu_findchainflags,		88},
	{"cvar_defstring",			PF_cvar_defstring,			89},
	{"setmodel",				PF_m_setmodel,				90},
	{"precache_model",			PF_m_precache_model,		91},
	{"setorigin",				PF_m_setorigin,				92},
															//gap
	{"getmodelindex",			PF_m_getmodelindex,			200},
	{"externcall",				PF_externcall,				201},
	{"addprogs",				PF_cl_addprogs,				202},
	{"externvalue",				PF_externvalue,				203},
	{"externset",				PF_externset,				204},
															//gap
	{"fork",					PF_Fork,					210},
	{"abort",					PF_Abort,					211},
	{"sleep",					PF_Sleep,					212},
															//gap
	{"strstrofs",				PF_strstrofs,				221},
	{"str2chr",					PF_str2chr,					222},
	{"chr2str",					PF_chr2str,					223},
	{"strconv",					PF_strconv,					224},
	{"strpad",					PF_strpad,					225},
	{"infoadd",					PF_infoadd,					226},
	{"infoget",					PF_infoget,					227},
	{"strcmp",					PF_strncmp,					228},
	{"strncmp",					PF_strncmp,					228},
	{"strcasecmp",				PF_strncasecmp,				229},
	{"strncasecmp",				PF_strncasecmp,				230},
	{"strtrim",					PF_strtrim,					0},
															//gap
	{"shaderforname",			PF_shaderforname,			238},
	{"sendpacket",				PF_cl_SendPacket,			242},
															//gap
	{"skel_create",				PF_skel_create,				263},//float(float modlindex) skel_create = #263; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_build",				PF_skel_build,				264},//float(float skel, entity ent, float modelindex, float retainfrac, float firstbone, float lastbone, optional float addition) skel_build = #264; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_build_ptr",			PF_skel_build_ptr,			0},//float(float skel, int numblends, __variant *blends, int blendsize) skel_build_ptr = #0;
	{"skel_get_numbones",		PF_skel_get_numbones,		265},//float(float skel) skel_get_numbones = #265; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_get_bonename",		PF_skel_get_bonename,		266},//string(float skel, float bonenum) skel_get_bonename = #266; // (FTE_CSQC_SKELETONOBJECTS) (returns tempstring)
	{"skel_get_boneparent",		PF_skel_get_boneparent,		267},//float(float skel, float bonenum) skel_get_boneparent = #267; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_find_bone",			PF_skel_find_bone,			268},//float(float skel, string tagname) skel_get_boneidx = #268; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_get_bonerel",		PF_skel_get_bonerel,		269},//vector(float skel, float bonenum) skel_get_bonerel = #269; // (FTE_CSQC_SKELETONOBJECTS) (sets v_forward etc)
	{"skel_get_boneabs",		PF_skel_get_boneabs,		270},//vector(float skel, float bonenum) skel_get_boneabs = #270; // (FTE_CSQC_SKELETONOBJECTS) (sets v_forward etc)
	{"skel_set_bone",			PF_skel_set_bone,			271},//void(float skel, float bonenum, vector org) skel_set_bone = #271; // (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
	{"skel_premul_bone",		PF_skel_premul_bone,		272},//void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
	{"skel_premul_bones",		PF_skel_premul_bones,		273},//void(float skel, float startbone, float endbone, vector org) skel_mul_bone = #273; // (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
	{"skel_postmul_bone",		PF_skel_postmul_bone,		0},//void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
//	{"skel_postmul_bones",		PF_skel_postmul_bones,		0},//void(float skel, float startbone, float endbone, vector org) skel_mul_bone = #273; // (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
	{"skel_copybones",			PF_skel_copybones,			274},//void(float skeldst, float skelsrc, float startbone, float entbone) skel_copybones = #274; // (FTE_CSQC_SKELETONOBJECTS)
	{"skel_delete",				PF_skel_delete,				275},//void(float skel) skel_delete = #275; // (FTE_CSQC_SKELETONOBJECTS)
	{"frameforname",			PF_frameforname,			276},//float(float modidx, string framename) frameforname = #276 (FTE_CSQC_SKELETONOBJECTS)
	{"frameduration",			PF_frameduration,			277},//float(float modidx, float framenum) frameduration = #277 (FTE_CSQC_SKELETONOBJECTS)
	{"frameforaction",			PF_frameforaction,			0},//float(float modidx, int actionid) frameforaction = #0
	{"processmodelevents",		PF_processmodelevents,		0},
	{"getnextmodelevent",		PF_getnextmodelevent,		0},
	{"getmodeleventidx",		PF_getmodeleventidx,		0},
	{"frametoname",				PF_frametoname,				0},//string(float modidx, float framenum) frametoname
	{"modelframecount",			PF_modelframecount,			0},
															//gap
	{"hash_createtab",			PF_hash_createtab,			287},
	{"hash_destroytab",			PF_hash_destroytab,			288},
	{"hash_add",				PF_hash_add,				289},
	{"hash_get",				PF_hash_get,				290},
	{"hash_delete",				PF_hash_delete,				291},
	{"hash_getkey",				PF_hash_getkey,				292},
	{"hash_getcb",				PF_hash_getcb,				293},
	{"checkcommand",			PF_checkcommand,			294},
	{"argescape",				PF_argescape,				295},
															//gap
	{"clearscene",				PF_m_clearscene,			300},
//	{"addentities",				PF_Fixme,					301},
	{"addentity",				PF_m_addentity,				302},//FIXME: needs setmodel, origin, angles, colormap(eep), frame etc, skin, 
	{"addentity_lighting",		PF_m_addentity_lighting,	0},
#ifdef CSQC_DAT
	{"setproperty",				PF_R_SetViewFlag,			303},//should be okay to share
#endif
	{"renderscene",				PF_m_renderscene,			304},//too module-specific
//	{"dynamiclight_add",		PF_R_DynamicLight_Add,		305},//should be okay to share
	{"R_BeginPolygon",			PF_R_PolygonBegin,			306},//useful for 2d stuff
	{"R_PolygonVertex",			PF_R_PolygonVertex,			307},
	{"R_EndPolygon",			PF_R_PolygonEnd,			308},
#ifdef CSQC_DAT
	{"getproperty",				PF_R_GetViewFlag,			309},//should be okay to share
#endif
//unproject													310
//project													311

#ifdef CSQC_DAT
	{"r_uploadimage",			PF_CL_uploadimage,			0},
#endif
	{"r_readimage",				PF_CL_readimage,			0},


	{"print_csqc",				PF_print,					339},
	{"setwatchpoint",			PF_setwatchpoint,			0},
	{"keynumtostring_csqc",		PF_cl_keynumtostring,		340},
	{"stringtokeynum_csqc",		PF_cl_stringtokeynum,		341},
	{"getkeybind",				PF_cl_getkeybind,			342},
	{"setcursormode",			PF_cl_setcursormode,		343},
	{"getcursormode",			PF_cl_getcursormode,		0},	
	{"setmousepos",				PF_cl_setmousepos,			0},


	{"clipboard_get",			PF_m_clipboard_get,			0},
	{"clipboard_set",			PF_cl_clipboard_set,		0},

//	{NULL,						PF_Fixme,					344},
//	{NULL,						PF_Fixme,					345},
//	{NULL,						PF_Fixme,					346},
//	{NULL,						PF_Fixme,					347},
//	{NULL,						PF_Fixme,					348},
	{"setlocaluserinfo",		PF_cl_setlocaluserinfo,			0},
	{"getlocaluserinfo",		PF_cl_getlocaluserinfostring,	0},
	{"setlocaluserinfoblob",	PF_cl_setlocaluserinfo,			0},
	{"getlocaluserinfoblob",	PF_cl_getlocaluserinfoblob,		0},
	{"isdemo",					PF_isdemo,					349},
//	{NULL,						PF_Fixme,					350},
//	{NULL,						PF_Fixme,					351},
	{"queueaudio",				PF_cl_queueaudio,			0},
	{"getqueuedaudiotime",		PF_cl_getqueuedaudiotime,	0},
	{"registercommand",			PF_menu_registercommand,	352},
	{"wasfreed",				PF_WasFreed,				353},
	{"serverkey",				PF_cl_serverkey,			354},	// #354 string(string key) serverkey;
	{"serverkeyfloat",			PF_cl_serverkeyfloat,		0},		// #0 float(string key) serverkeyfloat;
	{"serverkeyblob",			PF_cl_serverkeyblob,		0},
#ifdef HAVE_MEDIA_DECODER
	{"videoplaying",			PF_cs_media_getstate,		355},
#endif
	{"findfont",				PF_CL_findfont,				356},
	{"loadfont",				PF_CL_loadfont,				357},
															//gap
//	{"dynamiclight_get",		PF_R_DynamicLight_Get,		372},
//	{"dynamiclight_set",		PF_R_DynamicLight_Set,		373},
//	{NULL,						PF_Fixme,					374},
//	{NULL,						PF_Fixme,					375},
	{"setcustomskin",			PF_m_setcustomskin,			376},
															//gap
	{"memalloc",				PF_memalloc,				384},
	{"memrealloc",				PF_memrealloc,				0},
	{"memfree",					PF_memfree,					385},
	{"memcmp",					PF_memcmp,					0},
	{"memcpy",					PF_memcpy,					386},
	{"memfill8",				PF_memfill8,				387},
	{"memgetval",				PF_memgetval,				388},
	{"memsetval",				PF_memsetval,				389},
	{"memptradd",				PF_memptradd,				390},
	{"memstrsize",				PF_memstrsize,				0},
	{"con_getset",				PF_SubConGetSet,			391},
	{"con_printf",				PF_SubConPrintf,			392},
	{"con_draw",				PF_SubConDraw,				393},
	{"con_input",				PF_SubConInput,				394},
	{"setwindowcaption",		PF_cl_setwindowcaption,		0},
	{"cvars_haveunsaved",		PF_cvars_haveunsaved,		0},
															//gap
//	{"writebyte,				PF_Fixme,					401},
//	{"writechar,				PF_Fixme,					402},
//	{"writeshort,				PF_Fixme,					403},
//	{"writelong,				PF_Fixme,					404},
//	{"writeangle,				PF_Fixme,					405},
//	{"writecoord,				PF_Fixme,					406},
//	{"writestring,				PF_Fixme,					407},
//	{"writeentity,				PF_Fixme,					408},
															//gap
	{"buf_create",				PF_buf_create,				440},
	{"buf_del",					PF_buf_del,					441},
	{"buf_getsize",				PF_buf_getsize,				442},
	{"buf_copy",				PF_buf_copy,				443},
	{"buf_sort",				PF_buf_sort,				444},
	{"buf_implode",				PF_buf_implode,				445},
	{"bufstr_get",				PF_bufstr_get,				446},
	{"bufstr_set",				PF_bufstr_set,				447},
	{"bufstr_add",				PF_bufstr_add,				448},
	{"bufstr_free",				PF_bufstr_free,				449},
//	{NULL,						PF_Fixme,					450},
	{"iscachedpic",				PF_CL_is_cached_pic,		451},
	{"precache_pic",			PF_CL_precache_pic,			452},
	{"free_pic",				PF_CL_free_pic,				453},
	{"drawcharacter",			PF_CL_drawcharacter,		454},
	{"drawrawstring",			PF_CL_drawrawstring,		455},
	{"drawpic",					PF_CL_drawpic,				456},
	{"drawrotpic",				PF_CL_drawrotpic,			0},
	{"drawfill",				PF_CL_drawfill,				457},
	{"drawsetcliparea",			PF_CL_drawsetcliparea,		458},
	{"drawresetcliparea",		PF_CL_drawresetcliparea,	459},
	{"drawgetimagesize",		PF_CL_drawgetimagesize,		460},
#ifdef HAVE_MEDIA_DECODER
	{"cin_open",				PF_cs_media_create,			461},
	{"cin_close",				PF_cs_media_destroy,		462},
	{"cin_setstate",			PF_cs_media_setstate,		463},
	{"cin_getstate",			PF_cs_media_getstate,		464},
	{"cin_restart",				PF_cs_media_restart, 		465},
#endif
	{"drawline",				PF_CL_drawline,				466},
	{"drawstring",				PF_CL_drawcolouredstring,	467},
	{"stringwidth",				PF_CL_stringwidth,			468},
	{"drawsubpic",				PF_CL_drawsubpic,			469},
	{"drawrotsubpic",			PF_CL_drawrotsubpic,		0},
	{"drawtextfield",			PF_CL_DrawTextField,		0},

#ifdef HAVE_LEGACY
	{"drawrotpic_dp",			PF_CL_drawrotpic_dp,		470},
#endif
//MERGES WITH CLIENT+SERVER BUILTIN MAPPINGS BELOW
	{"asin",					PF_asin,					471},
	{"acos",					PF_acos,					472},
	{"atan",					PF_atan,					473},
	{"atan2",					PF_atan2,					474},
	{"tan",						PF_tan,						475},
	{"strlennocol",				PF_strlennocol,				476},
	{"strdecolorize",			PF_strdecolorize,			477},
	{"strftime",				PF_strftime,				478},
	{"tokenizebyseparator",		PF_tokenizebyseparator,		479},
	{"strtolower",				PF_strtolower,				480},
	{"strtoupper",				PF_strtoupper,				481},
	{"csqc_cvar_defstring",		PF_cvar_defstring,			482},
//	{NULL,						PF_Fixme,					483},
	{"strreplace",				PF_strreplace,				484},
	{"strireplace",				PF_strireplace,				485},
//	{NULL,						PF_Fixme,					486},
#ifdef HAVE_MEDIA_DECODER
	{"gecko_create",			PF_cs_media_create,			487},
	{"gecko_destroy",			PF_cs_media_destroy,		488},
	{"gecko_navigate",			PF_cs_media_command,		489},
	{"gecko_keyevent",			PF_cs_media_keyevent,		490},
	{"gecko_mousemove",			PF_cs_media_mousemove,		491},
	{"gecko_resize",			PF_cs_media_resize,			492},
	{"gecko_get_texture_extent",PF_cs_media_get_texture_extent,493},
	{"gecko_getproperty",		PF_cs_media_getproperty,	0},
#endif
	{"crc16",					PF_crc16,					494},
	{"cvar_type",				PF_cvar_type,				495},
	{"numentityfields",			PF_numentityfields,			496},
	{"findentityfield",			PF_findentityfield,			0},
	{"entityfieldref",			PF_entityfieldref,			0},
	{"entityfieldname",			PF_entityfieldname,			497},
	{"entityfieldtype",			PF_entityfieldtype,			498},
	{"getentityfieldstring",	PF_getentityfieldstring,	499},
	{"putentityfieldstring",	PF_putentityfieldstring,	500},
//	{NULL,						PF_Fixme,					501},
//	{NULL,						PF_Fixme,					502},
	{"whichpack",				PF_whichpack,				503},
															//gap
	{"uri_escape",				PF_uri_escape,				510},
	{"uri_unescape",			PF_uri_unescape,			511},
	{"base64encode",			PF_base64encode,			0},
	{"base64decode",			PF_base64decode,			0},
	{"num_for_edict",			PF_etof,					512},
	{"uri_get",					PF_uri_get,					513},
	{"uri_post",				PF_uri_get,					513},
	{"tokenize_console",		PF_tokenize_console,		514},
	{"argv_start_index",		PF_argv_start_index,		515},
	{"argv_end_index",			PF_argv_end_index,			516},
	{"buf_cvarlist",			PF_buf_cvarlist,			517},
	{"cvar_description",		PF_cvar_description,		518},
															//gap
	{"log",						PF_Logarithm,				532},
//	{"getsoundtime",			PF_Fixme,					533},
	{"soundlength",				PF_soundlength,				534},
	{"buf_loadfile",			PF_buf_loadfile,			535},
	{"buf_writefile",			PF_buf_writefile,			536},
	{"bufstr_find",				PF_bufstr_find,				537},
//	{"matchpattern",			PF_Fixme,					538},
															//gap
	{"setkeydest",				PF_cl_setkeydest,			601},
	{"getkeydest",				PF_cl_getkeydest,			602},
	{"setmousetarget",			PF_cl_setmousetarget,		603},
	{"getmousetarget",			PF_cl_getmousetarget,		604},
	{"callfunction",			PF_callfunction,			605},
	{"writetofile",				PF_writetofile,				606},
	{"isfunction",				PF_isfunction,				607},
	{"getresolution",			PF_cl_getresolution,		608},
	{"keynumtostring",			PF_cl_keynumtostring,		609},
	{"findkeysforcommand",		PF_cl_findkeysforcommand,	610},
	{"findkeysforcommandex",	PF_cl_findkeysforcommandex,	0},
	{"gethostcachevalue",		PF_cl_gethostcachevalue,	611},
	{"gethostcachestring",		PF_cl_gethostcachestring,	612},
	{"parseentitydata",			PF_parseentitydata,			613},
	{"generateentitydata",		PF_generateentitydata,		0},

	{"stringtokeynum",			PF_cl_stringtokeynum,		614},

	{"resethostcachemasks",		PF_cl_resethostcachemasks,	615},
	{"sethostcachemaskstring",	PF_cl_sethostcachemaskstring,616},
	{"sethostcachemasknumber",	PF_cl_sethostcachemasknumber,617},
	{"resorthostcache",			PF_cl_resorthostcache,		618},
	{"sethostcachesort",		PF_cl_sethostcachesort,		619},
	{"refreshhostcache",		PF_cl_refreshhostcache,		620},
	{"gethostcachenumber",		PF_cl_gethostcachenumber,	621},
	{"gethostcacheindexforkey",	PF_cl_gethostcacheindexforkey,622},
	{"addwantedhostcachekey",	PF_cl_addwantedhostcachekey,623},
#ifdef CL_MASTER
	{"getextresponse",			PF_cl_getextresponse,		624},
#endif
	{"netaddress_resolve",		PF_netaddress_resolve,		625},
	{"getgamedirinfo",			PF_cl_getgamedirinfo,		626},
#ifdef PACKAGEMANAGER
	{"getpackagemanagerinfo",	PF_cl_getpackagemanagerinfo,0},
#endif
	{"sprintf",					PF_sprintf,					627},
//	{NULL,						PF_Fixme,					628},
//	{NULL,						PF_Fixme,					629},
	{"setkeybind",				PF_cl_setkeybind,			630},
	{"getbindmaps",				PF_cl_GetBindMap,			631},
	{"setbindmaps",				PF_cl_SetBindMap,			632},
	{"crypto_getkeyfp",			PF_crypto_getkeyfp,			633},
	{"crypto_getidfp",			PF_crypto_getidfp,			634},
	{"crypto_getencryptlevel",	PF_crypto_getencryptlevel,	635},
	{"crypto_getmykeyfp",		PF_crypto_getmykeyfp,		636},
	{"crypto_getmyidfp",		PF_crypto_getmyidfp,		637},
//	{NULL,						PF_Fixme,					638},
	{"digest_hex",				PF_digest_hex,				639},
	{"digest_ptr",				PF_digest_ptr,				0},
//	{NULL,						PF_Fixme,					640},
	{"crypto_getmyidstatus",	PF_crypto_getmyidstatus,	641},
//	{"coverage",				PF_Fixme,					642},
	{"crypto_getidstatus",		PF_crypto_getidstatus,		643},
//	{NULL,						PF_Fixme,					644},
//	{NULL,						PF_Fixme,					645},
//	{NULL,						PF_Fixme,					646},
//	{NULL,						PF_Fixme,					647},
//	{NULL,						PF_Fixme,					648},
//	{NULL,						PF_Fixme,					649},
	{"fcopy",					PF_fcopy,					650},
	{"frename",					PF_frename,					651},
	{"fremove",					PF_fremove,					652},
	{"fexists",					PF_fexists,					653},
	{"rmtree",					PF_rmtree,					654},

	{"stachievement_unlock",	PF_Fixme,					730},	// EXT_STEAM_REKI
	{"stachievement_query",		PF_Fixme,					731},	// EXT_STEAM_REKI
	{"ststat_setvalue",			PF_Fixme,					732},	// EXT_STEAM_REKI
	{"ststat_increment",		PF_Fixme,					733},	// EXT_STEAM_REKI
	{"ststat_query",			PF_Fixme,					734},	// EXT_STEAM_REKI
	{"stachievement_register",	PF_Fixme,					735},	// EXT_STEAM_REKI
	{"ststat_register",			PF_Fixme,					736},	// EXT_STEAM_REKI
	{"controller_query",		PF_cl_gp_querywithcb,		740},	// EXT_CONTROLLER_REKI
	{"controller_rumble",		PF_cl_gp_rumble,			741},	// EXT_CONTROLLER_REKI
	{"controller_rumbletriggers",PF_cl_gp_rumbletriggers,	742},	// EXT_CONTROLLER_REKI

	{"gp_getlayout",			PF_cl_gp_getbuttontype,		0}, // #0 float(float devid) gp_getbuttontype
	{"gp_rumble",				PF_cl_gp_rumble,			0}, // #0 void(float devid, float amp_low, float amp_high, float duration) gp_rumble
	{"gp_rumbletriggers",		PF_cl_gp_rumbletriggers,	0}, // #0 void(float devid, float left, float right, float duration) gp_rumbletriggers
	{"gp_setledcolor",			PF_cl_gp_setledcolor,		0}, // #0 void(float devid, float red, float green, float blue) gp_setledcolor
	{"gp_settriggerfx",			PF_cl_gp_settriggerfx,		0}, // #0 void(float devid, const void *data, int size) gp_settriggerfx

	{NULL}
};
static builtin_t menu_builtins[1024];


int MP_BuiltinValid(const char *name, int num)
{
	int i;
	for (i = 0; BuiltinList[i].name; i++)
	{
		if (BuiltinList[i].ebfsnum == num)
		{
			if (!strcmp(BuiltinList[i].name, name))
			{
				if (/*BuiltinList[i].bifunc == PF_NoMenu ||*/ BuiltinList[i].bifunc == PF_Fixme)
					return false;
				else
					return true;
			}
		}
	}
	return false;
}

static void MP_SetupBuiltins(void)
{
	int i;
	for (i = 0; i < sizeof(menu_builtins)/sizeof(menu_builtins[0]); i++)
		menu_builtins[i] = PF_Fixme;
	for (i = 0; BuiltinList[i].bifunc; i++)
	{
		if (BuiltinList[i].ebfsnum)
			menu_builtins[BuiltinList[i].ebfsnum] = BuiltinList[i].bifunc;
	}
}

static int PDECL PR_Menu_MapNamedBuiltin(pubprogfuncs_t *progfuncs, int headercrc, const char *builtinname)
{
	int i, binum;
	for (i = 0;BuiltinList[i].name;i++)
	{
		if (!strcmp(BuiltinList[i].name, builtinname) && BuiltinList[i].bifunc != PF_Fixme)
		{
			for (binum = sizeof(menu_builtins)/sizeof(menu_builtins[0]); --binum; )
			{
				if (menu_builtins[binum] && menu_builtins[binum] != PF_Fixme && BuiltinList[i].bifunc)
					continue;
				menu_builtins[binum] = BuiltinList[i].bifunc;
				return binum;
			}
			Con_Printf("No more builtin slots to allocate for %s\n", builtinname);
			break;
		}
	}
	Con_DPrintf("Unknown menu builtin: %s\n", builtinname);
	return 0;
}

static qboolean MP_MouseMove(menu_t *menu, qboolean isabs, unsigned int devid, float xdelta, float ydelta)
{
	void *pr_globals;

	if (!menu_world.progs || !mpfuncs.inputevent)
		return false;

	if (setjmp(mp_abort))
		return false;
	inmenuprogs++;
	pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
	G_FLOAT(OFS_PARM0) = isabs?CSIE_MOUSEABS:CSIE_MOUSEDELTA;
	G_FLOAT(OFS_PARM1) = (xdelta * vid.width) / vid.pixelwidth;
	G_FLOAT(OFS_PARM2) = (ydelta * vid.height) / vid.pixelheight;
	G_FLOAT(OFS_PARM3) = devid;
	PR_ExecuteProgram (menu_world.progs, mpfuncs.inputevent);
	if (R2D_Flush)
		R2D_Flush();
	inmenuprogs--;
	return G_FLOAT(OFS_RETURN);
}

static qboolean MP_JoystickAxis(menu_t *menu, unsigned int devid, int axis, float value)
{
	void *pr_globals;
	if (!menu_world.progs || !mpfuncs.inputevent)
		return false;
	if (setjmp(mp_abort))
		return false;
	inmenuprogs++;
	pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
	G_FLOAT(OFS_PARM0) = CSIE_JOYAXIS;
	G_FLOAT(OFS_PARM1) = axis;
	G_FLOAT(OFS_PARM2) = value;
	G_FLOAT(OFS_PARM3) = devid;
	PR_ExecuteProgram (menu_world.progs, mpfuncs.inputevent);
	if (R2D_Flush)
		R2D_Flush();
	inmenuprogs--;
	return G_FLOAT(OFS_RETURN);
}
static qboolean MP_KeyEvent(menu_t *menu, qboolean isdown, unsigned int devid, int key, int unicode)
{
	qboolean result;

#ifdef TEXTEDITOR
	if (editormodal)
		return false;
#endif

	if (setjmp(mp_abort))
		return true;

	if (isdown)
	{
#ifndef NOBUILTINMENUS
#ifdef _DEBUG
		if (key == 'c')
		{
			if ((keydown[K_LCTRL] || keydown[K_RCTRL]) && (keydown[K_LSHIFT] || keydown[K_RSHIFT]))
			{
				MP_Shutdown();
				M_Init_Internal();
				return true;
			}
		}
#endif
#endif

		mpkeysdown[key>>3] |= (1<<(key&7));
	}
	else
	{	//don't fire up events if it was not actually pressed.
		if (key && !(mpkeysdown[key>>3] & (1<<(key&7))))
			return false;
		mpkeysdown[key>>3] &= ~(1<<(key&7));
	}

	menutime = Sys_DoubleTime();
	if (menu_world.g.time)
		*menu_world.g.time = menutime;

	inmenuprogs++;
	if (mpfuncs.inputevent)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		G_FLOAT(OFS_PARM0) = isdown?CSIE_KEYDOWN:CSIE_KEYUP;
		G_FLOAT(OFS_PARM1) = MP_TranslateFTEtoQCCodes(key);
		G_FLOAT(OFS_PARM2) = unicode;
		G_FLOAT(OFS_PARM3) = devid;
		if (isdown)
		{
			qcinput_scan = G_FLOAT(OFS_PARM1);
			qcinput_unicode = G_FLOAT(OFS_PARM2);
		}
		PR_ExecuteProgram(menu_world.progs, mpfuncs.inputevent);
		result = G_FLOAT(OFS_RETURN);
		if (!result && key == K_TOUCH)
		{	//convert touches to mouse, for compat. gestures are untranslated but expected to be ignored.
			G_FLOAT(OFS_PARM0) = isdown?CSIE_KEYDOWN:CSIE_KEYUP;
			G_FLOAT(OFS_PARM1) = MP_TranslateFTEtoQCCodes(K_MOUSE1);
			G_FLOAT(OFS_PARM2) = unicode;
			G_FLOAT(OFS_PARM3) = devid;
			PR_ExecuteProgram(menu_world.progs, mpfuncs.inputevent);
			result = G_FLOAT(OFS_RETURN);
		}
		qcinput_scan = 0;
		qcinput_unicode = 0;
	}
	else if (isdown && mpfuncs.keydown)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		if (key == K_TOUCH)
			key = K_MOUSE1;	//old api doesn't expect touches nor provides feedback for emulation. make sure stuff works.
		G_FLOAT(OFS_PARM0) = MP_TranslateFTEtoQCCodes(key);
		G_FLOAT(OFS_PARM1) = unicode;
		PR_ExecuteProgram(menu_world.progs, mpfuncs.keydown);
		result = true;	//doesn't have a return value, so if the menu is set up for key events, all events are considered eaten.
	}
	else if (!isdown && mpfuncs.keyup)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		if (key == K_TOUCH)
			key = K_MOUSE1;	//old api doesn't expect touches.
		G_FLOAT(OFS_PARM0) = MP_TranslateFTEtoQCCodes(key);
		G_FLOAT(OFS_PARM1) = unicode;
		PR_ExecuteProgram(menu_world.progs, mpfuncs.keyup);
		result = false; // doesn't have a return value, so don't block it
	}
	else
		result = false;
	inmenuprogs--;

	if (R2D_Flush)	//shouldn't be needed, but in case the mod is buggy.
		R2D_Flush();
	return result;
}

void MP_Shutdown (void)
{
	func_t temp;
	if (!menu_world.progs)
		return;

	menuqc.release = NULL; //don't notify
	Menu_Unlink(&menuqc, false);
/*
	{
		char *buffer;
		int size = 1024*1024*8;
		buffer = Z_Malloc(size);
		menuprogs->save_ents(menuprogs, buffer, &size, 1);
		COM_WriteFile("menucore.txt", buffer, size);
		Z_Free(buffer);
	}
*/
	temp = mpfuncs.shutdown;
	mpfuncs.shutdown = 0;
	if (temp && !inmenuprogs)
		PR_ExecuteProgram(menu_world.progs, temp);

	PR_Common_Shutdown(menu_world.progs, false);
	menu_world.progs->Shutdown(menu_world.progs);
	memset(&menu_world, 0, sizeof(menu_world));
	PR_ReleaseFonts(kdm_menu);

#ifdef CL_MASTER
	Master_ClearMasks();
#endif

	Cmd_RemoveCommands(MP_ConsoleCommand_f);

	Key_Dest_Remove(kdm_menu);
	key_dest_absolutemouse &= ~kdm_menu;
}

static void MP_TryRelease(menu_t *m, qboolean force)
{
	if (inmenuprogs)
		return;	//if the qc asked for it, the qc probably already knows about it. don't recurse.
	MP_Toggle(0);

	if (force && Menu_IsLinked(m))
	{
		Con_Printf(CON_ERROR"Menuqc retook key grabs when ordered to yield. Killing.\n");
		MP_Shutdown();
		M_Init_Internal();
	}
}

void *VARGS PR_CB_Malloc(int size);	//these functions should be tracked by the library reliably, so there should be no need to track them ourselves.
void VARGS PR_CB_Free(void *mem);

//Any menu builtin error or anything like that will come here.
void VARGS Menu_Abort (const char *format, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
	vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	Con_Printf("Menu_Abort: %s\nShutting down menu.dat\n", string);

	if (pr_menu_coreonerror.value)
	{
		char *buffer;
		size_t size = 1024*1024*8;
		buffer = Z_Malloc(size);
		menu_world.progs->save_ents(menu_world.progs, buffer, &size, size, 3);
		COM_WriteFile("menucore.txt", FS_GAMEONLY, buffer, size);
		Z_Free(buffer);
	}

	MP_Shutdown();
	M_Init_Internal();

	if (inmenuprogs)	//something in the menu caused the problem, so...
	{
		inmenuprogs = 0;
		longjmp(mp_abort, 1);
	}
}

void MP_CvarChanged(cvar_t *var)
{
	if (menu_world.progs)
	{
		PR_AutoCvar(menu_world.progs, var);
	}
}

pbool PDECL Menu_CheckHeaderCrc(pubprogfuncs_t *inst, progsnum_t idx, int crc, const char *filename)
{
	if (crc == 10020)
		return true;	//its okay
	Con_Printf("progs crc is invalid for %s\n", filename);
	if (crc == 12776)
	{	//whoever wrote wrath fucked up.
		Con_Printf("(please correct .src include orders)\n");
		return true;
	}
	return false;
}

static void *PDECL MP_PRReadFile (const char *path, qbyte *(PDECL *buf_get)(void *buf_ctx, size_t size), void *buf_ctx, size_t *size, pbool issource)
{
	flocation_t loc;
	if (FS_FLocateFile(path, FSLF_IFFOUND|FSLF_SECUREONLY, &loc))
	{
		qbyte *buffer = NULL;
		vfsfile_t *file = FS_OpenReadLocation(path, &loc);
		if (file)
		{
			*size = loc.len;
			buffer = buf_get(buf_ctx, *size);
			if (buffer)
				VFS_READ(file, buffer, *size);
			VFS_CLOSE(file);
		}
		return buffer;
	}
	else
	{
		if (FS_FLocateFile(path, FSLF_IFFOUND, &loc))
			Con_Printf("Not loading %s because it comes from an untrusted source\n", path);
		return NULL;
	}
}
static int PDECL MP_PRFileSize (const char *path)
{
	flocation_t loc;
	if (FS_FLocateFile(path, FSLF_IFFOUND|FSLF_SECUREONLY, &loc))
		return loc.len;
	else
		return -1;
}

qboolean MP_Init (void)
{
	struct key_cursor_s *m = &key_customcursor[kc_menuqc];

	if (qrenderer == QR_NONE)
	{
		return false;
	}

	if (forceqmenu.value)
	{
		Con_DPrintf("menu.dat disabled\n");
		return false;
	}

	MP_SetupBuiltins();

	memset(&menuc_eval, 0, sizeof(menuc_eval));


	menuprogparms.progsversion = PROGSTRUCT_VERSION;
	menuprogparms.ReadFile = MP_PRReadFile;//char *(*ReadFile) (char *fname, void *buffer, int *len);
	menuprogparms.FileSize = MP_PRFileSize;//int (*FileSize) (char *fname);	//-1 if file does not exist
	menuprogparms.WriteFile = QC_WriteFile;//bool (*WriteFile) (char *name, void *data, int len);
	menuprogparms.Printf = PR_Printf;//Con_Printf;//void (*printf) (char *, ...);
	menuprogparms.DPrintf = PR_DPrintf;//Con_DPrintf;//void (*dprintf) (char *, ...);
	menuprogparms.Sys_Error = Sys_Error;
	menuprogparms.Abort = Menu_Abort;
	menuprogparms.CheckHeaderCrc = Menu_CheckHeaderCrc;
	menuprogparms.edictsize = sizeof(menuedict_t);

	menuprogparms.entspawn = NULL;//void (*entspawn) (struct edict_s *ent);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	menuprogparms.entcanfree = NULL;//bool (*entcanfree) (struct edict_s *ent);	//return true to stop ent from being freed
	menuprogparms.stateop = NULL;//StateOp;//void (*stateop) (float var, func_t func);
	menuprogparms.cstateop = NULL;//CStateOp;
	menuprogparms.cwstateop = NULL;//CWStateOp;
	menuprogparms.thinktimeop = NULL;//ThinkTimeOp;
	menuprogparms.MapNamedBuiltin = PR_Menu_MapNamedBuiltin;
	menuprogparms.loadcompleate = NULL;//void (*loadcompleate) (int edictsize);	//notification to reset any pointers.

	menuprogparms.memalloc = PR_CB_Malloc;//void *(*memalloc) (int size);	//small string allocation	malloced and freed randomly
	menuprogparms.memfree = PR_CB_Free;//void (*memfree) (void * mem);


	menuprogparms.globalbuiltins = menu_builtins;//builtin_t *globalbuiltins;	//these are available to all progs
	menuprogparms.numglobalbuiltins = sizeof(menu_builtins) / sizeof(menu_builtins[0]);

	menuprogparms.autocompile = PR_COMPILEIGNORE;//PR_COMPILEEXISTANDCHANGED;//enum {PR_NOCOMPILE, PR_COMPILENEXIST, PR_COMPILECHANGED, PR_COMPILEALWAYS} autocompile;

	menuprogparms.gametime = &menutime;
#ifdef MULTITHREAD
	menuprogparms.usethreadedgc = pr_gc_threaded.ival;
#endif

	menuprogparms.edicts = (struct edict_s **)&menu_edicts;
	menuprogparms.num_edicts = &num_menu_edicts;

	menuprogparms.useeditor = QCEditor;//void (*useeditor) (char *filename, int line, int nump, char **parms);
	menuprogparms.user = &menu_world;
	menu_world.keydestmask = kdm_menu;

	//default to free mouse+hidden cursor, to match dp's default setting, and because its generally the right thing for a menu.
	Q_strncpyz(m->name, "none", sizeof(m->name));
	m->hotspot[0] = 0;
	m->hotspot[1] = 0;
	m->scale = 1;
	m->dirty = true;

	menuqc.cursor = m;
	menuqc.drawmenu = NULL;		//menuqc sucks!
	menuqc.mousemove = MP_MouseMove;
	menuqc.keyevent = MP_KeyEvent;
	menuqc.joyaxis = MP_JoystickAxis;
	menuqc.release = MP_TryRelease;

	menutime = Sys_DoubleTime();
	if (!menu_world.progs)
	{
		vec3_t fwd,rht,up;
		int mprogs;
		Con_DPrintf("Initializing menu.dat\n");
		menu_world.Get_CModel = MP_GetCModel;
		menu_world.Get_FrameState = MP_Get_FrameState;

		menu_world.progs = InitProgs(&menuprogparms);
		PR_Configure(menu_world.progs, PR_ReadBytesString(pr_menu_memsize.string), 16, pr_enable_profiling.ival);
		mprogs = PR_LoadProgs(menu_world.progs, "menu.dat");
		if (mprogs < 0) //no per-progs builtins.
		{
			//failed to load or something
//			CloseProgs(menu_world.progs);
//			menuprogs = NULL;
			return false;
		}
		if (setjmp(mp_abort))
		{
			Con_DPrintf("Failed to initialize menu.dat\n");
			inmenuprogs = false;
			return false;
		}
		inmenuprogs++;

		M_DeInit_Internal();

		PF_InitTempStrings(menu_world.progs);

		menu_world.g.self = (int*)PR_FindGlobal(menu_world.progs, "self", 0, NULL);
		menu_world.g.time = (float*)PR_FindGlobal(menu_world.progs, "time", 0, NULL);
		if (menu_world.g.time)
			*menu_world.g.time = Sys_DoubleTime();
		menu_world.g.frametime = (float*)PR_FindGlobal(menu_world.progs, "frametime", 0, NULL);

		menu_world.g.v_forward	= (float*)PR_FindGlobal(menu_world.progs, "v_forward",	0, NULL); if (!menu_world.g.v_forward)	menu_world.g.v_forward = fwd;
		menu_world.g.v_right	= (float*)PR_FindGlobal(menu_world.progs, "v_right",	0, NULL); if (!menu_world.g.v_right)	menu_world.g.v_right = rht;
		menu_world.g.v_up		= (float*)PR_FindGlobal(menu_world.progs, "v_up",		0, NULL); if (!menu_world.g.v_up)		menu_world.g.v_up = up;

		menu_world.g.drawfont = (float*)PR_FindGlobal(menu_world.progs, "drawfont", 0, NULL);
		menu_world.g.drawfontscale = (float*)PR_FindGlobal(menu_world.progs, "drawfontscale", 0, NULL);

		PR_ProgsAdded(menu_world.progs, mprogs, "menu.dat");

		//ensure that there's space for these fields in.
		//other fields will always be referenced/defined by the qc, or 0.
		PR_RegisterFieldVar(menu_world.progs, ev_string, "model", -1, -1);
		PR_RegisterFieldVar(menu_world.progs, ev_vector, "origin", -1, -1);
		PR_RegisterFieldVar(menu_world.progs, ev_float, "skinobject", -1, -1);

		menuentsize = PR_InitEnts(menu_world.progs, 8192);


		//'world' edict
//		EDICT_NUM_PB(menu_world.progs, 0)->readonly = true;
		EDICT_NUM_PB(menu_world.progs, 0)->ereftype = ER_ENTITY;


		mpfuncs.init = PR_FindFunction(menu_world.progs, "m_init", PR_ANY);
		mpfuncs.shutdown = PR_FindFunction(menu_world.progs, "m_shutdown", PR_ANY);
		{
			int args = 0;
			qbyte *argsizes = NULL;
			mpfuncs.draw = PR_FindFunction(menu_world.progs, "m_draw", PR_ANY);
			menu_world.progs->GetFunctionInfo(menu_world.progs, mpfuncs.draw, &args, &argsizes, NULL, NULL, 0);
			mpfuncs.fuckeddrawsizes = (args == 2 && argsizes[0] == 1 && argsizes[1] == 1);
		}
		mpfuncs.drawloading = PR_FindFunction(menu_world.progs, "m_drawloading", PR_ANY);
		mpfuncs.inputevent = PR_FindFunction(menu_world.progs, "Menu_InputEvent", PR_ANY);
		mpfuncs.keydown = PR_FindFunction(menu_world.progs, "m_keydown", PR_ANY);
		mpfuncs.keyup = PR_FindFunction(menu_world.progs, "m_keyup", PR_ANY);
		mpfuncs.toggle = PR_FindFunction(menu_world.progs, "m_toggle", PR_ANY);
		mpfuncs.consolecommand = PR_FindFunction(menu_world.progs, "m_consolecommand", PR_ANY);
		mpfuncs.gethostcachecategory = PR_FindFunction(menu_world.progs, "m_gethostcachecategory", PR_ANY);
		mpfuncs.rendererrestarted = PR_FindFunction(menu_world.progs, "Menu_RendererRestarted", PR_ANY);
		if (mpfuncs.init)
			PR_ExecuteProgram(menu_world.progs, mpfuncs.init);
		inmenuprogs--;

		EDICT_NUM_PB(menu_world.progs, 0)->readonly = true;

		Con_DPrintf("Initialized menu.dat\n");
		return true;
	}
	return false;
}

static void MP_GameCommand_f(void)
{
	void *pr_globals;
	func_t gamecommand;
	if (!menu_world.progs)
		return;
	gamecommand = PR_FindFunction(menu_world.progs, "GameCommand", PR_ANY);
	if (!gamecommand)
		return;

	if (setjmp(mp_abort))
		return;
	inmenuprogs++;
	pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = PR_TempString(menu_world.progs, Cmd_Args()));
	PR_ExecuteProgram (menu_world.progs, gamecommand);
	inmenuprogs--;
}

qboolean MP_ConsoleCommand(const char *cmdtext)
{
	void *pr_globals;
	if (!menu_world.progs)
		return false;
	if (!mpfuncs.consolecommand)
		return false;

	if (setjmp(mp_abort))
		return true;
	inmenuprogs++;
	pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = PR_TempString(menu_world.progs, cmdtext));
	PR_ExecuteProgram (menu_world.progs, mpfuncs.consolecommand);
	inmenuprogs--;
	return G_FLOAT(OFS_RETURN);
}

void MP_CoreDump_f(void)
{
	if (Cmd_IsInsecure())
	{
		Con_TPrintf("Refusing to execute insecure %s\n", Cmd_Argv(0));
		return;
	}
	if (!menu_world.progs)
	{
		Con_Printf("Can't core dump, you need to be running the MenuQC progs first.");
		return;
	}

	{
		size_t size = 1024*1024*8;
		char *buffer = BZ_Malloc(size);
		menu_world.progs->save_ents(menu_world.progs, buffer, &size, size, 3);
		COM_WriteFile("menucore.txt", FS_GAMEONLY, buffer, size);
		BZ_Free(buffer);
	}
}

static void MP_Poke_f(void)
{
	if (Cmd_IsInsecure())
		Con_TPrintf("Refusing to execute insecure %s\n", Cmd_Argv(0));
	/*else if (!SV_MayCheat())
		Con_TPrintf ("Please set sv_cheats 1 and restart the map first.\n");*/
	else if (menu_world.progs && menu_world.progs->EvaluateDebugString)
		Con_TPrintf("Result: %s\n", menu_world.progs->EvaluateDebugString(menu_world.progs, Cmd_Args()));
	else
		Con_TPrintf ("not supported.\n");
}

static void MP_Breakpoint_f(void)
{
	int wasset;
	int isset;
	char *filename = Cmd_Argv(1);
	int line = atoi(Cmd_Argv(2));

	if (Cmd_IsInsecure())
	{
		Con_TPrintf("Refusing to execute insecure %s\n", Cmd_Argv(0));
		return;
	}
	if (!menu_world.progs)
	{
		Con_Printf("Menu not running\n");
		return;
	}
	wasset = menu_world.progs->ToggleBreak(menu_world.progs, filename, line, 3);
	isset = menu_world.progs->ToggleBreak(menu_world.progs, filename, line, 2);

	if (wasset == isset)
		Con_Printf("Breakpoint was not valid\n");
	else if (isset)
		Con_Printf("Breakpoint has been set\n");
	else
		Con_Printf("Breakpoint has been cleared\n");

	Cvar_Set(Cvar_FindVar("pr_debugger"), "1");
}
static void MP_Watchpoint_f(void)
{
	char *variable = Cmd_Argv(1);
	if (!*variable)
		variable = NULL;

	if (Cmd_IsInsecure())
	{
		Con_TPrintf("Refusing to execute insecure %s\n", Cmd_Argv(0));
		return;
	}
	if (!menu_world.progs)
	{
		Con_Printf("menuqc not running\n");
		return;
	}
	if (menu_world.progs->SetWatchPoint(menu_world.progs, variable, variable))
		Con_Printf("Watchpoint set\n");
	else
		Con_Printf("Watchpoint cleared\n");
}
static void MP_Profile_f(void)
{
	if (Cmd_IsInsecure())
	{
		Con_TPrintf("Refusing to execute insecure %s\n", Cmd_Argv(0));
		return;
	}
	if (menu_world.progs && menu_world.progs->DumpProfile)
		if (!menu_world.progs->DumpProfile(menu_world.progs, !atof(Cmd_Argv(1))))
			Con_Printf("Enabled menuqc Profiling.\n");
}

void MP_RegisterCvarsAndCmds(void)
{
	Cmd_AddCommand("coredump_menuqc", MP_CoreDump_f);
	Cmd_AddCommand("menu_cmd", MP_GameCommand_f);
	Cmd_AddCommand("breakpoint_menuqc", MP_Breakpoint_f);
	Cmd_AddCommand("watchpoint_menuqc", MP_Watchpoint_f);
#ifdef HAVE_LEGACY
	Cmd_AddCommand("loadfont", CL_LoadFont_f);
#endif

	Cmd_AddCommand("poke_menuqc", MP_Poke_f);
	Cmd_AddCommand("profile_menuqc", MP_Profile_f);


	Cvar_Register(&forceqmenu, MENUPROGSGROUP);
	Cvar_Register(&pr_menu_coreonerror, MENUPROGSGROUP);
	Cvar_Register(&pr_menu_memsize, MENUPROGSGROUP);
}

qboolean MP_UsingGamecodeLoadingScreen(void)
{
	return menu_world.progs && mpfuncs.drawloading;
}

int MP_GetServerCategory(int index)
{
	int category = 0;
	if (menu_world.progs && mpfuncs.gethostcachecategory)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		if (!setjmp(mp_abort))
		{
			inmenuprogs++;
			G_FLOAT(OFS_PARM0) = index;
			PR_ExecuteProgram(menu_world.progs, mpfuncs.gethostcachecategory);
			category = G_FLOAT(OFS_RETURN);
			inmenuprogs--;
		}
	}
	return category;
}

void MP_RendererRestarted(void)
{
	int i;
	if (!menu_world.progs)
		return;

	menu_world.worldmodel = cl.worldmodel;

	for (i = 0; i < MAX_CSMODELS; i++)
	{
		cl.model_csqcprecache[i] = NULL;
	}

	//FIXME: registered shaders

	//let the csqc know that its rendertargets got purged
	if (mpfuncs.rendererrestarted)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		(((string_t *)pr_globals)[OFS_PARM0] = PR_TempString(menu_world.progs, rf->description));
		PR_ExecuteProgram(menu_world.progs, mpfuncs.rendererrestarted);
	}
	//in case it drew to any render targets.
	if (R2D_Flush)
		R2D_Flush();
	if (*r_refdef.rt_destcolour[0].texname)
	{
		Q_strncpyz(r_refdef.rt_destcolour[0].texname, "", sizeof(r_refdef.rt_destcolour[0].texname));
		BE_RenderToTextureUpdate2d(true);
	}
}

void MP_Draw(void)
{
	extern qboolean scr_drawloading;
	globalvars_t *pr_globals;
	if (!menu_world.progs)
		return;
	if (setjmp(mp_abort))
		return;

	menutime = Sys_DoubleTime();
	if (menu_world.g.time)
		*menu_world.g.time = menutime;
	if (menu_world.g.frametime)
		*menu_world.g.frametime = host_frametime;

	inmenuprogs++;
	PR_RunThreads(&menu_world);

	pr_globals = PR_globals(menu_world.progs, PR_CURRENT);

	if (scr_drawloading||scr_disabled_for_loading)
	{	//don't draw the menu if we're meant to be drawing a loading screen
		//the menu should provide a special function if it wants to draw custom loading screens. this is for compat with old/dp/lazy/crappy menus.
		if (mpfuncs.drawloading)
		{
			((float *)pr_globals)[OFS_PARM0+0] = vid.width;
			((float *)pr_globals)[OFS_PARM0+1] = vid.height;
			((float *)pr_globals)[OFS_PARM0+2] = 0;

			((float *)pr_globals)[OFS_PARM1] = scr_disabled_for_loading;
			PR_ExecuteProgram(menu_world.progs, mpfuncs.drawloading);
		}
	}
	else if (mpfuncs.draw)
	{
		if (mpfuncs.fuckeddrawsizes)
		{	//pass useless sizes in two args if its a dp menu
			((float *)pr_globals)[OFS_PARM0] = vid.pixelwidth;
			((float *)pr_globals)[OFS_PARM0+1] = 0;	//make sure its set, just in case...
			((float *)pr_globals)[OFS_PARM0+2] = 0;
			((float *)pr_globals)[OFS_PARM1] = vid.pixelheight;
			((float *)pr_globals)[OFS_PARM1+1] = 0;
			((float *)pr_globals)[OFS_PARM1+2] = 0;
		}
		else
		{	//pass useful sizes in a 1-arg vector if its an fte menu.
			((float *)pr_globals)[OFS_PARM0+0] = vid.width;
			((float *)pr_globals)[OFS_PARM0+1] = vid.height;
			((float *)pr_globals)[OFS_PARM0+2] = 0;

			//make physical pixel counts available too, because we can.
			((float *)pr_globals)[OFS_PARM1+0] = vid.pixelwidth;
			((float *)pr_globals)[OFS_PARM1+1] = vid.pixelheight;
			((float *)pr_globals)[OFS_PARM1+2] = 0;
		}

		PR_ExecuteProgram(menu_world.progs, mpfuncs.draw);
	}
	inmenuprogs--;
}

qboolean MP_Toggle(int mode)
{
	if (!menu_world.progs)
		return false;
#ifdef TEXTEDITOR
	if (editormodal)
		return false;
#endif

	if (!mode && !Key_Dest_Has(kdm_menu))
		return false;

	if (setjmp(mp_abort))
		return false;

	menutime = Sys_DoubleTime();
	if (menu_world.g.time)
		*menu_world.g.time = menutime;

	inmenuprogs++;
	if (mpfuncs.toggle)
	{
		void *pr_globals = PR_globals(menu_world.progs, PR_CURRENT);
		G_FLOAT(OFS_PARM0) = mode;
		PR_ExecuteProgram(menu_world.progs, mpfuncs.toggle);
	}
	if (R2D_Flush)
		R2D_Flush();
	inmenuprogs--;

	return true;
}
#endif
