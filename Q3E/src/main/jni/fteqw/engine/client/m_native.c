#include "quakedef.h"
#ifdef MENU_NATIVECODE
static dllhandle_t *libmenu;
menu_export_t *mn_entry;

extern unsigned int r2d_be_flags;
#include "pr_common.h"
#include "shader.h"
#include "cl_master.h"

//static void MN_Menu_VideoReset(struct menu_s *m);
//static void MN_Menu_Released(struct menu_s *m);
static qboolean MN_Menu_KeyEvent(struct menu_s *m, qboolean isdown, unsigned int devid, int key, int unicode)
{
	if (mn_entry && mn_entry->InputEvent)
	{
		void *nctx = (m->ctx==libmenu)?NULL:m->ctx;
		struct menu_inputevent_args_s ev = {isdown?MIE_KEYDOWN:MIE_KEYUP, devid};
		ev.key.scancode = key;
		ev.key.charcode = unicode;
		return mn_entry->InputEvent(nctx, ev);
	}
	return false;
}
static qboolean MN_Menu_MouseMove(struct menu_s *m, qboolean abs, unsigned int devid, float x, float y)
{
	if (mn_entry && mn_entry->InputEvent)
	{
		void *nctx = (m->ctx==libmenu)?NULL:m->ctx;
		struct menu_inputevent_args_s ev = {abs?MIE_MOUSEABS:MIE_MOUSEDELTA, devid};
		ev.mouse.delta[0] = x;
		ev.mouse.delta[1] = y;
		ev.mouse.screen[0] = mousecursor_x;
		ev.mouse.screen[1] = mousecursor_y;
		return mn_entry->InputEvent(nctx, ev);
	}
	return false;
}
static qboolean MN_Menu_JoyAxis(struct menu_s *m, unsigned int devid, int axis, float val)
{
	if (mn_entry && mn_entry->InputEvent)
	{
		void *nctx = (m->ctx==libmenu)?NULL:m->ctx;
		struct menu_inputevent_args_s ev = {MIE_JOYAXIS, devid};
		ev.axis.axis = axis;
		ev.axis.val = val;
		return mn_entry->InputEvent(nctx, ev);
	}
	return false;
}
static void MN_Menu_DrawMenu(struct menu_s *m)
{
	void *nctx = (m->ctx==libmenu)?NULL:m->ctx;
	if (mn_entry && mn_entry->Draw)
		mn_entry->Draw(nctx, host_frametime);
	else
		Menu_Unlink(m);
}

static int MN_CheckExtension(const char *extname)
{
	unsigned int i;
	for (i = 0; i < QSG_Extensions_count; i++)
	{
		if (!strcmp(QSG_Extensions[i].name, extname))
			return true;
	}
	return false;
}
static void MN_LocalCmd(const char *text)
{
	Cbuf_AddText(text, RESTRICT_LOCAL);	//menus are implicitly trusted. latching and other stuff would be a nightmare otherwise.
}
static const char *MN_Cvar_String(const char *cvarname, qboolean effective)
{
	cvar_t *cv = Cvar_FindVar(cvarname);
	if (cv)
	{	//some cvars don't change instantly, giving them (temporary) effective values that are different from their intended values.
		if (cv->latched_string && !effective)
			return cv->latched_string;
		return cv->string;
	}
	else
		return NULL;
}
static const char *MN_Cvar_GetDefault(const char *cvarname)
{
	cvar_t *cv = Cvar_FindVar(cvarname);
	if (cv)
		return cv->defaultstr?cv->defaultstr:"";
	else
		return NULL;
}
static void MN_RegisterCvar(const char *cvarname, const char *defaulttext, unsigned int flags, const char *description)
{
	Cvar_Get2(cvarname, defaulttext, flags, description, NULL);
}
static void MN_RegisterCommand(const char *commandname, const char *description)
{
	if (!Cmd_Exists(commandname)) {
		Cmd_AddCommandD(commandname, NULL, description);
	}
}
static int MN_GetServerState(void)
{
	if (!sv.active)
		return 0;
	if (svs.allocated_client_slots <= 1)
		return 1;
	return 2;
}
static int MN_GetClientState(char const ** disconnect_reason)
{
	extern cvar_t cl_disconnectreason;
	*disconnect_reason = NULL;
	if (cls.state >= ca_active)
		return 2;
	if (cls.state != ca_disconnected)
		return 1;
	*disconnect_reason = (const char*)cl_disconnectreason.string;
	return 0;
}
static void MN_fclose(vfsfile_t *f)
{
	VFS_CLOSE(f);
}
static shader_t *MN_CachePic(const char *picname)
{
	return R2D_SafeCachePic(picname);
}
static qboolean MN_DrawGetImageSize(struct shader_s *pic, int *w, int *h)
{
	return R_GetShaderSizes(pic, w, h, true)>0;
}
static void MN_DrawQuad(const vec2_t position[4], const vec2_t texcoords[4], shader_t *pic, const vec4_t rgba, unsigned int be_flags)
{
	extern shader_t *shader_draw_fill, *shader_draw_fill_trans;
	r2d_be_flags = be_flags;
	if (!pic)
		pic = rgba[3]==1?shader_draw_fill:shader_draw_fill_trans;
	R2D_ImageColours(rgba[0], rgba[1], rgba[2], rgba[3]);
	R2D_Image2dQuad(position, texcoords, NULL, pic);
	r2d_be_flags = 0;
}
static float MN_DrawString(const vec2_t position, const char *text, struct font_s *font, float height, const vec4_t rgba, unsigned int be_flags)
{
	float px, py, ix;
	unsigned int codeflags, codepoint;
	conchar_t buffer[2048], *str = buffer;
	if (!font)
		font = font_default;

	COM_ParseFunString(CON_WHITEMASK, text, buffer, sizeof(buffer), false);

	R2D_ImageColours(rgba[0], rgba[1], rgba[2], rgba[3]);
	Font_BeginScaledString(font, position[0], position[1], height, height, &px, &py);
	ix=px;
	while(*str)
	{
		str = Font_Decode(str, &codeflags, &codepoint);
		px = Font_DrawScaleChar(px, py, codeflags, codepoint);
	}
	Font_EndString(font);
	return ((px-ix)*(float)vid.width)/(float)vid.rotpixelwidth;
}
static float MN_StringWidth(const char *text, struct font_s *font, float height)
{
	float px, py;
	conchar_t buffer[2048], *end;
	if (!font)
		font = font_default;

	end = COM_ParseFunString(CON_WHITEMASK, text, buffer, sizeof(buffer), false);

	Font_BeginScaledString(font, 0, 0, height, height, &px, &py);
	px = Font_LineScaleWidth(buffer, end);
	Font_EndString(font);
	return (px * (float)vid.width) / (float)vid.rotpixelwidth;
}
static void MN_DrawSetClipArea(float x, float y, float width, float height)
{
	srect_t srect;
	if (R2D_Flush)
		R2D_Flush();

	srect.x = x / (float)vid.fbvwidth;
	srect.y = y / (float)vid.fbvheight;
	srect.width = width / (float)vid.fbvwidth;
	srect.height = height / (float)vid.fbvheight;
	srect.dmin = -99999;
	srect.dmax = 99999;
	srect.y = (1-srect.y) - srect.height;
	BE_Scissor(&srect);
}
static void MN_DrawResetClipArea(void)
{
	if (R2D_Flush)
		R2D_Flush();
	BE_Scissor(NULL);
}
static void MN_PushMenu(void *ctx)
{
	menu_t *m;
	if (!ctx)
		ctx = libmenu;	//to match kill
	m = Menu_FindContext(ctx);
	if (!m)
	{	//not created yet.
		m = Z_Malloc(sizeof(*m));
		m->ctx = ctx;

//		m->videoreset	= MN_Menu_VideoReset;
//		m->release		= MN_Menu_Released;
		m->keyevent		= MN_Menu_KeyEvent;
		m->mousemove	= MN_Menu_MouseMove;
		m->joyaxis		= MN_Menu_JoyAxis;
		m->drawmenu		= MN_Menu_DrawMenu;
	}
	m->cursor = &key_customcursor[kc_nativemenu];
	Menu_Push(m, false);

	if (ctx == libmenu)
		ctx = NULL;
	if (m->cursor)
	{	//we're activating the mouse cursor now... make sure the position is actually current.
		//FIXME: we should probably get the input code to do this for us when switching cursor modes.
		struct menu_inputevent_args_s ev = {MIE_MOUSEABS, -1};
		ev.mouse.screen[0] = mousecursor_x;
		ev.mouse.screen[1] = mousecursor_y;
		mn_entry->InputEvent(ctx, ev);
	}
}
static qboolean MN_IsMenuPushed(void *ctx)
{
	menu_t *m;
	if (!ctx)
		ctx = libmenu;	//to match kill
	m = Menu_FindContext(ctx);
	return !!m;
}
static void MN_KillMenu(void *ctx)
{
	menu_t *m;
	if (!ctx)
		ctx = libmenu;	//don't allow null contexts, because that screws up any other menus.
	m = Menu_FindContext(ctx);
	if (m)
		Menu_Unlink(m);
}
static int MN_SetMouseTarget(const char *cursorname, float hot_x, float hot_y, float scale)
{
	if (cursorname)
	{
		struct key_cursor_s *m = &key_customcursor[kc_nativemenu];
		if (scale <= 0)
			scale = 1;
		if (!strcmp(m->name, cursorname) || m->hotspot[0] != hot_x || m->hotspot[1] != hot_y || m->scale != scale)
		{
			Q_strncpyz(m->name, cursorname, sizeof(m->name));
			m->hotspot[0] = hot_x;
			m->hotspot[1] = hot_y;
			m->scale = scale;
			m->dirty = true;
		}
	}
	return true;
}

static model_t *MN_CacheModel(const char *name)
{
	return Mod_ForName(name, MLV_SILENT);
}
static qboolean MN_GetModelSize(model_t *model, vec3_t out_mins, vec3_t out_maxs)
{
	if (model)
	{
		while(model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);

		VectorCopy(model->mins, out_mins);
		VectorCopy(model->maxs, out_maxs);
		return model->loadstate == MLS_LOADED;
	}
	VectorClear(out_mins);
	VectorClear(out_maxs);
	return false;
}
static void MN_RenderScene(menuscene_t *scene)
{
	int i;
	entity_t ent;
	menuentity_t *e;
	if (R2D_Flush)
		R2D_Flush();

	CL_ClearEntityLists();
	memset(&ent, 0, sizeof(ent));
	for (i = 0; i < scene->numentities; i++)
	{
		e = &scene->entlist[i];
		ent.keynum = i;
		ent.model = scene->entlist[i].model;
		VectorCopy(e->matrix[0], ent.axis[0]); ent.origin[0] = e->matrix[0][3];
		VectorCopy(e->matrix[1], ent.axis[1]); ent.origin[1] = e->matrix[1][3];
		VectorCopy(e->matrix[2], ent.axis[2]); ent.origin[2] = e->matrix[2][3];

		ent.scale = 1;
		ent.framestate.g[FS_REG].frame[0] = e->frame[0];
		ent.framestate.g[FS_REG].frame[1] = e->frame[1];
		ent.framestate.g[FS_REG].lerpweight[1] = e->frameweight[0];
		ent.framestate.g[FS_REG].lerpweight[0] = e->frameweight[1];
		ent.framestate.g[FS_REG].frametime[0] = e->frametime[0];
		ent.framestate.g[FS_REG].frametime[1] = e->frametime[1];

		ent.playerindex = -1;
		ent.topcolour = TOP_DEFAULT;
		ent.bottomcolour = BOTTOM_DEFAULT;
		Vector4Set(ent.shaderRGBAf, 1, 1, 1, 1);
		VectorSet(ent.glowmod, 1, 1, 1);
#ifdef HEXEN2
		ent.drawflags = SCALE_ORIGIN_ORIGIN;
		ent.abslight = 0;
#endif
		ent.skinnum = 0;
		ent.fatness = 0;
		ent.forcedshader = NULL;
		ent.customskin = 0;

		V_AddAxisEntity(&ent);
	}

	VectorCopy(scene->viewmatrix[0], r_refdef.viewaxis[0]); r_refdef.vieworg[0] = scene->viewmatrix[0][3];
	VectorCopy(scene->viewmatrix[1], r_refdef.viewaxis[1]); r_refdef.vieworg[1] = scene->viewmatrix[1][3];
	VectorCopy(scene->viewmatrix[2], r_refdef.viewaxis[2]); r_refdef.vieworg[2] = scene->viewmatrix[2][3];
	
	r_refdef.viewangles[0] = -(atan2(r_refdef.viewaxis[0][2], sqrt(r_refdef.viewaxis[0][1]*r_refdef.viewaxis[0][1]+r_refdef.viewaxis[0][0]*r_refdef.viewaxis[0][0])) * 180 / M_PI);
	r_refdef.viewangles[1] = (atan2(r_refdef.viewaxis[0][1], r_refdef.viewaxis[0][0]) * 180 / M_PI);
	r_refdef.viewangles[2] = 0;

	r_refdef.flags = 0;
	if (scene->worldmodel && scene->worldmodel == cl.worldmodel)
		r_refdef.flags &= ~RDF_NOWORLDMODEL;
	else
		r_refdef.flags |= RDF_NOWORLDMODEL;
	r_refdef.fovv_x = r_refdef.fov_x = scene->fov[0];
	r_refdef.fovv_y = r_refdef.fov_y = scene->fov[1];
	r_refdef.vrect.x = scene->pos[0];
	r_refdef.vrect.y = scene->pos[1];
	r_refdef.vrect.width = scene->size[0];
	r_refdef.vrect.height = scene->size[1];
	r_refdef.time = scene->time;
	r_refdef.useperspective = true;
	r_refdef.mindist = bound(0.1, gl_mindist.value, 4);
	r_refdef.maxdist = gl_maxdist.value;
	r_refdef.playerview = &cl.playerview[0];

	memset(&r_refdef.globalfog, 0, sizeof(r_refdef.globalfog));
	r_refdef.areabitsknown = false;

	R_RenderView();
	r_refdef.playerview = NULL;
	r_refdef.time = 0;
}

void MN_Shutdown(void)
{
	if (mn_entry)
	{
		mn_entry->Shutdown(MI_INIT);
		mn_entry = NULL;
	}
	if (libmenu)
	{
		Sys_CloseLibrary(libmenu);
		libmenu = NULL;
	}
}
qboolean MN_Init(void)
{
	menu_export_t *(QDECL *pGetMenuAPI) ( menu_import_t *import );
	static menu_import_t imports =
	{
		NATIVEMENU_API_VERSION_MAX,
		NULL,

		MN_CheckExtension,
		Host_Error,
		Con_Printf,
		Con_DPrintf,
		MN_LocalCmd,
		Cvar_VariableValue,
		MN_Cvar_String,
		MN_Cvar_GetDefault,
		Cvar_SetNamed,
		MN_RegisterCvar,
		MN_RegisterCommand,

		COM_ParseType,

		MN_GetServerState,
		MN_GetClientState,
		S_LocalSound2,
	
		// file input / search crap
		FS_OpenVFS,
		MN_fclose,
		VFS_GETS,
		VFS_PRINTF,
		COM_EnumerateFiles,
		FS_SystemPath,

		// Drawing stuff
		MN_DrawSetClipArea,
		MN_DrawResetClipArea,

		//pics
		MN_CachePic,
		MN_DrawGetImageSize,
		MN_DrawQuad,

		//strings
		MN_DrawString,
		MN_StringWidth,
		Font_LoadFont,
		Font_Free,

		//3d stuff
		MN_CacheModel,
		MN_GetModelSize,
		MN_RenderScene,

		// Menu specific stuff
		MN_PushMenu,
		MN_IsMenuPushed,
		MN_KillMenu,
		MN_SetMouseTarget,
		Key_KeynumToString,
		Key_StringToKeynum,
		M_FindKeysForBind,

		// Server browser stuff
		Master_KeyForName,
		Master_SortedServer,
		Master_ReadKeyString,
		Master_ReadKeyFloat,

		Master_ClearMasks,
		Master_SetMaskString,
		Master_SetMaskInteger,
		Master_SetSortField,
		Master_SortServers,
		MasterInfo_Refresh,
		CL_QueryServers,
	};
	dllfunction_t funcs[] =
	{
		{(void*)&pGetMenuAPI, "GetMenuAPI"},
		{NULL}
	};
	void *iterator = NULL;
	char syspath[MAX_OSPATH];
	char gamepath[MAX_QPATH];

	while(COM_IteratePaths(&iterator, syspath, sizeof(syspath), gamepath, sizeof(gamepath)))
	{
		if (com_gamedirnativecode.ival)
			libmenu = Sys_LoadLibrary(va("%smenu_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, syspath), funcs);
		if (libmenu)
			break;

		if (host_parms.binarydir && !strchr(gamepath, '/') && !strchr(gamepath, '\\'))
			libmenu = Sys_LoadLibrary(va("%smenu_"ARCH_CPU_POSTFIX ARCH_DL_POSTFIX, host_parms.binarydir), funcs);
		if (libmenu)
			break;

		//some build systems don't really know the cpu type.
		if (host_parms.binarydir && !strchr(gamepath, '/') && !strchr(gamepath, '\\'))
			libmenu = Sys_LoadLibrary(va("%smenu_%s" ARCH_DL_POSTFIX, host_parms.binarydir, gamepath), funcs);
		if (libmenu)
			break;
	}

	if (libmenu)
	{
		imports.engine_version = version_string();

		mn_entry = pGetMenuAPI (&imports); 
		if (mn_entry && mn_entry->api_version >= NATIVEMENU_API_VERSION_MIN && mn_entry->api_version <= NATIVEMENU_API_VERSION_MAX)
		{
			mn_entry->Init(0, vid.width, vid.height, vid.pixelwidth, vid.pixelheight);
			return true;
		}
		else
			mn_entry = NULL;
		MN_Shutdown();
		Sys_CloseLibrary(libmenu);
		libmenu = NULL;
	}

	return false;
}
#endif
