//read menu.h

#include "quakedef.h"
#include "winquake.h"
#include "shader.h"

#ifndef NOBUILTINMENUS

extern cvar_t maxclients;

/* MULTIPLAYER MENU */
void M_Menu_MultiPlayer_f (void)
{
	menubutton_t *b;
	emenu_t *menu;
	mpic_t *p;
	int mgt;
	static menuresel_t resel;

	p = NULL;

	mgt = M_GameType();

	menu = M_CreateMenu(0);

#ifdef Q2CLIENT
	if (mgt == MGT_QUAKE2)
	{
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_multiplayer");

		menu->selecteditem = (menuoption_t*)
		MC_AddConsoleCommand	(menu, 64, 170, 40,	"Join network server",	"menu_slist\n");
#ifdef HAVE_PACKET
		MC_AddConsoleCommand	(menu, 64, 170, 48,	"Quick Connect",		"quickconnect qw\n");
#endif
#ifdef Q2SERVER
		MC_AddConsoleCommand	(menu, 64, 170, 56,	"Start network server",	"menu_newmulti\n");
#endif
		MC_AddConsoleCommand	(menu, 64, 170, 64,	"Player setup",			"menu_setup\n");
		MC_AddConsoleCommand	(menu, 64, 170, 72,	"Demos",				"menu_demo\n");

		menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 48, 0, 40, NULL, false);
		return;
	}
	else
#endif
#ifdef HEXEN2
		if (mgt == MGT_HEXEN2)
	{
		MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title4.lmp");

		mgt=64;
		menu->selecteditem = (menuoption_t*)
		MC_AddConsoleCommandHexen2BigFont	(menu, 80, mgt,	"Server List ",	"menu_slist\n");mgt+=20;
		MC_AddConsoleCommandHexen2BigFont	(menu, 80, mgt,	"New Server  ",	"menu_newmulti\n");mgt+=20;
		MC_AddConsoleCommandHexen2BigFont	(menu, 80, mgt,	"Player Setup",	"menu_setup\n");mgt+=20;
		MC_AddConsoleCommandHexen2BigFont	(menu, 80, mgt,	"Demos       ",	"menu_demo\n");mgt+=20;

		menu->cursoritem = (menuoption_t *)MC_AddCursor(menu, &resel, 48, 64);
		return;
	}
	else
#endif
		if (QBigFontWorks())
	{
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_multi.lmp");

		mgt=32;
		menu->selecteditem = (menuoption_t*)
		MC_AddConsoleCommandQBigFont	(menu, 72, mgt,	localtext("Server List "),	"menu_slist\n");mgt+=20;
#ifdef HAVE_PACKET
		MC_AddConsoleCommandQBigFont	(menu, 72, mgt,	localtext("Quick Connect"), "quickconnect qw\n");mgt+=20;
#endif
		MC_AddConsoleCommandQBigFont	(menu, 72, mgt,	localtext("New Server  "),	"menu_newmulti\n");mgt+=20;
		MC_AddConsoleCommandQBigFont	(menu, 72, mgt,	localtext("Player Setup"),	"menu_setup\n");mgt+=20;
		MC_AddConsoleCommandQBigFont	(menu, 72, mgt,	localtext("Demos       "),	"menu_demo\n");mgt+=20;

		menu->cursoritem = (menuoption_t*)MC_AddCursor(menu, &resel, 54, 32);
		return;
	}
	else
	{
		int width;

		p = R2D_SafeCachePic("gfx/mp_menu.lmp");
		if (p)
		{
			MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
			MC_AddCenterPicture(menu, 4, 24, "gfx/p_multi.lmp");
			MC_AddPicture(menu, 72, 32, 232, 64, "gfx/mp_menu.lmp");
		}

		if (R_GetShaderSizes(p, &width, NULL, true) <= 0)
			width = 232;

		b = MC_AddConsoleCommand(menu, 72, 320, 32, "", "menu_slist\n");
		menu->selecteditem = (menuoption_t*)b;
		b->common.height = 20;
		b->common.width = width;
		b = MC_AddConsoleCommand(menu, 72, 320, 52, "", "menu_newmulti\n");
		b->common.height = 20;
		b->common.width = width;
		b = MC_AddConsoleCommand(menu, 72, 320, 72, "", "menu_setup\n");
		b->common.height = 20;
		b->common.width = width;

		b = MC_AddConsoleCommand(menu, 72, 320, 92, "", "menu_demo\n");
		MC_AddWhiteText(menu, 72, 0, 92+20/2-6, localtext("^aDemos"), false);
		b->common.height = 20;
		b->common.width = width;

#ifdef HAVE_PACKET
		b = MC_AddConsoleCommand(menu, 72, 320, 112, "", "quickconnect qw\n");
		MC_AddWhiteText(menu, 72, 0, 112+20/2-6, localtext("^aQuick Connect"), false);
		b->common.height = 20;
		b->common.width = width;
#endif
	}

	menu->cursoritem = (menuoption_t*)MC_AddCursor(menu, &resel, 54, 32);
}

extern cvar_t	team, skin;
extern cvar_t	topcolor;
extern cvar_t	bottomcolor;
extern cvar_t skill;
typedef struct {
	menuedit_t *nameedit;
	menuedit_t *teamedit;
	menuedit_t *skinedit;
#ifdef HEXEN2
	menucombo_t *classedit;
	int ticlass;
#endif
	menucombo_t *modeledit;
	unsigned int topcolour;
	unsigned int lowercolour;

	int tiwidth, tiheight;
	qbyte translationimage[128*128];
} setupmenu_t;
void MSetup_Removed(emenu_t *menu)
{
	char bot[64], top[64];
	setupmenu_t *info = menu->data;

	if (info->nameedit)
		Cvar_Set(&name, info->nameedit->text);
	if (info->teamedit)
		Cvar_Set(&team, info->teamedit->text);
	if (info->skinedit)
		Cvar_Set(&skin, info->skinedit->text);
#ifdef HEXEN2
	if (info->classedit)
		Cvar_SetValue(Cvar_FindVar("cl_playerclass"), info->classedit->selectedoption+1);
#endif
	if (info->lowercolour >= 16)
		Q_snprintfz(bot, sizeof(bot), "0x%x", info->lowercolour&0xffffff);
	else
		Q_snprintfz(bot, sizeof(bot), "%i", info->lowercolour);
	if (info->topcolour >= 16)
		Q_snprintfz(top, sizeof(top), "0x%x", info->topcolour&0xffffff);
	else
		Q_snprintfz(top, sizeof(top), "%i", info->topcolour);
	Cbuf_AddText(va("color %s %s\n", top, bot), RESTRICT_LOCAL);
}

//http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
static void rgbtohsv(unsigned int rgb, vec3_t result)
{
	int r = (rgb>>16)&0xff, g = (rgb>>8)&0xff, b = (rgb>>0)&0xff;
	float maxc = max(r, max(g, b)), minc = min(r, min(g, b));
    float h, s, l = (maxc + minc) / 2;

	float d = maxc - minc;
	if (maxc)
		s = d / maxc;
	else
		s = 0;

	if(maxc == minc)
	{
		h = 0; // achromatic
	}
	else
	{
		if (maxc == r)
			h = (g - b) / d + ((g < b) ? 6 : 0);
		else if (maxc == g)
			h = (b - r) / d + 2;
		else
			h = (r - g) / d + 4;
		h /= 6;
    }

	result[0] = h;
	result[1] = s;
	result[2] = l;
};

//http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
static unsigned int hsvtorgb(float inh, float s, float v)
{
	int r, g, b;
	float h = inh - (int)floor(inh);
	int i = h * 6;
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	switch(i)
	{
	default:
	case 0: r = v*0xff, g = t*0xff, b = p*0xff; break;
	case 1: r = q*0xff, g = v*0xff, b = p*0xff; break;
	case 2: r = p*0xff, g = v*0xff, b = t*0xff; break;
	case 3: r = p*0xff, g = q*0xff, b = v*0xff; break;
	case 4: r = t*0xff, g = p*0xff, b = v*0xff; break;
	case 5: r = v*0xff, g = p*0xff, b = q*0xff; break;
	}

	return 0xff000000 | (r<<16)|(g<<8)|(b<<0);
};

qboolean SetupMenuColour (union menuoption_s *option,struct emenu_s *menu, int key)
{
	setupmenu_t *info = menu->data;
	unsigned int *ptr = (*option->button.text == 'T')?&info->topcolour:&info->lowercolour;

	//okay, this is a bit weird.
	//fte supports rgb colours, but we only allow hue to be chosen via the menu (people picking pure black are annoying, also conversions and precisions limit us)
	//for NQ compat, we stick to old-skool values (so we don't end up with far too many teams)
	//but we give the top free reign.
	//unless they hold shift, in which case it switches around
	//this allows for whatever you want
	if (key == K_ENTER || key == K_KP_ENTER || key == K_GP_DIAMOND_CONFIRM || key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_MOUSE1 || key == K_TOUCHTAP || key == K_GP_DPAD_RIGHT)
	{
		if ((keydown[K_LSHIFT] || keydown[K_RSHIFT]) ^ (ptr == &info->topcolour))
		{
			vec3_t hsv;
			rgbtohsv(*ptr, hsv);
			*ptr = hsvtorgb(hsv[0]+((key == K_MOUSE1)?5:1)/128.0, 1, 1);//hsv[1], hsv[2]);
		}
		else
		{
			if (*ptr >= 13 || *ptr >= 16)
				*ptr = 0;
			else
				*ptr += 1;
		}
		S_LocalSound ("misc/menu2.wav");
		return true;
	}
	if (key == K_DEL)
	{
		*ptr = 0;
		S_LocalSound ("misc/menu2.wav");
		return true;
	}
	if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT)
	{
		if ((keydown[K_LSHIFT] || keydown[K_RSHIFT]) ^ (ptr == &info->topcolour))
		{
			vec3_t hsv;
			rgbtohsv(*ptr, hsv);
			*ptr = hsvtorgb(hsv[0]-1/128.0, 1, 1);//hsv[1], hsv[2]);
		}
		else
		{
			if (*ptr==0 || *ptr >= 16)
				*ptr=12;
			else
				*ptr -= 1;
		}
		S_LocalSound ("misc/menu2.wav");
		return true;
	}
	return false;
}


typedef struct {
	char **names;
	int entries;
	int match;
} q2skinsearch_t;

int QDECL q2skin_enumerate(const char *name, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	char blah[MAX_QPATH];
	q2skinsearch_t *s = parm;

	COM_StripExtension(name+8, blah, sizeof(blah));
	if (strlen(blah) < 2)
		return false;	//this should never happen
	blah[strlen(blah)-2] = 0;

	s->names = BZ_Realloc(s->names, ((s->entries+64)&~63) * sizeof(char*));
	s->names[s->entries] = BZ_Malloc(strlen(blah)+1);
	strcpy(s->names[s->entries], blah);

	if (!strcmp(blah, skin.string))
		s->match = s->entries;

	s->entries++;
	return true;
}
void q2skin_destroy(q2skinsearch_t *s)
{
	int i;
	for (i = 0; i < s->entries; i++)
	{
		BZ_Free(s->names[i]);
	}
	BZ_Free(s);
}

qboolean MSetupQ2_ChangeSkin (struct menucustom_s *option,struct emenu_s *menu, int key, unsigned int unicode)
{
	setupmenu_t *info = menu->data;
	q2skinsearch_t *s = Z_Malloc(sizeof(*s));
	COM_EnumerateFiles(va("players/%s/*_i.*", info->modeledit->values[info->modeledit->selectedoption]), q2skin_enumerate, s);
	if (key == K_ENTER || key == K_KP_ENTER || key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_GP_DPAD_RIGHT || key == K_GP_DIAMOND_CONFIRM)
	{
		s->match ++;
		if (s->match>=s->entries)
			s->match=0;
	}
	else if (key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_GP_DPAD_LEFT || key == K_GP_DIAMOND_ALTCONFIRM)
	{
		s->match --;
		if (s->match<=0)
			s->match=s->entries-1;
	}
	else
	{
		q2skin_destroy(s);
		return false;
	}
	if (s->entries)
		Cvar_Set(&skin, s->names[s->match]);
	S_LocalSound ("misc/menu2.wav");
	q2skin_destroy(s);
	return true;
}
void MSetupQ2_TransDraw (int x, int y, menucustom_t *option, emenu_t *menu)
{
	setupmenu_t *info = menu->data;
	mpic_t	*p;
	int w, h;

	Draw_FunStringWidth(x, y, "Skin", 160-64, true, !menu->cursoritem && menu->selecteditem == (menuoption_t*)option);
	x += 160-40;//172-16

	p = R2D_SafeCachePic (va("players/%s_i", skin.string));
	if (!R_GetShaderSizes(p, &w, &h, false))
	{
		q2skinsearch_t *s = Z_Malloc(sizeof(*s));
		COM_EnumerateFiles(va("players/%s/*_i.*", info->modeledit->values[info->modeledit->selectedoption]), q2skin_enumerate, s);
		if (s->entries)
			Cvar_Set(&skin, s->names[rand()%s->entries]);
		q2skin_destroy(s);

		p = R2D_SafeCachePic (va("players/%s_i", skin.string));
	}
	if (R_GetShaderSizes(p, &w, &h, false)>0)
		R2D_ScalePic (x, y-8, w, h, p);
}

void MSetup_TransDraw (int x, int y, menucustom_t *option, emenu_t *menu)
{
	unsigned int translationTable[256];
	setupmenu_t *info = menu->data;
	mpic_t	*p;
	void *f;
	qboolean reloadtimage = false;
	unsigned int pc = 0;

	if (info->skinedit && info->skinedit->modified)
	{
		info->skinedit->modified = false;
		reloadtimage = true;
	}
#ifdef HEXEN2
	if (info->classedit)
	{
		if (info->classedit->selectedoption != info->ticlass)
		{
			info->ticlass = info->classedit->selectedoption;
			reloadtimage = true;
		}
		pc = info->ticlass+1;
	}
#endif

	if (reloadtimage)
	{
#ifdef HEXEN2
		if (info->classedit)	//quake2 main menu.
		{
			FS_LoadFile(va("gfx/menu/netp%i.lmp", info->ticlass+1), &f);
		}
		else
#endif
		{
			if (*info->skinedit->text)
				FS_LoadFile(va("gfx/player/%s.lmp", info->skinedit->text), &f);
			else
				f = NULL;
			if (!f)
				FS_LoadFile("gfx/menuplyr.lmp", &f);
		}

		if (f)
		{
			info->tiwidth = ((int*)f)[0];
			info->tiheight = ((int*)f)[1];
			if (info->tiwidth * info->tiheight > sizeof(info->translationimage))
				info->tiwidth = info->tiheight = 0;
			memcpy(info->translationimage, (char*)f+8, info->tiwidth*info->tiheight);
			FS_FreeFile(f);
		}
	}

	R2D_ImageColours(1,1,1,1);
	p = R2D_SafeCachePic ("gfx/bigbox.lmp");
	if (R_GetShaderSizes(p, NULL, NULL, false)>0)
		R2D_ScalePic (x-12, y-8, 72, 72, p);

	M_BuildTranslationTable(pc, info->topcolour, info->lowercolour, translationTable);
	R2D_TransPicTranslate (x, y, info->tiwidth, info->tiheight, info->translationimage, translationTable);
}

void M_Menu_Setup_f (void)
{
	setupmenu_t *info;
	emenu_t *menu;
	menucustom_t *ci;
	menubutton_t *b;
	static menuresel_t resel;
	int y;

#ifdef Q2CLIENT
	if (M_GameType() == MGT_QUAKE2)	//quake2 main menu.
	{
		static const char *modeloptions[] =
		{
			"male",
			"female",
			NULL
		};
		menucustom_t *cu;

		menu = M_CreateMenu(sizeof(setupmenu_t));
		menu->remove = MSetup_Removed;
		info = menu->data;
//		menu->key = MC_Main_Key;

		MC_AddPicture(menu, 0, 4, 38, 166, "pics/m_main_plaque");
		MC_AddPicture(menu, 0, 173, 36, 42, "pics/m_main_logo");

		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_player_setup");

		menu->selecteditem = (menuoption_t*)
		(info->nameedit = MC_AddEdit(menu, 64, 160, 40, localtext("Your name"), name.string));
		(info->modeledit = MC_AddCvarCombo(menu, 64, 160,72, localtext("model"), &skin, (const char **)modeloptions, (const char **)modeloptions));
		info->modeledit->selectedoption = !strncmp(skin.string, "female", 6);
		cu = MC_AddCustom(menu, 64, 88+16, NULL, 0, NULL);
		cu->draw = MSetupQ2_TransDraw;
		cu->key = MSetupQ2_ChangeSkin;

		menu->cursoritem = (menuoption_t*)MC_AddCursorSmall(menu, &resel, 54);
		return;
	}
#endif

	menu = M_CreateMenu(sizeof(setupmenu_t));
	menu->remove = MSetup_Removed;
	info = menu->data;

//	MC_AddPicture(menu, 72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	y = 40;
	menu->selecteditem = (menuoption_t*)
	(info->nameedit = MC_AddEdit(menu, 64, 160, y, localtext("Your name"), name.string)); y+= info->nameedit->common.height;
	(info->teamedit = MC_AddEdit(menu, 64, 160, y, localtext("Your team"), team.string)); y+= info->teamedit->common.height;
#ifdef HEXEN2
	info->ticlass = -1;
	if (M_GameType() == MGT_HEXEN2)
	{
		static const char *classnames[] =
		{
			"Paladin",
			"Crusader",
			"Necromancer",
			"Assasin",
			"Demoness",
			NULL
		};
		cvar_t *pc = Cvar_Get("cl_playerclass", "1", CVAR_USERINFO|CVAR_ARCHIVE, "Hexen2");
		(info->classedit = MC_AddCombo(menu, 64, 160, y, localtext("Your class"), (const char **)classnames, pc->ival-1)); y+= info->classedit->common.height;

		//trim options if the artwork is missing.
		while (info->classedit->numoptions && !COM_FCheckExists(va("gfx/menu/netp%i.lmp", info->classedit->numoptions)))
			info->classedit->numoptions--;
	}
	else
#endif
	{
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_multi.lmp");

		(info->skinedit = MC_AddEdit(menu, 64, 160, y, localtext("Your skin"), skin.string)); y+= info->skinedit->common.height;
	}

	ci = MC_AddCustom(menu, 172+32, y, NULL, 0, NULL);
	ci->draw = MSetup_TransDraw;
	ci->key = NULL;

	MC_AddCommand(menu, 64, 160, y+8, localtext("Top colour"), SetupMenuColour);
	MC_AddCommand(menu, 64, 160, y+32, localtext("Lower colour"), SetupMenuColour);
	y+= 16;
	y+=4;

	b = MC_AddConsoleCommand(menu, 64, 204, 168, localtext("Network Settings"), "menu_network\n");
	b->common.tooltip = localtext("Change network and client prediction settings.");
	y += b->common.height;
	b = MC_AddConsoleCommand(menu, 64, 204, 176, localtext("Teamplay Settings"), "menu_teamplay\n");
	b->common.tooltip = localtext("Change teamplay macro settings.");
	y += b->common.height;
	menu->cursoritem = (menuoption_t*)MC_AddCursorSmall(menu, &resel, 54);


	info->lowercolour = bottomcolor.value;
	info->topcolour = topcolor.value;
	if (info->skinedit)
		info->skinedit->modified = true;
}




#ifdef CLIENTONLY
void M_Menu_GameOptions_f (void)
{
}
#else

typedef struct {
	menuedit_t *hostnameedit;
	menucombo_t *publicgame;
	menucombo_t *deathmatch;
	menucombo_t *numplayers;
	menucombo_t *teamplay;
	menucombo_t *skill;
	menucombo_t *timelimit;
	menucombo_t *fraglimit;
	menucombo_t *mapname;
	menucheck_t *rundedicated;

	int topcolour;
	int lowercolour;
} newmultimenu_t;

static const char *numplayeroptions[] = {
	"2",
	"3",
	"4",
	"8",
	"12",
	"16",
	"20",
	"24",
	"32",
	NULL
};

qboolean MultiBeginGame (union menuoption_s *option,struct emenu_s *menu, int key)
{
	newmultimenu_t *info = menu->data;
	char quoted[1024];
	if (key != K_ENTER && key != K_KP_ENTER && key != K_GP_DIAMOND_CONFIRM && key != K_MOUSE1 && key != K_TOUCHTAP)
		return false;

	if (cls.state)
		Cbuf_AddText("disconnect\n", RESTRICT_LOCAL);

	Cbuf_AddText("sv_playerslots \"\"\n", RESTRICT_LOCAL);	//just in case.
	Cbuf_AddText(va("maxclients \"%s\"\n", numplayeroptions[info->numplayers->selectedoption]), RESTRICT_LOCAL);
	if (info->rundedicated->value)
		Cbuf_AddText("setrenderer dedicated\n", RESTRICT_LOCAL);
	Cbuf_AddText(va("hostname %s\n", COM_QuotedString(info->hostnameedit->text, quoted, sizeof(quoted), false)), RESTRICT_LOCAL);
	Cbuf_AddText(va("deathmatch %i\n", info->deathmatch->selectedoption), RESTRICT_LOCAL);
	if (!info->deathmatch->selectedoption)
		Cbuf_AddText("coop 1\n", RESTRICT_LOCAL);
	else
		Cbuf_AddText("coop 0\n", RESTRICT_LOCAL);
	Cbuf_AddText(va("teamplay %i\n", info->teamplay->selectedoption), RESTRICT_LOCAL);
	Cbuf_AddText(va("skill %i\n", info->skill->selectedoption), RESTRICT_LOCAL);
	Cbuf_AddText(va("timelimit %i\n", info->timelimit->selectedoption*5), RESTRICT_LOCAL);
	Cbuf_AddText(va("fraglimit %i\n", info->fraglimit->selectedoption*10), RESTRICT_LOCAL);

	if (info->publicgame->selectedoption-1 == 2)
	{
		const char *hostname = info->hostnameedit->text;
		const char *shn;
		for (shn = hostname; *shn; shn++)
		{
			if (*shn >= 'a' && *shn <= 'z')
				continue;
			if (*shn >= 'A' && *shn <= 'Z')
				continue;
			if (*shn >= '0' && *shn <= '9')
				continue;
			if (*shn == '-' || *shn <= '_')
				continue;
			break;
		}
		if (*shn || !*hostname || !strcasecmp(hostname, "player") || !strcasecmp(hostname, "unnamed"))	//not simple enough...
			Cbuf_AddText(va("sv_public \"/\"\n"), RESTRICT_LOCAL);
		else
			Cbuf_AddText(va("sv_public \"/%s\"\n", info->hostnameedit->text), RESTRICT_LOCAL);
	}
	else
		Cbuf_AddText(va("sv_public %i\n", info->publicgame->selectedoption-1), RESTRICT_LOCAL);

	Cbuf_AddText(va("map \"%s\"\n", info->mapname->options[info->mapname->selectedoption]), RESTRICT_LOCAL);

	if (info->rundedicated->value)
	{
		Cbuf_AddText("echo You can use the setrenderer command to return to a graphical interface at any time\n", RESTRICT_LOCAL);
	}

	M_RemoveAllMenus(true);

	return true;
}

struct mapopts_s
{
	size_t max, count;
	const char **maps;
};
static int QDECL M_Menu_GameOptions_AddMap(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct mapopts_s *ctx = parm;
	size_t i;
	char *ext;
	char trimmedfname[MAX_QPATH];
	if (Q_strncasecmp(fname, "maps/", 5))
		return true; //o.O
	fname += 5;
	if (fname[0] == 'b' && fname[1] == '_')
		return true;	//stoopid ammo boxes.
	ext = strrchr(fname, '.');
	if (ext && !strcmp(ext, ".bsp") && ext-fname<sizeof(trimmedfname))
	{
		memcpy(trimmedfname, fname, ext-fname);
		trimmedfname[ext-fname] = 0;
		fname = trimmedfname;
	}

	for (i = 0; i < ctx->count; i++)
		if (!Q_strcasecmp(ctx->maps[i], fname))
			return true; //don't do dupes.
	if (ctx->count+1 >= ctx->max)
		Z_ReallocElements((void**)&ctx->maps, &ctx->max, ctx->count + 64, sizeof(char*));
	ctx->maps[ctx->count++] = Z_StrDup(fname);
	return true;
}


void M_Menu_GameOptions_f (void)
{
	static const char *deathmatchoptions[] = {
		"Cooperative",
		"Deathmatch 1",
		"Deathmatch 2",
		"Deathmatch 3",
		"Deathmatch 4",
		"Deathmatch 5",
		NULL
	};
	static const char *teamplayoptions[] = {
		"off",				//no teams at all
		"no self+team fire",	//don't hurt same-team (with bugs in coop)
		"friendly fire",	//scoreboard shows teams, gamecode doesn't care
		"no team fire (qw-only)",	//like 1, except you still hurt yourself.
		NULL
	};
	static const char *teamplayoptions_rogue[] = {
		"off",				//no teams at all
		"no self+team fire",	//don't hurt same-team (with bugs in coop)
		"friendly fire",	//scoreboard shows teams, gamecode doesn't care
		"tag",
		"Capture The Flag",
		"One Flag CTF",
		"Three Team CTF",
		NULL
	};
	static const char *skilloptions[] = {
		"Easy",
		"Medium",
		"Hard",
		"NIGHTMARE",
		NULL
	};
	static const char *timelimitoptions[] = {
		"no limit",
		"5 minutes",
		"10 minutes",
		"15 minutes",
		"20 minutes",
		"25 minutes",
		"30 minutes",
		"35 minutes",
		"40 minutes",
		"45 minutes",
		"50 minutes",
		"55 minutes",
		"1 hour",
		NULL
	};
	static const char *fraglimitoptions[] = {
		"no limit",
		"10 frags",
		"20 frags",
		"30 frags",
		"40 frags",
		"50 frags",
		"60 frags",
		"70 frags",
		"80 frags",
		"90 frags",
		"100 frags",
		NULL
	};
	static const char *publicoptions[] = {
		"Splitscreen Only",
		"Private/LAN",
		"Public (Manual)",
		"Public (Holepunch)",
		NULL
	};
	extern cvar_t sv_public;
	newmultimenu_t *info;
	emenu_t *menu;
	int y = 40;
	int mgt;
	int players;
	struct mapopts_s mapopts = {0};

	menu = M_CreateMenu(sizeof(newmultimenu_t));
	info = menu->data;

	mgt = M_GameType();

	if (mgt == MGT_QUAKE2)
	{
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_start_server");
		y += 8;
	}
	else if (mgt == MGT_HEXEN2)
	{
	}
	else
	{
		MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_multi.lmp");
	}

//	MC_AddPicture(menu, 72, 32, ("gfx/mp_menu.lmp") );

	menu->selecteditem = (menuoption_t*)
	MC_AddCommand						(menu, 64, 160, y,	localtext("Start game"), MultiBeginGame);y+=16;

	y+=4;
	info->hostnameedit	= MC_AddEdit	(menu, 64, 160, y,			localtext("Hostname"), name.string);y+=info->hostnameedit->common.height;
	info->publicgame	= MC_AddCombo	(menu, 64, 160, y,			localtext("Public"), publicoptions, bound(0, sv_public.ival+1, 4));y+=8;
#if !defined(FTE_TARGET_WEB) && defined(HAVE_DTLS)
	{
		static const char *encoptions[] =
		{
			"Disabled",
			"Accept",
			"Request",
			"Require",
			NULL
		};
		MC_AddCvarCombo (menu, 64, 160, y,		localtext("DTLS Encryption"), &net_enable_dtls, encoptions, NULL);y+=8;
	}
#endif
	y+=4;

	for (players = 0; players < sizeof(numplayeroptions)/ sizeof(numplayeroptions[0]); players++)
	{
		if (atoi(numplayeroptions[players]) >= maxclients.value)
			break;
	}

	info->numplayers	= MC_AddCombo	(menu, 64, 160, y,			localtext("Max players"), (const char **)numplayeroptions,	players);y+=8;

	info->deathmatch	= MC_AddCombo	(menu, 64, 160, y,			localtext("Deathmatch"), (const char **)deathmatchoptions,	deathmatch.value);y+=8;
	info->teamplay		= MC_AddCombo	(menu, 64, 160, y,			localtext("Teamplay"), (!strcasecmp(FS_GetGamedir(true), "rogue")?(const char **)teamplayoptions_rogue:(const char **)teamplayoptions),		teamplay.value);y+=8;
	info->skill			= MC_AddCombo	(menu, 64, 160, y,			localtext("Skill"), (const char **)skilloptions,			skill.value);y+=8;
	info->rundedicated	= MC_AddCheckBox(menu, 64, 160, y,			localtext("dedicated"), NULL, 0);y+=8;
	y+=8;
	info->timelimit		= MC_AddCombo	(menu, 64, 160, y,			localtext("Time Limit"), (const char **)timelimitoptions,		timelimit.value/5);y+=8;
	info->fraglimit		= MC_AddCombo	(menu, 64, 160, y,			localtext("Frag Limit"), (const char **)fraglimitoptions,		fraglimit.value/10);y+=8;
	y+=8;

	//populate it with an appropriate default. its a shame it won't change with the deathmatch/coop options
	switch(mgt)
	{
	case MGT_QUAKE2:
		M_Menu_GameOptions_AddMap("maps/base1.bsp", 0, 0, &mapopts, NULL);
		break;
	case MGT_HEXEN2:
		M_Menu_GameOptions_AddMap("maps/demo1.bsp", 0, 0, &mapopts, NULL);
		break;
	case MGT_QUAKE1:
		M_Menu_GameOptions_AddMap("maps/start.bsp", 0, 0, &mapopts, NULL);
		break;
	default:
		break;
	}
	COM_EnumerateFiles("maps/*.bsp",	M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.bsp.gz",	M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.bsp.xz",	M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.map",	M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.map.gz",	M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.cm",		M_Menu_GameOptions_AddMap, &mapopts);
	COM_EnumerateFiles("maps/*.hmp",	M_Menu_GameOptions_AddMap, &mapopts);
	info->mapname		= MC_AddCombo	(menu, 64, 160, y,			localtext("Map"), (const char **)mapopts.maps,		0);y+=8;
	y += 16;

	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 54, 0, menu->selecteditem->common.posy, NULL, false);


	info->lowercolour = bottomcolor.value;
	info->topcolour = topcolor.value;
}
#endif

void M_Menu_Teamplay_f (void)
{
#ifdef QWSKINS
	static const char *noskinsoptions[] =
	{
		"Enabled",
		"Disabled",
		"No Download",
		NULL
	};
	static const char *noskinsvalues[] =
	{
		"0",
		"1",
		"2",
		NULL
	};
#endif

	extern cvar_t cl_parseSay, cl_triggers, tp_forceTriggers, tp_loadlocs, cl_parseFunChars, cl_noblink, noskins;
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Options", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
#ifdef QWSKINS
		MB_COMBOCVAR("Skins", noskins, noskinsoptions, noskinsvalues, "Enable or disable player skin usage. No download will use skins but will not download them from the server."),
#endif
		MB_EDITCVARTIP("Enemy Skin", "cl_enemyskin", "Override enemy skin with this."),
		MB_EDITCVARTIP("Team Skin", "cl_teamskin", "Override teammate skin with this."),
		MB_EDITCVARTIP("Fake Name", "cl_fakename", "Name that appears in teamplay messages"),
		MB_CHECKBOXCVARTIP("Parse Fun Chars", cl_parseFunChars, 0, "Whether to parse fun characters"),
		MB_CHECKBOXCVARTIP("Parse Macros", cl_parseSay, 0, "Whether to parse teamplay macros like %l etc."),
		MB_CHECKBOXCVARTIP("Load Locs", tp_loadlocs, 0, "Whether to load teamplay locations from .loc files"),
		MB_CHECKBOXCVARTIP("No Blink", cl_noblink, 0, "No blinking characters"),
		MB_EDITCVARTIP("Sound Trigger", "tp_soundtrigger", "Character that indicates the following text is a wav file.\nExample:\nsay_team ~location.wav$\\me: I'm at %l #a"),
		MB_EDITCVARTIP("Weapon Order", "tp_weapon_order","Weapon preference order:\n8 = Lightning Gun\n7 = Rocket Launcher\n6 = Grenade Launcher\n5 = Super Nailgun\n4 = Nailgun\n3 = Super Shotgun\n2 = Shotgun\n1 = Axe"),
		MB_CHECKBOXCVARTIP("Teamplay Triggers", cl_triggers, 0, "Enable or disable teamplay triggers"),
		MB_CHECKBOXCVARTIP("Force Triggers", tp_forceTriggers, 0, "Whether to force teamplay triggers in non-teamplay play like in a 1 on 1 situation"),
		MB_SPACING(4),
		MB_CONSOLECMD("Location Names", "menu_teamplay_locations\n", "Modify team play location settings."),
		MB_CONSOLECMD("Item Needs", "menu_teamplay_needs\n", "Modify messages for item needs in team play macros."),
		MB_CONSOLECMD("Item Names", "menu_teamplay_items\n", "Modify messages for items in team play macros."),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Locations_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Location Names", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Separator", "loc_name_separator", "Location name seperator character(s)"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Super Shotgun", "loc_name_ssg", "Short name for Super Shotgun in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Nailgun", "loc_name_ng", "Short name for Nailgun in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Super Nailgun", "loc_name_sng", "Short name for Super Nailgun in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Grenade Launcher", "loc_name_gl", "Short name for Grenade Launcher in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Rocket Launcher", "loc_name_rl", "Short name for Rocket Launcher in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Lightning Gun", "loc_name_lg", "Short name for Lightning Gun in teamplay location 'reports'"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Quad Damage", "loc_name_quad", "Short name for Quad Damage in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Pentagram", "loc_name_pent", "Short name for Pentagram of Protection in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Ring of Invis", "loc_name_ring", "Short name for Ring of Invisibility in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Suit", "loc_name_suit", "Short name for Environment Suit in teamplay location 'reports'"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Green Armor", "loc_name_ga", "Short name for Green Armor in teamplay location 'reports'"),
		MB_EDITCVARSLIM("Yellow Armor", "loc_name_ya", "Short name for Yellow Armor in teamplay location 'reports'" ),
		MB_EDITCVARSLIM("Red Armor", "loc_name_ra", "Short name for Red Armor in teamplay location 'reports'"),
		// TODO: we probably need an actual back button or some such
		//MB_SPACING(4),
		//MB_CONSOLECMD("\x7f Teamplay", "menu_teamplay\n", "Return to the teamplay menu."),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Needs_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Needed Items", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Shells", "tp_need_shells", "Short name for Shotgun Shells in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Nails", "tp_need_nails", "Short name for Nails in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Rockets", "tp_need_rockets", "Short name for Rockets/Grenades in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Cells", "tp_need_cells", "Short name for Power Cells in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Rocket Launcher", "tp_need_rl", "Short name for Rocket Launcher in teamplay 'need' reports"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Green Armor", "tp_need_ga", "Short name for Green Armor in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Yellow Armor", "tp_need_ya", "Short name for Yellow Armor in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Red Armor", "tp_need_ra", "Short name for Red Armor in teamplay 'need' reports"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Health", "tp_need_health", "Short name for Health in teamplay 'need' reports"),
		MB_EDITCVARSLIM("Weapon", "tp_need_weapon", "Need weapon preference order:\n8 = Lightning Gun\n7 = Rocket Launcher\n6 = Grenade Launcher\n5 = Super Nailgun\n4 = Nailgun\n3 = Super Shotgun\n2 = Shotgun\n1 = Axe"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Item Names", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_CONSOLECMD("Armor", "menu_teamplay_armor\n", "Modify team play macro armor names."),
		MB_CONSOLECMD("Weapon", "menu_teamplay_weapons\n", "Modify team play macro weapon names."),
		MB_CONSOLECMD("Powerups", "menu_teamplay_powerups\n", "Modify team play macro powerup names."),
		MB_CONSOLECMD("Ammo/Health", "menu_teamplay_ammo_health\n", "Modify team play macro ammo and health names."),
		MB_CONSOLECMD("Team Fortress", "menu_teamplay_team_fortress\n", "Modify Team Fortress exclusive team play macro names."),
		MB_CONSOLECMD("Status/Location/Misc", "menu_teamplay_status_location_misc\n", "Modify status, location, and miscellaneous team play macro names."),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 224, y);
}

void M_Menu_Teamplay_Items_Armor_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Armor Names", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Armor", "tp_name_armor", "Short name for Armor type"),
		MB_EDITCVARSLIM("Green Type -", "tp_name_armortype_ga", "Short name for Green Armor type"),
		MB_EDITCVARSLIM("Yellow Type -", "tp_name_armortype_ya", "Short name for Yellow Armor type"),
		MB_EDITCVARSLIM("Red Type -", "tp_name_armortype_ra", "Short name for Red Armor type"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Green Armor", "tp_name_ga", "Short name for Green Armor"),
		MB_EDITCVARSLIM("Yellow Armor", "tp_name_ya", "Short name for Yellow Armor"),
		MB_EDITCVARSLIM("Red Armor", "tp_name_ra", "Short name for Red Armor"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_Weapons_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Weapon Names", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Weapon", "tp_name_weapon", "Short name for Weapon"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Axe", "tp_name_axe", "Short name for Weapon"),
		MB_EDITCVARSLIM("Shotgun", "tp_name_sg", "Short name for Shotgun"),
		MB_EDITCVARSLIM("Super Shotgun", "tp_name_ssg", "Short name for Super Shotgun"),
		MB_EDITCVARSLIM("Nailgun", "tp_name_ng", "Short name for Nailgun"),
		MB_EDITCVARSLIM("Super Nailgun", "tp_name_sng", "Short name for Super Nailgun"),
		MB_EDITCVARSLIM("Grenade Launcher", "tp_name_gl", "Short name for Grenade Launcher"),
		MB_EDITCVARSLIM("Rocket Launcher", "tp_name_rl", "Short name for Rocket Launcher"),
		MB_EDITCVARSLIM("Lightning Gun", "tp_name_lg", "Short name for Lightning Gun"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_Powerups_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Powerup Names", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Quad Damage", "tp_name_quad", "Short name for Quad Damage"),
		MB_EDITCVARSLIM("Pentagram", "tp_name_pent", "Short name for Pentgram of Protection"),
		MB_EDITCVARSLIM("Ring of Invis", "tp_name_ring", "Short name for Ring Of Invisibilty"),
		MB_EDITCVARSLIM("Suit", "tp_name_suit", "Short name for Environment Suit"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Quaded", "tp_name_quaded", "Short name for reporting being 'Quaded'. Dying by another player who has Quad Damage"),
		MB_EDITCVARSLIM("Pented", "tp_name_pented", "Short name for reporting being 'Pented'. Dying by another player who has the Pentagram"),
		MB_EDITCVARSLIM("Eyes (Ringed)", "tp_name_eyes", "Short name for reporting being 'Ringed', Dying by another player who has Eyes (Invisibility)"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Resistance Rune", "tp_name_rune_1", "Short name for Resistance Rune"),
		MB_EDITCVARSLIM("Strength Rune", "tp_name_rune_2", "Short name for Strength Rune"),
		MB_EDITCVARSLIM("Haste Rune", "tp_name_rune_3", "Short name for Haste Rune"),
		MB_EDITCVARSLIM("Regen Rune", "tp_name_rune_4", "Short name for Regeneration Rune"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_Ammo_Health_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Ammo/Health", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Shells", "tp_name_shells", "Short name for Shells"),
		MB_EDITCVARSLIM("Nails", "tp_name_nails", "Short name for Nails"),
		MB_EDITCVARSLIM("Rockets", "tp_name_rockets", "Short name for Rockets"),
		MB_EDITCVARSLIM("Cells", "tp_name_cells", "Short name for Cells"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Backpack", "tp_name_backpack", "Short name for Backpack"),
		MB_EDITCVARSLIM("Health", "tp_name_health", "Short name for Health"),
		MB_EDITCVARSLIM("Mega Health", "tp_name_mh", "Short name for Mega Health"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_Team_Fortress_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Team Fortress", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Sentry Gun", "tp_name_sentry", "Short name for the Engineer's Sentry Gun"),
		MB_EDITCVARSLIM("Dispenser", "tp_name_disp", "Short name for the Engineer's Ammo Dispenser"),
		MB_EDITCVARSLIM("Flag", "tp_name_flag", "Short name for Flag"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Teamplay_Items_Status_Location_Misc_f (void)
{
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Teamplay Misc", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Enemy", "tp_name_enemy", "Short for Enemy in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Teammate", "tp_name_teammate", "Short for Enemy in teamplay 'status' & 'location' reports"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("At (Location)", "tp_name_at", "Short for @ (Location) in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("None", "tp_name_none", "Short for None in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Nothing", "tp_name_nothing", "Short for Nothing in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Separator", "tp_name_separator", "Seperator character(s) in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Some place", "tp_name_someplace", "Short for Someplace in teamplay 'status' & 'location' reports"),
		MB_SPACING(4),
		MB_EDITCVARSLIM("Red Status", "tp_name_status_red", "Macro for Status Red in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Green Status", "tp_name_status_green", "Macro for Status Green in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Blue Status", "tp_name_status_blue", "Macro for Status Blue in teamplay 'status' & 'location' reports"),
		MB_EDITCVARSLIM("Yellow Status", "tp_name_status_yellow", "Macro for Status Yellow in teamplay 'status' & 'location' reports"),
		MB_END()
	};
	static menuresel_t resel;
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}

void M_Menu_Network_f (void)
{
#if MAX_SPLITS > 1
	static const char *splitopts[] = {
		"Disabled",
		"2 Screens",
#if MAX_SPLITS > 2
		"3 Screens",
#endif
#if MAX_SPLITS > 3
		"4 Screens",
#endif
		NULL
	};
	static const char *splitvalues[] =
	{
		"0",
		"1",
#if MAX_SPLITS > 2
		"2",
#endif
#if MAX_SPLITS > 3
		"3",
#endif
		NULL
	};
#endif
	static const char *smoothingopts[] = {
		"Lower Latency",
		"Smoother",
		"Smooth Demos Only",
		NULL
	};
#ifdef HAVE_DTLS
	static const char *dtlsopts[] = {
		"Disabled",
		"Accept",
		"Request",
		"Require",
		NULL
	};
#endif
	static const char *smoothingvalues[] = {"0", "1", "2", NULL};
	extern cvar_t cl_download_csprogs, cl_download_redirection, requiredownloads, cl_solid_players;
	extern cvar_t cl_predict_players, cl_lerp_smooth, cl_predict_extrapolate;
	static menuresel_t resel;
	int y;
	menubulk_t bulk[] =
	{
		MB_REDTEXT("Network Settings", true),
		MB_TEXT("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082", true),
		MB_EDITCVARSLIM("Network FPS", "cl_netfps", "Sets ammount of FPS used to communicate with server (sent and received)"),
		MB_EDITCVARSLIM("Rate", "rate", "Maximum bytes per second that the server should send to the client"),
		MB_EDITCVARSLIM("Download Rate", "drate", "Maximum bytes per second that the server should send maps and demos to the client"),
#ifdef HAVE_DTLS
		MB_COMBOCVAR("DTLS Encryption", net_enable_dtls, dtlsopts, NULL, "Use this to avoid snooping. Certificates will be pinned."),
#endif
		MB_SPACING(4),
		MB_CHECKBOXCVARTIP("Wait for Downloads", requiredownloads, 0, "Ignore downloaded content sent to the client and connect immediately"),
		MB_CHECKBOXCVARTIP("Redirect Download", cl_download_redirection, 0, "Whether the client will ignore download redirection from servers"),
		MB_CHECKBOXCVARTIP("Download CSQC", cl_download_csprogs, 0, "Whether to allow the client to download CSQC (client-side QuakeC) progs from servers"),
		MB_SPACING(4),
		MB_COMBOCVAR("Network Smoothing", cl_lerp_smooth, smoothingopts, smoothingvalues, "Smoother gameplay comes at the cost of higher latency. Which do you favour?"),
		MB_CHECKBOXCVARTIP("Extrapolate Prediction", cl_predict_extrapolate, 0, "Extrapolate local player movement beyond the frames already sent to the server"),
		MB_CHECKBOXCVARTIP("Predict Other Players", cl_predict_players, 0, "Toggle player prediction"),
		MB_CHECKBOXCVARTIP("Solid Players", cl_solid_players, 0, "When running/clipping into other players, ON make it appear they are solid, OFF will make it appear like running into a marshmellon."),
#if MAX_SPLITS > 1
		MB_COMBOCVAR("Split-screen", cl_splitscreen, splitopts, splitvalues, "Enables split screen with a number of clients. This feature requires server support."),
#endif
		MB_END()
	};
	emenu_t *menu = M_Options_Title(&y, 0);
	MC_AddBulk(menu, &resel, bulk, 16, 200, y);
}
#endif
