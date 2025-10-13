#include "ezquakeisms.h"
#include "hud.h"
#include "hud_editor.h"

#ifdef FTEENGINE
#define Plug_Init Plug_EZHud_Init
#endif

plug2dfuncs_t *drawfuncs;
plugclientfuncs_t *clientfuncs;
plugfsfuncs_t *filefuncs;
pluginputfuncs_t *inputfuncs;

struct ezcl_s cl;
struct ezcls_s cls;
struct ezvid_s vid;

int sb_lines;
float scr_con_current;
int sb_showteamscores;
int sb_showscores;
int host_screenupdatecount;
float alphamul;

cvar_t *scr_newHud;

void HUD_InitSbarImages(void);
static void QDECL EZHud_UpdateVideo(int width, int height, qboolean restarted)
{
	vid.width = width;
	vid.height = height;

	if (restarted)
		HUD_InitSbarImages();
}

char *Cmd_Argv(int arg)
{
	static char buf[4][128];
	if (arg >= 4)
		return "";
	cmdfuncs->Argv(arg, buf[arg], sizeof(buf[arg]));
	return buf[arg];
}

float infofloat(char *info, char *findkey, float def);

void Draw_SetOverallAlpha(float a)
{
	alphamul = a;
}
void Draw_AlphaFillRGB(float x, float y, float w, float h, qbyte r, qbyte g, qbyte b, qbyte a)
{
	drawfuncs->Colour4f(r/255.0, g/255.0, b/255.0, a/255.0 * alphamul);
	drawfuncs->Fill(x, y, w, h);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void Draw_Fill(float x, float y, float w, float h, qbyte pal)
{
	drawfuncs->Colourpa(pal, alphamul);
	drawfuncs->Fill(x, y, w, h);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
const char *ColorNameToRGBString (const char *newval)
{
	return newval;
}
byte *StringToRGB(const char *str)
{
	static byte rgba[4];
	int i;
	for (i = 0; i < 4; i++)
	{
		while(*str && *str <= ' ')
			str++;
		if (!*str)
			rgba[i] = 255;
		else
			rgba[i] = strtoul(str, (char**)&str, 0);
	}
	return rgba;
}
void Draw_TextBox (int x, int y, int width, int lines)
{
}

char *TP_LocationName (const vec3_t location)
{
	static char locname[256];
	clientfuncs->GetLocationName(location, locname, sizeof(locname));
	return locname;
}


void Draw_SPic(float x, float y, mpic_t *pic, float scale)
{
	qhandle_t image = (intptr_t)pic;
	float w=64, h=64;
	drawfuncs->ImageSize(image, &w, &h);
	drawfuncs->Image(x, y, w*scale, h*scale, 0, 0, 1, 1, image);
}
void Draw_SSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float scale)
{
	qhandle_t image = (intptr_t)pic;
	float w=64, h=64;
	drawfuncs->ImageSize(image, &w, &h);
	drawfuncs->Image(x, y, (s2-s1)*scale, (t2-t1)*scale, s1/w, t1/h, s2/w, t2/h, image);
}
void Draw_EZString(float x, float y, char *str, float scale, qboolean red)
{
	unsigned int flags = 0;
	if (red)
		flags |= 1;
	drawfuncs->StringH(x, y, scale, flags, str);
}

#define Draw_STransPic Draw_SPic
void Draw_Character(float x, float y, unsigned int ch)
{
	drawfuncs->Character(x, y, 0xe000|ch);
}
void Draw_SCharacter(float x, float y, unsigned int ch, float scale)
{
	drawfuncs->CharacterH(x, y, 8*scale, 0, 0xe000|ch);
}

void SCR_DrawWadString(float x, float y, float scale, char *str)
{
	drawfuncs->String(x, y, str);	//FIXME
}

void Draw_SAlphaSubPic2(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float sw, float sh, float alpha)
{
	qhandle_t image = (intptr_t)pic;
	float w=64, h=64;
	drawfuncs->ImageSize(image, &w, &h);
	drawfuncs->Colour4f(1, 1, 1, alpha * alphamul);
	drawfuncs->Image(x, y, (s2-s1)*sw, (t2-t1)*sh, s1/w, t1/h, s2/w, t2/h, image);
	drawfuncs->Colour4f(1, 1, 1, 1);
}

void Draw_AlphaFill(float x, float y, float w, float h, unsigned int pal, float alpha)
{
	if (pal >= 256)
		drawfuncs->Colour4f(((pal>>16)&0xff)/255.0, ((pal>>8)&0xff)/255.0, ((pal>>0)&0xff)/255.0, alpha * alphamul);
	else
		drawfuncs->Colourpa(pal, alpha * alphamul);
	drawfuncs->Fill(x, y, w, h);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void Draw_AlphaPic(float x, float y, mpic_t *pic, float alpha)
{
	qhandle_t image = (intptr_t)pic;
	float w, h;
	drawfuncs->ImageSize(image, &w, &h);
	drawfuncs->Colour4f(1, 1, 1, alpha * alphamul);
	drawfuncs->Image(x, y, w, h, 0, 0, 1, 1, image);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void Draw_AlphaSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float alpha)
{
	qhandle_t image = (intptr_t)pic;
	float w, h;
	drawfuncs->ImageSize(image, &w, &h);
	drawfuncs->Colour4f(1, 1, 1, alpha * alphamul);
	drawfuncs->Image(x, y, s2-s1, t2-t1, s1/w, t1/h, s2/w, t2/h, image);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void SCR_HUD_DrawBar(int direction, int value, float max_value, float *rgba, int x, int y, int width, int height)
{
	int amount;

	if(direction >= 2)
		// top-down
		amount = Q_rint(fabs((height * value) / max_value));
	else// left-right
		amount = Q_rint(fabs((width * value) / max_value));

	drawfuncs->Colour4f(rgba[0]/255.0, rgba[1]/255.0, rgba[2]/255.0, rgba[3]/255.0 * alphamul);
	if(direction == 0)
		// left->right
		drawfuncs->Fill(x, y, amount, height);
	else if (direction == 1)
		// right->left
		drawfuncs->Fill(x + width - amount, y, amount, height);
	else if (direction == 2)
		// down -> up
		drawfuncs->Fill(x, y + height - amount, width, amount);
	else
		// up -> down
		drawfuncs->Fill(x, y, width, amount);
	drawfuncs->Colour4f(1, 1, 1, 1);
}

void Draw_Polygon(int x, int y, vec3_t *vertices, int num_vertices, qbool fill, byte r, byte g, byte b, byte a)
{
	drawfuncs->Colour4f(r/255.0, g/255.0, b/255.0, a/255.0 * alphamul);
//	drawfuncs->Line(x1, y1, x2, y1);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void Draw_ColoredString3(float x, float y, const char *str, clrinfo_t *clr, int huh, int wut)
{
	drawfuncs->Colour4f(clr->c[0]/255.0, clr->c[1]/255.0, clr->c[2]/255.0, clr->c[3]/255.0 * alphamul);
	drawfuncs->String(x, y, str);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void UI_PrintTextBlock(float x, float y, float w, float h, char *str, int flags)
{
}
void Draw_AlphaRectangleRGB(int x, int y, int w, int h, int foo, int bar, byte r, byte g, byte b, byte a)
{
	float x1 = x;
	float x2 = x+w;
	float y1 = y;
	float y2 = y+h;
	drawfuncs->Colour4f(r/255.0, g/255.0, b/255.0, a/255.0 * alphamul);
	drawfuncs->Line(x1, y1, x2, y1);
	drawfuncs->Line(x2, y1, x2, y2);
	drawfuncs->Line(x1, y2, x2, y2);
	drawfuncs->Line(x1, y1, x1, y2);
	drawfuncs->Colour4f(1, 1, 1, 1);
}
void Draw_AlphaLineRGB(float x1, float y1, float x2, float y2, float width, byte r, byte g, byte b, byte a)
{
	drawfuncs->Colour4f(r/255.0, g/255.0, b/255.0, a/255.0 * alphamul);
	drawfuncs->Line(x1, y1, x2, y2);
	drawfuncs->Colour4f(1, 1, 1, 1);
}

mpic_t *Draw_CachePicSafe(const char *name, qbool crash, qbool ignorewad)
{
	if (!*name)
		return NULL;
	return (mpic_t*)(qintptr_t)drawfuncs->LoadImage(name);
}
mpic_t *Draw_CacheWadPic(const char *name)
{
	char ftename[MAX_QPATH];
	Q_snprintf(ftename, sizeof(ftename), "gfx/%s", name);
	return (mpic_t*)(qintptr_t)drawfuncs->LoadImage(ftename);
}

mpic_t *SCR_LoadCursorImage(char *cursorimage)
{
	return Draw_CachePicSafe(cursorimage, false, true);
}

unsigned int	Sbar_ColorForMap (unsigned int m)
{
	if (m >= 16)
		return m;
	m = (m < 0) ? 0 : ((m > 13) ? 13 : m);
	m *= 16;
	return m < 128 ? m + 8 : m + 8;
}
int Sbar_TopColor(player_info_t *pi)
{
	return Sbar_ColorForMap(pi->topcolour);
}
int Sbar_BottomColor(player_info_t *pi)
{
	return Sbar_ColorForMap(pi->bottomcolour);
}
int dehex(char nib)
{
	if (nib >= '0' && nib <= '9')
		return nib - '0';
	if (nib >= 'a' && nib <= 'f')
		return nib - 'a' + 10;
	if (nib >= 'A' && nib <= 'F')
		return nib - 'A' + 10;
	return 0;
}
char *TP_ParseFunChars(char *str)
{
	static char resultbuf[1024];
	char *out = resultbuf, *end = resultbuf+sizeof(resultbuf)-1;

	while (out < end)
	{
		if (str[0] == '$' && str[1] == 'x' && str[2] && str[3])
		{
			*out++ = (dehex(str[2]) << 4) | dehex(str[3]);
			str+=4;
		}
		else if (str[0] == '$')
		{
			int c = 0;
			switch (str[1])
			{
				case '\\': c = 0x0D; break;
				case ':': c = 0x0A; break;
				case '[': c = 0x10; break;
				case ']': c = 0x11; break;
				case 'G': c = 0x86; break;
				case 'R': c = 0x87; break;
				case 'Y': c = 0x88; break;
				case 'B': c = 0x89; break;
				case '(': c = 0x80; break;
				case '=': c = 0x81; break;
				case ')': c = 0x82; break;
				case 'a': c = 0x83; break;
				case '<': c = 0x1d; break;
				case '-': c = 0x1e; break;
				case '>': c = 0x1f; break;
				case ',': c = 0x1c; break;
				case '.': c = 0x9c; break;
				case 'b': c = 0x8b; break;
				case 'c':
				case 'd': c = 0x8d; break;
				case '$': c = '$'; break;
				case '^': c = '^'; break;
				case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9': c = str[1] -'0' + 0x12;break;
			}
			if (c)
			{
				*out++ = c;
				str++;
			}
			str++;
		}
		else if (*str)
			*out++ = *str++;
		else
			break;
	}
	*out = 0;
	return resultbuf;
}
char *TP_ItemName(unsigned int itbit)
{
	cvar_t *var = NULL;
	switch (itbit)
	{
#define ITEMNAME(it, nam)	case it: var = cvarfuncs->GetNVFDG("tp_name_"#nam, #nam, 0, NULL, "Item Names"); break
	ITEMNAME(IT_SHOTGUN,			sg);
	ITEMNAME(IT_SUPER_SHOTGUN,		ssg);
	ITEMNAME(IT_NAILGUN,			ng);
	ITEMNAME(IT_SUPER_NAILGUN,		sng);
	ITEMNAME(IT_GRENADE_LAUNCHER,	gl);
	ITEMNAME(IT_ROCKET_LAUNCHER,	rl);
	ITEMNAME(IT_LIGHTNING,			lg);
//	ITEMNAME(IT_SUPER_LIGHTNING,	???);
	ITEMNAME(IT_SHELLS,				shells);
	ITEMNAME(IT_NAILS,				nails);
	ITEMNAME(IT_ROCKETS,			rockets);
	ITEMNAME(IT_CELLS,				cells);
	ITEMNAME(IT_AXE,				axe);
	ITEMNAME(IT_ARMOR1,				ga);
	ITEMNAME(IT_ARMOR2,				ya);
	ITEMNAME(IT_ARMOR3,				ra);
	ITEMNAME(IT_SUPERHEALTH,		mh);
//	ITEMNAME(IT_KEY1,				);
//	ITEMNAME(IT_KEY2,				);
	ITEMNAME(IT_INVISIBILITY,		ring);
	ITEMNAME(IT_INVULNERABILITY,	pent);
	ITEMNAME(IT_SUIT,				suit);
	ITEMNAME(IT_QUAD,				quad);
//	ITEMNAME(IT_SIGIL1,				);
//	ITEMNAME(IT_SIGIL2,				);
//	ITEMNAME(IT_SIGIL3,				);
//	ITEMNAME(IT_SIGIL4,				);
	}
	if (var)
		return var->string;
	return va("it%#x", itbit);
}

void Replace_In_String(char *src, size_t strsize, char leadchar, int patterns, ...)
{
	char orig[1024];
	char *out, *outstop;
	va_list ap;
	int i;

	strlcpy(orig, src, sizeof(orig));
	out = src;
	outstop = out + strsize-1;
	src = orig;

	while(*src)
	{
		if (out == outstop)
			break;
		if (*src != leadchar)
			*out++ = *src++;
		else if (*++src == leadchar)
			*out++ = leadchar;
		else
		{
			va_start(ap, patterns);
			for (i = 0; i < patterns; i++)
			{
				const char *arg = va_arg(ap, const char *);
				const char *val = va_arg(ap, const char *);
				size_t alen = strlen(arg);
				if (!strncmp(src, arg, strlen(arg)))
				{
					strlcpy(out, val, (outstop-out)+1);
					out += strlen(out);
					src += alen;
					break;
				}
			}
			if (i == patterns)
			{
				strlcpy(out, "unknown", (outstop-out)+1);
				out += strlen(out);
			}
			va_end(ap);
		}
	}
	*out = 0;
}

int SCR_GetClockStringWidth(const char *s, qbool big, float scale)
{
	int w = 0;
	if (big)
	{
		while(*s)
			w += ((*s++==':')?16:24);
	}
	else
		w = strlen(s) * 8;
	return w * scale;
}
int SCR_GetClockStringHeight(qbool big, float scale)
{
	return (big?24:8)*scale;
}
char *SecondsToMinutesString(int print_time, char *buffer, size_t buffersize) {
	int tens_minutes, minutes, tens_seconds, seconds;

	tens_minutes = fmod (print_time / 600, 6);
	minutes = fmod (print_time / 60, 10);
	tens_seconds = fmod (print_time / 10, 6);
	seconds = fmod (print_time, 10);
	snprintf (buffer, buffersize, "%i%i:%i%i", tens_minutes, minutes, tens_seconds, seconds);
	return buffer;
}
char *SCR_GetGameTime(int t, char *buffer, size_t buffersize)
{
	float timelimit;

	timelimit = (t == TIMETYPE_GAMECLOCKINV) ? 60 * infofloat(cl.serverinfo, "timelimit", 0) + 1: 0;

	if (cl.countdown || cl.standby)
		SecondsToMinutesString(timelimit, buffer, buffersize);
	else
		SecondsToMinutesString((int) fabs(timelimit - (cl.time - cl.matchstart)), buffer, buffersize);
	return buffer;
}
	
const char* SCR_GetTimeString(int timetype, const char *format)
{
	static char buffer[256];
	switch(timetype)
	{
	case TIMETYPE_CLOCK:
		{
			time_t t;
			struct tm *ptm;
			time (&t);
			ptm = localtime (&t);
			if (!ptm) return "-:-";
			if (!strftime(buffer, sizeof(buffer)-1, format, ptm)) return "-:-";
			return buffer;
		}
	case TIMETYPE_GAMECLOCK:
	case TIMETYPE_GAMECLOCKINV:
		return SCR_GetGameTime(timetype, buffer, sizeof(buffer));

	case TIMETYPE_DEMOCLOCK:
		return SecondsToMinutesString(infofloat(cl.serverinfo, "demotime", 0), buffer, sizeof(buffer));

	default:
		return "01234";
	}
}
void SCR_DrawBigClock(int x, int y, int style, int blink, float scale, const char *t)
{
    extern  mpic_t  *sb_nums[2][11];
    extern  mpic_t  *sb_colon/*, *sb_slash*/;
	qbool lblink = (int)(cls.realtime*2) & 1;

    if (style > 1)  style = 1;
    if (style < 0)  style = 0;

	while (*t)
    {
        if (*t >= '0'  &&  *t <= '9')
        {
			Draw_STransPic(x, y, sb_nums[style][*t-'0'], scale);
            x += 24*scale;
        }
        else if (*t == ':')
        {
            if (lblink || !blink)
				Draw_STransPic (x, y, sb_colon, scale);

			x += 16*scale;
        }
        else
		{
			Draw_SCharacter(x, y, *t+(style?128:0), 3*scale);
            x += 24*scale;
		}
        t++;
    }
}
void SCR_DrawSmallClock(int x, int y, int style, int blink, float scale, const char *t)
{
	qbool lblink = (int)(cls.realtime*2) & 1;
	int c;

    if (style > 3)  style = 3;
    if (style < 0)  style = 0;

    while (*t)
    {
		c = (int) *t;
        if (c >= '0'  &&  c <= '9')
        {
            if (style == 1)
                c += 128;
            else if (style == 2  ||  style == 3)
                c -= 30;
        }
        else if (c == ':')
        {
            if (style == 1  ||  style == 3)
                c += 128;
            if (lblink ||  !blink)
                ;
            else
                c = ' ';
        }
        Draw_SCharacter(x, y, c, scale);
        x+= 8*scale;
        t++;
    }
}

#include "builtin_huds.h"
void EZHud_UseNquake_f(void)
{
	const char *hudstr = builtin_hud_nquake;
	cmdfuncs->AddText(hudstr, true);
}

int IN_BestWeapon(void)
{
	return 0;
}

qbool VID_VSyncIsOn(void){return false;}
double vid_vsync_lag;


vrect_t scr_vrect;

void EZHud_Tick(double realtime, double gametime)
{
	cls.realtime = realtime;
	cl.time = gametime;
}
char *findinfo(char *info, char *findkey)
{
	int kl = strlen(findkey);
	char *key, *value;
	while(*info)
	{
		key = strchr(info, '\\');
		if (!key)
			break;
		key++;
		value = strchr(key, '\\');
		if (!value)
			break;
		info = value+1;
		if (!strncmp(key, findkey, kl) && key[kl] == '\\')
			return info;
	}
	return NULL;
}
char *infostring(char *info, char *findkey, char *buffer, size_t bufsize)
{
	char *value = findinfo(info, findkey);
	char *end;
	if (value)
	{
		end = strchr(value, '\\');
		if (!end)
			end = value + strlen(value);
		bufsize--;
		if (bufsize > (end - value))
			bufsize = (end - value);
		memcpy(buffer, value, bufsize);
		buffer[bufsize] = 0;
		return buffer;
	}
	*buffer = 0;
	return buffer;
}
float infofloat(char *info, char *findkey, float def)
{
	char *value = findinfo(info, findkey);
	if (value)
		return atof(value);
	return def;
}
int EZHud_Draw(int seat, float viewx, float viewy, float viewwidth, float viewheight, int showscores)
{
	char serverinfo[4096];
	char val[64];
	int i;

	static float lasttime, lasttime_min = 99999;
	static int framecount;
	static float oldtime;
	if (cls.realtime - lasttime > 1)
	{
		cls.fps = framecount/(cls.realtime - lasttime);
		lasttime = cls.realtime;
		framecount = 0;

		if (cls.realtime - lasttime_min > 30)
		{
			cls.min_fps = cls.fps;
			lasttime_min = cls.realtime;
		}
		else if (cls.min_fps > cls.fps)
			cls.min_fps = cls.fps;
	}
	if (!seat)
	{
		cls.frametime = cls.realtime - oldtime;
		framecount++;
		oldtime = cls.realtime;
	}

	cl.splitscreenview = seat;
	scr_vrect.x = viewx;
	scr_vrect.y = viewy;
	scr_vrect.width = viewwidth;
	scr_vrect.height = viewheight;
	sb_showscores = showscores & 1;
	sb_showteamscores = showscores & 2;

	clientfuncs->GetStats(0, cl.stats, sizeof(cl.stats)/sizeof(cl.stats[0]));
	for (i = 0; i < 32; i++)
		clientfuncs->GetPlayerInfo(i, &cl.players[i]);

	clientfuncs->GetLocalPlayerNumbers(cl.splitscreenview, 1, &cl.playernum, &cl.tracknum);
	clientfuncs->GetServerInfoRaw(cl.serverinfo, sizeof(serverinfo));
	cl.deathmatch = infofloat(cl.serverinfo, "deathmatch", 0);
	cl.teamplay = infofloat(cl.serverinfo, "teamplay", 0);
	cl.intermission = infofloat(cl.serverinfo, "intermission", 0);
	cl.spectator = (cl.playernum>=32)||cl.players[cl.playernum].spectator;
	infostring(cl.serverinfo, "status", val, sizeof(val));
	cl.standby = !strcmp(val, "standby");
	cl.countdown = !strcmp(val, "countdown");
	cl.matchstart = infofloat(cl.serverinfo, "matchstart", 0);
	cls.state = ca_active;

	infostring(cl.serverinfo, "demotype", val, sizeof(val));
	cls.mvdplayback = !strcmp(val, "mvd");
	cls.demoplayback = strcmp(val, "");

	
	{
		static cvar_t *pscr_viewsize = NULL;
		int size;
		if (!pscr_viewsize)
			pscr_viewsize = cvarfuncs->GetNVFDG("viewsize", "100", 0, NULL, NULL);
		size = cl.intermission ? 120 : pscr_viewsize->value;
		if (size >= 120)
			sb_lines = 0;           // no status bar at all
		else if (size >= 110)
			sb_lines = 24;          // no inventory
		else
			sb_lines = 24 + 16 + 8;
	}

	clientfuncs->GetPredInfo(seat, cl.simvel);


	//cl.faceanimtime
	//cl.item_gettime

	//cls.state;
	//cls.min_fps;
	//cls.fps;
	////cls.realtime;
	////cls.trueframetime;

	host_screenupdatecount++;
	HUD_Draw();
	return true;
}

unsigned int keydown[K_MAX];
float cursor_x;
float cursor_y;
float mouse_x;
float mouse_y;
mpic_t	*scr_cursor_icon	= NULL;
qboolean QDECL EZHud_MenuEvent(int eventtype, int keyparam, int unicodeparm, float mousecursor_x, float mousecursor_y, float vidwidth, float vidheight)
{
	mouse_x += mousecursor_x - cursor_x;	//FIXME: the hud editor should NOT need this sort of thing
	mouse_y += mousecursor_y - cursor_y;
	cursor_x = mousecursor_x;
	cursor_y = mousecursor_y;

	HUD_Editor_MouseEvent(cursor_x, cursor_y);

	switch(eventtype)
	{
	case 0:	//draw
		host_screenupdatecount++;
		HUD_Draw();
		HUD_Editor_Draw();

		mouse_x = 0;
		mouse_y = 0;
		break;
	case 1:
		if (keyparam < K_MAX)
			keydown[keyparam] = true;
		HUD_Editor_Key(keyparam, 0, true);
		break;
	case 2:
		if (keyparam < K_MAX)
			keydown[keyparam] = false;
		HUD_Editor_Key(keyparam, 0, false);
		break;
	}
	return 1;
}

qboolean Plug_Init(void)
{
	drawfuncs = plugfuncs->GetEngineInterface(plug2dfuncs_name, sizeof(*drawfuncs));
	clientfuncs = plugfuncs->GetEngineInterface(plugclientfuncs_name, sizeof(*clientfuncs));
	filefuncs =  plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	inputfuncs = plugfuncs->GetEngineInterface(pluginputfuncs_name, sizeof(*inputfuncs));

	plugfuncs->ExportFunction("UpdateVideo", EZHud_UpdateVideo);

	if (cvarfuncs && drawfuncs && clientfuncs && filefuncs && inputfuncs &&
		plugfuncs->ExportFunction("SbarBase", EZHud_Draw) &&
		plugfuncs->ExportFunction("MenuEvent", EZHud_MenuEvent) &&
		plugfuncs->ExportFunction("Tick", EZHud_Tick))
	{
		Cmd_AddCommand("ezhud_nquake", EZHud_UseNquake_f);
		HUD_Init();
		HUD_Editor_Init();
		return true;
	}
	Con_Printf("EZHud: Unable to initiailise\n");
	return false;
}
