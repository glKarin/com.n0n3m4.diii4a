//read menu.h

#include "quakedef.h"
#include "winquake.h"
#include "shader.h"
#ifndef NOBUILTINMENUS
#if !defined(CLIENTONLY) && defined(SAVEDGAMES)
//=============================================================================
/* LOAD/SAVE MENU */

typedef struct {
	int issave;
	int cursorpos;
	menutext_t *cursoritem;

	int picslot;
	shader_t *picshader;
} loadsavemenuinfo_t;

#define SAVEFIRST_AUTO 1
#define SAVECOUNT_AUTO 3
#define SAVEFIRST_STANDARD (SAVEFIRST_AUTO + SAVECOUNT_AUTO)
#define SAVECOUNT_STANDARD 20
#define	MAX_SAVEGAMES		(1+SAVECOUNT_AUTO+SAVECOUNT_STANDARD)
static struct
{
	qboolean loadable;
	qbyte	saveable; //0=autosave, 1=regular, 2=quick
	char	sname[9];
	char	desc[22+1];
	char	kills[39-22+1];
	char	time[64];
	char	map[32];
} m_saves[MAX_SAVEGAMES];

static void M_ScanSave(unsigned int slot, const char *name, qbyte savable)
{
	char	*in, *out, *end;
	int		j;
	char	line[MAX_OSPATH];
	flocation_t loc;
	time_t	mtime;
	vfsfile_t	*f;
	int		version;

	m_saves[slot].saveable = savable;
	m_saves[slot].loadable = false;
	Q_strncpyz (m_saves[slot].sname, name, sizeof(m_saves[slot].sname));
	Q_strncpyz (m_saves[slot].desc, "--- UNUSED SLOT ---", sizeof(m_saves[slot].desc));
	Q_strncpyz (m_saves[slot].kills, "", sizeof(m_saves[slot].kills));
	Q_strncpyz (m_saves[slot].time, "", sizeof(m_saves[slot].time));

	snprintf (line, sizeof(line), "saves/%s/info.fsv", m_saves[slot].sname);
	if (!FS_FLocateFile(line, FSLF_DONTREFERENCE|FSLF_IGNOREPURE, &loc))
	{	//legacy saved games from some other engine
		snprintf (line, sizeof(line), "%s.sav", m_saves[slot].sname);
		if (!FS_FLocateFile(line, FSLF_DONTREFERENCE|FSLF_IGNOREPURE, &loc))
			return;	//not found
	}
	f = FS_OpenReadLocation(line, &loc);
	if (f)
	{
		VFS_GETS(f, line, sizeof(line));
		version = atoi(line);
		if (version != SAVEGAME_VERSION_NQ && version != SAVEGAME_VERSION_QW && version != SAVEGAME_VERSION_FTE_LEG && (version < SAVEGAME_VERSION_FTE_HUB || version >= SAVEGAME_VERSION_FTE_HUB+GT_MAX))
		{
			Q_strncpyz (m_saves[slot].desc, "Incompatible version", sizeof(m_saves[slot].desc));
			VFS_CLOSE (f);
			return;
		}

		// read the desc, change _ back to space, fill the separate fields
		VFS_GETS(f, line, sizeof(line));
		for (j=0 ; line[j] ; j++)
			if (line[j] == '_')
				line[j] = ' ';
		for (; j < sizeof(line[j]); j++)
			line[j] = '\0';
		memcpy(m_saves[slot].desc, line, 22);
		m_saves[slot].desc[22] = 0;

		for (in = line+22, out = m_saves[slot].kills, end = line+39; in < end && *in == ' '; )
			in++;
		for (out = m_saves[slot].kills; in < end; )
			*out++ = *in++;
		for (end = m_saves[slot].kills; out > end && out[-1] == ' '; )
			out--;
		*out = 0;

		if (strlen(line) > 39)
			Q_strncpyz(m_saves[slot].time, line+39, sizeof(m_saves[slot].time));
		else if (FS_GetLocMTime(&loc, &mtime))
			strftime(m_saves[slot].time, sizeof(m_saves[slot].time), "%Y-%m-%d %H:%M:%S", localtime( &mtime ));
		// else time unknown, just leave it blank

		if (version == 5 || version == 6)
		{
			for (j = 0; j < 16; j++)
				VFS_GETS(f, line, sizeof(line));	//16 parms
			VFS_GETS(f, line, sizeof(line));	//skill
			VFS_GETS(f, m_saves[slot].map, sizeof(m_saves[slot].map));
		}

		m_saves[slot].loadable = true;
		VFS_CLOSE (f);
	}
}

const char *M_ChooseAutoSave(void)
{
	int i, j;

	for (i = SAVEFIRST_AUTO; i < SAVEFIRST_AUTO+SAVECOUNT_AUTO; i++)
	{
		M_ScanSave(i, va("a%i", i-SAVEFIRST_AUTO), false);
		if (!m_saves[i].loadable)
			return m_saves[i].sname;
	}

	for (i = SAVEFIRST_AUTO; i < SAVEFIRST_AUTO+SAVECOUNT_AUTO; i++)
	{
		for (j = SAVEFIRST_AUTO; j < SAVEFIRST_AUTO+SAVECOUNT_AUTO; j++)
			if (strcmp(m_saves[i].time, m_saves[j].time) > 0)
				break;
		if (j == SAVEFIRST_AUTO+SAVECOUNT_AUTO)
			return m_saves[i].sname;		
	}

	return m_saves[SAVEFIRST_AUTO].sname;
}

static void M_ScanSaves (void)
{
	int i;
	M_ScanSave(0, "quick", 2);
	for (i=SAVEFIRST_AUTO ; i<SAVEFIRST_AUTO+SAVECOUNT_AUTO ; i++)
		M_ScanSave(i, va("a%i", i-SAVEFIRST_AUTO), false);
	for (i=SAVEFIRST_STANDARD ; i<SAVEFIRST_STANDARD+SAVECOUNT_STANDARD ; i++)
		M_ScanSave(i, va("s%i", i-SAVEFIRST_STANDARD), true);
}

static void M_Menu_LoadSave_UnloadShaders(emenu_t *menu)
{
	loadsavemenuinfo_t *info = menu->data;
	if (info->picshader)
	{
		Image_UnloadTexture(info->picshader->defaulttextures->base);
		R_UnloadShader(info->picshader);
		info->picshader = NULL;
	}
}

static void M_Menu_LoadSave_Preview_Draw(int x, int y, menucustom_t *item, emenu_t *menu)
{
	loadsavemenuinfo_t *info = menu->data;
	int slot;
	if (!menu->selecteditem)
		return;
	slot = (menu->selecteditem->common.posy - 32)/8;
	if (slot >= 0 && slot < MAX_SAVEGAMES)
	{
		int width, height;
		if (slot != info->picslot || !info->picshader)
		{
			info->picslot = slot;
			if (info->picshader)
			{
				Image_UnloadTexture(info->picshader->defaulttextures->base);
				R_UnloadShader(info->picshader);
			}
			info->picshader = R_RegisterPic(va("saves/%s/screeny.tga", m_saves[slot].sname), NULL);
		}
		if (info->picshader)
		{
			shader_t *pic = NULL;
			switch(R_GetShaderSizes(info->picshader, &width, &height, false))
			{
			case 1:
				pic = info->picshader;
				break;
			case 0:
				if (*m_saves[slot].map)
					pic = R_RegisterPic(va("levelshots/%s", m_saves[slot].map), NULL);
				break;
			}
			if (pic)
			{
				int w = 160;
				int h = 120;
				if (R_GetShaderSizes(pic, &width, &height, false) <= 0)
				{
					width = 64;
					height = 64;
				}
	
				if ((float)width/height > (float)w/h)
				{
					w = 160;
					h = ((float)w*height) / width;
				}
				else
				{
					h = 120;
					w = ((float)h*width) / height;
				}
				R2D_ScalePic (x + (160-w)/2, y + (120-h)/2, w, h, pic);
			}
		}
		Draw_FunStringWidth(x, y+120+0, m_saves[slot].time, 160, 2, false);
		Draw_FunStringWidth(x, y+120+8, m_saves[slot].kills, 160, 2, false);

		switch(m_saves[slot].saveable)
		{
		case 2:
			Draw_FunStringWidth(x, y+120+16, "Quick Save", 160, 2, false);
			break;
		case 0:
			Draw_FunStringWidth(x, y+120+16, "Autosave", 160, 2, false);
			break;
		}
		Draw_FunStringWidth(x, y+120+24, m_saves[slot].sname, 160, 2, false);
	}
}

void M_Menu_Save_f (void)
{
	menuoption_t *op = NULL;
	emenu_t *menu;
	int		i;

	if (!sv.state)
		return;

	if (cl.intermissionmode != IM_NONE)
		return;

	menu = M_CreateMenu(sizeof(loadsavemenuinfo_t));
	menu->data = menu+1;
	menu->remove = M_Menu_LoadSave_UnloadShaders;
	menu->reset	 = M_Menu_LoadSave_UnloadShaders;
	
	switch(M_GameType())
	{
#ifdef Q2CLIENT
	case MGT_QUAKE2:
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_save_game.pcx");
		break;
#endif
	default:
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_save.lmp");
		break;
	}

	menu->cursoritem = (menuoption_t *)MC_AddRedText(menu, 8, 0, 32, NULL, false);	

	M_ScanSaves ();

	for (i=0 ; i< MAX_SAVEGAMES; i++)
	{
		if (m_saves[i].saveable)
			op = (menuoption_t *)MC_AddConsoleCommandf(menu, 16, 192, 32+8*i, false, m_saves[i].desc, "savegame %s\nclosemenu\n", m_saves[i].sname);
		else
			MC_AddWhiteText(menu, 16, 170, 32+8*i, m_saves[i].desc, false);
		if (!menu->selecteditem)
			menu->selecteditem = op;
	}

	MC_AddCustom(menu, 192, 60-16, NULL, 0, NULL)->draw = M_Menu_LoadSave_Preview_Draw;
}
void M_Menu_Load_f (void)
{
	menuoption_t *op = NULL;
	emenu_t *menu;
	int		i;
	char time[64];

	menu = M_CreateMenu(sizeof(loadsavemenuinfo_t));
	menu->data = menu+1;
	menu->remove = M_Menu_LoadSave_UnloadShaders;
	menu->reset	 = M_Menu_LoadSave_UnloadShaders;

	switch(M_GameType())
	{
#ifdef Q2CLIENT
	case MGT_QUAKE2:
		MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_load_game.pcx");
		break;
#endif
	default:
		MC_AddCenterPicture(menu, 4, 24, "gfx/p_load.lmp");
		break;
	}

	M_ScanSaves ();

	for (i=0 ; i< MAX_SAVEGAMES; i++)
	{
		if (m_saves[i].loadable)
			op = (menuoption_t *)MC_AddConsoleCommandf(menu, 16, 170, 32+8*i, false, m_saves[i].desc, "loadgame %s\nclosemenu\n", m_saves[i].sname);
		else
			MC_AddWhiteText(menu, 16, 170, 32+8*i, m_saves[i].desc, false);
		if (op)
			if (!menu->selecteditem || (*m_saves[i].time && strcmp(time, m_saves[i].time) < 0))
			{
				menu->selecteditem = op;
				Q_strncpyz(time, m_saves[i].time, sizeof(time));
			}
	}

	if (menu->selecteditem)
		menu->cursoritem = (menuoption_t *)MC_AddRedText(menu, 8, 0, menu->selecteditem->common.posy, NULL, false);	

	MC_AddCustom(menu, 192, 60-16, NULL, 0, NULL)->draw = M_Menu_LoadSave_Preview_Draw;
}


#endif

static qboolean M_SingleParseMapDBEpisodes(emenu_t *menu, int *y, qboolean bigfont)
{	//use the remaster's episode selection lists.
	size_t sz;
	char *file = FS_LoadMallocFile("mapdb.json", &sz);
	if (file)
	{
		json_t *j = JSON_Parse(file);
		json_t *episodes = JSON_FindChild(j, "episodes"), *e;
		int i = 0;
		while ((e=JSON_GetIndexed(episodes, i++)))
		{
			char namebuf[MAX_QPATH];
			char cmdbuf[MAX_QPATH];
			const char *command = JSON_GetString(e, "command", cmdbuf,sizeof(cmdbuf), NULL);
			const char *name = JSON_GetString(e, "name", namebuf,sizeof(namebuf), NULL);
			if (!command)
			{
				command = JSON_GetString(e, "dir", cmdbuf,sizeof(cmdbuf), NULL);
				if (command)
					command = va("gamedir %s; map start", command);
			}
			if (!command)
				continue;
			name = TL_Translate(com_language, name);
			if (name && command)
			{
				menubutton_t *b;
				if (bigfont)
					b = MC_AddConsoleCommandQBigFont(menu, 72, *y,	name,  va("closemenu;disconnect;maxclients 1;spectator \"\";samelevel \"\";deathmatch \"\";set_calc coop ($cl_splitscreen>0);%s\n", command)), *y += 20-8;
				else if (JSON_GetInteger(e, "needsClassSelect", false))
					b = MC_AddConsoleCommand		(menu, 64, 260, *y,	name,		va("menu_single class %s\n", command));
				else if (JSON_GetInteger(e, "needsSkillSelect", false))
					b = MC_AddConsoleCommand		(menu, 64, 260, *y,	name,		va("menu_single skill %s\n", command));
				else
					b = MC_AddConsoleCommand		(menu, 64, 260, *y,	name,		va("closemenu; skill 0;deathmatch 0; set_calc coop ($cl_splitscreen>0);%s\n",command));
				*y+=8;
				if (!menu->selecteditem)
					menu->selecteditem = (menuoption_t*)b;
			}
		}
		JSON_Destroy(j);
		FS_FreeFile(file);
		return true;
	}
	return false;
}

void M_Menu_SinglePlayer_f (void)
{
	emenu_t *menu;
#ifdef HAVE_SERVER
	menubutton_t *b;
	mpic_t *p;
	static menuresel_t resel;

#if MAX_SPLITS > 1
	static const char *splitopts[] =
		{
			"Single",
			"Dual",
			"Tripple",
			"QUAD",
			NULL
		};
	static const char *splitvals[] =
		{
			"0",
			"1",
			"2",
			"3",
			NULL
		};
#endif

	switch(M_GameType())
	{
#ifdef Q2CLIENT
	case MGT_QUAKE2:
		{
			int y = 40;
			const char *command = "newgame";
			menu = M_CreateMenu(0);

			MC_AddCenterPicture(menu, 4, 24, "pics/m_banner_game");

			if (!strncmp(Cmd_Argv(1), "skill", 5))
				command = Cmd_Argv(2);
			else
				M_SingleParseMapDBEpisodes(menu, &y, false);

			if (!menu->selecteditem)
			{ 	//quake2 uses the 'newgame' alias, which controls the intro video and then start map.
				menu->selecteditem = (menuoption_t*)
				MC_AddConsoleCommand	(menu, 64, 170, y,	"Easy",		va("closemenu; skill 0;deathmatch 0; set_calc coop ($cl_splitscreen>0);%s\n",command)); y+=8;
				MC_AddConsoleCommand	(menu, 64, 170, y,	"Medium",	va("closemenu; skill 1;deathmatch 0; set_calc coop ($cl_splitscreen>0);%s\n",command)); y+=8;
				MC_AddConsoleCommand	(menu, 64, 170, y,	"Hard",		va("closemenu; skill 2;deathmatch 0; set_calc coop ($cl_splitscreen>0);%s\n",command)); y+=8;
			}
			if (strncmp(Cmd_Argv(1), "skill", 5))
			{
#ifdef SAVEDGAMES
				y+=8;
				MC_AddConsoleCommand	(menu, 64, 170, y,	"Load Game", "menu_load\n"); y+=8;
				MC_AddConsoleCommand	(menu, 64, 170, y,	"Save Game", "menu_save\n"); y+=8;
#endif

#if MAX_SPLITS > 1
				y+=8;
				MC_AddCvarCombo(menu, 72, 170, y, localtext("Splitscreen"), &cl_splitscreen, splitopts, splitvals);
#endif
			}

			menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 48, 0, 40, NULL, false);
		}
		return;
#endif
#ifdef HEXEN2
	case MGT_HEXEN2:
		{
			int y;
			int i;
			cvar_t *pc;
			qboolean havemp;
			static char *classlistmp[] = {
				"Random",
				"Paladin",
				"Crusader",
				"Necromancer",
				"Assasin",
				"Demoness"
			};
			menubutton_t *b;
			havemp = !!COM_FCheckExists("maps/keep1.bsp");
			menu = M_CreateMenu(0);
			MC_AddPicture(menu, 16, 0, 35, 176, "gfx/menu/hplaque.lmp");

			Cvar_Get("cl_playerclass", "1", CVAR_USERINFO|CVAR_ARCHIVE, "Hexen2");

			y = 64-20;

			if (!strncmp(Cmd_Argv(1), "class", 5))
			{
				unsigned taken = 0;
				int oldclass;
				int pnum;
				pnum = atoi(Cmd_Argv(1)+5);
				cl_splitscreen.ival = bound(0, cl_splitscreen.ival, MAX_SPLITS-1);
				pnum = bound(1, pnum, cl_splitscreen.ival+1);

				MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title2.lmp");

				if (cl_splitscreen.ival)
					MC_AddBufferedText(menu, 80, 0, (y+=8)+12, va(localtext("Player %i\n"), pnum), false, true);

				for (i = 0; i < pnum-1 && i < countof(cls.userinfo); i++)
					taken |= 1<<atoi(InfoBuf_ValueForKey(&cls.userinfo[i], "cl_playerclass"));
				oldclass = atoi(InfoBuf_ValueForKey(&cls.userinfo[pnum-1], "cl_playerclass"));

				for (i = 0; i <= 4+havemp; i++)
				{
					b = MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,		(!i || !(taken&(1<<i)))?classlistmp[i]:(va(S_COLOR_GRAY"%s", classlistmp[i])),
							va("p%i setinfo cl_playerclass %i; menu_single %s %s\n",
								pnum,
								i?i:((rand()%(4+havemp))+1),
								((pnum+1 > cl_splitscreen.ival+1)?"skill":va("class%i",pnum+1)),
								Cmd_Argv(2)));
					if (!menu->selecteditem || i == oldclass)
						menu->selecteditem = (menuoption_t*)b;
				}
			}
			else if (!strncmp(Cmd_Argv(1), "skill", 5))
			{
				extern cvar_t skill;
				//yes, hexen2 has per-class names for the skill levels. because being weird and obtuse is kinda its forte
				static char *skillnames[6][4] =
				{
					{	//generic/random
						"Easy",
						"Medium",
						"Hard",
						"Nightmare"
					},
					{	//barbarian
						"Servant",	//string changed, because somehow the original is malicious. was: "Apprentice",
						"Squire",
						"Adept",
						"Lord"
					},
					{	//paladin
						"Gallant",
						"Holy Avenger",
						"Divine Hero",
						"Legend"
					},
					{	//necromancer
						"Sorcerer",
						"Dark Servant",
						"Warlock",
						"Lich King"
					},
					{	//assassin
						"Rogue",
						"Cutthroat",
						"Executioner",
						"Widow Maker"
					},
					{	//demoness
						"Larva",
						"Spawn",
						"Fiend",
						"She Bitch"
					}
				};
				char **sn = skillnames[0];
				pc = Cvar_Get("cl_playerclass", "1", CVAR_USERINFO|CVAR_ARCHIVE, "Hexen2");
				if (pc && (unsigned)pc->ival <= 5)
					sn = skillnames[pc->ival];

				MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title5.lmp");
				for (i = 0; i < 4; i++)
				{
					b = MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,	sn[i],	va("skill %i; closemenu; disconnect; deathmatch 0; coop %i;wait;map %s\n", i, cl_splitscreen.ival>0, Cmd_Argv(2)));
					if (!menu->selecteditem || i == skill.ival)
						menu->selecteditem = (menuoption_t*)b;
				}
			}
			else
			{
				MC_AddCenterPicture(menu, 0, 60, "gfx/menu/title1.lmp");
				//startmap selection in hexen2 is nasty.
				if (havemp)
				{
					menu->selecteditem = (menuoption_t*)
					MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,	"New Mission",	"menu_single class keep1\n");
					MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,	"Old Mission",	"menu_single class demo1\n");
				}
				else
				{
					menu->selecteditem = (menuoption_t*)
					MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,	"New Game",		"menu_single class demo1\n");
				}
#ifdef SAVEDGAMES
				MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,		"Save Game",	"menu_save\n");
				MC_AddConsoleCommandHexen2BigFont(menu, 80, y+=20,		"Load Game",	"menu_load\n");
#endif

				MC_AddCvarCombo(menu, 72, 170, y+=20, localtext("Splitscreen"), &cl_splitscreen, splitopts, splitvals);
			}

			menu->cursoritem = (menuoption_t *)MC_AddCursor(menu, menu->selecteditem?NULL:&resel, 56, menu->selecteditem?menu->selecteditem->common.posy:0);

			return;
		}
		break;
#endif
	default:
		if (QBigFontWorks())
		{
			int y = 32;
			menu = M_CreateMenu(0);
			MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
			MC_AddCenterPicture(menu, 4, 24, "gfx/ttl_sgl.lmp");

			if (M_SingleParseMapDBEpisodes(menu, &y, true))
				y += 20;
			else
			{
				menu->selecteditem = (menuoption_t*)
				MC_AddConsoleCommandQBigFont	(menu, 72, y,	"New Game",  "closemenu;disconnect;maxclients 1;spectator \"\";samelevel \"\";deathmatch \"\";set_calc coop ($cl_splitscreen>0);startmap_sp\n"); y += 20;
			}
#ifdef SAVEDGAMES
			MC_AddConsoleCommandQBigFont	(menu, 72, y,	"Load Game", "menu_load\n"); y+=20;
			MC_AddConsoleCommandQBigFont	(menu, 72, y,	"Save Game", "menu_save\n"); y+=20;
#endif

			menu->cursoritem = (menuoption_t*)MC_AddCursor(menu, &resel, 54, 32);
			return;
		}
		else
		{	//q1
			menu = M_CreateMenu(0);
			MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
			MC_AddCenterPicture(menu, 4, 24, "gfx/ttl_sgl.lmp");
		}
		break;
	}

	p = R2D_SafeCachePic("gfx/sp_menu.lmp");
	if (!p)
	{
		MC_AddWhiteText(menu, 92, 0, 12*8, "Couldn't find file", false);
		MC_AddWhiteText(menu, 92, 0, 13*8, "gfx/sp_menu.lmp", false);
		MC_AddBox (menu, 72, 11*8, 23*8, 4*8);
	}
	else
	{
		int width;
		if (R_GetShaderSizes(p, &width, NULL, true) <= 0)
			width = 232;

		MC_AddPicture(menu, 72, 32, 232, 64, "gfx/sp_menu.lmp");

		b = MC_AddConsoleCommand	(menu, 72, 304, 32,	"", "closemenu;disconnect;maxclients 1;spectator \"\";samelevel \"\";deathmatch \"\";set_calc coop ($cl_splitscreen>0);startmap_sp\n");
		menu->selecteditem = (menuoption_t *)b;
		b->common.width = width;
		b->common.height = 20;
#ifdef SAVEDGAMES
		b = MC_AddConsoleCommand	(menu, 72, 304, 52,	"", "menu_load\n");
		b->common.width = width;
		b->common.height = 20;
		b = MC_AddConsoleCommand	(menu, 72, 304, 72,	"", "menu_save\n");
		b->common.width = width;
		b->common.height = 20;
#endif

#if MAX_SPLITS > 1
		b = (menubutton_t*)MC_AddCvarCombo(menu, 72, 72+width/2, 92, "", &cl_splitscreen, splitopts, splitvals);
		MC_AddRedText(menu, 72, 0, 92, localtext("Splitscreen"), false);
		b->common.height = 20;
		b->common.width = width;
#endif

		menu->cursoritem = (menuoption_t*)MC_AddCursor(menu, &resel, 54, 32);
	}

#else
	menu = M_CreateMenu(0);

	MC_AddWhiteText(menu, 84, 0, 12*8, "This build is unable", false);
	MC_AddWhiteText(menu, 84, 0, 13*8, "to start a local game", false);

	MC_AddBox (menu, 60, 11*8, 25*8, 4*8);
#endif
}


typedef struct demoitem_s {
	qboolean isdir;
	int size;
	struct demoitem_s *next;
	struct demoitem_s *prev;
	char name[1];
} demoitem_t;

typedef struct {
	int fsroot;	//FS_SYSTEM, FS_GAME, FS_GAMEONLY. if FS_SYSTEM, executed command will have a leading #
	char path[MAX_OSPATH];
	char selname[MAX_OSPATH];
} demoloc_t;

typedef struct {
	menucustom_t *list;
	demoitem_t *selected;
	demoitem_t *firstitem;

	demoloc_t *fs; 
	int pathlen;

	//for the basedir picker...
	ftemanifest_t *man;

	char *command[64];	//these let the menu be used for nearly any sort of file browser.
	char *ext[64];
	int numext;

	int dragscroll;
	int mousedownpos;

	demoitem_t *items;
} demomenu_t;

static void M_DemoDraw(int x, int y, menucustom_t *control, emenu_t *menu)
{
	char *text;
	demomenu_t *info = menu->data;
	demoitem_t *item, *lostit;
	int ty;

	char displaypath[MAX_OSPATH];
	if (FS_DisplayPath(info->fs->path, (info->fs->fsroot==FS_GAME)?FS_GAMEONLY:info->fs->fsroot, displaypath, sizeof(displaypath)))
		Draw_FunString(x, y-16, displaypath);

	ty = vid.height-24;
	item = info->selected;
	while(item)
	{
		if (info->firstitem == item)
			break;
		if (ty < y)
		{
			//we couldn't find it
			for (lostit = info->firstitem; lostit; lostit = lostit->prev)
			{
				if (info->selected == lostit)
				{
					item = lostit;
					break;
				}
			}
			info->firstitem = item;
			break;
		}
		item = item->prev;
		ty-=8;
	}
	if (!item)
		info->firstitem = info->items;

	if (keydown[K_MOUSE1] || keydown[K_TOUCHSLIDE])
	{
		if (!info->dragscroll)
		{
			info->dragscroll = 1;
			info->mousedownpos = mousecursor_y-y;
		}
		if (info->dragscroll)
		{
			if (info->mousedownpos >= mousecursor_y-y+8)
			{
				info->dragscroll = 2;
				info->mousedownpos -= 8;
				if (info->firstitem->next)
				{
					if (info->firstitem == info->selected)
						info->selected = info->firstitem->next;
					info->firstitem = info->firstitem->next;
				}
			}
			if (info->mousedownpos+8 <= mousecursor_y-y)
			{
				info->dragscroll = 2;
				info->mousedownpos += 8;
				if (info->firstitem->prev)
				{
					if (ty <= 24)
						info->selected = info->selected->prev;
					info->firstitem = info->firstitem->prev;
				}
			}
		}
	}
	else
		info->dragscroll = 0;

	control->common.height = vid.height-y;

	item = info->firstitem;
	while(item)
	{
		if (y >= y+control->common.height)
			return;
		if (!item->isdir)
			text = va("%-32.32s%6iKB", item->name+info->pathlen, item->size/1024);
		else
			text = item->name+info->pathlen;
		if (item == info->selected)
			Draw_AltFunString(x, y, text);
		else
			Draw_FunString(x, y, text);
		y+=8;
		item = item->next;
	}
}
static void ShowDemoMenu (emenu_t *menu, const char *path);
static qboolean M_DemoKey(menucustom_t *control, emenu_t *menu, int key, unsigned int unicode)
{
	demomenu_t *info = menu->data;
	demoitem_t *it;
	int i;

	switch (key)
	{
	case K_MWHEELUP:
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_GP_DPAD_UP:
		if (info->selected && info->selected->prev)
			info->selected = info->selected->prev;
		break;
	case K_MWHEELDOWN:
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_GP_DPAD_DOWN:
		if (info->selected && info->selected->next)
			info->selected = info->selected->next;
		break;
	case K_HOME:
		info->selected = info->items;
		break;
	case K_END:
		info->selected = info->items;
		while(info->selected->next)
			info->selected = info->selected->next;
		break;
	case K_PGUP:
		for (i = 0; i < 10; i++)
		{
			if (info->selected && info->selected->prev)
				info->selected = info->selected->prev;
		}
		break;
	case K_PGDN:
		for (i = 0; i < 10; i++)
		{
			if (info->selected && info->selected->next)
				info->selected = info->selected->next;
		}
		break;
	case K_MOUSE1:	//this is on release
		if (info->dragscroll == 2)
			break;
	case K_TOUCHTAP:
		it = info->firstitem;
		i = (mousecursor_y - control->common.posy) / 8;
		while(i > 0 && it && it->next)
		{
			it = it->next;
			i--;
		}
		if (info->selected != it)
		{
			info->selected = it;
			info->dragscroll = 0;
			break;
		}
		//fallthrough
	case K_ENTER:
	case K_KP_ENTER:
	case K_GP_DIAMOND_CONFIRM:
		if (info->selected)
		{
			if (info->selected->isdir)
				ShowDemoMenu(menu, info->selected->name);
			else if (info->numext)
			{
				extern int		shift_down;
				int extnum;
				const char *ext = COM_GetFileExtension(info->selected->name, NULL);
				if (!Q_strcasecmp(ext, ".gz") || !Q_strcasecmp(ext, ".xz"))
					ext = COM_GetFileExtension(info->selected->name, ext);
				for (extnum = 0; extnum < info->numext; extnum++)
					if (!Q_strcasecmp(ext, info->ext[extnum]))
						break;

				if (extnum == info->numext)	//wasn't on our list of extensions.
					extnum = 0;

				if (!info->command[extnum])
				{	//acceptable archive formats
					ShowDemoMenu(menu, va("%s/", info->selected->name));
					return true;
				}

				Cbuf_AddText(va("%s \"%s%s\"\n", info->command[extnum], (info->fs->fsroot==FS_SYSTEM)?"#":"", info->selected->name), RESTRICT_LOCAL);
				if (!shift_down)
					M_RemoveMenu(menu);
				return true;
			}
		}
		break;
	default:
		return false;
	}
	if (info->selected)
		Q_strncpyz(info->fs->selname, info->selected->name, sizeof(info->fs->selname));
	else
		Q_strncpyz(info->fs->selname, "", sizeof(info->fs->selname));
	return true;
}

static int QDECL DemoAddItem(const char *filename, qofs_t size, time_t modified, void *parm, searchpathfuncs_t *spath)
{
	int extnum;
	demomenu_t *menu = parm;
	demoitem_t *link, *newi;
	int side;
	qboolean isdir;
	char tempfname[MAX_QPATH];

	char *i;

	i = strchr(filename+menu->pathlen, '/');
	if (i == NULL)
	{
		const char *ext = COM_GetFileExtension(filename, NULL);
		if (!Q_strcasecmp(ext, ".gz") || !Q_strcasecmp(ext, ".xz"))
			ext = COM_GetFileExtension(filename, ext);
		for (extnum = 0; extnum < menu->numext; extnum++)
			if (!Q_strcasecmp(ext, menu->ext[extnum]))
				break;

		if (extnum == menu->numext)	//wasn't on our list of extensions.
			return true;
		isdir = false;
	}
	else
	{
		i++;
		if (i-filename > sizeof(tempfname)-2)
			return true;	//too long to fit in our buffers anyway
		strncpy(tempfname, filename, i-filename);
		tempfname[i-filename] = 0;
		filename = tempfname;

		size = 0;
		isdir = true;
	}

	if (!menu->items)
		menu->items = newi = BZ_Malloc(sizeof(*newi) + strlen(filename));
	else
	{
		link = menu->items;
		for(;;)
		{
			if (link->isdir != isdir)	//bias directories, so they sink
				side = (link->isdir > isdir)?1:-1;
			else
				side = Q_strcasecmp(link->name, filename);
			if (side == 0)
				return true;	//already got this file
			else if (side > 0)
			{
				if (!link->prev)
				{
					link->prev = newi = BZ_Malloc(sizeof(*newi) + strlen(filename));
					break;
				}
				link = link->prev;
			}
			else
			{
				if (!link->next)
				{
					link->next = newi = BZ_Malloc(sizeof(*newi) + strlen(filename));
					break;
				}
				link = link->next;
			}
		}
	}
	
	strcpy(newi->name, filename);
	newi->size = size;
	newi->isdir = isdir;
	newi->prev = NULL;
	newi->next = NULL;

	return true;
}

//converts the binary tree into sorted linked list
static void M_Demo_Flatten(demomenu_t *info)
{
	demoitem_t *btree = info->items, *item, *lastitem;
	demoitem_t *listhead = NULL, *listlast = NULL;

	while(btree)
	{
		if (!btree->prev)
		{	//none on left side, descend down right removing head node
			item = btree;
			btree = btree->next;
		}
		else
		{
			item = btree;
			lastitem = item;
			for (;;)
			{
				if (!item->prev)
				{
					lastitem->prev = item->next;
					break;
				}
				lastitem = item;
				item = lastitem->prev;
			}
		}
		if (listlast)
		{
			listlast->next = item;
			item->prev = listlast;
			listlast = item;
		}
		else
		{
			listhead = listlast = item;
			item->prev = NULL;
		}
	}
	if (listlast)
		listlast->next = NULL;
	info->items = listhead;
	info->selected = listhead;
	info->firstitem = listhead;
}

static void M_Demo_Flush (demomenu_t *info)
{
	demoitem_t *item;
	while (info->items)
	{
		item = info->items;
		info->items = item->next;
		BZ_Free(item);
	}
	info->items = NULL;
	info->selected = NULL;
	info->firstitem = NULL;
}

static void M_Demo_Remove (emenu_t *menu)
{
	demomenu_t *info = menu->data;
	M_Demo_Flush(info);

	FS_Manifest_Free(info->man);
	info->man = NULL;
}

static void FS_GameDirPrompted(void *ctx, promptbutton_t btn)
{
	emenu_t *menu = ctx;
	if (Menu_IsLinked(&menu->menu))
	{
		demomenu_t *info = menu->data;
		ftemanifest_t *man = info->man;
		if (!man || info->fs->fsroot != FS_SYSTEM)
			return;	//erk? no exploits!

		switch(btn)
		{
		case PROMPT_CANCEL:
			return;
		case PROMPT_YES:
			info->man = NULL;
			Menu_Unlink(&menu->menu, true);	//try to kill the dialog menu.
			FS_ChangeGame(man, true, true);	//switch to that new gamedir
			break;
		case PROMPT_NO:
			return;
		}
	}
}

static void ShowDemoMenu (emenu_t *menu, const char *path)
{
	demomenu_t *info = menu->data;

	int c;
	char *s;
	char match[256];

	if (path != info->fs->path)
	{
		if (*path == '/' && info->fs->fsroot != FS_SYSTEM)
			path++;
		Q_strncpyz(info->fs->path, path, sizeof(info->fs->path));
	}

	if (info->fs->fsroot == FS_GAME)
	{
		if (!strcmp(path, "../"))
		{
			Q_strncpyz(info->fs->path, "", sizeof(info->fs->path));
			info->fs->fsroot = FS_ROOT;
		}
	}
	else if (info->fs->fsroot == FS_ROOT)
	{
		if (!strcmp(path, "../"))
		{
			FS_SystemPath("", FS_ROOT, info->fs->path, sizeof(info->fs->path));
			Q_strncatz(info->fs->path, "../", sizeof(info->fs->path));
			info->fs->fsroot = FS_SYSTEM;
			while((s = strchr(info->fs->path, '\\')))
				*s = '/';
		}
	}
	while (!strcmp(info->fs->path+strlen(info->fs->path)-3, "../"))
	{
		c = 0;
		for (s = info->fs->path+strlen(info->fs->path)-3; s >= info->fs->path; s--)
		{
			if (*s == '/')
			{
				c++;
				s[1] = '\0';
				if (c == 2)
					break;
			}
		}
		if (c<2)
			*info->fs->path = '\0';
	}
	info->selected = NULL;
	info->pathlen = strlen(info->fs->path);

	M_Demo_Flush(menu->data);
	if (info->fs->fsroot == FS_SYSTEM)
	{
		s = strchr(info->fs->path, '/');
		if (s && strchr(s+1, '/'))
		{
			Q_snprintfz(match, sizeof(match), "%s../", info->fs->path);
			DemoAddItem(match, 0, 0, info, NULL);
		}
	}
	else if (*info->fs->path)
	{
		Q_snprintfz(match, sizeof(match), "%s../", info->fs->path);
		DemoAddItem(match, 0, 0, info, NULL);
	}
	else if (info->fs->fsroot == FS_GAME || info->fs->fsroot == FS_ROOT)
	{
		Q_snprintfz(match, sizeof(match), "../");
		DemoAddItem(match, 0, 0, info, NULL);
	}
	if (info->fs->fsroot == FS_SYSTEM)
	{
		if (*info->fs->path)
			Q_snprintfz(match, sizeof(match), "%s*", info->fs->path);
		else
			Q_snprintfz(match, sizeof(match), "/*");
		Sys_EnumerateFiles("", match, DemoAddItem, info, NULL);
	}
	else if (info->fs->fsroot == FS_ROOT)
	{
		Q_snprintfz(match, sizeof(match), "%s*", info->fs->path);
		if (*com_homepath)
			Sys_EnumerateFiles(com_homepath, match, DemoAddItem, info, NULL);
		Sys_EnumerateFiles(com_gamepath, match, DemoAddItem, info, NULL);
	}
	else
	{
		Q_snprintfz(match, sizeof(match), "%s*", info->fs->path);
		CL_ListFilesInPackage(NULL, match, DemoAddItem, info, NULL);
//		COM_EnumerateFiles(match, DemoAddItem, info);
	}
	M_Demo_Flatten(info);

	if (info->man && FS_DirHasAPackage(info->fs->path, info->man))
	{
		if (promptmenu)
			return	//wut? don't confuse basedirs here...
		Z_Free(info->man->basedir);
		info->man->basedir = Z_StrDup(info->fs->path);
		Menu_Prompt(FS_GameDirPrompted, &menu->menu, va(localtext("Use this directory?\n%s"), info->fs->path), "Yes!", NULL, "No", true);
	}
}
void M_Demo_Reselect(demomenu_t *info, const char *name)
{
	demoitem_t *item;
	for(item = info->items; item; item = item->next)
	{
		if (!strcmp(item->name, name))
		{
			info->selected = item;
			return;
		}
	}
}

void M_Menu_Demos_f (void)
{
	char *demoexts[] = {
		".mvd", ".mvd.gz",
		".qwd", ".qwd.gz",
		//".qwz", ".qwz.gz",
#ifdef NQPROT
		".dem", ".dem.gz",
#endif
#ifdef Q2CLIENT
		".dm2", ".dm2.gz"
#endif
		//there are also qizmo demos (.qwz) out there...
		//we don't support them, but if we were to ask qizmo to decode them for us, we could do.
	};
	char *archiveexts[] = {
#ifdef PACKAGE_PK3
		".zip", ".pk3", ".pk4",
#endif
#ifdef PACKAGE_Q1PAK
		".pak",
#endif
#ifdef PACKAGE_DZIP
		".dz",
#endif
		NULL	//in case none of the above are defined. compilers don't much like 0-length arrays.
	};
	size_t u;
	demomenu_t *info;
	emenu_t *menu;
	static demoloc_t mediareenterloc = {FS_GAME, "demos/"};

	Key_Dest_Remove(kdm_console);

	menu = M_CreateMenu(sizeof(demomenu_t));
	menu->remove = M_Demo_Remove;
	info = menu->data;

	info->fs = &mediareenterloc;

	if (Cmd_Argc()>1)
	{
		char *startdemo = Cmd_Argv(1);
		if (*startdemo == '#')
		{
			startdemo++;
			info->fs->fsroot = FS_SYSTEM;
		}
		else
			info->fs->fsroot = FS_GAME;
		Q_strncpyz(info->fs->path, startdemo, sizeof(info->fs->path));
		*COM_SkipPath(info->fs->path) = 0;
		Q_strncpyz(info->fs->selname, startdemo, sizeof(info->fs->selname));
	}

	info->numext = 0;
	for (u = 0; u < countof(demoexts); u++)
	{
		info->command[info->numext] = "closemenu;playdemo";
		info->ext[info->numext++] = demoexts[u];
	}

	//and some archive formats... for the luls
	for (u = 0; u < countof(archiveexts); u++)
	{
		if (!archiveexts[u])
			continue;
		info->command[info->numext] = NULL;
		info->ext[info->numext++] = archiveexts[u];
	}

	MC_AddWhiteText(menu, 24, 170, 8, localtext("Choose a Demo"), false);
	MC_AddWhiteText(menu, 16, 170, 24, "^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f", false);

	info->list = MC_AddCustom(menu, 0, 32, NULL, 0, NULL);
	info->list->draw = M_DemoDraw;
	info->list->key = M_DemoKey;

	menu->selecteditem = (menuoption_t*)info->list;

	ShowDemoMenu(menu, info->fs->path);
	M_Demo_Reselect(info, info->fs->selname);
}

#ifdef HAVE_JUKEBOX
void M_Menu_MediaFiles_f (void)
{
	demomenu_t *info;
	emenu_t *menu;
	static demoloc_t mediareenterloc = {FS_GAME};

	menu = M_CreateMenu(sizeof(demomenu_t));
	menu->remove = M_Demo_Remove;
	info = menu->data;

	info->fs = &mediareenterloc;
	info->numext = 0;

#ifdef HAVE_JUKEBOX
//	info->ext[info->numext] = ".m3u";
//	info->command[info->numext] = "mediaplaylist";
//	info->numext++;
	info->ext[info->numext] = ".wav";
	info->command[info->numext] = "media_add";
	info->numext++;
#if defined(AVAIL_OGGOPUS) || defined(FTE_TARGET_WEB) || defined(PLUGINS)
	info->ext[info->numext] = ".opus";
	info->command[info->numext] = "media_add";
	info->numext++;
#endif
#if defined(AVAIL_OGGVORBIS) || defined(FTE_TARGET_WEB) || defined(PLUGINS)
	info->ext[info->numext] = ".ogg";
	info->command[info->numext] = "media_add";
	info->numext++;
#endif
#if defined(AVAIL_MP3_ACM) || defined(FTE_TARGET_WEB) || defined(PLUGINS)
	info->ext[info->numext] = ".mp3";
	info->command[info->numext] = "media_add";
	info->numext++;
#endif
#if defined(PLUGINS)
	info->ext[info->numext] = ".flac";
	info->command[info->numext] = "media_add";
	info->numext++;
#endif
#endif

#ifdef HAVE_MEDIA_DECODER
	info->ext[info->numext] = ".roq";
	info->command[info->numext] = "playfilm";
	info->numext++;
#ifdef _WIN32	//avis are only playable on windows due to a windows dll being used to decode them.
	info->ext[info->numext] = ".avi";
	info->command[info->numext] = "playfilm";
	info->numext++;
#endif
#endif

	MC_AddWhiteText(menu, 24, 170, 8, localtext("Media List"), false);
	MC_AddWhiteText(menu, 16, 170, 24, "^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f", false);

	info->list = MC_AddCustom(menu, 0, 32, NULL, 0, NULL);
	info->list->draw = M_DemoDraw;
	info->list->key = M_DemoKey;

	menu->selecteditem = (menuoption_t*)info->list;

	ShowDemoMenu(menu, info->fs->path);
	M_Demo_Reselect(info, info->fs->selname);
}
#endif

#include <stdlib.h>
void M_Menu_BasedirPrompt(ftemanifest_t *man)
{
	demomenu_t *info;
	emenu_t *menu;
	char *start = getenv("HOME");
	size_t l;

	Key_Dest_Remove(kdm_console);

	menu = M_CreateMenu(sizeof(demomenu_t) + sizeof(demoloc_t));
	menu->remove = M_Demo_Remove;
	info = menu->data;

	info->man = man;

	info->fs = (demoloc_t*)(info+1);
	info->fs->fsroot = FS_SYSTEM;
	if (!start || !*start || (l = strlen(info->fs->path))>=sizeof(info->fs->path))
		strcpy(info->fs->path, "/");
	else
		strcpy(info->fs->path, start);
	//make sure it has a trailing slash.
	l = strlen(info->fs->path);
#ifdef _WIN32
	if (info->fs->path[l-1] == '\\')
		info->fs->path[l-1] = '/';
#endif
	if (info->fs->path[l-1] != '/')
	{
		info->fs->path[l] = '/';
		info->fs->path[l+1] = 0;
	}

	info->numext = 0;

	MC_AddWhiteText(menu, 24, 170, 8, va(localtext("Where is %s installed?"), man->formalname), false);
	MC_AddWhiteText(menu, 16, 170, 24, "^Ue01d^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01e^Ue01f", false);

	info->list = MC_AddCustom(menu, 0, 32, NULL, 0, NULL);
	info->list->common.width = 320;
	info->list->draw = M_DemoDraw;
	info->list->key = M_DemoKey;

	menu->selecteditem = (menuoption_t*)info->list;

	ShowDemoMenu(menu, info->fs->path);
	M_Demo_Reselect(info, info->fs->selname);
}

#endif
