//ezquake likes this
#ifndef FTEPLUGIN
#include "quakedef.h"
#define FTEENGINE	//we're getting statically linked. lucky us.
#define FTEPLUGIN
#endif
#include "../plugin.h"
#include <assert.h>
#include <ctype.h>

#ifdef FTEENGINE
#define drawfuncs ezhud_drawfuncs
#define filefuncs ezhud_filefuncs
#define clientfuncs ezhud_clientfuncs
#define inputfuncs ezhud_inputfuncs
#endif

extern plug2dfuncs_t *drawfuncs;
extern plugfsfuncs_t *filefuncs;
extern plugclientfuncs_t *clientfuncs;
extern pluginputfuncs_t *inputfuncs;

//ezquake sucks. I'd fix these, but that'd make diffs more messy.
#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wold-style-definition"
	#pragma GCC diagnostic ignored "-Wstrict-prototypes"
	#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

//ezquake types.
#define byte qbyte
#define qbool qboolean
#define Com_Printf Con_Printf
#define Com_DPrintf Con_DPrintf
#define Cvar_Find(n) cvarfuncs->GetNVFDG(n,NULL,0,NULL,NULL)
#define Cvar_SetValue(var,val) cvarfuncs->SetFloat(var->name,val)
#define Cvar_Set(var,val) cvarfuncs->SetString(var->name,val)
#define Cmd_AddCommand(nam,ptr) cmdfuncs->AddCommand(nam,ptr,NULL)
#define Cmd_Argc cmdfuncs->Argc
#define Cbuf_AddText(x) cmdfuncs->AddText(x,false)
#define Sys_Error(x) plugfuncs->Error(x)
#define Q_calloc calloc
#define Q_malloc malloc
#define Q_strdup strdup
#define Q_free free
#define Q_rint(x) ((int)(x+0.5))
#define Q_atoi atoi
#ifdef FTEENGINE
	#define strlcpy Q_strncpyz
	#define strlcat Q_strncatz
#else
	#define strlcpy Q_strlcpy
	#define strlcat Q_strlcat
#endif

//ezhud has a number of common symbol conflicts, which matter when sttatically linking into the engine
#define Cmd_Argv			ezCmd_Argv
#define TP_LocationName		ezTP_LocationName
#define TP_ParseFunChars	ezTP_ParseFunChars
#define Sbar_ColorForMap	ezSbar_ColorForMap
#define scr_vrect			ezscr_vrect
#define sb_lines			ezsb_lines
#define keydown				ezkeydown
#define scr_con_current		ezscr_con_current


#undef mpic_t
#define mpic_t void


#define MV_VIEWS 4


extern float cursor_x;
extern float cursor_y;
extern int host_screenupdatecount;
extern cvar_t *scr_newHud;
extern cvar_t *cl_multiview;

#define Cam_TrackNum() cl.tracknum
#define spec_track cl.tracknum
#define autocam ((spec_track==-1)?CAM_NONE:CAM_TRACK)
#define CAM_TRACK true
#define CAM_NONE false
//#define HAXX

#define vid plugvid
#define cls plugcls
#define cl plugcl
#define player_info_t plugclientinfo_t

extern struct ezcl_s{
	int intermission;
	int teamplay;
	int deathmatch;
	int stats[MAX_CL_STATS];
	int item_gettime[32];
	char serverinfo[4096];
	player_info_t players[MAX_CLIENTS];
	int playernum;
	int tracknum;
	vec3_t simvel;
	float time;
	float matchstart;
	float faceanimtime;
	qboolean spectator;
	qboolean standby;
	qboolean countdown;

	int splitscreenview;
} cl;
extern struct ezcls_s{
	int state;
	float min_fps;
	float fps;
	float realtime;
	float frametime;
	qbool mvdplayback;
	int demoplayback;
} cls;
extern struct ezvid_s{
	int width;
	int height;
//	float displayFrequency;
} vid;


//reimplementations of ezquake functions
void Draw_SetOverallAlpha(float a);
void Draw_AlphaFillRGB(float x, float y, float w, float h, qbyte r, qbyte g, qbyte b, qbyte a);
void Draw_Fill(float x, float y, float w, float h, qbyte pal);
const char *ColorNameToRGBString (const char *newval);
byte *StringToRGB(const char *str);

#define Draw_String					drawfuncs->String

void Draw_EZString(float x, float y, char *str, float scale, qboolean red);
#define Draw_Alt_String(x,y,s)			Draw_EZString(x,y,s,8,true)
#define Draw_ColoredString(x,y,str,alt)	Draw_EZString(x,y,str,8,alt)
#define Draw_SString(x,y,str,sc)		Draw_EZString(x,y,str,8*sc,false)
#define Draw_SAlt_String(x,y,str,sc)	Draw_EZString(x,y,str,8*sc,true)

void Draw_SPic(float x, float y, mpic_t *pic, float scale);
void Draw_SSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float scale);
#define Draw_STransPic Draw_SPic
void Draw_Character(float x, float y, unsigned int ch);
void Draw_SCharacter(float x, float y, unsigned int ch, float scale);

void SCR_DrawWadString(float x, float y, float scale, char *str);

void Draw_SAlphaSubPic2(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float w, float h, float alpha);

void Draw_AlphaFill(float x, float y, float w, float h, unsigned int pal, float alpha);
void Draw_AlphaPic(float x, float y, mpic_t *pic, float alpha);
void Draw_AlphaSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float alpha);
void SCR_HUD_DrawBar(int direction, int value, float max_value, float *rgba, int x, int y, int width, int height);

mpic_t *Draw_CachePicSafe(const char *name, qbool crash, qbool ignorewad);
mpic_t *Draw_CacheWadPic(const char *name);

int Sbar_TopColor(player_info_t *pi);
int Sbar_BottomColor(player_info_t *pi);
char *TP_ParseFunChars(char*);
char *TP_ItemName(unsigned int itbit);
char*		TP_LocationName (const vec3_t location);

char *Cmd_Argv(int arg);
extern float           scr_con_current;	//current console lines shown

#define Util_SkipChars(src,strip,dst,dstlen) strlcpy(dst,src,dstlen)
#define Util_SkipEZColors(src,dst,dstlen) strlcpy(dst,src,dstlen)

void Replace_In_String(char *string, size_t strsize, char leadchar, int patterns, ...);
//static qbool Utils_RegExpMatch(char *regexp, char *term) {return true;}
#define Utils_RegExpMatch(regexp,term) (true)

#define clamp(v,min,max) v=bound(min,v,max)
#define strlen_color(line) (drawfuncs->StringWidth(8, 0, line)/8.0)

#define TIMETYPE_CLOCK 0
#define TIMETYPE_GAMECLOCK 1
#define TIMETYPE_GAMECLOCKINV 2
#define TIMETYPE_DEMOCLOCK 3
int SCR_GetClockStringWidth(const char *s, qbool big, float scale);
int SCR_GetClockStringHeight(qbool big, float scale);
const char* SCR_GetTimeString(int timetype, const char *format);
void SCR_DrawBigClock(int x, int y, int style, int blink, float scale, const char *t);
void SCR_DrawSmallClock(int x, int y, int style, int blink, float scale, const char *t);

typedef struct
{
	qbyte c[4];
} clrinfo_t;
void Draw_ColoredString3(float x, float y, const char *str, clrinfo_t *clr, int huh, int wut);
void UI_PrintTextBlock(float x, float y, float w, float h, char *str, int flags);
void Draw_AlphaRectangleRGB(int x, int y, int w, int h, int foo, int bar, byte r, byte g, byte b, byte a);
void Draw_AlphaLineRGB(float x1, float y1, float x2, float y2, float width, byte r, byte g, byte b, byte a);
void Draw_Polygon(int x, int y, vec3_t *vertices, int num_vertices, qbool fill, byte r, byte g, byte b, byte a);

extern int			sb_lines;			// scan lines to draw
#ifndef SBAR_HEIGHT
#define SBAR_HEIGHT 24
#define STAT_HEALTH			0
#define STAT_WEAPONMODELI	2
#define STAT_AMMO			3
#define STAT_ARMOR			4
#define STAT_WEAPONFRAME	5
#define STAT_SHELLS			6
#define STAT_NAILS			7
#define STAT_ROCKETS		8
#define STAT_CELLS			9
#define STAT_ACTIVEWEAPON	10
#define STAT_TOTALSECRETS	11
#define STAT_TOTALMONSTERS	12
#define STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define STAT_MONSTERS		14		// bumped by svc_killedmonster
#define STAT_ITEMS			15
#define STAT_VIEWHEIGHT		16	//same as zquake
#define STAT_TIME			17	//zquake
#define STAT_MATCHSTARTTIME 18

#define	IT_SHOTGUN				(1u<<0)
#define	IT_SUPER_SHOTGUN		(1u<<1)
#define	IT_NAILGUN				(1u<<2)
#define	IT_SUPER_NAILGUN		(1u<<3)

#define	IT_GRENADE_LAUNCHER		(1u<<4)
#define	IT_ROCKET_LAUNCHER		(1u<<5)
#define	IT_LIGHTNING			(1u<<6)
#define	IT_SUPER_LIGHTNING		(1u<<7)

#define	IT_SHELLS				(1u<<8)
#define	IT_NAILS				(1u<<9)
#define	IT_ROCKETS				(1u<<10)
#define	IT_CELLS				(1u<<11)

#define	IT_AXE					(1u<<12)

#define	IT_ARMOR1				(1u<<13)
#define	IT_ARMOR2				(1u<<14)
#define	IT_ARMOR3				(1u<<15)

#define	IT_SUPERHEALTH			(1u<<16)

#define	IT_KEY1					(1u<<17)
#define	IT_KEY2					(1u<<18)

#define	IT_INVISIBILITY			(1u<<19)

#define	IT_INVULNERABILITY		(1u<<20)
#define	IT_SUIT					(1u<<21)
#define	IT_QUAD					(1u<<22)

#define	IT_SIGIL1				(1u<<28)

#define	IT_SIGIL2				(1u<<29)
#define	IT_SIGIL3				(1u<<30)
#define	IT_SIGIL4				(1u<<31)
#endif