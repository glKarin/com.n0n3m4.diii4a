//included directly from plugin.c
//this is the client-only things.

static plugin_t *protocolclientplugin;


static void PlugMenu_Close(menu_t *m, qboolean forced)
{
	Z_Free(m);
}
static qboolean PlugMenu_Event(menu_t *m, int eventtype, int keyparam, int unicodeparam)	//eventtype = draw/keydown/keyup, param = time/key
{
	plugin_t *oc=currentplug;
	qboolean ret;

	currentplug = m->ctx;
	ret = currentplug->menufunction(eventtype, keyparam, unicodeparam, mousecursor_x, mousecursor_y, vid.width, vid.height);
	currentplug=oc;
	return ret;
}
static qboolean PlugMenu_KeyEvent(menu_t *m, qboolean isdown, unsigned int devid, int key, int unicode)
{
	return PlugMenu_Event(m, isdown?1:2, key, unicode);
}
static void PlugMenu_Draw(menu_t *m)
{
	PlugMenu_Event (m, 0, (realtime*1000), 0);
}
static qboolean QDECL Plug_SetMenuFocus (qboolean wantkeyfocus, const char *cursorname, float hot_x, float hot_y, float scale) //null cursorname=relmouse, set/empty cursorname=absmouse
{
	menu_t *m;
	if (qrenderer == QR_NONE)
		return false;

	m = Menu_FindContext(currentplug);

	if (wantkeyfocus)
	{
		if (!m)
		{
			m = Z_Malloc(sizeof(*m));
			m->ctx = currentplug;
			m->cursor = &key_customcursor[kc_plugin];
			m->release = PlugMenu_Close;
			m->keyevent = PlugMenu_KeyEvent;
			m->drawmenu = PlugMenu_Draw;
			Menu_Push(m, false);
		}
	}
	else if (m)
		Menu_Unlink(m, false);

	if (wantkeyfocus)
	{
		struct key_cursor_s *mc = &key_customcursor[kc_plugin];

		if (cursorname)
		{
			if (scale <= 0)
				scale = 1;
			if (strcmp(cursorname, mc->name) || mc->hotspot[0] != hot_x || mc->hotspot[1] != hot_y || mc->scale != scale)
			{
				Q_strncpyz(mc->name, cursorname, sizeof(mc->name));
				mc->hotspot[0] = hot_x;
				mc->hotspot[1] = hot_y;
				mc->scale = scale;
				mc->dirty = true;
			}
		}
	}
	return true;
}
static qboolean QDECL Plug_HasMenuFocus(void)
{
	return topmenu&&topmenu->ctx==currentplug && Key_Dest_Has(kdm_menu);
}

static int QDECL Plug_Key_GetKeyCode(const char *keyname, int *modifier)
{
	int modifier_;
	if (!modifier)
		modifier = &modifier_;
	return Key_StringToKeynum(keyname, modifier);
}
static const char *QDECL Plug_Key_GetKeyName(int keycode, int modifier)
{
	return Key_KeynumToString(keycode, modifier);
}

const char *QDECL Plug_Key_GetKeyBind(int bindmap, int keynum, int modifier)
{
	return Key_GetBinding(keynum, bindmap, modifier);
}
void QDECL Plug_Key_SetKeyBind(int bindmap, int keycode, int modifier, const char *newbinding)
{
	if (bindmap && !modifier)
		modifier = (bindmap-1) | KEY_MODIFIER_ALTBINDMAP;
	Key_SetBinding (keycode, modifier, newbinding, RESTRICT_LOCAL);
}

static unsigned int IN_GetKeyDest(void)
{
	return key_dest_mask;
}

qboolean QDECL Plug_Input_IsKeyDown(int key)
{
	extern unsigned int keydown[K_MAX];
	if (key >= 0 && key < K_MAX)
		return !!keydown[key];
	return false;
}
void QDECL Plug_Input_ClearKeyStates(void)
{
	Key_ClearStates();
}
void QDECL Plug_Input_SetSensitivityScale(float scale)
{
	in_sensitivityscale = scale;
}
unsigned int QDECL Plug_Input_GetMoveCount(void)
{
	return cl.movesequence;
}
usercmd_t *QDECL Plug_Input_GetMoveEntry(unsigned int move)
{
	if (move == cl.movesequence)
		return NULL;	//the partial
	else if (move >= cl.movesequence)
		return NULL;	//too new
	else if (cl.outframes[move&UPDATE_MASK].cmd_sequence != move)
		return NULL;	//too old or otherwise missing
	else
		return &cl.outframes[move&UPDATE_MASK].cmd[0];
}

/*

static void QDECL Plug_SCR_CenterPrint(int seat, const char *text)
{
	if (qrenderer != QR_NONE)
		SCR_CenterPrint(seat, text, true);
}
*/



#include "shader.h"

static qboolean Plug_Draw_GetScreenSize(float *vsize, unsigned int *psize)
{
	if (qrenderer<=0)
		return false;
	if (vsize)
		vsize[0] = vid.width, vsize[1] = vid.height;
	if (psize)
		psize[0] = vid.pixelwidth, psize[1] = vid.pixelheight;
	return true;
}

static qhandle_t Plug_Draw_LoadImage(const char *name, int type, const char *script)
{
	shader_t *pic;
	if (qrenderer != QR_NONE)
	{
		if (type == 3)
			pic = NULL;
		else if (type == 2)
			pic = R_RegisterShader(name, SUF_NONE, script);
		else
			pic = R2D_SafeCachePic(name);
	}
	else
		pic = NULL;

	if (pic)
		return pic->id+1;
	return 0;
}

static qhandle_t QDECL Plug_Draw_LoadImageData(const char *name, const char *mimetype, void *codeddata, size_t datalength)
{
	qhandle_t ret = 0;
	image_t *t;
	qbyte *rgbdata;
	unsigned int width, height;
	uploadfmt_t format;
		
	if ((rgbdata = ReadRawImageFile(codeddata, datalength, &width, &height, &format, false, name)))
	{
		t = Image_FindTexture(name, NULL, IF_PREMULTIPLYALPHA|IF_NOMIPMAP|IF_UIPIC|IF_CLAMP);
		if (!TEXVALID(t))
			t = Image_CreateTexture(name, NULL, IF_PREMULTIPLYALPHA|IF_NOMIPMAP|IF_UIPIC|IF_CLAMP);
		if (TEXVALID(t))
		{
			Image_Upload(t, format, rgbdata, NULL, width, height, 1, IF_PREMULTIPLYALPHA|IF_NOMIPMAP|IF_UIPIC|IF_CLAMP);
			ret = Plug_Draw_LoadImage(name, 3, NULL);
		}
		
		BZ_Free(rgbdata);
	}
	return ret;
}
static qhandle_t QDECL Plug_Draw_LoadImageShader(const char *name, const char *script)
{
	return Plug_Draw_LoadImage(name, 2, script);
}
static qhandle_t QDECL Plug_Draw_LoadImagePic(const char *name)
{
	return Plug_Draw_LoadImage(name, 0, NULL);
}
static shader_t *Plug_Draw_ShaderFromId(qhandle_t id)
{
	if (--id >= r_numshaders)
		return NULL;
	return r_shaders[id];
}
static void Plug_Draw_UnloadImage(qhandle_t id)
{
	R_UnloadShader(Plug_Draw_ShaderFromId(id));
}

static int QDECL Plug_Draw_ImageSize(qhandle_t image, float *w, float *h)
{
	int iw, ih, ret;

	if (image > 0 && image <= r_numshaders)
	{
		ret = R_GetShaderSizes(r_shaders[image-1], &iw, &ih, true);
		if (w)
			*w = iw;
		if (h)
			*h = ih;
		return ret;
	}
	return -1;
}

static int QDECL Plug_Draw_Image(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t image)
{
	if (image > 0 && image <= r_numshaders)
	{
		R2D_Image(x, y, w, h, s1, t1, s2, t2, r_shaders[image-1]);
		return 1;
	}
	return 0;
}
static int QDECL Plug_Draw_Image2dQuad(const vec2_t *points, const vec2_t *texcoords, const vec4_t *colours, qhandle_t image)
{
	if (image > 0 && image <= r_numshaders)
	{
		R2D_Image2dQuad(points, texcoords, colours, r_shaders[image-1]);
		return 1;
	}
	return 0;
}
//x1,y1,x2,y2
static void QDECL Plug_Draw_Line(float x1, float y1, float x2, float y2)
{
	R2D_Line(x1,y1, x2,y2, NULL);
}
static void QDECL Plug_Draw_Character(float x, float y, unsigned int character)
{
	float px, py;
	if (qrenderer == QR_NONE)
		return;
	Font_BeginScaledString(font_default, x, y, 8, 8, &px, &py);
	Font_DrawScaleChar(px, py, CON_WHITEMASK, character);
	Font_EndString(font_default);
}
static void QDECL Plug_Draw_CharacterH(float x, float y, float h, unsigned int flags, unsigned int charc)
{
	conchar_t cmask = CON_WHITEMASK;
	if (qrenderer == QR_NONE)
		return;
	if (flags & 1)
		cmask |= CON_2NDCHARSETTEXT;
	if (!(flags & 2))
		cmask |= 0xe000;
	Font_BeginScaledString(font_default, x, y, h, h, &x, &y);
	Font_DrawScaleChar(x, y, cmask, charc);
	Font_EndString(font_default);
}
static void QDECL Plug_Draw_String(float x, float y, const char *string)
{
	int ipx, px, py;
	conchar_t buffer[2048], *str;
	unsigned int codeflags, codepoint;
	if (qrenderer == QR_NONE)
		return;
	COM_ParseFunString(CON_WHITEMASK, string, buffer, sizeof(buffer), false);
	str = buffer;
	Font_BeginString(font_default, x, y, &px, &py);
	ipx = px;
	while(*str)
	{
		str = Font_Decode(str, &codeflags, &codepoint);
		if (codepoint == '\n')
			py += Font_CharHeight();
		else if (codepoint == '\r')
			px = ipx;
		else
			px = Font_DrawChar(px, py, codeflags, codepoint);
	}
	Font_EndString(font_default);
}
static void QDECL Plug_Draw_StringH(float x, float y, float h, unsigned int flags, const char *instr)
{
	float ipx;
	conchar_t buffer[2048], *str, cmask = CON_WHITEMASK;
	unsigned int codeflags, codepoint;
	unsigned int parseflags = 0;
	if (qrenderer == QR_NONE)
		return;
	if (flags & 1)
		cmask |= CON_2NDCHARSETTEXT;
	if (flags & 2)
		parseflags |= PFS_FORCEUTF8;
	COM_ParseFunString(CON_WHITEMASK, instr, buffer, sizeof(buffer), parseflags);
	str = buffer;
	Font_BeginScaledString(font_default, x, y, h, h, &x, &y);
	ipx = x;
	while(*str)
	{
		str = Font_Decode(str, &codeflags, &codepoint);
		if (codepoint == '\n')
			y += Font_CharScaleHeight();
		else if (codepoint == '\r')
			x = ipx;
		else
			x = Font_DrawScaleChar(x, y, codeflags, codepoint);
	}
	Font_EndString(font_default);
}

static float QDECL Plug_Draw_StringWidth(float h, unsigned int flags, const char *instr)
{
	conchar_t buffer[2048], *str, cmask = CON_WHITEMASK;
	unsigned int parseflags = 0;
	float px,py;
	if (qrenderer == QR_NONE)
		return 0;
	if (flags & 1)
		cmask |= CON_2NDCHARSETTEXT;
	if (flags & 2)
		parseflags |= PFS_FORCEUTF8;
	str = COM_ParseFunString(CON_WHITEMASK, instr, buffer, sizeof(buffer), parseflags);
	
	Font_BeginScaledString(font_default, 0, 0, h, h, &px, &py);
	px = Font_LineScaleWidth(buffer, str);
	Font_EndString(NULL);
	
	//put it back in virtual space
	return (px*(float)vid.width) / (float)vid.rotpixelwidth;
}

static void QDECL Plug_Draw_Fill(float x, float y, float width, float height)
{
	if (qrenderer != QR_NONE)
		R2D_FillBlock(x, y, width, height);
}
static void QDECL Plug_Draw_ColourP(int palcol, float a)
{
	if (palcol>=0 && palcol<=255)
		R2D_ImagePaletteColour(palcol, a);
}
static void QDECL Plug_Draw_Colour4f(float r, float g, float b, float a)
{
	R2D_ImageColours(r,g,b,a);
}

static void QDECL Plug_Draw_RedrawScreen(void)
{
	SCR_UpdateScreen();
}

#ifdef HAVE_MEDIA_DECODER
static void QDECL Plug_Media_SetState(cin_t *cin, int state)
{
	Media_SetState(cin, state);
}
static int QDECL Plug_Media_GetState(cin_t *cin)
{
	return Media_GetState(cin);
}
#endif

static qhandle_t Plug_Scene_ModelToId(model_t *mod)
{
	if (!mod)
		return 0;
	return (mod-mod_known)+1;
}
static model_t *Plug_Scene_ModelFromId(qhandle_t id)
{
	extern int mod_numknown;
	if ((unsigned)(--id) >= mod_numknown)
		return NULL;
	return mod_known+id;
}
static qhandle_t Plug_Scene_ShaderForSkin(qhandle_t modelid, int surfaceidx, int skinnum, float time)
{
	shader_t *s = Mod_ShaderForSkin(Plug_Scene_ModelFromId(modelid), surfaceidx, skinnum, time, NULL);
	return s->id+1;
}
static void QDECL Plug_Scene_Clear(void)
{
	CL_ClearEntityLists();
	rtlights_first = RTL_FIRST;
}
static unsigned int Plug_Scene_AddPolydata(struct shader_s *s, unsigned int beflags, size_t numverts, size_t numidx, vecV_t **vertcoord, vec2_t **texcoord, vec4_t **colour, index_t **indexes)
{
	unsigned int ret;
	scenetris_t *t;

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == s && cl_stris[cl_numstris-1].flags == beflags)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = s;
		t->flags = beflags;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
	}
	ret = cl_numstrisvert - t->firstvert;

	if (cl_maxstrisvert < cl_numstrisvert+numverts)
		cl_stris_ExpandVerts(cl_numstrisvert+numverts + 64);
	if (cl_maxstrisidx < cl_numstrisidx+numidx)
	{
		cl_maxstrisidx = cl_numstrisidx+numidx + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}

	*vertcoord = cl_strisvertv+cl_numstrisvert;
	*texcoord = cl_strisvertt+cl_numstrisvert;
	*colour = cl_strisvertc+cl_numstrisvert;
	*indexes = cl_strisidx+cl_numstrisidx;

	t->numvert += numverts;
	t->numidx += numidx;
	cl_numstrisvert += numverts;
	cl_numstrisidx += numidx;

	return ret;
}

void R_DrawNameTags(void);
static void Plug_Scene_RenderScene(plugrefdef_t *in, size_t areabytes, const qbyte *areadata)
{
	size_t i;
	extern cvar_t r_torch;
	if (R2D_Flush)
		R2D_Flush();

	VectorCopy(in->viewaxisorg[0], r_refdef.viewaxis[0]);
	VectorCopy(in->viewaxisorg[1], r_refdef.viewaxis[1]);
	VectorCopy(in->viewaxisorg[2], r_refdef.viewaxis[2]);
	VectorCopy(in->viewaxisorg[3], r_refdef.vieworg);

	VectorSet(r_refdef.weaponmatrix[0], 1,0,0);
	VectorSet(r_refdef.weaponmatrix[1], 0,1,0);
	VectorSet(r_refdef.weaponmatrix[2], 0,0,1);
	VectorSet(r_refdef.weaponmatrix[3], 0,0,0);

	VectorSet(r_refdef.weaponmatrix_bob[0], 1,0,0);
	VectorSet(r_refdef.weaponmatrix_bob[1], 0,1,0);
	VectorSet(r_refdef.weaponmatrix_bob[2], 0,0,1);
	VectorSet(r_refdef.weaponmatrix_bob[3], 0,0,0);

	VectorAngles(r_refdef.viewaxis[0], r_refdef.viewaxis[2], r_refdef.viewangles, false);	//do we actually still need this?
	r_refdef.flags = in->flags;
	r_refdef.fov_x = in->fov[0];
	r_refdef.fov_y = in->fov[1];
	r_refdef.fovv_x = in->fov_viewmodel[0];
	r_refdef.fovv_y = in->fov_viewmodel[1];
	r_refdef.vrect.x = in->rect.x;
	r_refdef.vrect.y = in->rect.y;
	r_refdef.vrect.width = in->rect.w;
	r_refdef.vrect.height = in->rect.h;
	r_refdef.time = in->time;
	r_refdef.useperspective = true;
	r_refdef.mindist = bound(0.1, gl_mindist.value, 4);
	r_refdef.maxdist = gl_maxdist.value;
	r_refdef.playerview = &cl.playerview[0];

	if (in->flags & RDF_SKYROOMENABLED)
	{
		r_refdef.skyroom_enabled = true;
		VectorCopy(in->skyroom_org, r_refdef.skyroom_pos);
	}
	else
		r_refdef.skyroom_enabled = false;

	if (r_refdef.vrect.y < 0)
	{	//evil hack to work around player model ui bug.
		//if the y coord is off screen, reduce the height to keep things centred, and reduce the fov to compensate.
		r_refdef.vrect.height += r_refdef.vrect.y*2;
		r_refdef.fov_y = in->fov[1] * r_refdef.vrect.height / in->rect.h;
		r_refdef.fovv_y = in->fov_viewmodel[1] * r_refdef.vrect.height / in->rect.h;
		r_refdef.vrect.y = 0;
	}

	memset(&r_refdef.globalfog, 0, sizeof(r_refdef.globalfog));

	if (r_torch.ival)
	{
		dlight_t *dl;
		dl = CL_NewDlight(0, r_refdef.vieworg, 300, r_torch.ival, 0.5, 0.5, 0.2);
		dl->flags |= LFLAG_SHADOWMAP|LFLAG_FLASHBLEND;
		dl->fov = 60;
		VectorCopy(r_refdef.viewaxis[0], dl->axis[0]);
		VectorCopy(r_refdef.viewaxis[1], dl->axis[1]);
		VectorCopy(r_refdef.viewaxis[2], dl->axis[2]);
	}

	r_refdef.areabitsknown = areabytes>0;
	for (i = 0; i < sizeof(r_refdef.areabits)/sizeof(int) && i < areabytes/sizeof(int); i++)
		((int*)r_refdef.areabits)[i] = ((int*)areadata)[i] ^ ~0;
	R_PushDlights();
	R_RenderView();
	R_DrawNameTags();
	r_refdef.playerview = NULL;
	r_refdef.time = 0;
}

static void QDECL Plug_LocalSound(const char *soundname, int channel, float volume)
{
	if (qrenderer != QR_NONE)
		S_LocalSound2(soundname, channel, volume);
}


static void QDECL Plug_CL_SetLoadscreenState(qboolean state)
{
	if (state)
		SCR_BeginLoadingPlaque();
	else
		SCR_EndLoadingPlaque();
}

static int QDECL Plug_CL_GetStats(int pnum, unsigned int *stats, int maxstats)
{
	int i = 0;
	int max;

	if (qrenderer == QR_NONE || !cls.state)
		return 0;

	max = maxstats;
	if (max > MAX_CL_STATS)
		max = MAX_CL_STATS;
	if (pnum < 0)
	{
		pnum = -pnum-1;
		if (pnum < MAX_CLIENTS)
		{
			for (i = 0; i < max; i++)
				stats[i] = cl.players[pnum].stats[i];
		}
	}
	else if (pnum < cl.splitclients)
	{
		for (i = 0; i < max; i++)
		{	//fill stats with the right player's stats
			stats[i] = cl.playerview[pnum].stats[i];
		}
	}

	max = i;
	for (; i < maxstats; i++)	//plugin has too many stats (wow)
		stats[i] = 0;					//fill the rest.
	return max;
}

static void QDECL Plug_GetPlayerInfo(int playernum, plugclientinfo_t *out)
{
	int i;

	//queries for the local seats
	if (playernum < 0)
		playernum = cl.playerview[-playernum-1].playernum;

	if (playernum < 0 || playernum >= MAX_CLIENTS)
	{
		memset(out, 0, sizeof(*out));
		return;
	}

	i = playernum;
	if (out)
	{
		out->bottomcolour = cl.players[i].rbottomcolor;
		out->topcolour = cl.players[i].rtopcolor;
		out->frags = cl.players[i].frags;
		Q_strncpyz(out->name, cl.players[i].name, PLUGMAX_SCOREBOARDNAME);
		out->ping = cl.players[i].ping;
		out->pl = cl.players[i].pl;
		out->activetime = realtime - cl.players[i].realentertime;
		out->userid = cl.players[i].userid;
		out->spectator = cl.players[i].spectator;
		InfoBuf_ToString(&cl.players[i].userinfo, out->userinfo, sizeof(out->userinfo), basicuserinfos, NULL, NULL, NULL, NULL);
		Q_strncpyz(out->team, cl.players[i].team, sizeof(out->team));
	}
}

static size_t QDECL Plug_GetLocalPlayerNumbers(size_t first, size_t count, int *playernums, int *spectracks)
{
	size_t i;
	if (count < 0 || count > 1000) count = 0;
	if (first > cl.splitclients) first = cl.splitclients;
	if (first+count > cl.splitclients) count = cl.splitclients-first;
	for (i = 0; i < count; i++)
	{
		playernums[i] = cl.playerview[first+i].playernum;
		spectracks[i] = Cam_TrackNum(&cl.playerview[first+i]);
	}
	return count;
}

static void QDECL Plug_GetServerInfoRaw(char *outptr, size_t outlen)
{
	extern float demtime;

	InfoBuf_ToString(&cl.serverinfo, outptr, outlen, NULL, NULL, NULL, NULL, NULL);
	Q_strncatz(outptr, va("\\intermission\\%i", cl.intermissionmode), outlen);
	switch(cls.demoplayback)
	{
	case DPB_NONE:
		break;
	case DPB_MVD:
		Q_strncatz(outptr, "\\demotype\\mvd", outlen);
		break;
	case DPB_QUAKEWORLD:
		Q_strncatz(outptr, "\\demotype\\qw", outlen);
		break;
#ifdef NQPROT
	case DPB_NETQUAKE:
		Q_strncatz(outptr, "\\demotype\\nq", outlen);
		break;
#endif
#ifdef Q2CLIENT
	case DPB_QUAKE2:
		Q_strncatz(outptr, "\\demotype\\q2", outlen);
		break;
#endif
	}
	Q_strncatz(outptr, va("\\demotime\\%f", demtime-cls.demostarttime), outlen);

#ifdef QUAKEHUD
	if (cl.playerview[0].statsf[STAT_MATCHSTARTTIME])
		Q_strncatz(outptr, va("\\matchstart\\%f", cl.playerview[0].statsf[STAT_MATCHSTARTTIME]/1000), outlen);
	else
#endif
		Q_strncatz(outptr, va("\\matchstart\\%f", cl.matchgametimestart), outlen);
}
static size_t QDECL Plug_GetServerInfoBlob(const char *key, void *outptr, size_t outsize)
{
	char tmp[32];
	size_t blobsize;
	const char *blob = InfoBuf_BlobForKey(&cl.serverinfo, key, &blobsize, NULL);
	if (!blob)
	{	//inescapable hacks
		if (!strcmp(key, "matchstart"))
		{
			float matchstart = cl.matchgametimestart;
#ifdef QUAKEHUD
			if (cl.playerview[0].statsf[STAT_MATCHSTARTTIME])
				matchstart = cl.playerview[0].statsf[STAT_MATCHSTARTTIME]/1000;
#endif
			snprintf(tmp, sizeof(tmp), "%f", matchstart), blob=tmp;
		}
		else if (!strcmp(key, "demotime"))
		{
			extern float demtime;
			snprintf(tmp, sizeof(tmp), "%f", demtime-cls.demostarttime), blob=tmp;
		}
		else if (!strcmp(key, "demotype"))
		{
			switch(cls.demoplayback)
			{
			case DPB_NONE:
				break;
			case DPB_MVD:
				blob = "mvd";
				break;
			case DPB_QUAKEWORLD:
				blob = "qw";
				break;
#ifdef NQPROT
			case DPB_NETQUAKE:
				blob = "nq";
				break;
#endif
#ifdef Q2CLIENT
			case DPB_QUAKE2:
				blob = "q2";
				break;
#endif
			}
		}
		else if (!strcmp(key, "intermission"))
			snprintf(tmp, sizeof(tmp), "%i", cl.intermissionmode), blob=tmp;
		if (blob)
			blobsize = strlen(blob);
	}
	if (outptr)
	{
		if (blobsize > outsize)
			return 0;	//error
		memcpy(outptr, blob, blobsize);
		return blobsize;
	}
	else
		return blobsize;
}

static void QDECL Plug_SetUserInfo(int seat, const char *key, const char *value)
{
	CL_SetInfo(seat, key, value);
}
static void QDECL Plug_SetUserInfoBlob(int seat, const char *key, const void *value, size_t size)
{
	CL_SetInfoBlob(seat, key, value, size);
}

static size_t QDECL Plug_GetUserInfoBlob(int seat, const char *key, void *outptr, size_t outsize)
{
	size_t blobsize;
	const char *blob;
	if (seat >= countof(cls.userinfo))
		blob = NULL, blobsize = 0;
	else
		blob = InfoBuf_BlobForKey(&cls.userinfo[seat], key, &blobsize, NULL);
	if (outptr)
	{
		if (blobsize > outsize)
			return 0;	//error
		memcpy(outptr, blob, blobsize);
		return blobsize;
	}
	else
		return blobsize;
}

void QDECL Plug_CL_ClearState(void)
{
	CL_ClearState(true);
}
void QDECL Plug_CL_UpdateGameTime(double servertime)
{
	cl.oldgametime = cl.gametime;
	cl.oldgametimemark = cl.gametimemark;
	cl.gametime = servertime;
	cl.gametimemark = realtime;
}

static qboolean QDECL Plug_GetLastInputFrame(int seat, usercmd_t *outcmd)
{
	unsigned int curframe = (cl.movesequence-1u) & UPDATE_MASK;
	if (!cl.movesequence || seat < 0 || seat >= cl.splitclients)
		return false;
	*outcmd = cl.outframes[curframe].cmd[seat];
	return true;
}

#define has(x) (((quintptr_t)&((plugnetinfo_t*)NULL)->x + sizeof(((plugnetinfo_t*)NULL)->x)) <= outlen)
//aka: misc other hud timing crap
static size_t QDECL Plug_GetNetworkInfo(plugnetinfo_t *outptr, size_t outlen)
{
	if (has(capturing))
	{
#ifdef HAVE_MEDIA_ENCODER
		outptr->capturing = Media_Capturing();
#else
		outptr->capturing = 0;
#endif
	}
	
	if (has(seats))
		outptr->seats = cl.splitclients;
	if (has(ping))		
		CL_CalcNet2 (&outptr->ping.s_avg, &outptr->ping.s_mn, &outptr->ping.s_mx, &outptr->ping.ms_stddev, &outptr->ping.fr_avg, &outptr->ping.fr_mn, &outptr->ping.fr_mx, &outptr->loss.dropped, &outptr->loss.choked, &outptr->loss.invalid);
		
	if (has(mlatency))
		outptr->mlatency = 0;
	if (has(mrate))
		outptr->mrate = IN_DetermineMouseRate();
	if (has(vlatency))
		outptr->vlatency = 0;
		
	if (has(speed))
		VectorCopy(outptr->speed, r_refdef.playerview->simvel);

	if (has(clrate))
		NET_GetRates(cls.sockets, &outptr->clrate.in_pps, &outptr->clrate.out_pps, &outptr->clrate.in_bps, &outptr->clrate.out_bps);		
	if (has(svrate))
	{
		memset(&outptr->svrate, 0, sizeof(outptr->svrate));
#ifndef CLIENTONLY
		NET_GetRates(svs.sockets, &outptr->svrate.in_pps, &outptr->svrate.out_pps, &outptr->svrate.in_bps, &outptr->svrate.out_bps);
#endif
	}
	
	return min(outlen,sizeof(*outptr));
}
#undef has

#ifdef QUAKEHUD
static float QDECL Plug_GetTrackerOwnFrags(int seat, char *outptr, size_t outlen)
{
	if (!outlen)
		return 0;
	else
		return Stats_GetLastOwnFrag(seat, outptr, outlen);
}
static void QDECL Plug_GetPredInfo(int seat, vec3_t outvel)
{
	if ((unsigned)seat < MAX_SPLITS)
		VectorCopy(cl.playerview[seat].simvel, outvel);
}
#endif

static void QDECL Plug_GetLocationName(const float *locpoint, char *outbuffer, size_t bufferlen)
{
	const char *result = TP_LocationName(locpoint);
	Q_strncpyz(outbuffer, result, bufferlen);
}

#ifdef QUAKEHUD
static size_t QDECL Plug_GetTeamInfo(teamplayerinfo_t *players, size_t maxplayers, qboolean showenemies, int seat)
{
	int count = 0;
	int i;
	int self;
	lerpents_t		*le;
	player_info_t	*pl;

	maxplayers = min(maxplayers, cl.allocated_client_slots);
	
	Cvar_Get("ti", "1", CVAR_USERINFO, "Hacks because ktx sucks. Must be 1 in order to receive team information in ktx.");
	
	if (seat >= 0)
	{
		self = cl.playerview[seat].playernum;
		if (cl.playerview[seat].cam_state != CAM_FREECAM)
			self = cl.playerview[seat].cam_spec_track;
	}
	else
		self = -1;
	
	for (i = 0; i < cl.allocated_client_slots && maxplayers > 0; i++)
	{
		if (!*cl.players[i].name)	//empty slot
			continue;
		if (cl.players[i].spectator)	//shoo!
			continue;
		if (i == self)
			continue;
		if (!showenemies && strcmp(cl.players[i].team, cl.players[self].team))
			continue;
		players->client = i;

		pl = &cl.players[i];
		if (pl->tinfo.time > cl.time)
		{	//mod is explicitly telling us this junk
			players->items = pl->tinfo.items;
			players->health = pl->tinfo.health;
			players->armor = pl->tinfo.armour;
			VectorCopy(pl->tinfo.org, players->org);
			Q_strncpyz(players->nick, pl->tinfo.nick, sizeof(players->nick));
		}
		else if (i == self)
		{	//oh hey look, its me.
			players->items = cl.playerview[seat].stats[STAT_ITEMS];
			players->armor = cl.playerview[seat].statsf[STAT_ARMOR];
			players->health = cl.playerview[seat].statsf[STAT_HEALTH];
			Q_strncpyz(players->nick, "", sizeof(players->nick));
		}
		else if (cls.demoplayback == DPB_MVD)
		{	//scrape it from the mvd (assuming there is one...
			players->items = cl.players[i].stats[STAT_ITEMS];
			players->armor = cl.players[i].statsf[STAT_ARMOR];
			players->health = cl.players[i].statsf[STAT_HEALTH];
			Q_strncpyz(players->nick, "", sizeof(players->nick));
			
			VectorClear(players->org);
		}
		else
			continue;	//no stats, don't bother telling the plugin.

		//scrape origin from interpolation, if its more valid.
		if (i+1 < cl.maxlerpents && cl.lerpentssequence && cl.lerpents[i+1].sequence == cl.lerpentssequence)
		{
			le = &cl.lerpents[i+1];
			VectorCopy(le->origin, players->org);
		}
		else if (cl.lerpentssequence && cl.lerpplayers[i].sequence == cl.lerpentssequence)
		{
			le = &cl.lerpplayers[i];
			VectorCopy(le->origin, players->org);
		}

		players++;
		maxplayers--;
		count++;
	}
	
	return count;
}
#endif
#ifdef QUAKEHUD
static int QDECL Plug_GetWeaponStats(int self, struct wstats_s *result, size_t maxresults)
{
	//FIXME: we should support some way to clear this to 0 again, other than nosave.
	Cvar_Get("wpsx", "1", CVAR_USERINFO|CVAR_NOSAVE, "Hacks because ktx sucks. Must be 1 in order to receive weapon stats information in ktx.");

	if (self < 0)
	{
		unsigned int seat = (unsigned)(-self-1)%MAX_SPLITS;
		self = cl.playerview[seat].playernum;
		if (cl.playerview[seat].cam_state != CAM_FREECAM)
			self = cl.playerview[seat].cam_spec_track;
	}
	if (self < 0)
		return 0;

	if (maxresults > countof(cl.players[self].weaponstats))
		maxresults = countof(cl.players[self].weaponstats);
	memcpy(result, cl.players[self].weaponstats, sizeof(*result) * maxresults);
	return maxresults;
}
#endif

static qboolean QDECL Plug_Con_SubPrint(const char *name, const char *text)
{
	console_t *con;
	if (!name)
		name = "";

	if (qrenderer == QR_NONE)
	{
		if (!*name)
		{
			Con_Printf("%s", text);
			return true;
		}
		return false;
	}

	con = Con_FindConsole(name);
	if (!con)
	{
		con = Con_Create(name, 0);
		Con_SetActive(con);

		if (currentplug->conexecutecommand)
		{
			con->notif_x = 0;
			con->notif_y = 8*4;
			con->notif_w = vid.width;
			con->notif_t = 8;
			con->notif_l = 4;
			con->flags |= CONF_NOTIFY;
			con->userdata = currentplug;
			con->linebuffered = Plug_SubConsoleCommand;
		}
	}

	Con_PrintCon(con, text, con->parseflags);

	return true;
}
static qboolean QDECL Plug_Con_RenameSub(const char *oldname, const char *newname)
{
	console_t *con;
	if (qrenderer == QR_NONE)
		return false;
	con = Con_FindConsole(oldname);
	if (!con)
		return false;

	Q_strncpyz(con->name, newname, sizeof(con->name));

	return true;
}
static qboolean QDECL Plug_Con_IsActive(const char *conname)
{
	console_t *con;
	if (qrenderer == QR_NONE)
		return false;
	con = Con_FindConsole(conname);
	if (!con)
		return false;

	return Con_IsActive(con);
}
static qboolean QDECL Plug_Con_SetActive(const char *conname)
{
	console_t *con;
	if (qrenderer == QR_NONE)
		return false;
	con = Con_FindConsole(conname);
	if (!con)
		con = Con_Create(conname, 0);

	Con_SetActive(con);
	return true;
}
static qboolean QDECL Plug_Con_Destroy(const char *conname)
{
	console_t *con;
	if (qrenderer == QR_NONE)
		return false;
	con = Con_FindConsole(conname);
	if (!con)
		return false;

	Con_Destroy(con);
	return true;
}
static qboolean QDECL Plug_Con_NameForNum(qintptr_t connum, char *outconname, size_t connamesize)
{
	if (qrenderer == QR_NONE)
		return false;

	return Con_NameForNum(connum, outconname, connamesize);
}

static float QDECL Plug_Con_GetConsoleFloat(const char *conname, const char *attrib)
{
	float ret;
	console_t *con = Con_FindConsole(conname);
	ret = -1;

	if (!con)
		ret = -1;
	else if (!strcmp(attrib, "unseen"))
		ret = con->unseentext;
	else if (!strcmp(attrib, "markup"))	
	{
		if (con->parseflags & PFS_NOMARKUP)
			ret = 0;
		else if (con->parseflags & PFS_KEEPMARKUP)
			ret = 2;
		else
			ret = 1;
	}
	else if (!strcmp(attrib, "forceutf8"))
		ret = (con->parseflags&PFS_FORCEUTF8)?true:false;
	else if (!strcmp(attrib, "hidden"))
		ret = (con->flags & CONF_HIDDEN)?true:false;
	else if (!strcmp(attrib, "iswindow"))
		ret = (con->flags & CONF_ISWINDOW)?true:false;
	else if (!strcmp(attrib, "maxlines"))
		ret = con->maxlines;
	else if (!strcmp(attrib, "wnd_x"))
		ret = con->wnd_x;
	else if (!strcmp(attrib, "wnd_y"))
		ret = con->wnd_y;
	else if (!strcmp(attrib, "wnd_w"))
		ret = con->wnd_w;
	else if (!strcmp(attrib, "wnd_h"))
		ret = con->wnd_h;
	else if (!strcmp(attrib, "linecount"))
		ret = con->linecount;

	return ret;
}

static qboolean QDECL Plug_Con_SetConsoleFloat(const char *conname, const char *attrib, float val)
{
	console_t *con = Con_FindConsole(conname);

	if (!con)
	{
		con = Con_Create(conname, 0);
		if (!con)
			return false;
		con->userdata = currentplug;
		con->linebuffered = Plug_SubConsoleCommand;
	}

	if (!strcmp(attrib, "unseen"))
		con->unseentext = !!val;
	else if (!strcmp(attrib, "markup"))	
	{
		int cur = val;
		con->parseflags &= ~(PFS_NOMARKUP|PFS_KEEPMARKUP);
		if (cur == 0)
			con->parseflags |= PFS_NOMARKUP;
		else if (cur == 2)
			con->parseflags |= PFS_KEEPMARKUP;
	}
	else if (!strcmp(attrib, "forceutf8"))
		con->parseflags = (con->parseflags & ~PFS_FORCEUTF8) | (val?PFS_FORCEUTF8:0);
	else if (!strcmp(attrib, "hidden"))
		con->flags = (con->flags & ~CONF_HIDDEN) | (val?CONF_HIDDEN:0);
	else if (!strcmp(attrib, "iswindow"))
	{
		con->flags = (con->flags & ~CONF_ISWINDOW) | (val?CONF_ISWINDOW:0);
		con->flags = (con->flags & ~CONF_NOTIFY) | (val>1?CONF_NOTIFY:0);
		if (con_curwindow == con && !(con->flags & CONF_ISWINDOW))
			con_curwindow = NULL;
		else if (!con_curwindow && (con->flags & CONF_ISWINDOW))
			con_curwindow = con;
	}
	else if (!strcmp(attrib, "maxlines"))
		con->maxlines = val;
	else if (!strcmp(attrib, "wnd_x"))
		con->wnd_x = val;
	else if (!strcmp(attrib, "wnd_y"))
		con->wnd_y = val;
	else if (!strcmp(attrib, "wnd_w"))
		con->wnd_w = val;
	else if (!strcmp(attrib, "wnd_h"))
		con->wnd_h = val;
	else if (!strcmp(attrib, "linebuffered"))
	{
		con->userdata = currentplug;
		if (val == 2)
			con->linebuffered = NULL;//Con_Navigate;
		else if (val == 1)
			con->linebuffered = Plug_SubConsoleCommand;
		else
			con->linebuffered = NULL;
	}
	else if (!strcmp(attrib, "linecount"))
	{
		if (val == 0)
		{
			int pfl = con->parseflags;
			Con_ClearCon(con);
			con->parseflags = pfl;
		}
		else
			return false;
	}
	else
		return false;
	return true;
}

static qboolean QDECL Plug_Con_GetConsoleString(const char *conname, const char *attrib, char *value, size_t size)
{
	console_t *con = Con_FindConsole(conname);

	if (!con)
		return false;
	else if (!strcmp(attrib, "footer"))
		;
	else if (!strcmp(attrib, "title"))
	{
		Q_strncpyz(value, con->title, size);
	}
	else if (!strcmp(attrib, "icon"))
	{
		Q_strncpyz(value, con->icon, size);
	}
	else if (!strcmp(attrib, "prompt"))
	{
		Q_strncpyz(value, con->prompt, size);
	}
	else if (!strcmp(attrib, "backimage"))
	{
		if (con->backshader)
			Q_strncpyz(value, con->backshader->name, size);
		else
			Q_strncpyz(value, con->backimage, size);
	}
	else
		return false;
	return true;
}
static qboolean QDECL Plug_Con_SetConsoleString(const char *conname, const char *attrib, const char *value)
{
	console_t *con = Con_FindConsole(conname);

	if (!con)
	{
		con = Con_Create(conname, 0);
		if (!con)
			return false;
		con->userdata = currentplug;
		con->linebuffered = Plug_SubConsoleCommand;
	}
	if (!con)
		return false;
	else if (!strcmp(attrib, "footer"))
		Con_Footerf(con, false, "%s", value);
	else if (!strcmp(attrib, "title"))
		Q_strncpyz(con->title, value, sizeof(con->title));
	else if (!strcmp(attrib, "icon"))
		Q_strncpyz(con->icon, value, sizeof(con->icon));
	else if (!strcmp(attrib, "prompt"))
		Q_strncpyz(con->prompt, value, sizeof(con->prompt));
	else if (!strcmp(attrib, "backimage"))
	{
		Q_strncpyz(con->backimage, value, sizeof(con->backimage));
		if (con->backshader)
			R_UnloadShader(con->backshader);
	}
	else if (!strcmp(attrib, "backvideomap"))
	{
		Q_strncpyz(con->backimage, "", sizeof(con->backimage));
		if (con->backshader)
			R_UnloadShader(con->backshader);
		if (qrenderer != QR_NONE)
			con->backshader = R_RegisterCustom(NULL, va("consolevid_%s", con->name), SUF_NONE, Shader_DefaultCinematic, value);
		else
			con->backshader = NULL;
	}
	else
		return false;
	return true;
}

static void QDECL Plug_S_RawAudio(int sourceid, void *data, int speed, int samples, int channels, int width, float volume)
{
	S_RawAudio(sourceid, data, speed, samples, channels, width, volume);
}
static void QDECL S_Spacialize(unsigned int seat, int entnum, vec3_t origin, vec3_t axis[3], int reverb, vec3_t velocity)
{
	if (seat >= countof(cl.playerview))
		return;
	cl.playerview[seat].audio.defaulted = false;
	cl.playerview[seat].audio.entnum = entnum;
	VectorCopy(origin, cl.playerview[seat].audio.origin);
	VectorCopy(axis[0], cl.playerview[seat].audio.forward);
	VectorCopy(axis[1], cl.playerview[seat].audio.right);
	VectorCopy(axis[2], cl.playerview[seat].audio.up);
	cl.playerview[seat].audio.reverbtype = reverb;
	VectorCopy(velocity, cl.playerview[seat].audio.velocity);
}
static sfx_t *QDECL Plug_S_PrecacheSound(const char *sndname)
{
	return S_PrecacheSound(sndname);
}

static void Plug_Client_Close(plugin_t *plug)
{
	menu_t *m = Menu_FindContext(currentplug);

	if (m)
		Menu_Unlink(m, true);
	if (protocolclientplugin == plug)
	{
		protocolclientplugin = NULL;
		if (cls.protocol == CP_PLUGIN)
			cls.protocol = CP_UNKNOWN;
	}
}






