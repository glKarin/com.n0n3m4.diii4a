#include "quakedef.h"
#include "shader.h"

#ifdef _WIN32
#include "winquake.h"
#endif

#ifdef HLCLIENT
extern unsigned int r2d_be_flags;
struct hlcvar_s *QDECL GHL_CVarGetPointer(char *varname);



#if defined(_MSC_VER)
#	if _MSC_VER >= 1300
#		define __func__ __FUNCTION__
#	else
#		define __func__ "unknown"
#	endif
#else
	//I hope you're c99 and have a __func__
#endif

//extern cvar_t temp1;
#define ignore(s) Con_Printf("Fixme: " s "\n")
#define notimpl(l) Con_Printf("halflife cl builtin not implemented on line %i\n", l)
#define notimpf(f) Con_Printf("halflife cl builtin %s not implemented\n", f)
#define notimp() Con_Printf("halflife cl builtin %s not implemented\n", __func__)
#define bi_begin() //if (temp1.ival)Con_Printf("enter %s\n", __func__)
#define bi_end() //if (temp1.ival)Con_Printf("leave %s\n", __func__)
#define bi_trace() bi_begin(); bi_end()

#if HLCLIENT >= 1
#define HLCL_API_VERSION HLCLIENT
#else
#define HLCL_API_VERSION 7
#endif



void *vgui_panel;
void *(QDECL *vgui_init)(void);
void (QDECL *vgui_frame)(void);
void (QDECL *vgui_key)(int down, int scan);
void (QDECL *vgui_mouse)(int x, int y);

qboolean VGui_Setup(void)
{
	void *vguidll;

	dllfunction_t funcs[] =
	{
		{(void*)&vgui_init, "init"},
		{(void*)&vgui_frame, "frame"},
		{(void*)&vgui_key, "key"},
		{(void*)&vgui_mouse, "mouse"},
		{NULL}
	};

	vguidll = Sys_LoadLibrary("vguiwrap", funcs);

	if (vguidll)
		vgui_panel = vgui_init();
	return !!vgui_panel;
}



#define HLPIC model_t*

typedef struct
{
	int	l;
	int r;
	int t;
	int b;
} hlsubrect_t;

typedef struct
{
	vec3_t origin;

#if HLCL_API_VERSION >= 7
	vec3_t viewangles;
	int weapons;
	float fov;
#else
	float viewheight;
	float maxspeed;
	vec3_t viewangles;
	vec3_t punchangles;
	int keys;
	int weapons;
	float fov;
	float idlescale;
	float mousesens;
#endif
} hllocalclientdata_t;

typedef struct
{
	short lerpmsecs;
	qbyte msec;
	//pad1
	vec3_t viewangles;
	float forwardmove;
	float sidemove;
	float upmove;
	qbyte lightlevel;
	//pad1
	unsigned short buttons;
	qbyte impulse;
	qbyte weaponselect;
	//pad2
	int impact_index;
	vec3_t impact_position;
} hlusercmd_t;

typedef struct
{
	char name[64];
	char sprite[64];
	int unk;
	int forscrwidth;
	hlsubrect_t rect;
} hlspriteinf_t;

typedef struct 
{
	char *name;
	short ping;
	qbyte isus;
	qbyte isspec;
	qbyte pl;
	//pad3
	char *model;
	short tcolour;
	short bcolour;
} hlplayerinfo_t;

typedef struct
{
	int size;
	int width;
	int height;
	int flags;
	int charheight;
	short charwidths[256];
} hlscreeninfo_t;

typedef struct
{
	int effect;
	byte_vec4_t c1;
	byte_vec4_t c2;
	float x;
	float y;
	float fadein;
	float fadeout;
	float holdtime;
	float fxtime;
	char *name;
	char *message;
} hlmsginfo_t;

typedef struct
{
	HLPIC (QDECL *pic_load) (char *picname);
	int (QDECL *pic_getnumframes) (HLPIC pic);
	int (QDECL *pic_getheight) (HLPIC pic, int frame);
	int (QDECL *pic_getwidth) (HLPIC pic, int frame);
	void (QDECL *pic_select) (HLPIC pic, int r, int g, int b);
	void (QDECL *pic_drawcuropaque) (int frame, int x, int y, void *loc);
	void (QDECL *pic_drawcuralphatest) (int frame, int x, int y, void *loc);
	void (QDECL *pic_drawcuradditive) (int frame, int x, int y, void *loc);
	void (QDECL *pic_enablescissor) (int x, int y, int width, int height);
	void (QDECL *pic_disablescissor) (void);
	hlspriteinf_t *(QDECL *pic_parsepiclist) (char *filename, int *numparsed);

	void (QDECL *fillrgba) (int x, int y, int width, int height, int r, int g, int b, int a);
	int (QDECL *getscreeninfo) (hlscreeninfo_t *info);
	void (QDECL *setcrosshair) (HLPIC pic, hlsubrect_t rect, int r, int g, int b);	//I worry about stuff like this
	
	struct hlcvar_s *(QDECL *cvar_register) (char *name, char *defvalue, int flags);
	float (QDECL *cvar_getfloat) (char *name);
	char *(QDECL *cvar_getstring) (char *name);

	void (QDECL *cmd_register) (char *name, void (*func) (void));
	void (QDECL *hooknetmsg) (char *msgname, void *func);
	void (QDECL *forwardcmd) (char *command);
	void (QDECL *localcmd) (char *command);

	void (QDECL *getplayerinfo) (int entnum, hlplayerinfo_t *result);

	void (QDECL *startsound_name) (char *name, float vol);
	void (QDECL *startsound_idx) (int idx, float vol);

	void (QDECL *anglevectors) (float *ina, float *outf, float *outr, float *outu);

	hlmsginfo_t *(QDECL *get_message_info) (char *name);	//translated+scaled+etc intro stuff
	int (QDECL *drawchar) (int x, int y, int charnum, int r, int g, int b);
	int (QDECL *drawstring) (int x, int y, char *string);
#if HLCL_API_VERSION >= 7
	void (QDECL *settextcolour) (float r, float b, float g);
#endif
	void (QDECL *drawstring_getlen) (char *string, int *outlen, int *outheight);
	void (QDECL *consoleprint) (char *str);
	void (QDECL *centerprint) (char *str);

#if HLCL_API_VERSION >= 7
	int (QDECL *getwindowcenterx)(void);	//yes, really, window center. for use with Get/SetCursorPos, the windows function.
	int (QDECL *getwindowcentery)(void);	//yes, really, window center. for use with Get/SetCursorPos, the windows function.
	void (QDECL *getviewnangles)(float*ang);
	void (QDECL *setviewnangles)(float*ang);
	void (QDECL *getmaxclients)(float*ang);
	void (QDECL *cvar_setvalue)(char *cvarname, char *value);

	int (QDECL *cmd_argc)(void);
	char *(QDECL *cmd_argv)(int i);
	void (QDECL *con_printf)(char *fmt, ...);
	void (QDECL *con_dprintf)(char *fmt, ...);
	void (QDECL *con_notificationprintf)(int pos, char *fmt, ...);
	void (QDECL *con_notificationprintfex)(void *info, char *fmt, ...);	//arg1 is of specific type
	char *(QDECL *physkey)(char *key);
	char *(QDECL *serverkey)(char *key);
	float (QDECL *getclientmaxspeed)(void);
	int (QDECL *checkparm)(char *str, char **next);
	int (QDECL *keyevent)(int key, int down);
	void (QDECL *getmousepos)(int *outx, int *outy);
	int (QDECL *movetypeisnoclip)(void);
	struct hlclent_s *(QDECL *getlocalplayer)(void);
	struct hlclent_s *(QDECL *getviewent)(void);
	struct hlclent_s *(QDECL *getentidx)(int idx);
	float (QDECL *getlocaltime)(void);
	void (QDECL *calcshake)(void);
	void (QDECL *applyshake)(float *,float *,float);
	int (QDECL *pointcontents)(float *point, float *truecon);
	int (QDECL *waterentity)(float *point);
	void (QDECL *traceline) (float *start, float *end, int flags, int hull, int forprediction);

	model_t *(QDECL *loadmodel)(char *modelname, int *mdlindex);
	int (QDECL *addrentity)(int type, void *ent);

	model_t *(QDECL *modelfrompic) (HLPIC pic);
	void (QDECL *soundatloc)(char*sound, float volume, float *org);

	unsigned short (QDECL *precacheevent)(int evtype, char *name);
	void (QDECL *playevent)(int flags, struct hledict_s *ent, unsigned short evindex, float delay, float *origin, float *angles, float f1, float f2, int i1, int i2, int b1, int b2);
	void (QDECL *weaponanimate)(int anim, int body);
	float (QDECL *randfloat) (float minv, float maxv);
	long (QDECL *randlong) (long minv, long maxv);
	void (QDECL *hookevent) (char *name, void (*func)(struct hlevent_s *event));
	int (QDECL *con_isshown) (void);
	char *(QDECL *getgamedir) (void);
	struct hlcvar_s *(QDECL *cvar_find) (char *name);
	char *(QDECL *lookupbinding) (char *command);
	char *(QDECL *getlevelname) (void);
	void (QDECL *getscreenfade) (struct hlsfade_s *fade);
	void (QDECL *setscreenfade) (struct hlsfade_s *fade);
	void *(QDECL *vgui_getpanel) (void);
	void (QDECL *vgui_paintback) (int extents[4]);

	void *(QDECL *loadfile) (char *path, int onhunk, int *length);
	char *(QDECL *parsefile) (char *data, char *token);
	void (QDECL *freefile) (void *file);

	struct hl_tri_api_s
	{
		int vers;
		int sentinal;
	} *triapi;
	struct hl_sfx_api_s
	{
		int vers;
		int sentinal;
	} *efxapi;
	struct hl_event_api_s 
	{
		int vers;
		int sentinal;
	} *eventapi;
	struct hl_demo_api_s 
	{
		int (QDECL *isrecording)(void);
		int (QDECL *isplaying)(void);
		int (QDECL *istimedemo)(void);
		void (QDECL *writedata)(int size, void *data);

		int sentinal;
	} *demoapi;
	struct hl_net_api_s 
	{
		int vers;
		int sentinal;
	} *netapi;

	struct hl_voicetweek_s
	{
		int sentinal;
	} *voiceapi;

	int (QDECL *forcedspectator) (void);
	model_t *(QDECL *loadmapsprite) (char *name);

	void (QDECL *fs_addgamedir) (char *basedir, char *appname);
	int (QDECL *expandfilename) (char *filename, char *outbuff, int outsize);

	char *(QDECL *player_key) (int pnum, char *key);
	void (QDECL *player_setkey) (char *key, char *value);	//wait, no pnum?

	qboolean (QDECL *getcdkey) (int playernum, char key[16]);
	int (QDECL *trackerfromplayer) (int pl);
	int (QDECL *playerfromtracker) (int tr);
	int (QDECL *sendcmd_unreliable) (char *cmd);
	void (QDECL *getsysmousepos) (long *xandy);
	void (QDECL *setsysmousepos) (int x, int y);
	void (QDECL *setmouseenable) (qboolean enable);
#endif

	int sentinal;
} CLHL_enginecgamefuncs_t;


typedef struct
{
	int (QDECL *HUD_VidInit) (void);
	int (QDECL *HUD_Init) (void);
	int (QDECL *HUD_Shutdown) (void);
	int (QDECL *HUD_Redraw) (float maptime, int inintermission);
	int (QDECL *HUD_UpdateClientData) (hllocalclientdata_t *localclientdata, float maptime);
	int (QDECL *HUD_Reset) (void);
#if HLCL_API_VERSION >= 7
	void (QDECL *CL_CreateMove) (float frametime, hlusercmd_t *cmd, int isplaying);
	void (QDECL *IN_ActivateMouse) (void);
	void (QDECL *IN_DeactivateMouse) (void);
	void (QDECL *IN_MouseEvent) (int buttonmask);
#endif
} CLHL_cgamefuncs_t;



//FIXME
typedef struct
{
	vec3_t	origin;
	vec3_t	oldorigin;

	int firstframe;
	int numframes;

	int		type;
	vec3_t	angles;
	int		flags;
	float	alpha;
	float	start;
	float	framerate;
	model_t	*model;
	int skinnum;
} explosion_t;





typedef struct
{
	char name[64];
	int (QDECL *hook) (char *name, int bufsize, void *bufdata);
} CLHL_UserMessages_t;
CLHL_UserMessages_t usermsgs[256];

int numnewhooks;
CLHL_UserMessages_t pendingusermsgs[256];


static HLPIC selectedpic;

float hl_viewmodelsequencetime;
int hl_viewmodelsequencecur;
int hl_viewmodelsequencebody;





HLPIC QDECL CLGHL_pic_load (char *picname)
{
	void *ret;
	bi_begin();
	ret = Mod_ForName(picname, false);
	bi_end();
	return ret;
//	return R2D_SafeCachePic(picname);
}
int QDECL CLGHL_pic_getnumframes (HLPIC pic)
{
	bi_trace();
	if (pic)
		return pic->numframes;
	else
		return 0;
}

static mspriteframe_t *getspriteframe(HLPIC pic, int frame)
{
	msprite_t		*psprite;
	mspritegroup_t *pgroup;
	bi_trace();
	if (!pic)
		return NULL;
	psprite = pic->meshinfo;
	if (!psprite)
		return NULL;

	if (psprite->frames[frame].type == SPR_SINGLE)
		return psprite->frames[frame].frameptr;
	else
	{
		pgroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		return pgroup->frames[0];
	}
}
static mpic_t *getspritepic(HLPIC pic, int frame)
{
	mspriteframe_t *f;
	bi_trace();
	f = getspriteframe(pic, frame);
	if (f)
		return f->shader;
	return NULL;
}

int QDECL CLGHL_pic_getheight (HLPIC pic, int frame)
{
	mspriteframe_t *pframe;
	bi_trace();

	pframe = getspriteframe(pic, frame);
	if (!pframe)
		return 0;

	return pframe->shader->width;
}
int QDECL CLGHL_pic_getwidth (HLPIC pic, int frame)
{
	mspriteframe_t *pframe;
	bi_trace();

	pframe = getspriteframe(pic, frame);
	if (!pframe)
		return 0;

	return pframe->shader->height;
}
void QDECL CLGHL_pic_select (HLPIC pic, int r, int g, int b)
{
	bi_trace();
	selectedpic = pic;
	R2D_ImageColours(r/255.0f, g/255.0f, b/255.0f, 1);
}
void QDECL CLGHL_pic_drawcuropaque (int frame, int x, int y, hlsubrect_t *loc)
{
	mpic_t *pic = getspritepic(selectedpic, frame);
	bi_trace();
	if (!pic)
		return;

	//faster SW render: no blends/holes
	pic->flags &= ~1;

	R2D_Image(x, y,
		loc->r-loc->l, loc->b-loc->t,
		(float)loc->l/pic->width, (float)loc->t/pic->height,
		(float)loc->r/pic->width, (float)loc->b/pic->height,
		pic);
}
void QDECL CLGHL_pic_drawcuralphtest (int frame, int x, int y, hlsubrect_t *loc)
{
	mpic_t *pic = getspritepic(selectedpic, frame);
	bi_trace();
	if (!pic)
		return;
	//use some kind of alpha
	pic->flags |= 1;

	R2D_Image(x, y,
		loc->r-loc->l, loc->b-loc->t,
		(float)loc->l/pic->width, (float)loc->t/pic->height,
		(float)loc->r/pic->width, (float)loc->b/pic->height,
		pic);
}
void QDECL CLGHL_pic_drawcuradditive (int frame, int x, int y, hlsubrect_t *loc)
{
	mpic_t *pic = getspritepic(selectedpic, frame);

	bi_trace();
	if (!pic)
		return;

	r2d_be_flags = BEF_FORCEADDITIVE;

	//use some kind of alpha
	pic->flags |= 1;
	if (loc)
	{
		R2D_Image(x, y,
			loc->r-loc->l, loc->b-loc->t,
			(float)loc->l/pic->width, (float)loc->t/pic->height,
			(float)loc->r/pic->width, (float)loc->b/pic->height,
			pic);
	}
	else
	{
		R2D_Image(x, y,
			pic->width, pic->height,
			0, 0,
			1, 1,
			pic);
	}
	r2d_be_flags = 0;
}
void QDECL CLGHL_pic_enablescissor (int x, int y, int width, int height)
{
	srect_t srect;

	bi_trace();

	srect.x = x / vid.width;
	srect.y = y / vid.height;
	srect.width = width / vid.width;
	srect.height = height / vid.height;
	srect.dmin = -99999;
	srect.dmax = 99999;
	srect.y = (1-srect.y) - srect.height;
	BE_Scissor(&srect);
}
void QDECL CLGHL_pic_disablescissor (void)
{
	bi_trace();
	BE_Scissor(NULL);
}
hlspriteinf_t *QDECL CLGHL_pic_parsepiclist (char *filename, int *numparsed)
{
	hlspriteinf_t *result;
	int entry;
	int entries;
	void *file;
	char *pos;

	bi_trace();

	*numparsed = 0;

	FS_LoadFile(filename, &file);
	if (!file)
		return NULL;
	pos = file;

	pos = COM_Parse(pos);
	entries = atoi(com_token);

	//name, res, pic, x, y, w, h

	result = Z_Malloc(sizeof(*result)*entries);
	for (entry = 0; entry < entries; entry++)
	{
		pos = COM_Parse(pos);
		Q_strncpyz(result[entry].name, com_token, sizeof(result[entry].name));

		pos = COM_Parse(pos);
		result[entry].forscrwidth = atoi(com_token);

		pos = COM_Parse(pos);
		Q_strncpyz(result[entry].sprite, com_token, sizeof(result[entry].name));

		pos = COM_Parse(pos);
		result[entry].rect.l = atoi(com_token);

		pos = COM_Parse(pos);
		result[entry].rect.t = atoi(com_token);

		pos = COM_Parse(pos);
		result[entry].rect.r = result[entry].rect.l+atoi(com_token);

		pos = COM_Parse(pos);
		result[entry].rect.b = result[entry].rect.t+atoi(com_token);

		if (pos)
			*numparsed = entry;
	}

	if (!pos || COM_Parse(pos))
		Con_Printf("unexpected end of file\n");

	FS_FreeFile(file);

	return result;
}

void QDECL CLGHL_fillrgba (int x, int y, int width, int height, int r, int g, int b, int a)
{
	bi_trace();
}
int QDECL CLGHL_getscreeninfo (hlscreeninfo_t *info)
{
	int i;
	bi_trace();
	if (info->size != sizeof(*info))
		return false;

	info->width = vid.width;
	info->height = vid.height;
	info->flags = 0;
	info->charheight = 8;
	for (i = 0; i < 256; i++)
		info->charwidths[i] = 8;

	return true;
}
void QDECL CLGHL_setcrosshair (HLPIC pic, hlsubrect_t rect, int r, int g, int b)
{
	bi_trace();
}

struct hlcvar_s *QDECL CLGHL_cvar_register (char *name, char *defvalue, int flags)
{
	bi_trace();
	if (Cvar_Get(name, defvalue, 0, "Halflife cvars"))
		return GHL_CVarGetPointer(name);
	else
		return NULL;
}
float QDECL CLGHL_cvar_getfloat (char *name)
{
	cvar_t *var = Cvar_FindVar(name);
	bi_trace();
	if (var)
		return var->value;
	return 0;
}
char *QDECL CLGHL_cvar_getstring (char *name)
{
	cvar_t *var = Cvar_FindVar(name);
	bi_trace();
	if (var)
		return var->string;
	return "";
}

void QDECL CLGHL_cmd_register (char *name, xcommand_t func)
{
	bi_trace();
	Cmd_AddCommand(name, func);
}
void QDECL CLGHL_hooknetmsg (char *msgname, void *func)
{
	int i;
	bi_trace();
	//update the current list now.
	for (i = 0; i < sizeof(usermsgs)/sizeof(usermsgs[0]); i++)
	{
		if (!strcmp(usermsgs[i].name, msgname))
		{
			usermsgs[i].hook = func;
			break;	//one per name
		}
	}

	//we already asked for it perhaps?
	for (i = 0; i < numnewhooks; i++)
	{
		if (!strcmp(pendingusermsgs[i].name, msgname))
		{
			pendingusermsgs[i].hook = func;
			return;	//nothing to do
		}
	}

	Q_strncpyz(pendingusermsgs[numnewhooks].name, msgname, sizeof(pendingusermsgs[i].name));
	pendingusermsgs[numnewhooks].hook = func;
	numnewhooks++;
}
void QDECL CLGHL_forwardcmd (char *command)
{
	bi_trace();
	CL_SendClientCommand(true, "%s", command);
}
void QDECL CLGHL_localcmd (char *command)
{
	bi_trace();
	Cbuf_AddText(command, RESTRICT_SERVER);
}

void QDECL CLGHL_getplayerinfo (int entnum, hlplayerinfo_t *result)
{
	player_info_t *player;
	bi_trace();
	entnum--;
	if (entnum < 0 || entnum >= MAX_CLIENTS)
		return;

	player = &cl.players[entnum];
	result->name = player->name;
	result->ping = player->ping;
	result->tcolour = player->rtopcolor;
	result->bcolour = player->rbottomcolor;
	result->isus = true;
	result->isspec = player->spectator;
	result->pl = player->pl;
	if (player->qwskin)
		result->model = player->qwskin->name;
	else
		result->model = "";
}

void QDECL CLGHL_startsound_name (char *name, float vol)
{
	sfx_t *sfx = S_PrecacheSound (name);
	bi_trace();
	if (!sfx)
	{
		Con_Printf ("CLGHL_startsound_name: can't cache %s\n", name);
		return;
	}
	S_StartSound (-1, -1, sfx, vec3_origin, vol, 1, 0, 0, 0);
}
void QDECL CLGHL_startsound_idx (int idx, float vol)
{
	sfx_t *sfx = cl.sound_precache[idx];
	bi_trace();
	if (!sfx)
	{
		Con_Printf ("CLGHL_startsound_name: index not precached %s\n", name);
		return;
	}
	S_StartSound (-1, -1, sfx, vec3_origin, vol, 1, 0, 0, 0);
}

void QDECL CLGHL_anglevectors (float *ina, float *outf, float *outr, float *outu)
{
	bi_trace();
	AngleVectors(ina, outf, outr, outu);
}

hlmsginfo_t *QDECL CLGHL_get_message_info (char *name)
{
	//fixme: add parser for titles.txt
	hlmsginfo_t *ret;
	bi_trace();
	ret = Z_Malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	ret->name = name;
	ret->message = name;
	ret->x = -1;
	ret->y = -1;
	*(int*)&ret->c1 = 0xffffffff;
	*(int*)&ret->c2 = 0xffffffff;
	ret->effect = 0;
	ret->fadein = 1;
	ret->fadeout = 1.5;
	ret->fxtime = 0;
	ret->holdtime = 3;
	return ret;
}
int QDECL CLGHL_drawchar (int x, int y, int charnum, int r, int g, int b)
{
	bi_trace();
	return 0;
}
int QDECL CLGHL_drawstring (int x, int y, char *string)
{
	bi_trace();
	return 0;
}
void QDECL CLGHL_settextcolour(float r, float g, float b)
{
	bi_trace();
}
void QDECL CLGHL_drawstring_getlen (char *string, int *outlen, int *outheight)
{
	bi_trace();
	*outlen = strlen(string)*8;
	*outheight = 8;
}
void QDECL CLGHL_consoleprint (char *str)
{
	bi_trace();
	Con_Printf("%s", str);
}
void QDECL CLGHL_centerprint (char *str)
{
	bi_trace();
	SCR_CenterPrint(0, str, true);
}


int QDECL CLGHL_getwindowcenterx(void)
{
	bi_trace();
	return window_center_x;
}
int QDECL CLGHL_getwindowcentery(void)
{
	bi_trace();
	return window_center_y;
}
void QDECL CLGHL_getviewnangles(float*ang)
{
	bi_trace();
	VectorCopy(cl.playerview[0].viewangles, ang);
}
void QDECL CLGHL_setviewnangles(float*ang)
{
	bi_trace();
	VectorCopy(ang, cl.playerview[0].viewangles);
}
void QDECL CLGHL_getmaxclients(float*ang){notimpf(__func__);}
void QDECL CLGHL_cvar_setvalue(char *cvarname, char *value){notimpf(__func__);}

int QDECL CLGHL_cmd_argc(void)
{
	bi_trace();
	return Cmd_Argc();
}
char *QDECL CLGHL_cmd_argv(int i)
{
	bi_trace();
	return Cmd_Argv(i);
}
#define CLGHL_con_printf Con_Printf
#define CLGHL_con_dprintf Con_DPrintf
void QDECL CLGHL_con_notificationprintf(int pos, char *fmt, ...){notimpf(__func__);}
void QDECL CLGHL_con_notificationprintfex(void *info, char *fmt, ...){notimpf(__func__);}
char *QDECL CLGHL_physkey(char *key){notimpf(__func__);return NULL;}
char *QDECL CLGHL_serverkey(char *key){notimpf(__func__);return NULL;}
float QDECL CLGHL_getclientmaxspeed(void)
{
	bi_trace();
	return 320;
}
int QDECL CLGHL_checkparm(char *str, const char **next)
{
	int i;
	bi_trace();
	i = COM_CheckParm(str);
	if (next)
	{
		if (i && i+1<com_argc)
			*next = com_argv[i+1];
		else
			*next = NULL;
	}
	return i;
}
int QDECL CLGHL_keyevent(int key, int down)
{
	bi_trace();
	if (key >= 241 && key <= 241+5)
		Key_Event(0, K_MOUSE1+key-241, 0, down);
	else
		Con_Printf("CLGHL_keyevent: Unrecognised HL key code\n");
	return true;	//fixme: check the return type
}
void QDECL CLGHL_getmousepos(int *outx, int *outy){notimpf(__func__);}
int QDECL CLGHL_movetypeisnoclip(void)
{
	bi_trace();
	if (cl.playerview[0].pmovetype == PM_SPECTATOR)
		return true;
	return false;
}
struct hlclent_s *QDECL CLGHL_getlocalplayer(void){notimpf(__func__);return NULL;}
struct hlclent_s *QDECL CLGHL_getviewent(void){notimpf(__func__);return NULL;}
struct hlclent_s *QDECL CLGHL_getentidx(int idx)
{
	bi_trace();
	notimpf(__func__);return NULL;
}
float QDECL CLGHL_getlocaltime(void){return cl.time;}
void QDECL CLGHL_calcshake(void){notimpf(__func__);}
void QDECL CLGHL_applyshake(float *origin, float *angles, float factor){notimpf(__func__);}
int QDECL CLGHL_pointcontents(float *point, float *truecon){notimpf(__func__);return 0;}
int QDECL CLGHL_entcontents(float *point){notimpf(__func__);return 0;}
void QDECL CLGHL_traceline(float *start, float *end, int flags, int hull, int forprediction){notimpf(__func__);}

model_t *QDECL CLGHL_loadmodel(char *modelname, int *mdlindex){notimpf(__func__);return Mod_ForName(modelname, false);}
int QDECL CLGHL_addrentity(int type, void *ent){notimpf(__func__);return 0;}

model_t *QDECL CLGHL_modelfrompic(HLPIC pic){notimpf(__func__);return NULL;}
void QDECL CLGHL_soundatloc(char*sound, float volume, float *org){notimpf(__func__);}

unsigned short QDECL CLGHL_precacheevent(int evtype, char *name){notimpf(__func__);return 0;}
void QDECL CLGHL_playevent(int flags, struct hledict_s *ent, unsigned short evindex, float delay, float *origin, float *angles, float f1, float f2, int i1, int i2, int b1, int b2){notimpf(__func__);}
void QDECL CLGHL_weaponanimate(int newsequence, int body)
{
	bi_trace();
	hl_viewmodelsequencetime = cl.time;
	hl_viewmodelsequencecur = newsequence;
	hl_viewmodelsequencebody = body;
}
float QDECL CLGHL_randfloat(float minv, float maxv){notimpf(__func__);return minv;}
long QDECL CLGHL_randlong(long minv, long maxv){notimpf(__func__);return minv;}
void QDECL CLGHL_hookevent(char *name, void (*func)(struct hlevent_s *event))
{
	bi_trace();
	Con_Printf("CLGHL_hookevent: not implemented. %s\n", name);
//	notimpf(__func__);
}
int QDECL CLGHL_con_isshown(void)
{
	bi_trace();
	return scr_con_current > 0;
}
char *QDECL CLGHL_getgamedir(void)
{
	extern char	gamedirfile[];
	bi_trace();
	return gamedirfile;
}
struct hlcvar_s *QDECL CLGHL_cvar_find(char *name)
{
	bi_trace();
	return GHL_CVarGetPointer(name);
}
char *QDECL CLGHL_lookupbinding(char *command)
{
	bi_trace();
	return NULL;
}
char *QDECL CLGHL_getlevelname(void)
{
	bi_trace();
	if (!cl.worldmodel)
		return "";
	return cl.worldmodel->name;
}
void QDECL CLGHL_getscreenfade(struct hlsfade_s *fade){notimpf(__func__);}
void QDECL CLGHL_setscreenfade(struct hlsfade_s *fade){notimpf(__func__);}
void *QDECL CLGHL_vgui_getpanel(void)
{
	bi_trace();
	return vgui_panel;
}
void QDECL CLGHL_vgui_paintback(int extents[4])
{
	bi_trace();
	notimpf(__func__);
}

void *QDECL CLGHL_loadfile(char *path, int alloctype, int *length)
{
	void *ptr = NULL;
	int flen = -1;
	bi_trace();
	if (alloctype == 5)
	{
		flen = FS_LoadFile(path, &ptr);
	}
	else
		notimpf(__func__);	//don't leak, just fail

	if (length)
		*length = flen;

	return ptr;
}
char *QDECL CLGHL_parsefile(char *data, char *token)
{
	bi_trace();
	return COM_ParseOut(data, token, 1024);
}
void QDECL CLGHL_freefile(void *file)
{
	bi_trace();
	//only valid for alloc type 5
	FS_FreeFile(file);
}


int QDECL CLGHL_forcedspectator(void)
{
	bi_trace();
	return cls.demoplayback;
}
model_t *QDECL CLGHL_loadmapsprite(char *name)
{
	bi_trace();
	notimpf(__func__);return NULL;
}

void QDECL CLGHL_fs_addgamedir(char *basedir, char *appname){notimpf(__func__);}
int QDECL CLGHL_expandfilename(char *filename, char *outbuff, int outsize){notimpf(__func__);return false;}

char *QDECL CLGHL_player_key(int pnum, char *key){notimpf(__func__);return NULL;}
void QDECL CLGHL_player_setkey(char *key, char *value){notimpf(__func__);return;}

qboolean QDECL CLGHL_getcdkey(int playernum, char key[16]){notimpf(__func__);return false;}
int QDECL CLGHL_trackerfromplayer(int pslot){notimpf(__func__);return 0;}
int QDECL CLGHL_playerfromtracker(int tracker){notimpf(__func__);return 0;}
int QDECL CLGHL_sendcmd_unreliable(char *cmd){notimpf(__func__);return 0;}
void QDECL CLGHL_getsysmousepos(long *xandy)
{
	bi_trace();
#ifdef _WIN32
	GetCursorPos((LPPOINT)xandy);
#endif
}
void QDECL CLGHL_setsysmousepos(int x, int y)
{
	bi_trace();
#ifdef _WIN32
	SetCursorPos(x, y);
#endif
}
void QDECL CLGHL_setmouseenable(qboolean enable)
{
	bi_trace();
	Cvar_Set(&in_windowed_mouse, enable?"1":"0");
}



#if HLCL_API_VERSION >= 7
int QDECL CLGHL_demo_isrecording(void)
{
	bi_trace();
	return cls.demorecording;
}
int QDECL CLGHL_demo_isplaying(void)
{
	bi_trace();
	return cls.demoplayback;
}
int QDECL CLGHL_demo_istimedemo(void)
{
	bi_trace();
	return cls.timedemo;
}
void QDECL CLGHL_demo_writedata(int size, void *data)
{
	bi_trace();
	notimpf(__func__);
}

struct hl_demo_api_s hl_demo_api = 
{
	CLGHL_demo_isrecording,
	CLGHL_demo_isplaying,
	CLGHL_demo_istimedemo,
	CLGHL_demo_writedata,

	0xdeadbeef
};
#endif

CLHL_cgamefuncs_t CLHL_cgamefuncs;
CLHL_enginecgamefuncs_t CLHL_enginecgamefuncs =
{
	CLGHL_pic_load,
	CLGHL_pic_getnumframes,
	CLGHL_pic_getheight,
	CLGHL_pic_getwidth,
	CLGHL_pic_select,
	CLGHL_pic_drawcuropaque,
	CLGHL_pic_drawcuralphtest,
	CLGHL_pic_drawcuradditive,
	CLGHL_pic_enablescissor,
	CLGHL_pic_disablescissor,
	CLGHL_pic_parsepiclist,

	CLGHL_fillrgba,
	CLGHL_getscreeninfo,
	CLGHL_setcrosshair,

	CLGHL_cvar_register,
	CLGHL_cvar_getfloat,
	CLGHL_cvar_getstring,

	CLGHL_cmd_register,
	CLGHL_hooknetmsg,
	CLGHL_forwardcmd,
	CLGHL_localcmd,

	CLGHL_getplayerinfo,

	CLGHL_startsound_name,
	CLGHL_startsound_idx,

	CLGHL_anglevectors,

	CLGHL_get_message_info,
	CLGHL_drawchar,
	CLGHL_drawstring,
#if HLCL_API_VERSION >= 7
	CLGHL_settextcolour,
#endif
	CLGHL_drawstring_getlen,
	CLGHL_consoleprint,
	CLGHL_centerprint,

#if HLCL_API_VERSION >= 7
	CLGHL_getwindowcenterx,
	CLGHL_getwindowcentery,
	CLGHL_getviewnangles,
	CLGHL_setviewnangles,
	CLGHL_getmaxclients,
	CLGHL_cvar_setvalue,

	CLGHL_cmd_argc,
	CLGHL_cmd_argv,
	CLGHL_con_printf,
	CLGHL_con_dprintf,
	CLGHL_con_notificationprintf,
	CLGHL_con_notificationprintfex,
	CLGHL_physkey,
	CLGHL_serverkey,
	CLGHL_getclientmaxspeed,
	CLGHL_checkparm,
	CLGHL_keyevent,
	CLGHL_getmousepos,
	CLGHL_movetypeisnoclip,
	CLGHL_getlocalplayer,
	CLGHL_getviewent,
	CLGHL_getentidx,
	CLGHL_getlocaltime,
	CLGHL_calcshake,
	CLGHL_applyshake,
	CLGHL_pointcontents,
	CLGHL_entcontents,
	CLGHL_traceline,

	CLGHL_loadmodel,
	CLGHL_addrentity,

	CLGHL_modelfrompic,
	CLGHL_soundatloc,

	CLGHL_precacheevent,
	CLGHL_playevent,
	CLGHL_weaponanimate,
	CLGHL_randfloat,
	CLGHL_randlong,
	CLGHL_hookevent,
	CLGHL_con_isshown,
	CLGHL_getgamedir,
	CLGHL_cvar_find,
	CLGHL_lookupbinding,
	CLGHL_getlevelname,
	CLGHL_getscreenfade,
	CLGHL_setscreenfade,
	CLGHL_vgui_getpanel,
	CLGHL_vgui_paintback,

	CLGHL_loadfile,
	CLGHL_parsefile,
	CLGHL_freefile,

	NULL, //triapi;
	NULL, //efxapi;
	NULL, //eventapi;
	&hl_demo_api,
	NULL, //netapi;

//sdk 2.3+
	NULL, //voiceapi;

	CLGHL_forcedspectator,
	CLGHL_loadmapsprite,

	CLGHL_fs_addgamedir,
	CLGHL_expandfilename,

	CLGHL_player_key,
	CLGHL_player_setkey,

	CLGHL_getcdkey,
	CLGHL_trackerfromplayer,
	CLGHL_playerfromtracker,
	CLGHL_sendcmd_unreliable,
	CLGHL_getsysmousepos,
	CLGHL_setsysmousepos,
	CLGHL_setmouseenable,
#endif

	0xdeadbeef
};

dllhandle_t *clg;

int CLHL_GamecodeDoesMouse(void)
{
#if HLCL_API_VERSION >= 7
	if (clg && CLHL_cgamefuncs.CL_CreateMove)
		return true;
#endif
	return false;
}

int CLHL_MouseEvent(unsigned int buttonmask)
{
#if HLCL_API_VERSION >= 7
	if (!CLHL_GamecodeDoesMouse())
		return false;

	CLHL_cgamefuncs.IN_MouseEvent(buttonmask);
	return true;
#else
	return false;
#endif
}

void CLHL_SetMouseActive(int activate)
{
#if HLCL_API_VERSION >= 7
	static int oldactive;
	if (!clg)
	{
		oldactive = false;
		return;
	}
	if (activate == oldactive)
		return;
	oldactive = activate;

	if (activate)
	{
		if (CLHL_cgamefuncs.IN_ActivateMouse)
			CLHL_cgamefuncs.IN_ActivateMouse();
	}
	else
	{
		if (CLHL_cgamefuncs.IN_DeactivateMouse)
			CLHL_cgamefuncs.IN_DeactivateMouse();
	}
#endif
}

void CLHL_UnloadClientGame(void)
{
	if (!clg)
		return;

	CLHL_SetMouseActive(false);
	if (CLHL_cgamefuncs.HUD_Shutdown)
		CLHL_cgamefuncs.HUD_Shutdown();
	Sys_CloseLibrary(clg);
	memset(&CLHL_cgamefuncs, 0, sizeof(CLHL_cgamefuncs));
	clg = NULL;

	hl_viewmodelsequencetime = 0;
}

void CLHL_LoadClientGame(void)
{
	char fullname[MAX_OSPATH];
	char path[MAX_OSPATH];
	void *iterator;

	int (QDECL *initfunc)(CLHL_enginecgamefuncs_t *funcs, int version);
	dllfunction_t funcs[] =
	{
		{(void*)&initfunc, "Initialize"},
		{NULL}
	};

	CLHL_UnloadClientGame();

	memset(&CLHL_cgamefuncs, 0, sizeof(CLHL_cgamefuncs));

	clg = NULL;
	iterator = NULL;
	//FIXME: dlls in gamepaths is evil 
	while(COM_IteratePaths(&iterator, path, sizeof(path), NULL, 0))
	{
		snprintf (fullname, sizeof(fullname), "%s%s", path, "cl_dlls/client");
		clg = Sys_LoadLibrary(fullname, funcs);
		if (clg)
			break;
	}

	if (!clg)
		return;

	if (!initfunc(&CLHL_enginecgamefuncs, HLCL_API_VERSION))
	{
		Con_Printf("HalfLife cldll is not version %i\n", HLCL_API_VERSION);
		Sys_CloseLibrary(clg);
		clg = NULL;
		return;
	}

	CLHL_cgamefuncs.HUD_VidInit = (void*)Sys_GetAddressForName(clg, "HUD_VidInit");
	CLHL_cgamefuncs.HUD_Init = (void*)Sys_GetAddressForName(clg, "HUD_Init");
	CLHL_cgamefuncs.HUD_Shutdown = (void*)Sys_GetAddressForName(clg, "HUD_Shutdown");
	CLHL_cgamefuncs.HUD_Redraw = (void*)Sys_GetAddressForName(clg, "HUD_Redraw");
	CLHL_cgamefuncs.HUD_UpdateClientData = (void*)Sys_GetAddressForName(clg, "HUD_UpdateClientData");
	CLHL_cgamefuncs.HUD_Reset = (void*)Sys_GetAddressForName(clg, "HUD_Reset");
#if HLCL_API_VERSION >= 7
	CLHL_cgamefuncs.CL_CreateMove = (void*)Sys_GetAddressForName(clg, "CL_CreateMove");
	CLHL_cgamefuncs.IN_ActivateMouse = (void*)Sys_GetAddressForName(clg, "IN_ActivateMouse");
	CLHL_cgamefuncs.IN_DeactivateMouse = (void*)Sys_GetAddressForName(clg, "IN_DeactivateMouse");
	CLHL_cgamefuncs.IN_MouseEvent = (void*)Sys_GetAddressForName(clg, "IN_MouseEvent");
#endif

	VGui_Setup();

	if (CLHL_cgamefuncs.HUD_Init)
		CLHL_cgamefuncs.HUD_Init();
	if (CLHL_cgamefuncs.HUD_VidInit)
		CLHL_cgamefuncs.HUD_VidInit();
	{
		struct hlkbutton_s
		{
			int		down[2];		// key nums holding it down
			int		state;			// low bit is down state
		} *but;
		struct hlkbutton_s *(QDECL *pKB_Find) (const char *foo) = (void*)Sys_GetAddressForName(clg, "KB_Find");
		if (pKB_Find)
		{
			but = pKB_Find("in_mlook");
			if (but)
				but->state |= 1;
		}
	}
}

int CLHL_BuildUserInput(int msecs, usercmd_t *cmd)
{
#if HLCL_API_VERSION >= 7
	hlusercmd_t hlcmd;
	if (!clg || !CLHL_cgamefuncs.CL_CreateMove)
		return false;

	CLHL_cgamefuncs.CL_CreateMove(msecs/255.0f, &hlcmd, cls.state>=ca_active && !cls.demoplayback);

#define ANGLE2SHORT(x) (x) * (65536/360.0)
	cmd->msec = msecs;
	cmd->angles[0] = ANGLE2SHORT(hlcmd.viewangles[0]);
	cmd->angles[1] = ANGLE2SHORT(hlcmd.viewangles[1]);
	cmd->angles[2] = ANGLE2SHORT(hlcmd.viewangles[2]);
	cmd->forwardmove = hlcmd.forwardmove;
	cmd->sidemove = hlcmd.sidemove;
	cmd->upmove = hlcmd.upmove;
	cmd->weapon = hlcmd.weaponselect;
	cmd->impulse = hlcmd.impulse;
	cmd->buttons = hlcmd.buttons & 0xff;	//FIXME: quake's protocols are more limited than this
	cmd->lightlevel = hlcmd.lightlevel;
	return true;
#else
	return false;
#endif
}

int CLHL_DrawHud(void)
{
	extern kbutton_t in_attack;
	hllocalclientdata_t state;
	int ret;

	if (!clg || !CLHL_cgamefuncs.HUD_Redraw)
		return false;

	memset(&state, 0, sizeof(state));

//	state.origin;
//	state.viewangles;
#if HLCL_API_VERSION < 7
//	state.viewheight;
//	state.maxspeed;
//	state.punchangles;
	state.idlescale = 0;
	state.mousesens = 0;
	state.keys = (in_attack.state[0]&3)?1:0;
#endif
	state.weapons = cl.playerview[0].stats[STAT_ITEMS];
	state.fov = 90;

	V_StopPitchDrift(&cl.playerview[0]);

	CLHL_cgamefuncs.HUD_UpdateClientData(&state, cl.time);

	ret = CLHL_cgamefuncs.HUD_Redraw(cl.time, cl.intermissionmode != IM_NONE);	
	return ret;
}

int CLHL_AnimateViewEntity(entity_t *ent)
{
	float time;
	if (!hl_viewmodelsequencetime)
		return false;

	time = cl.time - hl_viewmodelsequencetime;
	ent->framestate.g[FS_REG].frame[0] = hl_viewmodelsequencecur;
	ent->framestate.g[FS_REG].frame[1] = hl_viewmodelsequencecur;
	ent->framestate.g[FS_REG].frametime[0] = time;
	ent->framestate.g[FS_REG].frametime[1] = time;
	return true;
}

explosion_t *CL_AllocExplosion (vec3_t org);

int CLHL_ParseGamePacket(void)
{
	int subcode;
	char *end;
	char *str;
	int tempi;

	end = net_message.data+msg_readcount+2+MSG_ReadShort();

	if (end > net_message.data+net_message.cursize)
		return false;

	subcode = MSG_ReadByte();

	if (usermsgs[subcode].hook)
		if (usermsgs[subcode].hook(usermsgs[subcode].name, end - (net_message.data+msg_readcount), net_message.data+msg_readcount))
		{
			msg_readcount = end - net_message.data;
			return true;
		}

	switch(subcode)
	{
	case 1:
		//register the server-sent code.
		tempi = MSG_ReadByte();
		str = MSG_ReadString();
		Q_strncpyz(usermsgs[tempi].name, str, sizeof(usermsgs[tempi].name));

		//get the builtin to reregister its hooks.
		for (tempi = 0; tempi < numnewhooks; tempi++)
			CLGHL_hooknetmsg(pendingusermsgs[tempi].name, pendingusermsgs[tempi].hook);
		break;
	case svc_temp_entity:
		{
			vec3_t startp, endp, vel;
			float ang;
			int midx;
			int mscale;
			int mrate;
			int flags;
			int lifetime;
			explosion_t *ef;

			subcode = MSG_ReadByte();

			switch(subcode)
			{
			case 3:
				MSG_ReadPos(startp);
				midx = MSG_ReadShort();
				mscale = MSG_ReadByte();
				mrate = MSG_ReadByte();
				flags = MSG_ReadByte();
				if (!(flags & 8))
					P_RunParticleEffectType(startp, NULL, 1, pt_explosion);
				if (!(flags & 4))
					S_StartSound(0, 0, S_PrecacheSound("explosion"), startp, 1, 1, 0, 0, 0);
				if (!(flags & 2))
					CL_NewDlight(0, startp, 200, 1, 2.0,2.0,2.0);

				ef = CL_AllocExplosion(startp);
				ef->start = cl.time;
				ef->model = cl.model_precache[midx];
				ef->framerate = mrate;
				ef->firstframe = 0;
				ef->numframes = ef->model->numframes;
				if (!(flags & 1))
					ef->flags = RF_ADDITIVE;
				else
					ef->flags = 0;
				break;
			case 4:
				MSG_ReadPos(startp);
				P_RunParticleEffectType(startp, NULL, 1, pt_tarexplosion);
				break;
			case 5:
				MSG_ReadPos(startp);
				MSG_ReadShort();
				MSG_ReadByte();
				MSG_ReadByte();
				break;
			case 6:
				MSG_ReadPos(startp);
				MSG_ReadPos(endp);
				break;
			case 9:
				MSG_ReadPos(startp);
				P_RunParticleEffectType(startp, NULL, 1, ptdp_spark);
				break;
			case 22:
				MSG_ReadShort();
				MSG_ReadShort();
				MSG_ReadByte();
				MSG_ReadByte();
				MSG_ReadByte();
				MSG_ReadByte();
				MSG_ReadByte();
				break;
			case 23:
				MSG_ReadPos(startp);
				MSG_ReadShort();
				MSG_ReadByte();
				MSG_ReadByte();
				MSG_ReadByte();
				break;
			case 106:
				MSG_ReadPos(startp);
				MSG_ReadPos(vel);
				ang = MSG_ReadAngle();
				midx = MSG_ReadShort();
				MSG_ReadByte();
				lifetime = MSG_ReadByte();

				ef = CL_AllocExplosion(startp);
				ef->start = cl.time;
				ef->angles[1] = ang;
				ef->model = cl.model_precache[midx];
				ef->firstframe = 0;
				ef->numframes = lifetime;
				ef->flags = 0;
				ef->framerate = 10;
				break;
			case 108:
				MSG_ReadPos(startp);
				MSG_ReadPos(endp);
				MSG_ReadPos(vel);
				MSG_ReadByte();
				MSG_ReadShort();
				MSG_ReadByte();
				MSG_ReadByte();
				MSG_ReadByte();
				break;
			case 109:
				MSG_ReadPos(startp);
				MSG_ReadShort();
				MSG_ReadByte();
				break;
			case 116:
				MSG_ReadPos(startp);
				MSG_ReadByte();
				break;
			case 117:
				MSG_ReadPos(startp);
				MSG_ReadByte()+256;
				break;
			case 118:
				MSG_ReadPos(startp);
				MSG_ReadByte()+256;
				break;
			default:
				Con_Printf("CLHL_ParseGamePacket: Unable to parse gamecode tempent %i\n", subcode);
				msg_readcount = end - net_message.data;
				break;
			}
		}
		if (msg_readcount != end - net_message.data)
		{
			Con_Printf("CLHL_ParseGamePacket: Gamecode temp entity %i not parsed correctly read %i bytes too many\n", subcode, msg_readcount - (end - net_message.data));
			msg_readcount = end - net_message.data;
		}
		break;
	case svc_intermission:
		//nothing.
		cl.intermissionmode = IM_NQSCORES;
		break;
	case svc_cdtrack:
		{
			unsigned int firsttrack;
			unsigned int looptrack;
			firsttrack = MSG_ReadByte ();
			looptrack = MSG_ReadByte ();
			Media_NumberedTrack (firsttrack, looptrack);
		}
		break;

	case 35: //svc_weaponanimation:
		tempi = MSG_ReadByte();
		CLGHL_weaponanimate(tempi, MSG_ReadByte());
		break;
	case 37: //svc_roomtype
		tempi = MSG_ReadShort();
//		S_SetUnderWater(tempi==14||tempi==15||tempi==16);
		break;
	default:
		Con_Printf("Unrecognised gamecode packet %i (%s)\n", subcode, usermsgs[subcode].name);
		msg_readcount = end - net_message.data;
		break;
	}

	if (msg_readcount != end - net_message.data)
	{
		Con_Printf("CLHL_ParseGamePacket: Gamecode packet %i not parsed correctly read %i bytestoo many\n", subcode, msg_readcount - (end - net_message.data));
		msg_readcount = end - net_message.data;
	}
	return true;
}

#endif
